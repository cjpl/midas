/********************************************************************\

  Name:         history_schema.cxx
  Created by:   Konstantin Olchanski

  Contents:     Schema based MIDAS history. Available drivers:
                FileHistory: storage of data in binary files (replacement for the traditional MIDAS history)
                MysqlHistory: storage of data in MySQL database (replacement for the ODBC based SQL history)
                SqliteHistory: storage of data in SQLITE3 database (not suitable for production use)

\********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <string>
#include <map>
#include <algorithm>

#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

//
// benchmarks
//
// /usr/bin/time ./linux/bin/mh2sql . /ladd/iris_data2/alpha/alphacpc09-elog-history/history/121019.hst
// -rw-r--r-- 1 alpha users 161028048 Oct 19  2012 /ladd/iris_data2/alpha/alphacpc09-elog-history/history/121019.hst
// flush   10000, sync=OFF    -> 51.51user 1.51system 0:53.76elapsed 98%CPU
// flush 1000000, sync=NORMAL -> 51.83user 2.09system 1:08.37elapsed 78%CPU (flush never activated)
// flush  100000, sync=NORMAL -> 51.38user 1.94system 1:06.94elapsed 79%CPU
// flush   10000, sync=NORMAL -> 51.37user 2.03system 1:31.63elapsed 58%CPU
// flush    1000, sync=NORMAL -> 52.16user 2.70system 4:38.58elapsed 19%CPU

////////////////////////////////////////
//         MIDAS includes             //
////////////////////////////////////////

#include "midas.h"
#include "history.h"

////////////////////////////////////////
//           helper stuff             //
////////////////////////////////////////

#define STRLCPY(dst, src) strlcpy((dst), (src), sizeof(dst))
#define FREE(x) { if (x) free(x); (x) = NULL; }

static std::string TimeToString(time_t t)
{
   const char* sign = "";

   if (t == 0)
      return "0";

   time_t tt = t;

   if (t < 0) {
      sign = "-";
      tt = -t;
   }
      
   assert(tt > 0);

   std::string v;
   while (tt) {
      char c = '0' + tt%10;
      tt /= 10;
      v = c + v;
   }

   v = sign + v;

   //printf("time %.0f -> %s\n", (double)t, v.c_str());

   return v;
}

static std::string SmallIntToString(int i)
{
   //int ii = i;

   if (i == 0)
      return "0";

   assert(i > 0);

   std::string v;
   while (i) {
      char c = '0' + i%10;
      i /= 10;
      v = c + v;
   }

   //printf("SmallIntToString: %d -> %s\n", ii, v.c_str());

   return v;
}

static bool MatchEventName(const char* event_name, const char* var_event_name)
{
   // new-style event name: "equipment_name/variable_name:tag_name"
   // old-style event name: "equipment_name:tag_name" ("variable_name" is missing)
   bool newStyleEventName = (strchr(var_event_name, '/')!=NULL);

   //printf("looking for event_name [%s], try table [%s] event name [%s], new style [%d]\n", var_event_name, table_name, event_name, newStyleEventName);

   if (strcmp(event_name, var_event_name) == 0) {
      return true;
   } else if (newStyleEventName) {
      return false;
   } else { // for old style names, need more parsing
      bool match = false;
         
      const char* s = event_name;
      for (int j=0; s[j]; j++) {
            
         if ((var_event_name[j]==0) && (s[j]=='/')) {
            match = true;
            break;
         }
            
         if ((var_event_name[j]==0) && (s[j]=='_')) {
            match = true;
            break;
         }
            
         if (var_event_name[j]==0) {
            match = false;
            break;
         }
            
         if (tolower(var_event_name[j]) != tolower(s[j])) {
            match = false;
            break;
         }
      }

      return match;
   }
}

static bool MatchTagName(const char* tag_name, const char* var_tag_name, const int var_tag_index)
{
   char alt_tag_name[1024]; // maybe this is an array without "Names"?
   sprintf(alt_tag_name, "%s[%d]", var_tag_name, var_tag_index);
   
   //printf("  looking for tag [%s] alt [%s], try column name [%s]\n", var_tag_name, alt_tag_name, tag_name);
   
   if (strcmp(tag_name, var_tag_name) == 0)
      return true;
   
   if (strcmp(tag_name, alt_tag_name) == 0)
      return true;
   
   return false;
}

static int midas_tid(const char* type_name)
{
   for (int i=0; i<TID_LAST; i++)
      if (strcmp(type_name, rpc_tid_name(i)) == 0)
         return i;
   return 0;
}

////////////////////////////////////////
//         SQL data types             //
////////////////////////////////////////

#ifdef HAVE_SQLITE
static const char *sql_type_sqlite[TID_LAST] = {
   "xxxINVALIDxxxNULL", // TID_NULL
   "INTEGER",           // TID_BYTE
   "INTEGER",           // TID_SBYTE
   "TEXT",              // TID_CHAR
   "INTEGER",           // TID_WORD
   "INTEGER",           // TID_SHORT
   "INTEGER",           // TID_DWORD
   "INTEGER",           // TID_INT
   "INTEGER",           // TID_BOOL
   "REAL",              // TID_FLOAT
   "REAL",              // TID_DOUBLE
   "INTEGER",           // TID_BITFIELD
   "TEXT",              // TID_STRING
   "xxxINVALIDxxxARRAY",
   "xxxINVALIDxxxSTRUCT",
   "xxxINVALIDxxxKEY",
   "xxxINVALIDxxxLINK"
};
#endif

#ifdef HAVE_PGSQL
static const char *sql_type_pgsql[TID_LAST] = {
   "xxxINVALIDxxxNULL", // TID_NULL
   "SMALLINT",  // MYSQL "TINYINT SIGNED", // TID_BYTE
   "SMALLINT",  // MYSQL "TINYINT UNSIGNED",  // TID_SBYTE
   "CHAR(1)",   // TID_CHAR
   "SMALLINT",  // MYSQL "SMALLINT UNSIGNED ", // TID_WORD
   "SMALLINT",  // MYSQL "SMALLINT SIGNED ", // TID_SHORT
   "INTEGER",   // MYSQL "INT UNSIGNED ", // TID_DWORD
   "INTEGER",   // MYSQL "INT SIGNED ", // TID_INT
   "BOOL",      // TID_BOOL
   "FLOAT(53)", // MYSQL "DOUBLE" TID_FLOAT
   "FLOAT(53)", // MYSQL "DOUBLE" TID_DOUBLE
   "SMALLINT",  // MYSQL "TINYINT UNSIGNED", // TID_BITFIELD
   "VARCHAR",   // TID_STRING
   "xxxINVALIDxxxARRAY",
   "xxxINVALIDxxxSTRUCT",
   "xxxINVALIDxxxKEY",
   "xxxINVALIDxxxLINK"
};
#endif

#ifdef HAVE_MYSQL
static const char *sql_type_mysql[TID_LAST] = {
   "xxxINVALIDxxxNULL", // TID_NULL
   "tinyint unsigned",  // TID_BYTE
   "tinyint",           // TID_SBYTE
   "char",              // TID_CHAR
   "smallint unsigned", // TID_WORD
   "smallint",          // TID_SHORT
   "integer unsigned",  // TID_DWORD
   "integer",           // TID_INT
   "tinyint",           // TID_BOOL
   "float",             // TID_FLOAT
   "double",            // TID_DOUBLE
   "tinyint unsigned",  // TID_BITFIELD
   "VARCHAR",           // TID_STRING
   "xxxINVALIDxxxARRAY",
   "xxxINVALIDxxxSTRUCT",
   "xxxINVALIDxxxKEY",
   "xxxINVALIDxxxLINK"
};
#endif

#if 0
static int sql2midasType_mysql(const char* name)
{
   for (int tid=0; tid<TID_LAST; tid++)
      if (strcasecmp(name, sql_type_mysql[tid])==0)
         return tid;
   // FIXME!
   printf("sql2midasType: Cannot convert SQL data type \'%s\' to a MIDAS data type!\n", name);
   return 0;
}
#endif

#if 0
static int sql2midasType_sqlite(const char* name)
{
   if (strcmp(name, "INTEGER") == 0)
      return TID_INT;
   if (strcmp(name, "REAL") == 0)
      return TID_DOUBLE;
   if (strcmp(name, "TEXT") == 0)
      return TID_STRING;
   // FIXME!
   printf("sql2midasType: Cannot convert SQL data type \'%s\' to a MIDAS data type!\n", name);
   return 0;
}
#endif

////////////////////////////////////////
//        Schema base classes         //
////////////////////////////////////////

struct HsSchemaEntry {
   std::string name;
   int type;
   int n_data;
   int n_bytes;

   HsSchemaEntry() // ctor
   {
      type = 0;
      n_data = 0;
      n_bytes = 0;
   }
};

struct HsSchema
{
   bool active;
   std::string event_name;
   time_t time_from;
   time_t time_to;
   std::vector<HsSchemaEntry> variables;
   std::vector<int> offsets;

   HsSchema() // ctor
   {
      active = false;
      time_from = 0;
      time_to = 0;
   }

   virtual void print(bool print_tags = true) const;
   virtual ~HsSchema() { }; // dtor
   virtual int flush_buffers() = 0;
   virtual int close() = 0;
   virtual int write_event(const time_t t, const char* data, const int data_size) = 0;
   virtual int match_event_var(const char* event_name, const char* var_name, const int var_index);
   virtual int read_last_written(const time_t timestamp,
                                 const int debug,
                                 time_t* last_written) = 0;
   virtual int read_data(const time_t start_time,
                         const time_t end_time,
                         const int num_var, const int var_schema_index[],
                         const int debug,
                         MidasHistoryBufferInterface* buffer[]) = 0;
};

class HsSchemaVector
{
protected:
   std::vector<HsSchema*> data;

public:
   ~HsSchemaVector() { // dtor
      clear();
   }

   HsSchema* operator[](int index) const {
      return data[index];
   }

   unsigned size() const {
      return data.size();
   }

   void add(HsSchema* s);

   void clear() {
      for (unsigned i=0; i<data.size(); i++)
         delete data[i];
      data.clear();
   }

   void print(bool print_tags = true) const {
      for (unsigned i=0; i<data.size(); i++)
         data[i]->print(print_tags);
   }

   HsSchema* find_event(const char* event_name, const time_t timestamp);
};

////////////////////////////////////////////
//        Base class functions            //
////////////////////////////////////////////

void HsSchemaVector::add(HsSchema* s)
{
   time_t last_time_from = 0;
   std::vector<HsSchema*>::iterator it_last = data.end();

   for (std::vector<HsSchema*>::iterator it = data.begin(); it != data.end(); it++) {
      if ((*it)->event_name == s->event_name) {
         if (s->time_from == (*it)->time_from) {
            s->time_to = (*it)->time_to;
            delete (*it);
            (*it) = s;
            return;
         }

         if (s->time_from > (*it)->time_from) {
            (*it)->time_to = s->time_from;
            data.insert(it, s);
            return;
         }

         last_time_from = (*it)->time_from;
         it_last = it+1;
      }
   }
   s->time_to = last_time_from;
   data.insert(it_last, s);
}

HsSchema* HsSchemaVector::find_event(const char* event_name, time_t t)
{
   HsSchema* ss = NULL;

   if (0) {
      printf("find_event: All schema for event %s: (total %d)\n", event_name, (int)data.size());
      int found = 0;
      for (unsigned i=0; i<data.size(); i++) {
         HsSchema* s = data[i];
         if (s->event_name != event_name)
            continue;
         s->print();
         found++;
      }
      printf("find_event: Found %d schemas for event %s\n", found, event_name);
   }

   for (unsigned i=0; i<data.size(); i++) {
      HsSchema* s = data[i];

      // wrong event
      if (s->event_name != event_name)
         continue;

      // schema is from after the time we are looking for
      if (s->time_from > t)
         continue;

      if (!ss)
         ss = s;

      // remember the newest schema
      if (s->time_from > ss->time_from)
         ss = s;
   }

   if (0 && ss) {
      printf("find_event: for time %s, returning:\n", TimeToString(t).c_str());
      ss->print();
   }

   return ss;
}

////////////////////////////////////////////
//        Sql interface class             //
////////////////////////////////////////////

class SqlBase
{
public:
   int  fDebug;
   bool fIsConnected;

   SqlBase() {  // ctor
      fDebug = 0;
      fIsConnected = false;
   };

   virtual ~SqlBase() { // dtor
      // confirm that the destructor of the concrete class
      // disconnected the database
      assert(!fIsConnected);
      fDebug = 0;
      fIsConnected = false;
   }

   virtual int Connect(const char* path) = 0;
   virtual int Disconnect() = 0;
   virtual bool IsConnected() = 0;

   virtual int ListTables(std::vector<std::string> *plist) = 0;
   virtual int ListColumns(const char* table_name, std::vector<std::string> *plist) = 0;

   // sql commands
   virtual int Exec(const char* table_name, const char* sql) = 0;

   // queries
   virtual int Prepare(const char* table_name, const char* sql) = 0;
   virtual int Step() = 0;
   virtual const char* GetText(int column) = 0;
   virtual time_t      GetTime(int column) = 0;
   virtual double      GetDouble(int column) = 0;
   virtual int Finalize() = 0;

   // transactions
   virtual int OpenTransaction(const char* table_name) = 0;
   virtual int CommitTransaction(const char* table_name) = 0;
   virtual int RollbackTransaction(const char* table_name) = 0;

   // data types
   virtual const char* ColumnType(int midas_tid) = 0;
   virtual bool TypesCompatible(int midas_tid, const char* sql_type) = 0;
};

////////////////////////////////////////////
//        Schema concrete classes         //
////////////////////////////////////////////

struct HsSqlSchema : public HsSchema {
   SqlBase* sql;
   int transaction_count;
   std::string table_name;
   std::vector<std::string> column_names;
   std::vector<std::string> column_types;

   HsSqlSchema() // ctor
   {
      transaction_count = 0;
   }

   ~HsSqlSchema() // dtor
   {
      assert(transaction_count == 0);
   }

   void print(bool print_tags = true) const;
   int close_transaction();
   int flush_buffers() { return close_transaction(); }
   int close() { return close_transaction(); }
   int write_event(const time_t t, const char* data, const int data_size);
   int match_event_var(const char* event_name, const char* var_name, const int var_index);
   int read_last_written(const time_t timestamp,
                         const int debug,
                         time_t* last_written);
   int read_data(const time_t start_time,
                 const time_t end_time,
                 const int num_var, const int var_schema_index[],
                 const int debug,
                 MidasHistoryBufferInterface* buffer[]);
};

struct HsFileSchema : public HsSchema {
   std::string file_name;
   int record_size;
   int data_offset;
   int last_size;
   int writer_fd;

   HsFileSchema() // ctor
   {
      record_size = 0;
      data_offset = 0;
      last_size = 0;
      writer_fd = -1;
   }

   ~HsFileSchema() // dtor
   {
      close();
      record_size = 0;
      data_offset = 0;
      last_size = 0;
      writer_fd = -1;
   }

   void print(bool print_tags = true) const;
   int flush_buffers() { return HS_SUCCESS; };
   int close();
   int write_event(const time_t t, const char* data, const int data_size);
   int read_last_written(const time_t timestamp,
                         const int debug,
                         time_t* last_written);
   int read_data(const time_t start_time,
                 const time_t end_time,
                 const int num_var, const int var_schema_index[],
                 const int debug,
                 MidasHistoryBufferInterface* buffer[]);
};

////////////////////////////////////////////
//        Print functions                 //
////////////////////////////////////////////

void HsSchema::print(bool print_tags) const
{
   int nv = this->variables.size();
   printf("event [%s], time %s..%s, variables [%d]\n", this->event_name.c_str(), TimeToString(this->time_from).c_str(), TimeToString(this->time_to).c_str(), nv);
   if (print_tags)
      for (unsigned j=0; j<nv; j++)
         printf("  %d: name [%s], type [%s] tid %d, n_data %d, n_bytes %d, offset %d\n", j, this->variables[j].name.c_str(), rpc_tid_name(this->variables[j].type), this->variables[j].type, this->variables[j].n_data, this->variables[j].n_bytes, this->offsets[j]);
};

void HsSqlSchema::print(bool print_tags) const
{
   int nv = this->variables.size();
   printf("event [%s], sql_table [%s], time %s..%s, variables [%d]\n", this->event_name.c_str(), this->table_name.c_str(), TimeToString(this->time_from).c_str(), TimeToString(this->time_to).c_str(), nv);
   if (print_tags)
      for (unsigned j=0; j<nv; j++) {
         printf("  %d: name [%s], type [%s] tid %d, n_data %d, n_bytes %d", j, this->variables[j].name.c_str(), rpc_tid_name(this->variables[j].type), this->variables[j].type, this->variables[j].n_data, this->variables[j].n_bytes);
         printf(", sql_column [%s], sql_type [%s], offset %d", this->column_names[j].c_str(), this->column_types[j].c_str(), this->offsets[j]);
         printf("\n");
      }
}

void HsFileSchema::print(bool print_tags) const
{
   int nv = this->variables.size();
   printf("event [%s], file_name [%s], time %s..%s, variables [%d]\n", this->event_name.c_str(), this->file_name.c_str(), TimeToString(this->time_from).c_str(), TimeToString(this->time_to).c_str(), nv);
   if (print_tags) {
      for (unsigned j=0; j<nv; j++)
         printf("  %d: name [%s], type [%s] tid %d, n_data %d, n_bytes %d, offset %d\n", j, this->variables[j].name.c_str(), rpc_tid_name(this->variables[j].type), this->variables[j].type, this->variables[j].n_data, this->variables[j].n_bytes, this->offsets[j]);
      printf("  record_size: %d, data_offset: %d\n", this->record_size, this->data_offset);
   }
}

////////////////////////////////////////////
//        File functions                  //
////////////////////////////////////////////

#ifdef HAVE_MYSQL

////////////////////////////////////////
//     MYSQL/MariaDB database access  //
////////////////////////////////////////

#include <my_global.h>
#include <mysql.h>

class Mysql: public SqlBase
{
public:
   MYSQL* fMysql;
   MYSQL_RES* fResult;
   MYSQL_ROW  fRow;
   int fNumFields;

   Mysql(); // ctor
   ~Mysql(); // dtor

   int Connect(const char* path);
   int Disconnect();
   bool IsConnected();

   int ConnectTable(const char* table_name);

   int ListTables(std::vector<std::string> *plist);
   int ListColumns(const char* table_name, std::vector<std::string> *plist);

   int Exec(const char* table_name, const char* sql);

   int Prepare(const char* table_name, const char* sql);
   int Step();
   const char* GetText(int column);
   time_t      GetTime(int column);
   double      GetDouble(int column);
   int Finalize();

   int OpenTransaction(const char* table_name);
   int CommitTransaction(const char* table_name);
   int RollbackTransaction(const char* table_name);

   const char* ColumnType(int midas_tid);
   bool TypesCompatible(int midas_tid, const char* sql_type);
};

Mysql::Mysql() // ctor
{
   fMysql = NULL;
   fResult = NULL;
   fRow = NULL;
   fNumFields = 0;
}

Mysql::~Mysql() // dtor
{
   Disconnect();
   fMysql = NULL;
   fResult = NULL;
   fRow = NULL;
   fNumFields = 0;
}

int Mysql::Connect(const char* connect_string)
{
   if (!fMysql) {
      fMysql = mysql_init(NULL);
      if (!fMysql) {
         return DB_FILE_ERROR;
      }
   }

   const char* host_name = "localhost";
   const char* user_name = "root";
   const char* user_password = "xxx";
   const char* db_name = "history";
   int tcp_port = 0;
   const char* unix_socket = NULL;
   int client_flag = 0|CLIENT_IGNORE_SIGPIPE;

   if (mysql_real_connect(fMysql, host_name, user_name, user_password, db_name, tcp_port, unix_socket, client_flag) == NULL) {
      fprintf(stderr, "%s\n", mysql_error(fMysql));
      Disconnect();
      return DB_FILE_ERROR;
   }  

   // FIXME:
   //my_bool reconnect = 0;
   //mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);

   //if (mysql_query(fMysql, "CREATE DATABASE testdb")) {
   //fprintf(stderr, "%s\n", mysql_error(fMysql));
   //Disconnect();
   //return DB_FILE_ERROR;
   //}

   fIsConnected = true;
   return DB_SUCCESS;
}

int Mysql::Disconnect()
{
   if (fRow) {
      // FIXME: mysql_free_result(fResult);
   }

   if (fResult)
      mysql_free_result(fResult);

   if (fMysql)
      mysql_close(fMysql);

   fMysql = NULL;
   fResult = NULL;
   fRow = NULL;

   fIsConnected = false;
   return DB_SUCCESS;
}

bool Mysql::IsConnected()
{
   return fIsConnected;
}

int Mysql::OpenTransaction(const char* table_name)
{
   // FIXME
   // Exec(table_name, "START TRANSACTION");
   return DB_SUCCESS;
}

int Mysql::CommitTransaction(const char* table_name)
{
   // FIXME
   // Exec(table_name, "COMMIT");
   return DB_SUCCESS;
}

int Mysql::RollbackTransaction(const char* table_name)
{
   // FIXME
   // Exec(table_name, "ROLLBACK");
   return DB_SUCCESS;
}

int Mysql::ListTables(std::vector<std::string> *plist)
{
   // FIXME
   //fResult = mysql_list_tables(fMysql, NULL);
   //while (1) { if (Step()!=DB_SUCCESS) break; GetText(0); };
   //Finalize();
   return DB_SUCCESS;
}

int Mysql::ListColumns(const char* table_name, std::vector<std::string> *plist)
{
   // FIXME
   //fResult = mysql_list_fields(fMysql, table_name, NULL);
   //while (1) { if (Step()!=DB_SUCCESS) break; GetText(0); };
   //Finalize();
   return DB_SUCCESS;
}

int Mysql::Exec(const char* table_name, const char* sql)
{
   assert(fMysql);
   assert(fResult == NULL); // there should be no unfinalized queries
   assert(fRow == NULL);

   if (mysql_query(fMysql, sql)) {
      // FIXME finish_with_error(con);
      return DB_FILE_ERROR;
   }

   return DB_SUCCESS;
}

int Mysql::Prepare(const char* table_name, const char* sql)
{
   assert(fMysql);
   assert(fResult == NULL); // there should be no unfinalized queries
   assert(fRow == NULL);

   if (mysql_query(fMysql, sql)) {
      // FIXME: finish_with_error(con);
      return DB_FILE_ERROR;
   }

   fResult = mysql_store_result(fMysql);
   //fResult = mysql_use_result(fMysql); // cannot use this because it blocks writing into table
  
   if (!fResult) {
      // FIXME: finish_with_error(con);
      return DB_FILE_ERROR;
   }

   fNumFields = mysql_num_fields(fResult);

   return DB_SUCCESS;
}

int Mysql::Step()
{
   assert(fMysql);
   assert(fResult);

   fRow = mysql_fetch_row(fResult);

   if (fRow)
      return DB_SUCCESS;

   // FIXME: check for errors

   return DB_NO_MORE_SUBKEYS;
}

const char* Mysql::GetText(int column)
{
   assert(fMysql);
   assert(fResult);
   assert(fRow);
   assert(fNumFields > 0);
   assert(column >= 0);
   assert(column < fNumFields);
   
   return fRow[column];
}

double Mysql::GetDouble(int column)
{
   return atof(GetText(column));
}

time_t Mysql::GetTime(int column)
{
   return strtoul(GetText(column), NULL, 0);
}

int Mysql::Finalize()
{
   assert(fMysql);
   assert(fResult);

   mysql_free_result(fResult);
   fResult = NULL;
   fRow = NULL;
   fNumFields = 0;

   return DB_SUCCESS;
}

const char* Mysql::ColumnType(int midas_tid)
{
   assert(midas_tid>=0);
   assert(midas_tid<TID_LAST);
   return sql_type_mysql[midas_tid];
}

bool Mysql::TypesCompatible(int midas_tid, const char* sql_type)
{
   if (0)
      printf("compare types midas \'%s\'=\'%s\' and sql \'%s\'\n", rpc_tid_name(midas_tid), ColumnType(midas_tid), sql_type);

   //if (sql2midasType_mysql(sql_type) == midas_tid)
   //   return true;

   if (strcasecmp(ColumnType(midas_tid), sql_type) == 0)
      return true;

   // permit writing FLOAT into DOUBLE
   if (midas_tid==TID_FLOAT && strcmp(sql_type, "double")==0)
      return true;

   // T2K quirk!
   // permit writing BYTE into signed tinyint
   if (midas_tid==TID_BYTE && strcmp(sql_type, "tinyint")==0)
      return true;

   // T2K quirk!
   // permit writing WORD into signed tinyint
   if (midas_tid==TID_WORD && strcmp(sql_type, "tinyint")==0)
      return true;

   return false;
}

#endif // HAVE_MYSQL

#ifdef HAVE_SQLITE

////////////////////////////////////////
//        SQLITE database access      //
////////////////////////////////////////

#include <sqlite3.h>

typedef std::map<std::string, sqlite3*> DbMap;

class Sqlite: public SqlBase
{
public:
   std::string fPath;

   DbMap fMap;

   // temporary storage of query data
   sqlite3* fTempDB;
   sqlite3_stmt* fTempStmt;

   Sqlite(); // ctor
   ~Sqlite(); // dtor

   int Connect(const char* path);
   int Disconnect();
   bool IsConnected();

   int ConnectTable(const char* table_name);
   sqlite3* GetTable(const char* table_name);

   int ListTables(std::vector<std::string> *plist);
   int ListColumns(const char* table_name, std::vector<std::string> *plist);

   int Exec(const char* table_name, const char* sql);

   int Prepare(const char* table_name, const char* sql);
   int Step();
   const char* GetText(int column);
   time_t      GetTime(int column);
   double      GetDouble(int column);
   int Finalize();

   int OpenTransaction(const char* table_name);
   int CommitTransaction(const char* table_name);
   int RollbackTransaction(const char* table_name);

   const char* ColumnType(int midas_tid);
   bool TypesCompatible(int midas_tid, const char* sql_type);
};

const char* Sqlite::ColumnType(int midas_tid)
{
   assert(midas_tid>=0);
   assert(midas_tid<TID_LAST);
   return sql_type_sqlite[midas_tid];
}

bool Sqlite::TypesCompatible(int midas_tid, const char* sql_type)
{
   if (0)
      printf("compare types midas \'%s\'=\'%s\' and sql \'%s\'\n", rpc_tid_name(midas_tid), ColumnType(midas_tid), sql_type);

   //if (sql2midasType_sqlite(sql_type) == midas_tid)
   //   return true;

   if (strcasecmp(ColumnType(midas_tid), sql_type) == 0)
      return true;

   // permit writing FLOAT into DOUBLE
   if (midas_tid==TID_FLOAT && strcasecmp(sql_type, "double")==0)
      return true;

   return false;
}

const char* Sqlite::GetText(int column)
{
   return (const char*)sqlite3_column_text(fTempStmt, column);
}

time_t Sqlite::GetTime(int column)
{
   return sqlite3_column_int64(fTempStmt, column);
}

double Sqlite::GetDouble(int column)
{
   return sqlite3_column_double(fTempStmt, column);
}

Sqlite::Sqlite() // ctor
{
   fIsConnected = false;
   fTempDB = NULL;
   fTempStmt = NULL;
   fDebug = 0;
}

Sqlite::~Sqlite() // dtor
{
   Disconnect();
}

const char* xsqlite3_errstr(sqlite3* db, int errcode)
{
   //return sqlite3_errstr(errcode);
   return sqlite3_errmsg(db);
}

int Sqlite::ConnectTable(const char* table_name)
{
   std::string fname = fPath + "/" + "mh_" + table_name + ".sqlite3";

   sqlite3* db = NULL;

   int status = sqlite3_open(fname.c_str(), &db);

   if (status != SQLITE_OK) {
      cm_msg(MERROR, "Sqlite::Connect", "Table %s: sqlite3_open(%s) error %d (%s)", table_name, fname.c_str(), status, xsqlite3_errstr(db, status));
      sqlite3_close(db);
      db = NULL;
      return DB_FILE_ERROR;
   }

#if SQLITE_VERSION_NUMBER >= 3006020
   status = sqlite3_extended_result_codes(db, 1);
   if (status != SQLITE_OK) {
      cm_msg(MERROR, "Sqlite::Connect", "Table %s: sqlite3_extended_result_codes(1) error %d (%s)", table_name, status, xsqlite3_errstr(db, status));
   }
#else
#warning Missing sqlite3_extended_result_codes()!
#endif

   fMap[table_name] = db;

   Exec(table_name, "PRAGMA journal_mode=persist;");
   Exec(table_name, "PRAGMA synchronous=normal;");
   //Exec(table_name, "PRAGMA synchronous=off;");
   Exec(table_name, "PRAGMA journal_size_limit=-1;");

   if (0) {
      Exec(table_name, "PRAGMA legacy_file_format;");
      Exec(table_name, "PRAGMA synchronous;");
      Exec(table_name, "PRAGMA journal_mode;");
      Exec(table_name, "PRAGMA journal_size_limit;");
   }

   if (0) {
      int max_columns = sqlite3_limit(db, SQLITE_LIMIT_COLUMN, -1);
      printf("Sqlite::Connect: SQLITE_LIMIT_COLUMN=%d\n", max_columns);
   }

   if (fDebug)
      cm_msg(MINFO, "Sqlite::Connect", "Table %s: connected to Sqlite file \'%s\'", table_name, fname.c_str());

   return DB_SUCCESS;
}

sqlite3* Sqlite::GetTable(const char* table_name)
{
   sqlite3* db = fMap[table_name];

   if (db)
      return db;

   int status = ConnectTable(table_name);
   if (status != DB_SUCCESS)
      return NULL;

   return fMap[table_name];
}

int Sqlite::Connect(const char* path)
{
   if (fIsConnected)
      Disconnect();

   fPath = path;

   if (fDebug)
      cm_msg(MINFO, "Sqlite::Connect", "Connected to Sqlite database in \'%s\'", path);

   fIsConnected = true;

   return DB_SUCCESS;
}

int Sqlite::Disconnect()
{
   if (!fIsConnected)
      return DB_SUCCESS;

   for (DbMap::iterator iter = fMap.begin(); iter != fMap.end(); ++iter) {
      const char* table_name = iter->first.c_str();
      sqlite3* db = iter->second;
      int status = sqlite3_close(db);
      if (status != SQLITE_OK) {
         cm_msg(MERROR, "Sqlite::Disconnect", "sqlite3_close(%s) error %d (%s)", table_name, status, xsqlite3_errstr(db, status));
      }
   }

   fMap.clear();

   fIsConnected = false;

   return DB_SUCCESS;
}

bool Sqlite::IsConnected()
{
   return fIsConnected;
}

int Sqlite::OpenTransaction(const char* table_name)
{
   int status = Exec(table_name, "BEGIN TRANSACTION");
   return status;
}

int Sqlite::CommitTransaction(const char* table_name)
{
   int status = Exec(table_name, "COMMIT TRANSACTION");
   return status;
}

int Sqlite::RollbackTransaction(const char* table_name)
{
   int status = Exec(table_name, "ROLLBACK TRANSACTION");
   return status;
}

int Sqlite::Prepare(const char* table_name, const char* sql)
{
   sqlite3* db = GetTable(table_name);
   if (!db)
      return DB_FILE_ERROR;

   if (fDebug)
      printf("Sqlite::Prepare(%s, %s)\n", table_name, sql);

   assert(fTempDB==NULL);
   fTempDB = db;

#if SQLITE_VERSION_NUMBER >= 3006020
   int status = sqlite3_prepare_v2(db, sql, strlen(sql), &fTempStmt, NULL);
#else
#warning Missing sqlite3_prepare_v2()!
   int status = sqlite3_prepare(db, sql, strlen(sql), &fTempStmt, NULL);
#endif

   if (status == SQLITE_OK)
      return DB_SUCCESS;

   std::string sqlstring = sql;
   cm_msg(MERROR, "Sqlite::Prepare", "Table %s: sqlite3_prepare_v2(%s...) error %d (%s)", table_name, sqlstring.substr(0,60).c_str(), status, xsqlite3_errstr(db, status));

   fTempDB = NULL;

   return DB_FILE_ERROR;
}

int Sqlite::Step()
{
   if (0 && fDebug)
      printf("Sqlite::Step()\n");

   assert(fTempDB);
   assert(fTempStmt);

   int status = sqlite3_step(fTempStmt);

   if (status == SQLITE_DONE)
      return DB_NO_MORE_SUBKEYS;

   if (status == SQLITE_ROW)
      return DB_SUCCESS;

   cm_msg(MERROR, "Sqlite::Step", "sqlite3_step() error %d (%s)", status, xsqlite3_errstr(fTempDB, status));

   return DB_FILE_ERROR;
}

int Sqlite::Finalize()
{
   if (0 && fDebug)
      printf("Sqlite::Finalize()\n");

   assert(fTempDB);
   assert(fTempStmt);

   int status = sqlite3_finalize(fTempStmt);

   if (status != SQLITE_OK) {
      cm_msg(MERROR, "Sqlite::Finalize", "sqlite3_finalize() error %d (%s)", status, xsqlite3_errstr(fTempDB, status));

      fTempDB = NULL;
      fTempStmt = NULL; // FIXME: maybe a memory leak?
      return DB_FILE_ERROR;
   }

   fTempDB = NULL;
   fTempStmt = NULL;

   return DB_SUCCESS;
}

int Sqlite::ListTables(std::vector<std::string> *plist)
{
   if (!fIsConnected)
      return DB_FILE_ERROR;

   if (fDebug)
      printf("Sqlite::ListTables at path [%s]\n", fPath.c_str());

   int status;

   const char* cmd = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";

   DIR *dir = opendir(fPath.c_str());
   if (!dir) {
      cm_msg(MERROR, "Sqlite::ListTables", "Cannot opendir(%s), errno %d (%s)", fPath.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   while (1) {
      const struct dirent* de = readdir(dir);
      if (!de)
         break;

      const char* dn = de->d_name;

      //if (dn[0]!='m' || dn[1]!='h')
      //continue;

      const char* s;

      s = strstr(dn, "mh_");
      if (!s || s!=dn)
         continue;

      s = strstr(dn, ".sqlite3");
      if (!s || s[8]!=0)
         continue;

      char table_name[256];
      strlcpy(table_name, dn+3, sizeof(table_name));
      char* ss = strstr(table_name, ".sqlite3");
      if (!ss)
         continue;
      *ss = 0;

      //printf("dn [%s] tn [%s]\n", dn, table_name);

      status = Prepare(table_name, cmd);
      if (status != DB_SUCCESS)
         continue;
      
      while (1) {
         status = Step();
         if (status != DB_SUCCESS)
            break;
         
         const char* tablename = GetText(0);
         //printf("table [%s]\n", tablename);
         plist->push_back(tablename);
      }
      
      status = Finalize();
   }

   closedir(dir);
   dir = NULL;

   return DB_SUCCESS;
}

int Sqlite::ListColumns(const char* table, std::vector<std::string> *plist)
{
   if (!fIsConnected)
      return DB_FILE_ERROR;

   if (fDebug)
      printf("Sqlite::ListColumns for table \'%s\'\n", table);

   std::string cmd;
   cmd = "PRAGMA table_info(";
   cmd += table;
   cmd += ");";

   int status;

   //status = Exec(cmd);

   status = Prepare(table, cmd.c_str());
   if (status != DB_SUCCESS)
      return status;

   while (1) {
      status = Step();
      if (status != DB_SUCCESS)
         break;

      const char* colname = GetText(1);
      const char* coltype = GetText(2);
      //printf("column [%s] [%s]\n", colname, coltype);
      plist->push_back(colname); // column name
      plist->push_back(coltype); // column type
   }

   status = Finalize();

   return DB_SUCCESS;
}

static int callback_debug = 0;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   if (callback_debug) {
      printf("history_sqlite::callback---->\n");
      for (int i=0; i<argc; i++){
         printf("history_sqlite::callback[%d] %s = %s\n", i, azColName[i], argv[i] ? argv[i] : "NULL");
      }
   }
   return 0;
}

int Sqlite::Exec(const char* table_name, const char* sql)
{
   // return values:
   // DB_SUCCESS
   // DB_FILE_ERROR: not connected
   // DB_NO_KEY: "table not found"
   // DB_KEY_EXIST: "table already exists"

   if (!fIsConnected)
      return DB_FILE_ERROR;

   sqlite3* db = GetTable(table_name);
   if (!db)
      return DB_FILE_ERROR;

   if (fDebug)
      printf("Sqlite::Exec(%s, %s)\n", table_name, sql);

   int status;

   callback_debug = fDebug;
   char* errmsg = NULL;

   status = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if (status != SQLITE_OK) {
      if (status == SQLITE_ERROR && strstr(errmsg, "duplicate column name"))
         return DB_KEY_EXIST;
      std::string sqlstring = sql;
      cm_msg(MERROR, "Sqlite::Exec", "Table %s: sqlite3_exec(%s...) error %d (%s)", table_name, sqlstring.substr(0,60).c_str(), status, errmsg);
      sqlite3_free(errmsg);
      return DB_FILE_ERROR;
   }

   return DB_SUCCESS;
}

#endif // HAVE_SQLITE

////////////////////////////////////////////////////////
//  Data structures to keep track of Events and Tags  //
////////////////////////////////////////////////////////

static void PrintTags(int ntags, const TAG tags[])
{
   for (int i=0; i<ntags; i++)
      printf("tag %d: %s %s[%d]\n", i, rpc_tid_name(tags[i].type), tags[i].name, tags[i].n_data);
}

// convert MIDAS names to SQL names

static std::string MidasNameToSqlName(const char* s)
{
   std::string out;

   for (int i=0; s[i]!=0; i++) {
      char c = s[i];
      if (isalpha(c) || isdigit(c))
         out += tolower(c);
      else
         out += '_';
   }
   
   return out;
}

static std::string MidasNameToFileName(const char* s)
{
   std::string out;

   for (int i=0; s[i]!=0; i++) {
      char c = s[i];
      if (isalpha(c) || isdigit(c))
         out += tolower(c);
      else
         out += '_';
   }
   
   return out;
}

////////////////////////////////////
//    Methods of HsFileSchema     //
////////////////////////////////////

int HsFileSchema::write_event(const time_t t, const char* data, const int data_size)
{
   HsFileSchema* s = this;

   int status;

   if (s->writer_fd < 0) {
      s->writer_fd = open(s->file_name.c_str(), O_RDWR);
      if (s->writer_fd < 0) {
         cm_msg(MERROR, "FileHistory::write_event", "Cannot write to \'%s\', open() errno %d (%s)", s->file_name.c_str(), errno, strerror(errno));
         return HS_FILE_ERROR;
      }

      int file_size = lseek(s->writer_fd, 0, SEEK_END);
         
      int nrec = (file_size - s->data_offset)/s->record_size;
      if (nrec < 0)
         nrec = 0;
      int data_end = s->data_offset + nrec*s->record_size;

      //printf("file_size %d, nrec %d, data_end %d\n", file_size, nrec, data_end);

      if (data_end != file_size) {
         if (nrec > 0)
            cm_msg(MERROR, "FileHistory::write_event", "File \'%s\' may be truncated, data offset %d, record size %d, file size: %d, should be %d, truncating the file", s->file_name.c_str(), s->data_offset, s->record_size, file_size, data_end);

         status = lseek(s->writer_fd, data_end, SEEK_SET);
         if (status < 0) {
            cm_msg(MERROR, "FileHistory::write_event", "Cannot seek \'%s\' to offset %d, lseek() errno %d (%s)", s->file_name.c_str(), data_end, errno, strerror(errno));
            return HS_FILE_ERROR;
         }
         status = ftruncate(s->writer_fd, data_end);
         if (status < 0) {
            cm_msg(MERROR, "FileHistory::write_event", "Cannot truncate \'%s\' to size %d, ftruncate() errno %d (%s)", s->file_name.c_str(), data_end, errno, strerror(errno));
            return HS_FILE_ERROR;
         }
      }
   }

   int expected_size = s->record_size - 4;

   if (s->last_size == 0)
      s->last_size = expected_size;

   if (data_size != s->last_size) {
      cm_msg(MERROR, "FileHistory::write_event", "Event \'%s\' data size mismatch, expected %d bytes, got %d bytes, previously %d bytes", s->event_name.c_str(), expected_size, data_size, s->last_size);
      //printf("schema:\n");
      //s->print();

      if (data_size < expected_size)
         return HS_FILE_ERROR;

      // truncate for now
      // data_size = expected_size;
      s->last_size = data_size;
   }

   status = write(s->writer_fd, &t, 4);
   if (status != 4) {
      cm_msg(MERROR, "FileHistory::write_event", "Cannot write to \'%s\', write(timestamp) errno %d (%s)", s->file_name.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   status = write(s->writer_fd, data, expected_size);
   if (status != expected_size) {
      cm_msg(MERROR, "FileHistory::write_event", "Cannot write to \'%s\', write(%d) errno %d (%s)", s->file_name.c_str(), data_size, errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   return HS_SUCCESS;
}

int HsFileSchema::close()
{
   if (writer_fd >= 0)
      ::close(writer_fd);
   writer_fd = -1;
   return HS_SUCCESS;
}

static int ReadRecord(const char* file_name, int fd, int offset, int recsize, int irec, char* rec)
{
   int status;
   int fpos = offset + irec*recsize;

   status = ::lseek(fd, fpos, SEEK_SET);
   if (status == -1) {
      cm_msg(MERROR, "FileHistory::ReadRecord", "Cannot read \'%s\', lseek(%d) errno %d (%s)", file_name, fpos, errno, strerror(errno));
      return -1;
   }

   status = ::read(fd, rec, recsize);
   if (status == 0) {
      cm_msg(MERROR, "FileHistory::ReadRecord", "Cannot read \'%s\', unexpected end of file on read()", file_name);
      return -1;
   }
   if (status != recsize) {
      cm_msg(MERROR, "FileHistory::ReadRecord", "Cannot read \'%s\', read() errno %d (%s)", file_name, errno, strerror(errno));
      return -1;
   }
   return HS_SUCCESS;
}

static int FindTime(const char* file_name, int fd, int offset, int recsize, int nrec, time_t timestamp, int* irecp, time_t* trecp, time_t* trecp2, int debug)
{
   int status;
   char* buf = new char[recsize];

   int rec1, rec2;
   time_t t1, t2;
   
   rec1 = 0;
   rec2 = nrec-1;

   status = ReadRecord(file_name, fd, offset, recsize, rec1, buf);
   if (status != HS_SUCCESS) {
      delete buf;
      return HS_FILE_ERROR;
   }

   t1 = *(DWORD*)buf;

   // timestamp is older than any data in this file
   if (timestamp <= t1) {
      *irecp = -1;
      *trecp = 0;
      *trecp2 = t1;
      return HS_SUCCESS;
   }

   status = ReadRecord(file_name, fd, offset, recsize, rec2, buf);
   if (status != HS_SUCCESS) {
      delete buf;
      return HS_FILE_ERROR;
   }

   t2 = *(DWORD*)buf;

   // timestamp is newer than any data in this file
   if (timestamp > t2) {
      *irecp = nrec;
      *trecp = 0;
      *trecp2 = 0;
      return HS_SUCCESS;
   }

   assert(timestamp > t1);
   assert(timestamp <= t2);

   if (debug)
      printf("FindTime: rec %d..(x)..%d, time %s..(%s)..%s\n", rec1, rec2, TimeToString(t1).c_str(), TimeToString(timestamp).c_str(), TimeToString(t2).c_str());

   // implement binary search

   do {
      int rec = (rec1+rec2)/2;

      status = ReadRecord(file_name, fd, offset, recsize, rec, buf);
      if (status != HS_SUCCESS) {
         delete buf;
         return HS_FILE_ERROR;
      }

      time_t t = *(DWORD*)buf;

      if (timestamp <= t) {
         if (debug)
            printf("FindTime: rec %d..(x)..%d..%d, time %s..(%s)..%s..%s\n", rec1, rec, rec2, TimeToString(t1).c_str(), TimeToString(timestamp).c_str(), TimeToString(t).c_str(), TimeToString(t2).c_str());

         rec2 = rec;
         t2 = t;
      } else {
         if (debug)
            printf("FindTime: rec %d..%d..(x)..%d, time %s..%s..(%s)..%s\n", rec1, rec, rec2, TimeToString(t1).c_str(), TimeToString(t).c_str(), TimeToString(timestamp).c_str(), TimeToString(t2).c_str());

         rec1 = rec;
         t1 = t;
      }
   } while (rec2 - rec1 > 1);

   assert(rec1+1 == rec2);
   assert(t1 <= timestamp);
   assert(t2 >= timestamp);

   if (debug)
      printf("FindTime: rec %d..(x)..%d, time %s..(%s)..%s, this is the result.\n", rec1, rec2, TimeToString(t1).c_str(), TimeToString(timestamp).c_str(), TimeToString(t2).c_str());

   *irecp = rec1;
   *trecp = t1;
   *trecp2 = t2;

   delete buf;
   return HS_SUCCESS;
}


int HsFileSchema::read_last_written(const time_t timestamp,
                                    const int debug,
                                    time_t* last_written)
{
   int status;
   HsFileSchema* s = this;

   if (debug)
      printf("FileHistory::read_last_written: file %s, schema time %s..%s, timestamp %s\n", s->file_name.c_str(), TimeToString(s->time_from).c_str(), TimeToString(s->time_to).c_str(), TimeToString(timestamp).c_str());

   int fd = open(s->file_name.c_str(), O_RDONLY);
   if (fd < 0) {
      cm_msg(MERROR, "FileHistory::read_last_written", "Cannot read \'%s\', open() errno %d (%s)", s->file_name.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   int file_size = ::lseek(fd, 0, SEEK_END);
         
   int nrec = (file_size - s->data_offset)/s->record_size;
   if (nrec < 0)
      nrec = 0;

   if (nrec < 1) {
      ::close(fd);
      if (last_written)
         *last_written = 0;
      return HS_SUCCESS;
   }

   time_t lw = 0;

   // read last record to check if desired time is inside or outside of the file

   if (1) {
      char* buf = new char[s->record_size];

      status = ReadRecord(s->file_name.c_str(), fd, s->data_offset, s->record_size, nrec - 1, buf);
      if (status != HS_SUCCESS) {
         delete buf;
         ::close(fd);
         return HS_FILE_ERROR;
      }

      lw = *(DWORD*)buf;

      delete buf;
   }

   if (lw >= timestamp) {
      int irec = 0;
      time_t trec = 0;
      time_t trec2 = 0;

      status = FindTime(s->file_name.c_str(), fd, s->data_offset, s->record_size, nrec, timestamp, &irec, &trec, &trec2, 0*debug);
      if (status != HS_SUCCESS) {
         ::close(fd);
         return HS_FILE_ERROR;
      }

      lw = trec;
   }

   if (last_written)
      *last_written = lw;

   if (debug)
      printf("FileHistory::read_last_written: file %s, schema time %s..%s, timestamp %s, last_written %s\n", s->file_name.c_str(), TimeToString(s->time_from).c_str(), TimeToString(s->time_to).c_str(), TimeToString(timestamp).c_str(), TimeToString(lw).c_str());

   assert(lw < timestamp);

   ::close(fd);

   return HS_SUCCESS;
}

int HsFileSchema::read_data(const time_t start_time,
                            const time_t end_time,
                            const int num_var, const int var_schema_index[],
                            const int debug,
                            MidasHistoryBufferInterface* buffer[])
{
   int status;
   HsFileSchema* s = this;

   if (debug)
      printf("FileHistory::read_data: file %s, schema time %s..%s, read time %s..%s, %d vars\n", s->file_name.c_str(), TimeToString(s->time_from).c_str(), TimeToString(s->time_to).c_str(), TimeToString(start_time).c_str(), TimeToString(end_time).c_str(), num_var);

   int fd = open(s->file_name.c_str(), O_RDONLY);
   if (fd < 0) {
      cm_msg(MERROR, "FileHistory::read_data", "Cannot read \'%s\', open() errno %d (%s)", s->file_name.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   int file_size = ::lseek(fd, 0, SEEK_END);
         
   int nrec = (file_size - s->data_offset)/s->record_size;
   if (nrec < 0)
      nrec = 0;

   if (nrec < 1) {
      ::close(fd);
      return HS_SUCCESS;
   }

   int irec = 0;
   time_t trec = 0;
   time_t trec2 = 0;

   status = FindTime(s->file_name.c_str(), fd, s->data_offset, s->record_size, nrec, start_time, &irec, &trec, &trec2, 0*debug);
   if (status != HS_SUCCESS) {
      ::close(fd);
      return HS_FILE_ERROR;
   }

   // irec points to the entry right before "start_time",
   // so we need to increment it. "begin of file" is irec==-1, becomes irec=0
   if (irec < 0)
      irec = 0;
   else
      irec += 1;

   int count = 0;
      
   if (irec < nrec) {
      if (trec)
         assert(trec < start_time);
      assert(trec2 >= start_time);

      int fpos = s->data_offset + irec*s->record_size;

      status = ::lseek(fd, fpos, SEEK_SET);
      if (status == -1) {
         cm_msg(MERROR, "FileHistory::read_data", "Cannot read \'%s\', lseek(%d) errno %d (%s)", s->file_name.c_str(), fpos, errno, strerror(errno));
         ::close(fd);
         return HS_FILE_ERROR;
      }
      
      char* buf = new char[s->record_size];
      
      while (1) {
         status = ::read(fd, buf, s->record_size);
         if (status == 0) // EOF
            break;
         if (status != s->record_size) {
            cm_msg(MERROR, "FileHistory::read_data", "Cannot read \'%s\', read() errno %d (%s)", s->file_name.c_str(), errno, strerror(errno));
            break;
         }
         
         time_t t = *(DWORD*)buf;

         assert(t >= start_time);
         if (t > end_time)
            break;
         
         char* data = buf + 4;
         
         count++;
         for (int i=0; i<num_var; i++) {
            int si = var_schema_index[i];
            if (si < 0)
               continue;
            double v = 0;
            void* ptr = data + s->offsets[si];

            switch (s->variables[si].type) {
            default:
               // FIXME!!!
               abort();
               break;
            case TID_BYTE:
               v = *((uint8_t*)ptr);
               break;
            case TID_SBYTE:
               v = *((int8_t*)ptr);
               break;
            case TID_CHAR:
               v = *((char*)ptr);
               break;
            case TID_WORD:
               v = *((uint16_t*)ptr);
               break;
            case TID_SHORT:
               v = *((int16_t*)ptr);
               break;
            case TID_DWORD:
               v = *((uint32_t*)ptr);
               break;
            case TID_INT:
               v = *((int32_t*)ptr);
               break;
            case TID_BOOL:
               v = *((uint32_t*)ptr);
               break;
            case TID_FLOAT:
               v = *((float*)ptr);
               break;
            case TID_DOUBLE:
               v = *((double*)ptr);
               break;
            }

            buffer[i]->Add(t, v);
         }
      }

      delete buf;
   }
      
   ::close(fd);

   if (debug)
      printf("FileHistory::read: file %s, schema time %s..%s, read time %s..%s, %d vars, read %d rows\n", s->file_name.c_str(), TimeToString(s->time_from).c_str(), TimeToString(s->time_to).c_str(), TimeToString(start_time).c_str(), TimeToString(end_time).c_str(), num_var, count);

   return HS_SUCCESS;
}

////////////////////////////////////////////////////////
//    Implementation of the MidasHistoryInterface     //
////////////////////////////////////////////////////////

class SchemaHistoryBase: public MidasHistoryInterface
{
protected:
   int fDebug;
   std::string fConnectString;

   // writer data
   HsSchemaVector fWriterCurrentSchema;
   std::vector<HsSchema*> fEvents;

   // reader data
   HsSchemaVector fSchema;

public:
   SchemaHistoryBase()
   {
      fDebug = 0;
   }

   virtual ~SchemaHistoryBase()
   {
      // empty
   }

   virtual int hs_set_debug(int debug)
   {
      int old = fDebug;
      fDebug = debug;
      return old;
   }
   
   virtual int hs_connect(const char* connect_string) = 0;
   virtual int hs_disconnect() = 0;

protected:
   ////////////////////////////////////////////////////////
   //             Schema functions                       //
   ////////////////////////////////////////////////////////

   // load existing schema
   virtual int read_schema(HsSchemaVector* sv, const char* event_name, const time_t timestamp) = 0; // timestamp: =0 means read all schema all the way to the beginning of time, =time means read schema in effect at this time and all newer schema

   // return a new copy of a schema for writing into history. If schema for this event does not exist, it will be created
   virtual HsSchema* new_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[]) = 0;

public:
   ////////////////////////////////////////////////////////
   //             Functions used by mlogger              //
   ////////////////////////////////////////////////////////

   int hs_define_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[]);
   int hs_write_event(const char* event_name, time_t timestamp, int buffer_size, const char* buffer);
   int hs_flush_buffers();

   ////////////////////////////////////////////////////////
   //             Functions used by mhttpd               //
   ////////////////////////////////////////////////////////

   int hs_clear_cache()
   {
      if (fDebug)
         printf("hs_clear_cache!\n");

      fWriterCurrentSchema.clear();
      fSchema.clear();

      return HS_SUCCESS;
   }

   int hs_get_events(std::vector<std::string> *pevents)
   {
      if (fDebug)
         printf("hs_get_events!\n");

      if (fSchema.size() == 0) {
         int status = read_schema(&fSchema, NULL, time(NULL));
         if (status != HS_SUCCESS)
            return status;
      }

      fSchema.print(false);

      assert(pevents);

      for (unsigned i=0; i<fSchema.size(); i++) {
         // FIXME: this will return dupes and raw table names for time 0
         bool dupe = false;
         for (unsigned j=0; j<pevents->size(); j++)
            if ((*pevents)[j] == fSchema[i]->event_name) {
               dupe = true;
               break;
            }

         if (!dupe)
            pevents->push_back(fSchema[i]->event_name);
      }
      
      std::sort(pevents->begin(), pevents->end());

      return HS_SUCCESS;
   }
      
   int hs_get_tags(const char* event_name, std::vector<TAG> *ptags)
   {
      time_t timestamp = time(NULL);

      if (fDebug)
         printf("hs_get_tags: event [%s], time %s\n", event_name, TimeToString(timestamp).c_str());

      assert(ptags);

      if (fSchema.size() == 0) {
         int status = read_schema(&fSchema, event_name, timestamp);
         if (status != HS_SUCCESS)
            return status;
      }

      const HsSchema* s = fSchema.find_event(event_name, timestamp);
            
      if (!s)
         return HS_UNDEFINED_EVENT;

      if (fDebug)
         s->print();
      
      for (unsigned i=0; i<s->variables.size(); i++) {
         const char* tagname = s->variables[i].name.c_str();
         //printf("event_name [%s], table_name [%s], column name [%s], tag name [%s]\n", event_name, tn.c_str(), cn.c_str(), tagname);

         bool dupe = false;
            
         for (unsigned k=0; k<ptags->size(); k++)
            if (strcmp((*ptags)[k].name, tagname) == 0) {
               dupe = true;
               break;
            }

         if (!dupe) {
            TAG t;
            STRLCPY(t.name, tagname);
            t.type = s->variables[i].type;
            t.n_data = s->variables[i].n_data;
            
            ptags->push_back(t);
         }
      }

      if (fDebug) {
         printf("hs_get_tags: %d tags\n", (int)ptags->size());
         for (unsigned i=0; i<ptags->size(); i++) {
            printf("  tag[%d]: %s[%d] type %d\n", i, (*ptags)[i].name, (*ptags)[i].n_data, (*ptags)[i].type);
         }
      }

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_get_last_written(time_t timestamp, int num_var, const char* const event_name[], const char* const var_name[], const int var_index[], time_t last_written[])
   {
      if (fDebug) {
         printf("hs_get_last_written: timestamp %s, num_var %d\n", TimeToString(timestamp).c_str(), num_var);
      }

      for (int j=0; j<num_var; j++) {
         last_written[j] = 0;
      }

      for (int i=0; i<num_var; i++) {
         int status = read_schema(&fSchema, event_name[i], 0);
         if (status != HS_SUCCESS)
            return status;
      }

      //fSchema.print(false);

      for (int i=0; i<num_var; i++) {
         bool done = false;
         for (int ss=0; ss<fSchema.size(); ss++) {
            HsSchema* s = fSchema[ss];
            // schema is too new
            if (s->time_from && s->time_from >= timestamp)
               continue;
            // schema for the variables we want?
            int sindex = s->match_event_var(event_name[i], var_name[i], var_index[i]);
            if (sindex < 0)
               continue;
            
            time_t lw = 0;
            
            int status = s->read_last_written(timestamp, fDebug, &lw);
            
            if (status == HS_SUCCESS && lw != 0) {
               last_written[i] = lw;
               done = true;
            }

            if (done)
               break;
         }
      }

      if (fDebug) {
         printf("hs_get_last_written: timestamp time %s, num_var %d, result:\n", TimeToString(timestamp).c_str(), num_var);
         for (int i=0; i<num_var; i++) {
            printf("  event [%s] tag [%s] index [%d] last_written %s\n", event_name[i], var_name[i], var_index[i], TimeToString(last_written[i]).c_str());
         }
      }

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_read_buffer(time_t start_time, time_t end_time,
                      int num_var, const char* const event_name[], const char* const var_name[], const int var_index[],
                      MidasHistoryBufferInterface* buffer[],
                      int hs_status[])
   {
      if (fDebug)
         printf("hs_read_buffer: %d variables\n", num_var);

      for (int i=0; i<num_var; i++) {
         int status = read_schema(&fSchema, event_name[i], start_time);
         if (status != HS_SUCCESS)
            return status;
      }

      if (0 && fDebug)
         fSchema.print(false);

      for (int i=0; i<num_var; i++) {
         hs_status[i] = HS_UNDEFINED_VAR;
      }

      typedef std::map<HsSchema*, int*> SchemaIndexMap;
      SchemaIndexMap smap;
      std::vector<HsSchema*> slist;

      for (int i=0; i<num_var; i++) {

         for (unsigned ss=0; ss<fSchema.size(); ss++) {
            HsSchema* s = fSchema[ss];
            // schema is too new?
            if (s->time_from && s->time_from > end_time)
               continue;
            // schema is too old
            if (s->time_to && s->time_to < start_time)
               continue;
            // schema for the variables we want?
            int sindex = s->match_event_var(event_name[i], var_name[i], var_index[i]);
            if (sindex < 0)
               continue;

            int* ia = smap[s];

            if (!ia) {
               ia = new int[num_var];
               for (int k=0; k<num_var; k++)
                  ia[k] = -1;

               slist.push_back(s);
               smap[s] = ia;
            }

            ia[i] = sindex;
            
            if (0&&fDebug) {
               printf("For event [%s] tag [%s] index [%d] found schema: ", event_name[i], var_name[i], var_index[i]);
               s->print();
            }
         }
      }

      if (1||fDebug) {
         printf("Collected schema:\n");
         
         for (unsigned i=0; i<slist.size(); i++) {
            HsSchema* s = slist[i];
            s->print();
            for (int k=0; k<num_var; k++)
               printf("  tag %s[%d] sindex %d\n", var_name[k], var_index[k], smap[s][k]);
         }
      }

      for (int i=slist.size()-1; i>=0; i--) {
         HsSchema* s = slist[i];
      
         int status = s->read_data(start_time, end_time, num_var, smap[s], fDebug, buffer);

         if (status == HS_SUCCESS)
            for (int j=0; j<num_var; j++) {
               int* si = smap[s];
               assert(si);
               if (si[j] >= 0)
                  hs_status[j] = HS_SUCCESS;
            }

         delete smap[s];
         smap[s] = NULL;
      }

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   class ReadBuffer: public MidasHistoryBufferInterface
   {
   public:
      time_t fFirstTime;
      time_t fLastTime;
      time_t fInterval;

      int fNumAdded;

      int      fNumAlloc;
      int     *fNumEntries;
      time_t **fTimeBuffer;
      double **fDataBuffer;

      time_t   fPrevTime;

      ReadBuffer(time_t first_time, time_t last_time, time_t interval) // ctor
      {
         fNumAdded = 0;

         fFirstTime = first_time;
         fLastTime = last_time;
         fInterval = interval;

         fNumAlloc = 0;
         fNumEntries = NULL;
         fTimeBuffer = NULL;
         fDataBuffer = NULL;

         fPrevTime = 0;
      }

      ~ReadBuffer() // dtor
      {
      }

      void Realloc(int wantalloc)
      {
         if (wantalloc < fNumAlloc - 10)
            return;

         int newalloc = fNumAlloc*2;

         if (newalloc <= 1000)
            newalloc = wantalloc + 1000;

         //printf("wantalloc %d, fNumEntries %d, fNumAlloc %d, newalloc %d\n", wantalloc, *fNumEntries, fNumAlloc, newalloc);

         *fTimeBuffer = (time_t*)realloc(*fTimeBuffer, sizeof(time_t)*newalloc);
         assert(*fTimeBuffer);

         *fDataBuffer = (double*)realloc(*fDataBuffer, sizeof(double)*newalloc);
         assert(*fDataBuffer);

         fNumAlloc = newalloc;
      }

      void Add(time_t t, double v)
      {
         if (t < fFirstTime)
            return;
         if (t > fLastTime)
            return;

         fNumAdded++;

         if ((fPrevTime==0) || (t >= fPrevTime + fInterval)) {
            int pos = *fNumEntries;

            Realloc(pos + 1);
            
            (*fTimeBuffer)[pos] = t;
            (*fDataBuffer)[pos] = v;
            
            (*fNumEntries) = pos + 1;

            fPrevTime = t;
         }
      }

      void Finish()
      {

      }
   };

   /*------------------------------------------------------------------*/

   int hs_read(time_t start_time, time_t end_time, time_t interval,
               int num_var,
               const char* const event_name[], const char* const var_name[], const int var_index[],
               int num_entries[],
               time_t* time_buffer[], double* data_buffer[],
               int st[])
   {
      int status;

      ReadBuffer** buffer = new ReadBuffer*[num_var];
      MidasHistoryBufferInterface** bi = new MidasHistoryBufferInterface*[num_var];

      for (int i=0; i<num_var; i++) {
         buffer[i] = new ReadBuffer(start_time, end_time, interval);
         bi[i] = buffer[i];

         // make sure outputs are initialized to something sane
         if (num_entries)
            num_entries[i] = 0;
         if (time_buffer)
            time_buffer[i] = NULL;
         if (data_buffer)
            data_buffer[i] = NULL;
         if (st)
            st[i] = 0;

         if (num_entries)
            buffer[i]->fNumEntries = &num_entries[i];
         if (time_buffer)
            buffer[i]->fTimeBuffer = &time_buffer[i];
         if (data_buffer)
            buffer[i]->fDataBuffer = &data_buffer[i];
      }

      status = hs_read_buffer(start_time, end_time,
                              num_var, event_name, var_name, var_index,
                              bi, st);

      for (int i=0; i<num_var; i++) {
         buffer[i]->Finish();
         delete buffer[i];
      }

      delete buffer;
      delete bi;

      return status;
   }
   
   /*------------------------------------------------------------------*/

   class BinnedBuffer: public MidasHistoryBufferInterface
   {
   public:
      int fNumBins;
      time_t fFirstTime;
      time_t fLastTime;

      int fNumEntries;
      double *fSum0;
      double *fSum1;
      double *fSum2;

      int    *fCount;
      double *fMean;
      double *fRms;
      double *fMin;
      double *fMax;

      time_t *fLastTimePtr;
      double *fLastValuePtr;

      BinnedBuffer(time_t first_time, time_t last_time, int num_bins) // ctor
      {
         fNumEntries = 0;

         fNumBins = num_bins;
         fFirstTime = first_time;
         fLastTime = last_time;

         fSum0 = new double[num_bins];
         fSum1 = new double[num_bins];
         fSum2 = new double[num_bins];

         for (int i=0; i<num_bins; i++) {
            fSum0[i] = 0;
            fSum1[i] = 0;
            fSum2[i] = 0;
         }

         fMean = NULL;
         fRms = NULL;
         fMin = NULL;
         fMax = NULL;
         fLastTimePtr = NULL;
         fLastValuePtr = NULL;
      }

      ~BinnedBuffer() // dtor
      {
         delete fSum0;
         delete fSum1;
         delete fSum2;
      }

      void Add(time_t t, double v)
      {
         if (t < fFirstTime)
            return;
         if (t > fLastTime)
            return;

         fNumEntries++;

         double a = t - fFirstTime;
         double b = fLastTime - fFirstTime;
         double fbin = fNumBins*a/b;

         int ibin = fbin;

         if (ibin < 0)
            ibin = 0;
         else if (ibin >= fNumBins)
            ibin = fNumBins;

         if (fSum0[ibin] == 0) {
            if (fMin)
               fMin[ibin] = v;
            if (fMax)
               fMax[ibin] = v;
            if (fLastTimePtr)
               *fLastTimePtr = t;
            if (fLastValuePtr)
               *fLastValuePtr = v;
         }

         fSum0[ibin] += 1.0;
         fSum1[ibin] += v;
         fSum2[ibin] += v*v;

         if (fMin)
            if (v < fMin[ibin])
               fMin[ibin] = v;

         if (fMax)
            if (v > fMax[ibin])
               fMax[ibin] = v;

         if (fLastTimePtr)
            if (t > *fLastTimePtr) {
               *fLastTimePtr = t;
               if (fLastValuePtr)
                  *fLastValuePtr = v;
            }
      }

      void Finish()
      {
         for (int i=0; i<fNumBins; i++) {
            double num = fSum0[i];
            double mean = fSum1[i]/num;
            double variance = fSum2[i]/num-mean*mean;
            double rms = 0;
            if (variance > 0)
               rms = sqrt(variance);

            if (fCount)
               fCount[i] = num;

            if (fMean)
               fMean[i] = mean;

            if (fRms)
               fRms[i] = rms;
         }
      }
   };

   int hs_read_binned(time_t start_time, time_t end_time, int num_bins,
                      int num_var, const char* const event_name[], const char* const var_name[], const int var_index[],
                      int num_entries[],
                      int* count_bins[], double* mean_bins[], double* rms_bins[], double* min_bins[], double* max_bins[],
                      time_t last_time[], double last_value[],
                      int st[])
   {
      int status;

      BinnedBuffer** buffer = new BinnedBuffer*[num_var];
      MidasHistoryBufferInterface** xbuffer = new MidasHistoryBufferInterface*[num_var];

      for (int i=0; i<num_var; i++) {
         buffer[i] = new BinnedBuffer(start_time, end_time, num_bins);
         xbuffer[i] = buffer[i];

         if (count_bins)
            buffer[i]->fCount = count_bins[i];
         if (mean_bins)
            buffer[i]->fMean = mean_bins[i];
         if (rms_bins)
            buffer[i]->fRms = rms_bins[i];
         if (min_bins)
            buffer[i]->fMin = min_bins[i];
         if (max_bins)
            buffer[i]->fMax = max_bins[i];
         if (last_time)
            buffer[i]->fLastTimePtr = &last_time[i];
         if (last_value)
            buffer[i]->fLastValuePtr = &last_value[i];
      }

      status = hs_read_buffer(start_time, end_time,
                              num_var, event_name, var_name, var_index,
                              xbuffer,
                              st);

      for (int i=0; i<num_var; i++) {
         buffer[i]->Finish();
         delete buffer[i];
      }

      delete buffer;
      delete xbuffer;

      return status;
   }
};

int SchemaHistoryBase::hs_define_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[])
{
   if (fDebug) {
      printf("hs_define_event: event name [%s] with %d tags\n", event_name, ntags);
      if (fDebug > 1)
         PrintTags(ntags, tags);
   }

   // delete all events with the same name
   for (unsigned int i=0; i<fEvents.size(); i++)
      if (fEvents[i])
         if (fEvents[i]->event_name == event_name) {
            if (fDebug)
               printf("deleting exising event %s\n", event_name);
            fEvents[i]->close();
            delete fEvents[i];
            fEvents[i] = NULL;
         }

   // check for duplicate tag names
   for (int i=0; i<ntags; i++) {
      if (strlen(tags[i].name) < 1) {
         cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\' has empty name at index %d", event_name, i);
         return HS_FILE_ERROR;
      }
      if (tags[i].type <= 0 || tags[i].type >= TID_LAST) {
         cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\' tag \'%s\' at index %d has invalid type %d", event_name, tags[i].name, i, tags[i].type);
         return HS_FILE_ERROR;
      }
      if (tags[i].type == TID_STRING) {
         cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\' tag \'%s\' at index %d has forbidden type TID_STRING", event_name, tags[i].name, i);
         return HS_FILE_ERROR;
      }
      if (tags[i].n_data <= 0) {
         cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\' tag \'%s\' at index %d has invalid n_data %d", event_name, tags[i].name, i, tags[i].n_data);
         return HS_FILE_ERROR;
      }
      for (int j=i+1; j<ntags; j++) {
         if (strcmp(tags[i].name, tags[j].name) == 0) {
            cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\' has duplicate tag name \'%s\' at indices %d and %d", event_name, tags[i].name, i, j);
            return HS_FILE_ERROR;
         }
      }
   }
      
   HsSchema* s = new_event(event_name, timestamp, ntags, tags);
   if (!s)
      return HS_FILE_ERROR;

   s->active = true;

   // find empty slot in events list
   for (unsigned int i=0; i<fEvents.size(); i++)
      if (!fEvents[i]) {
         fEvents[i] = s;
         s = NULL;
         break;
      }

   // if no empty slots, add at the end
   if (s)
      fEvents.push_back(s);

   return HS_SUCCESS;
}

int SchemaHistoryBase::hs_write_event(const char* event_name, time_t timestamp, int buffer_size, const char* buffer)
{
   if (fDebug)
      printf("hs_write_event: write event \'%s\', time %d, size %d\n", event_name, (int)timestamp, buffer_size);

   HsSchema *s = NULL;

   // find this event
   for (size_t i=0; i<fEvents.size(); i++)
      if (fEvents[i] && fEvents[i]->event_name == event_name) {
         s = fEvents[i];
         break;
      }

   // not found
   if (!s)
      return HS_UNDEFINED_EVENT;

   // deactivated because of error?
   if (!s->active)
      return HS_FILE_ERROR;

   int status = s->write_event(timestamp, buffer, buffer_size);

   // if could not write to SQL?
   if (status != HS_SUCCESS) {
      // otherwise, deactivate this event

      s->active = false;

      cm_msg(MERROR, "hs_write_event", "Event \'%s\' disabled after write error %d", event_name, status);

      return HS_FILE_ERROR;
   }

   return HS_SUCCESS;
}

int SchemaHistoryBase::hs_flush_buffers()
{
   int status = HS_SUCCESS;
   
   if (fDebug)
      printf("hs_flush_buffers!\n");
   
   for (unsigned int i=0; i<fEvents.size(); i++)
      if (fEvents[i]) {
         int xstatus = fEvents[i]->flush_buffers();
         if (xstatus != HS_SUCCESS)
            status = xstatus;
      }
   
   return status;
}

////////////////////////////////////////////////////////
//                    SQL schema                      //
////////////////////////////////////////////////////////

int HsSqlSchema::close_transaction()
{
   int status = HS_SUCCESS;
   if (transaction_count > 0) {
      status = sql->CommitTransaction(table_name.c_str());
      transaction_count = 0;
   }
   return status;
}


int HsSchema::match_event_var(const char* event_name, const char* var_name, const int var_index)
{
   if (!MatchEventName(this->event_name.c_str(), event_name))
      return -1;

   for (unsigned j=0; j<this->variables.size(); j++) {
      if (MatchTagName(this->variables[j].name.c_str(), var_name, var_index))
         return j;
   }

   return -1;
}

int HsSqlSchema::match_event_var(const char* event_name, const char* var_name, const int var_index)
{
   if (this->table_name == event_name) {
      for (unsigned j=0; j<this->variables.size(); j++) {
         if (this->column_names[j] == var_name)
            return j;
      }
   }

   return HsSchema::match_event_var(event_name, var_name, var_index);
}

int HsSqlSchema::write_event(const time_t t, const char* data, const int data_size)
{
   HsSqlSchema* s = this;

   std::string tags;
   std::string values;
   
   for (int i=0; i<s->variables.size(); i++) {
      if (s->variables[i].name.length() < 1)
         continue;

      int type   = s->variables[i].type;
      int n_data = s->variables[i].n_data;
      int offset = s->offsets[i];
      const char* column_name = s->column_names[i].c_str();

      if (offset < 0)
         continue;

      assert(n_data == 1);
      assert(strlen(column_name) > 0);
      assert(offset < data_size);

      void* ptr = (void*)(data+offset);

      tags += ", ";
      tags += "\'";
      tags += column_name;
      tags += "\'";

      values += ", ";

      char buf[1024];
      int j=0;
         
      switch (type) {
      default:
         sprintf(buf, "unknownType%d", type);
         break;
      case TID_BYTE:
         sprintf(buf, "%u",((uint8_t*)ptr)[j]);
         break;
      case TID_SBYTE:
         sprintf(buf, "%d",((int8_t*)ptr)[j]);
         break;
      case TID_CHAR:
         sprintf(buf, "\'%c\'",((char*)ptr)[j]);
         break;
      case TID_WORD:
         sprintf(buf, "%u",((uint16_t*)ptr)[j]);
         break;
      case TID_SHORT:
         sprintf(buf, "%d",((int16_t*)ptr)[j]);
         break;
      case TID_DWORD:
         sprintf(buf, "%u",((uint32_t*)ptr)[j]);
         break;
      case TID_INT:
         sprintf(buf, "%d",((int32_t*)ptr)[j]);
         break;
      case TID_BOOL:
         sprintf(buf, "%u",((uint32_t*)ptr)[j]);
         break;
      case TID_FLOAT:
         sprintf(buf, "\'%.8g\'",((float*)ptr)[j]);
         break;
      case TID_DOUBLE:
         sprintf(buf, "\'%.16g\'",((double*)ptr)[j]);
         break;
      }
      
      values += buf;
   }

   // 2001-02-16 20:38:40.1
   char buf[1024];
   strftime(buf, sizeof(buf)-1, "%Y-%m-%d %H:%M:%S.0", localtime(&t));

   std::string cmd;
   cmd = "INSERT INTO \'";
   cmd += s->table_name;
   cmd += "\' (_t_time, _i_time";
   cmd += tags;
   cmd += ") VALUES (\'";
   cmd += buf;
   cmd += "\', \'";
   cmd += TimeToString(t);
   cmd += "\'";
   cmd += values;
   cmd += ");";

   if (s->transaction_count == 0)
      sql->OpenTransaction(s->table_name.c_str());

   s->transaction_count++;
   
   int status = sql->Exec(s->table_name.c_str(), cmd.c_str());

   if (s->transaction_count > 100000) {
      //printf("flush table %s\n", table_name);
      sql->CommitTransaction(s->table_name.c_str());
      s->transaction_count = 0;
   }

   if (status != DB_SUCCESS) {
      return status;
   }

   return HS_SUCCESS;
}

int HsSqlSchema::read_last_written(const time_t timestamp,
                                   const int debug,
                                   time_t* last_written)
{
   if (debug)
      printf("SqlHistory::read_last_written: table [%s], timestamp %s\n", table_name.c_str(), TimeToString(timestamp).c_str());

   std::string cmd;
   cmd += "SELECT _i_time FROM \'";
   cmd += table_name;
   cmd += "\' WHERE _i_time < ";
   cmd += TimeToString(timestamp);
   cmd += " ORDER BY _i_time DESC LIMIT 2;";

   int status = sql->Prepare(table_name.c_str(), cmd.c_str());
            
   if (status != DB_SUCCESS)
      return status;

   time_t lw = 0;
            
   /* Loop through the rows in the result-set */
            
   for (int k=0; ; k++) {
      status = sql->Step();
      if (status != DB_SUCCESS)
         break;
               
      time_t t = sql->GetTime(0);

      if (t >= timestamp)
         continue;

      if (t > lw)
         lw = t;
   }
            
   sql->Finalize();
         
   *last_written = lw;

   if (debug)
      printf("SqlHistory::read_last_written: table [%s], timestamp %s, last_written %s\n", table_name.c_str(), TimeToString(timestamp).c_str(), TimeToString(lw).c_str());

   return HS_SUCCESS;
}

int HsSqlSchema::read_data(const time_t start_time,
                           const time_t end_time,
                           const int num_var, const int var_schema_index[],
                           const int debug,
                           MidasHistoryBufferInterface* buffer[])
{
   if (debug)
      printf("SqlHistory::read_data: table [%s], start %s, end %s\n", table_name.c_str(), TimeToString(start_time).c_str(), TimeToString(end_time).c_str());

   std::string collist;

   for (int i=0; i<num_var; i++) {
      int j = var_schema_index[i];
      if (j < 0)
         continue;
      if (collist.length() > 0)
         collist += ", ";
      collist += std::string("\"") + column_names[j] + "\"";
   }

   std::string cmd;
   cmd += "SELECT _i_time, ";
   cmd += collist;
   cmd += " FROM \'";
   cmd += table_name;
   cmd += "\' WHERE _i_time>=";
   cmd += TimeToString(start_time);
   cmd += " and _i_time<=";
   cmd += TimeToString(end_time);
   cmd += " ORDER BY _i_time;";

   int status = sql->Prepare(table_name.c_str(), cmd.c_str());
      
   if (status != DB_SUCCESS)
      return HS_FILE_ERROR;
      
   /* Loop through the rows in the result-set */

   int count = 0;

   while (1) {
      status = sql->Step();
      if (status != DB_SUCCESS)
         break;

      count++;
         
      time_t t = sql->GetTime(0);
         
      if (t < start_time || t > end_time)
         continue;

      int k = 0;
      for (int i=0; i<num_var; i++) {
         int j = var_schema_index[i];
         if (j < 0)
            continue;
            
         double v = sql->GetDouble(1+k);

         //printf("Column %d, index %d, Row %d, time %d, value %f\n", k, colindex[k], count, t, v);

         buffer[i]->Add(t, v);
         k++;
      }
   }
      
   sql->Finalize();
   
   if (debug)
      printf("SqlHistory::read_data: table [%s], start %s, end %s, read %d rows\n", table_name.c_str(), TimeToString(start_time).c_str(), TimeToString(end_time).c_str(), count);

   return HS_SUCCESS;
}

////////////////////////////////////////////////////////
//             SQL history functions                  //
////////////////////////////////////////////////////////

static int StartSqlTransaction(SqlBase* sql, const char* table_name, bool* have_transaction)
{
   if (*have_transaction)
      return HS_SUCCESS;
   
   int status = sql->OpenTransaction(table_name);
   if (status != DB_SUCCESS)
      return HS_FILE_ERROR;
   
   *have_transaction = true;
   return HS_SUCCESS;
}

static int CreateSqlTable(SqlBase* sql, const char* table_name, bool* have_transaction)
{
   int status;

   status = StartSqlTransaction(sql, table_name, have_transaction);
   if (status != DB_SUCCESS)
      return HS_FILE_ERROR;
      
   std::string cmd;
      
   cmd = "CREATE TABLE \'";
   cmd += table_name;
   cmd += "\' (_t_time TIMESTAMP NOT NULL, _i_time INTEGER NOT NULL);";

   status = sql->Exec(table_name, cmd.c_str());
   if (status != DB_SUCCESS)
      return HS_FILE_ERROR;
         
   cmd = "CREATE INDEX \'";
   cmd += table_name;
   cmd += "_i_time_index\' ON \'";
   cmd += table_name;
   cmd += "\' (_i_time ASC);";
   
   status = sql->Exec(table_name, cmd.c_str());
   if (status != DB_SUCCESS)
      return HS_FILE_ERROR;
         
   cmd = "CREATE INDEX \'";
   cmd += table_name;
   cmd += "_t_time_index\' ON \'";
   cmd += table_name;
   cmd += "\' (_t_time);";
   
   status = sql->Exec(table_name, cmd.c_str());
   if (status != DB_SUCCESS)
      return HS_FILE_ERROR;

   return status;
}

static int CreateSqlColumn(SqlBase* sql, const char* table_name, const char* column_name, const char* column_type, bool* have_transaction, int debug)
{
   if (debug)
      printf("CreateSqlColumn: table [%s], column [%s], type [%s]\n", table_name, column_name, column_type);

   int status = StartSqlTransaction(sql, table_name, have_transaction);
   if (status != HS_SUCCESS)
      return status;

   std::string cmd;
   cmd = "ALTER TABLE \'";
   cmd += table_name;
   cmd += "\' ADD COLUMN \'";
   cmd += column_name;
   cmd += "\' ";
   cmd += column_type;
   cmd += ";";
      
   return sql->Exec(table_name, cmd.c_str());
}

////////////////////////////////////////////////////////
//             SQL history base classes               //
////////////////////////////////////////////////////////

class SqlHistoryBase: public SchemaHistoryBase
{
public:
   SqlBase *fSql;

   SqlHistoryBase() // ctor
   {
      fSql = NULL;
      hs_clear_cache();
   }

   virtual ~SqlHistoryBase() // dtor
   {
      hs_disconnect();
      if (fSql)
         delete fSql;
      fSql = NULL;
   }

   int hs_set_debug(int debug)
   {
      if (fSql)
         fSql->fDebug = debug;
      return SchemaHistoryBase::hs_set_debug(debug);
   }
   
   int hs_connect(const char* connect_string)
   {
      if (fDebug)
         printf("hs_connect [%s]!\n", connect_string);

      assert(fSql);

      if (fSql->IsConnected())
         if (strcmp(fConnectString.c_str(), connect_string) == 0)
            return HS_SUCCESS;
    
      hs_disconnect();

      if (!connect_string || strlen(connect_string) < 1) {
         // FIXME: should use "logger dir" or some such default, that code should be in hs_get_history(), not here
         connect_string = ".";
      }
    
      fConnectString = connect_string;
    
      if (fDebug)
         printf("hs_connect: connecting to SQL database \'%s\'\n", fConnectString.c_str());
    
      int status = fSql->Connect(fConnectString.c_str());
      if (status != DB_SUCCESS)
         return status;

      return HS_SUCCESS;
   }

   int hs_disconnect()
   {
      if (fDebug)
         printf("hs_disconnect!\n");

      hs_flush_buffers();

      fSql->Disconnect();

      hs_clear_cache();
      
      return HS_SUCCESS;
   }
};

////////////////////////////////////////////////////////
//             SQLITE functions                       //
////////////////////////////////////////////////////////

static int ReadSqliteTableNames(SqlBase* sql, HsSchemaVector *sv, const char* table_name, int debug)
{
   if (debug)
      printf("ReadSqliteTableNames: table [%s]\n", table_name);

   int status;
   std::string cmd;

   cmd = "SELECT event_name, _i_time FROM \'_event_name_";
   cmd += table_name;
   cmd += "\' WHERE table_name='";
   cmd += table_name;
   cmd += "';";

   status = sql->Prepare(table_name, cmd.c_str());
      
   if (status != DB_SUCCESS)
      return status;
      
   while (1) {
      status = sql->Step();
         
      if (status != DB_SUCCESS)
         break;
         
      std::string xevent_name  = sql->GetText(0);
      time_t      xevent_time  = sql->GetTime(1);
         
      //printf("read event name [%s] time %s\n", xevent_name.c_str(), TimeToString(xevent_time).c_str());

      HsSqlSchema* s = new HsSqlSchema;
      s->sql = sql;
      s->event_name = xevent_name;
      s->time_from = xevent_time;
      s->time_to = 0;
      s->table_name = table_name;
      sv->add(s);
   }
      
   status = sql->Finalize();

   return HS_SUCCESS;
}

static HsSqlSchema* NewSqlSchema(HsSchemaVector* sv, const char* table_name, time_t t)
{
   time_t tt = 0;
   int j=-1;
   for (unsigned i=0; i<sv->size(); i++) {
      HsSqlSchema* s = (HsSqlSchema*)(*sv)[i];
      if (s->table_name != table_name)
         continue;

      if (s->time_from == t)
         return s;

      // remember the last schema before time t
      if (s->time_from < t) {
         if (s->time_from > tt) {
            tt = s->time_from;
            j = i;
         }
      }
   }

   //printf("NewSqlSchema: will copy schema j=%d, tt=%d at time %d\n", j, tt, t);

   //printf("cloned schema at time %s: ", TimeToString(t).c_str());
   //(*sv)[j]->print(false);

   //printf("schema before:\n");
   //sv->print(false);

   HsSqlSchema* s = new HsSqlSchema;
   *s = *(HsSqlSchema*)(*sv)[j]; // make a copy
   s->time_from = t;
   sv->add(s);

   //printf("schema after:\n");
   //sv->print(false);

   return s;
}

static int ReadSqliteColumnNames(SqlBase* sql, HsSchemaVector *sv, const char* table_name, int debug)
{
   // for all schema for table_name, prepopulate is with column names

   std::vector<std::string> columns;
   sql->ListColumns(table_name, &columns);

   // first, populate column names

   for (unsigned i=0; i<sv->size(); i++) {
      HsSqlSchema* s = (HsSqlSchema*)(*sv)[i];

      if (s->table_name != table_name)
         continue;

      // schema should be empty at this point
      //assert(s->variables.size() == 0);

      for (unsigned j=0; j<columns.size(); j+=2) {
         const char* cn = columns[j+0].c_str();
         const char* ct = columns[j+1].c_str();

         if (strcmp(cn, "_t_time") == 0)
            continue;
         if (strcmp(cn, "_i_time") == 0)
            continue;

         bool found = false;

         for (unsigned k=0; k<s->column_names.size(); k++) {
            if (s->column_names[k] == cn) {
               found = true;
               break;
            }
         }

         //printf("column [%s] sql type [%s]\n", cn.c_str(), ct);
         
         if (!found) {
            HsSchemaEntry se;
            se.name = cn;
            se.type = 0;
            se.n_data = 1;
            se.n_bytes = 0;
            s->variables.push_back(se);
            s->column_names.push_back(cn);
            s->column_types.push_back(ct);
            s->offsets.push_back(-1);
         }
      }
   }

   // then read column name information

   std::string cmd;
   cmd = "SELECT column_name, tag_name, tag_type, _i_time FROM \'_column_names_";
   cmd += table_name;
   cmd += "\' WHERE table_name='";
   cmd += table_name;
   cmd += "' ORDER BY _i_time ASC;";

   int status = sql->Prepare(table_name, cmd.c_str());
      
   if (status != DB_SUCCESS) {
      return status;
   }
   
   while (1) {
      status = sql->Step();
         
      if (status != DB_SUCCESS)
         break;
         
      // NOTE: SQL "SELECT ORDER BY _i_time ASC" returns data sorted by time
      // in this code we use the data from the last data row
      // so if multiple rows are present, the latest one is used
      
      std::string col_name  = sql->GetText(0);
      std::string tag_name  = sql->GetText(1);
      std::string tag_type  = sql->GetText(2);
      time_t   schema_time  = sql->GetTime(3);
      
      //printf("read table [%s] column [%s] tag name [%s] time %s\n", table_name, col_name.c_str(), tag_name.c_str(), TimeToString(xxx_time).c_str());

      // make sure a schema exists at this time point
      NewSqlSchema(sv, table_name, schema_time);

      // add this information to all schema

      for (unsigned i=0; i<sv->size(); i++) {
         HsSqlSchema* s = (HsSqlSchema*)(*sv)[i];
         if (s->table_name != table_name)
            continue;
         if (s->time_from < schema_time)
            continue;

         //printf("add column to schema %d\n", s->time_from);

         for (unsigned j=0; j<s->column_names.size(); j++) {
            if (col_name != s->column_names[j])
               continue;
            s->variables[j].name = tag_name;
            s->variables[j].type = midas_tid(tag_type.c_str());
            s->variables[j].n_data = 1;
            s->variables[j].n_bytes = rpc_tid_size(s->variables[j].type);
         }
      }
   }
   
   status = sql->Finalize();

   return HS_SUCCESS;
}

static int ReadSqliteTableSchema(SqlBase* sql, HsSchemaVector *sv, const char* table_name, int debug)
{
   if (debug)
      printf("ReadSqliteTableSchema: table [%s]\n", table_name);

   if (1) {
      // seed schema with table names
      HsSqlSchema* s = new HsSqlSchema;
      s->sql = sql;
      s->event_name = table_name;
      s->time_from = 0;
      s->time_to = 0;
      s->table_name = table_name;
      sv->add(s);
   }

   return ReadSqliteTableNames(sql, sv, table_name, debug);
}

static int ReadSqliteSchema(SqlBase* sql, HsSchemaVector *sv, int debug)
{
   int status;

   if (debug)
      printf("ReadSqliteSchema!\n");

   // loop over all tables

   std::vector<std::string> tables;
   status = sql->ListTables(&tables);
   if (status != DB_SUCCESS)
      return status;

   for (unsigned i=0; i<tables.size(); i++) {
      const char* table_name = tables[i].c_str();

      const char* s;
      s = strstr(table_name, "_event_name_");
      if (s == table_name)
         continue;
      s = strstr(table_name, "_column_names_");
      if (s == table_name)
         continue;

      status = ReadSqliteTableSchema(sql, sv, table_name, debug);
   }

   return HS_SUCCESS;
}

static int CreateSqliteEvent(SqlBase* sql, HsSchemaVector* sv, const char* event_name, time_t timestamp, int debug)
{
   if (debug)
      printf("CreateSqliteEvent: event [%s], timestamp %s\n", event_name, TimeToString(timestamp).c_str());

   int status;
   bool have_transaction = false;
   std::string table_name = MidasNameToSqlName(event_name);
   
   // FIXME: what about duplicate table names?
   status = CreateSqlTable(sql, table_name.c_str(), &have_transaction);
   
   if (status != HS_SUCCESS) {
      // FIXME: ???
      // FIXME: at least close or revert the transaction
      return status;
   }

   std::string cmd;
         
   cmd = "CREATE TABLE \'_event_name_";
   cmd += table_name;
   cmd += "\' (table_name TEXT NOT NULL, event_name TEXT NOT NULL, _i_time INTEGER NOT NULL);";

   status = sql->Exec(table_name.c_str(), cmd.c_str());
   
   cmd = "INSERT INTO \'_event_name_";
   cmd += table_name;
   cmd += "\' (table_name, event_name, _i_time) VALUES (\'";
   cmd += table_name;
   cmd += "\', \'";
   cmd += event_name;
   cmd += "\', \'";
   cmd += TimeToString(timestamp);
   cmd += "\');";
   
   status = sql->Exec(table_name.c_str(), cmd.c_str());
   
   cmd = "CREATE TABLE \'_column_names_";
   cmd += table_name;
   cmd += "\' (table_name TEXT NOT NULL, column_name TEXT NOT NULL, tag_name TEXT NOT NULL, tag_type TEXT NOT NULL, column_type TEXT NOT NULL, _i_time INTEGER NOT NULL);";
   
   status = sql->Exec(table_name.c_str(), cmd.c_str());
   
   status = sql->CommitTransaction(table_name.c_str());
   if (status != DB_SUCCESS) {
      return HS_FILE_ERROR;
   }
   
   return ReadSqliteTableSchema(sql, sv, table_name.c_str(), debug);
}

static int UpdateSqliteColumn(SqlBase* sql, const char* table_name, const char* column_name, const char* column_type, const char* tag_name, const char* tag_type, const time_t timestamp, bool* have_transaction, int debug)
{
   if (debug)
      printf("UpdateSqliteColumn: table [%s], column [%s], new name [%s], timestamp %s\n", table_name, column_name, tag_name, TimeToString(timestamp).c_str());

   int status = StartSqlTransaction(sql, table_name, have_transaction);
   if (status != HS_SUCCESS)
      return status;
   
   std::string cmd;
   cmd = "INSERT INTO \'_column_names_";
   cmd += table_name;
   cmd += "\' (table_name, column_name, tag_name, tag_type, column_type, _i_time) VALUES (\'";
   cmd += table_name;
   cmd += "\', \'";
   cmd += column_name;
   cmd += "\', \'";
   cmd += tag_name;
   cmd += "\', \'";
   cmd += tag_type;
   cmd += "\', \'";
   cmd += column_type;
   cmd += "\', \'";
   cmd += TimeToString(timestamp);
   cmd += "\');";
   status = sql->Exec(table_name, cmd.c_str());
   
   return status;
}

static int UpdateSqliteSchema1(HsSqlSchema* s, const time_t timestamp, const int ntags, const TAG tags[], bool write_enable, bool* have_transaction, int debug)
{
   int status;

   // check that compare schema with tags[]

   bool schema_ok = true;

   int offset = 0;
   for (int i=0; i<ntags; i++) {
      for (unsigned int j=0; j<tags[i].n_data; j++) {
         int         tagtype = tags[i].type;
         std::string tagname = tags[i].name;
         std::string maybe_colname = MidasNameToSqlName(tags[i].name);

         if (tags[i].n_data > 1) {
            char s[256];
            sprintf(s, "[%d]", j);
            tagname += s;

            sprintf(s, "_%d", j);
            maybe_colname += s;
         }

         int count = 0;

         for (unsigned j=0; j<s->variables.size(); j++) {
            if (tagname == s->variables[j].name) {
               if (s->sql->TypesCompatible(tagtype, s->column_types[j].c_str())) {
                  if (count == 0) {
                     s->offsets[j] = offset;
                     offset += rpc_tid_size(tagtype);
                  }
                  count++;
               } else {
                  // column with incompatible type, mark it as unused
                  schema_ok = false;
                  if (write_enable) {
                     status = UpdateSqliteColumn(s->sql, s->table_name.c_str(), s->column_names[j].c_str(), s->column_types[j].c_str(), "" /*tagname*/, rpc_tid_name(tagtype), timestamp, have_transaction, debug);
                     if (status != HS_SUCCESS)
                        return status;
                  }
               }
            }
         }

         if (count == 0) {
            // tag does not have a corresponding column
            schema_ok = false;

            // create column
            if (write_enable) {
               std::string col_name = maybe_colname;
               const char* col_type = s->sql->ColumnType(tagtype);

               bool dupe = false;
               for (unsigned kk=0; kk<s->column_names.size(); kk++)
                  if (s->column_names[kk] == col_name) {
                     dupe = true;
                     break;
                  }

               bool retry = false;
               for (int t=0; t<10; t++) {

                  // if duplicate column name, change it, try again
                  if (dupe || retry) {
                     char s[256];
                     sprintf(s, "_%d", rand());
                     col_name = maybe_colname + s;
                  }

                  if (debug)
                     printf("UpdateSqliteSchema: table [%s], add column [%s] type [%s] for tag [%s]\n", s->table_name.c_str(), col_name.c_str(), col_type, tagname.c_str());

                  status = CreateSqlColumn(s->sql, s->table_name.c_str(), col_name.c_str(), col_type, have_transaction, debug);

                  if (status == DB_KEY_EXIST) {
                     if (debug)
                        printf("UpdateSqliteSchema: table [%s], add column [%s] type [%s] for tag [%s] failed: duplicate column name\n", s->table_name.c_str(), col_name.c_str(), col_type, tagname.c_str());
                     retry = true;
                     continue;
                  }

                  if (status != HS_SUCCESS)
                     return status;

                  break;
               }

               if (status != HS_SUCCESS)
                  return status;

               status = UpdateSqliteColumn(s->sql, s->table_name.c_str(), col_name.c_str(), col_type, tagname.c_str(), rpc_tid_name(tagtype), timestamp, have_transaction, debug);
               if (status != HS_SUCCESS)
                  return status;
            }
         }

         if (count > 1) {
            // schema has duplicate tags
            schema_ok = false;
         }
      }
   }

   // mark as unused all columns not listed in tags

   for (unsigned k=0; k<s->column_names.size(); k++)
      if (s->variables[k].name.length() > 0) {
         bool found = false;

         for (int i=0; i<ntags; i++) {
            for (unsigned int j=0; j<tags[i].n_data; j++) {
               std::string tagname = tags[i].name;

               if (tags[i].n_data > 1) {
                  char s[256];
                  sprintf(s, "[%d]", j);
                  tagname += s;
               }

               if (s->variables[k].name == tagname) {
                  found = true;
                  break;
               }
            }

            if (found)
               break;
         }

         if (!found) {
            // column with incompatible type, mark it as unused
            schema_ok = false;
            if (write_enable) {
               if (debug)
                  printf("UpdateSqliteSchema: table [%s], column %d name [%s] type [%s] is marked as unused\n", s->table_name.c_str(), k, s->column_names[k].c_str(), s->column_types[k].c_str());
               status = UpdateSqliteColumn(s->sql, s->table_name.c_str(), s->column_names[k].c_str(), s->column_types[k].c_str(), "" /*tagname*/, "" /* tagtype */, timestamp, have_transaction, debug);
               if (status != HS_SUCCESS)
                  return status;
            }
         }
      }

   if (!write_enable)
      if (!schema_ok)
         return HS_FILE_ERROR;
   
   return HS_SUCCESS;
}

static int UpdateSqliteSchema(HsSqlSchema* s, const time_t timestamp, const int ntags, const TAG tags[], bool write_enable, int debug)
{
   int status;
   bool have_transaction = false;

   status = UpdateSqliteSchema1(s, timestamp, ntags, tags, write_enable, &have_transaction, debug);

   if (have_transaction) {
      int xstatus;

      if (status == HS_SUCCESS)
         xstatus = s->sql->CommitTransaction(s->table_name.c_str());
      else
         xstatus = s->sql->RollbackTransaction(s->table_name.c_str());

      if (xstatus != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }
      have_transaction = false;
   }

   return status;
}

////////////////////////////////////////////////////////
//             SQLITE history classes                 //
////////////////////////////////////////////////////////

class SqliteHistory: public SqlHistoryBase
{
public:
   SqliteHistory() { // ctor
#ifdef HAVE_SQLITE
      fSql = new Sqlite();
#endif
   }

   int read_schema(HsSchemaVector* sv, const char* event_name, const time_t timestamp);
   HsSchema* new_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[]);
};

int SqliteHistory::read_schema(HsSchemaVector* sv, const char* event_name, const time_t timestamp)
{
   if (fDebug)
      printf("SqliteHistory::read_schema: loading schema for event [%s] at time %s\n", event_name, TimeToString(timestamp).c_str());

   int status;

   if (fSchema.size() == 0) {
      status = ReadSqliteSchema(fSql, sv, fDebug);
      if (status != HS_SUCCESS)
         return status;
   }

   //sv->print(false);

   for (unsigned i=0; i<sv->size(); i++) {
      HsSqlSchema* h = (HsSqlSchema*)(*sv)[i];
      // skip schema with already read column names
      if (h->variables.size() > 0)
         continue;
      // skip schema with different name
      if (!MatchEventName(h->event_name.c_str(), event_name))
         continue;

      unsigned nn = sv->size();

      status = ReadSqliteColumnNames(fSql, sv, h->table_name.c_str(), fDebug);

      // if new schema was added, loop all over again
      if (sv->size() != nn)
         i=0;
   }
   
   //sv->print(false);

   return HS_SUCCESS;
}

HsSchema* SqliteHistory::new_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[])
{
   if (fDebug)
      printf("SqliteHistory::new_event: event [%s], timestamp %s, ntags %d\n", event_name, TimeToString(timestamp).c_str(), ntags);

   int status;

   if (fWriterCurrentSchema.size() == 0) {
      status = ReadSqliteSchema(fSql, &fWriterCurrentSchema, fDebug);
      if (status != HS_SUCCESS)
         return NULL;
   }

   HsSqlSchema* s = (HsSqlSchema*)fWriterCurrentSchema.find_event(event_name, timestamp);

   // schema does not exist, the SQL tables probably do not exist yet

   if (!s) {
      status = CreateSqliteEvent(fSql, &fWriterCurrentSchema, event_name, timestamp, fDebug);
      if (status != HS_SUCCESS)
         return NULL;

      s = (HsSqlSchema*)fWriterCurrentSchema.find_event(event_name, timestamp);
      
      if (!s) {
         cm_msg(MERROR, "SqliteHistory::new_event", "Error: Cannot create schema for event \'%s\', see previous messages", event_name);
         return NULL;
      }
   }

   assert(s != NULL);

   status = ReadSqliteColumnNames(fSql, &fWriterCurrentSchema, s->table_name.c_str(), fDebug);
   if (status != HS_SUCCESS)
      return NULL;

   s = (HsSqlSchema*)fWriterCurrentSchema.find_event(event_name, timestamp);

   if (!s) {
      cm_msg(MERROR, "SqliteHistory::new_event", "Error: Cannot update schema database for event \'%s\', see previous messages", event_name);
      return NULL;
   }

   if (0||fDebug) {
      printf("SqliteHistory::new_event: schema for [%s] is %p\n", event_name, s);
      if (s)
         s->print();
   }

   status = UpdateSqliteSchema(s, timestamp, ntags, tags, true, fDebug);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "SqliteHistory::new_event", "Error: Cannot create schema for event \'%s\', see previous messages", event_name);
      return NULL;
   }

   status = ReadSqliteColumnNames(fSql, &fWriterCurrentSchema, s->table_name.c_str(), fDebug);
   if (status != HS_SUCCESS)
      return NULL;

   s = (HsSqlSchema*)fWriterCurrentSchema.find_event(event_name, timestamp);

   if (!s) {
      cm_msg(MERROR, "SqliteHistory::new_event", "Error: Cannot update schema database for event \'%s\', see previous messages", event_name);
      return NULL;
   }

   if (0||fDebug) {
      printf("SqliteHistory::new_event: schema for [%s] is %p\n", event_name, s);
      if (s)
         s->print();
   }

   // last call to UpdateSqliteSchema with "false" will check that new schema matches the new tags

   status = UpdateSqliteSchema(s, timestamp, ntags, tags, false, fDebug);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "SqliteHistory::new_event", "Error: Cannot create schema for event \'%s\', see previous messages", event_name);
      return NULL;
   }

   HsSqlSchema* e = new HsSqlSchema();

   *e = *s; // make a copy of the schema

   return e;
}

////////////////////////////////////////////////////////
//              Mysql history classes                 //
////////////////////////////////////////////////////////

class MysqlHistory: public SqlHistoryBase
{
public:
   MysqlHistory() { // ctor
#ifdef HAVE_MYSQL
      fSql = new Mysql();
#endif
   }

   int read_schema(HsSchemaVector* sv, const char* event_name, const time_t timestamp)
   {
      // FIXME: only read column data for this event_name
      // FIXME: read ODBC compatible schema
      //ReadSqlSchema(fSql, sv, true);
      abort();
      return HS_SUCCESS;
   }

   HsSchema* new_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[])
   {
      // FIXME
      abort();
      return NULL;
   }
};

////////////////////////////////////////////////////////
//               File history class                   //
////////////////////////////////////////////////////////

class FileHistory: public SchemaHistoryBase
{
protected:
   std::string fPath;
   struct timespec last_mtimespec;

public:
   FileHistory() // ctor
   {
      memset(&last_mtimespec, 0, sizeof(last_mtimespec));
   }

   int hs_connect(const char* connect_string)
   {
      if (fDebug)
         printf("hs_connect [%s]!\n", connect_string);

      hs_disconnect();

      if (!connect_string || strlen(connect_string) < 1) {
         // FIXME: should use "logger dir" or some such default, that code should be in hs_get_history(), not here
         connect_string = ".";
      }
    
      fConnectString = connect_string;
      fPath = connect_string;
    
      return HS_SUCCESS;
   }

   int hs_disconnect()
   {
      if (fDebug)
         printf("hs_disconnect!\n");

      hs_flush_buffers();
      hs_clear_cache();
      
      return HS_SUCCESS;
   }

   int read_schema(HsSchemaVector* sv, const char* event_name, const time_t timestamp);
   HsSchema* new_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[]);

protected:
   int create_file(const char* event_name, time_t timestamp, int ntags, const TAG tags[], std::string* filenamep);
   HsFileSchema* read_file_schema(const char* filename);
};

int FileHistory::read_schema(HsSchemaVector* sv, const char* event_name, const time_t timestamp)
{
   int status;

   struct stat stat_buf;
   status = lstat(fPath.c_str(), &stat_buf);
   if (status != 0) {
      cm_msg(MERROR, "FileHistory::read_schema", "Cannot lstat(%s), errno %d (%s)", fPath.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   //printf("dir [%s], mtime: %d %d last: %d %d, mtime %s", fPath.c_str(), stat_buf.st_mtimespec.tv_sec, stat_buf.st_mtimespec.tv_nsec, last_mtimespec.tv_sec, last_mtimespec.tv_nsec, ctime(&stat_buf.st_mtimespec.tv_sec));

   if (memcmp(&stat_buf.st_mtimespec, &last_mtimespec, sizeof(last_mtimespec))==0) {
      if (fDebug)
         printf("FileHistory::read_schema: loading schema for event [%s] at time %s: nothing to reload, history directory mtime did not change\n", event_name, TimeToString(timestamp).c_str());
      return HS_SUCCESS;
   }

   last_mtimespec = stat_buf.st_mtimespec;

   if (fDebug)
      printf("FileHistory::read_schema: loading schema for event [%s] at time %s\n", event_name, TimeToString(timestamp).c_str());

   std::vector<std::string> flist;

   DIR *dir = opendir(fPath.c_str());
   if (!dir) {
      cm_msg(MERROR, "FileHistory::read_schema", "Cannot opendir(%s), errno %d (%s)", fPath.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   while (1) {
      const struct dirent* de = readdir(dir);
      if (!de)
         break;

      const char* dn = de->d_name;

      const char* s;

      s = strstr(dn, "mhf_");
      if (!s || s!=dn)
         continue;

      s = strstr(dn, ".dat");
      if (!s || s[4]!=0)
         continue;

      flist.push_back(dn);
   }

   closedir(dir);
   dir = NULL;

   //printf("Found %d files\n", flist.size());

   // note: use reverse iterator to sort filenames by time, newest first
   std::sort(flist.rbegin(), flist.rend());

   if (0) {
      printf("file names sorted by time:\n");
      for (unsigned i=0; i<flist.size(); i++) {
         printf("%d: %s\n", i, flist[i].c_str());
      }
   }

   for (unsigned i=0; i<flist.size(); i++) {
      std::string file_name = fPath + "/" + flist[i];
      bool dupe = false;
      for (unsigned ss=0; ss<sv->size(); ss++) {
         HsFileSchema* ssp = (HsFileSchema*)(*sv)[ss];
         if (file_name == ssp->file_name) {
            dupe = true;
            break;
         }
      }
      if (dupe)
         continue;
      HsFileSchema* s = read_file_schema(file_name.c_str());
      if (!s)
         continue;
      sv->add(s);
      // NB: this function always loads all data
      // if (event_name)
      //    if (s->event_name == event_name)
      //       if (s->time_from <= timestamp)
      //          break;
   }

   return HS_SUCCESS;
}

HsSchema* FileHistory::new_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[])
{
   if (fDebug)
      printf("FileHistory::new_event: event [%s], timestamp %s, ntags %d\n", event_name, TimeToString(timestamp).c_str(), ntags);

   int status;

   HsFileSchema* s = (HsFileSchema*)fWriterCurrentSchema.find_event(event_name, timestamp);

   if (!s) {
      status = read_schema(&fWriterCurrentSchema, event_name, timestamp);
      if (status != HS_SUCCESS)
         return NULL;
      s = (HsFileSchema*)fWriterCurrentSchema.find_event(event_name, timestamp);
   }

   bool xdebug = false;

   if (s) { // is existing schema the same as new schema?
      bool same = true;

      if (same)
         if (s->event_name != event_name) {
            if (xdebug)
               printf("AAA: [%s] [%s]!\n", s->event_name.c_str(), event_name);
            same = false;
         }

      if (same)
         if (s->variables.size() != ntags) {
            if (xdebug)
               printf("BBB: event [%s]: ntags: %d -> %d!\n", event_name, (int)s->variables.size(), ntags);
            same = false;
         }

      if (same)
         for (unsigned i=0; i<s->variables.size(); i++) {
            if (s->variables[i].name != tags[i].name) {
               if (xdebug)
                  printf("CCC: event [%s] index %d: name [%s] -> [%s]!\n", event_name, i, s->variables[i].name.c_str(), tags[i].name);
               same = false;
            }
            if (s->variables[i].type != tags[i].type) {
               if (xdebug)
                  printf("DDD: event [%s] index %d: type %d -> %d!\n", event_name, i, s->variables[i].type, tags[i].type);
               same = false;
            }
            if (s->variables[i].n_data != tags[i].n_data) {
               if (xdebug)
                  printf("EEE: event [%s] index %d: n_data %d -> %d!\n", event_name, i, s->variables[i].n_data, tags[i].n_data);
               same = false;
            }
            if (!same)
               break;
         }
      
      if (!same) {
         if (xdebug) {
            printf("*** Schema for event %s has changed!\n", event_name);

            printf("*** Old schema for event [%s] time %s:\n", event_name, TimeToString(timestamp).c_str());
            s->print();
            printf("*** New tags:\n");
            PrintTags(ntags, tags);
         }

         if (fDebug)
            printf("FileHistory::new_event: event [%s], timestamp %s, ntags %d: schema mismatch, starting a new file.\n", event_name, TimeToString(timestamp).c_str(), ntags);

         s = NULL;
      }
   }

   if (s) {
      // maybe this schema is too old - rotate files every so often
      time_t age = timestamp - s->time_from;
      const time_t kDay = 24*60*60;
      const time_t kMonth = 30*kDay;
      //printf("*** age %s (%.1f months), timestamp %s, time_from %s, file %s\n", TimeToString(age).c_str(), (double)age/(double)kMonth, TimeToString(timestamp).c_str(), TimeToString(s->time_from).c_str(), s->file_name.c_str());
      if (age > 6*kMonth) {
         if (fDebug)
            printf("FileHistory::new_event: event [%s], timestamp %s, ntags %d: schema is too old, age %.1f months, starting a new file.\n", event_name, TimeToString(timestamp).c_str(), ntags, (double)age/(double)kMonth);

         //printf("*** age %s (%.1f months), timestamp %s, time_from %s, file %s\n", TimeToString(age).c_str(), (double)age/(double)kMonth, TimeToString(timestamp).c_str(), TimeToString(s->time_from).c_str(), s->file_name.c_str());

         s = NULL;
      }
   }

   if (s) {
      // maybe this file is too big - rotate files to limit maximum size
#if 0
      time_t age = timestamp - s->time_from;
      const time_t kDay = 24*60*60;
      const time_t kMonth = 30*kDay;
      printf("*** age %s (%.1f months), timestamp %s, time_from %s, file %s\n", TimeToString(age).c_str(), (double)age/(double)kMonth, TimeToString(timestamp).c_str(), TimeToString(s->time_from).c_str(), s->file_name.c_str());
      if (age > 6*kMonth) {
         if (fDebug)
            printf("FileHistory::new_event: event [%s], timestamp %s, ntags %d: schema is too old, age %.1f months, starting a new file.\n", event_name, TimeToString(timestamp).c_str(), ntags, (double)age/(double)kMonth);

         s = NULL;
      }
#endif
   }

   if (!s) {
      std::string filename;

      status = create_file(event_name, timestamp, ntags, tags, &filename);
      if (status != HS_SUCCESS)
         return NULL;

      HsFileSchema* ss = read_file_schema(filename.c_str());
      if (!ss)
         return NULL;

      fWriterCurrentSchema.add(ss);

      s = (HsFileSchema*)fWriterCurrentSchema.find_event(event_name, timestamp);

      if (!s) {
         cm_msg(MERROR, "FileHistory::new_event", "Error: Cannot create schema for event \'%s\', see previous messages", event_name);
         return NULL;
      }

      if (xdebug) {
         printf("*** New schema for event [%s] time %s:\n", event_name, TimeToString(timestamp).c_str());
         s->print();
      }
   }

   assert(s != NULL);

   if (0) {
      printf("schema for [%s] is %p\n", event_name, s);
      if (s)
         s->print();
   }

   HsFileSchema* e = new HsFileSchema();

   *e = *s; // make a copy of the schema

   return e;
}

int FileHistory::create_file(const char* event_name, time_t timestamp, int ntags, const TAG tags[], std::string* filenamep)
{
   if (fDebug)
      printf("FileHistory::create_file: event [%s]\n", event_name);

   std::string filename;
   filename += fPath;
   filename += "/";
   filename += "mhf_";
   filename += TimeToString(timestamp);
   filename += "_";
   filename += MidasNameToFileName(event_name);

   std::string try_filename = filename + ".dat";

   FILE *fp = NULL;
   for (int itry=0; itry<10; itry++) {
      if (itry > 0) {
         char s[256];
         sprintf(s, "_%d", rand());
         try_filename = filename + s + ".dat";
      }

      fp = fopen(try_filename.c_str(), "r");
      if (fp != NULL) {
         // this file already exists, try with a different name
         fclose(fp);
         continue;
      }

      fp = fopen(try_filename.c_str(), "w");
      if (fp == NULL) {
         // somehow cannot create this file, try again
         cm_msg(MERROR, "FileHistory::create_file", "Error: Cannot create file \'%s\' for event \'%s\', fopen() errno %d (%s)", try_filename.c_str(), event_name, errno, strerror(errno));
         continue;
      }

      // file opened
      break;
   }

   if (fp == NULL) {
      // somehow cannot create any file, whine!
      cm_msg(MERROR, "FileHistory::create_file", "Error: Cannot create file \'%s\' for event \'%s\'", filename.c_str(), event_name);
      return HS_FILE_ERROR;
   }

   std::string ss;

   ss += "version: 2.0\n";
   ss += "event_name: ";
   ss    += event_name;
   ss    += "\n";
   ss += "time: ";
   ss    += TimeToString(timestamp);
   ss    += "\n";

   int recsize = 0;

   ss += "tag: /DWORD 1 4 /timestamp\n";
   recsize += 4;

   bool padded = false;
   int offset = 0;

   bool xdebug = false; // (strcmp(event_name, "u_Beam") == 0);

   for (int i=0; i<ntags; i++) {
      int tsize = rpc_tid_size(tags[i].type);
      int n_bytes = tags[i].n_data*tsize;
      int xalign = (offset % tsize);

      if (xdebug)
         printf("tag %d, tsize %d, n_bytes %d, xalign %d\n", i, tsize, n_bytes, xalign);

      // looks like history data does not do alignement and padding
      if (0 && xalign != 0) {
         padded = true;
         int pad_bytes = tsize - xalign;
assert(pad_bytes > 0);

         ss += "tag: ";
         ss    += "XPAD";
         ss    += " ";
         ss    += SmallIntToString(1);
         ss    += " ";
         ss    += SmallIntToString(pad_bytes);
         ss    += " ";
         ss    += "pad_";
         ss    += SmallIntToString(i);
         ss    += "\n";

         offset += pad_bytes;
         recsize += pad_bytes;

         assert((offset % tsize) == 0);
         fprintf(stderr, "FIXME: need to debug padding!\n");
         //abort();
      }

      ss += "tag: ";
      ss    += rpc_tid_name(tags[i].type);
      ss    += " ";
      ss    += SmallIntToString(tags[i].n_data);
      ss    += " ";
      ss    += SmallIntToString(n_bytes);
      ss    += " ";
      ss    += tags[i].name;
      ss    += "\n";

      recsize += n_bytes;
      offset += n_bytes;
   }

   ss += "record_size: ";
   ss    += SmallIntToString(recsize);
   ss    += "\n";

   // reserve space for "data_offset: ..."
   int sslength = ss.length() + 127;

   int block = 1024;
   int nb = (sslength + block - 1)/block;
   int data_offset = block * nb;

   ss += "data_offset: ";
   ss    += SmallIntToString(data_offset);
   ss    += "\n";

   fprintf(fp, "%s", ss.c_str());

   fclose(fp);

   if (1 && padded) {
      printf("Schema in file %s has padding:\n", try_filename.c_str());
      printf("%s", ss.c_str());
   }

   if (filenamep)
      *filenamep = try_filename;

   return HS_SUCCESS;
}

HsFileSchema* FileHistory::read_file_schema(const char* filename)
{
   if (fDebug)
      printf("FileHistory::read_file_schema: file %s\n", filename);

   FILE* fp = fopen(filename, "r");
   if (!fp) {
      cm_msg(MERROR, "FileHistory::read_file_schema", "Cannot read \'%s\', fopen() errno %d (%s)", filename, errno, strerror(errno));
      return NULL;
   }

   HsFileSchema* s = NULL;

   // File format looks like this:
   // version: 2.0
   // event_name: u_Beam
   // time: 1023174012
   // tag: /DWORD 1 4 /timestamp
   // tag: FLOAT 1 4 B1
   // ...
   // tag: FLOAT 1 4 Ref Heater
   // record_size: 84
   // data_offset: 1024

   int rd_recsize = 0;
   int offset = 0;

   while (1) {
      char buf[1024];
      char* b = fgets(buf, sizeof(buf), fp);

      //printf("read: %s\n", b);

      if (!b)
         break; // end of file

      char*bb;

      bb = strchr(b, '\n');
      if (bb)
         *bb = 0;

      bb = strchr(b, '\r');
      if (bb)
         *bb = 0;

      bb = strstr(b, "version: 2.0");
      if (bb == b) {
         s = new HsFileSchema();
         assert(s);

         s->file_name = filename;
         continue;
      }

      if (!s) {
         cm_msg(MERROR, "FileHistory::read_file_schema", "Malformed history schema in \'%s\', maybe it is not a history file", filename);
         if (s)
            delete s;
         return NULL;
      }

      bb = strstr(b, "event_name: ");
      if (bb == b) {
         s->event_name = bb + 12;
         continue;
      }

      bb = strstr(b, "time: ");
      if (bb == b) {
         s->time_from = strtoul(bb + 6, NULL, 10);
         continue;
      }

      // tag format is like this:
      //
      // tag: FLOAT 1 4 Ref Heater
      //
      // "FLOAT" is the MIDAS type, "/DWORD" is special tag for the timestamp
      // "1" is the number of array elements
      // "4" is the total tag size in bytes (n_data*tid_size)
      // "Ref Heater" is the tag name

      bb = strstr(b, "tag: ");
      if (bb == b) {
         bb += 5; // now points to the tag MIDAS type
         const char* midas_type = bb;
         char* bbb = strchr(bb, ' ');
         if (bbb) {
            *bbb = 0;
            HsSchemaEntry t;
            t.type = midas_tid(midas_type);
            bbb++;
            while (*bbb == ' ')
               bbb++;
            if (*bbb) {
               t.n_data = strtoul(bbb, &bbb, 10);
               while (*bbb == ' ')
                  bbb++;
               if (*bbb) {
                  t.n_bytes = strtoul(bbb, &bbb, 10);
                  while (*bbb == ' ')
                     bbb++;
                  t.name = bbb;
               }
            }

            if (midas_type[0] != '/') {
               s->variables.push_back(t);
               s->offsets.push_back(offset);
               offset += t.n_bytes;
            }

            rd_recsize += t.n_bytes;
         }
         continue;
      }

      bb = strstr(b, "record_size: ");
      if (bb == b) {
         s->record_size = atoi(bb + 12);
         continue;
      }

      bb = strstr(b, "data_offset: ");
      if (bb == b) {
         s->data_offset = atoi(bb + 12);
         // data offset is the last entry in the file
         break;
      }
   }

   fclose(fp);

   if (rd_recsize != s->record_size) {
      cm_msg(MERROR, "FileHistory::read_file_schema", "Record size mismatch in history schema from \'%s\', file says %d while total of all tags is %d", filename, s->record_size, rd_recsize);
      if (s)
         delete s;
      return NULL;
   }

   if (!s) {
      cm_msg(MERROR, "FileHistory::read_file_schema", "Could not read history schema from \'%s\', maybe it is not a history file", filename);
      if (s)
         delete s;
      return NULL;
   }

   if (fDebug > 1)
      s->print();

   return s;
}

////////////////////////////////////////////////////////
//               Factory constructors                 //
////////////////////////////////////////////////////////

MidasHistoryInterface* MakeMidasHistorySqlite()
{
#ifdef HAVE_SQLITE
   return new SqliteHistory();
#else
   return NULL;
#endif
}

MidasHistoryInterface* MakeMidasHistoryMysql()
{
#ifdef HAVE_MYSQL
   return new MysqlHistory();
#else
   return NULL;
#endif
}

MidasHistoryInterface* MakeMidasHistoryFile()
{
   return new FileHistory();
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

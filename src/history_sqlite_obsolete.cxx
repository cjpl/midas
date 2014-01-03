/********************************************************************\

  Name:         history_sqlite.cxx
  Created by:   Konstantin Olchanski

  Contents:     Interface class for writing MIDAS history data to SQLITE databases

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <errno.h>

#include <vector>
#include <string>
#include <map>

#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

typedef std::vector<int> IntVector;
typedef std::vector<std::string> StringVector;
typedef std::map<std::string, std::string>  StringMap;
typedef std::map<std::string, StringVector> StringVectorMap;
typedef std::map<std::string, StringMap>    StringMapMap;

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

#ifdef HAVE_SQLITE

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
   int ii = i;

   if (i == 0)
      return "0";

   assert(i > 0);

   std::string v;
   while (i) {
      char c = '0' + i%10;
      i /= 10;
      v = c + v;
   }

   printf("SmallIntToString: %d -> %s\n", ii, v.c_str());

   return v;
}

static void xcmp(const std::string& x, const char* y)
{
   printf("->%s<-\n", y);
   printf("=>%s<=\n", x.c_str());
}

////////////////////////////////////////
// Definitions extracted from midas.c //
////////////////////////////////////////

/********************************************************************/
/* data type sizes */
static const int tid_size[] = {
   0,                           /* tid == 0 not defined                               */
   1,                           /* TID_BYTE      unsigned byte         0       255    */
   1,                           /* TID_SBYTE     signed byte         -128      127    */
   1,                           /* TID_CHAR      single character      0       255    */
   2,                           /* TID_WORD      two bytes             0      65535   */
   2,                           /* TID_SHORT     signed word        -32768    32767   */
   4,                           /* TID_DWORD     four bytes            0      2^32-1  */
   4,                           /* TID_INT       signed dword        -2^31    2^31-1  */
   4,                           /* TID_BOOL      four bytes bool       0        1     */
   4,                           /* TID_FLOAT     4 Byte float format                  */
   8,                           /* TID_DOUBLE    8 Byte float format                  */
   1,                           /* TID_BITFIELD  8 Bits Bitfield    00000000 11111111 */
   0,                           /* TID_STRING    zero terminated string               */
   0,                           /* TID_ARRAY     variable length array of unkown type */
   0,                           /* TID_STRUCT    C structure                          */
   0,                           /* TID_KEY       key in online database               */
   0                            /* TID_LINK      link in online database              */
};

/* data type names */
static const char *tid_name[] = {
   "NULL",
   "BYTE",
   "SBYTE",
   "CHAR",
   "WORD",
   "SHORT",
   "DWORD",
   "INT",
   "BOOL",
   "FLOAT",
   "DOUBLE",
   "BITFIELD",
   "STRING",
   "ARRAY",
   "STRUCT",
   "KEY",
   "LINK"
};

static const char *sql_type[] = {
   "xxxINVALIDxxxNULL", // TID_NULL
   "INTEGER",  // TID_BYTE
   "INTEGER",           // TID_SBYTE
   "TEXT",              // TID_CHAR
   "INTEGER", // TID_WORD
   "INTEGER",          // TID_SHORT
   "INTEGER",  // TID_DWORD
   "INTEGER",           // TID_INT
   "INTEGER",           // TID_BOOL
   "REAL",             // TID_FLOAT
   "REAL",            // TID_DOUBLE
   "INTEGER",  // TID_BITFIELD
   "TEXT",           // TID_STRING
   "xxxINVALIDxxxARRAY",
   "xxxINVALIDxxxSTRUCT",
   "xxxINVALIDxxxKEY",
   "xxxINVALIDxxxLINK"
};

////////////////////////////////////////
//    Handling of data types          //
////////////////////////////////////////

static const char* midasTypeName(int tid)
{
   if (tid>=0 && tid<15)
      return tid_name[tid];

   static char buf[1024];
   sprintf(buf, "TID_%d", tid);
   return buf;
}

static const char* midas2sqlType(int tid)
{
   assert(tid>=0);
   assert(tid<15);
   return sql_type[tid];
}

static int sql2midasType(const char* name)
{
   for (int tid=0; tid<15; tid++)
      if (strcasecmp(name, sql_type[tid])==0)
         return tid;
   printf("sql2midasType: Cannot convert SQL data type \'%s\' to a MIDAS data type!\n", name);
   return 0;
}

static bool isCompatible(int tid, const char* sqlType)
{
   if (0)
      printf("compare types midas \'%s\'=\'%s\' and sql \'%s\'\n", midasTypeName(tid), midas2sqlType(tid), sqlType);

   if (sql2midasType(sqlType) == tid)
      return true;

   if (strcasecmp(midas2sqlType(tid), sqlType) == 0)
      return true;

   // permit writing FLOAT into DOUBLE
   if (tid==TID_FLOAT && strcmp(sqlType, "double")==0)
      return true;

   // T2K quirk!
   // permit writing BYTE into signed tinyint
   if (tid==TID_BYTE && strcmp(sqlType, "tinyint")==0)
      return true;

   // T2K quirk!
   // permit writing WORD into signed tinyint
   if (tid==TID_WORD && strcmp(sqlType, "tinyint")==0)
      return true;

   return false;
}

////////////////////////////////////////
//        SQLITE includes             //
////////////////////////////////////////

#include <sqlite3.h>

//////////////////////////////////////////
//   Sqlite: SQL access                 //
//////////////////////////////////////////

typedef std::map<std::string, sqlite3*> DbMap;

class Sqlite
{
public:
   int  fDebug;
   bool fIsConnected;

   std::string fPath;

   DbMap fMap;

   sqlite3* fTempDB;

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
   int Prepare(const char* table_name, const char* sql, sqlite3_stmt** st);
   int Step(sqlite3_stmt* st);
   int Finalize(sqlite3_stmt** st);

   int OpenTransaction(const char* table_name);
   int CloseTransaction(const char* table_name);

   const char* GetText(sqlite3_stmt* st, int column);
   int64_t GetInt64(sqlite3_stmt* st, int column);
   double  GetDouble(sqlite3_stmt* st, int column);
};

const char* Sqlite::GetText(sqlite3_stmt* st, int column)
{
   return (const char*)sqlite3_column_text(st, column);
}

int64_t Sqlite::GetInt64(sqlite3_stmt* st, int column)
{
   return sqlite3_column_int64(st, column);
}

double Sqlite::GetDouble(sqlite3_stmt* st, int column)
{
   return sqlite3_column_double(st, column);
}

Sqlite::Sqlite() // ctor
{
   fIsConnected = false;
   fTempDB = NULL;
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

int Sqlite::CloseTransaction(const char* table_name)
{
   int status = Exec(table_name, "COMMIT TRANSACTION");
   return status;
}

int Sqlite::Prepare(const char* table_name, const char* sql, sqlite3_stmt** st)
{
   sqlite3* db = GetTable(table_name);
   if (!db)
      return DB_FILE_ERROR;

   if (fDebug)
      printf("Sqlite::Prepare(%s, %s)\n", table_name, sql);

   assert(fTempDB==NULL);
   fTempDB = db;

#if SQLITE_VERSION_NUMBER >= 3006020
   int status = sqlite3_prepare_v2(db, sql, strlen(sql), st, NULL);
#else
#warning Missing sqlite3_prepare_v2()!
   int status = sqlite3_prepare(db, sql, strlen(sql), st, NULL);
#endif

   if (status == SQLITE_OK)
      return DB_SUCCESS;

   std::string sqlstring = sql;
   cm_msg(MERROR, "Sqlite::Prepare", "Table %s: sqlite3_prepare_v2(%s...) error %d (%s)", table_name, sqlstring.substr(0,60).c_str(), status, xsqlite3_errstr(db, status));

   fTempDB = NULL;

   return DB_FILE_ERROR;
}

int Sqlite::Step(sqlite3_stmt* st)
{
   if (0 && fDebug)
      printf("Sqlite::Step()\n");

   assert(fTempDB);

   int status = sqlite3_step(st);

   if (status == SQLITE_DONE)
      return DB_NO_MORE_SUBKEYS;

   if (status == SQLITE_ROW)
      return DB_SUCCESS;

   cm_msg(MERROR, "Sqlite::Step", "sqlite3_step() error %d (%s)", status, xsqlite3_errstr(fTempDB, status));

   return DB_FILE_ERROR;
}

int Sqlite::Finalize(sqlite3_stmt** st)
{
   if (0 && fDebug)
      printf("Sqlite::Finalize()\n");

   assert(fTempDB);

   int status = sqlite3_finalize(*st);

   if (status != SQLITE_OK) {
      cm_msg(MERROR, "Sqlite::Finalize", "sqlite3_finalize() error %d (%s)", status, xsqlite3_errstr(fTempDB, status));

      fTempDB = NULL;
      st = NULL; // FIXME: maybe a memory leak?
      return DB_FILE_ERROR;
   }

   fTempDB = NULL;

   st = NULL;
   return DB_SUCCESS;
}

#include <dirent.h>

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

      sqlite3_stmt* st;

      status = Prepare(table_name, cmd, &st);
      if (status != DB_SUCCESS)
         continue;
      
      while (1) {
         status = Step(st);
         if (status != DB_SUCCESS)
            break;
         
         const char* tablename = GetText(st, 0);
         //printf("table [%s]\n", tablename);
         plist->push_back(tablename);
      }
      
      status = Finalize(&st);
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

   sqlite3_stmt* st;

   status = Prepare(table, cmd.c_str(), &st);
   if (status != DB_SUCCESS)
      return status;

   while (1) {
      status = Step(st);
      if (status != DB_SUCCESS)
         break;

      const char* colname = GetText(st, 1);
      const char* coltype = GetText(st, 2);
      //printf("column [%s] [%s]\n", colname, coltype);
      plist->push_back(colname); // column name
      plist->push_back(coltype); // column type
   }

   status = Finalize(&st);

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
      std::string sqlstring = sql;
      cm_msg(MERROR, "Sqlite::Exec", "Table %s: sqlite3_exec(%s...) error %d (%s)", table_name, sqlstring.substr(0,60).c_str(), status, errmsg);
      sqlite3_free(errmsg);
      return DB_FILE_ERROR;
   }

   return DB_SUCCESS;
}

//////////////////////////////////////////
//        Done with SQL stuff           //
//////////////////////////////////////////

////////////////////////////////////////////////////////
//  Data structures to keep track of Events and Tags  //
////////////////////////////////////////////////////////

struct Tag
{
   std::string column_name;
   int   offset;
   TAG   tag;
   bool  create;
};

struct Event
{
   std::string event_name;
   std::string table_name;
   std::vector<Tag> tags;
   bool  active;
   int transactionCount;
   
   Event() // ctor
   {
      active = false;
      transactionCount = 0;
   }
   
   ~Event() // dtor
   {
      active = false;
      assert(transactionCount == 0);
   }
};

static void PrintTags(int ntags, const TAG tags[])
{
   for (int i=0; i<ntags; i++)
      printf("tag %d: %s %s[%d]\n", i, midasTypeName(tags[i].type), tags[i].name, tags[i].n_data);
}

int WriteEvent(Sqlite* sql, Event *e, time_t t, const char*buf, int size)
{
   //printf("event %d, time %s", rec.event_id, ctime(&t));

   const char* table_name = e->table_name.c_str();

   int n  = e->tags.size();
   
   std::string tags;
   std::string values;
   
   //if (n>0)
   //  printf(" %s", ctime(&t));
   
   for (int i=0; i<n; i++) {
      const Tag*t = &e->tags[i];
      
      if (t) {
         int offset = t->offset;
         void* ptr = (void*)(buf+offset);

         int arraySize = t->tag.n_data;
         
         for (int j=0; j<arraySize; j++) {
            tags   += ", ";
            values += ", ";
            
            if (arraySize <= 1)
               tags += "\'" + t->column_name + "\'";
            else {
               tags += "\'" + t->column_name;
               char s[256];
               sprintf(s,"_%d", j);
               tags += s;
               tags += "\'";
            }
                
            char s[1024];
            
            switch (t->tag.type) {
            default:
               sprintf(s, "unknownType%d", t->tag.type);
               break;
            case 1: /* BYTE */
               sprintf(s, "%u",((uint8_t*)ptr)[j]);
               break;
            case 2: /* SBYTE */
               sprintf(s, "%d",((int8_t*)ptr)[j]);
               break;
            case 3: /* CHAR */
               sprintf(s, "\'%c\'",((char*)ptr)[j]);
               break;
            case 4: /* WORD */
               sprintf(s, "%u",((uint16_t*)ptr)[j]);
               break;
            case 5: /* SHORT */
               sprintf(s, "%d",((int16_t*)ptr)[j]);
               break;
            case 6: /* DWORD */
               sprintf(s, "%u",((uint32_t*)ptr)[j]);
               break;
            case 7: /* INT */
               sprintf(s, "%d",((int32_t*)ptr)[j]);
               break;
            case 8: /* BOOL */
               sprintf(s, "%u",((uint32_t*)ptr)[j]);
               break;
            case 9: /* FLOAT */
               sprintf(s, "\'%.8g\'",((float*)ptr)[j]);
               break;
            case 10: /* DOUBLE */
               sprintf(s, "\'%.16g\'",((double*)ptr)[j]);
                    break;
            }
            
            values += s;
         }
      }
   }

   // 2001-02-16 20:38:40.1
   char s[1024];
   strftime(s,sizeof(s)-1,"%Y-%m-%d %H:%M:%S.0",localtime(&t));

   std::string cmd;
   cmd = "INSERT INTO \'";
   cmd += table_name;
   cmd += "\' (_t_time, _i_time";
   cmd += tags;
   cmd += ") VALUES (\'";
   cmd += s;
   cmd += "\', \'";
   cmd += TimeToString(t);
   cmd += "\'";
   cmd += values;
   cmd += ");";

   if (e->transactionCount == 0)
      sql->OpenTransaction(table_name);

   e->transactionCount++;
   
   int status = sql->Exec(table_name, cmd.c_str());

   if (e->transactionCount > 100000) {
      //printf("flush table %s\n", table_name);
      sql->CloseTransaction(table_name);
      e->transactionCount = 0;
   }

   if (status != DB_SUCCESS) {
      return status;
   }

   return HS_SUCCESS;
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

////////////////////////////////////////////////////////
//    Implementation of the MidasHistoryInterface     //
////////////////////////////////////////////////////////

class SqliteHistory: public MidasHistoryInterface
{
public:
   Sqlite *fSql;
   int fDebug;
   std::string fConnectString;
   std::vector<Event*> fEvents;

   SqliteHistory()
   {
      fDebug = 0;
      fSql = new Sqlite();
      hs_clear_cache();
   }

   ~SqliteHistory()
   {
      hs_disconnect();
      delete fSql;
      fSql = NULL;
   }

   int hs_set_debug(int debug)
   {
      int old = fDebug;
      fDebug = debug;
      fSql->fDebug = debug;
      return old;
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

   std::string GetEventName(const char* table_name)
   {
      int status;

      std::string cmd;
      cmd = "SELECT event_name, _i_time FROM \'_event_name_";
      cmd += table_name;
      cmd += "\' WHERE table_name='";
      cmd += table_name;
      cmd += "' ORDER BY _i_time ASC;";

      sqlite3_stmt* st;

      status = fSql->Prepare(table_name, cmd.c_str(), &st);
      
      if (status != DB_SUCCESS) {
         return table_name;
      }
      
      std::string xevent_name;
      time_t      xevent_time = 0;
      
      while (1) {
         status = fSql->Step(st);
         
         if (status != DB_SUCCESS)
            break;
         
         // NOTE: SQL "SELECT ORDER BY _i_time ASC" returns data sorted by time
         // in this code we use the data from the last data row
         // so if multiple rows are present, the latest one is used
         
         xevent_name  = fSql->GetText(st, 0);
         xevent_time  = fSql->GetInt64(st, 1);
         
         //printf("read event name [%s] time %d\n", xevent_name.c_str(), (int)xevent_time);
      }
      
      status = fSql->Finalize(&st);

      return xevent_name;
   }

   std::string GetTableName(const char* event_name)
   {
      std::vector<std::string> tables;
      int status = fSql->ListTables(&tables);
      if (status != DB_SUCCESS)
         return "";
 
      for (unsigned i=0; i<tables.size(); i++) {
         const char* tt = tables[i].c_str();

         const char *s = strstr(tt, "_event_name");
         if (!s || s!=tt)
            continue;

         const char* tn = tt + 12;
         //printf("looking for event name [%s] maybe [%s], found [%s] [%s]\n", event_name, maybe_table_name, tt, tn);

         std::string xevent_name = GetEventName(tn);

         if (strcmp(xevent_name.c_str(), event_name) == 0) {
            //printf("table name for event [%s] is [%s]\n", event_name, tn);
            //assert(!"here!");
            return tn;
         }
      }

      return "";
   }

   int GetColumnNames(const char* table_name, StringMap *ptag2col, StringMap *pcol2tag)
   {
      std::string cmd;
      cmd = "SELECT column_name, tag_name, _i_time FROM \'_column_names_";
      cmd += table_name;
      cmd += "\' WHERE table_name='";
      cmd += table_name;
      cmd += "' ORDER BY _i_time ASC;";

      sqlite3_stmt* st;
      
      int status = fSql->Prepare(table_name, cmd.c_str(), &st);
      
      if (status != DB_SUCCESS) {
         return status;
      }
   
      while (1) {
         status = fSql->Step(st);
         
         if (status != DB_SUCCESS)
            break;
         
         // NOTE: SQL "SELECT ORDER BY _i_time ASC" returns data sorted by time
         // in this code we use the data from the last data row
         // so if multiple rows are present, the latest one is used

         std::string col_name  = fSql->GetText(st, 0);
         std::string tag_name  = fSql->GetText(st, 1);
         time_t xxx_time  = fSql->GetInt64(st, 2);

         if (ptag2col)
            (*ptag2col)[tag_name] = col_name;

         if (pcol2tag)
            (*pcol2tag)[col_name] = tag_name;
         
         if (fDebug>1)
            printf("read table [%s] column [%s] tag name [%s] time %d\n", table_name, col_name.c_str(), tag_name.c_str(), (int)xxx_time);
      }

      status = fSql->Finalize(&st);

      return HS_SUCCESS;
   }

   ////////////////////////////////////////////////////////
   //             Functions used by mlogger              //
   ////////////////////////////////////////////////////////

   int hs_define_event(const char* event_name, int ntags, const TAG tags[])
   {
      int status;

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

               if (fEvents[i]->transactionCount > 0) {
                  status = fSql->CloseTransaction(fEvents[i]->table_name.c_str());
                  fEvents[i]->transactionCount = 0;
               }

               delete fEvents[i];
               fEvents[i] = NULL;
            }

      // check for duplicate tag names
      for (int i=0; i<ntags; i++) {
         for (int j=i+1; j<ntags; j++) {
            if (strcmp(tags[i].name, tags[j].name) == 0) {
               cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\' has duplicate tag name \'%s\' at indices %d and %d", event_name, tags[i].name, i, j);
               return HS_FILE_ERROR;
            }
         }
      }

      Event* e = new Event();

      e->active = true;
      e->event_name = event_name;

      std::string table_name = GetTableName(event_name);

      if (table_name.length() < 1) {
         table_name = MidasNameToSqlName(event_name);
      }

      e->table_name = table_name;

      std::vector<std::string> columns;

      status = fSql->ListColumns(e->table_name.c_str(), &columns);
      if (status != DB_SUCCESS)
         return status;

      StringMap colnames;

      if (columns.size() > 0) {
         status = GetColumnNames(table_name.c_str(), &colnames, NULL);
         if (status != HS_SUCCESS)
            return HS_FILE_ERROR;
      }
         
      double now = time(NULL);

      status = fSql->OpenTransaction(e->table_name.c_str());
      if (status != DB_SUCCESS)
         return HS_FILE_ERROR;

      if (columns.size() <= 0) {
         // SQL table does not exist

         std::string cmd;

         cmd = "CREATE TABLE \'";
         cmd += e->table_name;
         cmd += "\' (_t_time TIMESTAMP NOT NULL, _i_time INTEGER NOT NULL);";

         status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
         
         cmd = "CREATE INDEX \'";
         cmd += e->table_name;
         cmd += "_i_time_index\' ON \'";
         cmd += e->table_name;
         cmd += "\' (_i_time ASC);";

         status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
         
         cmd = "CREATE INDEX \'";
         cmd += e->table_name;
         cmd += "_t_time_index\' ON \'";
         cmd += e->table_name;
         cmd += "\' (_t_time);";

         status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
         
         cmd = "CREATE TABLE \'_event_name_";
         cmd += e->table_name;
         cmd += "\' (table_name TEXT NOT NULL, event_name TEXT NOT NULL, _i_time INTEGER NOT NULL);";

         status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
         
         cmd = "INSERT INTO \'_event_name_";
         cmd += e->table_name;
         cmd += "\' (table_name, event_name, _i_time) VALUES (\'";
         cmd += e->table_name;
         cmd += "\', \'";
         cmd += e->event_name;
         cmd += "\', \'";
         cmd += TimeToString(now);
         cmd += "\');";

         status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
         
         cmd = "CREATE TABLE \'_column_names_";
         cmd += e->table_name;
         cmd += "\' (table_name TEXT NOT NULL, column_name TEXT NOT NULL, tag_name TEXT NOT NULL, tag_type TEXT NOT NULL, column_type TEXT NOT NULL, _i_time INTEGER NOT NULL);";

         status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
      }

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

            std::string colname = colnames[tagname];

            if (colname.length() < 1) {
               // no column name entry for this tag name
               colname = maybe_colname;
            }

            // check for duplicate column names and for incompatible column data type

            while (1) {
               bool dupe = false;
               
               for (unsigned k=0; k<e->tags.size(); k++) {
                  if (colname == e->tags[k].column_name) {
                     printf("hs_define_event: event [%s] tag [%s] duplicate column name [%s] with tag [%s]\n", event_name, tagname.c_str(), colname.c_str(), e->tags[k].tag.name);
                     dupe = true;
                     break;
                  }
               }

               // if duplicate column name, change it, try again
               if (dupe) {
                  char s[256];
                  sprintf(s, "_%d", rand());
                  colname += s;
                  continue;
               }
               
               bool compatible = true;

               for (size_t k=0; k<columns.size(); k+=2) {
                  if (colname == columns[k]) {
                     // column already exists, check it's data type
                     
                     compatible = isCompatible(tagtype, columns[k+1].c_str());
                     
                     //printf("column \'%s\', data type %s\n", colname.c_str(), columns[k+1].c_str());

                     //printf("hs_define_event: event [%s] tag [%s] type %d (%s), column [%s] type [%s] compatible %d\n", event_name, tagname.c_str(), tagtype, midasTypeName(tagtype), colname.c_str(), columns[k+1].c_str(), compatible);
                     
                     break;
                  }
               }

               // if incompatible column data type, change column name, try again
               if (!compatible) {
                  char s[256];
                  sprintf(s, "_%d", rand());
                  colname += s;
                  continue;
               }

               break;
            }

            std::string coltype = midas2sqlType(tagtype);

            // if column name had changed (or is not listed in _column_names), add it
            if (colname != colnames[tagname]) {
               std::string cmd;
               cmd = "INSERT INTO \'_column_names_";
               cmd += e->table_name;
               cmd += "\' (table_name, column_name, tag_name, tag_type, column_type, _i_time) VALUES (\'";
               cmd += e->table_name;
               cmd += "\', \'";
               cmd += colname;
               cmd += "\', \'";
               cmd += tagname;
               cmd += "\', \'";
               cmd += midasTypeName(tagtype);
               cmd += "\', \'";
               cmd += coltype;
               cmd += "\', \'";
               cmd += TimeToString(now);
               cmd += "\');";
               status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
            }

            // if SQL column does not exist, create it

            bool column_exists = false;
            for (size_t k=0; k<columns.size(); k+=2) {
               if (colname == columns[k]) {
                  column_exists = true;
                  break;
               }
            }

            if (!column_exists) {
               std::string cmd;
               cmd = "ALTER TABLE \'";
               cmd += e->table_name;
               cmd += "\' ADD COLUMN \'";
               cmd += colname;
               cmd += "\' ";
               cmd += coltype;
               cmd += ";";
               
               status = fSql->Exec(e->table_name.c_str(), cmd.c_str());
               
               if (status != DB_SUCCESS) {
                  e->active = false;
                  return HS_FILE_ERROR;
               }
            }

            Tag t;
            t.column_name = colname;
            t.offset = offset;
            t.tag = tags[i];
            t.tag.n_data = 1;
            e->tags.push_back(t);
            int size = tid_size[tags[i].type];
            offset += size;
         }
      }

      status = fSql->CloseTransaction(e->table_name.c_str());
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }

      // find empty slot in events list
      for (unsigned int i=0; i<fEvents.size(); i++)
         if (!fEvents[i]) {
            fEvents[i] = e;
            e = NULL;
            break;
         }

      // if no empty slots, add at the end
      if (e)
         fEvents.push_back(e);

      return HS_SUCCESS;
   }

   int hs_write_event(const char* event_name, time_t timestamp, int buffer_size, const char* buffer)
   {
      if (fDebug)
         printf("hs_write_event: write event \'%s\', time %d, size %d\n", event_name, (int)timestamp, buffer_size);

      Event *e = NULL;

      // find this event
      for (size_t i=0; i<fEvents.size(); i++)
         if (fEvents[i]->event_name == event_name) {
            e = fEvents[i];
            break;
         }

      // not found
      if (!e)
         return HS_UNDEFINED_EVENT;

      // deactivated because of error?
      if (!e->active)
         return HS_FILE_ERROR;

      int status = WriteEvent(fSql, e, timestamp, buffer, buffer_size);

      // if could not write to SQL?
      if (status != HS_SUCCESS) {
         // otherwise, deactivate this event

         e->active = false;

         cm_msg(MERROR, "hs_write_event", "Event \'%s\' disabled after write error %d into SQL database \'%s\'", event_name, status, fConnectString.c_str());

         return HS_FILE_ERROR;
      }

      return HS_SUCCESS;
   }

   int hs_flush_buffers()
   {
      int status = HS_SUCCESS;

      if (fDebug)
         printf("hs_flush_buffers!\n");

      for (unsigned int i=0; i<fEvents.size(); i++)
         if (fEvents[i])
            if (fEvents[i]->transactionCount > 0) {
               int xstatus = fSql->CloseTransaction(fEvents[i]->table_name.c_str());
               if (xstatus != HS_SUCCESS)
                  status = xstatus;
               fEvents[i]->transactionCount = 0;
            }

      return status;
   }

   ////////////////////////////////////////////////////////
   //             Functions used by mhttpd               //
   ////////////////////////////////////////////////////////

   std::vector<std::string> fEventsCache;
   std::vector<std::string> fTablesCache;
   StringMap                fTableNamesCache;
   StringVectorMap          fColumnsCache;
   StringMapMap             fColumnNamesCache;

   int hs_clear_cache()
   {
      if (fDebug)
         printf("hs_clear_cache!\n");

      fTablesCache.clear();
      fEventsCache.clear();
      fTableNamesCache.clear();
      fColumnsCache.clear();
      fColumnNamesCache.clear();

      return HS_SUCCESS;
   }

   void ReadTablesCache()
   {
      if (fDebug)
         printf("ReadTablesCache!\n");

      fTablesCache.clear();

      fSql->ListTables(&fTablesCache);
   }

   void ReadEventsCache()
   {
      if (fDebug)
         printf("ReadEventsCache!\n");

      if (fTablesCache.size() == 0)
         ReadTablesCache();

         for (unsigned i=0; i<fTablesCache.size(); i++) {
            const char* tn = fTablesCache[i].c_str();

            const char* s;
            s = strstr(tn, "_event_name_");
            if (s == tn)
               continue;
            s = strstr(tn, "_column_names_");
            if (s == tn)
               continue;

            std::string en = GetEventName(tn);

            bool dupe = false;
            for (unsigned j=0; j<fEventsCache.size(); j++)
               if (fEventsCache[j] == en) {
                  dupe = true;
                  break;
               }

            fTableNamesCache[tn] = en;

            if (!dupe)
               fEventsCache.push_back(en);
         }
   }

   void ReadColumnsCache(const char* table_name)
   {
      fColumnsCache[table_name].clear();
      fSql->ListColumns(table_name, &fColumnsCache[table_name]);

      fColumnNamesCache[table_name].clear();

      // assign default column name same as SQL column name
      for (unsigned i=0; i<fColumnsCache[table_name].size(); i+=2) {
         const std::string cn = fColumnsCache[table_name][i];
         fColumnNamesCache[table_name][cn] = cn;
      }

      // read column names from SQL
      GetColumnNames(table_name, NULL, &fColumnNamesCache[table_name]);
   }

   int hs_get_events(std::vector<std::string> *pevents)
   {
      if (fDebug)
         printf("hs_get_events!\n");

      if (fEventsCache.size() == 0) {
         ReadEventsCache();
      }

      assert(pevents);
      *pevents = fEventsCache;
      
      return HS_SUCCESS;
   }
      
   int hs_get_tags(const char* event_name, std::vector<TAG> *ptags)
   {
      if (fDebug)
         printf("hs_get_tags for [%s]\n", event_name);

      assert(ptags);

      if (fEventsCache.size() == 0)
         ReadEventsCache();

      bool not_found = true;
      for (unsigned i=0; i<fTablesCache.size(); i++) {
         const std::string tn = fTablesCache[i].c_str();
         const char* en = fTableNamesCache[tn].c_str();

         if (strcmp(tn.c_str(), event_name) != 0) // match table name?
            if (strcmp(en, event_name) != 0) // match event name?
               continue;
         not_found = false;

         if (fColumnsCache[tn].size() == 0)
            ReadColumnsCache(tn.c_str());
            
         for (unsigned int j=0; j<fColumnsCache[tn].size(); j+=2) {
            const std::string cn = fColumnsCache[tn][j];
            const std::string ct = fColumnsCache[tn][j+1];
            if (cn == "_t_time")
               continue;
            if (cn == "_i_time")
               continue;

            const char* tagname = fColumnNamesCache[tn][cn].c_str();

            //printf("event_name [%s], table_name [%s], column name [%s], tag name [%s]\n", event_name, tn.c_str(), cn.c_str(), tagname);

            if (strlen(tagname) < 1)
               tagname = cn.c_str();

            bool dupe = false;
            
            for (unsigned k=0; k<ptags->size(); k++)
               if (strcmp((*ptags)[k].name, tagname) == 0) {
                  dupe = true;
                  break;
               }

            if (!dupe) {
               TAG t;
               STRLCPY(t.name, tagname);
               t.type = sql2midasType(ct.c_str());
               t.n_data = 1;
               
               ptags->push_back(t);
            }
         }
      }

      if (fDebug) {
         printf("hs_get_tags: %d tags\n", (int)ptags->size());
         for (unsigned i=0; i<ptags->size(); i++) {
            printf("  tag[%d]: %s[%d] type %d\n", i, (*ptags)[i].name, (*ptags)[i].n_data, (*ptags)[i].type);
         }
      }

      if (not_found)
         return HS_UNDEFINED_EVENT;
      
      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   struct XItem {
      XItem(const char* tn, const char* cn) : tableName(tn), columnName(cn) { }; // ctor

      std::string tableName;
      std::string columnName;
   };

   typedef std::vector<XItem> XItemVector;

   int FindItem(const char* const event_name, const char* const tag_name, int tag_index, XItemVector* result)
   {
      if (fEventsCache.size() == 0)
         ReadEventsCache();

      // new-style event name: "equipment_name/variable_name:tag_name"
      // old-style event name: "equipment_name:tag_name" ("variable_name" is missing)
      bool newStyleEventName = (strchr(event_name, '/')!=NULL);

      for (unsigned i=0; i<fTablesCache.size(); i++) {
         const char* tn = fTablesCache[i].c_str();
         const char* en = fTableNamesCache[tn].c_str();
         if (strlen(en) < 1)
            en = tn;

         //printf("looking for event_name [%s], try table [%s] event name [%s], new style [%d]\n", event_name, tn, en, newStyleEventName);

         if (strcmp(tn, event_name) == 0) {
            // match
         } else if (strcmp(en, event_name) == 0) {
            // match
         } else if (newStyleEventName) {
            // mismatch
            continue;
         } else { // for old style names, need more parsing
            bool match = false;

            const char* s = en;
            for (int j=0; s[j]; j++) {

               if ((event_name[j]==0) && (s[j]=='/')) {
                  match = true;
                  break;
               }
               
               if ((event_name[j]==0) && (s[j]=='_')) {
                  match = true;
                  break;
               }
               
               if (event_name[j]==0) {
                  match = false;
                  break;
               }
               
               if (tolower(event_name[j]) != tolower(s[j])) {
                  match = false;
                  break;
               }
            }

            if (!match)
               continue;
         }

         if (fColumnNamesCache[tn].size() == 0)
            ReadColumnsCache(tn);

         for (unsigned j=0; j<fColumnsCache[tn].size(); j+=2) {
            const char* cn = fColumnsCache[tn][j].c_str();
            const char* name = fColumnNamesCache[tn][cn].c_str();
            if (strlen(name) < 1)
               name = cn;

            char alt_tag_name[1024]; // maybe this is an array without "Names"?
            sprintf(alt_tag_name, "%s[%d]", tag_name, tag_index);

            //printf("  looking for tag [%s] alt [%s], try column name [%s]\n", tag_name, alt_tag_name, name);

            if (strcmp(cn, tag_name) != 0)
               if (strcmp(name, tag_name) != 0)
                  if (strcmp(name, alt_tag_name) != 0)
                     continue;

            //printf("**found table [%s] column [%s]\n", tn, cn);

            result->push_back(XItem(tn, cn));
         }
      }

      return HS_SUCCESS;
   }
   

   /*------------------------------------------------------------------*/

   int hs_get_last_written(time_t start_time, int num_var, const char* const event_name[], const char* const tag_name[], const int var_index[], time_t last_written[])
   {
      double dstart_time = start_time;

      if (fDebug) {
         printf("hs_get_last_written: start time %.0f, num_var %d\n", dstart_time, num_var);
      }

      for (int i=0; i<num_var; i++) {
         last_written[i] = 0;

         XItemVector xitem;
         FindItem(event_name[i], tag_name[i], var_index[i], &xitem);

         if (fDebug) {
            printf("For event [%s] tag [%s] index [%d] found %d entries: ", event_name[i], tag_name[i], var_index[i], (int)xitem.size());
            for (unsigned j=0; j<xitem.size(); j++) {
               printf(" table [%s], column [%s]", xitem[j].tableName.c_str(), xitem[j].columnName.c_str());
            }
            printf("\n");
         }

         if (xitem.size() < 1) // not found
            continue;

         time_t tt = 0;

         for (unsigned j=0; j<xitem.size(); j++) {
            const char* tn = xitem[j].tableName.c_str();
            const char* cn = xitem[j].columnName.c_str();

            std::string cmd;
            cmd = "SELECT _i_time, \"";
            cmd += cn;
            cmd += "\" FROM \'";
            cmd += tn;
            cmd += "\' WHERE _i_time <= ";
            cmd += TimeToString(dstart_time);
            cmd += " ORDER BY _i_time DESC LIMIT 2;";

            sqlite3_stmt *st;
            
            int status = fSql->Prepare(tn, cmd.c_str(), &st);
            
            if (fDebug) {
               printf("hs_get_last_written: event \"%s\", tag \"%s\", index %d: Read table \"%s\" column \"%s\": status %d\n",
                      event_name[i], tag_name[i], var_index[i],
                      tn,
                      cn,
                      status
                      );
            }
            
            if (status != DB_SUCCESS) {
               continue;
            }
            
            /* Loop through the rows in the result-set */
            
            for (int k=0; ; k++) {
               status = fSql->Step(st);
               if (status != DB_SUCCESS)
                  break;
               
               time_t t = fSql->GetInt64(st, 0);
               double v = fSql->GetDouble(st, 1);

               if (t > start_time)
                  continue;

               if (0) {
                  if (k<10)
                     printf("count %d, t %d, v %f, tt %d\n", k, (int)t, v, (int)tt);
               }

               if (t > tt)
                  tt = t;
            }
            
            fSql->Finalize(&st);
         }
         
         last_written[i] = tt;
      }

      if (fDebug) {
         printf("hs_get_last_written: start time %.0f, num_var %d\n", dstart_time, num_var);
         for (int i=0; i<num_var; i++) {
            printf("  event [%s] tag [%s] index [%d] last_written %d\n", event_name[i], tag_name[i], var_index[i], (int)last_written[i]);
         }
      }

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_read_table(double start_time, double end_time,
                     const std::string& tn,
                     int num_var, const XItemVector vvv[],
                     MidasHistoryBufferInterface* buffer[],
                     int xstatus[])
   {
      if (fDebug)
         printf("hs_read_table: table [%s], start %f, end %f, dt %f\n", tn.c_str(), start_time, end_time, end_time-start_time);

#if 0
      if (1) {
         printf("For event [%s] tag [%s] index [%d] found %d entries: ", event_name, tag_name, tag_index, (int)xitem.size());
         for (unsigned j=0; j<xitem.size(); j++) {
            printf(" table [%s], column [%s]", xitem[j].tableName.c_str(), xitem[j].columnName.c_str());
         }
         printf("\n");
      }

      if (xitem.size() < 1) // not found
         return HS_UNDEFINED_VAR;
#endif

      StringVector colnames;
      IntVector colindex;

      std::string collist;

      for (int i=0; i<num_var; i++) {
         for (unsigned j=0; j<vvv[i].size(); j++) {
            if (vvv[i][j].tableName == tn) {
               colnames.push_back(vvv[i][j].columnName);
               colindex.push_back(i);

               if (collist.length() > 0)
                  collist += ", ";
               collist += std::string("\"") + vvv[i][j].columnName + "\"";
            }
         }
      }

      int numcol = (int)colnames.size();

      if (fDebug) {
         printf("From table [%s]\n", tn.c_str());
         for (int k=0; k<numcol; k++) {
            printf("read column [%s] var index [%d]\n", colnames[k].c_str(), colindex[k]);
         }
      }

      std::string cmd;
      cmd += "SELECT _i_time, ";
      cmd += collist;
      cmd += " FROM \'";
      cmd += tn;
      cmd += "\' WHERE _i_time>=";
      cmd += TimeToString(start_time);
      cmd += " and _i_time<=";
      cmd += TimeToString(end_time);
      cmd += " ORDER BY _i_time;";

      if (fDebug) {
         printf("hs_read_table: cmd %s\n", cmd.c_str());
      }
      
      sqlite3_stmt *st;
      
      int status = fSql->Prepare(tn.c_str(), cmd.c_str(), &st);
      
      if (fDebug) {
         printf("hs_read_table: Read table \"%s\" columns \"%s\": status %d\n", tn.c_str(), collist.c_str(), status);
      }
         
      if (status != DB_SUCCESS) {
         //for (unsigned k=0; k<colnames.size(); k++)
         //xstatus[colindex[k]] = HS_FILE_ERROR;
         return HS_FILE_ERROR;
      }
      
      /* Loop through the rows in the result-set */

      int count = 0;

      while (1) {
         status = fSql->Step(st);
         if (status != DB_SUCCESS)
            break;

         count++;
         
         time_t t = fSql->GetInt64(st, 0);
         
         if (t < start_time || t > end_time)
            continue;

         for (int k=0; k<numcol; k++) {
            double v = fSql->GetDouble(st, 1+k);

            //printf("Column %d, index %d, Row %d, time %d, value %f\n", k, colindex[k], count, t, v);

            buffer[colindex[k]]->Add(t, v);
         }
      }
      
      fSql->Finalize(&st);
   
      for (unsigned k=0; k<colnames.size(); k++)
         xstatus[colindex[k]] = HS_SUCCESS;
      
      if (fDebug)
         printf("hs_read_table: read %d rows\n", count);

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_read_buffer(time_t start_time, time_t end_time,
                      int num_var, const char* const event_name[], const char* const tag_name[], const int tag_index[],
                      MidasHistoryBufferInterface* buffer[],
                      int status[])
   {
      if (fDebug)
         printf("hs_read_buffer: %d variables\n", num_var);

      if (!fSql->IsConnected())
         return HS_FILE_ERROR;

      for (int i=0; i<num_var; i++) {
         status[i] = HS_FILE_ERROR;
      }

      XItemVector vvv[num_var];
      StringVector ttt;

      for (int i=0; i<num_var; i++) {

         if (event_name[i]==NULL) {
            status[i] = HS_UNDEFINED_EVENT;
            continue;
         }

         FindItem(event_name[i], tag_name[i], tag_index[i], &vvv[i]);
         
         for (unsigned j=0; j<vvv[i].size(); j++) {
            bool found = false;
            for (unsigned k=0; k<ttt.size(); k++)
               if (ttt[k] == vvv[i][j].tableName) {
                  found = true;
                  break;
               }

            if (!found)
               ttt.push_back(vvv[i][j].tableName);
         }
      }

      if (fDebug) {
         for (int i=0; i<num_var; i++) {
            printf("For event [%s] tag [%s] index [%d] found %d entries: ", event_name[i], tag_name[i], tag_index[i], (int)vvv[i].size());
            for (unsigned j=0; j<vvv[i].size(); j++) {
               printf(" table [%s], column [%s]", vvv[i][j].tableName.c_str(), vvv[i][j].columnName.c_str());
            }
            printf("\n");
         }
         printf("Tables:");
         for (unsigned k=0; k<ttt.size(); k++)
            printf(" %s", ttt[k].c_str());
         printf("\n");
      }

      for (unsigned k=0; k<ttt.size(); k++) {
         const std::string tn =  ttt[k].c_str();

         hs_read_table(start_time, end_time,
                       tn,
                       num_var, vvv,
                       buffer,
                       status);
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
               const char* const event_name[], const char* const tag_name[], const int var_index[],
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
                              num_var, event_name, tag_name, var_index,
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
                      int num_var, const char* const event_name[], const char* const tag_name[], const int var_index[],
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
                              num_var, event_name, tag_name, var_index,
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

////////////////////////////////////////////////////////
//               Factory constructors                 //
////////////////////////////////////////////////////////

MidasHistoryInterface* MakeMidasHistorySqlite()
{
   return new SqliteHistory();
}

#else // HAVE_SQLITE

MidasHistoryInterface* MakeMidasHistorySqlite()
{
   return NULL;
}

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

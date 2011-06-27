/********************************************************************\

  Name:         history_sql.cxx
  Created by:   Konstantin Olchanski

  Contents:     Interface class for writing MIDAS history data to SQL databases

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <vector>
#include <string>

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

// SQL types
#ifdef HAVE_ODBC
static const char *sql_type_pgsql[] = {
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

static const char *sql_type_mysql[] = {
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

////////////////////////////////////////
//    Handling of data types          //
////////////////////////////////////////

static const char **sql_type = NULL;

static const char* midasTypeName(int tid)
{
   assert(tid>=0);
   assert(tid<15);
   return tid_name[tid];
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

/////////////////////////////////////////////////
//    Base class for access to SQL functions   //
/////////////////////////////////////////////////

class SqlBase
{
public:
  virtual int SetDebug(int debug) = 0;
  virtual int Connect(const char* dsn = 0) = 0;
  virtual int Disconnect() = 0;
  virtual bool IsConnected() = 0;
  virtual int Exec(const char* sql) = 0;
  virtual int GetNumRows() = 0;
  virtual int GetNumColumns() = 0;
  virtual int Fetch() = 0;
  virtual int Done() = 0;
  virtual int ListTables(std::vector<std::string> *plist) = 0;
  virtual int ListColumns(const char* table, std::vector<std::string> *plist) = 0;
  virtual const char* GetColumn(int icol) = 0;
  virtual ~SqlBase() { }; // virtual dtor
};

////////////////////////////////////////////////////////////////////
//   SqlDebug: for debugging: write all SQL commands to stdout   //
////////////////////////////////////////////////////////////////////

class SqlDebug: public SqlBase
{
public:
   FILE *fp;
   bool fIsConnected;
   int fDebug;
   
public:
   
   SqlDebug() // ctor
   {
      fp = NULL;
      fIsConnected = false;
   }

   ~SqlDebug() // dtor
   {
      if (fp)
         fclose(fp);
      fp = NULL;
   }

   int SetDebug(int debug)
   {
      int old_debug = fDebug;
      fDebug = debug;
      return old_debug;
   }

   int Connect(const char* filename = NULL)
   {
      if (!filename)
         filename = "/dev/fd/1";
      fp = fopen(filename, "w");
      assert(fp);
      sql_type = sql_type_mysql;
      fIsConnected = true;
      return DB_SUCCESS;
   }

   int Exec(const char* sql)
   {
      fprintf(fp, "%s\n", sql);
      return DB_SUCCESS;
   }
   
   int Disconnect()
   {
      // do nothing
      fIsConnected = false;
      return DB_SUCCESS;
   }
   
   bool IsConnected()
   {
      return fIsConnected;
   }

   int GetNumRows() { return DB_SUCCESS; }
   int GetNumColumns() { return DB_SUCCESS; }
   int Fetch() { return DB_NO_MORE_SUBKEYS; }
   int Done() { return DB_SUCCESS; }
   int ListTables(std::vector<std::string> *plist) { return DB_SUCCESS; };
   int ListColumns(const char* table, std::vector<std::string> *plist) { return DB_SUCCESS; };
   const char* GetColumn(int icol) { return NULL; };
};

#ifdef HAVE_ODBC

////////////////////////////////////////
//          ODBC includes             //
////////////////////////////////////////

// MIDAS defines collide with ODBC

#define DWORD DWORD_xxx
#define BOOL  BOOL_xxx

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

//////////////////////////////////////////
//   SqlODBC: SQL access through ODBC   //
//////////////////////////////////////////

class SqlODBC: public SqlBase
{
public:
   bool fIsConnected;

   std::string fDSN;

   int fDebug;

   SQLHENV  fEnv;
   SQLHDBC  fDB;
   SQLHSTMT fStmt;

   SqlODBC(); // ctor
   ~SqlODBC(); // dtor

   int SetDebug(int debug)
   {
      int old_debug = fDebug;
      fDebug = debug;
      return old_debug;
   }

   int Connect(const char* dsn);
   int Disconnect();
   bool IsConnected();

   int ListTables(std::vector<std::string> *plist);
   int ListColumns(const char* table_name, std::vector<std::string> *plist);

   int Exec(const char* sql);

   int GetNumRows();
   int GetNumColumns();
   int Fetch();
   const char* GetColumn(int icol);
   int Done();

protected:
   void ReportErrors(const char* from, const char* sqlfunc, int status);
   int DecodeError();
};

SqlODBC::SqlODBC() // ctor
{
   fIsConnected = false;
   fDebug = 0;
}

SqlODBC::~SqlODBC() // dtor
{
   Disconnect();
}

int SqlODBC::Connect(const char* dsn)
{
   if (fIsConnected)
      Disconnect();

   fDSN = dsn;

   int status = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &fEnv);

   if (!SQL_SUCCEEDED(status)) {
      cm_msg(MERROR, "SqlODBC::Connect", "SQLAllocHandle(SQL_HANDLE_ENV) error %d", status);
      return DB_FILE_ERROR;
   }

   status = SQLSetEnvAttr(fEnv,
                          SQL_ATTR_ODBC_VERSION, 
                          (void*)SQL_OV_ODBC2,
                          0); 
   if (!SQL_SUCCEEDED(status)) {
      cm_msg(MERROR, "SqlODBC::Connect", "SQLSetEnvAttr() error %d", status);
      SQLFreeHandle(SQL_HANDLE_ENV, fEnv);
      return DB_FILE_ERROR;
   }

   status = SQLAllocHandle(SQL_HANDLE_DBC, fEnv, &fDB); 
   if (!SQL_SUCCEEDED(status)) {
      cm_msg(MERROR, "SqlODBC::Connect", "SQLAllocHandle(SQL_HANDLE_DBC) error %d", status);
      SQLFreeHandle(SQL_HANDLE_ENV, fEnv);
      return DB_FILE_ERROR;
   }

   SQLSetConnectAttr(fDB, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);

   if (0) {
      // connect to PgSQL database
      
      sql_type = sql_type_pgsql;
      status = SQLConnect(fDB, (SQLCHAR*) dsn, SQL_NTS,
                          (SQLCHAR*) "xxx", SQL_NTS,
                          (SQLCHAR*) "", SQL_NTS);
   }
   
   if (1) {
      // connect to MySQL database
      
      sql_type = sql_type_mysql;
      status = SQLConnect(fDB, (SQLCHAR*) dsn, SQL_NTS,
                          (SQLCHAR*) NULL, SQL_NTS,
                          (SQLCHAR*) NULL, SQL_NTS);
   }

   if ((status != SQL_SUCCESS) && (status != SQL_SUCCESS_WITH_INFO)) {
      SQLINTEGER    V_OD_err;
      SQLSMALLINT   V_OD_mlen;
      SQLCHAR       V_OD_stat[10]; // Status SQL
      SQLCHAR       V_OD_msg[200];
      
      SQLGetDiagRec(SQL_HANDLE_DBC, fDB, 1, V_OD_stat, &V_OD_err, V_OD_msg, 100, &V_OD_mlen);
      cm_msg(MERROR, "SqlODBC::Connect", "SQLConnect() error %d, %s (%d)", status, V_OD_msg,V_OD_err);
      SQLFreeHandle(SQL_HANDLE_ENV, fEnv);
      return DB_FILE_ERROR;
   }

   SQLAllocHandle(SQL_HANDLE_STMT, fDB, &fStmt);

   if (fDebug)
      cm_msg(MINFO, "SqlODBC::Connect", "Connected to ODBC database DSN \'%s\'", dsn);

   fIsConnected = true;

   return DB_SUCCESS;
}

int SqlODBC::Disconnect()
{
   if (!fIsConnected)
      return DB_SUCCESS;

   SQLDisconnect(fDB);

   SQLFreeHandle(SQL_HANDLE_DBC,  fDB);
   SQLFreeHandle(SQL_HANDLE_STMT, fStmt);
   SQLFreeHandle(SQL_HANDLE_ENV,  fEnv);

   fIsConnected = false;

   return DB_SUCCESS;
}

bool SqlODBC::IsConnected()
{
   return fIsConnected;
}

void SqlODBC::ReportErrors(const char* from, const char* sqlfunc, int status)
{
   if (fDebug)
      printf("%s: %s error %d\n", from, sqlfunc, status);

   for (int i=1; ; i++) {
      SQLCHAR 		 state[10]; // Status SQL
      SQLINTEGER		 error;
      SQLCHAR              message[1024];
      SQLSMALLINT		 mlen;
      
      status = SQLGetDiagRec(SQL_HANDLE_STMT,
                             fStmt,
                             i,
                             state,
                             &error,
                             message,
                             sizeof(message),
                             &mlen);
      
      if (status == SQL_NO_DATA)
         break;

      if (!SQL_SUCCEEDED(status)) {
         cm_msg(MERROR, "SqlODBC::ReportErrors", "SQLGetDiagRec() error %d", status);
         break;
      }
      
      if (1 || (error != 1060) && (error != 1050)) {
         if (fDebug)
            printf("%s: %s error: state: \'%s\', message: \'%s\', native error: %d\n", from, sqlfunc, state, message, (int)error);
         cm_msg(MERROR, from, "%s error: state: \'%s\', message: \'%s\', native error: %d", sqlfunc, state, message, (int)error);
      }
   }
}

int SqlODBC::DecodeError()
{
   // returns:
   // DB_SUCCESS
   // DB_NO_KEY
   // DB_KEY_EXIST

   for (int i=1; ; i++) {
      SQLCHAR 		 state[10]; // Status SQL
      SQLINTEGER		 error;
      SQLCHAR              message[1024];
      SQLSMALLINT		 mlen;

      error = 0;
      
      int status = SQLGetDiagRec(SQL_HANDLE_STMT,
                                 fStmt,
                                 i,
                                 state,
                                 &error,
                                 message,
                                 sizeof(message),
                                 &mlen);
      
      if (status == SQL_NO_DATA)
         return DB_SUCCESS;

      if (error==1146)
         return DB_NO_KEY;

      if (error==1050)
         return DB_KEY_EXIST;
   }
}

int SqlODBC::ListTables(std::vector<std::string> *plist)
{
   if (!fIsConnected)
      return DB_FILE_ERROR;

   for (int i=0; i<2; i++) {
      if (fDebug)
         printf("SqlODBC::ListTables!\n");

      /* Retrieve a list of tables */
      int status = SQLTables(fStmt, NULL, 0, NULL, 0, NULL, 0, (SQLCHAR*)"TABLE", SQL_NTS);

      if (SQL_SUCCEEDED(status))
         break;

      if (fDebug)
         printf("SqlODBC::ListTables: SQLTables() error %d\n", status);
      
      ReportErrors("SqlODBC::ListTables", "SQLTables()", status);

      status = DecodeError();

      //if (status == DB_NO_KEY)
      //   return status;
      //
      //if (status == DB_KEY_EXIST)
      //  return status;
      
      cm_msg(MINFO, "SqlODBC::ListTables", "Reconnecting to ODBC database DSN \'%s\'", fDSN.c_str());

      // try to reconnect
      std::string dsn = fDSN;
      Disconnect();
      status = Connect(dsn.c_str());

      if (!fIsConnected) {
         cm_msg(MERROR, "SqlODBC::ListTables", "Cannot reconnect to ODBC database DSN \'%s\', status %d. Database is down?", fDSN.c_str(), status);
         return DB_FILE_ERROR;
      }

      cm_msg(MINFO, "SqlODBC::ListTables", "Reconnected to ODBC database DSN \'%s\'", fDSN.c_str());
   }

   int ncols = GetNumColumns();
   int nrows = GetNumRows();

   if (ncols <= 0 || nrows <= 0) {
      cm_msg(MERROR, "SqlODBC::ListTables", "Error: SQLTables() returned unexpected number of columns %d or number of rows %d", ncols, nrows);
   }

   int row = 0;
   while (1) {
      int status = Fetch();
      if (status != DB_SUCCESS)
         break;

      if (0) {
         printf("row %d: ", row);
         for (int i=1; i<=ncols; i++) {
            const char* s = GetColumn(i);
            printf("[%s]", s);
         }
         printf("\n");
         row++;
      }

      plist->push_back(GetColumn(3));
   }

   Done();

   return DB_SUCCESS;
}

int SqlODBC::ListColumns(const char* table, std::vector<std::string> *plist)
{
   if (!fIsConnected)
      return DB_FILE_ERROR;

   for (int i=0; i<2; i++) {
      if (fDebug)
         printf("SqlODBC::ListColumns for table \'%s\'\n", table);

      /* Retrieve a list of columns */
      int status = SQLColumns(fStmt, NULL, 0, NULL, 0, (SQLCHAR*)table, SQL_NTS, NULL, 0);

      if (SQL_SUCCEEDED(status))
         break;

      if (fDebug)
         printf("SqlODBC::ListColumns: SQLColumns(%s) error %d\n", table, status);
      
      ReportErrors("SqlODBC::ListColumns", "SQLColumns()", status);

      status = DecodeError();

      //if (status == DB_NO_KEY)
      //   return status;
      //
      //if (status == DB_KEY_EXIST)
      //  return status;
      
      cm_msg(MINFO, "SqlODBC::ListColumns", "Reconnecting to ODBC database DSN \'%s\'", fDSN.c_str());

      // try to reconnect
      std::string dsn = fDSN;
      Disconnect();
      status = Connect(dsn.c_str());

      if (!fIsConnected) {
         cm_msg(MERROR, "SqlODBC::ListColumns", "Cannot reconnect to ODBC database DSN \'%s\', status %d. Database is down?", fDSN.c_str(), status);
         return DB_FILE_ERROR;
      }

      cm_msg(MINFO, "SqlODBC::ListColumns", "Reconnected to ODBC database DSN \'%s\'", fDSN.c_str());
   }

   int ncols = GetNumColumns();
   int nrows = GetNumRows(); // nrows seems to be always "-1"

   if (ncols <= 0 /*|| nrows <= 0*/) {
      cm_msg(MERROR, "SqlODBC::ListColumns", "Error: SQLColumns(\'%s\') returned unexpected number of columns %d or number of rows %d", table, ncols, nrows);
   }

   //printf("get columns [%s]: status %d, ncols %d, nrows %d\n", table, status, ncols, nrows);

   int row = 0;
   while (1) {
      int status = Fetch();
      if (status != DB_SUCCESS)
         break;

      if (0) {
         printf("row %d: ", row);
         for (int i=1; i<=ncols; i++) {
            const char* s = GetColumn(i);
            printf("[%s]", s);
         }
         printf("\n");
         row++;
      }

      plist->push_back(GetColumn(4)); // column name
      plist->push_back(GetColumn(6)); // column type
   }

   Done();

   return DB_SUCCESS;
}

int SqlODBC::Exec(const char* sql)
{
   // return values:
   // DB_SUCCESS
   // DB_FILE_ERROR: not connected
   // DB_NO_KEY: "table not found"

   if (!fIsConnected)
      return DB_FILE_ERROR;
    
   int status;

   for (int i=0; i<2; i++) {
      if (fDebug)
         printf("SqlODBC::Exec: %s\n", sql);

      status = SQLExecDirect(fStmt,(SQLCHAR*)sql,SQL_NTS);

      if (SQL_SUCCEEDED(status)) {
         return DB_SUCCESS;
      }

      if (fDebug)
         printf("SqlODBC::Exec: SQLExecDirect() error %d: SQL command: \"%s\"\n", status, sql);
      
      ReportErrors("SqlODBC::Exec", "SQLExecDirect()", status);

      status = DecodeError();

      if (status == DB_NO_KEY)
         return status;
      
      if (status == DB_KEY_EXIST)
         return status;
      
      cm_msg(MINFO, "SqlODBC::Exec", "Reconnecting to ODBC database DSN \'%s\'", fDSN.c_str());

      // try to reconnect
      std::string dsn = fDSN;
      Disconnect();
      status = Connect(dsn.c_str());

      if (!fIsConnected) {
         cm_msg(MERROR, "SqlODBC::Exec", "Cannot reconnect to ODBC database DSN \'%s\', status %d. Database is down?", fDSN.c_str(), status);
         return DB_FILE_ERROR;
      }

      cm_msg(MINFO, "SqlODBC::Exec", "Reconnected to ODBC database DSN \'%s\'", fDSN.c_str());
   }

   return DB_SUCCESS;
}

int SqlODBC::GetNumRows()
{
   SQLLEN nrows = 0;
   /* How many rows are there */
   int status = SQLRowCount(fStmt, &nrows);
   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::GetNumRow", "SQLRowCount()", status);
      return -1;
   }
   return nrows;
}

int SqlODBC::GetNumColumns()
{
   SQLSMALLINT ncols = 0;
   /* How many columns are there */
   int status = SQLNumResultCols(fStmt, &ncols);
   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::GetNumColumns", "SQLNumResultCols()", status);
      return -1;
   }
   return ncols;
}

int SqlODBC::Fetch()
{
   int status = SQLFetch(fStmt);

   if (status == SQL_NO_DATA)
      return DB_NO_MORE_SUBKEYS;

   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::Fetch", "SQLFetch()", status);
      return DB_FILE_ERROR;
   }

   return DB_SUCCESS;
}

int SqlODBC::Done()
{
   int status = SQLCloseCursor(fStmt);
   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::Done", "SQLCloseCursor()", status);
      return DB_FILE_ERROR;
   }
   return DB_SUCCESS;
}

const char* SqlODBC::GetColumn(int icol)
{
  static char buf[1024];
  SQLLEN indicator;
  int status = SQLGetData(fStmt, icol, SQL_C_CHAR, buf, sizeof(buf), &indicator);

  if (!SQL_SUCCEEDED(status)) {
    return NULL;
  }

  if (indicator == SQL_NULL_DATA)
    return NULL;

  return buf;
}

#endif

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
   
   Event() // ctor
   {
      active = false;
   }
   
   ~Event() // dtor
   {
      active = false;
   }
};

static void PrintTags(int ntags, const TAG tags[])
{
   for (int i=0; i<ntags; i++)
      printf("tag %d: %s %s[%d]\n", i, midasTypeName(tags[i].type), tags[i].name, tags[i].n_data);
}

int WriteEvent(SqlBase* sql, Event *e, time_t t, const char*buf, int size)
{
   //printf("event %d, time %s", rec.event_id, ctime(&t));

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
               tags += t->column_name;
            else {
               tags += t->column_name;
               char s[256];
               sprintf(s,"_%d", j);
               tags += s;
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
   
   char sss[102400];
   sprintf(sss, "INSERT INTO %s (_t_time, _i_time%s) VALUES (\'%s\', \'%d\'%s);",
           e->table_name.c_str(),
           tags.c_str(),
           s,
           (int)t,
           values.c_str());

   int status = sql->Exec(sss);
   
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

struct IndexEntryTag
{
   std::string tag_name;
   std::string column_name;
   int timestamp;
};

struct IndexEntry
{
   std::string event_name;
   std::string table_name;
   int timestamp;
   std::vector<IndexEntryTag> tags;
   std::vector<TAG> tags_cache;
};

std::vector<IndexEntry*> gHistoryIndex;

static void PrintIndex()
{
   for (unsigned i=0; i<gHistoryIndex.size(); i++) {
      IndexEntry *e = gHistoryIndex[i];

      printf("entry %d: [%s] [%s], time %d, tags\n", i, e->event_name.c_str(), e->table_name.c_str(), e->timestamp);

      for (unsigned j=0; j<e->tags.size(); j++)
         printf("  tag %d: [%s] [%s], time %d\n", j, e->tags[j].tag_name.c_str(), e->tags[j].column_name.c_str(), e->tags[j].timestamp);
   }
}

static IndexEntry* FindIndexByTableName(const char* table_name)
{
   for (unsigned i=0; i<gHistoryIndex.size(); i++)
      if (equal_ustring(gHistoryIndex[i]->table_name.c_str(), table_name)) {
         return gHistoryIndex[i];
      }
   return NULL;
}

static IndexEntry* FindIndexByEventName(const char* event_name)
{
   for (unsigned i=0; i<gHistoryIndex.size(); i++)
      if (equal_ustring(gHistoryIndex[i]->event_name.c_str(), event_name)) {
         return gHistoryIndex[i];
      }
   return NULL;
}

static IndexEntryTag* FindIndexByTagName(IndexEntry* ie, const char* tag_name)
{
   for (unsigned i=0; i<ie->tags.size(); i++)
      if (equal_ustring(ie->tags[i].tag_name.c_str(), tag_name)) {
         return &ie->tags[i];
      }
   return NULL;
}

static IndexEntryTag* FindIndexByColumnName(IndexEntry* ie, const char* column_name)
{
   for (unsigned i=0; i<ie->tags.size(); i++)
      if (equal_ustring(ie->tags[i].column_name.c_str(), column_name)) {
         return &ie->tags[i];
      }
   return NULL;
}

static int gHaveIndex = true;
static int gHaveIndexAll = false;

static int gTrace = 0;

static int ReadIndex(SqlBase* sql, const char* event_name)
{
   if (gTrace)
      printf("ReadIndex [%s]\n", event_name);

   if (!gHaveIndex)
      return HS_FILE_ERROR;

   if (gHaveIndexAll)
      return HS_SUCCESS;

   if (gTrace)
      printf("ReadIndex: reading index for event [%s]\n", event_name);

   //event_name = NULL;

   char cmd[256];

   if (event_name)
      sprintf(cmd, "SELECT event_name, table_name, tag_name, column_name, itimestamp FROM _history_index where event_name=\'%s\';", event_name);
   else
      sprintf(cmd, "SELECT event_name, table_name, tag_name, column_name, itimestamp FROM _history_index;");
   
   int status = sql->Exec(cmd);

   if (status == DB_NO_KEY) {
      gHaveIndex = false;
      return HS_FILE_ERROR;
   }
   
   if (gTrace) {
      printf("ReadIndex: event %s, Read status %d, nrows: %d\n",
             event_name,
             status,
             sql->GetNumRows());
   }
   
   if (status != SUCCESS)
      return HS_FILE_ERROR;
   
   if (sql->GetNumRows() == 0) {
      sql->Done();
      return HS_FILE_ERROR;
   }
   
   int nrows = sql->GetNumRows();
   int ncols = sql->GetNumColumns();
   
   if (nrows == 0)
      return HS_SUCCESS;
   
   if (gTrace)
      printf("ReadIndex: event %s, nrows: %d, ncols: %d\n",
             event_name,
             nrows, ncols);
   
   if (nrows < 0)
      return HS_FILE_ERROR;
   
   if (ncols < 1)
      return HS_FILE_ERROR;
   
   /* Loop through the rows in the result-set */
   while (1) {
      status = sql->Fetch();
      if (status != DB_SUCCESS)
         break;

      std::string xevent_name  = sql->GetColumn(1);

      const char* p = sql->GetColumn(2);
      if (p) { // table declaration
         std::string xtable_name  = p;
         std::string xtimestamp   = sql->GetColumn(5);
         int timestamp = atoi(xtimestamp.c_str());

         IndexEntry* ie = FindIndexByEventName(xevent_name.c_str());
         if (!ie) {
            ie = new IndexEntry;
            gHistoryIndex.push_back(ie);
            ie->timestamp = timestamp - 1; // make sure we update this entry
         }

         if (timestamp > ie->timestamp) {
            ie->event_name = xevent_name;
            ie->table_name = xtable_name;
            ie->timestamp  = timestamp;
         }

         //printf("%s %s %s %s %s [%s]\n", xevent_name.c_str(), xtable_name.c_str(), "???", "???", xtimestamp.c_str(), p);
         continue;
      }
         
      p = sql->GetColumn(3);
      if (p) { // tag declaration
         std::string xtag_name    = p;
         std::string xcolumn_name = sql->GetColumn(4);
         std::string xtimestamp   = sql->GetColumn(5);
         int timestamp = atoi(xtimestamp.c_str());

         IndexEntry* ie = FindIndexByEventName(xevent_name.c_str());
         if (!ie) {
            ie = new IndexEntry;
            gHistoryIndex.push_back(ie);
            ie->timestamp = 0;
            ie->event_name = xevent_name;
         }

         bool found = false;
         for (unsigned j=0; j<ie->tags.size(); j++)
            if (ie->tags[j].tag_name == xtag_name) {
               if (timestamp > ie->tags[j].timestamp) {
                  ie->tags[j].timestamp = timestamp;
                  ie->tags[j].column_name = xcolumn_name;
               }
               found = true;
               break;
            }

         if (!found) {
            IndexEntryTag it;
            it.tag_name = xtag_name;
            it.column_name = xcolumn_name;
            it.timestamp  = timestamp;
            ie->tags.push_back(it);
         }

         //printf("%s %s %s %s %s\n", xevent_name.c_str(), "???", xtag_name.c_str(), xcolumn_name.c_str(), xtimestamp.c_str());
         continue;
      }

   }

   sql->Done();

   gHaveIndex = true;

   if (event_name == NULL)
      gHaveIndexAll = true;

   //PrintIndex();

   return HS_SUCCESS;
}

////////////////////////////////////////////////////////
//    Implementation of the MidasHistoryInterface     //
////////////////////////////////////////////////////////

class SqlHistory: public MidasHistoryInterface
{
public:
   SqlBase *fSql;
   int fDebug;
   std::string fConnectString;
   int fConnectRetry;
   int fNextConnect;
   std::vector<Event*> fEvents;
   std::vector<std::string> fIndexEvents;
   bool fHaveIndex;
   bool fHaveXIndex;

   SqlHistory(SqlBase* b)
   {
      fDebug = 0;
      fConnectRetry = 0;
      fNextConnect  = 0;
      fSql = b;
      fHaveIndex = false;
      fHaveXIndex = false;
   }

   ~SqlHistory()
   {
      hs_disconnect();
      delete fSql;
      fSql = NULL;
   }

   int hs_set_debug(int debug)
   {
      int old = fDebug;
      fDebug = debug;
      gTrace = debug;
      fSql->SetDebug(debug);
      return old;
   }
   
   int hs_connect(const char* connect_string)
   {
      if (fDebug)
         printf("hs_connect %s!\n", connect_string);

      assert(fSql);

      if (fSql->IsConnected())
         if (strcmp(fConnectString.c_str(), connect_string) == 0)
            return HS_SUCCESS;
    
      hs_disconnect();
    
      fConnectString = connect_string;
    
      if (fDebug)
         printf("hs_connect: connecting to SQL database \'%s\'\n", fConnectString.c_str());
    
      int status = fSql->Connect(fConnectString.c_str());
      if (status != DB_SUCCESS)
         return status;

      std::vector<std::string> tables;

      status = fSql->ListTables(&tables);
      if (status != DB_SUCCESS)
         return status;

      for (unsigned i=0; i<tables.size(); i++) {
         if (tables[i] == "_history_index") {
            fHaveIndex = true;
            break;
         }
      }

      return HS_SUCCESS;
   }

   int hs_disconnect()
   {
      if (fDebug)
         printf("hs_disconnect!\n");
      
      fSql->Disconnect();

      hs_clear_cache();
      
      return HS_SUCCESS;
   }

   int Reconnect()
   {
      if (fDebug)
         printf("Reconnect to SQL database!\n");
      
      fSql->Disconnect();
      fSql->Connect(fConnectString.c_str());
      if (!fSql->IsConnected()) {
         return HS_FILE_ERROR;
      }
      
      return HS_SUCCESS;
   }

   ////////////////////////////////////////////////////////
   //             Internal data caches                   //
   ////////////////////////////////////////////////////////

   int hs_clear_cache()
   {
      if (fDebug)
         printf("hs_clear_cache!\n");

      gHaveIndex = true;
      gHaveIndexAll = false;
      fHaveXIndex = false;

      for (unsigned i=0; i<gHistoryIndex.size(); i++) {
         IndexEntry* ie = gHistoryIndex[i];
         delete ie;
      }
      gHistoryIndex.clear();

      fIndexEvents.clear();

      return HS_SUCCESS;
   }

   int XReadIndex()
   {
      if (fHaveXIndex)
         return HS_SUCCESS;

      if (fDebug)
         printf("XReadIndex!\n");

      std::vector<std::string> tables;

      int status = fSql->ListTables(&tables);
      if (status != DB_SUCCESS)
         return status;
      
      for (unsigned i=0; i<tables.size(); i++) {
         if (tables[i] == "_history_index")
            continue;

         IndexEntry* ie = NULL; //FindEventName(tables[i].c_str());

         if (!ie) {
            ie = new IndexEntry;

            ie->table_name = tables[i];
            ie->event_name = ie->table_name;

            gHistoryIndex.push_back(ie);
         }

         std::vector<std::string> columns;

         status = fSql->ListColumns(ie->table_name.c_str(), &columns);
         if (status != DB_SUCCESS)
            return status;

         for (unsigned int j=0; j<columns.size(); j+=2) {
            if (columns[j] == "_t_time")
               continue;
            if (columns[j] == "_i_time")
               continue;

            IndexEntryTag t;
            t.column_name = columns[j];
            t.tag_name = t.column_name;
            t.timestamp = 0;

            ie->tags.push_back(t);
         }
      }

      fHaveXIndex = true;

      //PrintIndex();

      return HS_SUCCESS;
   }

   ////////////////////////////////////////////////////////
   //             Functions used by mlogger              //
   ////////////////////////////////////////////////////////

   int hs_define_event(const char* event_name, int ntags, const TAG tags[])
   {
      int status;

      if (fDebug) {
         printf("define event [%s] with %d tags:\n", event_name, ntags);
         PrintTags(ntags, tags);
      }

      // delete all events with the same name
      for (unsigned int i=0; i<fEvents.size(); i++)
         if (fEvents[i])
            if (fEvents[i]->event_name == event_name) {
               if (fDebug)
                  printf("deleting exising event %s\n", event_name);
               delete fEvents[i];
               fEvents[i] = NULL;
            }

      Event* e = new Event();

      e->event_name = event_name;

      if (!fHaveIndex) {
         char buf[1024];
         sprintf(buf, "CREATE TABLE _history_index (event_name VARCHAR(256) NOT NULL, table_name VARCHAR(256), tag_name VARCHAR(256), column_name VARCHAR(256), itimestamp INTEGER NOT NULL);");
         int status = fSql->Exec(buf);
         if (status == DB_KEY_EXIST)
            /* do nothing */ ;
         else if (status != DB_SUCCESS)
            return status;
         fHaveIndex = true;
      }

      IndexEntry* ie = FindIndexByEventName(event_name);

      if (!ie) {
         ReadIndex(fSql, event_name);
         ie = FindIndexByEventName(event_name);
      }

      if (!ie) {
         std::string table_name = MidasNameToSqlName(event_name);

         double now = time(NULL);
         
         char sss[102400];
         sprintf(sss, "INSERT INTO _history_index (event_name, table_name, itimestamp) VALUES (\'%s\', \'%s\', \'%.0f\');",
                 event_name,
                 table_name.c_str(),
                 now);
         
         int status = fSql->Exec(sss);
         if (status != DB_SUCCESS)
            return HS_FILE_ERROR;

         ReadIndex(fSql, event_name);
         ie = FindIndexByEventName(event_name);
      }

      if (!ie) {
         cm_msg(MERROR, "hs_define_event", "could not add event name to SQL history index table, see messages");
         return HS_FILE_ERROR;
      }

      e->table_name = ie->table_name;
      e->active = true;

      bool create_event = false;

      int offset = 0;
      for (int i=0; i<ntags; i++) {
         for (unsigned int j=0; j<tags[i].n_data; j++) {
            std::string tagname = tags[i].name;
            std::string colname = MidasNameToSqlName(tags[i].name);

            if (tags[i].n_data > 1) {
               char s[256];
               sprintf(s, "[%d]", j);
               tagname += s;
               
               sprintf(s, "_%d", j);
               colname += s;
            }

            IndexEntryTag *it = FindIndexByTagName(ie, tagname.c_str());
            if (!it) {
               // check for duplicate names

               while (1) {
                  bool dupe = false;

                  for (unsigned i=0; i<e->tags.size(); i++) {
                     if (colname == e->tags[i].column_name) {
                        //printf("duplicate name %s\n", colname.c_str());
                        dupe = true;
                        break;
                     }
                  }

                  if (!dupe)
                     break;

                  char s[256];
                  sprintf(s, "_%d", rand());
                  colname += s;
               }

               // add tag name to column name translation to the history index

               double now = time(NULL);
      
               char sss[102400];
               sprintf(sss, "INSERT INTO _history_index (event_name, tag_name, column_name, itimestamp) VALUES (\'%s\', \'%s\', \'%s\', \'%.0f\');",
                       event_name,
                       tagname.c_str(),
                       colname.c_str(),
                       now);
               
               int status = fSql->Exec(sss);
               if (status != DB_SUCCESS)
                  return HS_FILE_ERROR;

               // reload the history index
               
               ReadIndex(fSql, event_name);
               ie = FindIndexByEventName(event_name);
               assert(ie);
               it = FindIndexByTagName(ie, tagname.c_str());
            }

            if (!it) {
               cm_msg(MERROR, "hs_define_event", "could not add event tags to SQL history index table, see messages");
               return HS_FILE_ERROR;
            }

            Tag t;
            t.column_name = it->column_name;
            t.create = false;
            t.offset = offset;
            t.tag = tags[i];
            t.tag.n_data = 1;
            e->tags.push_back(t);
            int size = tid_size[tags[i].type];
            offset += size;
         }
      }

      std::vector<std::string> columns;

      status = fSql->ListColumns(e->table_name.c_str(), &columns);
      if (status != DB_SUCCESS)
         return status;

      if (columns.size() <= 0)
         create_event = true;

      for (size_t i=0; i<e->tags.size(); i++) {
         // check for duplicate column names
         for (size_t j=i+1; j<e->tags.size(); j++)
            if (e->tags[i].column_name == e->tags[j].column_name) {
               cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\': Duplicated column name \'%s\' from tags %d \'%s\' and %d \'%s\'", event_name, e->tags[i].column_name.c_str(), i, e->tags[i].tag.name, j, e->tags[j].tag.name);
               e->active = false;
               break;
            }

         // check if new column needs to be created
         bool found = false;
         for (size_t j=0; j<columns.size(); j+=2) {
            if (e->tags[i].column_name == columns[j]) {
               // column exists, check data type
               //printf("column \'%s\', data type %s\n", e->tags[i].column_name.c_str(), columns[j+1].c_str());

               if (!isCompatible(e->tags[i].tag.type, columns[j+1].c_str())) {
                  cm_msg(MERROR, "hs_define_event", "Error: History event \'%s\': Incompatible data type for tag \'%s\' type \'%s\', SQL column \'%s\' type \'%s\'", event_name, e->tags[i].tag.name, midasTypeName(e->tags[i].tag.type), columns[j].c_str(), columns[j+1].c_str());
                  e->active = false;
               }

               found = true;
               break;
            }
         }

         if (!found) {
            // create it
            //printf("column \'%s\', data type %s  --- create!\n", e->tags[i].column_name.c_str(), midasTypeName(e->tags[i].tag.type));
            e->tags[i].create = true;
         }
      }

      if (create_event) {
         char buf[1024];
         sprintf(buf, "CREATE TABLE %s (_t_time TIMESTAMP NOT NULL, _i_time INTEGER NOT NULL, INDEX (_i_time), INDEX (_t_time));", e->table_name.c_str());
         status = fSql->Exec(buf);
         if (status != DB_SUCCESS) {
            e->active = false;
            return HS_FILE_ERROR;
         }
      }
   
      for (size_t i=0; i<e->tags.size(); i++)
         if (e->tags[i].create) {
            char buf[1024];
            
            sprintf(buf, "ALTER TABLE %s ADD COLUMN %s %s;",
                    e->table_name.c_str(),
                    e->tags[i].column_name.c_str(),
                    midas2sqlType(e->tags[i].tag.type));
            
            status = fSql->Exec(buf);
            
            if (status != DB_SUCCESS) {
               e->active = false;
               return HS_FILE_ERROR;
            }
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

      // if disconnected, try to reconnect

      if (!fSql->IsConnected()) {
         time_t now = time(NULL);

         // too early to try reconnecting?
         if (fConnectRetry !=0 && now < fNextConnect) {
            return HS_FILE_ERROR;
         }

         cm_msg(MINFO, "hs_write_event", "Trying to reconnect to SQL database \'%s\'", fConnectString.c_str());

         int status = fSql->Connect(fConnectString.c_str());

         if (status != DB_SUCCESS) {

            // first retry in 5 seconds
            if (fConnectRetry == 0)
               fConnectRetry = 5;

            fNextConnect = now + fConnectRetry;

            // exponential backoff
            fConnectRetry *= 2;

            // but no more than every 10 minutes
            if (fConnectRetry > 10*60)
               fConnectRetry = 10*60;

            return HS_FILE_ERROR;
         }

         cm_msg(MINFO, "hs_write_event", "Reconnected to SQL database \'%s\'", fConnectString.c_str());
      }

      fNextConnect = 0;
      fConnectRetry = 0;

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

         // if lost SQL connection, try again later

         if (!fSql->IsConnected()) {
            return HS_FILE_ERROR;
         }

         // otherwise, deactivate this event

         e->active = false;

         cm_msg(MERROR, "hs_write_event", "Event \'%s\' disabled after write error %d into SQL database \'%s\'", event_name, status, fConnectString.c_str());

         return HS_FILE_ERROR;
      }

      return HS_SUCCESS;
   }

   ////////////////////////////////////////////////////////
   //             Functions used by mhttpd               //
   ////////////////////////////////////////////////////////

   int hs_get_events(std::vector<std::string> *pevents)
   {
      if (fDebug)
         printf("hs_get_events!\n");

      if (fIndexEvents.size() == 0) {

         if (fDebug)
            printf("hs_get_events: reading event names!\n");

         ReadIndex(fSql, NULL);

         std::vector<std::string> tables;
         int status = fSql->ListTables(&tables);
         if (status != DB_SUCCESS)
            return status;
 
         for (unsigned i=0; i<tables.size(); i++) {
            if (tables[i] == "_history_index")
               continue;

            IndexEntry* ie = FindIndexByTableName(tables[i].c_str());
            if (!ie) {
               ReadIndex(fSql, NULL);
               ie = FindIndexByTableName(tables[i].c_str());
            }

            if (ie)
               fIndexEvents.push_back(ie->event_name);
            else
               fIndexEvents.push_back(tables[i]);
         }
      }

      assert(pevents);
      *pevents = fIndexEvents;
      
      return HS_SUCCESS;
   }
      
   int hs_get_tags(const char* event_name, std::vector<TAG> *ptags)
   {
      if (fDebug)
         printf("hs_get_tags for [%s]\n", event_name);

      assert(ptags);

      IndexEntry* ie = FindIndexByEventName(event_name);

      if (!ie) {
         ReadIndex(fSql, event_name);
         ie = FindIndexByEventName(event_name);
      }

      if (!ie) {
         XReadIndex();
         ie = FindIndexByEventName(event_name);
      }

      if (!ie)
         return HS_UNDEFINED_EVENT;

      if (ie->tags_cache.size() == 0) {
         if (fDebug)
            printf("hs_get_tags reading tags for [%s]\n", event_name);

         std::string tname = ie->table_name;

         std::vector<std::string> columns;

         int status = fSql->ListColumns(tname.c_str(), &columns);
         if (status != DB_SUCCESS)
            return status;

         if (columns.size() < 1) {
            cm_msg(MERROR, "hs_get_tags", "Cannot get columns for table \'%s\', try to reconnect to the database", tname.c_str());

            int status = Reconnect();
            if (status != HS_SUCCESS)
               return status;

            columns.clear();
            status = fSql->ListColumns(tname.c_str(), &columns);      
            if (status != DB_SUCCESS)
               return status;
         }

         TAG* t = (TAG*)malloc(sizeof(TAG)*columns.size());
         assert(t);

         for (unsigned int j=0; j<columns.size(); j+=2) {
            if (columns[j] == "_t_time")
               continue;
            if (columns[j] == "_i_time")
               continue;

            IndexEntryTag* it = FindIndexByColumnName(ie, columns[j].c_str());

            TAG t;
            if (it)
               STRLCPY(t.name, it->tag_name.c_str());
            else
               STRLCPY(t.name, columns[j].c_str());
            t.type = sql2midasType(columns[j+1].c_str());
            t.n_data = 1;

            ie->tags_cache.push_back(t);
         }
      }
      
      for (unsigned i=0; i<ie->tags_cache.size(); i++)
         ptags->push_back(ie->tags_cache[i]);

      return HS_SUCCESS;
   }

   int hs_read_old_style(double start_time, double end_time, double interval,
                         const char* event_name, const char* tag_name, int var_index,
                         int *num_entries,
                         time_t** time_buffer, double**data_buffer)
   {
      if (fDebug) {
         printf("hs_read_old_style: event \"%s\", tag \"%s\"\n", event_name, tag_name);
      }

      ReadIndex(fSql, NULL);

      for (unsigned e=0; e<gHistoryIndex.size(); e++) {

         const char* s = gHistoryIndex[e]->event_name.c_str();

         bool match = false;
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

         //printf("try %s, match %d\n", s, match);

         if (match) {
            bool found_tag = false;
            IndexEntry *ie = gHistoryIndex[e];
            for (unsigned v=0; v<ie->tags.size(); v++) {
               //printf("try tag [%s] looking for [%s]\n", ie->tags[v].tag_name.c_str(), tag_name);
               if (equal_ustring(tag_name, ie->tags[v].tag_name.c_str())) {
                  found_tag = true;
                  break;
               }
            }

            if (!found_tag)
               match = false;
         }

         if (match) {
            if (fDebug)
               printf("hs_read_old_style: event \"%s\", tag \"%s\", try matching event \'%s\'\n", event_name, tag_name, s);

            int status = hs_read(start_time, end_time, interval,
                                 s, tag_name, var_index,
                                 num_entries,
                                 time_buffer, data_buffer);

            if (status==HS_SUCCESS && *num_entries>0)
               return HS_SUCCESS;
         }
      }

      return HS_UNDEFINED_VAR;
   }

   int hs_read(double start_time, double end_time, double interval,
               const char* event_name, const char* tag_name, int tag_index,
               int *num_entries,
               time_t** time_buffer, double**data_buffer)
   {
      *num_entries = 0;
      *time_buffer = NULL;
      *data_buffer = NULL;

      if (fDebug)
         printf("hs_read: event [%s], tag [%s], index %d, start %f, end %f, dt %f, interval %f, max points %f\n",
                event_name, tag_name, tag_index,
                start_time, end_time, end_time-start_time, interval, (end_time-start_time)/interval);

      if (event_name==NULL)
         return HS_SUCCESS;

      IndexEntry*ie = FindIndexByEventName(event_name);

      if (!ie) {
         ReadIndex(fSql, event_name);
         ie = FindIndexByEventName(event_name);
      }

      if (!ie) {
         XReadIndex();
         ie = FindIndexByEventName(event_name);
      }

      IndexEntryTag *it = NULL;

      if (ie)
         it = FindIndexByTagName(ie, tag_name);

      if (ie && !it) { // maybe this is an array without "Names"?
         char xxx[256];
         sprintf(xxx, "%s[%d]", tag_name, tag_index);
         it = FindIndexByTagName(ie, xxx);
      }
         
      // new-style event name: "equipment_name/variable_name:tag_name"
      // old style event name: "equipment_name:tag_name" ("variable_name" is missing)
      bool oldStyleEventName = (strchr(event_name, '/')==NULL);

      if (oldStyleEventName)
         if (!ie || !it) {
            return hs_read_old_style(start_time, end_time, interval,
                                     event_name, tag_name, tag_index,
                                     num_entries,
                                     time_buffer, data_buffer);
         }

      if (!it)
         return HS_UNDEFINED_VAR;

      assert(ie);
      assert(it);

      std::string tname = ie->table_name;
      std::string cname = it->column_name;

      char cmd[256];
      sprintf(cmd, "SELECT _i_time, %s FROM %s WHERE _i_time>=%.0f and _i_time<=%.0f ORDER BY _i_time;",
              cname.c_str(), tname.c_str(),
              start_time, end_time);
           
      int status = fSql->Exec(cmd);

      if (fDebug) {
         printf("hs_read: event \"%s\", tag \"%s\", index %d: Read table \"%s\" column \"%s\": status %d, nrows: %d, ncolumns: %d\n",
                event_name, tag_name, tag_index,
                tname.c_str(),
                cname.c_str(),
                status,
                fSql->GetNumRows(),
                fSql->GetNumColumns()
                );
      }

      if (status != SUCCESS) {
         return HS_FILE_ERROR;
      }

      if (fSql->GetNumRows() == 0) {
         fSql->Done();

         if (oldStyleEventName) {
            return hs_read_old_style(start_time, end_time, interval,
                                     event_name, tag_name, tag_index,
                                     num_entries,
                                     time_buffer, data_buffer);
         }

         return HS_SUCCESS;
      }

      int nrows = fSql->GetNumRows();
      int ncols = fSql->GetNumColumns();

      if (nrows < 0)
         return HS_FILE_ERROR;

      if (ncols < 1)
         return HS_FILE_ERROR;

      *num_entries = 0;
      *time_buffer = (time_t*)malloc(nrows * sizeof(time_t));
      *data_buffer = (double*)malloc(nrows * sizeof(double));
  
      /* Loop through the rows in the result-set */
      int row = 0;
      time_t tt = 0;
      int ann = 0;
      double att = 0;
      double avv = 0;
      while (1) {
         status = fSql->Fetch();
         if (status != DB_SUCCESS)
            break;

         time_t t = 0;
         double v = 0;
     
         const char* timedata = fSql->GetColumn(1);
         if (timedata)
            t = atoi(timedata);
     
         const char* valuedata = fSql->GetColumn(2);
         if (valuedata)
            v = atof(valuedata);
     
         if (t < start_time || t > end_time)
            continue;
     
         //printf("Row %d, time %d, value %f\n", row, t, v);
         //printf("tt: %d, ann: %d\n", tt, ann);
     
         if (tt == 0 || t >= tt + interval) {
        
            if (ann > 0) {
               assert(row < nrows);
           
               (*time_buffer)[row] = (time_t)(att/ann);
               (*data_buffer)[row] = avv/ann;
           
               row++;
               (*num_entries) = row;
            }

            ann = 0;
            att = 0;
            avv = 0;
            tt  = t;
         
         }

         ann++;
         att += t;
         avv += v;
      }

      if (ann > 0) {
         assert(row < nrows);

         (*time_buffer)[row] = (time_t)(att/ann);
         (*data_buffer)[row] = avv/ann;
     
         row++;
         (*num_entries) = row;
      }

      fSql->Done();

      if (fDebug)
         printf("hs_read: return %d entries\n", *num_entries);

      return HS_SUCCESS;
   }

   int hs_read(time_t start_time, time_t end_time, time_t interval,
               int num_var,
               const char* const event_name[], const char* const tag_name[], const int tag_index[],
               int num_entries[],
               time_t* time_buffer[], double* data_buffer[],
               int st[])
   {
      if (fDebug)
         printf("hs_read: %d variables\n", num_var);

      if (!fSql->IsConnected())
         return HS_FILE_ERROR;

      for (int i=0; i<num_var; i++) {

         if (event_name[i]==NULL) {
            st[i] = HS_UNDEFINED_EVENT;
            num_entries[i] = 0;
            continue;
         }

         st[i] = hs_read(start_time, end_time, interval,
                         event_name[i], tag_name[i], tag_index[i],
                         &num_entries[i],
                         &time_buffer[i], &data_buffer[i]);
      }
      
      return HS_SUCCESS;
   }
};

////////////////////////////////////////////////////////
//               Factory constructors                 //
////////////////////////////////////////////////////////

#ifdef HAVE_ODBC
MidasHistoryInterface* MakeMidasHistoryODBC()
{
   return new SqlHistory(new SqlODBC());
}
#endif

MidasHistoryInterface* MakeMidasHistorySqlDebug()
{
   return new SqlHistory(new SqlDebug());
}

// end

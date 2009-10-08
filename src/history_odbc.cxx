// history_odb.cxx

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
#include "history_odbc.h"

////////////////////////////////////////
//          ODBC includes             //
////////////////////////////////////////

// MIDAS defines collide with ODBC

#define DWORD DWORD_xxx
#define BOOL  BOOL_xxx

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

////////////////////////////////////////
//           helper stuff             //
////////////////////////////////////////

#define STRLCPY(dst, src) strlcpy(dst, src, sizeof(dst))
#define FREE(x) { if (x) free(x); x = NULL; }

////////////////////////////////////////
//        global variables            //
////////////////////////////////////////

static int         gTrace = 0;
static std::string gAlarmName;
static std::string gOdbcDsn;
static time_t      gNextConnect = 0;
static int         gConnectRetry = 0;

////////////////////////////////////////
// Definitions extracted from midas.c //
////////////////////////////////////////

/********************************************************************/
/* data type sizes */
static int tid_size[] = {
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
static char *tid_name[] = {
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
static char *sql_type_pgsql[] = {
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

static char *sql_type_mysql[] = {
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

static char **sql_type = NULL;

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
   if (0 && gTrace)
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
  virtual int Connect(const char* dsn = 0) = 0;
  virtual int Disconnect() = 0;
  virtual bool IsConnected() = 0;
  virtual int Exec(const char* sql) = 0;
  virtual int GetNumRows() = 0;
  virtual int GetNumColumns() = 0;
  virtual int Fetch() = 0;
  virtual int Done() = 0;
  virtual std::vector<std::string> ListTables() = 0;
  virtual std::vector<std::string> ListColumns(const char* table) = 0;
  virtual const char* GetColumn(int icol) = 0;
  virtual ~SqlBase() { }; // virtual dtor
};

////////////////////////////////////////////////////////////////////
//   SqlStdout: for debugging: write all SQL commands to stdout   //
////////////////////////////////////////////////////////////////////

class SqlStdout: public SqlBase
{
public:
  FILE *fp;
  bool fIsConnected;

public:
  int Connect(const char* filename = NULL)
  {
    if (!filename)
      filename = "/dev/fd/1";
    fp = fopen(filename, "w");
    assert(fp);
    sql_type = sql_type_mysql;
    fIsConnected = true;
    return 0;
  }

  int Exec(const char* sql)
  {
    fprintf(fp, "%s\n", sql);
    return 0;
  }

  int Disconnect()
  {
    // do nothing
    fIsConnected = false;
    return 0;
  }
  
  bool IsConnected()
  {
    return fIsConnected;
  }

  SqlStdout() // ctor
  {
    fp = NULL;
    fIsConnected = false;
  }

  ~SqlStdout() // dtor
  {
    if (fp)
      fclose(fp);
    fp = NULL;
  }

  int GetNumRows() { return 0; }
  int GetNumColumns() { return 0; }
  int Fetch() { return 0; }
  int Done() { return 0; }
  std::vector<std::string> ListTables() { std::vector<std::string> list; return list; };
  std::vector<std::string> ListColumns(const char* table) { std::vector<std::string> list; return list; };
  const char* GetColumn(int icol) { return NULL; };
};

//////////////////////////////////////////
//   SqlODBC: SQL access through ODBC   //
//////////////////////////////////////////

class SqlODBC: public SqlBase
{
public:
   bool fIsConnected;

   std::string fDSN;

   SQLHENV  fEnv;
   SQLHDBC  fDB;
   SQLHSTMT fStmt;

   SqlODBC(); // ctor
   ~SqlODBC(); // dtor

   int Connect(const char* dsn);
   int Disconnect();
   bool IsConnected();

   std::vector<std::string> ListTables();
   std::vector<std::string> ListColumns(const char* table_name);

   int Exec(const char* sql);

   int GetNumRows();
   int GetNumColumns();
   int Fetch();
   const char* GetColumn(int icol);
   int Done();

protected:
   void ReportErrors(const char* from, const char* sqlfunc, int status);
};

SqlODBC::SqlODBC() // ctor
{
   fIsConnected = false;
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
      return -1;
   }

   status = SQLSetEnvAttr(fEnv,
                          SQL_ATTR_ODBC_VERSION, 
                          (void*)SQL_OV_ODBC2,
                          0); 
   if (!SQL_SUCCEEDED(status)) {
      cm_msg(MERROR, "SqlODBC::Connect", "SQLSetEnvAttr() error %d", status);
      SQLFreeHandle(SQL_HANDLE_ENV, fEnv);
      return -1;
   }

   status = SQLAllocHandle(SQL_HANDLE_DBC, fEnv, &fDB); 
   if (!SQL_SUCCEEDED(status)) {
      cm_msg(MERROR, "SqlODBC::Connect", "SQLAllocHandle(SQL_HANDLE_DBC) error %d", status);
      SQLFreeHandle(SQL_HANDLE_ENV, fEnv);
      exit(0);
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
      return -1;
   }

   SQLAllocHandle(SQL_HANDLE_STMT, fDB, &fStmt);

   cm_msg(MINFO, "SqlODBC::Connect", "history_odbc: Connected to %s", dsn);

   if (gAlarmName.length()>0)
      al_reset_alarm(gAlarmName.c_str());

   fIsConnected = true;

   return 0;
}

int SqlODBC::Disconnect()
{
   if (!fIsConnected)
      return 0;

   SQLDisconnect(fDB);

   SQLFreeHandle(SQL_HANDLE_DBC,  fDB);
   SQLFreeHandle(SQL_HANDLE_STMT, fStmt);
   SQLFreeHandle(SQL_HANDLE_ENV,  fEnv);

   fIsConnected = false;

   if (gAlarmName.length() > 0) {
      char buf[256];
      sprintf(buf, "%s lost connection to the history database", gAlarmName.c_str());
      al_trigger_alarm(gAlarmName.c_str(), buf, "Alarm", "", AT_INTERNAL);
   }

   return 0;
}

bool SqlODBC::IsConnected()
{
   return fIsConnected;
}

void SqlODBC::ReportErrors(const char* from, const char* sqlfunc, int status)
{
   if (gTrace)
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
         if (gTrace)
            printf("%s: %s error: state: \'%s\', message: \'%s\', native error: %d\n", from, sqlfunc, state, message, error);
         cm_msg(MERROR, from, "%s error: state: \'%s\', message: \'%s\', native error: %d", sqlfunc, state, message, error);
      }
   }
}

std::vector<std::string> SqlODBC::ListTables()
{
   std::vector<std::string> list;

   if (!fIsConnected)
      return list;

   /* Retrieve a list of tables */
   int status = SQLTables(fStmt, NULL, 0, NULL, 0, NULL, 0, (SQLCHAR*)"TABLE", SQL_NTS);
   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::ListTables", "SQLTables()", status);
      return list;
   }

   int ncols = GetNumColumns();
   int nrows = GetNumRows();

   if (ncols <= 0 || nrows <= 0) {
      cm_msg(MERROR, "SqlODBC::ListTables", "Error: SQLTables() returned unexpected number of columns %d or number of rows %d, status %d", ncols, nrows, status);
   }

   int row = 0;
   while (Fetch()) {
      if (0) {
         printf("row %d: ", row);
         for (int i=1; i<=ncols; i++) {
            const char* s = GetColumn(i);
            printf("[%s]", s);
         }
         printf("\n");
         row++;
      }

      list.push_back(GetColumn(3));
   }

   Done();

   return list;
}

std::vector<std::string> SqlODBC::ListColumns(const char* table)
{
   std::vector<std::string> list;

   if (!fIsConnected)
      return list;

   /* Retrieve a list of tables */
   int status = SQLColumns(fStmt, NULL, 0, NULL, 0, (SQLCHAR*)table, SQL_NTS, NULL, 0);
   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::ListColumns", "SQLColumns()", status);
      return list;
   }

   int ncols = GetNumColumns();
   int nrows = GetNumRows(); // nrows seems to be always "-1"

   if (ncols <= 0 /*|| nrows <= 0*/) {
      cm_msg(MERROR, "SqlODBC::ListColumns", "Error: SQLColumns(\'%s\') returned unexpected number of columns %d or number of rows %d, status %d", table, ncols, nrows, status);
   }

   //printf("get columns [%s]: status %d, ncols %d, nrows %d\n", table, status, ncols, nrows);

   int row = 0;
   while (Fetch()) {
      if (0) {
         printf("row %d: ", row);
         for (int i=1; i<=ncols; i++) {
            const char* s = GetColumn(i);
            printf("[%s]", s);
         }
         printf("\n");
         row++;
      }

      list.push_back(GetColumn(4)); // column name
      list.push_back(GetColumn(6)); // column type
   }

   Done();

   return list;
}

int SqlODBC::Exec(const char* sql)
{
   if (!fIsConnected)
      return -1;
    
   int status;

   for (int i=0; i<2; i++) {
      if (gTrace)
         printf("SqlODBC::Exec: %s\n", sql);

      status = SQLExecDirect(fStmt,(SQLCHAR*)sql,SQL_NTS);

      if (SQL_SUCCEEDED(status)) {
         return 0;
      }

      if (gTrace)
         printf("SqlODBC::Exec: SQLExecDirect() error %d: SQL command: \"%s\"\n", status, sql);
      
      ReportErrors("SqlODBC::Exec", "SQLExecDirect()", status);
      
      cm_msg(MINFO, "SqlODBC::Exec", "history_odbc: Trying to reconnect to %s", fDSN.c_str());

      // try to reconnect
      std::string dsn = fDSN;
      Disconnect();
      Connect(dsn.c_str());

      if (!fIsConnected) {
         cm_msg(MERROR, "SqlODBC::Exec", "history_odbc: Reconnect to %s failed. Database is down?", fDSN.c_str());
      }
   }

   return 0;
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
      return 0;

   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::Fetch", "SQLFetch()", status);
      return -1;
   }

   return 1;
}

int SqlODBC::Done()
{
   int status = SQLCloseCursor(fStmt);
   if (!SQL_SUCCEEDED(status)) {
      ReportErrors("SqlODBC::Done", "SQLCloseCursor()", status);
      return -1;
   }
   return 0;
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
   bool  create;
   
   Event() // ctor
   {
      active = false;
      create = false;
   }
   
   ~Event() // dtor
   {
      create = false;
      active = false;
   }
};

static void PrintTags(int ntags, const TAG tags[])
{
   for (int i=0; i<ntags; i++)
      printf("tag %d: %s %s[%d]\n", i, midasTypeName(tags[i].type), tags[i].name, tags[i].n_data);
}

int CreateEvent(SqlBase* sql, Event* e)
{
   int status;
   
   if (e->create) {
      char buf[1024];
      sprintf(buf, "CREATE TABLE %s (_t_time TIMESTAMP NOT NULL, _i_time INTEGER NOT NULL, INDEX (_i_time), INDEX (_t_time));", e->table_name.c_str());
      status = sql->Exec(buf);
      if (status != 0) {
         e->active = false;
         return -1;
      }
   }
   
   for (size_t i=0; i<e->tags.size(); i++)
      if (e->tags[i].create) {
         char buf[1024];

         sprintf(buf, "ALTER TABLE %s ADD COLUMN %s %s;",
                 e->table_name.c_str(),
                 e->tags[i].column_name.c_str(),
                 midas2sqlType(e->tags[i].tag.type));

         status = sql->Exec(buf);

         if (status != 0) {
            e->active = false;
            return -1;
         }
      }

   return 0;
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
   
   if (status != 0) {
      e->active = false;
      return -1;
   }

   return 0;
}

////////////////////////////////////////////////////////
//    Implementations of public hs_xxx() functions    //
////////////////////////////////////////////////////////

static SqlBase *gSql = NULL;

int hs_debug_odbc(int debug)
{
   int old = gTrace;
   gTrace = debug;
   return old;
}

int hs_set_alarm_odbc(const char* alarm_name)
{
   if (alarm_name)
      gAlarmName = alarm_name;
   else
      gAlarmName = "";
   return HS_SUCCESS;
}

int hs_connect_odbc(const char* odbc_dsn)
{
   int status;

   if (gSql && gSql->IsConnected())
      if (strcmp(gOdbcDsn.c_str(), odbc_dsn) == 0)
         return HS_SUCCESS;
   
   if (gSql)
      hs_disconnect_odbc();
   
   assert(!gSql);

   gOdbcDsn = odbc_dsn;
   
   if (gTrace)
      printf("hs_connect_odbc: set DSN to \'%s\'\n", gOdbcDsn.c_str());

   if (gOdbcDsn[0] == '/')
      gSql = new SqlStdout();
   else
      gSql = new SqlODBC();
   
   status = gSql->Connect(gOdbcDsn.c_str());
   if (status != 0)
      return HS_FILE_ERROR;
   
   return HS_SUCCESS;
}

int hs_disconnect_odbc()
{
   if (gTrace)
      printf("hs_disconnect_odbc!\n");

   if (gSql) {
      gSql->Disconnect();
      delete gSql;
      gSql = NULL;
   }

   return HS_SUCCESS;
}

// convert MIDAS names to SQL names

static std::string tagName2columnName(const char* s)
{
  char out[1024];
  int i;
  for (i=0; s[i]!=0; i++)
    {
      char c = s[i];
      if (isalpha(c) || isdigit(c))
        out[i] = tolower(c);
      else
        out[i] = '_';
    }

  out[i] = 0;
  return out;
}

static std::string eventName2tableName(const char* s)
{
   return tagName2columnName(s);
}

////////////////////////////////////////////////////////
//             Functions used by mlogger              //
////////////////////////////////////////////////////////

static std::vector<Event*> gEvents;

int hs_define_event_odbc(const char* event_name, const TAG tags[], int tags_size)
{
   assert(gSql);

   int ntags = tags_size/sizeof(TAG);

   if (gTrace) {
      printf("define event [%s] with %d tags:\n", event_name, ntags);
      PrintTags(ntags, tags);
   }

   // delete all events with the same name
   for (unsigned int i=0; i<gEvents.size(); i++)
      if (gEvents[i])
         if (gEvents[i]->event_name == event_name) {
            printf("deleting exising event %s\n", event_name);
            delete gEvents[i];
            gEvents[i] = NULL;
         }

   Event* e = new Event();

   e->event_name = event_name;
   e->table_name = eventName2tableName(event_name);
   e->active = true;
   e->create = false;

   int offset = 0;
   for (int i=0; i<ntags; i++) {
      for (unsigned int j=0; j<tags[i].n_data; j++) {
         Tag t;
         t.create = false;
         if (tags[i].n_data == 1)
            t.column_name = tagName2columnName(tags[i].name);
         else {
            char s[256];
            sprintf(s, "_%d", j);
            t.column_name = tagName2columnName(tags[i].name) + s;
         }
         t.offset = offset;
         t.tag = tags[i];
         t.tag.n_data = 1;
         e->tags.push_back(t);
         int size = tid_size[tags[i].type];
         offset += size;
      }
   }

   std::vector<std::string> columns = gSql->ListColumns(e->table_name.c_str());

   if (columns.size() <= 0)
      e->create = true;

   for (size_t i=0; i<e->tags.size(); i++) {
      // check for duplicate column names
      for (size_t j=i+1; j<e->tags.size(); j++)
         if (e->tags[i].column_name == e->tags[j].column_name) {
            cm_msg(MERROR, "hs_define_event_odbc", "Error: History event \'%s\': Duplicated column name \'%s\' from tags %d \'%s\' and %d \'%s\'", event_name, e->tags[i].column_name.c_str(), i, e->tags[i].tag.name, j, e->tags[j].tag.name);
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
               cm_msg(MERROR, "hs_define_event_odbc", "Error: History event \'%s\': Incompatible data type for tag \'%s\' type \'%s\', SQL column \'%s\' type \'%s\'", event_name, e->tags[i].tag.name, midasTypeName(e->tags[i].tag.type), columns[j].c_str(), columns[j+1].c_str());
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

   int status = CreateEvent(gSql, e);

   if (status != 0) {
      // if cannot create event in SQL database, disable this event and carry on.

      e->active = false;

      if (gAlarmName.length() > 0) {
         char buf[256];
         sprintf(buf, "%s cannot define history event \'%s\', see messages", gAlarmName.c_str(), e->event_name.c_str());
         al_trigger_alarm(gAlarmName.c_str(), buf, "Alarm", "", AT_INTERNAL);
      }
   }

   // find empty slot in events list
   for (unsigned int i=0; i<gEvents.size(); i++)
      if (!gEvents[i]) {
         gEvents[i] = e;
         e = NULL;
         break;
      }

   // if no empty slots, add at the end
   if (e)
      gEvents.push_back(e);

   return HS_SUCCESS;
}

int hs_write_event_odbc(const char* event_name, time_t timestamp, const char* buffer, int buffer_size)
{
   if (gTrace)
      printf("write event [%s] size %d\n", event_name, buffer_size);

   assert(gSql);

   // if disconnected, try to reconnect

   if (!gSql->IsConnected()) {
      time_t now = time(NULL);

      // too early to try reconnecting?
      if (gConnectRetry!=0 && now < gNextConnect) {
         return HS_FILE_ERROR;
      }

      int status = gSql->Connect(gOdbcDsn.c_str());

      if (status != 0) {

         // first retry in 5 seconds
         if (gConnectRetry == 0)
            gConnectRetry = 5;

         gNextConnect = now + gConnectRetry;

         // exponential backoff
         gConnectRetry *= 2;

         // but no more than every 10 minutes
         if (gConnectRetry > 10*60)
            gConnectRetry = 10*60;

         return HS_FILE_ERROR;
      }
   }

   gNextConnect = 0;
   gConnectRetry = 0;

   Event *e = NULL;

   // find this event
   for (size_t i=0; i<gEvents.size(); i++)
      if (gEvents[i]->event_name == event_name) {
         e = gEvents[i];
         break;
      }

   // not found
   if (!e)
      return HS_FILE_ERROR;

   // deactivated because of error?
   if (!e->active)
      return HS_FILE_ERROR;

   int status = WriteEvent(gSql, e, timestamp, buffer, buffer_size);

   // if could not write to SQL?
   if (status != 0) {

      // if lost SQL connection, try again later
      if (!gSql->IsConnected()) {
         return HS_FILE_ERROR;
      }

      // otherwise, deactivate this event, raise alarm

      e->active = 0;

      if (gAlarmName.length() > 0) {
         char buf[256];
         sprintf(buf, "%s cannot write history event \'%s\', see messages", gAlarmName.c_str(), e->event_name.c_str());
         al_trigger_alarm(gAlarmName.c_str(), buf, "Alarm", "", AT_INTERNAL);
      }

      return HS_FILE_ERROR;
   }

   return HS_SUCCESS;
}

////////////////////////////////////////////////////////
//             Functions used by mhttpd               //
////////////////////////////////////////////////////////

int hs_get_tags_odbc(const char* event_name, int *n_tags, TAG **tags)
{
   assert(gSql);

   *n_tags = 0;
   *tags = NULL;

   std::string tname = eventName2tableName(event_name);

   std::vector<std::string> columns = gSql->ListColumns(tname.c_str());

   if (columns.size() < 1) {
      cm_msg(MERROR, "hs_get_tags_odbc", "Cannot get columns for table \'%s\', try to reconnect to the database", tname.c_str());

      gSql->Disconnect();
      gSql->Connect(gOdbcDsn.c_str());
      if (!gSql->IsConnected()) {

         if (gAlarmName.length() > 0) {
            char buf[256];
            sprintf(buf, "%s lost connection to the history database", gAlarmName.c_str());
            al_trigger_alarm(gAlarmName.c_str(), buf, "Alarm", "", AT_INTERNAL);
         }

         return HS_FILE_ERROR;
      }

      columns = gSql->ListColumns(tname.c_str());      
   }

   TAG* t = (TAG*)malloc(sizeof(TAG)*columns.size());
   assert(t);

   int n=0;
   for (unsigned int j=0; j<columns.size(); j+=2) {
      if (columns[j] == "_t_time")
         continue;
      if (columns[j] == "_i_time")
         continue;
      STRLCPY(t[n].name, columns[j].c_str());
      t[n].type = sql2midasType(columns[j+1].c_str());
      t[n].n_data = 1;
      n++;
   }

   if (0) {
      printf("event [%s] table [%s], tags: %d\n", event_name, tname.c_str(), n);
      PrintTags(n, t);
   }

   *n_tags = n;
   *tags = t;

   return HS_SUCCESS;
}

int hs_read_odbc(time_t start_time, time_t end_time, time_t interval,
                 const char* event_name, const char* tag_name, int var_index,
                 int *num_entries,
                 time_t** time_buffer, double**data_buffer)
{
   assert(gSql);

   *num_entries = 0;
   *time_buffer = NULL;
   *data_buffer = NULL;

   //printf("start %d, end %d, dt %d, interval %d, max points %d\n", start_time, end_time, end_time-start_time, interval, (end_time-start_time)/interval);

   std::string ename = eventName2tableName(event_name);
   std::string tname = tagName2columnName(tag_name);

   int status = 1;

   std::vector<std::string> tables = gSql->ListTables();

   if (tables.size() <= 1) {
        cm_msg(MERROR, "hs_read_odbc", "ListTables() returned nothing, trying to reconnect to the database");

        gSql->Disconnect();
        gSql->Connect(gOdbcDsn.c_str());
        if (!gSql->IsConnected()) {

           if (gAlarmName.length() > 0) {
              char buf[256];
              sprintf(buf, "%s lost connection to the history database", gAlarmName.c_str());
              al_trigger_alarm(gAlarmName.c_str(), buf, "Alarm", "", AT_INTERNAL);
           }

           return HS_FILE_ERROR;
        }

        tables = gSql->ListTables();
   }

   int len = strlen(event_name);
   for (unsigned int i=0; i<tables.size(); i++) {
      //printf("table %s\n", tables[i].c_str());

      const char* t = tables[i].c_str();
      const char* s = strstr(t, ename.c_str());

      if (s==t && (t[len]=='_'||t[len]==0)) {

         bool found = false;
         std::vector<std::string> columns = gSql->ListColumns(tables[i].c_str());
         for (unsigned int j=0; j<columns.size(); j+=2) {
            //printf("column %s\n", columns[j].c_str());
            if (columns[j] == tname) {
               found = true;
               break;
            }
         }

         if (found) {
            char cmd[256];
            sprintf(cmd, "SELECT _i_time, %s FROM %s where _i_time>=%d and _i_time<=%d;",
                    tname.c_str(), tables[i].c_str(),
                    (int)start_time, (int)end_time);
           
            status = gSql->Exec(cmd);

            if (gTrace) {
               printf("hs_read_odbc: event %s, name %s, index %d: Read table %s: status %d, nrows: %d\n",
                      event_name, tag_name, var_index,
                      tables[i].c_str(),
                      status,
                      gSql->GetNumRows());
            }

            if (status)
               continue;

            if (gSql->GetNumRows() == 0) {
               gSql->Done();
               status = SQL_NO_DATA;
               continue;
            }

            break;
         }
      }
   }

   if (status == SQL_NO_DATA)
      return HS_SUCCESS;

   if (status) {
      if (gTrace)
         printf("hs_read_odbc: event %s, name %s, index %d, could not find the right table? status %d\n",
                event_name, tag_name, var_index,
                status);
      
      return HS_FILE_ERROR;
   }

   int nrows = gSql->GetNumRows();
   int ncols = gSql->GetNumColumns();

   if (nrows == 0)
      return HS_SUCCESS;

   if (gTrace)
      printf("hs_read_odbc: event %s, name %s, index %d, nrows: %d, ncols: %d\n",
             event_name, tag_name, var_index,
             nrows, ncols);

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
   while (gSql->Fetch()) {
      time_t t = 0;
      double v = 0;
     
      const char* timedata = gSql->GetColumn(1);
      if (timedata)
         t = atoi(timedata);
     
      const char* valuedata = gSql->GetColumn(2);
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

   gSql->Done();

   if (gTrace)
      printf("hs_read_odbc: return %d events\n", *num_entries);

   return HS_SUCCESS;
}

// end

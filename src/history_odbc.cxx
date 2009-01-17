// history_odb.cxx

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// MIDAS BOOL collides with ODBC BOOL
#define BOOL xxxBOOL

#include "midas.h"
#include "history_odbc.h"

#include <vector>
#include <string>

std::string tagName2columnName(const char* s)
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

#define DWORD xxx

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

static int  gTrace = 0;

int hs_debug_odbc(int debug)
{
  int old = gTrace;
  gTrace = debug;
  return old;
}

////////////////////////////////////////
//              SQL Stuff             //
////////////////////////////////////////

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
   "TINYINT SIGNED", // TID_BYTE
   "TINYINT UNSIGNED",  // TID_SBYTE
   "CHAR(1)",   // TID_CHAR
   "SMALLINT UNSIGNED ", // TID_WORD
   "SMALLINT SIGNED ", // TID_SHORT
   "INT UNSIGNED ", // TID_DWORD
   "INT SIGNED ", // TID_INT
   "BOOL",      // TID_BOOL
   "DOUBLE", // TID_FLOAT
   "DOUBLE", // TID_DOUBLE
   "TINYINT UNSIGNED", // TID_BITFIELD
   "VARCHAR",   // TID_STRING
   "xxxINVALIDxxxARRAY",
   "xxxINVALIDxxxSTRUCT",
   "xxxINVALIDxxxKEY",
   "xxxINVALIDxxxLINK"
};

static char **sql_type = NULL;

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

static char* gAlarmName = "unknown";

class SqlODBC: public SqlBase
{
public:
   bool fIsConnected;

   SQLHENV  fEnv;
   SQLHDBC  fDB;
   SQLHSTMT fStmt;

   SqlODBC(); // ctor
   ~SqlODBC(); // dtor

   int Connect(const char* dsn);
   int Disconnect();
   bool IsConnected();

   std::vector<std::string> ListTables();
   std::vector<std::string> ListColumns(const char* table);

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
   gAlarmName = "Logger";
}

SqlODBC::~SqlODBC() // dtor
{
   Disconnect();
}

int SqlODBC::Connect(const char* dsn)
{
   if (fIsConnected)
      Disconnect();

   int status = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &fEnv);

   if (!SQL_SUCCEEDED(status))
      {
         cm_msg(MERROR, "SqlODBC::Connect", "SQLAllocHandle(SQL_HANDLE_ENV) error %d", status);
         return -1;
      }

   status = SQLSetEnvAttr(fEnv,
                          SQL_ATTR_ODBC_VERSION, 
                          (void*)SQL_OV_ODBC2,
                          0); 
   if (!SQL_SUCCEEDED(status))
      {
         cm_msg(MERROR, "SqlODBC::Connect", "SQLSetEnvAttr() error %d", status);
         SQLFreeHandle(SQL_HANDLE_ENV, fEnv);
         return -1;
      }

   // 2. allocate connection handle, set timeout

   status = SQLAllocHandle(SQL_HANDLE_DBC, fEnv, &fDB); 
   if (!SQL_SUCCEEDED(status))
      {
         cm_msg(MERROR, "SqlODBC::Connect", "SQLAllocHandle(SQL_HANDLE_DBC) error %d", status);
         SQLFreeHandle(SQL_HANDLE_ENV, fEnv);
         exit(0);
      }

   SQLSetConnectAttr(fDB, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);

   // 3. Connect to the datasource "web" 

   if (0)
      {
         // connect to PgSQL database

         sql_type = sql_type_pgsql;
         status = SQLConnect(fDB, (SQLCHAR*) dsn, SQL_NTS,
                             (SQLCHAR*) "xxx", SQL_NTS,
                             (SQLCHAR*) "", SQL_NTS);
      }

   if (1)
      {
         // connect to MySQL database

         sql_type = sql_type_mysql;
         status = SQLConnect(fDB, (SQLCHAR*) dsn, SQL_NTS,
                             (SQLCHAR*) NULL, SQL_NTS,
                             (SQLCHAR*) NULL, SQL_NTS);
      }

   if ((status != SQL_SUCCESS) && (status != SQL_SUCCESS_WITH_INFO))
      {
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

   cm_msg(MINFO, "SqlODBC::Connect", "Connected to %s", dsn);

   al_reset_alarm(gAlarmName);

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

   char buf[256];
   sprintf(buf, "%s lost connection to the history database", gAlarmName);
   al_trigger_alarm(gAlarmName, buf, "Alarm", "", AT_INTERNAL);

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
      
      if ((error != 1060) && (error != 1050)) {
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

      list.push_back(GetColumn(4));
   }

   Done();

   return list;
}

int SqlODBC::Exec(const char* sql)
{
   if (!fIsConnected)
      return -1;
    
   int status;

   if (gTrace)
      printf("SqlODBC::Exec: %s\n", sql);

   status = SQLExecDirect(fStmt,(SQLCHAR*)sql,SQL_NTS);

   if (!SQL_SUCCEEDED(status))
      {
         if (gTrace)
            printf("SqlODBC::Exec: SQLExecDirect() error %d: SQL command: \"%s\"\n", status, sql);

         ReportErrors("SqlODBC::Exec", "SQLExecDirect()", status);
         return -1;
      }

   return 0;
}

int SqlODBC::GetNumRows()
{
   SQLINTEGER nrows = 0;
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
  SQLINTEGER indicator;
  int status = SQLGetData(fStmt, icol, SQL_C_CHAR, buf, sizeof(buf), &indicator);

  if (!SQL_SUCCEEDED(status)) {
    return NULL;
  }

  if (indicator == SQL_NULL_DATA)
    return NULL;

  return buf;
}

#define FREE(x) { if (x) free(x); x = NULL; }

struct Tag
{
  char* column_name;
  int   offset;
  TAG   tag;

  ~Tag() // dtor
  {
    FREE(column_name);
  }
};

struct Event
{
  char* event_name;
  char* table_name;
  int ntags;
  Tag* tags;

  ~Event() // dtor
  {
    ntags = 0;
    FREE(event_name);
    FREE(table_name);
    FREE(tags);
  }
};

const char* typeName(int tid)
{
  //printf("tid %d\n", tid);
  assert(tid>=0);
  assert(tid<15);
  return sql_type[tid];
}

class SqlOutput
{
public:
  SqlBase* fSql;

  SqlOutput(SqlBase* sqlptr)
  {
    fSql = sqlptr;
  }

  void CreateTable(Event* e)
  {
    int status;
    char buf[1024];
    sprintf(buf, "CREATE TABLE %s (_t_time TIMESTAMP NOT NULL, _i_time INTEGER NOT NULL);", e->table_name);
    status = fSql->Exec(buf);

    for (int i=0; i<e->ntags; i++)
      {
        int arraySize = e->tags[i].tag.n_data;
        if (arraySize == 1)
          {
            char buf[1024];
            sprintf(buf, "ALTER TABLE %s ADD COLUMN %s %s;",
                    e->table_name,
                    e->tags[i].column_name,
                    typeName(e->tags[i].tag.type));
            int status = fSql->Exec(buf);
            //assert(status == 0);
          }
        else
          {
            for (int j=0; j<arraySize; j++)
              {
                char buf[1024];
                sprintf(buf, "ALTER TABLE %s ADD COLUMN %s_%d %s;",
                        e->table_name,
                        e->tags[i].column_name,
                        j,
                        typeName(e->tags[i].tag.type));
                int status = fSql->Exec(buf);
                //assert(status == 0);
              }
          }
      }
  }

  void Write(Event *e, time_t t, const char*buf, int size)
  {
    if (!fSql->IsConnected())
       return;
       
    //printf("event %d, time %s", rec.event_id, ctime(&t));

    int n  = e->ntags;
    
    std::string tags;
    std::string values;
    
    //if (n>0)
    //  printf(" %s", ctime(&t));
    
    for (int i=0; i<n; i++)
      {
        const Tag*t = &e->tags[i];
        
        if (t)
          {
            int offset = t->offset;
            void* ptr = (void*)(buf+offset);

            int arraySize = t->tag.n_data;
            
            for (int j=0; j<arraySize; j++)
              {
                tags   += ", ";
                values += ", ";
                
                if (arraySize <= 1)
                  tags += t->column_name;
                else
                  {
                    tags += t->column_name;
                    char s[256];
                    sprintf(s,"_%d", j);
                    tags += s;
                  }
                
                char s[1024];
                
                switch (t->tag.type)
                  {
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
           e->table_name,
           tags.c_str(),
           s,
           (int)t,
           values.c_str());
    int status = fSql->Exec(sss);
  
    if (status != 0)
      {
        cm_msg(MERROR, "SqlOutput::Write", "SQL database write error %d, disconnecting", status);
        fSql->Disconnect();
      }
  }
};

static char       gOdbcDsn[265];
static SqlBase   *sqlx = NULL;
static SqlOutput *sql  = NULL;

int hs_connect_odbc(const char* odbc_dsn)
{
  int status;

  if (strcmp(gOdbcDsn, odbc_dsn) == 0)
    return HS_SUCCESS;

  if (sqlx)
    hs_disconnect_odbc();

  assert(!sqlx);

  strlcpy(gOdbcDsn, odbc_dsn, sizeof(gOdbcDsn));

  if (gTrace)
    printf("hs_connect_odbc: set DSN to \'%s\'\n", gOdbcDsn);

  if (gOdbcDsn[0] == '/')
    sqlx = new SqlStdout();
  else
    sqlx = new SqlODBC();

  status = sqlx->Connect(gOdbcDsn);
  if (status != 0)
    return !HS_SUCCESS;

  sql = new SqlOutput(sqlx);

  return HS_SUCCESS;
}

int hs_disconnect_odbc()
{
   if (gTrace)
      printf("hs_disconnect_odbc!\n");

   if (sqlx) {
      sqlx->Disconnect();
      delete sqlx;
   }

   sqlx = NULL;
   gOdbcDsn[0] = 0;

  return HS_SUCCESS;
}

#define STRLCPY(dst, src) strlcpy(dst, src, sizeof(dst))

#include <vector>

static std::vector<Event*> events;

Event* FindEvent(const char* event_name)
{
   int ne = events.size();
   for (int i=0; i<ne; i++)
      if (strcmp(events[i]->event_name, event_name)==0)
         return events[i];
   assert(!"Attempt to write an undefined event!");
   // NOT REACHED
   return NULL;
}

int hs_define_event_odbc(const char* event_name, const TAG tags[], int tags_size)
{
   assert(sql);

   int ntags = tags_size/sizeof(TAG);

   if (gTrace)
      printf("define event [%s] with %d tags\n", event_name, ntags);

   Event* e = new Event();

   e->event_name = strdup(event_name);
   e->table_name = strdup(tagName2columnName(event_name).c_str());
   e->ntags = ntags;
   e->tags = new Tag[ntags];

   int offset = 0;
   for (int i=0; i<ntags; i++)
      {
         e->tags[i].column_name = strdup(tagName2columnName(tags[i].name).c_str());
         e->tags[i].offset = offset;
         e->tags[i].tag = tags[i];
         int size = tags[i].n_data * tid_size[tags[i].type];
         offset += size;
      }

   sql->CreateTable(e);

   events.push_back(e);

   return HS_SUCCESS;
}

int hs_write_event_odbc(const char*  event_name, time_t timestamp, const char* buffer, int buffer_size)
{
   if (gTrace)
      printf("write event [%s] size %d\n", event_name, buffer_size);
   assert(sql);
   sql->Write(FindEvent(event_name), timestamp, buffer, buffer_size);
   return HS_SUCCESS;
}

static char* encode(const std::vector<std::string>& list)
{
   int len = 0;
   for (unsigned int i=0; i<list.size(); i++) {
      len += 1 + list[i].length();
   }
   len += 1;

   char *s = (char*)malloc(len);
   assert(s!=NULL);

   int pos = 0;
   for (unsigned int i=0; i<list.size(); i++) {
      const char* eee = list[i].c_str();
      int   lll = strlen(eee);
      memcpy(s+pos, eee, lll);
      s[pos+lll] = 0;
      pos += lll + 1;
   }
   s[pos] = 0;
   pos++;

   assert(pos == len);

   return s;
}

int hs_enum_events_odbc(int *num_events, char**event_names)
{
   assert(sqlx);

   *num_events = 0;
   *event_names = NULL;

   std::vector<std::string> tables = sqlx->ListTables();

   std::vector<std::string> events;

   for (unsigned int i=0; i<tables.size(); i++) {
      std::string ename = tables[i];
      int pos = ename.find("_");
      if (pos > 0)
         ename = ename.substr(0, pos);

      bool found = false;
      for (unsigned int j=0; j<events.size(); j++) {
         if (events[j] == ename) {
            found = true;
            break;
         }
      }

      if (!found)
         events.push_back(ename);
   }

#if 0
   int len = 0;
   for (unsigned int i=0; i<events.size(); i++) {
      len += 1 + events[i].length();
   }
   len += 1;

   char *s = (char*)malloc(len);
   int pos = 0;
   for (unsigned int i=0; i<events.size(); i++) {
      const char* eee = events[i].c_str();
      int   lll = strlen(eee);
      memcpy(s+pos, eee, lll);
      s[pos+lll] = 0;
      pos += lll + 1;
   }
   s[pos] = 0;
   pos++;

   assert(pos == len);
#endif

   *num_events  = events.size();
   *event_names = encode(events);
      
   return HS_SUCCESS;
}

int hs_enum_vars_odbc(const char* event_name, int *num_vars, char** var_names)
{
   assert(sqlx);

   *num_vars = 0;
   *var_names = NULL;

   std::vector<std::string> tables = sqlx->ListTables();
   std::vector<std::string> vars;

   std::string ename = tagName2columnName(event_name);

   int len = ename.length();
   for (unsigned int i=0; i<tables.size(); i++) {
      const char* t = tables[i].c_str();
      const char* s = strstr(t, ename.c_str());

      //printf("ename [%s], table [%s], t: %p, s: %p, char: [%c]\n", ename.c_str(), tables[i].c_str(), t, s, t[len]);
      
      if (s==t && (t[len]=='_'||t[len]==0)) {
         
         std::vector<std::string> columns = sqlx->ListColumns(tables[i].c_str());
         
         for (unsigned int j=0; j<columns.size(); j++) {
            if (columns[j] == "_t_time")
               continue;
            if (columns[j] == "_i_time")
               continue;
            //printf("column %s\n", columns[j].c_str());
            vars.push_back(columns[j]);
         }
      }
   }

#if 0
   char* p = (char*)malloc(vars.size()*NAME_LENGTH);

   for (unsigned int i=0; i<vars.size(); i++) {
      strlcpy(p + i*NAME_LENGTH, vars[i].c_str(), NAME_LENGTH);
   }
#endif
      
   *num_vars = vars.size();
   *var_names = encode(vars);

   return HS_SUCCESS;
}

int hs_get_events_odbc(int *num_events, char**event_names)
{
   assert(sqlx);

   *num_events = 0;
   *event_names = NULL;

   std::vector<std::string> tables = sqlx->ListTables();

   std::vector<std::string> events;

   for (unsigned int i=0; i<tables.size(); i++) {
      std::string ename = tables[i];
      events.push_back(ename);
   }

   *num_events  = events.size();
   *event_names = encode(events);
      
   return HS_SUCCESS;
}

int hs_get_tags_odbc(const char* event_name, int *n_tags, TAG **tags)
{
   assert(sqlx);

   *n_tags = 0;
   *tags = NULL;

   std::string ename = tagName2columnName(event_name);

   std::vector<std::string> columns = sqlx->ListColumns(ename.c_str());

   TAG* t = (TAG*)malloc(sizeof(TAG)*columns.size());
   assert(t);

   int n=0;
   for (unsigned int j=0; j<columns.size(); j++) {
      if (columns[j] == "_t_time")
         continue;
      if (columns[j] == "_i_time")
         continue;
      STRLCPY(t[n].name, columns[j].c_str());
      t[n].type = TID_DOUBLE;
      t[n].n_data = 1;
      n++;
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
   assert(sqlx);

   gAlarmName = "mhttpd_history";

   *num_entries = 0;
   *time_buffer = NULL;
   *data_buffer = NULL;

   //printf("start %d, end %d, dt %d, interval %d, max points %d\n", start_time, end_time, end_time-start_time, interval, (end_time-start_time)/interval);

   std::string ename = tagName2columnName(event_name);
   std::string tname = tagName2columnName(tag_name);

   int status = 1;

   std::vector<std::string> tables = sqlx->ListTables();

   if (tables.size() <= 1) {
        cm_msg(MERROR, "hs_read_odbc", "ListTables() returned nothing, trying to reconnect to the database");

        std::string dsn = gOdbcDsn;
        sqlx->Disconnect();
        sqlx->Connect(dsn.c_str());
        if (!sqlx->IsConnected()) {
           return HS_FILE_ERROR;
        }

        tables = sqlx->ListTables();
   }

   int len = strlen(event_name);
   for (unsigned int i=0; i<tables.size(); i++) {
      //printf("table %s\n", tables[i].c_str());

      const char* t = tables[i].c_str();
      const char* s = strstr(t, ename.c_str());

      if (s==t && (t[len]=='_'||t[len]==0)) {

         bool found = false;
         std::vector<std::string> columns = sqlx->ListColumns(tables[i].c_str());
         for (unsigned int j=0; j<columns.size(); j++) {
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
           
            status = sqlx->Exec(cmd);

            if (gTrace) {
               printf("hs_read_odbc: event %s, name %s, index %d: Read table %s: status %d, nrows: %d\n",
                      event_name, tag_name, var_index,
                      tables[i].c_str(),
                      status,
                      sqlx->GetNumRows());
            }

            if (status)
               continue;

            if (sqlx->GetNumRows() == 0) {
               sqlx->Done();
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

   int nrows = sqlx->GetNumRows();
   int ncols = sqlx->GetNumColumns();

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
   while (sqlx->Fetch()) {
      time_t t = 0;
      double v = 0;
     
      const char* timedata = sqlx->GetColumn(1);
      if (timedata)
         t = atoi(timedata);
     
      const char* valuedata = sqlx->GetColumn(2);
      if (valuedata)
         v = atof(valuedata);
     
      if (t < start_time || t > end_time)
         continue;
     
      //printf("Row %d, time %d, value %f\n", row, t, v);
      //printf("tt: %d, ann: %d\n", tt, ann);
     
      if (tt == 0 || t >= tt + interval) {
        
         if (ann > 0) {
            assert(row < nrows);
           
            (*time_buffer)[row] = att/ann;
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

      (*time_buffer)[row] = att/ann;
      (*data_buffer)[row] = avv/ann;
     
      row++;
      (*num_entries) = row;
   }

   sqlx->Done();

   if (gTrace)
      printf("hs_read_odbc: return %d events\n", *num_entries);

   return HS_SUCCESS;
}

// end

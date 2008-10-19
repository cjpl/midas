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

#if 0
int hs_read_odbc(uint32_t event_id, time_t start_time, time_t end_time,
                 time_t interval, char *tag_name, int var_index,
                 uint32_t * time_buffer, uint32_t * tbsize,
                 void *data_buffer, uint32_t * dbsize,
                 uint32_t * type, uint32_t * n)
{
  SQLHENV env;
  SQLHDBC dbc;
  SQLHSTMT stmt;
  SQLRETURN ret; /* ODBC API return status */
  SQLSMALLINT columns; /* number of columns in result-set */
  char cname[1024];
  SQLCHAR cmd[1024];
  int row = 0;
  int status;
  double *dptr;

  /* Allocate an environment handle */
  status = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
  if (!SQL_SUCCEEDED(status))
    {
      printf("SQLAllocHandle(SQL_HANDLE_ENV) error %d\n", status);
      exit(1);
    }

  /* We want ODBC 3 support */
  status = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
  if (!SQL_SUCCEEDED(status))
    {
      printf("SQLSetEnvAttr error %d\n", status);
      exit(1);
    }

  /* Allocate a connection handle */
  status = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
  if (!SQL_SUCCEEDED(status))
    {
      printf("SQLAllocHandle(SQL_HANDLE_DBC) error %d\n", status);
      exit(1);
    }

  /* Connect to the DSN mydsn */
  /* You will need to change mydsn to one you have created and tested */
  //SQLDriverConnect(dbc, NULL, "DSN=mysqlDSN;", SQL_NTS,
  //                 NULL, 0, NULL, SQL_DRIVER_COMPLETE);
  status = SQLConnect(dbc, (SQLCHAR*) gOdbcDsn, SQL_NTS,
                      (SQLCHAR*) "t2kmysqlwrite", SQL_NTS,
                      (SQLCHAR*) "neutwrite_280", SQL_NTS);
  //if ((status != SQL_SUCCESS) && (status != SQL_SUCCESS_WITH_INFO))
  if (!SQL_SUCCEEDED(status))
    {
      SQLINTEGER		 V_OD_err;
      SQLSMALLINT		 V_OD_mlen;
      SQLCHAR 		 V_OD_stat[10]; // Status SQL
      SQLCHAR            V_OD_msg[200];

      printf("SQLConnect() error %d\n", status);
      SQLGetDiagRec(SQL_HANDLE_DBC, dbc,1, 
                    V_OD_stat, &V_OD_err,V_OD_msg,100,&V_OD_mlen);
      printf("%s (%d)\n",V_OD_msg,V_OD_err);
      return -HS_SUCCESS;
    }

  /* Allocate a statement handle */
  status = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
  if (!SQL_SUCCEEDED(status))
    {
      printf("SQLAllocHandle(SQL_HANDLE_STMT) error %d\n", status);
      exit(1);
    }

  if (0)
    {
      /* Retrieve a list of tables */
      status = SQLTables(stmt, NULL, 0, NULL, 0, NULL, 0, (SQLCHAR*)"TABLE", SQL_NTS);
      if (!SQL_SUCCEEDED(status))
        {
          printf("SQLTables() error %d\n", status);
          exit(1);
        }
      
      /* How many columns are there */
      status = SQLNumResultCols(stmt, &columns);
      if (!SQL_SUCCEEDED(status))
        {
          printf("SQLNumResultCols() error %d\n", status);
          exit(1);
        }
      
      /* Loop through the rows in the result-set */
      while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
        SQLUSMALLINT i;
        printf("Row %d\n", row++);
        /* Loop through the columns */
        for (i = 1; i <= columns; i++) {
          SQLINTEGER indicator;
          char buf[512];
          /* retrieve column data as a string */
          ret = SQLGetData(stmt, i, SQL_C_CHAR,
                           buf, sizeof(buf), &indicator);
          if (SQL_SUCCEEDED(ret)) {
            /* Handle null columns */
            if (indicator == SQL_NULL_DATA) strcpy(buf, "NULL");
            printf("  Column %u : %s\n", i, buf);
          }
        }
      }
    }

  tagName2columnName(tag_name, cname);

  sprintf((char*)cmd, "SELECT _i_time, %s FROM event%d;", cname, event_id);

  printf("SQL Query: \'%s\'\n", cmd);

  status = SQLExecDirect(stmt,cmd,SQL_NTS);
  
  if (!SQL_SUCCEEDED(status))
    {
      int i;
      printf("SQLExecDirect error %d: %s\n", status, cmd);
      
      for (i=1; ; i++)
        {
            SQLCHAR 		 state[10]; // Status SQL
            SQLINTEGER		 error;
            SQLCHAR              message[1024];
            SQLSMALLINT		 mlen;
            
            status = SQLGetDiagRec(SQL_HANDLE_STMT,
                                   stmt,
                                   i,
                                   state,
                                   &error,
                                   message,
                                   sizeof(message),
                                   &mlen);

            if (!SQL_SUCCEEDED(status))
              break;

            printf("SQLGetDiagRec: state: \'%s\', message: \'%s\', native error: %d\n", state, message, error);
        }

      exit(1);
    }

  /* How many columns are there */
  status = SQLNumResultCols(stmt, &columns);
  if (!SQL_SUCCEEDED(status))
    {
      printf("SQLNumResultCols() error %d\n", status);
      exit(1);
    }

  *n = 0;
  *type = TID_DOUBLE;
  dptr = (double*)data_buffer;
  
  /* Loop through the rows in the result-set */
  row = 0;
  while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
    SQLCHAR col1[256];
    SQLCHAR col2[256];
    SQLINTEGER indicator1, indicator2;
    int t = 0;
    double v = 0;

    status = SQLGetData(stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &indicator1);
    if (SQL_SUCCEEDED(status)) {
      /* Handle null columns */
      if (indicator1 == SQL_NULL_DATA) t = 9999;
      else t = atoi((char*)col1);
    }

    status = SQLGetData(stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &indicator2);
    if (SQL_SUCCEEDED(status)) {
      /* Handle null columns */
      if (indicator2 == SQL_NULL_DATA) v = 9999;
      else v = atof((char*)col2);
    }

    row++;

    if (t < start_time || t > end_time)
      continue;

    printf("Row %d, columns [%s] [%s], time %d, value %f\n", row, col1, col2, t, v);

    time_buffer[*n] = t;
    dptr[*n] = v;
    (*n)++;

    if ((*n) * sizeof(DWORD) >= *tbsize || (*n) * sizeof(double) >= *dbsize) {
      *dbsize = (*n) * sizeof(double);
      *tbsize = (*n) * sizeof(DWORD);
      return HS_TRUNCATED;
    }


    //exit(123);
  }
  //exit(1);

  *dbsize = (*n) * sizeof(double);
  *tbsize = (*n) * sizeof(DWORD);

  return HS_SUCCESS;
}
#endif

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
  virtual int GetNumColumns() = 0;
  virtual int Fetch() = 0;
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

  int GetNumColumns() { return 0; }
  int Fetch() { return 0; }
  const char* GetColumn(int icol) { return NULL; };
};

class SqlODBC: public SqlBase
{
public:

  bool fIsConnected;

  SQLHENV  fEnv;
  SQLHDBC  fDB;
  SQLHSTMT fStmt;

  SqlODBC() // ctor
  {
    fIsConnected = false;
  }

  int Connect(const char* dsn)
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

    fIsConnected = true;

    return 0;
  }
  
  bool IsConnected()
  {
     return fIsConnected;
  }

  int Exec(const char* sql)
  {

    if (!fIsConnected)
      return -1;

    if (gTrace)
      printf("SqlODBC::Exec: %s\n", sql);

    int status = SQLExecDirect(fStmt,(SQLCHAR*)sql,SQL_NTS);

    if (!SQL_SUCCEEDED(status))
      {
        //cm_msg(MERROR, "SqlODBC::Exec", "SQLExecDirect() error %d", status);

        if (gTrace)
          printf("SqlODBC::Exec: SQLExecDirect() error %d: SQL command: \"%s\"\n", status, sql);

        for (int i=1; ; i++)
          {
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

            if (!SQL_SUCCEEDED(status))
	    {
	        if (status != 100)
	    	   cm_msg(MERROR, "SqlODBC::Exec", "SQLGetDiagRec() error %d", status);
              	break;
	    }
	      
	    if ((error != 1060) && (error != 1050))
            	cm_msg(MERROR, "SqlODBC::Exec", "SQLGetDiagRec: state: \'%s\', message: \'%s\', native error: %d", state, message, error);
          }

        return -1;
      }

    return 0;
  }

  int Disconnect()
  {
    if (!fIsConnected)
      return 0;

    SQLDisconnect(fDB);

    SQLFreeHandle(SQL_HANDLE_DBC,  fDB);
    SQLFreeHandle(SQL_HANDLE_STMT, fStmt);
    SQLFreeHandle(SQL_HANDLE_ENV,  fEnv);

    fIsConnected = false;
    return 0;
  }

  ~SqlODBC() // dtor
  {
    Disconnect();
  }

  int GetNumColumns();
  int Fetch();
  const char* GetColumn(int icol);
};

int SqlODBC::GetNumColumns()
{
  SQLSMALLINT ncols = 0;
  /* How many columns are there */
  int status = SQLNumResultCols(fStmt, &ncols);
  if (!SQL_SUCCEEDED(status))
    {
      cm_msg(MERROR, "SqlODBC::GetNumColumns", "SQLNumResultCols() error %d", status);
      return -1;
    }
  return ncols;
}

int SqlODBC::Fetch()
{
  return SQL_SUCCEEDED(SQLFetch(fStmt));
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

int hs_read_odbc(time_t start_time, time_t end_time, time_t interval,
                 const char* event_name, const char *tag_name, int var_index,
                 uint32_t * time_buffer, uint32_t * tbsize,
                 void *data_buffer, uint32_t * dbsize,
                 uint32_t * type, uint32_t * n)
{
  assert(sqlx);

  std::string ename = tagName2columnName(event_name);
  std::string tname = tagName2columnName(tag_name);

  char cmd[256];
  sprintf(cmd, "SELECT _i_time, %s FROM %s;", tname.c_str(), ename.c_str());

  int status = sqlx->Exec(cmd);

  if (status)
    return HS_FILE_ERROR;

  int ncols = sqlx->GetNumColumns();

  if (ncols < 1)
    return HS_FILE_ERROR;

  *n = 0;
  *type = TID_DOUBLE;

  double *dptr = (double*)data_buffer;
  
  /* Loop through the rows in the result-set */
  int row = 0;
  while (sqlx->Fetch())
    {
      time_t t = 0;
      double v = 0;

      const char* timedata = sqlx->GetColumn(1);
      if (timedata)
        t = atoi(timedata);

      const char* valuedata = sqlx->GetColumn(2);
      if (valuedata)
        v = atof(valuedata);
      
      row++;
      
      if (t < start_time || t > end_time)
        continue;
      
      //printf("Row %d, columns [%s] [%s], time %d, value %f\n", row, col1, col2, t, v);
      
      time_buffer[*n] = t;
      dptr[*n] = v;
      (*n)++;
      
      if ((*n) * sizeof(DWORD) >= *tbsize || (*n) * sizeof(double) >= *dbsize) {
        *dbsize = (*n) * sizeof(double);
        *tbsize = (*n) * sizeof(DWORD);
        return HS_TRUNCATED;
      }
    }

  *dbsize = (*n) * sizeof(double);
  *tbsize = (*n) * sizeof(DWORD);

  return HS_SUCCESS;
}

// end

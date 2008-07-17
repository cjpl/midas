/********************************************************************\

  Name:         mlogger.c
  Created by:   Stefan Ritt

  Contents:     MIDAS logger program

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "hardware.h"
#include <errno.h>              /* for mkdir() */
#include <assert.h>

#define INCLUDE_LOGGING
#include "ybos.h"

#ifdef HAVE_ROOT
#undef GetCurrentTime
#include "TApplication.h"
#include "TFile.h"
#include "TNtuple.h"
#include "TLeaf.h"
#endif

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#ifdef HAVE_MYSQL
#ifdef OS_UNIX
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#endif
#ifdef OS_WINNT
#include <mysql.h>
#include <mysqld_error.h>
int errno;                      // under NT, "ignore libcd" is required, so errno has to be defined here
#endif
void create_sql_tree();
#endif

/*---- globals -----------------------------------------------------*/

#ifndef DEBUG_TRANS
#define DEBUG_TRANS 0
#endif

#define LOGGER_TIMEOUT 60000

#define MAX_CHANNELS 10
#define MAX_HISTORY 100

BOOL in_stop_transition = FALSE;
BOOL tape_message = TRUE;
DWORD auto_restart = 0;
BOOL verbose = FALSE;

LOG_CHN log_chn[MAX_CHANNELS];

struct hist_log_s {
   WORD event_id;
   void *buffer;
   INT buffer_size;
   HNDLE hKeyVar;
   DWORD n_var;
   DWORD period;
   DWORD last_log;
} hist_log[MAX_HISTORY];

HNDLE hDB;

/*---- ODB records -------------------------------------------------*/

CHN_SETTINGS_STR(chn_settings_str);

/*---- data structures for MIDAS format ----------------------------*/

typedef struct {
   char *buffer;
   char *write_pointer;
} MIDAS_INFO;

/*---- forward declarations ----------------------------------------*/

void receive_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER * pheader, void *pevent);
INT log_write(LOG_CHN * log_chn, EVENT_HEADER * pheader);
void log_system_history(HNDLE hDB, HNDLE hKey, void *info);

/*== common code FAL/MLOGGER start =================================*/

/*---- Logging initialization --------------------------------------*/

void logger_init()
{
   INT size, status;
   BOOL flag;
   HNDLE hKey;
   char str[256];

   /*---- create /logger entries -----*/

   cm_get_path(str);
   size = sizeof(str);
   db_get_value(hDB, 0, "/Logger/Data dir", str, &size, TID_STRING, TRUE);

   strcpy(str, "midas.log");
   size = sizeof(str);
   db_get_value(hDB, 0, "/Logger/Message file", str, &size, TID_STRING, TRUE);

   size = sizeof(BOOL);
   flag = TRUE;
   db_get_value(hDB, 0, "/Logger/Write data", &flag, &size, TID_BOOL, TRUE);

   flag = FALSE;
   db_get_value(hDB, 0, "/Logger/ODB Dump", &flag, &size, TID_BOOL, TRUE);

   strcpy(str, "run%05d.odb");
   size = sizeof(str);
   db_get_value(hDB, 0, "/Logger/ODB Dump File", str, &size, TID_STRING, TRUE);

   flag = FALSE;
   size = sizeof(BOOL);
   db_get_value(hDB, 0, "/Logger/Auto restart", &flag, &size, TID_BOOL, TRUE);

   flag = TRUE;
   db_get_value(hDB, 0, "/Logger/Tape message", &flag, &size, TID_BOOL, TRUE);

   /* create at least one logging channel */
   status = db_find_key(hDB, 0, "/Logger/Channels/0", &hKey);
   if (status != DB_SUCCESS) {
      /* if no channels are defined, define at least one */
      status = db_create_record(hDB, 0, "/Logger/Channels/0", strcomb(chn_settings_str));
      if (status != DB_SUCCESS)
         cm_msg(MERROR, "logger_init", "Cannot create channel entry in database");
   }
#ifdef HAVE_MYSQL
   create_sql_tree();
#endif
}

/*---- ODB dump routine --------------------------------------------*/

void log_odb_dump(LOG_CHN * log_chn, short int event_id, INT run_number)
{
   INT status, buffer_size, size;
   EVENT_HEADER *pevent;

   /* write ODB dump */
   buffer_size = 100000;
   do {
      pevent = (EVENT_HEADER *) malloc(buffer_size);
      if (pevent == NULL) {
         cm_msg(MERROR, "log_odb_dump", "Cannot allocate ODB dump buffer");
         break;
      }

      size = buffer_size - sizeof(EVENT_HEADER);
      status = db_copy_xml(hDB, 0, (char *) (pevent + 1), &size);

      /* following line would dump ODB in old ASCII format instead of XML */
      //status = db_copy(hDB, 0, (char *) (pevent + 1), &size, "");
      if (status != DB_TRUNCATED) {
         bm_compose_event(pevent, event_id, MIDAS_MAGIC,
                          buffer_size - sizeof(EVENT_HEADER) - size + 1, run_number);
         log_write(log_chn, pevent);
         free(pevent);
         break;
      }

      /* increase buffer size if truncated */
      free(pevent);
      buffer_size *= 10;
   } while (1);
}

/*---- ODB save routine --------------------------------------------*/

void odb_save(char *filename)
{
   int size;
   char dir[256];
   char path[256];

   if (strchr(filename, DIR_SEPARATOR) == NULL) {
      size = sizeof(dir);
      dir[0] = 0;
      db_get_value(hDB, 0, "/Logger/Data Dir", dir, &size, TID_STRING, TRUE);
      if (dir[0] != 0)
         if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
            strcat(dir, DIR_SEPARATOR_STR);
      strcpy(path, dir);
      strcat(path, filename);
   } else
      strcpy(path, filename);

   if (strstr(filename, ".xml") || strstr(filename, ".XML"))
      db_save_xml(hDB, 0, path);
   else
      db_save(hDB, 0, path, FALSE);
}


#ifdef HAVE_MYSQL

/*==== SQL routines =================================================*/

/*---- Convert ctime() type date/time to SQL 'datetime' -------------*/

typedef struct {
   char column_name[NAME_LENGTH];
   char column_type[NAME_LENGTH];
   char data[256];
} SQL_LIST;

char *mname[] = {
   "January",
   "February",
   "March",
   "April",
   "May",
   "June",
   "July",
   "August",
   "September",
   "October",
   "November",
   "December"
};

void ctime_to_datetime(char *date)
{
   char ctime_date[30];
   struct tm tms;
   int i;

   strlcpy(ctime_date, date, sizeof(ctime_date));
   memset(&tms, 0, sizeof(struct tm));

   for (i = 0; i < 12; i++)
      if (strncmp(ctime_date + 4, mname[i], 3) == 0)
         break;
   tms.tm_mon = i;

   tms.tm_mday = atoi(ctime_date + 8);
   tms.tm_hour = atoi(ctime_date + 11);
   tms.tm_min = atoi(ctime_date + 14);
   tms.tm_sec = atoi(ctime_date + 17);
   tms.tm_year = atoi(ctime_date + 20) - 1900;
   tms.tm_isdst = -1;

   if (tms.tm_year < 90)
      tms.tm_year += 100;

   mktime(&tms);
   sprintf(date, "%d-%02d-%02d %02d-%02d-%02d",
           tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday, tms.tm_hour, tms.tm_min, tms.tm_sec);
}

/*---- mySQL debugging output --------------------------------------*/

int mysql_query_debug(MYSQL * db, char *query)
{
   int status, size, fh;
   char filename[256], path[256], dir[256];
   HNDLE hKey;

   /* comment in this line if you need debugging output */
   //cm_msg(MINFO, "mysql_query_debug", "SQL query: %s", query);

   /* write query into logfile if requested */
   size = sizeof(filename);
   filename[0] = 0;
   db_get_value(hDB, 0, "/Logger/SQL/Logfile", filename, &size, TID_STRING, TRUE);
   if (filename[0]) {
      status = db_find_key(hDB, 0, "/Logger/Data dir", &hKey);
      if (status == DB_SUCCESS) {
         size = sizeof(dir);
         memset(dir, 0, size);
         db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);
         if (dir[0] != 0)
            if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
               strcat(dir, DIR_SEPARATOR_STR);

         strcpy(path, dir);
         strcat(path, filename);
      } else {
         cm_get_path(dir);
         if (dir[0] != 0)
            if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
               strcat(dir, DIR_SEPARATOR_STR);

         strcpy(path, dir);
         strcat(path, filename);
      }

      fh = open(path, O_WRONLY | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
      if (fh < 0) {
         printf("Cannot open message log file %s\n", path);
      } else {
         write(fh, query, strlen(query));
         write(fh, "\n", 1);
         close(fh);
      }
   }

   /* execut sql query */
   status = mysql_query(db, query);

   if (status)
      cm_msg(MERROR, "mysql_query_debug", "SQL error: %s", mysql_error(db));

   return status;
}

/*---- Retrieve list of columns from ODB tree ----------------------*/

int sql_get_columns(HNDLE hKeyRoot, SQL_LIST ** sql_list)
{
   HNDLE hKey;
   int n, i, size, status;
   KEY key;
   char str[256], data[256];

   for (i = 0;; i++) {
      status = db_enum_key(hDB, hKeyRoot, i, &hKey);
      if (status == DB_NO_MORE_SUBKEYS)
         break;
   }

   if (i == 0)
      return 0;

   n = i;

   *sql_list = (SQL_LIST *) malloc(sizeof(SQL_LIST) * n);

   for (i = 0; i < n; i++) {

      /* get name of link, NOT of link target */
      db_enum_link(hDB, hKeyRoot, i, &hKey);
      db_get_link(hDB, hKey, &key);
      strlcpy((*sql_list)[i].column_name, key.name, NAME_LENGTH);

      /* get key */
      db_enum_key(hDB, hKeyRoot, i, &hKey);
      db_get_key(hDB, hKey, &key);

      /* get key data */
      size = sizeof(data);
      db_get_data(hDB, hKey, data, &size, key.type);
      db_sprintf(str, data, size, 0, key.type);
      if (key.type == TID_BOOL)
         strcpy((*sql_list)[i].data, str[0] == 'y' || str[0] == 'Y' ? "1" : "0");
      else
         strcpy((*sql_list)[i].data, str);

      if (key.type == TID_STRING) {
         /* check if string is date/time */
         if (strlen(str) == 24 && str[10] == ' ' && str[13] == ':') {
            strcpy(str, "DATETIME");
            ctime_to_datetime((*sql_list)[i].data);
         } else if (key.item_size < 256)
            sprintf(str, "VARCHAR (%d)", key.item_size);
         else
            sprintf(str, " TEXT");

      } else {
         switch (key.type) {
         case TID_BYTE:
            strcpy(str, "TINYINT UNSIGNED ");
            break;
         case TID_SBYTE:
            strcpy(str, "TINYINT  ");
            break;
         case TID_CHAR:
            strcpy(str, "CHAR ");
            break;
         case TID_WORD:
            strcpy(str, "SMALLINT UNSIGNED ");
            break;
         case TID_SHORT:
            strcpy(str, "SMALLINT ");
            break;
         case TID_DWORD:
            strcpy(str, "INT UNSIGNED ");
            break;
         case TID_INT:
            strcpy(str, "INT ");
            break;
         case TID_BOOL:
            strcpy(str, "BOOLEAN ");
            break;
         case TID_FLOAT:
            strcpy(str, "FLOAT ");
            break;
         case TID_DOUBLE:
            strcpy(str, "DOUBLE ");
            break;
         default:
            cm_msg(MERROR, "sql_create_database",
                   "No SQL type mapping for key \"%s\" of type %s", key.name, rpc_tid_name(key.type));
         }
      }

      strcpy((*sql_list)[i].column_type, str);
   }

   return n;
}

/*---- Create mySQL table from ODB tree ----------------------------*/

BOOL sql_create_table(MYSQL * db, char *database, char *table, HNDLE hKeyRoot)
{
   char str[256], query[5000];
   int i, n_col;
   SQL_LIST *sql_list;

   sprintf(query, "CREATE TABLE `%s`.`%s` (", database, table);

   n_col = sql_get_columns(hKeyRoot, &sql_list);
   if (n_col == 0) {
      db_get_path(hDB, hKeyRoot, str, sizeof(str));
      cm_msg(MERROR, "sql_create_database", "ODB tree \"%s\" contains no variables", str);
      return FALSE;
   }

   for (i = 0; i < n_col; i++)
      sprintf(query + strlen(query), "`%s` %s  NOT NULL, ", sql_list[i].column_name, sql_list[i].column_type);

   sprintf(query + strlen(query), "PRIMARY KEY (`%s`))", sql_list[0].column_name);
   free(sql_list);

   if (mysql_query_debug(db, query)) {
      cm_msg(MERROR, "sql_create_table", "Failed to create table: Error: %s", mysql_error(db));
      return FALSE;
   }

   return TRUE;
}

/*---- Create mySQL table from ODB tree ----------------------------*/

BOOL sql_modify_table(MYSQL * db, char *database, char *table, HNDLE hKeyRoot)
{
   char str[256], query[5000];
   int i, n_col;
   SQL_LIST *sql_list;

   n_col = sql_get_columns(hKeyRoot, &sql_list);
   if (n_col == 0) {
      db_get_path(hDB, hKeyRoot, str, sizeof(str));
      cm_msg(MERROR, "sql_modify_table", "ODB tree \"%s\" contains no variables", str);
      return FALSE;
   }

   for (i = 0; i < n_col; i++) {

      /* try to add column */
      if (i == 0)
         sprintf(query, "ALTER TABLE `%s`.`%s` ADD `%s` %s",
                 database, table, sql_list[i].column_name, sql_list[i].column_type);
      else
         sprintf(query, "ALTER TABLE `%s`.`%s` ADD `%s` %s AFTER `%s`",
                 database, table, sql_list[i].column_name, sql_list[i].column_type,
                 sql_list[i - 1].column_name);

      if (mysql_query_debug(db, query)) {
         if (mysql_errno(db) == ER_DUP_FIELDNAME) {

            /* try to modify column */
            sprintf(query, "ALTER TABLE `%s`.`%s` MODIFY `%s` %s",
                    database, table, sql_list[i].column_name, sql_list[i].column_type);

            if (mysql_query_debug(db, query)) {
               free(sql_list);
               cm_msg(MERROR, "sql_modify_table", "Failed to modify column: Error: %s", mysql_error(db));
               return FALSE;
            }

         } else {
            free(sql_list);
            cm_msg(MERROR, "sql_modify_table", "Failed to add column: Error: %s", mysql_error(db));
            return FALSE;
         }
      }
   }

   cm_msg(MINFO, "sql_insert", "SQL table '%s.%s' modified successfully", database, table);

   return TRUE;
}

/*---- Create mySQL database ---------------------------------------*/

BOOL sql_create_database(MYSQL * db, char *database)
{
   char query[256];

   sprintf(query, "CREATE DATABASE `%s`", database);
   if (mysql_query_debug(db, query)) {
      cm_msg(MERROR, "sql_create_database", "Failed to create database: Error: %s", mysql_error(db));
      return FALSE;
   }

   /* select database */
   sprintf(query, "USE `%s`", database);
   if (mysql_query_debug(db, query)) {
      cm_msg(MERROR, "sql_create_database", "Failed to select database: Error: %s", mysql_error(db));
      return FALSE;
   }

   return TRUE;
}

/*---- Insert table row from ODB tree -------------------------------*/

int sql_insert(MYSQL * db, char *database, char *table, HNDLE hKeyRoot, BOOL create_flag)
{
   char query[5000], *pstr;
   int status, i, j, n_col;
   SQL_LIST *sql_list;

   /* 
      build SQL query in the form 
      "INSERT INTO `<table>` (`<name>`, <name`,..) VALUES (`<value>`, `value`, ...) 
    */
   sprintf(query, "INSERT INTO `%s`.`%s` (", database, table);
   n_col = sql_get_columns(hKeyRoot, &sql_list);
   if (n_col == 0)
      return DB_SUCCESS;

   for (i = 0; i < n_col; i++) {
      sprintf(query + strlen(query), "`%s`", sql_list[i].column_name);
      if (i < n_col - 1)
         strcat(query, ", ");
   }

   strlcat(query, ") VALUES (", sizeof(query));

   for (i = 0; i < n_col; i++) {
      strcat(query, "'");

      /* convert "\" to "\\" (MySQL standard) */
      for (j = 0; j < (int) strlen(sql_list[i].data); j++) {
         if (sql_list[i].data[j] == '\\')
            strcat(query, "\\\\");
         else {
            pstr = query + strlen(query);
            *pstr++ = sql_list[i].data[j];
            *pstr = 0;
         }
      }

      strcat(query, "'");

      if (i < n_col - 1)
         strcat(query, ", ");
   }

   free(sql_list);
   strlcat(query, ")", sizeof(query));
   if (mysql_query_debug(db, query)) {

      /* if entry for this run exists alreay return */
      if (mysql_errno(db) == ER_DUP_ENTRY) {

         return ER_DUP_ENTRY;

      } else if (mysql_errno(db) == ER_NO_SUCH_TABLE && create_flag) {

         /* if table does not exist, creat it and try again */
         sql_create_table(db, database, table, hKeyRoot);
         if (mysql_query_debug(db, query)) {
            cm_msg(MERROR, "sql_insert", "Failed to update database: Error: %s", mysql_error(db));
            return mysql_errno(db);
         }
         cm_msg(MINFO, "sql_insert", "SQL table '%s.%s' created successfully", database, table);

      } else if (mysql_errno(db) == ER_BAD_FIELD_ERROR && create_flag) {

         /* if table structure is different, adjust it and try again */
         sql_modify_table(db, database, table, hKeyRoot);
         if (mysql_query_debug(db, query)) {
            cm_msg(MERROR, "sql_insert", "Failed to update database: Error: %s", mysql_error(db));
            return mysql_errno(db);
         }

      } else {
         status = mysql_errno(db);
         cm_msg(MERROR, "sql_insert", "Failed to update database: Error: %s", mysql_error(db));
         return mysql_errno(db);
      }
   }

   return DB_SUCCESS;
}

/*---- Update table row from ODB tree ------------------------------*/

int sql_update(MYSQL * db, char *database, char *table, HNDLE hKeyRoot, BOOL create_flag, char *where)
{
   char query[5000];
   int i, n_col;
   SQL_LIST *sql_list;

   /* 
      build SQL query in the form 
      "UPDATE `<database`.`<table>` SET `<name>`='<value', ... WHERE `<name>`='value' 
    */

   sprintf(query, "UPDATE `%s`.`%s` SET ", database, table);
   n_col = sql_get_columns(hKeyRoot, &sql_list);
   if (n_col == 0)
      return DB_SUCCESS;

   for (i = 0; i < n_col; i++) {
      sprintf(query + strlen(query), "`%s`='%s'", sql_list[i].column_name, sql_list[i].data);
      if (i < n_col - 1)
         strcat(query, ", ");
   }
   free(sql_list);

   sprintf(query + strlen(query), " %s", where);
   if (mysql_query_debug(db, query)) {
      if (mysql_errno(db) == ER_NO_SUCH_TABLE && create_flag) {

         /* if table does not exist, creat it and try again */
         sql_create_table(db, database, table, hKeyRoot);
         return sql_insert(db, database, table, hKeyRoot, create_flag);

      } else if (mysql_errno(db) == ER_BAD_FIELD_ERROR && create_flag) {

         /* if table structure is different, adjust it and try again */
         sql_modify_table(db, database, table, hKeyRoot);
         if (mysql_query_debug(db, query)) {
            cm_msg(MERROR, "sql_update", "Failed to update database: Error: %s", mysql_error(db));
            return mysql_errno(db);
         }

      } else {
         cm_msg(MERROR, "sql_update", "Failed to update database: Error: %s", mysql_error(db));
         return mysql_errno(db);
      }
   }

   return DB_SUCCESS;
}

/*---- Write ODB tree to SQL table ----------------------------------*/

void create_sql_tree()
{
   char hostname[80], username[80], password[80], database[80], table[80], filename[80];
   int size, write_flag, create_flag;
   HNDLE hKeyRoot, hKey;

   size = sizeof(create_flag);
   create_flag = 0;
   db_get_value(hDB, 0, "/Logger/SQL/Create database", &create_flag, &size, TID_BOOL, TRUE);

   size = sizeof(write_flag);
   write_flag = 0;
   db_get_value(hDB, 0, "/Logger/SQL/Write data", &write_flag, &size, TID_BOOL, TRUE);

   size = sizeof(hostname);
   strcpy(hostname, "localhost");
   db_get_value(hDB, 0, "/Logger/SQL/Hostname", hostname, &size, TID_STRING, TRUE);

   size = sizeof(username);
   strcpy(username, "root");
   db_get_value(hDB, 0, "/Logger/SQL/Username", username, &size, TID_STRING, TRUE);

   size = sizeof(password);
   password[0] = 0;
   db_get_value(hDB, 0, "/Logger/SQL/Password", password, &size, TID_STRING, TRUE);

   /* use experiment name as default database name */
   size = sizeof(database);
   db_get_value(hDB, 0, "/Experiment/Name", database, &size, TID_STRING, TRUE);
   db_get_value(hDB, 0, "/Logger/SQL/Database", database, &size, TID_STRING, TRUE);

   size = sizeof(table);
   strcpy(table, "Runlog");
   db_get_value(hDB, 0, "/Logger/SQL/Table", table, &size, TID_STRING, TRUE);

   size = sizeof(filename);
   strcpy(filename, "sql.log");
   db_get_value(hDB, 0, "/Logger/SQL/Logfile", filename, &size, TID_STRING, TRUE);

   db_find_key(hDB, 0, "/Logger/SQL/Links BOR", &hKeyRoot);
   if (!hKeyRoot) {
      /* create some default links */
      db_create_key(hDB, 0, "/Logger/SQL/Links BOR", TID_KEY);

      if (db_find_key(hDB, 0, "/Runinfo/Run number", &hKey) == DB_SUCCESS)
         db_create_link(hDB, 0, "/Logger/SQL/Links BOR/Run number", "/Runinfo/Run number");

      if (db_find_key(hDB, 0, "/Experiment/Run parameters/Comment", &hKey) == DB_SUCCESS)
         db_create_link(hDB, 0, "/Logger/SQL/Links BOR/Comment", "/Experiment/Run parameters/Comment");

      if (db_find_key(hDB, 0, "/Runinfo/Start time", &hKey) == DB_SUCCESS)
         db_create_link(hDB, 0, "/Logger/SQL/Links BOR/Start time", "/Runinfo/Start time");
   }

   db_find_key(hDB, 0, "/Logger/SQL/Links EOR", &hKeyRoot);
   if (!hKeyRoot) {
      /* create some default links */
      db_create_key(hDB, 0, "/Logger/SQL/Links EOR", TID_KEY);

      if (db_find_key(hDB, 0, "/Runinfo/Stop time", &hKey) == DB_SUCCESS)
         db_create_link(hDB, 0, "/Logger/SQL/Links EOR/Stop time", "/Runinfo/Stop time");

      if (db_find_key(hDB, 0, "/Equipment/Trigger/Statistics/Events sent", &hKey) == DB_SUCCESS)
         db_create_link(hDB, 0, "/Logger/SQL/Links EOR/Number of events",
                        "/Equipment/Trigger/Statistics/Events sent");

   }
}

/*---- Write ODB tree to SQL table ----------------------------------*/

void write_sql(BOOL bor)
{
   MYSQL db;
   char hostname[80], username[80], password[80], database[80], table[80], query[5000], where[500];
   int status, size, write_flag, create_flag;
   BOOL insert;
   HNDLE hKey, hKeyRoot;
   SQL_LIST *sql_list;

   /* do not update SQL if logger does not write data */
   size = sizeof(BOOL);
   write_flag = FALSE;
   db_get_value(hDB, 0, "/Logger/Write data", &write_flag, &size, TID_BOOL, TRUE);
   if (!write_flag)
      return;

   /* insert SQL on bor, else update */
   insert = bor;

   /* determine primary key */
   db_find_key(hDB, 0, "/Logger/SQL/Links BOR", &hKeyRoot);
   status = db_enum_link(hDB, hKeyRoot, 0, &hKey);

   /* if BOR list empty, take first one from EOR list */
   if (status == DB_NO_MORE_SUBKEYS) {
      insert = TRUE;
      db_find_key(hDB, 0, "/Logger/SQL/Links EOR", &hKeyRoot);
      status = db_enum_link(hDB, hKeyRoot, 0, &hKey);
      if (status == DB_NO_MORE_SUBKEYS)
         return;
   }

   sql_get_columns(hKeyRoot, &sql_list);
   sprintf(where, "WHERE `%s`='%s'", sql_list[0].column_name, sql_list[0].data);
   free(sql_list);

   /* get BOR or EOR list */
   if (bor) {
      db_find_key(hDB, 0, "/Logger/SQL/Links BOR", &hKeyRoot);
      if (!hKeyRoot) {
         cm_msg(MERROR, "write_sql", "Cannot find \"/Logger/SQL/Links BOR");
         return;
      }
   } else {
      db_find_key(hDB, 0, "/Logger/SQL/Links EOR", &hKeyRoot);
      if (!hKeyRoot) {
         cm_msg(MERROR, "write_sql", "Cannot find \"/Logger/SQL/Links EOR");
         return;
      }
   }

   size = sizeof(create_flag);
   create_flag = 0;
   db_get_value(hDB, 0, "/Logger/SQL/Create database", &create_flag, &size, TID_BOOL, TRUE);

   size = sizeof(write_flag);
   write_flag = 0;
   db_get_value(hDB, 0, "/Logger/SQL/Write data", &write_flag, &size, TID_BOOL, TRUE);

   size = sizeof(hostname);
   strcpy(hostname, "localhost");
   db_get_value(hDB, 0, "/Logger/SQL/Hostname", hostname, &size, TID_STRING, TRUE);

   size = sizeof(username);
   strcpy(username, "root");
   db_get_value(hDB, 0, "/Logger/SQL/Username", username, &size, TID_STRING, TRUE);

   size = sizeof(password);
   password[0] = 0;
   db_get_value(hDB, 0, "/Logger/SQL/Password", password, &size, TID_STRING, TRUE);

   /* use experiment name as default database name */
   size = sizeof(database);
   db_get_value(hDB, 0, "/Experiment/Name", database, &size, TID_STRING, TRUE);
   db_get_value(hDB, 0, "/Logger/SQL/Database", database, &size, TID_STRING, TRUE);

   size = sizeof(table);
   strcpy(table, "Runlog");
   db_get_value(hDB, 0, "/Logger/SQL/Table", table, &size, TID_STRING, TRUE);

   /* continue only if data should be written */
   if (!write_flag)
      return;

   /* connect to MySQL database */
   mysql_init(&db);

   if (!mysql_real_connect(&db, hostname, username, password, NULL, 0, NULL, 0)) {
      cm_msg(MERROR, "write_sql", "Failed to connect to database: Error: %s", mysql_error(&db));
      mysql_close(&db);
      return;
   }

   /* select database */
   sprintf(query, "USE `%s`", database);
   if (mysql_query_debug(&db, query)) {

      /* create database if selected */
      if (create_flag) {
         if (!sql_create_database(&db, database)) {
            mysql_close(&db);
            return;
         }
         cm_msg(MINFO, "write_sql", "Database \"%s\" created successfully", database);

      } else {
         cm_msg(MERROR, "write_sql", "Failed to select database: Error: %s", mysql_error(&db));
         mysql_close(&db);
         return;
      }
   }

   if (insert) {
      status = sql_insert(&db, database, table, hKeyRoot, create_flag);
      if (status == ER_DUP_ENTRY)
         sql_update(&db, database, table, hKeyRoot, create_flag, where);
   } else
      sql_update(&db, database, table, hKeyRoot, create_flag, where);

   mysql_close(&db);
}

#endif                          // HAVE_MYSQL

/*---- open tape and check for data --------------------------------*/

INT tape_open(char *dev, INT * handle)
{
   INT status, count;
   char buffer[16];

   status = ss_tape_open(dev, O_RDWR | O_CREAT | O_TRUNC, handle);
   if (status != SS_SUCCESS)
      return status;

   /* check if tape contains data */
   count = sizeof(buffer);
   status = ss_tape_read(*handle, buffer, &count);

   if (count == sizeof(buffer)) {
      /* tape contains data -> don't start */
      ss_tape_rskip(*handle, -1);
      cm_msg(MINFO, "tape_open", "Tape contains data, please spool tape with 'mtape seod'");
      cm_msg(MINFO, "tape_open", "or erase it with 'mtape weof', 'mtape rewind', then try again.");
      ss_tape_close(*handle);
      return SS_TAPE_ERROR;
   }

   return SS_SUCCESS;
}

/*---- open FTP channel --------------------------------------------*/

INT ftp_error(char *message)
{
   cm_msg(MERROR, "ftp_error", message);
   return 1;
}

INT ftp_open(char *destination, FTP_CON ** con)
{
   INT status;
   short port = 0;
   char *token, host_name[HOST_NAME_LENGTH],
       user[32], pass[32], directory[256], file_name[256], file_mode[256];

   /*
      destination should have the form:
      host, port, user, password, directory, run%05d.mid
    */

   /* break destination in components */
   token = strtok(destination, ",");
   if (token)
      strcpy(host_name, token);

   token = strtok(NULL, ", ");
   if (token)
      port = atoi(token);

   token = strtok(NULL, ", ");
   if (token)
      strcpy(user, token);

   token = strtok(NULL, ", ");
   if (token)
      strcpy(pass, token);

   token = strtok(NULL, ", ");
   if (token)
      strcpy(directory, token);

   token = strtok(NULL, ", ");
   if (token)
      strcpy(file_name, token);

   token = strtok(NULL, ", ");
   file_mode[0] = 0;
   if (token)
      strcpy(file_mode, token);

#ifdef FAL_MAIN
   ftp_debug(NULL, ftp_error);
#else
   ftp_debug((int (*)(char *)) puts, ftp_error);
#endif

   status = ftp_login(con, host_name, port, user, pass, "");
   if (status >= 0)
      return status;

   status = ftp_chdir(*con, directory);
   if (status >= 0)
      return status;

   status = ftp_binary(*con);
   if (status >= 0)
      return status;

   if (file_mode[0]) {
      status = ftp_command(*con, "umask %s", file_mode, 200, 250, EOF);
      if (status >= 0)
         return status;
   }

   if (ftp_open_write(*con, file_name) >= 0)
      return (*con)->err_no;

   return SS_SUCCESS;
}

/*---- MIDAS format routines ---------------------------------------*/

INT midas_flush_buffer(LOG_CHN * log_chn)
{
   INT i, size, written;
   MIDAS_INFO *info;

   info = (MIDAS_INFO *) log_chn->format_info;
   size = (POINTER_T) info->write_pointer - (POINTER_T) info->buffer;

   if (size == 0)
      return 0;

   /* write record to device */
   if (log_chn->type == LOG_TYPE_TAPE)
      written = ss_tape_write(log_chn->handle, info->buffer, size);
   else if (log_chn->type == LOG_TYPE_FTP)
      written =
          ftp_send(((FTP_CON *) log_chn->ftp_con)->data, info->buffer,
                   size) == size ? SS_SUCCESS : SS_FILE_ERROR;
   else if (log_chn->gzfile) {
#ifdef HAVE_ZLIB
      z_streamp s;

      s = (z_streamp) log_chn->gzfile;
      written = s->total_out;
      i = gzwrite(log_chn->gzfile, info->buffer, size);
      if (i != size)
         return -1;
      written = s->total_out - written;
#else
      assert(!"this cannot happen! support for ZLIB not compiled in");
#endif
   } else if (log_chn->handle) {
#ifdef OS_WINNT
      WriteFile((HANDLE) log_chn->handle, info->buffer, size, (unsigned long *) &written, NULL);
#else
      written = write(log_chn->handle, info->buffer, size);
#endif
   } else {
      /* we are writing into the void!?! */
      written = 0;
   }

   info->write_pointer = info->buffer;

   return written;
}


/*------------------------------------------------------------------*/

INT midas_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size)
{
   INT i, written, size_left;
   MIDAS_INFO *info;

   info = (MIDAS_INFO *) log_chn->format_info;
   written = 0;

   /* check if event fits into buffer */
   size_left = TAPE_BUFFER_SIZE - ((POINTER_T) info->write_pointer - (POINTER_T) info->buffer);

   if (size_left < evt_size) {
      /* copy first part of event */
      memcpy(info->write_pointer, pevent, size_left);
      info->write_pointer += size_left;

      /* flush buffer */
      written += midas_flush_buffer(log_chn);
      if (written < 0)
         return SS_FILE_ERROR;

      /* several writes for large events */
      while (evt_size - size_left >= TAPE_BUFFER_SIZE) {
         memcpy(info->buffer, (char *) pevent + size_left, TAPE_BUFFER_SIZE);
         info->write_pointer += TAPE_BUFFER_SIZE;
         size_left += TAPE_BUFFER_SIZE;

         i = midas_flush_buffer(log_chn);
         if (i < 0)
            return SS_FILE_ERROR;

         written += i;
      }

      /* copy remaining part of event */
      memcpy(info->buffer, (char *) pevent + size_left, evt_size - size_left);
      info->write_pointer = info->buffer + (evt_size - size_left);
   } else {
      /* copy event to buffer */
      memcpy(info->write_pointer, pevent, evt_size);
      info->write_pointer += evt_size;
   }

   /* update statistics */
   log_chn->statistics.events_written++;
   log_chn->statistics.bytes_written_uncompressed += evt_size;
   log_chn->statistics.bytes_written += written;
   log_chn->statistics.bytes_written_total += written;

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT midas_log_open(LOG_CHN * log_chn, INT run_number)
{
   MIDAS_INFO *info;
   INT status;

   /* allocate MIDAS buffer info */
   log_chn->format_info = (void **) malloc(sizeof(MIDAS_INFO));

   info = (MIDAS_INFO *) log_chn->format_info;
   if (info == NULL) {
      log_chn->handle = 0;
      return SS_NO_MEMORY;
   }

   /* allocate full ring buffer for that channel */
   if ((info->buffer = (char *) malloc(TAPE_BUFFER_SIZE)) == NULL) {
      free(info);
      log_chn->handle = 0;
      return SS_NO_MEMORY;
   }

   info->write_pointer = info->buffer;

   /* Create device channel */
   if (log_chn->type == LOG_TYPE_TAPE) {
      status = tape_open(log_chn->path, &log_chn->handle);
      if (status != SS_SUCCESS) {
         free(info->buffer);
         free(info);
         log_chn->handle = 0;
         return status;
      }
   } else if (log_chn->type == LOG_TYPE_FTP) {
      status = ftp_open(log_chn->path, &log_chn->ftp_con);
      if (status != SS_SUCCESS) {
         free(info->buffer);
         free(info);
         log_chn->handle = 0;
         return status;
      } else
         log_chn->handle = 1;
   } else {
      /* check if file exists */
      if (strstr(log_chn->path, "null") == NULL) {
         log_chn->handle = open(log_chn->path, O_RDONLY);
         if (log_chn->handle > 0) {
            /* check if file length is nonzero */
            if (lseek(log_chn->handle, 0, SEEK_END) > 0) {
               close(log_chn->handle);
               free(info->buffer);
               free(info);
               log_chn->handle = 0;
               return SS_FILE_EXISTS;
            }
         }
      }

      log_chn->gzfile = NULL;
      log_chn->handle = 0;
         
      /* check that compression level and file name match each other */
      if (1) {
         char *sufp = strstr(log_chn->path, ".gz");
         int isgz = sufp && sufp[3]==0;
         
         if (log_chn->compression>0 && !isgz) {
            cm_msg(MERROR, "midas_log_open",
                   "Compression enabled but output file name does not end with '.gz'");
            free(info->buffer);
            free(info);
            return SS_FILE_ERROR;
         }

         if (log_chn->compression==0 && isgz) {
            cm_msg(MERROR, "midas_log_open",
                   "Output file name ends with '.gz', but compression level is zero");
            free(info->buffer);
            free(info);
            return SS_FILE_ERROR;
         }
      }

#ifdef OS_WINNT
      log_chn->handle = (int) CreateFile(log_chn->path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                         CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN, 0);
#else
      log_chn->handle = open(log_chn->path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | O_LARGEFILE, 0644);
#endif
      if (log_chn->handle < 0) {
         free(info->buffer);
         free(info);
         log_chn->handle = 0;
         return SS_FILE_ERROR;
      }

      if (log_chn->compression > 0) {
#ifdef HAVE_ZLIB
         log_chn->gzfile = gzdopen(log_chn->handle, "wb");
         if (log_chn->gzfile == NULL) {
            cm_msg(MERROR, "midas_log_open",
                   "Error: gzdopen() failed, cannot open compression stream");
            free(info->buffer);
            free(info);
            log_chn->handle = 0;
            return SS_FILE_ERROR;
         }

         gzsetparams(log_chn->gzfile, log_chn->compression, Z_DEFAULT_STRATEGY);
#else
         cm_msg(MERROR, "midas_log_open", "Compression enabled but ZLIB support not compiled in");
         close(log_chn->handle);
         free(info->buffer);
         free(info);
         log_chn->handle = 0;
         return SS_FILE_ERROR;
#endif
      }
   }

   /* write ODB dump */
   if (log_chn->settings.odb_dump)
      log_odb_dump(log_chn, EVENTID_BOR, run_number);

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT midas_log_close(LOG_CHN * log_chn, INT run_number)
{
   int written;

   /* write ODB dump */
   if (log_chn->settings.odb_dump)
      log_odb_dump(log_chn, EVENTID_EOR, run_number);

   written = midas_flush_buffer(log_chn);

   /* update statistics */
   log_chn->statistics.bytes_written += written;
   log_chn->statistics.bytes_written_total += written;

   /* Write EOF if Tape */
   if (log_chn->type == LOG_TYPE_TAPE) {
      /* writing EOF mark on tape Fonly */
      ss_tape_write_eof(log_chn->handle);
      ss_tape_close(log_chn->handle);
   } else if (log_chn->type == LOG_TYPE_FTP) {
      ftp_close(log_chn->ftp_con);
      ftp_bye(log_chn->ftp_con);
   } else {
      if (log_chn->gzfile) {
#ifdef HAVE_ZLIB
         z_streamp s;

         s = (z_streamp) log_chn->gzfile;
         written = s->total_out;
         gzflush(log_chn->gzfile, Z_FULL_FLUSH);
         written = s->total_out - written;
         gzclose(log_chn->gzfile);
         log_chn->statistics.bytes_written += written;
         log_chn->statistics.bytes_written_total += written;
         log_chn->gzfile = NULL;
#else
         assert(!"this cannot happen! support for ZLIB not compiled in");
#endif
      }
#ifdef OS_WINNT
      CloseHandle((HANDLE) log_chn->handle);
#else
      close(log_chn->handle);
#endif
      log_chn->handle = 0;
   }

   free(((MIDAS_INFO *) log_chn->format_info)->buffer);
   free(log_chn->format_info);

   return SS_SUCCESS;
}

/*-- db_get_event_definition ---------------------------------------*/

typedef struct {
   short int event_id;
   WORD format;
   HNDLE hDefKey;
} EVENT_DEF;

EVENT_DEF *db_get_event_definition(short int event_id)
{
   INT i, index, status, size;
   char str[80];
   HNDLE hKey, hKeyRoot;
   WORD id;

#define EVENT_DEF_CACHE_SIZE 30
   static EVENT_DEF *event_def = NULL;

   /* allocate memory for cache */
   if (event_def == NULL)
      event_def = (EVENT_DEF *) calloc(EVENT_DEF_CACHE_SIZE, sizeof(EVENT_DEF));

   /* lookup if event definition in cache */
   for (i = 0; event_def[i].event_id; i++)
      if (event_def[i].event_id == event_id)
         return &event_def[i];

   /* search free cache entry */
   for (index = 0; index < EVENT_DEF_CACHE_SIZE; index++)
      if (event_def[index].event_id == 0)
         break;

   if (index == EVENT_DEF_CACHE_SIZE) {
      cm_msg(MERROR, "db_get_event_definition", "too many event definitions");
      return NULL;
   }

   /* check for system events */
   if (event_id < 0) {
      event_def[index].event_id = event_id;
      event_def[index].format = FORMAT_ASCII;
      event_def[index].hDefKey = 0;
      return &event_def[index];
   }

   status = db_find_key(hDB, 0, "/equipment", &hKeyRoot);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_get_event_definition", "cannot find /equipment entry in ODB");
      return NULL;
   }

   for (i = 0;; i++) {
      /* search for client with specific name */
      status = db_enum_key(hDB, hKeyRoot, i, &hKey);
      if (status == DB_NO_MORE_SUBKEYS) {
         sprintf(str, "Cannot find event id %d under /equipment", event_id);
         cm_msg(MERROR, "db_get_event_definition", str);
         return NULL;
      }

      size = sizeof(id);
      status = db_get_value(hDB, hKey, "Common/Event ID", &id, &size, TID_WORD, TRUE);
      if (status != DB_SUCCESS)
         continue;

      if (id == event_id) {
         /* set cache entry */
         event_def[index].event_id = id;

         size = sizeof(str);
         str[0] = 0;
         db_get_value(hDB, hKey, "Common/Format", str, &size, TID_STRING, TRUE);

         if (equal_ustring(str, "Fixed"))
            event_def[index].format = FORMAT_FIXED;
         else if (equal_ustring(str, "ASCII"))
            event_def[index].format = FORMAT_ASCII;
         else if (equal_ustring(str, "MIDAS"))
            event_def[index].format = FORMAT_MIDAS;
         else if (equal_ustring(str, "YBOS"))
            event_def[index].format = FORMAT_YBOS;
         else if (equal_ustring(str, "DUMP"))
            event_def[index].format = FORMAT_DUMP;
         else {
            cm_msg(MERROR, "db_get_event_definition", "unknown data format");
            event_def[index].event_id = 0;
            return NULL;
         }

         db_find_key(hDB, hKey, "Variables", &event_def[index].hDefKey);
         return &event_def[index];
      }
   }
}

/*---- DUMP format routines ----------------------------------------*/

#define STR_INC(p,base) { p+=strlen(p); \
                          if (p > base+sizeof(base)) \
                            cm_msg(MERROR, "STR_INC", "ASCII buffer too small"); }


INT dump_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size)
{
   INT status, size, i, j;
   EVENT_DEF *event_def;
   BANK_HEADER *pbh;
   BANK *pbk;
   BANK32 *pbk32;
   void *pdata;
   char buffer[100000], *pbuf, name[5], type_name[10];
   HNDLE hKey;
   KEY key;
   DWORD bkname;
   WORD bktype;

   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL)
      return SS_SUCCESS;

   /* write event header */
   pbuf = buffer;
   name[4] = 0;

   if (pevent->event_id == EVENTID_BOR)
      sprintf(pbuf, "%%ID BOR NR %d\n", pevent->serial_number);
   else if (pevent->event_id == EVENTID_EOR)
      sprintf(pbuf, "%%ID EOR NR %d\n", pevent->serial_number);
   else
      sprintf(pbuf, "%%ID %d TR %d NR %d\n", pevent->event_id, pevent->trigger_mask, pevent->serial_number);
   STR_INC(pbuf, buffer);

  /*---- MIDAS format ----------------------------------------------*/
   if (event_def->format == FORMAT_MIDAS) {
      LRS1882_DATA *lrs1882;
      LRS1877_DATA *lrs1877;
      LRS1877_HEADER *lrs1877_header;

      pbh = (BANK_HEADER *) (pevent + 1);
      bk_swap(pbh, FALSE);

      pbk = NULL;
      pbk32 = NULL;
      do {
         /* scan all banks */
         if (bk_is32(pbh)) {
            size = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            size = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         if (rpc_tid_size(bktype & 0xFF))
            size /= rpc_tid_size(bktype & 0xFF);

         lrs1882 = (LRS1882_DATA *) pdata;
         lrs1877 = (LRS1877_DATA *) pdata;
         lrs1877_header = (LRS1877_HEADER *) pdata;

         /* write bank header */
         *((DWORD *) name) = bkname;

         if ((bktype & 0xFF00) == 0)
            strcpy(type_name, rpc_tid_name(bktype & 0xFF));
         else if ((bktype & 0xFF00) == TID_LRS1882)
            strcpy(type_name, "LRS1882");
         else if ((bktype & 0xFF00) == TID_LRS1877)
            strcpy(type_name, "LRS1877");
         else if ((bktype & 0xFF00) == TID_PCOS3)
            strcpy(type_name, "PCOS3");
         else
            strcpy(type_name, "unknown");

         sprintf(pbuf, "BK %s TP %s SZ %d\n", name, type_name, size);
         STR_INC(pbuf, buffer);

         /* write data */
         for (i = 0; i < size; i++) {
            if ((bktype & 0xFF00) == 0)
               db_sprintf(pbuf, pdata, size, i, bktype & 0xFF);

            else if ((bktype & 0xFF00) == TID_LRS1882)
               sprintf(pbuf, "GA %d CH %02d DA %d", lrs1882[i].geo_addr, lrs1882[i].channel, lrs1882[i].data);

            else if ((bktype & 0xFF00) == TID_LRS1877) {
               if (i == 0)      /* header */
                  sprintf(pbuf, "GA %d BF %d CN %d",
                          lrs1877_header[i].geo_addr, lrs1877_header[i].buffer, lrs1877_header[i].count);
               else             /* data */
                  sprintf(pbuf, "GA %d CH %02d ED %d DA %1.1lf",
                          lrs1877[i].geo_addr, lrs1877[i].channel, lrs1877[i].edge, lrs1877[i].data * 0.5);
            }

            else if ((bktype & 0xFF00) == TID_PCOS3)
               *pbuf = '\0';
            else
               db_sprintf(pbuf, pdata, size, i, bktype & 0xFF);

            strcat(pbuf, "\n");
            STR_INC(pbuf, buffer);
         }

      } while (1);
   }

  /*---- FIXED format ----------------------------------------------*/
   if (event_def->format == FORMAT_FIXED) {
      if (event_def->hDefKey == 0)
         cm_msg(MERROR, "dump_write", "cannot find event definition");
      else {
         pdata = (char *) (pevent + 1);
         for (i = 0;; i++) {
            status = db_enum_key(hDB, event_def->hDefKey, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hKey, &key);
            sprintf(pbuf, "%s\n", key.name);
            STR_INC(pbuf, buffer);

            /* adjust for alignment */
            pdata = (void *) VALIGN(pdata, MIN(ss_get_struct_align(), key.item_size));

            for (j = 0; j < key.num_values; j++) {
               db_sprintf(pbuf, pdata, key.item_size, j, key.type);
               strcat(pbuf, "\n");
               STR_INC(pbuf, buffer);
            }

            /* shift data pointer to next item */
            pdata = ((char *) pdata) + key.item_size * key.num_values;
         }
      }
   }

  /*---- ASCII format ----------------------------------------------*/
   if (event_def->format == FORMAT_ASCII) {
      /* write event header to device */
      size = strlen(buffer);
      if (log_chn->type == LOG_TYPE_TAPE)
         status = ss_tape_write(log_chn->handle, buffer, size);
      else
         status = write(log_chn->handle, buffer, size) == size ? SS_SUCCESS : SS_FILE_ERROR;

      /* write event directly to device */
      size = strlen((char *) (pevent + 1));
      if (log_chn->type == LOG_TYPE_TAPE)
         status = ss_tape_write(log_chn->handle, (char *) (pevent + 1), size);
      else if (log_chn->type == LOG_TYPE_FTP)
         status = ftp_send((log_chn->ftp_con)->data, buffer, size) == size ?
             SS_SUCCESS : SS_FILE_ERROR;
      else
         status = write(log_chn->handle, (char *) (pevent + 1), size) == size ? SS_SUCCESS : SS_FILE_ERROR;
   } else {
      /* non-ascii format: only write buffer */

      /* insert empty line after each event */
      strcat(pbuf, "\n");
      size = strlen(buffer);

      /* write record to device */
      if (log_chn->type == LOG_TYPE_TAPE)
         status = ss_tape_write(log_chn->handle, buffer, size);
      else if (log_chn->type == LOG_TYPE_FTP)
         status = ftp_send((log_chn->ftp_con)->data, buffer, size) == size ?
             SS_SUCCESS : SS_FILE_ERROR;
      else
         status = write(log_chn->handle, buffer, size) == size ? SS_SUCCESS : SS_FILE_ERROR;
   }

   /* update statistics */
   log_chn->statistics.events_written++;
   log_chn->statistics.bytes_written += size;
   log_chn->statistics.bytes_written_total += size;

   return status;
}

/*------------------------------------------------------------------*/

INT dump_log_open(LOG_CHN * log_chn, INT run_number)
{
   INT status;

   /* Create device channel */
   if (log_chn->type == LOG_TYPE_TAPE) {
      status = tape_open(log_chn->path, &log_chn->handle);
      if (status != SS_SUCCESS) {
         log_chn->handle = 0;
         return status;
      }
   } else if (log_chn->type == LOG_TYPE_FTP) {
      status = ftp_open(log_chn->path, &log_chn->ftp_con);
      if (status != SS_SUCCESS) {
         log_chn->handle = 0;
         return status;
      } else
         log_chn->handle = 1;
   } else {
      log_chn->handle = open(log_chn->path, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0644);

      if (log_chn->handle < 0) {
         log_chn->handle = 0;
         return SS_FILE_ERROR;
      }
   }

   /* write ODB dump */
   if (log_chn->settings.odb_dump)
      log_odb_dump(log_chn, EVENTID_BOR, run_number);

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT dump_log_close(LOG_CHN * log_chn, INT run_number)
{
   /* write ODB dump */
   if (log_chn->settings.odb_dump)
      log_odb_dump(log_chn, EVENTID_EOR, run_number);

   /* Write EOF if Tape */
   if (log_chn->type == LOG_TYPE_TAPE) {
      /* writing EOF mark on tape only */
      ss_tape_write_eof(log_chn->handle);
      ss_tape_close(log_chn->handle);
   } else if (log_chn->type == LOG_TYPE_FTP) {
      ftp_close(log_chn->ftp_con);
      ftp_bye(log_chn->ftp_con);
   } else
      close(log_chn->handle);

   return SS_SUCCESS;
}

/*---- ASCII format routines ---------------------------------------*/

INT ascii_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size)
{
   INT status, size, i, j;
   EVENT_DEF *event_def;
   BANK_HEADER *pbh;
   BANK *pbk;
   BANK32 *pbk32;
   void *pdata;
   char buffer[10000], name[5], type_name[10];
   char *ph, header_line[10000];
   char *pd, data_line[10000];
   HNDLE hKey;
   KEY key;
   static short int last_event_id = -1;
   DWORD bkname;
   WORD bktype;

   if (pevent->serial_number == 0)
      last_event_id = -1;

   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL)
      return SS_SUCCESS;

   name[4] = 0;
   header_line[0] = 0;
   data_line[0] = 0;
   ph = header_line;
   pd = data_line;

  /*---- MIDAS format ----------------------------------------------*/
   if (event_def->format == FORMAT_MIDAS) {
      LRS1882_DATA *lrs1882;
      LRS1877_DATA *lrs1877;
      LRS1877_HEADER *lrs1877_header;

      pbh = (BANK_HEADER *) (pevent + 1);
      bk_swap(pbh, FALSE);

      pbk = NULL;
      pbk32 = NULL;
      do {
         /* scan all banks */
         if (bk_is32(pbh)) {
            size = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            size = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         if (rpc_tid_size(bktype & 0xFF))
            size /= rpc_tid_size(bktype & 0xFF);

         lrs1882 = (LRS1882_DATA *) pdata;
         lrs1877 = (LRS1877_DATA *) pdata;
         lrs1877_header = (LRS1877_HEADER *) pdata;

         /* write bank header */
         *((DWORD *) name) = bkname;

         if ((bktype & 0xFF00) == 0)
            strcpy(type_name, rpc_tid_name(bktype & 0xFF));
         else if ((bktype & 0xFF00) == TID_LRS1882)
            strcpy(type_name, "LRS1882");
         else if ((bktype & 0xFF00) == TID_LRS1877)
            strcpy(type_name, "LRS1877");
         else if ((bktype & 0xFF00) == TID_PCOS3)
            strcpy(type_name, "PCOS3");
         else
            strcpy(type_name, "unknown");

         sprintf(ph, "%s[%d]\t", name, size);
         STR_INC(ph, header_line);

         /* write data */
         for (i = 0; i < size; i++) {
            db_sprintf(pd, pdata, size, i, bktype & 0xFF);
            strcat(pd, "\t");
            STR_INC(pd, data_line);
         }

      } while (1);
   }

  /*---- FIXED format ----------------------------------------------*/
   if (event_def->format == FORMAT_FIXED) {
      if (event_def->hDefKey == 0)
         cm_msg(MERROR, "ascii_write", "cannot find event definition");
      else {
         pdata = (char *) (pevent + 1);
         for (i = 0;; i++) {
            status = db_enum_key(hDB, event_def->hDefKey, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hKey, &key);

            /* adjust for alignment */
            pdata = (void *) VALIGN(pdata, MIN(ss_get_struct_align(), key.item_size));

            for (j = 0; j < key.num_values; j++) {
               if (pevent->event_id != last_event_id) {
                  if (key.num_values == 1)
                     sprintf(ph, "%s\t", key.name);
                  else
                     sprintf(ph, "%s%d\t", key.name, j);

                  STR_INC(ph, header_line);
               }

               db_sprintf(pd, pdata, key.item_size, j, key.type);
               strcat(pd, "\t");
               STR_INC(pd, data_line);
            }

            /* shift data pointer to next item */
            pdata = ((char *) pdata) + key.item_size * key.num_values;
         }
      }
   }

   if (*(pd - 1) == '\t')
      *(pd - 1) = '\n';

   if (last_event_id != pevent->event_id) {
      if (*(ph - 1) == '\t')
         *(ph - 1) = '\n';
      last_event_id = pevent->event_id;
      strcpy(buffer, header_line);
      strcat(buffer, data_line);
   } else
      strcpy(buffer, data_line);

   /* write buffer to device */
   size = strlen(buffer);

   if (log_chn->type == LOG_TYPE_TAPE)
      status = ss_tape_write(log_chn->handle, buffer, size);
   else if (log_chn->type == LOG_TYPE_FTP)
      status = ftp_send(log_chn->ftp_con->data, buffer, size) == size ?
          SS_SUCCESS : SS_FILE_ERROR;
   else
      status = write(log_chn->handle, buffer, size) == size ? SS_SUCCESS : SS_FILE_ERROR;

   /* update statistics */
   log_chn->statistics.events_written++;
   log_chn->statistics.bytes_written += size;
   log_chn->statistics.bytes_written_total += size;

   return status;
}

/*------------------------------------------------------------------*/

INT ascii_log_open(LOG_CHN * log_chn, INT run_number)
{
   INT status;
   EVENT_HEADER event;

   /* Create device channel */
   if (log_chn->type == LOG_TYPE_TAPE) {
      status = tape_open(log_chn->path, &log_chn->handle);
      if (status != SS_SUCCESS) {
         log_chn->handle = 0;
         return status;
      }
   } else if (log_chn->type == LOG_TYPE_FTP) {
      status = ftp_open(log_chn->path, &log_chn->ftp_con);
      if (status != SS_SUCCESS) {
         log_chn->handle = 0;
         return status;
      } else
         log_chn->handle = 1;
   } else {
      log_chn->handle = open(log_chn->path, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0644);

      if (log_chn->handle < 0) {
         log_chn->handle = 0;
         return SS_FILE_ERROR;
      }
   }

   /* write ODB dump */
   if (log_chn->settings.odb_dump) {
      event.event_id = EVENTID_BOR;
      event.data_size = 0;
      event.serial_number = run_number;

      ascii_write(log_chn, &event, sizeof(EVENT_HEADER));
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT ascii_log_close(LOG_CHN * log_chn, INT run_number)
{
   /* Write EOF if Tape */
   if (log_chn->type == LOG_TYPE_TAPE) {
      /* writing EOF mark on tape only */
      ss_tape_write_eof(log_chn->handle);
      ss_tape_close(log_chn->handle);
   } else if (log_chn->type == LOG_TYPE_FTP) {
      ftp_close(log_chn->ftp_con);
      ftp_bye(log_chn->ftp_con);
   } else
      close(log_chn->handle);

   return SS_SUCCESS;
}

/*---- ROOT format routines ----------------------------------------*/

#ifdef HAVE_ROOT

#define MAX_BANKS 100

typedef struct {
   int event_id;
   TTree *tree;
   int n_branch;
   DWORD branch_name[MAX_BANKS];
   int branch_filled[MAX_BANKS];
   int branch_len[MAX_BANKS];
   TBranch *branch[MAX_BANKS];
} EVENT_TREE;

typedef struct {
   TFile *f;
   int n_tree;
   EVENT_TREE *event_tree;
} TREE_STRUCT;

/*------------------------------------------------------------------*/

INT root_book_trees(TREE_STRUCT * tree_struct)
{
   int index, size, status;
   WORD id;
   char str[1000];
   HNDLE hKeyRoot, hKeyEq;
   KEY eqkey;
   EVENT_TREE *et;

   status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "root_book_trees", "cannot find \"/Equipment\" entry in ODB");
      return 0;
   }

   tree_struct->n_tree = 0;

   for (index = 0;; index++) {
      /* loop through all events under /Equipment */
      status = db_enum_key(hDB, hKeyRoot, index, &hKeyEq);
      if (status == DB_NO_MORE_SUBKEYS)
         return 1;

      db_get_key(hDB, hKeyEq, &eqkey);

      /* create tree */
      tree_struct->n_tree++;
      if (tree_struct->n_tree == 1)
         tree_struct->event_tree = (EVENT_TREE *) malloc(sizeof(EVENT_TREE));
      else
         tree_struct->event_tree =
             (EVENT_TREE *) realloc(tree_struct->event_tree, sizeof(EVENT_TREE) * tree_struct->n_tree);

      et = tree_struct->event_tree + (tree_struct->n_tree - 1);

      size = sizeof(id);
      status = db_get_value(hDB, hKeyEq, "Common/Event ID", &id, &size, TID_WORD, TRUE);
      if (status != DB_SUCCESS)
         continue;

      et->event_id = id;
      et->n_branch = 0;

      /* check format */
      size = sizeof(str);
      str[0] = 0;
      db_get_value(hDB, hKeyEq, "Common/Format", str, &size, TID_STRING, TRUE);

      if (!equal_ustring(str, "MIDAS")) {
         cm_msg(MERROR, "root_book_events",
                "ROOT output only for MIDAS events, but %s in %s format", eqkey.name, str);
         return 0;
      }

      /* create tree */
      sprintf(str, "Event \"%s\", ID %d", eqkey.name, id);
      et->tree = new TTree(eqkey.name, str);
   }

   return 1;
}

/*------------------------------------------------------------------*/

INT root_book_bank(EVENT_TREE * et, HNDLE hKeyDef, int event_id, char *bank_name)
{
   int i, status;
   char str[1000];
   HNDLE hKeyVar, hKeySubVar;
   KEY varkey, subvarkey;

   /* find definition of bank */
   status = db_find_key(hDB, hKeyDef, bank_name, &hKeyVar);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "root_book_bank", "received unknown bank \"%s\" in event #%d", bank_name, event_id);
      return 0;
   }

   if (et->n_branch + 1 == MAX_BANKS) {
      cm_msg(MERROR, "root_book_bank", "max number of banks (%d) exceeded in event #%d", MAX_BANKS, event_id);
      return 0;
   }

   db_get_key(hDB, hKeyVar, &varkey);

   if (varkey.type != TID_KEY) {
      /* book variable length array size */

      sprintf(str, "n%s/I:%s[n%s]/", varkey.name, varkey.name, varkey.name);

      switch (varkey.type) {
      case TID_BYTE:
      case TID_CHAR:
         strcat(str, "b");
         break;
      case TID_SBYTE:
         strcat(str, "B");
         break;
      case TID_WORD:
         strcat(str, "s");
         break;
      case TID_SHORT:
         strcat(str, "S");
         break;
      case TID_DWORD:
         strcat(str, "i");
         break;
      case TID_INT:
         strcat(str, "I");
         break;
      case TID_BOOL:
         strcat(str, "I");
         break;
      case TID_FLOAT:
         strcat(str, "F");
         break;
      case TID_DOUBLE:
         strcat(str, "D");
         break;
      case TID_STRING:
         strcat(str, "C");
         break;
      }

      et->branch[et->n_branch] = et->tree->Branch(bank_name, NULL, str);
      et->branch_name[et->n_branch] = *(DWORD *) bank_name;
      et->n_branch++;
   } else {
      /* book structured bank */
      str[0] = 0;

      for (i = 0;; i++) {
         /* loop through bank variables */
         status = db_enum_key(hDB, hKeyVar, i, &hKeySubVar);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hKeySubVar, &subvarkey);

         if (i != 0)
            strcat(str, ":");
         strcat(str, subvarkey.name);
         strcat(str, "/");
         switch (subvarkey.type) {
         case TID_BYTE:
         case TID_CHAR:
            strcat(str, "b");
            break;
         case TID_SBYTE:
            strcat(str, "B");
            break;
         case TID_WORD:
            strcat(str, "s");
            break;
         case TID_SHORT:
            strcat(str, "S");
            break;
         case TID_DWORD:
            strcat(str, "i");
            break;
         case TID_INT:
            strcat(str, "I");
            break;
         case TID_BOOL:
            strcat(str, "I");
            break;
         case TID_FLOAT:
            strcat(str, "F");
            break;
         case TID_DOUBLE:
            strcat(str, "D");
            break;
         case TID_STRING:
            strcat(str, "C");
            break;
         }
      }

      et->branch[et->n_branch] = et->tree->Branch(bank_name, NULL, str);
      et->branch_name[et->n_branch] = *(DWORD *) bank_name;
      et->n_branch++;
   }

   return 1;
}

/*------------------------------------------------------------------*/

INT root_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size)
{
   INT size, i;
   char bank_name[32];
   EVENT_DEF *event_def;
   BANK_HEADER *pbh;
   void *pdata;
   static short int last_event_id = -1;
   TREE_STRUCT *ts;
   EVENT_TREE *et;
   BANK *pbk;
   BANK32 *pbk32;
   DWORD bklen;
   DWORD bkname;
   WORD bktype;
   TBranch *branch;

   if (pevent->serial_number == 0)
      last_event_id = -1;

   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL) {
      cm_msg(MERROR, "root_write", "Definition for event #%d not found under /Equipment", pevent->event_id);
      return SS_INVALID_FORMAT;
   }

   ts = (TREE_STRUCT *) log_chn->format_info;

   size = (INT) ts->f->GetBytesWritten();

  /*---- MIDAS format ----------------------------------------------*/

   if (event_def->format == FORMAT_MIDAS) {
      pbh = (BANK_HEADER *) (pevent + 1);
      bk_swap(pbh, FALSE);

      /* find event in tree structure */
      for (i = 0; i < ts->n_tree; i++)
         if (ts->event_tree[i].event_id == pevent->event_id)
            break;

      if (i == ts->n_tree) {
         cm_msg(MERROR, "root_write", "Event #%d not booked by root_book_events()", pevent->event_id);
         return SS_INVALID_FORMAT;
      }

      et = ts->event_tree + i;

      /* first mark all banks non-filled */
      for (i = 0; i < et->n_branch; i++)
         et->branch_filled[i] = FALSE;

      /* go thourgh all banks and set the address */
      pbk = NULL;
      pbk32 = NULL;
      do {
         /* scan all banks */
         if (bk_is32(pbh)) {
            bklen = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            bklen = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         if (rpc_tid_size(bktype & 0xFF))
            bklen /= rpc_tid_size(bktype & 0xFF);

         *((DWORD *) bank_name) = bkname;
         bank_name[4] = 0;

         for (i = 0; i < et->n_branch; i++)
            if (et->branch_name[i] == bkname)
               break;

         if (i == et->n_branch)
            root_book_bank(et, event_def->hDefKey, pevent->event_id, bank_name);

         branch = et->branch[i];
         et->branch_filled[i] = TRUE;
         et->branch_len[i] = bklen;

         if (bktype != TID_STRUCT) {
            TIter next(branch->GetListOfLeaves());
            TLeaf *leaf = (TLeaf *) next();

            /* varibale length array */
            leaf->SetAddress(&et->branch_len[i]);

            leaf = (TLeaf *) next();
            leaf->SetAddress(pdata);
         } else {
            /* structured bank */
            branch->SetAddress(pdata);
         }

      } while (1);

      /* check if all branches have been filled */
      for (i = 0; i < et->n_branch; i++)
         if (!et->branch_filled[i])
            cm_msg(MERROR, "root_write", "Bank %s booked but not received, tree cannot be filled", bank_name);

      /* fill tree */
      et->tree->Fill();
   }

   size = (INT) ts->f->GetBytesWritten() - size;

   /* update statistics */
   log_chn->statistics.events_written++;
   log_chn->statistics.bytes_written += size;
   log_chn->statistics.bytes_written_total += size;

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT root_log_open(LOG_CHN * log_chn, INT run_number)
{
   INT size, level;
   char str[256], name[256];
   EVENT_HEADER event;
   TREE_STRUCT *tree_struct;

   /* Create device channel */
   if (log_chn->type == LOG_TYPE_TAPE || log_chn->type == LOG_TYPE_FTP) {
      cm_msg(MERROR, "root_log_open", "ROOT files can only reside on disk");
      log_chn->handle = 0;
      return -1;
   } else {
      /* check if file exists */
      if (strstr(log_chn->path, "null") == NULL) {
         log_chn->handle = open(log_chn->path, O_RDONLY);
         if (log_chn->handle > 0) {
            /* check if file length is nonzero */
            if (lseek(log_chn->handle, 0, SEEK_END) > 0) {
               close(log_chn->handle);
               log_chn->handle = 0;
               return SS_FILE_EXISTS;
            }
         }
      }

      name[0] = 0;
      size = sizeof(name);
      db_get_value(hDB, 0, "/Experiment/Name", name, &size, TID_STRING, TRUE);

      sprintf(str, "MIDAS exp. %s, run #%d", name, run_number);

      TFile *f = new TFile(log_chn->path, "create", str, 1);
      if (!f->IsOpen()) {
         delete f;
         log_chn->handle = 0;
         return SS_FILE_ERROR;
      }
      log_chn->handle = 1;

      /* set compression level */
      level = 0;
      size = sizeof(level);
      db_get_value(hDB, log_chn->settings_hkey, "Compression", &level, &size, TID_INT, FALSE);
      f->SetCompressionLevel(level);

      /* create root structure with trees and branches */
      tree_struct = (TREE_STRUCT *) malloc(sizeof(TREE_STRUCT));
      tree_struct->f = f;

      /* book event tree */
      root_book_trees(tree_struct);

      /* store file object in format_info */
      log_chn->format_info = (void **) tree_struct;
   }

   /* write ODB dump */
   if (log_chn->settings.odb_dump) {
      event.event_id = EVENTID_BOR;
      event.data_size = 0;
      event.serial_number = run_number;

      //##root_write(log_chn, &event, sizeof(EVENT_HEADER));
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT root_log_close(LOG_CHN * log_chn, INT run_number)
{
   TREE_STRUCT *ts;

   ts = (TREE_STRUCT *) log_chn->format_info;

   /* flush and close file */
   ts->f->Write();
   ts->f->Close();
   delete ts->f;                // deletes also all trees and branches!

   /* delete event tree */
   free(ts->event_tree);
   free(ts);

   log_chn->format_info = NULL;

   return SS_SUCCESS;
}

#endif                          /* HAVE_ROOT */

/*---- log_open ----------------------------------------------------*/

INT log_open(LOG_CHN * log_chn, INT run_number)
{
   INT status;

   if (equal_ustring(log_chn->settings.format, "YBOS")) {
      log_chn->format = FORMAT_YBOS;
      status = ybos_log_open(log_chn, run_number);
   } else if (equal_ustring(log_chn->settings.format, "ASCII")) {
      log_chn->format = FORMAT_ASCII;
      status = ascii_log_open(log_chn, run_number);
   } else if (equal_ustring(log_chn->settings.format, "DUMP")) {
      log_chn->format = FORMAT_DUMP;
      status = dump_log_open(log_chn, run_number);
   } else if (equal_ustring(log_chn->settings.format, "ROOT")) {
#ifdef HAVE_ROOT
      log_chn->format = FORMAT_ROOT;
      status = root_log_open(log_chn, run_number);
#else
      return SS_NO_ROOT;
#endif
   } else if (equal_ustring(log_chn->settings.format, "MIDAS")) {
      log_chn->format = FORMAT_MIDAS;
      status = midas_log_open(log_chn, run_number);
   } else
      return SS_INVALID_FORMAT;

   return status;
}

/*---- log_close ---------------------------------------------------*/

INT log_close(LOG_CHN * log_chn, INT run_number)
{
   if (log_chn->format == FORMAT_YBOS)
      ybos_log_close(log_chn, run_number);

   if (log_chn->format == FORMAT_ASCII)
      ascii_log_close(log_chn, run_number);

   if (log_chn->format == FORMAT_DUMP)
      dump_log_close(log_chn, run_number);

#ifdef HAVE_ROOT
   if (log_chn->format == FORMAT_ROOT)
      root_log_close(log_chn, run_number);
#endif

   if (log_chn->format == FORMAT_MIDAS)
      midas_log_close(log_chn, run_number);

   log_chn->statistics.files_written += 1;
   log_chn->handle = 0;
   log_chn->ftp_con = NULL;

   return SS_SUCCESS;
}

/*---- log_write ---------------------------------------------------*/

INT log_write(LOG_CHN * log_chn, EVENT_HEADER * pevent)
{
   INT status = 0, size, izero;
   DWORD actual_time, start_time, watchdog_timeout;
   BOOL watchdog_flag, flag;
   static BOOL stop_requested = FALSE;
   static DWORD last_checked = 0;
   HNDLE htape, stats_hkey;
   char tape_name[256];
   double dzero;

   start_time = ss_millitime();

   if (log_chn->format == FORMAT_YBOS)
      status = ybos_write(log_chn, pevent, pevent->data_size + sizeof(EVENT_HEADER));

   if (log_chn->format == FORMAT_ASCII)
      status = ascii_write(log_chn, pevent, pevent->data_size + sizeof(EVENT_HEADER));

   if (log_chn->format == FORMAT_DUMP)
      status = dump_write(log_chn, pevent, pevent->data_size + sizeof(EVENT_HEADER));

   if (log_chn->format == FORMAT_MIDAS)
      status = midas_write(log_chn, pevent, pevent->data_size + sizeof(EVENT_HEADER));

#ifdef HAVE_ROOT
   if (log_chn->format == FORMAT_ROOT)
      status = root_write(log_chn, pevent, pevent->data_size + sizeof(EVENT_HEADER));
#endif

   actual_time = ss_millitime();
   if ((int) actual_time - (int) start_time > 3000)
      cm_msg(MINFO, "log_write", "Write operation on %s took %d ms", log_chn->path, actual_time - start_time);

   if (status != SS_SUCCESS && !stop_requested) {
      if (status == SS_IO_ERROR)
         cm_msg(MTALK, "log_write", "Physical IO error on %s, stopping run", log_chn->path);
      else
         cm_msg(MTALK, "log_write", "Error writing to %s, stopping run", log_chn->path);

      stop_requested = TRUE;
      cm_transition(TR_STOP, 0, NULL, 0, ASYNC, DEBUG_TRANS);
      stop_requested = FALSE;

      return status;
   }

   /* check if event limit is reached */
   if (!stop_requested && !in_stop_transition &&
       log_chn->settings.event_limit > 0 &&
       log_chn->statistics.events_written >= log_chn->settings.event_limit) {
      stop_requested = TRUE;

      cm_msg(MTALK, "log_write", "stopping run after having received %d events",
             log_chn->settings.event_limit);

      status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC, DEBUG_TRANS);
      if (status != CM_SUCCESS)
         cm_msg(MERROR, "log_write", "cannot stop run after reaching event limit");
      stop_requested = FALSE;

      /* check if autorestart, main loop will take care of it */
      size = sizeof(BOOL);
      flag = FALSE;
      db_get_value(hDB, 0, "/Logger/Auto restart", &flag, &size, TID_BOOL, TRUE);

      if (flag)
         auto_restart = ss_time() + 20; /* start in 20 sec. */

      return status;
   }

   /* check if byte limit is reached */
   if (!stop_requested && !in_stop_transition &&
       log_chn->settings.byte_limit > 0 &&
       log_chn->statistics.bytes_written >= log_chn->settings.byte_limit) {
      stop_requested = TRUE;

      cm_msg(MTALK, "log_write", "stopping run after having received %1.0lf mega bytes",
             log_chn->statistics.bytes_written / 1E6);

      status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC, DEBUG_TRANS);
      if (status != CM_SUCCESS)
         cm_msg(MERROR, "log_write", "cannot stop run after reaching bytes limit");
      stop_requested = FALSE;

      /* check if autorestart, main loop will take care of it */
      size = sizeof(BOOL);
      flag = FALSE;
      db_get_value(hDB, 0, "/Logger/Auto restart", &flag, &size, TID_BOOL, TRUE);

      if (flag)
         auto_restart = ss_time() + 20; /* start in 20 sec. */

      return status;
   }

   /* check if capacity is reached for tapes */
   if (!stop_requested && !in_stop_transition &&
       log_chn->type == LOG_TYPE_TAPE &&
       log_chn->settings.tape_capacity > 0 &&
       log_chn->statistics.bytes_written_total >= log_chn->settings.tape_capacity) {
      stop_requested = TRUE;
      cm_msg(MTALK, "log_write", "tape capacity reached, stopping run");

      /* remember tape name */
      strcpy(tape_name, log_chn->path);
      stats_hkey = log_chn->stats_hkey;

      status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC, DEBUG_TRANS);
      if (status != CM_SUCCESS)
         cm_msg(MERROR, "log_write", "cannot stop run after reaching tape capacity");
      stop_requested = FALSE;

      /* rewind tape */
      ss_tape_open(tape_name, O_RDONLY, &htape);
      cm_msg(MTALK, "log_write", "rewinding tape %s, please wait", log_chn->path);

      cm_get_watchdog_params(&watchdog_flag, &watchdog_timeout);
      cm_set_watchdog_params(watchdog_flag, 300000);    /* 5 min for tape rewind */
      ss_tape_unmount(htape);
      ss_tape_close(htape);
      cm_set_watchdog_params(watchdog_flag, watchdog_timeout);

      /* zero statistics */
      dzero = izero = 0;
      db_set_value(hDB, stats_hkey, "Bytes written total", &dzero, sizeof(dzero), 1, TID_DOUBLE);
      db_set_value(hDB, stats_hkey, "Files written", &izero, sizeof(izero), 1, TID_INT);

      cm_msg(MTALK, "log_write", "Please insert new tape and start new run.");

      return status;
   }

   /* stop run if less than 10MB free disk space */
   actual_time = ss_millitime();
   if (log_chn->type == LOG_TYPE_DISK && actual_time - last_checked > 1000) {
      last_checked = actual_time;

      if (ss_disk_free(log_chn->path) < 1E7 && !stop_requested && !in_stop_transition) {
         stop_requested = TRUE;
         cm_msg(MTALK, "log_write", "disk nearly full, stopping run");

         status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC, DEBUG_TRANS);
         if (status != CM_SUCCESS)
            cm_msg(MERROR, "log_write", "cannot stop run after reaching byte limit");
         stop_requested = FALSE;
      }
   }

   return status;
}

/*---- open_history ------------------------------------------------*/

void log_history(HNDLE hDB, HNDLE hKey, void *info);

static int add_event(int* indexp, int event_id, const char* event_name, HNDLE hKey, int ntags, const TAG* tags, int period, int hotlink)
{
   int status;
   int size, i;
   int index = *indexp;

   /* check for duplicate event id's */
   for (i=0; i<index; i++)
      if (hist_log[i].event_id == event_id) {
         cm_msg(MERROR, "add_event", "Duplicate event id %d for \'%s\'", event_id, event_name);
         return 0;
      }

   if (index >= MAX_HISTORY) {
      cm_msg(MERROR, "add_event", "Too many history events: %d, please increase MAX_HISTORY!", index);
      return 0;
   }

   /* check for invalid history tags */
   for (i=0; i<ntags; i++) {
      if (rpc_tid_size(tags[i].type) == 0) {
         cm_msg(MERROR, "add_event", "Invalid tag %d \'%s\' in event %d \'%s\': type size is zero", i, tags[i].name, event_id, event_name);
         return 0;
      }
   }

   status = hs_define_event(event_id, (char*)event_name, (TAG*)tags, sizeof(TAG) * ntags);
   assert(status == DB_SUCCESS);

   status = db_get_record_size(hDB, hKey, 0, &size);
   assert(status == DB_SUCCESS);

   /* setup hist_log structure for this event */
   hist_log[index].event_id    = event_id;
   hist_log[index].n_var       = ntags;
   hist_log[index].hKeyVar     = hKey;
   hist_log[index].buffer_size = size;
   hist_log[index].buffer      = malloc(size);
   hist_log[index].period      = period;
   hist_log[index].last_log    = 0;

   if (hist_log[index].buffer == NULL) {
      cm_msg(MERROR, "add_event", "Cannot allocate data buffer for event \"%s\" size %d", event_name, size);
      return 0;
   }
   
   /* open hot link to variables */
   if (hotlink) {
      status = db_open_record(hDB, hKey, hist_log[index].buffer,
                              size, MODE_READ, log_history, NULL);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "add_event",
                "Cannot hotlink event %d \"%s\" for history logging, db_open_record() status %d",
                event_id, event_name, status);
         return status;
      }
   }
   
   if (verbose)
      printf("Created event %d for equipment \"%s\", %d tags, size %d\n", event_id, event_name, ntags, size);

   /* create history tags for mhttpd */

   if (1) {
      char buf[256];

      sprintf(buf, "/History/Tags/%d", event_id);

      //printf("Set tag \'%s\' = \'%s\'\n", buf, event_name);

      status = db_set_value(hDB, 0, buf, (void*)event_name, strlen(event_name)+1, 1, TID_STRING);
      assert(status == DB_SUCCESS);

      for (i=0; i<ntags; i++) {
         WORD v = (WORD) tags[i].n_data;
         sprintf(buf, "/History/Tags/Tags %d/%s", event_id, tags[i].name);

         //printf("Set tag \'%s\' = %d\n", buf, v);

         status = db_set_value(hDB, 0, buf, &v, sizeof(v), 1, TID_WORD);
         assert(status == DB_SUCCESS);

         if (strlen(tags[i].name) == NAME_LENGTH-1)
            cm_msg(MERROR, "add_event",
                   "Tag name \'%s\' in event %d (%s) may have been truncated to %d characters",
                   tags[i].name, event_id, event_name, NAME_LENGTH-1);
      }
   }

   *indexp = index+1;

   return SUCCESS;
}

static int get_event_id(int eq_id, const char* eq_name, const char* var_name)
{
   HNDLE hDB, hKeyRoot;
   char name[NAME_LENGTH+NAME_LENGTH+2];
   int status, i;
   WORD max_id = 0;

   strlcpy(name, eq_name, sizeof(name));
   strlcat(name, ":", sizeof(name));
   strlcat(name, var_name, sizeof(name));

   //printf("Looking for event id for \'%s\'\n", name);

   cm_get_experiment_database(&hDB, NULL);
   
   status = db_find_key(hDB, 0, "/History/Events", &hKeyRoot);
   if (status == DB_SUCCESS) {
      for (i = 0;; i++) {
         HNDLE hKey;
         KEY key;
         WORD evid;
         int size;
         char tmp[NAME_LENGTH+NAME_LENGTH+2];
         
         status = db_enum_key(hDB, hKeyRoot, i, &hKey);
         if (status != DB_SUCCESS)
           break;
         
         status = db_get_key(hDB, hKey, &key);
         assert(status == DB_SUCCESS);
         
         //printf("key \'%s\'\n", key.name);
         
         evid = (WORD) strtol(key.name, NULL, 0);
         if (evid == 0)
            continue;

         size = sizeof(tmp);
         status = db_get_data(hDB, hKey, tmp, &size, TID_STRING);
         //printf("status %d\n", status);
         assert(status == DB_SUCCESS);

         //printf("got %d \'%s\'\n", evid, tmp);

         if (equal_ustring(name, tmp))
            return evid;

         if (evid/1000 == eq_id)
            max_id = evid;
      }
   }

   //printf("eq_id %d, max_id %d\n", eq_id, max_id);

   if (max_id == 0)
      max_id = eq_id * 1000;

   if (1) {
      char tmp[NAME_LENGTH+NAME_LENGTH+2];
      WORD evid = max_id + 1;

      sprintf(tmp,"/History/Events/%d", evid);

      status = db_set_value(hDB, 0, tmp, name, strlen(name)+1, 1, TID_STRING);
      assert(status == DB_SUCCESS);

      return evid;
   }

   /* not reached */
   return 0;
}

INT open_history()
{
   INT size, index, i_tag, status, i, j, li, max_event_id;
   int ieq;
   INT n_var, n_tags, n_names = 0;
   HNDLE hKeyRoot, hKeyVar, hKeyNames, hLinkKey, hVarKey, hKeyEq, hHistKey, hKey;
   DWORD history;
   TAG *tag;
   KEY key, varkey, linkkey, histkey;
   WORD eq_id;
   char str[256], eq_name[NAME_LENGTH], hist_name[NAME_LENGTH];
   BOOL single_names;
   int count_events = 0;
   int global_per_variable_history = 0;

   /* set directory for history files */
   size = sizeof(str);
   str[0] = 0;
   status = db_get_value(hDB, 0, "/Logger/History Dir", str, &size, TID_STRING, FALSE);
   if (status != DB_SUCCESS)
      db_get_value(hDB, 0, "/Logger/Data Dir", str, &size, TID_STRING, TRUE);

   if (str[0] != 0)
      hs_set_path(str);

   if (db_find_key(hDB, 0, "/History/Links", &hKeyRoot) != DB_SUCCESS ||
       db_find_key(hDB, 0, "/History/Links/System", &hKeyRoot) != DB_SUCCESS) {
      /* create default history keys */
      db_create_key(hDB, 0, "/History/Links", TID_KEY);

      if (db_find_key(hDB, 0, "/Equipment/Trigger/Statistics/Events per sec.", &hKeyEq) == DB_SUCCESS)
         db_create_link(hDB, 0, "/History/Links/System/Trigger per sec.",
                        "/Equipment/Trigger/Statistics/Events per sec.");

      if (db_find_key(hDB, 0, "/Equipment/Trigger/Statistics/kBytes per sec.", &hKeyEq) == DB_SUCCESS)
         db_create_link(hDB, 0, "/History/Links/System/Trigger kB per sec.",
                        "/Equipment/Trigger/Statistics/kBytes per sec.");
   }

   /*---- define equipment events as history ------------------------*/

   max_event_id = 0;

   status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "open_history", "Cannot find Equipment entry in database");
      return 0;
   }

   size = sizeof(int);
   status = db_get_value(hDB, 0, "/History/PerVariableHistory", &global_per_variable_history, &size, TID_INT, TRUE);
   assert(status==DB_SUCCESS);

   /* loop over equipment */
   index = 0;
   for (ieq = 0; ; ieq++) {
      status = db_enum_key(hDB, hKeyRoot, ieq, &hKeyEq);
      if (status != DB_SUCCESS)
         break;

      /* check history flag */
      size = sizeof(history);
      db_get_value(hDB, hKeyEq, "Common/Log history", &history, &size, TID_INT, TRUE);

      /* define history tags only if log history flag is on */
      if (history > 0) {
         BOOL per_variable_history = global_per_variable_history;

         /* get equipment name */
         db_get_key(hDB, hKeyEq, &key);
         strcpy(eq_name, key.name);


         status = db_find_key(hDB, hKeyEq, "Variables", &hKeyVar);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "open_history", "Cannot find /Equipment/%s/Variables entry in database", eq_name);
            return 0;
         }

         size = sizeof(eq_id);
         status = db_get_value(hDB, hKeyEq, "Common/Event ID", &eq_id, &size, TID_WORD, TRUE);
         assert(status == DB_SUCCESS);

         size = sizeof(int);
         status = db_get_value(hDB, hKeyEq, "Settings/PerVariableHistory", &per_variable_history, &size, TID_INT, FALSE);
         assert(status == DB_SUCCESS || status == DB_NO_KEY);

         if (verbose)
            printf
                ("\n==================== Equipment \"%s\", ID %d  =======================\n",
                 eq_name, eq_id);

         /* count keys in variables tree */
         for (n_var = 0, n_tags = 0;; n_var++) {
            status = db_enum_key(hDB, hKeyVar, n_var, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;
            db_get_key(hDB, hKey, &key);
            n_tags += key.num_values;
         }

         if (n_var == 0)
            cm_msg(MERROR, "open_history", "defined event %d with no variables in ODB", eq_id);
         
         /* create tag array */
         tag = (TAG *) malloc(sizeof(TAG) * n_tags);

         i_tag = 0;
         for (i=0; ; i++) {
            status = db_enum_key(hDB, hKeyVar, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            /* get variable key */
            db_get_key(hDB, hKey, &varkey);

            /* look for names */
            db_find_key(hDB, hKeyEq, "Settings/Names", &hKeyNames);
            single_names = (hKeyNames > 0);
            if (hKeyNames) {
               if (verbose)
                  printf("Using \"/Equipment/%s/Settings/Names\" for variable \"%s\"\n",
                         eq_name, varkey.name);

               /* define tags from names list */
               db_get_key(hDB, hKeyNames, &key);
               n_names = key.num_values;
            } else {
               sprintf(str, "Settings/Names %s", varkey.name);
               db_find_key(hDB, hKeyEq, str, &hKeyNames);
               if (hKeyNames) {
                  if (verbose)
                     printf
                         ("Using \"/Equipment/%s/Settings/Names %s\" for variable \"%s\"\n",
                          eq_name, varkey.name, varkey.name);

                  /* define tags from names list */
                  db_get_key(hDB, hKeyNames, &key);
                  n_names = key.num_values;
               }
            }

            if (hKeyNames && varkey.num_values != n_names) {
               cm_msg(MERROR, "open_history",
                      "Array size mismatch: \"/Equipment/%s/Settings/%s\" has %d entries while \"/Equipment/%s/Variables/%s\" has %d entries",
                      eq_name, key.name, n_names,
                      eq_name, varkey.name, varkey.num_values);
               free(tag);
               return 0;
            }

            if (hKeyNames) {
               /* loop over array elements */
               for (j = 0; j < varkey.num_values; j++) {
                  /* get name #j */
                  size = NAME_LENGTH;
                  db_get_data_index(hDB, hKeyNames, tag[i_tag].name, &size, j, TID_STRING);

                  if (strlen(tag[i_tag].name) < 1) {
                     char buf[256];
                     sprintf(buf, "%d", j);
                     strlcpy(tag[i_tag].name, varkey.name, NAME_LENGTH);
                     strlcat(tag[i_tag].name, "_", NAME_LENGTH);
                     strlcat(tag[i_tag].name, buf, NAME_LENGTH);
                  }

                  /* append variable key name for single name array */
                  if (single_names) {
                     if (strlen(tag[i_tag].name) + 1 + strlen(varkey.name) >= NAME_LENGTH) {
                        cm_msg(MERROR, "open_history",
                               "Name for history entry \"%s %s\" too long", tag[i_tag].name, varkey.name);
                        free(tag);
                        return 0;
                     }
                     strlcat(tag[i_tag].name, " ", NAME_LENGTH);
                     strlcat(tag[i_tag].name, varkey.name, NAME_LENGTH);
                  }

                  tag[i_tag].type = varkey.type;
                  tag[i_tag].n_data = 1;

                  if (verbose)
                     printf("Defined tag \"%s\", type %d, num_values %d\n",
                            tag[i_tag].name, tag[i_tag].type, tag[i_tag].n_data);

                  i_tag++;
               }
            } else if (varkey.type == TID_KEY) {
               int ii;
               for (ii=0;; ii++) {
                  KEY vvarkey;
                  HNDLE hhKey;

                  status = db_enum_key(hDB, hKey, ii, &hhKey);
                  if (status == DB_NO_MORE_SUBKEYS)
                     break;

                  /* get variable key */
                  db_get_key(hDB, hhKey, &vvarkey);

                  strlcpy(tag[i_tag].name, varkey.name, NAME_LENGTH);
                  strlcat(tag[i_tag].name, "_", NAME_LENGTH);
                  strlcat(tag[i_tag].name, vvarkey.name, NAME_LENGTH);
                  tag[i_tag].type = vvarkey.type;
                  tag[i_tag].n_data = vvarkey.num_values;

                  if (verbose)
                     printf("Defined tag \"%s\", type %d, num_values %d\n", tag[i_tag].name,
                            tag[i_tag].type, tag[i_tag].n_data);

                  i_tag++;
               }
            } else {
               strlcpy(tag[i_tag].name, varkey.name, NAME_LENGTH);
               tag[i_tag].type = varkey.type;
               tag[i_tag].n_data = varkey.num_values;

               if (verbose)
                  printf("Defined tag \"%s\", type %d, num_values %d\n", tag[i_tag].name,
                         tag[i_tag].type, tag[i_tag].n_data);

               i_tag++;
            }

            if (per_variable_history && i_tag>0) {
               WORD event_id;
               char event_name[NAME_LENGTH];

               event_id = get_event_id(eq_id, eq_name, varkey.name);
               assert(event_id > 0);

               strlcpy(event_name, eq_name, NAME_LENGTH);
               strlcat(event_name, "/", NAME_LENGTH);
               strlcat(event_name, varkey.name, NAME_LENGTH);

               status = add_event(&index, event_id, event_name, hKey, i_tag, tag, history, 1);
               if (status != DB_SUCCESS)
                  return status;

               count_events++;

               i_tag = 0;
            } /* if per-variable history */

         } /* loop over variables */

         if (!per_variable_history && i_tag>0) {
            status = add_event(&index, eq_id, eq_name, hKeyVar, i_tag, tag, history, 1);
            if (status != DB_SUCCESS)
               return status;

            count_events++;
         }

         free(tag);

         /* remember maximum event id for later use with system events */
         if (eq_id > max_event_id)
            max_event_id = eq_id;
      }
   } /* loop over equipments */

   if (index == MAX_HISTORY) {
      cm_msg(MERROR, "open_history", "too many equipments for history");
      return 0;
   }

   /*---- define linked trees ---------------------------------------*/

   /* round up event id */
   max_event_id = ((int) ((max_event_id + 1) / 10) + 1) * 10;

   status = db_find_key(hDB, 0, "/History/Links", &hKeyRoot);
   if (status == DB_SUCCESS) {
      for (li = 0;; li++) {
         status = db_enum_link(hDB, hKeyRoot, li, &hHistKey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hHistKey, &histkey);
         strcpy(hist_name, histkey.name);
         db_enum_key(hDB, hKeyRoot, li, &hHistKey);

         db_get_key(hDB, hHistKey, &key);
         if (key.type != TID_KEY) {
            cm_msg(MERROR, "open_history", "Only subkeys allows in /history/links");
            continue;
         }

         if (verbose)
            printf
                ("\n==================== History link \"%s\", ID %d  =======================\n",
                 hist_name, max_event_id);

         /* count subkeys in link */
         for (i = n_var = 0;; i++) {
            status = db_enum_key(hDB, hHistKey, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            if (status == DB_SUCCESS && db_get_key(hDB, hKey, &key) == DB_SUCCESS) {
               if (key.type != TID_KEY)
                  n_var++;
            } else {
               db_enum_link(hDB, hHistKey, i, &hKey);
               db_get_key(hDB, hKey, &key);
               cm_msg(MERROR, "open_history",
                      "History link /History/Links/%s/%s is invalid", hist_name, key.name);
               return 0;
            }
         }

         if (n_var == 0)
            cm_msg(MERROR, "open_history", "History event %s has no variables in ODB", hist_name);
         else {
            /* create tag array */
            tag = (TAG *) malloc(sizeof(TAG) * n_var);

            for (i = 0, size = 0, n_var = 0;; i++) {
               status = db_enum_link(hDB, hHistKey, i, &hLinkKey);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               /* get link key */
               db_get_key(hDB, hLinkKey, &linkkey);

               if (linkkey.type == TID_KEY)
                  continue;

               /* get link target */
               db_enum_key(hDB, hHistKey, i, &hVarKey);
               if (db_get_key(hDB, hVarKey, &varkey) == DB_SUCCESS) {
                  /* hot-link individual values */
                  if (histkey.type == TID_KEY)
                     db_open_record(hDB, hVarKey, NULL, varkey.total_size, MODE_READ,
                                    log_system_history, (void *) (POINTER_T) index);

                  strcpy(tag[n_var].name, linkkey.name);
                  tag[n_var].type = varkey.type;
                  tag[n_var].n_data = varkey.num_values;

                  if (verbose)
                     printf("Defined tag \"%s\", type %d, num_values %d\n",
                            tag[n_var].name, tag[n_var].type, tag[n_var].n_data);

                  size += varkey.total_size;
                  n_var++;
               }
            }

            /* hot-link whole subtree */
            if (histkey.type == TID_LINK)
               db_open_record(hDB, hHistKey, NULL, size, MODE_READ, log_system_history,
                              (void *) (POINTER_T) index);

            status = add_event(&index, max_event_id, hist_name, hHistKey, n_var, tag, 10, 0);
            if (status != DB_SUCCESS)
               return status;

            free(tag);

            count_events++;
            max_event_id++;

            if (index == MAX_HISTORY) {
               cm_msg(MERROR, "open_history", "too many equipments for history");
               return 0;
            }
         }
      }
   }

   /*---- define run start/stop event -------------------------------*/

   tag = (TAG *) malloc(sizeof(TAG) * 2);

   strcpy(tag[0].name, "State");
   tag[0].type = TID_DWORD;
   tag[0].n_data = 1;

   strcpy(tag[1].name, "Run number");
   tag[1].type = TID_DWORD;
   tag[1].n_data = 1;

   hs_define_event(0, "Run transitions", tag, sizeof(TAG) * 2);
   free(tag);

   cm_msg(MINFO, "open_history", "Configured history with %d events", count_events);

   return CM_SUCCESS;
}

/*---- close_history -----------------------------------------------*/

void close_history()
{
   INT i, status;
   HNDLE hKeyRoot, hKey;

   /* close system history */
   status = db_find_key(hDB, 0, "/History/Links", &hKeyRoot);
   if (status != DB_SUCCESS) {
      for (i = 0;; i++) {
         status = db_enum_key(hDB, hKeyRoot, i, &hKey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;
         db_close_record(hDB, hKey);
      }
   }

   /* close event history */
   for (i = 1; i < MAX_HISTORY; i++)
      if (hist_log[i].hKeyVar) {
         db_close_record(hDB, hist_log[i].hKeyVar);
         hist_log[i].hKeyVar = 0;
         if (hist_log[i].buffer)
            free(hist_log[i].buffer);
         hist_log[i].buffer = NULL;
      }
}

/*---- log_history -------------------------------------------------*/

void log_history(HNDLE hDB, HNDLE hKey, void *info)
{
   INT i, size;

   for (i = 0; i < MAX_HISTORY; i++)
      if (hist_log[i].hKeyVar == hKey)
         break;

   if (i == MAX_HISTORY)
      return;

   /* check if over period */
   if (ss_time() - hist_log[i].last_log < hist_log[i].period)
      return;

   /* check if event size has changed */
   db_get_record_size(hDB, hKey, 0, &size);
   if (size != hist_log[i].buffer_size) {
      close_history();
      open_history();
      return;
   }

   if (verbose)
      printf("write event: id %d, buffer %p, size %d\n", hist_log[i].event_id, hist_log[i].buffer, hist_log[i].buffer_size);

   hs_write_event(hist_log[i].event_id, hist_log[i].buffer, hist_log[i].buffer_size);
   hist_log[i].last_log = ss_time();
}

/*------------------------------------------------------------------*/

void log_system_history(HNDLE hDB, HNDLE hKey, void *info)
{
   INT size, total_size, status, index;
   DWORD i;
   KEY key;

   index = (INT) (POINTER_T) info;

   /* check if over period */
   if (ss_time() - hist_log[index].last_log < hist_log[index].period)
      return;

   for (i = 0, total_size = 0;; i++) {
      status = db_enum_key(hDB, hist_log[index].hKeyVar, i, &hKey);
      if (status == DB_NO_MORE_SUBKEYS)
         break;

      db_get_key(hDB, hKey, &key);
      size = key.total_size;
      db_get_data(hDB, hKey, (char *) hist_log[index].buffer + total_size, &size, key.type);
      total_size += size;
   }

   if (i != hist_log[index].n_var) {
      close_history();
      open_history();
   } else
      hs_write_event(hist_log[index].event_id, hist_log[index].buffer, hist_log[index].buffer_size);

   hist_log[index].last_log = ss_time();

   /* simulate odb key update for hot links connected to system history */
   if (!rpc_is_remote()) {
      db_lock_database(hDB);
      db_notify_clients(hDB, hist_log[index].hKeyVar, FALSE);
      db_unlock_database(hDB);
   }

}

/*------------------------------------------------------------------*/

INT log_callback(INT index, void *prpc_param[])
{
   HNDLE hKeyRoot, hKeyChannel;
   INT i, status, size, channel, izero, htape, online_mode;
   DWORD watchdog_timeout;
   BOOL watchdog_flag;
   char str[256];
   double dzero;

   /* rewind tapes */
   if (index == RPC_LOG_REWIND) {
      channel = *((INT *) prpc_param[0]);

      /* loop over all channels */
      status = db_find_key(hDB, 0, "/Logger/Channels", &hKeyRoot);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "log_callback", "cannot find Logger/Channels entry in database");
         return 0;
      }

      /* check online mode */
      online_mode = 0;
      size = sizeof(online_mode);
      db_get_value(hDB, 0, "/Runinfo/online mode", &online_mode, &size, TID_INT, TRUE);

      for (i = 0; i < MAX_CHANNELS; i++) {
         status = db_enum_key(hDB, hKeyRoot, i, &hKeyChannel);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         /* skip if wrong channel, -1 means rewind all channels */
         if (channel != i && channel != -1)
            continue;

         if (status == DB_SUCCESS) {
            size = sizeof(str);
            status = db_get_value(hDB, hKeyChannel, "Settings/Type", str, &size, TID_STRING, TRUE);
            if (status != DB_SUCCESS)
               continue;

            if (equal_ustring(str, "Tape")) {
               size = sizeof(str);
               status = db_get_value(hDB, hKeyChannel, "Settings/Filename", str, &size, TID_STRING, TRUE);
               if (status != DB_SUCCESS)
                  continue;

               if (ss_tape_open(str, O_RDONLY, &htape) == SS_SUCCESS) {
                  cm_msg(MTALK, "log_callback", "rewinding tape #%d, please wait", i);

                  cm_get_watchdog_params(&watchdog_flag, &watchdog_timeout);
                  cm_set_watchdog_params(watchdog_flag, 300000);        /* 5 min for tape rewind */
                  ss_tape_rewind(htape);
                  if (online_mode)
                     ss_tape_unmount(htape);
                  cm_set_watchdog_params(watchdog_flag, watchdog_timeout);

                  cm_msg(MINFO, "log_callback", "Tape %s rewound sucessfully", str);
               } else
                  cm_msg(MERROR, "log_callback", "Cannot rewind tape %s", str);

               ss_tape_close(htape);

               /* clear statistics */
               dzero = izero = 0;
               log_chn[i].statistics.bytes_written_total = 0;
               log_chn[i].statistics.files_written = 0;
               db_set_value(hDB, hKeyChannel, "Statistics/Bytes written total", &dzero,
                            sizeof(dzero), 1, TID_DOUBLE);
               db_set_value(hDB, hKeyChannel, "Statistics/Files written", &izero, sizeof(izero), 1, TID_INT);
            }
         }
      }

      cm_msg(MTALK, "log_callback", "tape rewind finished");
   }

   return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\

                         transition callbacks

\********************************************************************/

static struct {
   DWORD transition;
   DWORD run_number;
} eb;

/*------------------------------------------------------------------*/

INT tr_start(INT run_number, char *error)
/********************************************************************\

  Prestart:

    Loop through channels defined in /logger/channels.
    Neglect channels with are not active.
    If "filename" contains a "%", substitute it by the
    current run number. Open logging channel and
    corresponding buffer. Place a event request
    into the buffer.

\********************************************************************/
{
   INT size, index, status;
   HNDLE hKeyRoot, hKeyChannel;
   char str[256], path[256], dir[256], data_dir[256];
   CHN_SETTINGS *chn_settings;
   KEY key;
   BOOL write_data, tape_flag = FALSE;
   time_t now;
   struct tm *tms;

   /* save current ODB */
   odb_save("last.xml");

   /* read global logging flag */
   size = sizeof(BOOL);
   write_data = TRUE;
   db_get_value(hDB, 0, "/Logger/Write data", &write_data, &size, TID_BOOL, TRUE);

   /* read tape message flag */
   size = sizeof(tape_message);
   db_get_value(hDB, 0, "/Logger/Tape message", &tape_message, &size, TID_BOOL, TRUE);

   /* loop over all channels */
   status = db_find_key(hDB, 0, "/Logger/Channels", &hKeyRoot);
   if (status != DB_SUCCESS) {
      /* if no channels are defined, define at least one */
      status = db_create_record(hDB, 0, "/Logger/Channels/0/", strcomb(chn_settings_str));
      if (status != DB_SUCCESS) {
         strcpy(error, "Cannot create channel entry in database");
         cm_msg(MERROR, "tr_start", error);
         return 0;
      }

      status = db_find_key(hDB, 0, "/Logger/Channels", &hKeyRoot);
      if (status != DB_SUCCESS) {
         strcpy(error, "Cannot create channel entry in database");
         cm_msg(MERROR, "tr_start", error);
         return 0;
      }
   }

   for (index = 0; index < MAX_CHANNELS; index++) {
      status = db_enum_key(hDB, hKeyRoot, index, &hKeyChannel);
      if (status == DB_NO_MORE_SUBKEYS)
         break;

      /* correct channel record */
      db_get_key(hDB, hKeyChannel, &key);
      status = db_check_record(hDB, hKeyRoot, key.name, strcomb(chn_settings_str), TRUE);
      if (status != DB_SUCCESS && status != DB_OPEN_RECORD) {
         cm_msg(MERROR, "tr_start", "Cannot create/check channel record");
         break;
      }

      if (status == DB_SUCCESS || status == DB_OPEN_RECORD) {
         /* if file already open, we had an abort on the previous start. So
            close and delete file in order to create a new one */
         if (log_chn[index].handle) {
            log_close(&log_chn[index], run_number);
            if (log_chn[index].type == LOG_TYPE_DISK) {
               cm_msg(MINFO, "tr_start", "Deleting previous file \"%s\"", log_chn[index].path);
               unlink(log_chn[index].path);
            }
         }

         /* if FTP channel alreay open, don't re-open it again */
         if (log_chn[index].ftp_con)
            continue;

         /* save settings key */
         status = db_find_key(hDB, hKeyChannel, "Settings", &log_chn[index].settings_hkey);
         if (status != DB_SUCCESS) {
            strcpy(error, "Cannot find channel settings info");
            cm_msg(MERROR, "tr_start", error);
            return 0;
         }

         /* save statistics key */
         status = db_find_key(hDB, hKeyChannel, "Statistics", &log_chn[index].stats_hkey);
         if (status != DB_SUCCESS) {
            strcpy(error, "Cannot find channel statistics info");
            cm_msg(MERROR, "tr_start", error);
            return 0;
         }

         /* clear statistics */
         size = sizeof(CHN_STATISTICS);
         db_get_record(hDB, log_chn[index].stats_hkey, &log_chn[index].statistics, &size, 0);

         log_chn[index].statistics.events_written = 0;
         log_chn[index].statistics.bytes_written = 0;
         log_chn[index].statistics.bytes_written_uncompressed = 0;

         db_set_record(hDB, log_chn[index].stats_hkey, &log_chn[index].statistics, size, 0);

         /* get channel info structure */
         chn_settings = &log_chn[index].settings;
         size = sizeof(CHN_SETTINGS);
         status = db_get_record(hDB, log_chn[index].settings_hkey, chn_settings, &size, 0);
         if (status != DB_SUCCESS) {
            strcpy(error, "Cannot read channel info");
            cm_msg(MERROR, "tr_start", error);
            return 0;
         }

         /* don't start run if tape is full */
         if (log_chn[index].type == LOG_TYPE_TAPE &&
             chn_settings->tape_capacity > 0 &&
             log_chn[index].statistics.bytes_written_total >= chn_settings->tape_capacity) {
            strcpy(error, "Tape capacity reached. Please load new tape");
            cm_msg(MERROR, "tr_start", error);
            return 0;
         }

         /* check if active */
         if (!chn_settings->active || !write_data)
            continue;

         /* check for type */
         if (equal_ustring(chn_settings->type, "Tape"))
            log_chn[index].type = LOG_TYPE_TAPE;
         else if (equal_ustring(chn_settings->type, "FTP"))
            log_chn[index].type = LOG_TYPE_FTP;
         else if (equal_ustring(chn_settings->type, "Disk"))
            log_chn[index].type = LOG_TYPE_DISK;
         else {
            sprintf(error,
                    "Invalid channel type \"%s\", pease use \"Tape\", \"FTP\" or \"Disk\"",
                    chn_settings->type);
            cm_msg(MERROR, "tr_start", error);
            return 0;
         }

         /* set compression level */
         log_chn[index].compression = 0;
         size = sizeof(log_chn[index].compression);
         db_get_value(hDB, log_chn->settings_hkey, "Compression", &log_chn[index].compression, &size, TID_INT,
                      FALSE);

         data_dir[0] = 0;

         /* if disk, precede filename with directory if not already there */
         if (log_chn[index].type == LOG_TYPE_DISK && chn_settings->filename[0] != DIR_SEPARATOR) {
            size = sizeof(data_dir);
            dir[0] = 0;
            db_get_value(hDB, 0, "/Logger/Data Dir", data_dir, &size, TID_STRING, TRUE);
            if (data_dir[0] != 0)
               if (data_dir[strlen(data_dir) - 1] != DIR_SEPARATOR)
                  strcat(data_dir, DIR_SEPARATOR_STR);
            strcpy(str, data_dir);

            /* append subdirectory if requested */
            if (chn_settings->subdir_format[0]) {
               tzset();
               time(&now);
               tms = localtime(&now);

               strftime(dir, sizeof(dir), chn_settings->subdir_format, tms);
               strcat(str, dir);
               strcat(str, DIR_SEPARATOR_STR);
            }

            /* create directory if needed */
#ifdef OS_WINNT
            status = mkdir(str);
#else
            status = mkdir(str, 0755);
#endif
#if !defined(HAVE_MYSQL) && !defined(OS_WINNT)  /* errno not working with mySQL lib */
            if (status == -1 && errno != EEXIST)
               cm_msg(MERROR, "tr_start", "Cannot create subdirectory %s", str);
#endif

            strcat(str, chn_settings->filename);
         } else
            strcpy(str, chn_settings->filename);

         /* substitue "%d" by current run number */
         if (strchr(str, '%'))
            sprintf(path, str, run_number);
         else
            strcpy(path, str);

         strcpy(log_chn[index].path, path);

         /* write back current file name to ODB */
         if (strncmp(path, data_dir, strlen(data_dir)) == 0)
            strcpy(str, path + strlen(data_dir));
         else
            strcpy(str, path);
         db_set_value(hDB, hKeyChannel, "Settings/Current filename", str, 256, 1, TID_STRING);

         if (log_chn[index].type == LOG_TYPE_TAPE &&
             log_chn[index].statistics.bytes_written_total == 0 && tape_message) {
            tape_flag = TRUE;
            cm_msg(MTALK, "tr_start", "mounting tape #%d, please wait", index);
         }

         /* open logging channel */
         status = log_open(&log_chn[index], run_number);

         /* return if logging channel couldn't be opened */
         if (status != SS_SUCCESS) {
            if (status == SS_FILE_ERROR)
               sprintf(error, "Cannot open file %s (See messages)", path);
            if (status == SS_FILE_EXISTS)
               sprintf(error, "File %s exists already, run start aborted", path);
            if (status == SS_NO_TAPE)
               sprintf(error, "No tape in device %s", path);
            if (status == SS_TAPE_ERROR)
               sprintf(error, "Tape error, cannot start run");
            if (status == SS_DEV_BUSY)
               sprintf(error, "Device %s used by someone else", path);
            if (status == FTP_NET_ERROR || status == FTP_RESPONSE_ERROR)
               sprintf(error, "Cannot open FTP channel to [%s]", path);
            if (status == SS_NO_ROOT)
               sprintf(error, "No ROOT support compiled into mlogger, please compile with -DHAVE_ROOT flag");

            if (status == SS_INVALID_FORMAT)
               sprintf(error,
                       "Invalid data format, please use \"MIDAS\", \"YBOS\", \"ASCII\", \"DUMP\" or \"ROOT\"");

            cm_msg(MERROR, "tr_start", error);
            return 0;
         }

         /* close records if open from previous run start with abort */
         if (log_chn[index].stats_hkey)
            db_close_record(hDB, log_chn[index].stats_hkey);
         if (log_chn[index].settings_hkey)
            db_close_record(hDB, log_chn[index].settings_hkey);

         /* open hot link to statistics tree */
         status =
             db_open_record(hDB, log_chn[index].stats_hkey, &log_chn[index].statistics,
                            sizeof(CHN_STATISTICS), MODE_WRITE, NULL, NULL);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "tr_start", "cannot open statistics record, probably other logger is using it");

         /* open hot link to settings tree */
         status =
             db_open_record(hDB, log_chn[index].settings_hkey, &log_chn[index].settings,
                            sizeof(CHN_SETTINGS), MODE_READ, NULL, NULL);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "tr_start",
                   "cannot open channel settings record, probably other logger is using it");

#ifndef FAL_MAIN
         /* open buffer */
         status = bm_open_buffer(chn_settings->buffer, 2 * MAX_EVENT_SIZE, &log_chn[index].buffer_handle);
         if (status != BM_SUCCESS && status != BM_CREATED) {
            sprintf(error, "Cannot open buffer %s", chn_settings->buffer);
            cm_msg(MERROR, "tr_start", error);
            return 0;
         }
         bm_set_cache_size(log_chn[index].buffer_handle, 100000, 0);

         /* place event request */
         status = bm_request_event(log_chn[index].buffer_handle,
                                   (short) chn_settings->event_id,
                                   (short) chn_settings->trigger_mask,
                                   GET_ALL, &log_chn[index].request_id, receive_event);

         if (status != BM_SUCCESS) {
            sprintf(error, "Cannot place event request");
            cm_msg(MERROR, "tr_start", error);
            return 0;
         }

         /* open message buffer if requested */
         if (chn_settings->log_messages) {
            status =
                bm_open_buffer(MESSAGE_BUFFER_NAME, MESSAGE_BUFFER_SIZE, &log_chn[index].msg_buffer_handle);
            if (status != BM_SUCCESS && status != BM_CREATED) {
               sprintf(error, "Cannot open buffer %s", MESSAGE_BUFFER_NAME);
               cm_msg(MERROR, "tr_start", error);
               return 0;
            }

            /* place event request */
            status = bm_request_event(log_chn[index].msg_buffer_handle,
                                      (short) EVENTID_MESSAGE,
                                      (short) chn_settings->log_messages,
                                      GET_ALL, &log_chn[index].msg_request_id, receive_event);

            if (status != BM_SUCCESS) {
               sprintf(error, "Cannot place event request");
               cm_msg(MERROR, "tr_start", error);
               return 0;
            }
         }
#endif
      }
   }

   if (tape_flag && tape_message)
      cm_msg(MTALK, "tr_start", "tape mounting finished");

   /* reopen history channels if event definition has changed */
   close_history();
   status = open_history();
   if (status != CM_SUCCESS) {
      sprintf(error, "Error in history system, aborting run start");
      cm_msg(MERROR, "tr_start", error);
      return 0;
   }

   /* write transition event into history */
   eb.transition = STATE_RUNNING;
   eb.run_number = run_number;
   hs_write_event(0, &eb, sizeof(eb));

#ifdef HAVE_MYSQL
   /* write to SQL database if requested */
   write_sql(TRUE);
#endif

   return CM_SUCCESS;
}

/*-- poststop ------------------------------------------------------*/

INT tr_stop(INT run_number, char *error)
/********************************************************************\

   Poststop:

     Wait until buffers are empty, then close logging channels

\********************************************************************/
{
   INT i, size;
   BOOL flag, tape_flag = FALSE;
   char filename[256];
   char str[256];

   if (in_stop_transition)
      return CM_SUCCESS;

   in_stop_transition = TRUE;
   for (i = 0; i < MAX_CHANNELS; i++) {
      if (log_chn[i].handle || log_chn[i].ftp_con) {
         /* generate MTALK message */
         if (log_chn[i].type == LOG_TYPE_TAPE && tape_message) {
            tape_flag = TRUE;
            cm_msg(MTALK, "tr_stop", "closing tape channel #%d, please wait", i);
         }
#ifndef FAL_MAIN
         /* wait until buffer is empty */
         if (log_chn[i].buffer_handle) {
#ifdef DELAYED_STOP
            DWORD start_time;

            start_time = ss_millitime();
            do {
               cm_yield(100);
            } while (ss_millitime() - start_time < DELAYED_STOP);
#else
            INT n_bytes;
            do {
               bm_get_buffer_level(log_chn[i].buffer_handle, &n_bytes);
               if (n_bytes > 0)
                  cm_yield(100);
            } while (n_bytes > 0);
#endif
         }
#endif                          /* FAL_MAIN */

         /* close logging channel */
         log_close(&log_chn[i], run_number);

         /* close statistics record */
         db_set_record(hDB, log_chn[i].stats_hkey, &log_chn[i].statistics, sizeof(CHN_STATISTICS), 0);
         db_close_record(hDB, log_chn[i].stats_hkey);
         db_close_record(hDB, log_chn[i].settings_hkey);
         log_chn[i].stats_hkey = log_chn[i].settings_hkey = 0;
      }
   }

   /* close buffers */
   for (i = 0; i < MAX_CHANNELS; i++) {
#ifndef FAL_MAIN
      if (log_chn[i].buffer_handle) {
         INT j;

         bm_close_buffer(log_chn[i].buffer_handle);
         for (j = i + 1; j < MAX_CHANNELS; j++)
            if (log_chn[j].buffer_handle == log_chn[i].buffer_handle)
               log_chn[j].buffer_handle = 0;
      }

      if (log_chn[i].msg_request_id)
         bm_delete_request(log_chn[i].msg_request_id);
#endif

      /* clear channel info */
      memset(&log_chn[i].handle, 0, sizeof(LOG_CHN));
   }

   /* ODB dump if requested */
   size = sizeof(flag);
   flag = 0;
   db_get_value(hDB, 0, "/Logger/ODB Dump", &flag, &size, TID_BOOL, TRUE);
   if (flag) {
      strcpy(str, "run%d.odb");
      size = sizeof(str);
      str[0] = 0;
      db_get_value(hDB, 0, "/Logger/ODB Dump File", str, &size, TID_STRING, TRUE);
      if (str[0] == 0)
         strcpy(str, "run%d.odb");

      /* substitue "%d" by current run number */
      if (strchr(str, '%'))
         sprintf(filename, str, run_number);
      else
         strcpy(filename, str);

      odb_save(filename);
   }
#ifdef HAVE_MYSQL
   /* write to SQL database if requested */
   write_sql(FALSE);
#endif

   in_stop_transition = FALSE;

   if (tape_flag & tape_message)
      cm_msg(MTALK, "tr_stop", "all tape channels closed");

   /* write transition event into history */
   eb.transition = STATE_STOPPED;
   eb.run_number = run_number;
   hs_write_event(0, &eb, sizeof(eb));

   return CM_SUCCESS;
}

/*== common code FAL/MLOGGER end ===================================*/

/*----- pause/resume -----------------------------------------------*/

INT tr_pause(INT run_number, char *error)
{
   /* write transition event into history */
   eb.transition = STATE_PAUSED;
   eb.run_number = run_number;
   hs_write_event(0, &eb, sizeof(eb));

   return CM_SUCCESS;
}

INT tr_resume(INT run_number, char *error)
{
   /* write transition event into history */
   eb.transition = STATE_RUNNING;
   eb.run_number = run_number;
   hs_write_event(0, &eb, sizeof(eb));

   return CM_SUCCESS;
}

/*----- receive_event ----------------------------------------------*/

void receive_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER * pheader, void *pevent)
{
   INT i;

   /* find logging channel for this request id */
   for (i = 0; i < MAX_CHANNELS; i++) {
      if (log_chn[i].handle == 0 && log_chn[i].ftp_con == NULL)
         continue;

      /* write normal events */
      if (log_chn[i].request_id == request_id) {
         log_write(&log_chn[i], pheader);
         break;
      }

      /* write messages */
      if (log_chn[i].msg_request_id == request_id) {
         log_write(&log_chn[i], pheader);
         break;
      }
   }
}

/*------------------------ main ------------------------------------*/

int main(int argc, char *argv[])
{
   INT status, msg, i, size, run_number, ch = 0, state;
   char host_name[HOST_NAME_LENGTH], exp_name[NAME_LENGTH], dir[256];
   BOOL debug, daemon, save_mode;
   DWORD last_time_kb = 0;
   DWORD last_time_stat = 0;
   HNDLE hktemp;

#ifdef HAVE_ROOT
   char **rargv;
   int rargc;

   /* copy first argument */
   rargc = 0;
   rargv = (char **) malloc(sizeof(char *) * 2);
   rargv[rargc] = (char *) malloc(strlen(argv[rargc]) + 1);
   strcpy(rargv[rargc], argv[rargc]);
   rargc++;

   /* append argument "-b" for batch mode without graphics */
   rargv[rargc++] = "-b";

   TApplication theApp("mlogger", &rargc, rargv);

   /* free argument memory */
   free(rargv[0]);
   free(rargv);

#endif

   setbuf(stdout, NULL);
   setbuf(stderr, NULL);

   /* get default from environment */
   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

   debug = daemon = save_mode = FALSE;

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
         debug = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'D')
         daemon = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 's')
         save_mode = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'v')
         verbose = TRUE;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'e')
            strcpy(exp_name, argv[++i]);
         else {
          usage:
            printf("usage: mlogger [-e Experiment] [-d] [-D] [-s] [-v]\n\n");
            return 1;
         }
      }
   }

   if (daemon) {
      printf("Becoming a daemon...\n");
      ss_daemon_init(FALSE);
   }

   status = cm_connect_experiment(host_name, exp_name, "Logger", NULL);
   if (status != CM_SUCCESS)
      return 1;

   /* check if logger already running */
   status = cm_exist("Logger", FALSE);
   if (status == CM_SUCCESS) {
      printf("Logger runs already.\n");
      cm_disconnect_experiment();
      return 1;
   }

   cm_get_experiment_database(&hDB, NULL);

   /* set default watchdog timeout */
   cm_set_watchdog_params(TRUE, LOGGER_TIMEOUT);

   /* turn off watchdog if in debug mode */
   if (debug)
      cm_set_watchdog_params(TRUE, 0);

   /* turn on save mode */
   if (save_mode) {
      cm_set_watchdog_params(FALSE, 0);
      db_protect_database(hDB);
   }

   /* register transition callbacks */
   if (cm_register_transition(TR_START, tr_start, 200) != CM_SUCCESS) {
      cm_msg(MERROR, "main", "cannot register callbacks");
      return 1;
   }

   cm_register_transition(TR_STOP, tr_stop, 800);
   cm_register_transition(TR_PAUSE, tr_pause, 800);
   cm_register_transition(TR_RESUME, tr_resume, 200);

   /* register callback for rewinding tapes */
   cm_register_function(RPC_LOG_REWIND, log_callback);

   /* initialize ODB */
   logger_init();

   /* open history logging */
   if (open_history() != CM_SUCCESS) {
      printf("Error in history system, aborting startup.\n");
      cm_disconnect_experiment();
      return 1;
   }

   /* turn off message display, turn on message logging */
   cm_set_msg_print(MT_ALL, 0, NULL);

   /* print startup message */
   size = sizeof(dir);
   db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);
   printf("Log     directory is %s\n", dir);
   printf("Data    directory is same as Log unless specified in channels/\n");

   /* Alternate History and Elog path */
   size = sizeof(dir);
   dir[0] = 0;
   status = db_find_key(hDB, 0, "/Logger/History dir", &hktemp);
   if (status == DB_SUCCESS)
      db_get_value(hDB, 0, "/Logger/History dir", dir, &size, TID_STRING, TRUE);
   else
      sprintf(dir, "same as Log");
   printf("History directory is %s\n", dir);

   size = sizeof(dir);
   dir[0] = 0;
   status = db_find_key(hDB, 0, "/Logger/Elog dir", &hktemp);
   if (status == DB_SUCCESS)
      db_get_value(hDB, 0, "/Logger/Elog dir", dir, &size, TID_STRING, TRUE);
   else
      sprintf(dir, "same as Log");
   printf("ELog    directory is %s\n", dir);

#ifdef HAVE_MYSQL
   {
      char sql_host[256], sql_db[256], sql_table[256];

      status = db_find_key(hDB, 0, "/Logger/SQL/Hostname", &hktemp);
      if (status == DB_SUCCESS) {
         size = 256;
         db_get_value(hDB, 0, "/Logger/SQL/Hostname", sql_host, &size, TID_STRING, FALSE);
         size = 256;
         db_get_value(hDB, 0, "/Logger/SQL/Database", sql_db, &size, TID_STRING, FALSE);
         size = 256;
         db_get_value(hDB, 0, "/Logger/SQL/Table", sql_table, &size, TID_STRING, FALSE);
         printf("SQL     database is %s/%s/%s", sql_host, sql_db, sql_table);
      }
   }
#endif

   printf("\nMIDAS logger started. Stop with \"!\"\n");

   /* initialize ss_getchar() */
   ss_getchar(0);

   do {
      msg = cm_yield(1000);

      /* update channel statistics once every second */
      if (ss_millitime() - last_time_stat > 1000) {
         last_time_stat = ss_millitime();
         db_send_changed_records();
      }

      /* check for auto restart */
      if (auto_restart > 0 && ss_time() > auto_restart) {
         /* check if really stopped */
         size = sizeof(state);
         status = db_get_value(hDB, 0, "Runinfo/State", &state, &size, TID_INT, TRUE);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "main", "cannot get Runinfo/State in database");

         if (state == STATE_STOPPED) {
            auto_restart = 0;
            size = sizeof(run_number);
            status = db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT, TRUE);
            assert(status == SUCCESS);

            if (run_number <= 0) {
               cm_msg(MERROR, "main", "aborting on attempt to use invalid run number %d", run_number);
               abort();
            }

            cm_msg(MTALK, "main", "starting new run");
            status = cm_transition(TR_START, run_number + 1, NULL, 0, ASYNC, DEBUG_TRANS);
            if (status != CM_SUCCESS)
               cm_msg(MERROR, "main", "cannot restart run");
         }
      }

      /* check keyboard once every second */
      if (ss_millitime() - last_time_kb > 1000) {
         last_time_kb = ss_millitime();

         ch = 0;
         while (ss_kbhit()) {
            ch = ss_getchar(0);
            if (ch == -1)
               ch = getchar();

            if ((char) ch == '!')
               break;
         }
      }

   } while (msg != RPC_SHUTDOWN && msg != SS_ABORT && ch != '!');

   /* reset terminal */
   ss_getchar(TRUE);

   /* close history logging */
   close_history();

   cm_disconnect_experiment();

   return 0;
}

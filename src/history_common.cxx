/********************************************************************\

  Name:         history_common.cxx
  Created by:   Konstantin Olchanski

  Contents:     History functions common to all history implementations

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <map>

#include "midas.h"
#include "msystem.h"
#include "history.h"

#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

#define STRLCPY(dst, src) strlcpy(dst, src, sizeof(dst))

int hs_get_history(HNDLE hDB, HNDLE hKey, int flags, int debug_flag, MidasHistoryInterface **mh)
{
   int status, size;
   char type[NAME_LENGTH];
   int active;
   int debug;
   KEY key;

   *mh = NULL;

   if (!hKey || (flags&HS_GET_DEFAULT)) {
      status = hs_find_reader_channel(hDB, &hKey, debug_flag);
      if (status != HS_SUCCESS)
         return status;
   }

   status = db_get_key(hDB, hKey, &key);
   assert(status == DB_SUCCESS);

   active = 0;
   size = sizeof(active);
   status = db_get_value(hDB, hKey, "Active", &active, &size, TID_BOOL, TRUE);
   assert(status == DB_SUCCESS);
   
   strlcpy(type, "", sizeof(type));
   size = sizeof(type);
   status = db_get_value(hDB, hKey, "Type", type, &size, TID_STRING, TRUE);
   assert(status == DB_SUCCESS);
   
   debug = 0;
   size = sizeof(debug);
   status = db_get_value(hDB, hKey, "Debug", &debug, &size, TID_INT, TRUE);
   assert(status == DB_SUCCESS);

   if (debug_flag)
      printf("hs_get_history: see channel hkey %d, name \'%s\', active %d, type [%s], debug %d\n", hKey, key.name, active, type, debug);

   if (strcasecmp(type, "MIDAS")==0) {
      int i;
      
      i = 1;
      size = sizeof(i);
      status = db_get_value(hDB, 0, "/Logger/WriteFileHistory", &i, &size, TID_BOOL, FALSE);
      if (status==DB_SUCCESS) {
         cm_msg(MERROR, "hs_get_history", "mlogger ODB setting /Logger/WriteFileHistory is obsolete, please delete it. Use /Logger/History/MIDAS/Active instead");
         if (i==0)
            active = 0;
      }
      
      if (active || (flags & HS_GET_INACTIVE)) {
         *mh = MakeMidasHistory();
         assert(*mh);
         
         (*mh)->hs_set_debug(debug);
         
         status = (*mh)->hs_connect(NULL);
         if (status != HS_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "Cannot connect to MIDAS history, status %d", status);
            return status;
         }

         if (debug_flag)
            cm_msg(MINFO, "hs_get_history", "Connected history channel \'%s\' type MIDAS history", key.name);
      }
      
   } else if (strcasecmp(type, "ODBC")==0) {

      if (1) {
         int i;
      
         i = 0;
         size = sizeof(i);
         status = db_get_value(hDB, 0, "/Logger/ODBC_Debug", &i, &size, TID_INT, FALSE);
         if (status==DB_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "mlogger ODB setting /Logger/ODBC_Debug is obsolete, please delete it. Use /Logger/History/ODBC/Debug instead");
         }

         status = db_get_value(hDB, 0, "/History/ODBC_Debug", &i, &size, TID_INT, FALSE);
         if (status==DB_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "mhttpd ODB setting /History/ODBC_Debug is obsolete, please delete it. Use /Logger/History/ODBC/Debug instead");
         }
      
         char dsn[256];
         size = sizeof(dsn);
         dsn[0] = 0;
      
         status = db_get_value(hDB, 0, "/Logger/ODBC_DSN", dsn, &size, TID_STRING, FALSE);
         if (status==DB_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "mlogger ODB setting /Logger/ODBC_DSN is obsolete, please delete it. Use /Logger/History/ODBC/Writer_ODBC_DSN instead");
         }

         status = db_get_value(hDB, 0, "/History/ODBC_DSN", dsn, &size, TID_STRING, FALSE);
         if (status==DB_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "mhttpd ODB setting /History/ODBC_DSN is obsolete, please delete it. Use /Logger/History/ODBC/Reader_ODBC_DSN instead");
         }
      }

      char writer_dsn[256];
      char reader_dsn[256];
      
      size = sizeof(writer_dsn);
      strlcpy(writer_dsn, "history_writer", sizeof(writer_dsn));
      status = db_get_value(hDB, hKey, "Writer_ODBC_DSN", writer_dsn, &size, TID_STRING, TRUE);
      assert(status == DB_SUCCESS);

      size = sizeof(reader_dsn);
      strlcpy(reader_dsn, "history_reader", sizeof(reader_dsn));
      status = db_get_value(hDB, hKey, "Reader_ODBC_DSN", reader_dsn, &size, TID_STRING, TRUE);
      assert(status == DB_SUCCESS);
      
      const char* dsn = "";

      if (flags & HS_GET_READER)
         dsn = reader_dsn;
      else if (flags & HS_GET_WRITER)
         dsn = writer_dsn;
      
      if (active || (flags & HS_GET_INACTIVE)) {
         if (debug == 2) {
            *mh = MakeMidasHistorySqlDebug();
         } else if (strlen(dsn) > 1) {
            *mh = MakeMidasHistoryODBC();
         }

         if (*mh == NULL)
            return HS_FILE_ERROR;
         
         (*mh)->hs_set_debug(debug);

         status = (*mh)->hs_connect(dsn);
         if (status != HS_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "Cannot connect to ODBC SQL driver \'%s\', status %d. Check .odbc.ini and MIDAS documentation", dsn, status);
            return status;
         }

         if (debug_flag)
            cm_msg(MINFO, "hs_get_history", "Connected history channel \'%s\' type ODBC (MySQL), DSN \'%s\'", key.name, dsn);
      }
   } else if (strcasecmp(type, "SQLITE")==0) {

      char expt_path[1024];
      cm_get_path(expt_path, sizeof(expt_path));

      char dir[1024];
      dir[0] = 0;

      std::string path;

      size = sizeof(dir);
      status = db_get_value(hDB, hKey, "Sqlite dir", dir, &size, TID_STRING, TRUE);
      assert(status == DB_SUCCESS);

      if (dir[0] == DIR_SEPARATOR)
         path = dir;
      else {
         path = expt_path;
         // normally expt_path has the trailing '/', see midas.c::cm_set_path()
         if (path[path.length()-1] != DIR_SEPARATOR)
            path += DIR_SEPARATOR_STR;
         path += dir;
      }

      if (active || (flags & HS_GET_INACTIVE)) {
         *mh = MakeMidasHistorySqlite();
         if (*mh == NULL)
            return HS_FILE_ERROR;
         
         (*mh)->hs_set_debug(debug);

         status = (*mh)->hs_connect(path.c_str());
         if (status != HS_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "Cannot connect to SQLITE history, status %d, see messages", status);
            return status;
         }

         if (debug_flag)
            cm_msg(MINFO, "hs_get_history", "Connected history channel \'%s\' type SQLITE in \'%s\'", key.name, path.c_str());
      }
   } else if (strcasecmp(type, "FILE")==0) {

      char expt_path[1024];
      cm_get_path(expt_path, sizeof(expt_path));

      char dir[1024];
      dir[0] = 0;

      std::string path;

      size = sizeof(dir);
      status = db_get_value(hDB, hKey, "History dir", dir, &size, TID_STRING, TRUE);
      assert(status == DB_SUCCESS);

      if (dir[0] == DIR_SEPARATOR)
         path = dir;
      else {
         path = expt_path;
         // normally expt_path has the trailing '/', see midas.c::cm_set_path()
         if (path[path.length()-1] != DIR_SEPARATOR)
            path += DIR_SEPARATOR_STR;
         path += dir;
      }

      if (active || (flags & HS_GET_INACTIVE)) {
         *mh = MakeMidasHistoryFile();
         if (*mh == NULL)
            return HS_FILE_ERROR;
         
         (*mh)->hs_set_debug(debug);
         
         status = (*mh)->hs_connect(path.c_str());
         if (status != HS_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "Cannot connect to FILE history, status %d, see messages", status);
            return status;
         }
         
         if (debug_flag)
            cm_msg(MINFO, "hs_get_history", "Connected history channel \'%s\' type FILE in \'%s\'", key.name, path.c_str());
      }
   } else if (strcasecmp(type, "MYSQL")==0) {

      char writer_dsn[256];
      char reader_dsn[256];
      
      size = sizeof(writer_dsn);
      strlcpy(writer_dsn, "mysql_writer.txt", sizeof(writer_dsn));
      status = db_get_value(hDB, hKey, "MYSQL Writer", writer_dsn, &size, TID_STRING, TRUE);
      assert(status == DB_SUCCESS);

      size = sizeof(reader_dsn);
      strlcpy(reader_dsn, "mysql_reader.txt", sizeof(reader_dsn));
      status = db_get_value(hDB, hKey, "MYSQL Reader", reader_dsn, &size, TID_STRING, TRUE);
      assert(status == DB_SUCCESS);
      
      const char* dsn = "";

      if (flags & HS_GET_READER)
         dsn = reader_dsn;
      else if (flags & HS_GET_WRITER)
         dsn = writer_dsn;

      std::string path;

      if (dsn[0] == DIR_SEPARATOR)
         path = dsn;
      else {
         char expt_path[1024];
         cm_get_path(expt_path, sizeof(expt_path));

         path = expt_path;
         // normally expt_path has the trailing '/', see midas.c::cm_set_path()
         if (path[path.length()-1] != DIR_SEPARATOR)
            path += DIR_SEPARATOR_STR;
         path += dsn;
      }

      if (active || (flags & HS_GET_INACTIVE)) {
         *mh = MakeMidasHistoryMysql();
         if (*mh == NULL)
            return HS_FILE_ERROR;
         
         (*mh)->hs_set_debug(debug);
         
         status = (*mh)->hs_connect(path.c_str());
         if (status != HS_SUCCESS) {
            cm_msg(MERROR, "hs_get_history", "Cannot connect to MYSQL history, status %d", status);
            return status;
         }
         
         if (debug_flag)
            cm_msg(MINFO, "hs_get_history", "Connected history channel \'%s\' type MYSQL at \'%s\'", key.name, path.c_str());
      }
   } else {
      cm_msg(MERROR, "hs_get_history", "Logger history channel /Logger/History/%s/Type has invalid value \'%s\', valid values are MIDAS, ODBC, SQLITE, MYSQL and FILE", key.name, type);
      return HS_FILE_ERROR;
   }

   if (*mh == NULL)
      return HS_FILE_ERROR;
   
   strlcpy((*mh)->name, key.name, sizeof((*mh)->name));
   strlcpy((*mh)->type, type, sizeof((*mh)->type));

   return HS_SUCCESS;
}

// find history reader channel, returns ODB handle to the history channel definition in /Logger/History/...
int hs_find_reader_channel(HNDLE hDB, HNDLE* hKeyOut, int debug_flag)
{
   int status;
   int size;
   HNDLE hKeyChan;
   HNDLE hKey;
   char hschanname[NAME_LENGTH];

   *hKeyOut = 0;

   status = db_find_key(hDB, 0, "/Logger/History", &hKeyChan);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "hs_find_reader_channel", "Cannot find /Logger/History, db_find_key() status %d", status);
      return status;
   }

   // get history channel name selected by user in ODB

   size = sizeof(hschanname);
   hschanname[0] = 0;
   status = db_get_value(hDB, 0, "/History/LoggerHistoryChannel", hschanname, &size, TID_STRING, TRUE);
   assert(status == DB_SUCCESS);

   if (strlen(hschanname) > 0) {
      status = db_find_key(hDB, hKeyChan, hschanname, &hKey);
      if (status == DB_NO_KEY) {
         cm_msg(MERROR, "hs_find_reader_channel", "Misconfigured history: history channel name in /History/LoggerHistoryChannel is \'%s\', not present in /Logger/History, db_find_key() status %d", hschanname, status);
         return HS_FILE_ERROR;
      }
      assert(status == DB_SUCCESS);
      *hKeyOut = hKey;
      return HS_SUCCESS;
   }

   //  if selected channel name is blank, find first active channel
   
   for (int ichan=0; ; ichan++) {
      status = db_enum_key(hDB, hKeyChan, ichan, &hKey);
      if (status != DB_SUCCESS)
         break;
      
      int active = 0;
      size = sizeof(active);
      status = db_get_value(hDB, hKey, "Active", &active, &size, TID_BOOL, FALSE);
      if (status == DB_SUCCESS && active != 0) {
         *hKeyOut = hKey;
         return HS_SUCCESS;
      }
   }
   
   cm_msg(MERROR, "hs_find_reader_channel", "Cannot find default history: /History/LoggerHistoryChannel is empty and there are no active history channels in /Logger/History");
   return HS_FILE_ERROR;
}

static std::string hs_event_list_filename()
{
   char path[256];
   cm_get_path(path, sizeof(path));
   return std::string(path) + ".LOGGER_HISTORY_EVENTS.TXT";
}

// save list of active events
int hs_save_event_list(const std::vector<std::string> *pevents)
{
   std::string ss;
   for (unsigned i=0; i<pevents->size(); i++)
      ss += (*pevents)[i] + "\n";

   std::string fname = hs_event_list_filename();

   FILE *fp = fopen(fname.c_str(), "w");
   if (!fp) {
      cm_msg(MERROR, "hs_save_event_list", "Cannot open file \'%s\', errno %d (%s)", fname.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   const char* s = ss.c_str();
   int len = strlen(s);

   int wr = write(fileno(fp), s, len);

   if (wr != len) {
      cm_msg(MERROR, "hs_save_event_list", "Cannot write to file \'%s\', errno %d (%s)", fname.c_str(), errno, strerror(errno));
      fclose(fp);
      return HS_FILE_ERROR;
   }

   fclose(fp);
   return HS_SUCCESS;
}

// get list of active events
int hs_read_event_list(std::vector<std::string> *pevents)
{
   std::string fname = hs_event_list_filename();

   FILE *fp = fopen(fname.c_str(), "r");
   if (!fp) {
      cm_msg(MERROR, "hs_read_event_list", "Cannot open file \'%s\', errno %d (%s)", fname.c_str(), errno, strerror(errno));
      return HS_FILE_ERROR;
   }

   while (1) {
      char buf[256];
      char *s = fgets(buf, sizeof(buf), fp);
      if (!s) // EOF
         break;
      // kill trailing \n and \r
      s = strchr(buf, '\n');
      if (s)
         *s = 0;
      s = strchr(buf, '\r');
      if (s)
         *s = 0;
      pevents->push_back(buf);
   }

   fclose(fp);
   return HS_SUCCESS;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

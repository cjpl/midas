/********************************************************************\

  Name:         history_midas.cxx
  Created by:   Konstantin Olchanski

  Contents:     Interface class for traditional MIDAS history

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <map>

#include "midas.h"
#include "msystem.h"
#include "history.h"

static WORD get_variable_id(DWORD ltime, const char* evname, const char* tagname)
{
   HNDLE hDB, hKeyRoot;
   int status, i;

   cm_get_experiment_database(&hDB, NULL);
   
   status = db_find_key(hDB, 0, "/History/Events", &hKeyRoot);
   if (status != DB_SUCCESS) {
      return 0;
   }

   for (i = 0;; i++) {
      HNDLE hKey;
      KEY key;
      WORD evid;
      char buf[256];
      int size;
      char *s;
      int j;
      int ntags = 0;
      TAG* tags = NULL;
      char event_name[NAME_LENGTH];

      status = db_enum_key(hDB, hKeyRoot, i, &hKey);
      if (status != DB_SUCCESS)
	 break;

      status = db_get_key(hDB, hKey, &key);
      assert(status == DB_SUCCESS);

      if (!isdigit(key.name[0]))
         continue;

      evid = atoi(key.name);

      assert(key.item_size < (int)sizeof(buf));

      size = sizeof(buf);
      status = db_get_data(hDB, hKey, buf, &size, TID_STRING);
      assert(status == DB_SUCCESS);

      strlcpy(event_name, buf, sizeof(event_name));

      s = strchr(buf,':');
      if (s)
         *s = 0;

      //printf("Found event %d, event [%s] name [%s], looking for [%s][%s]\n", evid, event_name, buf, evname, tagname);

      if (!equal_ustring((char *)evname, buf))
         continue;

      status = hs_get_tags(ltime, evid, event_name, &ntags, &tags);

      //printf("status %d, ntags %d\n", status, ntags);

      //status = hs_get_tags(ltime, evid, event_name, &ntags, &tags);

      for (j=0; j<ntags; j++) {
         //printf("at %d [%s] looking for [%s]\n", j, tags[j].name, tagname);

         if (equal_ustring((char *)tagname, tags[j].name)) {
            if (tags)
               free(tags);
            return evid;
         }
      }

      if (tags)
         free(tags);
      tags = NULL;
   }

   return 0;
}

static WORD get_variable_id_tags(const char* evname, const char* tagname)
{
   HNDLE hDB, hKeyRoot;
   int status, i;

   cm_get_experiment_database(&hDB, NULL);
   
   status = db_find_key(hDB, 0, "/History/Tags", &hKeyRoot);
   if (status != DB_SUCCESS) {
      return 0;
   }

   for (i = 0;; i++) {
      HNDLE hKey;
      KEY key;
      WORD evid;
      char buf[256];
      int size;
      char *s;
      int j;

      status = db_enum_key(hDB, hKeyRoot, i, &hKey);
      if (status != DB_SUCCESS)
	 break;

      status = db_get_key(hDB, hKey, &key);
      assert(status == DB_SUCCESS);

      if (key.type != TID_STRING)
         continue;

      if (!isdigit(key.name[0]))
         continue;

      evid = atoi(key.name);

      assert(key.item_size < (int)sizeof(buf));

      size = sizeof(buf);
      status = db_get_data_index(hDB, hKey, buf, &size, 0, TID_STRING);
      assert(status == DB_SUCCESS);

      s = strchr(buf,'/');
      if (s)
         *s = 0;

      //printf("Found event %d, name [%s], looking for [%s][%s]\n", evid, buf, evname, tagname);

      if (!equal_ustring((char *)evname, buf))
         continue;

      for (j=1; j<key.num_values; j++) {
         size = sizeof(buf);
         status = db_get_data_index(hDB, hKey, buf, &size, j, TID_STRING);
         assert(status == DB_SUCCESS);

         if (!isdigit(buf[0]))
            continue;

         s = strchr(buf,' ');
         if (!s)
            continue;

         s++;
 
         //printf("at %d [%s] [%s] compare to [%s]\n", j, buf, s, tagname);

         if (equal_ustring((char *)tagname, s)) {
            //printf("Found evid %d\n", evid);
            return evid;
         }
      }
   }

   return 0;
}

#if 0
char* sort_names(char* names)
{
   int i, p;
   int len = 0;
   int num = 0;
   char* arr_names;
   struct poor_mans_list sorted;

   for (i=0, p=0; names[p]!=0; i++) {
      const char*pp = names+p;
      int pplen = strlen(pp);
      //printf("%d [%s] %d\n", i, pp, pplen);
      if (pplen > len)
         len = pplen;
      p += strlen(names+p) + 1;
   }

   num = i;

   len+=1; // space for string terminator '\0'

   arr_names = (char*)malloc(len*num);

   for (i=0, p=0; names[p]!=0; i++) {
      const char*pp = names+p;
      strlcpy(arr_names+i*len, pp, len);
      p += strlen(names+p) + 1;
   }

   free(names);

   qsort(arr_names, num, len, sort_tags);

   list_init(&sorted);

   for (i=0; i<num; i++)
      list_add(&sorted, arr_names+i*len);

   return sorted.names;
}
#endif

#define STRLCPY(dst, src) strlcpy(dst, src, sizeof(dst))

/*------------------------------------------------------------------*/

class MidasHistory: public MidasHistoryInterface
{
public:
   int fDebug;

   int fListSource;

   std::vector<std::string> fEventsCache;
   std::map<std::string, std::vector<TAG> > fTagsCache;

public:
   MidasHistory() // ctor
   {
      fDebug = 0;
      fListSource = 0;
   }

   ~MidasHistory() // dtor
   {
      // empty
   }

   /*------------------------------------------------------------------*/

   int hs_connect(const char* connect_string)
   {
      HNDLE hDB;
      int status;
      char str[1024];

      cm_get_experiment_database(&hDB, NULL);

      /* check dedicated history path */
      int size = sizeof(str);
      memset(str, 0, size);
      
      status = db_get_value(hDB, 0, "/Logger/History dir", str, &size, TID_STRING, FALSE);
      if (status != DB_SUCCESS)
         status = db_get_value(hDB, 0, "/Logger/Data dir", str, &size, TID_STRING, TRUE);

      if (status == DB_SUCCESS)
         ::hs_set_path(str);

      /* select which list of events and variables to use */

      int oldListSource = fListSource;
      
      fListSource = 0;
      size = sizeof(fListSource);
      status = db_get_value(hDB, 0, "/History/ListSource", &fListSource, &size, TID_INT, TRUE);
      
      /* by default "/History/ListSource" is set to zero, then use /History/Tags if available */
      
      if (fListSource == 0) {
         HNDLE hKey;
         status = db_find_key(hDB, 0, "/History/Tags", &hKey);
         if (status == DB_SUCCESS)
            fListSource = 2;
      }
      
      /* if "Tags" not present use "/History/Events" in conjunction with hs_get_tags() */
      
      if (fListSource == 0) {
         HNDLE hKey;
         status = db_find_key(hDB, 0, "/History/Events", &hKey);
         if (status == DB_SUCCESS)
            fListSource = 3;
      }

      if (fListSource != oldListSource)
         hs_clear_cache();

      if (fDebug)
         printf("hs_connect: path [%s], list source %d\n", str, fListSource);

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_disconnect()
   {
      hs_clear_cache();
      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_set_debug(int debug)
   {
      return debug;
   }

   /*------------------------------------------------------------------*/

   int hs_clear_cache()
   {
      if (fDebug)
         printf("hs_clear_cache!\n");

      fEventsCache.clear();
      fTagsCache.clear();
      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int xxx(const char*)
   {
      assert(!"not implemented!");
      return 0;
   }

   int hs_define_event(const char* event_name, int ntags, const TAG tags[])
   {
      int event_id = xxx(event_name);
      return ::hs_define_event(event_id, (char*)event_name, (TAG*)tags, ntags*sizeof(TAG));
   }

   int hs_write_event(const char*  event_name, time_t timestamp, int data_size, const char* data)
   {
      int event_id = xxx(event_name);
      return ::hs_write_event(event_id, (void*)data, data_size);
   }

   /*------------------------------------------------------------------*/

   int GetEventsFromEquipment(std::vector<std::string> *events)
   {
      HNDLE hDB, hKeyRoot;
      int i, status;

      cm_get_experiment_database(&hDB, NULL);

      status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }

      /* loop over equipment to display event name */
      for (i = 0;; i++) {
         HNDLE hKeyEq;
         int history;
         int size;
      
         status = db_enum_key(hDB, hKeyRoot, i, &hKeyEq);
         if (status != DB_SUCCESS)
            break;
    
         /* check history flag */
         size = sizeof(history);
         db_get_value(hDB, hKeyEq, "Common/Log history", &history, &size, TID_INT, TRUE);
    
         /* show event only if log history flag is on */
         if (history > 0) {
            KEY key;
            char *evname;

            /* get equipment name */
            db_get_key(hDB, hKeyEq, &key);

            evname = key.name;

            events->push_back(evname);
         
            //printf("event \'%s\'\n", evname);
         }
      }
   
      /* loop over history links to display event name */
      status = db_find_key(hDB, 0, "/History/Links", &hKeyRoot);
      if (status == DB_SUCCESS) {
         for (i = 0;; i++) {
            HNDLE hKey;
            KEY key;
            char* evname;

            status = db_enum_link(hDB, hKeyRoot, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;
      
            db_get_key(hDB, hKey, &key);
      
            evname = key.name;

            events->push_back(evname);

            //printf("event \'%s\'\n", evname);
         }
      }

      return HS_SUCCESS;
   }

   int GetEventsFromOdbEvents(std::vector<std::string> *events)
   {
      int i;
      HNDLE hDB, hKeyRoot;
      int status;

      cm_get_experiment_database(&hDB, NULL);

      status = db_find_key(hDB, 0, "/History/Events", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }

      /* loop over tags to display event names */
      for (i = 0;; i++) {
         HNDLE hKeyEq;
         char *s;
         char evname[1024+NAME_LENGTH];
         int size;
      
         status = db_enum_key(hDB, hKeyRoot, i, &hKeyEq);
         if (status != DB_SUCCESS)
            break;
    
         size = sizeof(evname);
         status = db_get_data(hDB, hKeyEq, evname, &size, TID_STRING);
         assert(status == DB_SUCCESS);

         s = strchr(evname,':');
         if (s)
            *s = '/';

         /* skip duplicated event names */

         int found = 0;
         for (unsigned i=0; i<events->size(); i++) {
            if (equal_ustring(evname, (*events)[i].c_str())) {
               found = 1;
               break;
            }
         }
    
         if (found)
            continue;

         events->push_back(evname);

         //printf("event \'%s\'\n", evname);
      }

      return HS_SUCCESS;
   }

   int GetEventsFromOdbTags(std::vector<std::string> *events)
   {
      HNDLE hDB;
      int i;
      HNDLE hKeyRoot;
      int status;

      cm_get_experiment_database(&hDB, NULL);

      status = db_find_key(hDB, 0, "/History/Tags", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }
   
      /* loop over tags to display event names */
      for (i = 0;; i++) {
         HNDLE hKeyEq;
         KEY key;
         char *s;
         WORD event_id;
         char evname[1024+NAME_LENGTH];
         int size;
      
         status = db_enum_key(hDB, hKeyRoot, i, &hKeyEq);
         if (status != DB_SUCCESS)
            break;
    
         /* get event name */
         db_get_key(hDB, hKeyEq, &key);
      
         //printf("key \'%s\'\n", key.name);
      
         if (key.type != TID_STRING)
            continue;

         /* parse event name in format: "event_id" or "event_id:var_name" */
         s = key.name;
      
         event_id = (WORD)strtoul(s,&s,0);
         if (event_id == 0)
            continue;
         if (s[0] != 0)
            continue;

         size = sizeof(evname);
         status = db_get_data_index(hDB, hKeyEq, evname, &size, 0, TID_STRING);
         assert(status == DB_SUCCESS);

         /* skip duplicated event names */

         int found = 0;
         for (unsigned i=0; i<events->size(); i++) {
            if (equal_ustring(evname, (*events)[i].c_str())) {
               found = 1;
               break;
            }
         }
    
         if (found)
            continue;

         events->push_back(evname);

         //printf("event %d \'%s\'\n", event_id, evname);
      }

      return HS_SUCCESS;
   }

   int hs_get_events(std::vector<std::string> *pevents)
   {
      assert(pevents);
      pevents->clear();

      if (fEventsCache.size() == 0) {
         int status;
         HNDLE hDB;
         cm_get_experiment_database(&hDB, NULL);

         if (fDebug)
            printf("hs_get_events: reading events list!\n");
         
         switch (fListSource) {
         case 0:
         case 1:
            status = GetEventsFromEquipment(&fEventsCache);
            break;
         case 2:
            status = GetEventsFromOdbTags(&fEventsCache);
            break;
         case 3:
            status = GetEventsFromOdbEvents(&fEventsCache);
            break;
         default:
            return HS_FILE_ERROR;
         }

         if (status != HS_SUCCESS)
            return status;
      }

      for (unsigned i=0; i<fEventsCache.size(); i++)
         pevents->push_back(fEventsCache[i]);
         
      return HS_SUCCESS;
   }

   int xhs_event_id(time_t t, const char* event_name, const char* tag_name, int *pevid)
   {
      if (fDebug)
         printf("xhs_event_id for event [%s], tag [%s]\n", event_name, tag_name);

      *pevid = 0;

      /* use "/History/Tags" if available */
      DWORD event_id = get_variable_id_tags(event_name, tag_name);
      
      /* if no Tags, use "/History/Events" and hs_get_tags() to read definition from history files */
      if (event_id == 0)
         event_id = get_variable_id((DWORD)t, event_name, tag_name);

      /* special kludge because run transitions are not listed in /History/Events */
      if ((event_id == 0) && equal_ustring(event_name, "Run transitions")) {
         *pevid = 0;
         return HS_SUCCESS;
      }
      
      /* if nothing works, use hs_get_event_id() */
      if (event_id == 0) {
         int status = hs_get_event_id(0, (char*)event_name, &event_id);
         if (status != HS_SUCCESS)
            return status;
      }
      
      if (event_id == 0)
         return HS_UNDEFINED_VAR;
      
      *pevid = event_id;
      
      return HS_SUCCESS;
   }
   
   int GetTagsFromEquipment(const char* event_name, std::vector<TAG> *ptags)
   {
      HNDLE hDB, hKeyRoot;
      HNDLE hKey, hKeyEq, hKeyVar;
      BOOL is_link = FALSE;
      int status;
      int i;
      char eq_name[NAME_LENGTH];
      char str[256];

      cm_get_experiment_database(&hDB, NULL);

      status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }

      /* display variables for selected event */
   
      STRLCPY(eq_name, event_name);
  
      is_link = FALSE;
      db_find_key(hDB, hKeyRoot, eq_name, &hKeyEq);
      if (!hKeyEq) {
         sprintf(str, "/History/Links/%s", eq_name);
         status = db_find_link(hDB, 0, str, &hKeyVar);
         if (status != DB_SUCCESS) {
            return HS_FILE_ERROR;
         } else
            is_link = TRUE;
      }
  
      /* go through variables for selected event */
      if (!is_link) {
         sprintf(str, "/Equipment/%s/Variables", eq_name);
         status = db_find_key(hDB, 0, str, &hKeyVar);
         if (status != DB_SUCCESS) {
            return HS_FILE_ERROR;
         }
      }
  
      for (i = 0;; i++) {
         KEY varkey;

         status = db_enum_link(hDB, hKeyVar, i, &hKey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         if (is_link) {
            db_get_key(hDB, hKey, &varkey);
            TAG t;
            STRLCPY(t.name, varkey.name);
            t.n_data = varkey.num_values;
            t.type = varkey.type;
            ptags->push_back(t);
         } else {
            int n_names;
            int single_names;
            HNDLE hKeyNames;
            char var_name[NAME_LENGTH];
         
            /* get variable key */
            db_get_key(hDB, hKey, &varkey);
            n_names = 0;
         
            /* look for names */
            db_find_key(hDB, hKeyEq, "Settings/Names", &hKeyNames);
            single_names = (hKeyNames > 0);
            if (hKeyNames) {
               KEY key;
               /* get variables from names list */
               db_get_key(hDB, hKeyNames, &key);
               n_names = key.num_values;
            } else {
               KEY key;
               sprintf(str, "Settings/Names %s", varkey.name);
               db_find_key(hDB, hKeyEq, str, &hKeyNames);
               if (hKeyNames) {
                  /* get variables from names list */
                  db_get_key(hDB, hKeyNames, &key);
                  n_names = key.num_values;
               }
            }
      
            if (hKeyNames) {
               int j;

               /* loop over array elements */
               for (j = 0; j < n_names; j++) {
                  int size;
                  /* get name #j */
                  size = NAME_LENGTH;
                  db_get_data_index(hDB, hKeyNames, var_name, &size, j, TID_STRING);
               
                  /* append variable key name for single name array */
                  if (single_names) {
                     strlcat(var_name, " ", sizeof(var_name));
                     strlcat(var_name, varkey.name, sizeof(var_name));
                  }

                  TAG t;
                  STRLCPY(t.name, var_name);
                  t.type = varkey.type;
                  t.n_data = 1;
                  
                  ptags->push_back(t);
               }
            } else {
               TAG t;
               STRLCPY(t.name, varkey.name);
               t.n_data = varkey.num_values;
               t.type = varkey.type;

               ptags->push_back(t);
            }
         }
      }

      return HS_SUCCESS;
   }

   int GetTagsFromHS(const char* event_name, std::vector<TAG> *ptags)
   {
      time_t now = time(NULL);
      int evid;
      int status = xhs_event_id(now, event_name, NULL, &evid);
      if (status != HS_SUCCESS)
         return status;
      
      if (fDebug)
         printf("hs_get_tags: get tags for event [%s] %d\n", event_name, evid);
      
      int ntags;
      TAG* tags;
      status =  ::hs_get_tags((DWORD)now, evid, (char*)event_name, &ntags, &tags);
      
      if (status != HS_SUCCESS)
         return status;
      
      for (int i=0; i<ntags; i++)
         ptags->push_back(tags[i]);
      
      if (tags)
         free(tags);
      
      if (fDebug)
         printf("hs_get_tags: get tags for event [%s] %d, found %d tags\n", event_name, evid, ntags);

      return HS_SUCCESS;
   }

   int GetTagsFromOdb(const char* event_name, std::vector<TAG> *ptags)
   {
      HNDLE hDB, hKeyRoot;
      int i, j;
      int status;

      cm_get_experiment_database(&hDB, NULL);

      status = db_find_key(hDB, 0, "/History/Tags", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }
   
      /* loop over equipment to display event name */
      for (i = 0;; i++) {
         HNDLE hKey;
         KEY key;
         WORD event_id;
         char buf[256];
         int size;
         char* s;
      
         status = db_enum_key(hDB, hKeyRoot, i, &hKey);
         if (status != DB_SUCCESS)
            break;
    
         /* get event name */
         status = db_get_key(hDB, hKey, &key);
         assert(status == DB_SUCCESS);
      
         /* parse event id */
         if (!isdigit(key.name[0]))
            continue;

         event_id = atoi(key.name);
         if (event_id == 0)
            continue;

         if (key.item_size >= (int)sizeof(buf))
            continue;

         if (key.num_values == 1) { // old format of "/History/Tags"

            HNDLE hKeyDir;
            sprintf(buf, "Tags %d", event_id);
            status = db_find_key(hDB, hKeyRoot, buf, &hKeyDir);
            if (status != DB_SUCCESS)
               continue;

            /* loop over tags */
            for (j=0; ; j++) {
               HNDLE hKey;
               WORD array;
               int size;
               char var_name[NAME_LENGTH];
            
               status = db_enum_key(hDB, hKeyDir, j, &hKey);
               if (status != DB_SUCCESS)
                  break;
            
               /* get event name */
               status = db_get_key(hDB, hKey, &key);
               assert(status == DB_SUCCESS);
            
               array = 1;
               size  = sizeof(array);
               status = db_get_data(hDB, hKey, &array, &size, TID_WORD);
               assert(status == DB_SUCCESS);
            
               strlcpy(var_name, key.name, sizeof(var_name));
            
               //printf("Found %s, event %d (%s), tag (%s) array %d\n", key.name, event_id, event_name, var_name, array);
            
               TAG t;
               STRLCPY(t.name, var_name);
               t.n_data = array;
               t.type = 0;
               
               ptags->push_back(t);
            }

            continue;
         }

         if (key.type != TID_STRING)
            continue;

         size = sizeof(buf);
         status = db_get_data_index(hDB, hKey, buf, &size, 0, TID_STRING);
         assert(status == DB_SUCCESS);

         if (strchr(event_name, '/')==NULL) {
            char* s = strchr(buf, '/');
            if (s)
               *s = 0;
         }

         //printf("evid %d, name [%s]\n", event_id, buf);

         if (!equal_ustring(buf, event_name))
            continue;

         /* loop over tags */
         for (j=1; j<key.num_values; j++) {
            int array;
            int size;
            char var_name[NAME_LENGTH];
            int ev_type;
         
            size = sizeof(buf);
            status = db_get_data_index(hDB, hKey, buf, &size, j, TID_STRING);
            assert(status == DB_SUCCESS);

            //printf("index %d [%s]\n", j, buf);

            if (!isdigit(buf[0]))
               continue;

            sscanf(buf, "%d[%d]", &ev_type, &array);

            s = strchr(buf, ' ');
            if (!s)
               continue;
            s++;

            STRLCPY(var_name, s);

            TAG t;
            STRLCPY(t.name, var_name);
            t.n_data = array;
            t.type = ev_type;

            //printf("Found %s, event %d, tag (%s) array %d, type %d\n", buf, event_id, var_name, array, ev_type);

            ptags->push_back(t);
         }
      }

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_get_tags(const char* event_name, std::vector<TAG> *ptags)
   {
      std::vector<TAG>& ttt = fTagsCache[event_name];

      if (ttt.size() == 0) {
         int status = HS_FILE_ERROR;

         if (fDebug)
            printf("hs_get_tags: reading tags for event [%s] using list source %d\n", event_name, fListSource);

         switch (fListSource) {
         case 0:
         case 1:
            status = GetTagsFromEquipment(event_name, &ttt);
            break;
         case 2:
            status = GetTagsFromOdb(event_name, &ttt);
            break;
         case 3:
            status = GetTagsFromHS(event_name, &ttt);
            break;
         }

         if (status != HS_SUCCESS)
            return status;
      }

      for (unsigned i=0; i<ttt.size(); i++)
         ptags->push_back(ttt[i]);

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_read(time_t start_time, time_t end_time, time_t interval,
               int num_var,
               const char* event_name[], const char* tag_name[], const int var_index[],
               int num_entries[],
               time_t* time_buffer[], double* data_buffer[],
               int st[])
   {
      DWORD* tbuffer = NULL;
      char* ybuffer = NULL;
      DWORD bsize, tsize;
      int hbuffer_size = 0;
      
      if (hbuffer_size == 0) {
         hbuffer_size = 1000 * sizeof(DWORD);
         tbuffer = (DWORD*)malloc(hbuffer_size);
         ybuffer = (char*)malloc(hbuffer_size);
      }

      for (int i=0; i<num_var; i++) {
         DWORD tid;
         int event_id;
         
         int status = xhs_event_id(start_time, event_name[i], tag_name[i], &event_id);
         
         if (status != HS_SUCCESS) {
            st[i] = status;
            continue;
         }
         
         DWORD n_point = 0;
         
         do {
            bsize = tsize = hbuffer_size;
            memset(ybuffer, 0, bsize);
            status = ::hs_read(event_id, (DWORD)start_time, (DWORD)end_time, (DWORD)interval,
                               (char*)tag_name[i], var_index[i],
                               tbuffer, &tsize,
                               ybuffer, &bsize,
                               &tid, &n_point);
         
            if (fDebug)
               printf("hs_read %d \'%s\' [%d] returned %d, %d entries\n", event_id, tag_name[i], var_index[i], status, n_point);
         
            if (status == HS_TRUNCATED) {
               hbuffer_size *= 2;
               tbuffer = (DWORD*)realloc(tbuffer, hbuffer_size);
               assert(tbuffer);
               ybuffer = (char*)realloc(ybuffer, hbuffer_size);
               assert(ybuffer);
               continue;
            }
         } while (false); // loop once
        
         st[i] = status;

         time_t* x = (time_t*)malloc(n_point*sizeof(time_t));
         assert(x);
         double* y = (double*)malloc(n_point*sizeof(double));
         assert(x);

         time_buffer[i] = x;
         data_buffer[i] = y;

         int n_vp = 0;

         for (unsigned j = 0; j < n_point; j++) {
            x[n_vp] = tbuffer[j];
          
            /* convert data to float */
            switch (tid) {
            case TID_BYTE:
               y[n_vp] =  *(((BYTE *) ybuffer) + j);
               break;
            case TID_SBYTE:
               y[n_vp] =  *(((char *) ybuffer) + j);
               break;
            case TID_CHAR:
               y[n_vp] =  *(((char *) ybuffer) + j);
               break;
            case TID_WORD:
               y[n_vp] =  *(((WORD *) ybuffer) + j);
               break;
            case TID_SHORT:
               y[n_vp] =  *(((short *) ybuffer) + j);
               break;
            case TID_DWORD:
               y[n_vp] =  *(((DWORD *) ybuffer) + j);
               break;
            case TID_INT:
               y[n_vp] =  *(((INT *) ybuffer) + j);
               break;
            case TID_BOOL:
               y[n_vp] =  *(((BOOL *) ybuffer) + j);
               break;
            case TID_FLOAT:
               y[n_vp] =  *(((float *) ybuffer) + j);
               break;
            case TID_DOUBLE:
               y[n_vp] =  *(((double *) ybuffer) + j);
               break;
            }
          
            n_vp++;
         }

         num_entries[i] = n_vp;
      }

      if (ybuffer)
         free(ybuffer);
      if (tbuffer)
         free(tbuffer);

      return HS_SUCCESS;
   }
}; // end class

MidasHistoryInterface* MakeMidasHistory()
{
   // midas history is a singleton class
   static MidasHistory* gh = NULL;
   if (!gh)
      gh = new MidasHistory;
   return gh;
}

// end

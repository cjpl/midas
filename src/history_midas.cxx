/********************************************************************\

  Name:         history_midas.cxx
  Created by:   Konstantin Olchanski

  Contents:     Interface class for traditional MIDAS history

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

/*------------------------------------------------------------------*/

class MidasHistory: public MidasHistoryInterface
{
public:
   HNDLE fDB;
   int fDebug;

   std::vector<std::string> fEventsCache;
   std::map<std::string, std::vector<TAG> > fTagsCache;
   std::map<std::string, int > fEvidCache;

public:
   MidasHistory() // ctor
   {
      fDebug = 0;
   }

   ~MidasHistory() // dtor
   {
      // empty
   }

   /*------------------------------------------------------------------*/

   int hs_connect(const char* unused_connect_string)
   {
      int status;
      char str[1024];

      status = cm_get_experiment_database(&fDB, NULL);
      assert(status == CM_SUCCESS);
      assert(fDB != 0);

      /* delete obsolete odb entries */

      if (1) {
         HNDLE hKey;
         status = db_find_key(fDB, 0, "/History/ListSource", &hKey);
         if (status == DB_SUCCESS)
            db_delete_key(fDB, hKey, FALSE);
      }

      /* check dedicated history path */
      int size = sizeof(str);
      memset(str, 0, size);
      
      status = db_get_value(fDB, 0, "/Logger/History dir", str, &size, TID_STRING, FALSE);
      if (status != DB_SUCCESS)
         status = db_get_value(fDB, 0, "/Logger/Data dir", str, &size, TID_STRING, TRUE);

      if (status == DB_SUCCESS)
         ::hs_set_path(str);

      if (fDebug)
         printf("hs_connect: path [%s]\n", str);

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
      fEvidCache.clear();
      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int FindEventId(const char* event_name)
   {
      HNDLE hKeyRoot;
      int status;
      char name[256];
      STRLCPY(name, event_name);
      char *s = strchr(name, '/');
      if (s)
         *s = ':';

      //printf("Looking for event id for \'%s\'\n", name);
   
      status = db_find_key(fDB, 0, "/History/Events", &hKeyRoot);
      if (status == DB_SUCCESS) {
         for (int i = 0;; i++) {
            HNDLE hKey;
            KEY key;
         
            status = db_enum_key(fDB, hKeyRoot, i, &hKey);
            if (status != DB_SUCCESS)
               break;
         
            status = db_get_key(fDB, hKey, &key);
            assert(status == DB_SUCCESS);
         
            //printf("key \'%s\'\n", key.name);
         
            int evid = (WORD) strtol(key.name, NULL, 0);
            if (evid == 0)
               continue;

            char tmp[NAME_LENGTH+NAME_LENGTH+2];
            int size = sizeof(tmp);
            status = db_get_data(fDB, hKey, tmp, &size, TID_STRING);
            assert(status == DB_SUCCESS);

            //printf("got %d \'%s\' looking for \'%s\'\n", evid, tmp, name);

            if (equal_ustring(name, tmp))
               return evid;
         }
      }

      return -1;
   }

   /*------------------------------------------------------------------*/

   int AllocateEventId(const char* event_name)
   {
      int status;
      char name[256];
      STRLCPY(name, event_name);
      char *s = strchr(name, '/');
      if (s)
         *s = ':';
      
      // special event id for run transitions
      if (strcmp(name, "Run transitions")==0) {
         status = db_set_value(fDB, 0, "/History/Events/0", name, strlen(name)+1, 1, TID_STRING);
         assert(status == DB_SUCCESS);
         return 0;
      }

      if (1) {
         char tmp[256];
         WORD evid;
         int size;

         sprintf(tmp,"/Equipment/%s/Common/Event ID", name);
         assert(strlen(tmp) < sizeof(tmp));

         size = sizeof(evid);
         status = db_get_value(fDB, 0, tmp, &evid, &size, TID_WORD, FALSE);
         if (status == DB_SUCCESS) {

            sprintf(tmp,"/History/Events/%d", evid);
            assert(strlen(tmp) < sizeof(tmp));

            status = db_set_value(fDB, 0, tmp, name, strlen(name)+1, 1, TID_STRING);
            assert(status == DB_SUCCESS);

            return evid;
         }
      }

      for (int evid = 101; evid < 65000; evid++) {
         char tmp[256];
         HNDLE hKey;

         sprintf(tmp,"/History/Events/%d", evid);

         status = db_find_key(fDB, 0, tmp, &hKey);
         if (status != DB_SUCCESS) {

            status = db_set_value(fDB, 0, tmp, name, strlen(name)+1, 1, TID_STRING);
            assert(status == DB_SUCCESS);
            
            return evid;
         }
      }

      cm_msg(MERROR, "AllocateEventId", "Cannot allocate history event id - all in use - please examine /History/Events");
      return -1;
   }

   /*------------------------------------------------------------------*/

   int CreateOdbTags(int event_id, const char* event_name, int ntags, const TAG tags[])
   {
      int disableTags;
      int oldTags;
      int size, status;

      /* create history tags for mhttpd */

      disableTags = 0;
      size = sizeof(disableTags);
      status = db_get_value(fDB, 0, "/History/DisableTags", &disableTags, &size, TID_BOOL, TRUE);

      oldTags = 0;
      size = sizeof(oldTags);
      status = db_get_value(fDB, 0, "/History/CreateOldTags", &oldTags, &size, TID_BOOL, FALSE);

      if (disableTags) {
         HNDLE hKey;

         status = db_find_key(fDB, 0, "/History/Tags", &hKey);
         if (status == DB_SUCCESS) {
            status = db_delete_key(fDB, hKey, FALSE);
            if (status != DB_SUCCESS)
               cm_msg(MERROR, "add_event", "Cannot delete /History/Tags, db_delete_key() status %d", status);
         }

      } else if (oldTags) {

         char buf[256];

         sprintf(buf, "/History/Tags/%d", event_id);

         //printf("Set tag \'%s\' = \'%s\'\n", buf, event_name);

         status = db_set_value(fDB, 0, buf, (void*)event_name, strlen(event_name)+1, 1, TID_STRING);
         assert(status == DB_SUCCESS);

         for (int i=0; i<ntags; i++) {
            WORD v = (WORD) tags[i].n_data;
            sprintf(buf, "/History/Tags/Tags %d/%s", event_id, tags[i].name);

            //printf("Set tag \'%s\' = %d\n", buf, v);

            status = db_set_value(fDB, 0, buf, &v, sizeof(v), 1, TID_WORD);
            assert(status == DB_SUCCESS);

            if (strlen(tags[i].name) == NAME_LENGTH-1)
               cm_msg(MERROR, "add_event",
                      "Tag name \'%s\' in event %d (%s) may have been truncated to %d characters",
                      tags[i].name, event_id, event_name, NAME_LENGTH-1);
         }

      } else {

         const int kLength = 32 + NAME_LENGTH + NAME_LENGTH;
         char buf[kLength];
         HNDLE hKey;

         sprintf(buf, "/History/Tags/%d", event_id);
         status = db_find_key(fDB, 0, buf, &hKey);

         if (status == DB_SUCCESS) {
            // add new tags
            KEY key;

            status = db_get_key(fDB, hKey, &key);
            assert(status == DB_SUCCESS);

            assert(key.type == TID_STRING);

            if (key.item_size < kLength && key.num_values == 1) {
               // old style tags are present. Convert them to new style!

               HNDLE hTags;

               cm_msg(MINFO, "add_event", "Converting old event %d (%s) tags to new style", event_id, event_name);

               status = db_set_data(fDB, hKey, event_name, kLength, 1, TID_STRING);
               assert(status == DB_SUCCESS);

               sprintf(buf, "/History/Tags/Tags %d", event_id);

               status = db_find_key(fDB, 0, buf, &hTags);

               if (status == DB_SUCCESS) {
                  for (int i=0; ; i++) {
                     HNDLE h;
                     int size;
                     KEY key;
                     WORD w;

                     status = db_enum_key(fDB, hTags, i, &h);
                     if (status == DB_NO_MORE_SUBKEYS)
                        break;
                     assert(status == DB_SUCCESS);

                     status = db_get_key(fDB, h, &key);

                     size = sizeof(w);
                     status = db_get_data(fDB, h, &w, &size, TID_WORD);
                     assert(status == DB_SUCCESS);

                     sprintf(buf, "%d[%d] %s", 0, w, key.name);
                  
                     status = db_set_data_index(fDB, hKey, buf, kLength, 1+i, TID_STRING);
                     assert(status == DB_SUCCESS);
                  }

                  status = db_delete_key(fDB, hTags, TRUE);
                  assert(status == DB_SUCCESS);
               }

               // format conversion has changed the key, get it again
               status = db_get_key(fDB, hKey, &key);
               assert(status == DB_SUCCESS);
            }

            if (1) {
               // add new tags
         
               int size = key.item_size * key.num_values;
               int num = key.num_values;

               char* s = (char*)malloc(size);
               assert(s != NULL);

               TAG* t = (TAG*)malloc(sizeof(TAG)*(key.num_values + ntags));
               assert(t != NULL);

               status = db_get_data(fDB, hKey, s, &size, TID_STRING);
               assert(status == DB_SUCCESS);

               for (int i=1; i<key.num_values; i++) {
                  char* ss = s + i*key.item_size;

                  t[i].type = 0;
                  t[i].n_data = 0;
                  t[i].name[0] = 0;

                  if (isdigit(ss[0])) {
                     //sscanf(ss, "%d[%d] %s", &t[i].type, &t[i].n_data, t[i].name);

                     t[i].type = strtoul(ss, &ss, 0);
                     assert(*ss == '[');
                     ss++;
                     t[i].n_data = strtoul(ss, &ss, 0);
                     assert(*ss == ']');
                     ss++;
                     assert(*ss == ' ');
                     ss++;
                     strlcpy(t[i].name, ss, sizeof(t[i].name));

                     //printf("type %d, n_data %d, name [%s]\n", t[i].type, t[i].n_data, t[i].name);
                  }
               }

               for (int i=0; i<ntags; i++) {
                  int k = 0;

                  for (int j=1; j<key.num_values; j++) {
                     if (equal_ustring((char*)tags[i].name, (char*)t[j].name)) {
                        if ((tags[i].type!=t[j].type) || (tags[i].n_data!=t[j].n_data)) {
                           cm_msg(MINFO, "add_event", "Event %d (%s) tag \"%s\" type and size changed from %d[%d] to %d[%d]",
                                  event_id, event_name,
                                  tags[i].name,
                                  t[j].type, t[j].n_data,
                                  tags[i].type, tags[i].n_data);
                           k = j;
                           break;
                        }

                        k = -1;
                        break;
                     }
                  }

                  // if tag not present, k==0, so append it to the array

                  if (k==0)
                     k = num;

                  if (k > 0) {
                     sprintf(buf, "%d[%d] %s", tags[i].type, tags[i].n_data, tags[i].name);

                     status = db_set_data_index(fDB, hKey, buf, kLength, k, TID_STRING);
                     assert(status == DB_SUCCESS);

                     if (k >= num)
                        num = k+1;
                  }
               }

               free(s);
               free(t);
            }

         } else if (status == DB_NO_KEY) {
            // create new array of tags
            status = db_create_key(fDB, 0, buf, TID_STRING);
            assert(status == DB_SUCCESS);

            status = db_find_key(fDB, 0, buf, &hKey);
            assert(status == DB_SUCCESS);

            status = db_set_data(fDB, hKey, event_name, kLength, 1, TID_STRING);
            assert(status == DB_SUCCESS);

            for (int i=0; i<ntags; i++) {
               sprintf(buf, "%d[%d] %s", tags[i].type, tags[i].n_data, tags[i].name);

               status = db_set_data_index(fDB, hKey, buf, kLength, 1+i, TID_STRING);
               assert(status == DB_SUCCESS);
            }
         } else {
            cm_msg(MERROR, "add_event", "Error: db_find_key(%s) status %d", buf, status);
            return HS_FILE_ERROR;
         }
      }

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_define_event(const char* event_name, time_t timestamp, int ntags, const TAG tags[])
   {
      int event_id = FindEventId(event_name);
      if (event_id < 0)
         event_id = AllocateEventId(event_name);
      if (event_id < 0)
         return HS_FILE_ERROR;
      fEvidCache[event_name] = event_id;
      CreateOdbTags(event_id, event_name, ntags, tags);
      return ::hs_define_event(event_id, (char*)event_name, (TAG*)tags, ntags*sizeof(TAG));
   }

   /*------------------------------------------------------------------*/

   int hs_write_event(const char*  event_name, time_t timestamp, int data_size, const char* data)
   {
      int event_id = fEvidCache[event_name];
      //printf("write event [%s] evid %d\n", event_name, event_id);
      return ::hs_write_event(event_id, (void*)data, data_size);
   }

   /*------------------------------------------------------------------*/

   int hs_flush_buffers()
   {
      //printf("hs_flush_buffers!\n");
      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int GetEventsFromOdbEvents(std::vector<std::string> *events)
   {
      HNDLE hKeyRoot;
      int status;

      status = db_find_key(fDB, 0, "/History/Events", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }

      /* loop over tags to display event names */
      for (int i = 0;; i++) {
         HNDLE hKeyEq;
         char *s;
         char evname[1024+NAME_LENGTH];
         int size;
      
         status = db_enum_key(fDB, hKeyRoot, i, &hKeyEq);
         if (status != DB_SUCCESS)
            break;
    
         size = sizeof(evname);
         status = db_get_data(fDB, hKeyEq, evname, &size, TID_STRING);
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
      HNDLE hKeyRoot;
      int status;

      status = db_find_key(fDB, 0, "/History/Tags", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }
   
      /* loop over tags to display event names */
      for (int i = 0;; i++) {
         HNDLE hKeyEq;
         KEY key;
         char *s;
         WORD event_id;
         char evname[1024+NAME_LENGTH];
         int size;
      
         status = db_enum_key(fDB, hKeyRoot, i, &hKeyEq);
         if (status != DB_SUCCESS)
            break;
    
         /* get event name */
         db_get_key(fDB, hKeyEq, &key);
      
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
         status = db_get_data_index(fDB, hKeyEq, evname, &size, 0, TID_STRING);
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

   int hs_get_events(time_t t, std::vector<std::string> *pevents)
   {
      assert(pevents);
      pevents->clear();

      if (fEventsCache.size() == 0) {
         int status;

         if (fDebug)
            printf("hs_get_events: reading events list!\n");
         
         status = GetEventsFromOdbTags(&fEventsCache);

         if (status != HS_SUCCESS)
            status = GetEventsFromOdbEvents(&fEventsCache);

         if (status != HS_SUCCESS)
            return status;
      }

      for (unsigned i=0; i<fEventsCache.size(); i++)
         pevents->push_back(fEventsCache[i]);
         
      return HS_SUCCESS;
   }

   int GetEventIdFromHS(time_t ltime, const char* evname, const char* tagname)
   {
      HNDLE hKeyRoot;
      int status;

      status = db_find_key(fDB, 0, "/History/Events", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return -1;
      }

      for (int i = 0;; i++) {
         HNDLE hKey;
         KEY key;
         int  evid;
         char buf[256];
         int size;
         char *s;
         int ntags = 0;
         TAG* tags = NULL;
         char event_name[NAME_LENGTH];

         status = db_enum_key(fDB, hKeyRoot, i, &hKey);
         if (status != DB_SUCCESS)
            break;

         status = db_get_key(fDB, hKey, &key);
         assert(status == DB_SUCCESS);

         if (!isdigit(key.name[0]))
            continue;

         evid = atoi(key.name);

         assert(key.item_size < (int)sizeof(buf));

         size = sizeof(buf);
         status = db_get_data(fDB, hKey, buf, &size, TID_STRING);
         assert(status == DB_SUCCESS);

         strlcpy(event_name, buf, sizeof(event_name));

         s = strchr(buf,':');
         if (s)
            *s = 0;

         //printf("Found event %d, event [%s] name [%s], looking for [%s][%s]\n", evid, event_name, buf, evname, tagname);

         if (!equal_ustring((char *)evname, buf))
            continue;

         status = ::hs_get_tags((DWORD)ltime, evid, event_name, &ntags, &tags);

         for (int j=0; j<ntags; j++) {
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

      return -1;
   }

   int GetEventIdFromOdbTags(const char* evname, const char* tagname)
   {
      HNDLE hKeyRoot;
      int status;
      
      status = db_find_key(fDB, 0, "/History/Tags", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return -1;
      }

      for (int i = 0;; i++) {
         HNDLE hKey;
         KEY key;
         int evid;
         char buf[256];
         int size;
         char *s;

         status = db_enum_key(fDB, hKeyRoot, i, &hKey);
         if (status != DB_SUCCESS)
            break;

         status = db_get_key(fDB, hKey, &key);
         assert(status == DB_SUCCESS);

         if (key.type != TID_STRING)
            continue;

         if (!isdigit(key.name[0]))
            continue;

         evid = atoi(key.name);

         assert(key.item_size < (int)sizeof(buf));

         size = sizeof(buf);
         status = db_get_data_index(fDB, hKey, buf, &size, 0, TID_STRING);
         assert(status == DB_SUCCESS);

         s = strchr(buf,'/');
         if (s)
            *s = 0;

         //printf("Found event %d, name [%s], looking for [%s][%s]\n", evid, buf, evname, tagname);

         if (!equal_ustring((char *)evname, buf))
            continue;

         for (int j=1; j<key.num_values; j++) {
            size = sizeof(buf);
            status = db_get_data_index(fDB, hKey, buf, &size, j, TID_STRING);
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

      return -1;
   }

   int GetEventId(time_t t, const char* event_name, const char* tag_name, int *pevid)
   {
      int event_id = -1;

      if (fDebug)
         printf("xhs_event_id for event [%s], tag [%s]\n", event_name, tag_name);

      *pevid = 0;

      /* use "/History/Tags" if available */
      event_id = GetEventIdFromOdbTags(event_name, tag_name);
      
      /* if no Tags, use "/History/Events" and hs_get_tags() to read definition from history files */
      if (event_id < 0)
         event_id = GetEventIdFromHS(t, event_name, tag_name);

      /* if nothing works, use hs_get_event_id() */
      if (event_id <= 0) {
         DWORD evid = 0;
         int status = ::hs_get_event_id((DWORD)t, (char*)event_name, &evid);
         if (status != HS_SUCCESS)
            return status;
         event_id = evid;
      }
      
      if (event_id < 0)
         return HS_UNDEFINED_VAR;
      
      *pevid = event_id;
      
      return HS_SUCCESS;
   }
   
   int GetTagsFromHS(const char* event_name, std::vector<TAG> *ptags)
   {
      time_t now = time(NULL);
      int evid;
      int status = GetEventId(now, event_name, NULL, &evid);
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
      HNDLE hKeyRoot;
      int status;

      status = db_find_key(fDB, 0, "/History/Tags", &hKeyRoot);
      if (status != DB_SUCCESS) {
         return HS_FILE_ERROR;
      }
   
      /* loop over equipment to display event name */
      for (int i = 0;; i++) {
         HNDLE hKey;
         KEY key;
         WORD event_id;
         char buf[256];
         int size;
         char* s;
      
         status = db_enum_key(fDB, hKeyRoot, i, &hKey);
         if (status != DB_SUCCESS)
            break;
    
         /* get event name */
         status = db_get_key(fDB, hKey, &key);
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
            status = db_find_key(fDB, hKeyRoot, buf, &hKeyDir);
            if (status != DB_SUCCESS)
               continue;

            /* loop over tags */
            for (int j=0; ; j++) {
               HNDLE hKey;
               WORD array;
               int size;
               char var_name[NAME_LENGTH];
            
               status = db_enum_key(fDB, hKeyDir, j, &hKey);
               if (status != DB_SUCCESS)
                  break;
            
               /* get event name */
               status = db_get_key(fDB, hKey, &key);
               assert(status == DB_SUCCESS);
            
               array = 1;
               size  = sizeof(array);
               status = db_get_data(fDB, hKey, &array, &size, TID_WORD);
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
         status = db_get_data_index(fDB, hKey, buf, &size, 0, TID_STRING);
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
         for (int j=1; j<key.num_values; j++) {
            int array;
            int size;
            char var_name[NAME_LENGTH];
            int ev_type;
         
            size = sizeof(buf);
            status = db_get_data_index(fDB, hKey, buf, &size, j, TID_STRING);
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

   int hs_get_tags(const char* event_name, time_t t, std::vector<TAG> *ptags)
   {
      std::vector<TAG>& ttt = fTagsCache[event_name];

      if (ttt.size() == 0) {
         int status = HS_FILE_ERROR;

         if (fDebug)
            printf("hs_get_tags: reading tags for event [%s]\n", event_name);

         status = GetTagsFromOdb(event_name, &ttt);

         if (status != HS_SUCCESS)
            status = GetTagsFromHS(event_name, &ttt);

         if (status != HS_SUCCESS)
            return status;
      }

      for (unsigned i=0; i<ttt.size(); i++)
         ptags->push_back(ttt[i]);

      return HS_SUCCESS;
   }

   /*------------------------------------------------------------------*/

   int hs_get_last_written(time_t start_time, int num_var, const char* const event_name[], const char* const tag_name[], const int var_index[], time_t last_written[])
   {
      for (int i=0; i<num_var; i++)
         last_written[i] = 0;
      return HS_FILE_ERROR;
   }

   /*------------------------------------------------------------------*/

   int hs_read(time_t start_time, time_t end_time, time_t interval,
               int num_var,
               const char* const event_name[], const char* const tag_name[], const int var_index[],
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

         if (event_name[i]==NULL) {
            st[i] = HS_UNDEFINED_EVENT;
            num_entries[i] = 0;
            continue;
         }
         
         int status = GetEventId(start_time, event_name[i], tag_name[i], &event_id);
         
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
            }

         } while (status == HS_TRUNCATED);
        
         st[i] = status;

         time_t* x = (time_t*)malloc(n_point*sizeof(time_t));
         assert(x);
         double* y = (double*)malloc(n_point*sizeof(double));
         assert(y);

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

   /*------------------------------------------------------------------*/

   int hs_read2(time_t start_time, time_t end_time, time_t interval,
                int num_var,
                const char* const event_name[], const char* const tag_name[], const int var_index[],
                int num_entries[],
                time_t* time_buffer[],
                double* mean_buffer[],
                double* rms_buffer[],
                double* min_buffer[],
                double* max_buffer[],
                int read_status[])
   {
      int status = hs_read(start_time, end_time, interval, num_var, event_name, tag_name, var_index, num_entries, time_buffer, mean_buffer, read_status);

      for (int i=0; i<num_var; i++) {
         int num = num_entries[i];
         rms_buffer[i] = (double*)malloc(sizeof(double)*num);
         min_buffer[i] = (double*)malloc(sizeof(double)*num);
         max_buffer[i] = (double*)malloc(sizeof(double)*num);

         for (int j=0; j<num; j++) {
            rms_buffer[i][j] = 0;
            min_buffer[i][j] = mean_buffer[i][j];
            max_buffer[i][j] = mean_buffer[i][j];
         }
      }
      
      return status;
   }

   int hs_read_buffer(time_t start_time, time_t end_time,
                      int num_var, const char* const event_name[], const char* const tag_name[], const int var_index[],
                      MidasHistoryBufferInterface* buffer[],
                      int status[])
   {
      return HS_FILE_ERROR;
   }
   
   int hs_read_binned(time_t start_time, time_t end_time, int num_bins,
                      int num_var, const char* const event_name[], const char* const tag_name[], const int var_index[],
                      int num_entries[],
                      int* count_bins[], double* mean_bins[], double* rms_bins[], double* min_bins[], double* max_bins[],
                      time_t last_time[], double last_value[],
                      int status[])
   {
      return HS_FILE_ERROR;
   }

}; // end class

MidasHistoryInterface* MakeMidasHistory()
{
#if 0
   // midas history is a singleton class
   static MidasHistory* gh = NULL;
   if (!gh)
      gh = new MidasHistory;
   return gh;
#endif
   return new MidasHistory();
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

//
// mh2sql: import midas history into an SQL database
//
// Author: Konstantin Olchanski / TRIUMF, August-2008
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "midas.h"
#include "history.h"

#include <map>

std::map<int,const char*> gEventName;

bool print_empty = false;
bool print_dupe = false;
bool print_string = false;

bool had_empty = false;
bool had_dupe = false;
bool had_string = false;

bool report_empty = true;
bool report_dupe = true;
bool report_string = true;

int copyHstFile(const char* filename, FILE*f, MidasHistoryInterface *mh)
{
   assert(f!=NULL);

   char *buf = NULL;
   int bufSize = 0;

   while (1)
      {
         HIST_RECORD rec;

         int rd = fread(&rec, sizeof(rec), 1, f);
         if (!rd)
            break;

#if 0
         printf("HIST_RECORD:\n");
         printf("  Record type: 0x%x\n", rec.record_type);
         printf("  Event ID: %d\n", rec.event_id);
         printf("  Time: %d\n", rec.time);
         printf("  Offset: 0x%x\n", rec.def_offset);
         printf("  Size: 0x%x\n", rec.data_size);
#endif

         switch (rec.record_type)
            {
            default:
               fprintf(stderr, "Error: %s: Unexpected record type: 0x%x\n", filename, rec.record_type);
               if (buf)
                  free(buf);
               return -1;
               break;

            case 0x46445348: // RT_DEF:
               {
                  char event_name[NAME_LENGTH];
                  rd = fread(event_name, 1, NAME_LENGTH, f);

                  if (rd != NAME_LENGTH) {
                     fprintf(stderr, "Error: %s: Error reading RT_DEF record, fread(%d) returned %d, errno %d (%s)\n", filename, NAME_LENGTH, rd, errno, strerror(errno));
                     if (buf)
                        free(buf);
                     return -1;
                     break;
                  }
	    
                  int size = rec.data_size;
                  int ntags = size/sizeof(TAG);

                  //  printf("Event %d, \"%s\", size %d, %d tags.\n", rec.event_id, event_name, size, ntags);

                  if (!((size > 0) &&
                        (size < 1*1024*1024) &&
                        (size == ntags*(int)sizeof(TAG)))) {

                     fprintf(stderr, "Error: %s: Invalid length of RT_DEF record: %d (0x%x)\n", filename, size, size);
                     if (buf)
                        free(buf);
                     return -1;
                     break;
                  }
	    
                  TAG *tags = new TAG[ntags];
                  rd = fread(tags, 1, size, f);

                  if (rd != size) {
                     fprintf(stderr, "Error: %s: Error reading RT_DEF record, fread(%d) returned %d, errno %d (%s)\n", filename, size, rd, errno, strerror(errno));
                     if (buf)
                        free(buf);
                     return -1;
                     break;
                  }

                  // need to sanitize the tag names
                  
                  for (int i=0; i<ntags; i++) {
                     if (tags[i].type == TID_STRING) {
                        if (print_string)
                           fprintf(stderr, "Warning: %s: Event \"%s\": Fixed by truncation at forbidden TID_STRING tag \"%s\" at index %d\n", filename, event_name, tags[i].name, i);
                        ntags = i;
                        had_string = true;
                        break;
                     }

                     if (strlen(tags[i].name) < 1) {
                        sprintf(tags[i].name, "empty_%d", i);
                        if (print_empty)
                           fprintf(stderr, "Warning: %s: Event \"%s\": Fixed empty tag name for tag %d: replaced with \"%s\"\n", filename, event_name, i, tags[i].name);
                        had_empty = true;
                     }

                     for (int j=i+1; j<ntags; j++) {
                        if (strcmp(tags[i].name, tags[j].name) == 0) {
                           char str[256];
                           sprintf(str, "_dup%d", j);
                           strlcat(tags[j].name, str, sizeof(tags[j].name));
                           if (print_dupe)
                              fprintf(stderr, "Warning: %s: Event \"%s\": Fixed duplicate tag names: tag \"%s\" at index %d duplicate at index %d replaced with \"%s\"\n", filename, event_name, tags[i].name, i, j, tags[j].name);
                           had_dupe = true;
                        }
                     }
                  }

                  mh->hs_define_event(event_name, rec.time, ntags, tags);

                  gEventName[rec.event_id] = strdup(event_name);

                  delete tags;
                  break;
               }

            case 0x41445348: // RT_DATA:
               {
                  int size = rec.data_size;

                  if (0)
                     printf("Data record, size %d.\n", size);

                  if (!((size > 0) &&
                        (size < 1*1024*1024))) {

                     fprintf(stderr, "Error: %s: Invalid length of RT_DATA record: %d (0x%x)\n", filename, size, size);
                     if (buf)
                        free(buf);
                     return -1;
                     break;
                  }
	    
                  if (size > bufSize)
                     {
                        bufSize = 1024*((size+1023)/1024);
                        char *newbuf = (char*)realloc(buf, bufSize);

                        if (newbuf == NULL) {
                           fprintf(stderr, "Error: %s: Cannot realloc(%d), errno %d (%s)\n", filename, bufSize, errno, strerror(errno));
                           if (buf)
                              free(buf);
                           return -1;
                           break;
                        }

                        buf = newbuf;
                     }

                  rd = fread(buf, 1, size, f);

                  if (rd != size) {
                     fprintf(stderr, "Error: %s: Error reading RT_DATA record, fread(%d) returned %d, errno %d (%s)\n", filename, size, rd, errno, strerror(errno));
                     if (buf)
                        free(buf);
                     return -1;
                     break;
                  }

                  time_t t = (time_t)rec.time;

                  mh->hs_write_event(gEventName[rec.event_id], t, size, buf);
          
                  break;
               }
            }

         cm_msg_flush_buffer();
      }

   if (buf)
      free(buf);

   return 0;
}

int copyHst(const char* name, MidasHistoryInterface* mh)
{
   FILE* f = fopen(name,"r");
   if (!f)
      {
         fprintf(stderr, "Error: Cannot open \'%s\', errno %d (%s)\n", name, errno, strerror(errno));
         return -1;
      }

   fprintf(stderr, "Reading %s\n", name);
   copyHstFile(name, f, mh);
   fclose(f);
   mh->hs_flush_buffers();

   if (had_empty && report_empty) {
      report_empty = false;
      fprintf(stderr, "Notice: Automatically repaired some empty tag names\n");
   }

   if (had_dupe && report_dupe) {
      report_dupe = false;
      fprintf(stderr, "Notice: Automatically repaired some duplicate tag names\n");
   }

   if (had_string && report_string) {
      report_string = false;
      fprintf(stderr, "Notice: Automatically truncated events that contain tags of forbidden type TID_STRING\n");
   }

   return 0;
}

void help()
{
   fprintf(stderr,"Usage: mh2sql [-h] [switches...] file1.hst file2.hst ...\n");
   fprintf(stderr,"\n");
   fprintf(stderr,"Switches:\n");
   fprintf(stderr,"  -h --- print this help message\n");
   fprintf(stderr,"  --report-repaired-tags --- print messages about all repaired empty and duplicate tag names\n");
   fprintf(stderr,"  --hs-debug <hs_debug_flag> --- set the history debug flag\n");
   fprintf(stderr,"  --odbc <ODBC_DSN> --- write to ODBC (SQL) history using given ODBC DSN\n");
   fprintf(stderr,"  --sqlite <path> --- write to SQLITE database at the given path\n");
   fprintf(stderr,"  --mysql <connect string> --- write to MYSQL database\n");
   fprintf(stderr,"  --file <path> --- write to FILE database at the given path\n");
   fprintf(stderr,"\n");
   fprintf(stderr,"Examples:\n");
   fprintf(stderr,"  mh2sql --hs-debug 1 --sqlite . 130813.hst\n");
   exit(1);
}

int main(int argc,char*argv[])
{
   int status;
   int hs_debug_flag = 0;
   HNDLE hDB;
   MidasHistoryInterface *mh = NULL;

   if (argc <= 2)
      help(); // DOES NOT RETURN

   status = cm_connect_experiment("", "", "mh2sql", NULL);
   if (status != SUCCESS)
      exit(1);

   status = cm_get_experiment_database(&hDB, NULL);
   assert(status == HS_SUCCESS);

   // turn off watchdog checks
   cm_set_watchdog_params(FALSE, 0);

   for (int iarg=1; iarg<argc; iarg++) {
      const char* arg = argv[iarg];
      if (arg[0] != '-')
         continue;
      
      if (strcmp(arg, "-h")==0) {
         help(); // DOES NOT RETURN
      } else if (strcmp(arg, "--report-repaired-tags") == 0) {
         print_empty = true;
         print_dupe = true;
         print_string = true;
      } else if (strcmp(arg, "--hs-debug") == 0) {
         hs_debug_flag = atoi(argv[iarg+1]);
         iarg++;
      } else if (strcmp(arg, "--odbc") == 0) {
         mh = MakeMidasHistoryODBC();
         assert(mh);
         mh->hs_set_debug(hs_debug_flag);
         status = mh->hs_connect(argv[iarg+1]);
         if (status != HS_SUCCESS)
            exit(1);
         iarg++;
      } else if (strcmp(arg, "--sqlite") == 0) {
         mh = MakeMidasHistorySqlite();
         assert(mh);
         mh->hs_set_debug(hs_debug_flag);
         status = mh->hs_connect(argv[iarg+1]);
         if (status != HS_SUCCESS)
            exit(1);
         iarg++;
      } else if (strcmp(arg, "--mysql") == 0) {
         mh = MakeMidasHistoryMysql();
         assert(mh);
         mh->hs_set_debug(hs_debug_flag);
         status = mh->hs_connect(argv[iarg+1]);
         if (status != HS_SUCCESS)
            exit(1);
         iarg++;
      } else if (strcmp(arg, "--file") == 0) {
         mh = MakeMidasHistoryFile();
         assert(mh);
         mh->hs_set_debug(hs_debug_flag);
         status = mh->hs_connect(argv[iarg+1]);
         if (status != HS_SUCCESS)
            exit(1);
         iarg++;
      }
   }

   if (!mh) {
      status = hs_get_history(hDB, 0, HS_GET_DEFAULT|HS_GET_WRITER|HS_GET_INACTIVE, hs_debug_flag, &mh);
      assert(status == HS_SUCCESS);
      assert(mh);
   }
   
   for (int iarg=1; iarg<argc; iarg++) {
      const char* arg = argv[iarg];

      if (arg[0] != '-')
         continue;
      
      if (strcmp(arg, "--hs-debug") == 0) {
         mh->hs_set_debug(atoi(argv[iarg+1]));
         iarg++;
      }
   }

   for (int iarg=1; iarg<argc; iarg++) {
      const char* arg = argv[iarg];

      if (strstr(arg, ".hst") != NULL) {
         copyHst(arg, mh);
      }
   }

   mh->hs_disconnect();

   cm_disconnect_experiment();

   return 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

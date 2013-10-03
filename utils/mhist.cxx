 /********************************************************************\

  Name:         mhist.c
  Created by:   Stefan Ritt

  Contents:     MIDAS history display utility

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "history.h"

void tmp()
{
   time_t tm;

   time(&tm);
   tm = ss_time();
   hs_dump(1, (DWORD) tm - 3600, (DWORD) tm, 0, FALSE);
}

/*------------------------------------------------------------------*/

TAG temp_tag[] = {
   {"Temperatures", TID_FLOAT, 100},
   {"Humidity", TID_FLOAT, 99},
   {"Pressure1", TID_FLOAT, 1},
};

TAG hv_tag[] = {
   {"HV", TID_FLOAT, 100},
};

float hist[200];
float hv[100];
BOOL binary_time;
TAG tag[10];
void write_hist_speed()
/* write some history */
{
   DWORD start_time, act_time;
   int i, j, bytes;

   hs_define_event(1, "Temperature", temp_tag, sizeof(temp_tag));
   hs_define_event(2, "HV", hv_tag, sizeof(hv_tag));

   start_time = ss_millitime();
   j = bytes = 0;
   do {
      for (i = 0; i < 100; i++) {
         hist[i] = (float) j;
         hs_write_event(1, hist, sizeof(hist));
         hs_write_event(2, hv, sizeof(hv));
      }

      j += 2 * i;
      bytes += (sizeof(hist) + sizeof(hv)) * i;
      act_time = ss_millitime();

      printf("%d\n", ss_time());

   } while (act_time - start_time < 10000);

   printf("%d events (%d kB) per sec.\n", j / (act_time - start_time) * 1000,
          bytes / 1024 / (act_time - start_time) * 1000);
}

void generate_hist()
/* write some history */
{
   int i, j;

   hs_define_event(1, "Temperature", temp_tag, sizeof(temp_tag));
   hs_write_event(1, hist, sizeof(hist));

   hs_define_event(2, "HV", hv_tag, sizeof(hv_tag));
   hs_write_event(2, hv, sizeof(hv));

   for (i = 0; i < 10; i++) {
      hist[0] = (float) i;
      hist[1] = i / 10.f;
      hs_write_event(1, hist, sizeof(hist));

      for (j = 0; j < 100; j++)
         hv[j] = j + i / 10.f;
      hs_write_event(2, hv, sizeof(hv));
      printf("%d\n", ss_time());
      ss_sleep(1000);
   }
}

/*------------------------------------------------------------------*/
INT query_params(MidasHistoryInterface* mh, std::string* event_name, DWORD * start_time, DWORD * end_time,
                 DWORD * interval, char *var_name, DWORD * var_type,
                 INT * var_n_data, DWORD * index)
{
   DWORD status, hour;
   int var_index;

   std::vector<std::string> events;
   status = mh->hs_get_events(&events);

   if (status != HS_SUCCESS) {
      printf("hs_get_events() error %d\n", status);
      return status;
   }

   printf("Available events:\n");
   for (unsigned i = 0; i < events.size(); i++) {
      printf("%d: %s\n", i, events[i].c_str());
   }

   if (events.size() == 0)
      return HS_FILE_ERROR;
   else if (events.size() == 1)
      *event_name = events[0];
   else {
      int i;
      printf("\nSelect event: ");
      scanf("%d", &i);
      *event_name = events[i];
   }

   std::vector<TAG> tags;
   status = mh->hs_get_tags(event_name->c_str(), &tags);

   if (status != HS_SUCCESS) {
      printf("hs_get_tags() error %d\n", status);
      return status;
   }

   printf("\nAvailable variables:\n");

   for (unsigned j = 0; j < tags.size(); j++)
      if (tags[j].n_data > 1)
         printf("%d: %s[%d]\n", j, tags[j].name, tags[j].n_data);
      else
         printf("%d: %s\n", j, tags[j].name);
   
   *index = var_index = 0;
   if (tags.size() > 1) {
      printf("\nSelect variable (0..%d,-1 for all): ", (int)tags.size() - 1);
      scanf("%d", &var_index);
      if (var_index >= (int)tags.size())
         var_index = tags.size() - 1;
   } else {
      var_index = 0;
   }

   var_name[0] = 0;

   if (var_index >= 0) {
      strlcpy(var_name, tags[var_index].name, NAME_LENGTH);
      *var_type = tags[var_index].type;
      *var_n_data = tags[var_index].n_data;

      if (tags[var_index].n_data > 1 && tags[var_index].type != TID_STRING) {
         printf("\nSelect index (0..%d): ", tags[var_index].n_data - 1);
         scanf("%d", index);
      }
   }

   printf("\nHow many hours: ");
   scanf("%d", &hour);
   *end_time = ss_time();
   *start_time = (*end_time) - hour * 3600;

   printf("\nInterval [sec]: ");
   scanf("%d", interval);
   printf("\n");

   return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT file_display_vars(const char *file_name)
{
   DWORD status, i, j, bytes, n, nv, ltime, n_bytes, name_size, id_size;

   ltime = 0;
   if (file_name[0]) {
      struct tm tms;

      memset(&tms, 0, sizeof(tms));
      tms.tm_hour = 12;
      tms.tm_year = 10 * (file_name[0] - '0') + (file_name[1] - '0');
      if (tms.tm_year < 90)
         tms.tm_year += 100;
      tms.tm_mon = 10 * (file_name[2] - '0') + (file_name[3] - '0') - 1;
      tms.tm_mday = 10 * (file_name[4] - '0') + (file_name[5] - '0');
      ltime = (DWORD) mktime(&tms);
   }

   status = hs_count_events(ltime, &n);
   if (status != HS_SUCCESS)
      return status;

   name_size = n * NAME_LENGTH;
   id_size = n * sizeof(INT);
   char* event_name = (char*)malloc(name_size);
   INT* event_id = (INT*)malloc(id_size);
   hs_enum_events(ltime, event_name, (DWORD *) & name_size, event_id,
                  (DWORD *) & id_size);

   for (i = 0; i < n; i++) {
      printf("\nEvent ID %d: %s\n", event_id[i], event_name + i * NAME_LENGTH);
      hs_count_vars(ltime, event_id[i], &nv);
      bytes = nv * NAME_LENGTH;
      n_bytes = nv * sizeof(DWORD);
      char* var_names = (char*)malloc(bytes);
      DWORD* var_n = (DWORD*)malloc(nv * sizeof(DWORD));

      hs_enum_vars(ltime, event_id[i], var_names, &bytes, var_n, &n_bytes);
      for (j = 0; j < nv; j++)
         if (var_n[j] > 1)
            printf("%d: %s[%d]\n", j, var_names + j * NAME_LENGTH, var_n[j]);
         else
            printf("%d: %s\n", j, var_names + j * NAME_LENGTH);

      free(var_n);
      free(var_names);
   }

   free(event_name);
   free(event_id);

   return HS_SUCCESS;
}

INT display_vars(MidasHistoryInterface* mh)
{
   std::vector<std::string> events;
   int status = mh->hs_get_events(&events);

   if (status != HS_SUCCESS) {
      printf("hs_get_events() error %d\n", status);
      return status;
   }

   for (unsigned i = 0; i < events.size(); i++) {
      printf("\nEvent \'%s\'\n", events[i].c_str());

      std::vector<TAG> tags;
      status = mh->hs_get_tags(events[i].c_str(), &tags);

      if (status != HS_SUCCESS) {
         printf("hs_get_tags() error %d\n", status);
         continue;
      }

      for (unsigned j = 0; j < tags.size(); j++)
         if (tags[j].n_data > 1)
            printf("%d: %s[%d]\n", j, tags[j].name, tags[j].n_data);
         else
            printf("%d: %s\n", j, tags[j].name);
   }

   return HS_SUCCESS;
}

/*------------------------------------------------------------------*/
void display_single_hist(MidasHistoryInterface* mh, const char* event_name, time_t start_time, time_t end_time,
                         time_t interval, const char *var_name, int index)
/* read back history */
{
   time_t *tbuffer = NULL;
   double *buffer = NULL;
   int n;
   int status = 0;
   int hs_status = 0;

   status = mh->hs_read(start_time, end_time, interval, 1, &event_name, &var_name, &index, &n, &tbuffer, &buffer, &hs_status);

   if (status != HS_SUCCESS) {
      printf("hs_read() error %d\n", status);
      return;
   }

   if (hs_status != HS_SUCCESS) {
      printf("hs_read() cannot read event, status %d\n", hs_status);
      return;
   }

   if (n == 0)
      printf("No data for event \"%s\" variable \"%s\" found in specified time range\n", event_name, var_name);

   for (int i = 0; i < n; i++) {
      char str[256];

      if (binary_time)
         sprintf(str, "%d %.16g", (int)tbuffer[i], buffer[i]);
      else {
         sprintf(str, "%s %.16g", ctime(&tbuffer[i]) + 4, buffer[i]);
      }

      char* s = strchr(str, '\n');
      if (s)
         *s = ' ';

      printf("%s\n", str);
   }

   free(tbuffer);
   free(buffer);
}

/*------------------------------------------------------------------*/

void display_range_hist(MidasHistoryInterface* mh, const char* event_name, time_t start_time, time_t end_time,
                        time_t interval, const char *var_name, int index1, int index2)
/* read back history */
{
   const int kMaxVars = 50;
   time_t* tbuffer[kMaxVars];
   double* buffer[kMaxVars];
   const char* en[kMaxVars];
   const char* vn[kMaxVars];
   int in[kMaxVars];
   int hs_status[kMaxVars];
   char str[256];
   INT status = 0;
   time_t ltime;
   int n[kMaxVars];

   if (index2 > index1 + kMaxVars-1) {
      printf("Specified range too large (max %d)\n", kMaxVars);
      return;
   } else if (index2 < index1) {
      printf("Wrong specified range (low>high)\n");
      return;
   }

   int nvars = index2 - index1 + 1;

   for (int i=0; i<nvars; i++) {
      en[i] = event_name;
      vn[i] = var_name;
      in[i] = index1 + i;
      hs_status[i] = 0;
   }

   status = mh->hs_read(start_time, end_time, interval, nvars, en, vn, in, n, tbuffer, buffer, hs_status);
   
   if (status != HS_SUCCESS) {
      printf("Cannot read history data, hs_read() error %d\n", status);
      return;
   }
      
   for (int i=0; i<nvars; i++) {
      if (n[i] == 0)
         printf("No data for event \"%s\" variable \"%s\" index %d found in specified time range\n", event_name, var_name, index1+i);
      else if (hs_status[i] != HS_SUCCESS)
         printf("Cannot read event \"%s\" variable \"%s\" index %d, status %d\n", event_name, var_name, index1+i, hs_status[i]);
   }

   printf("mhist for Var:%s[%d:%d]\n", var_name, index1, index2);
   for (int i = 0; i < n[0]; i++) {
      if (binary_time)
         sprintf(str, "%d ", (int)tbuffer[0][i]);
      else {
         ltime = (time_t) tbuffer[0][i];
         sprintf(str, "%s", ctime(&ltime) + 4);
         str[20] = '\t';
      }

      for (int j = 0, idx = index1; idx < index2 + 1; idx++, j++) {
         strcat(str, "\t");
         sprintf(&str[strlen(str)], "%.16g", buffer[j][i]);
      }

      strcat(str, "\n");

      printf("%s", str);
   }

   for (int i=0; i<nvars; i++) {
      free(tbuffer[i]);
      free(buffer[i]);
   }
}

/*------------------------------------------------------------------*/

void display_all_hist(MidasHistoryInterface* mh, const char* event_name, time_t start_time, time_t end_time, time_t interval)
/* read back history */
{
   const int kMaxVars = 50;
   time_t* tbuffer[kMaxVars];
   double* buffer[kMaxVars];
   const char* en[kMaxVars];
   const char* vn[kMaxVars];
   int in[kMaxVars];
   int hs_status[kMaxVars];
   int n_data[kMaxVars];
   char str[256];
   INT status = 0;
   time_t ltime;
   int n[kMaxVars];

   std::vector<TAG> tags;
   status = mh->hs_get_tags(event_name, &tags);
   
   if (status != HS_SUCCESS) {
      printf("Cannot get list of variables for event \'%s\', hs_get_tags() error %d\n", event_name, status);
      return;
   }
   
   for (unsigned j = 0; j < tags.size(); j++) {
      if (tags[j].n_data > 1)
         printf("%d: %s[%d]\n", j, tags[j].name, tags[j].n_data);
      else
         printf("%d: %s\n", j, tags[j].name);
   }

   if (tags.size() > kMaxVars-1) {
      printf("Event \'%s\' has too many variables: %d, maximum is %d\n", event_name, (int)tags.size(), kMaxVars);
      return;
   }

   int nvars = 0;

   for (unsigned i=0; i<tags.size(); i++) {
      if (nvars >= kMaxVars-1) {
         printf("Event \'%s\' has too many variables or array elements: %d, maximum is %d\n", event_name, nvars, kMaxVars);
         break;
      }

      if (tags[i].n_data == 1) {
         en[nvars] = event_name;
         vn[nvars] = tags[i].name;
         in[nvars] = 0;
         hs_status[nvars] = 0;
         n_data[nvars] = tags[i].n_data;
         nvars++;
      } else {
         for (unsigned j=0; j<tags[i].n_data; j++) {
            if (nvars >= kMaxVars-1) {
               printf("Event \'%s\' has too many variables or array elements: %d, maximum is %d\n", event_name, nvars, kMaxVars);
               break;
            }
            en[nvars] = event_name;
            vn[nvars] = tags[i].name;
            in[nvars] = j;
            hs_status[nvars] = 0;
            n_data[nvars] = tags[i].n_data;
            nvars++;
         }
      }
   }

   assert(nvars < kMaxVars);

   status = mh->hs_read(start_time, end_time, interval, nvars, en, vn, in, n, tbuffer, buffer, hs_status);
   
   if (status != HS_SUCCESS) {
      printf("Cannot read history data, hs_read() error %d\n", status);
      return;
   }

   int nread = n[0];
      
   for (int i=0; i<nvars; i++) {
      if (n[i] == 0)
         printf("No data for event \"%s\" variable \"%s\" index %d found in specified time range\n", en[i], vn[i], in[i]);
      else if (hs_status[i] != HS_SUCCESS)
         printf("Cannot read event \"%s\" variable \"%s\" index %d, status %d\n", en[i], vn[i], in[i], hs_status[i]);
      else if (n[i] == nread)
         printf("Number of entries for event \"%s\" variable \"%s\" index %d is %d instead of %d\n", en[i], vn[i], in[i], n[i], nread);
   }

   printf("Event \'%s\', %d variables, %d entries\n", event_name, nvars, nread);

   printf("Time ");
   printf("\t");
   for (int i=0; i<nvars; i++) {
      if (n_data[i] == 1) {
         printf("%s ", vn[i]);
      } else {
         printf("%s[%d] ", vn[i], in[i]);
      }
      printf("\t");
   }
   printf("\n");

   for (int i = 0; i < nread; i++) {
      if (binary_time)
         sprintf(str, "%d ", (int)tbuffer[0][i]);
      else {
         ltime = (time_t) tbuffer[0][i];
         sprintf(str, "%s", ctime(&ltime) + 4);
         str[20] = '\t';
      }

      for (int j=0; j<nvars; j++) {
         strcat(str, "\t");
         sprintf(&str[strlen(str)], "%.16g", buffer[j][i]);
      }

      strcat(str, "\n");

      printf("%s", str);
   }

   for (int i=0; i<nvars; i++) {
      free(tbuffer[i]);
      free(buffer[i]);
   }
}

/*------------------------------------------------------------------*/
DWORD convert_time(char *t)
/* convert date in format YYMMDD[.HHMM[SS]] into decimal time */
{
   struct tm tms;
   INT isdst;

#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
   tzset();
#endif
#endif

   memset(&tms, 0, sizeof(tms));

   tms.tm_year = 10 * (t[0] - '0') + (t[1] - '0');
   tms.tm_mon = 10 * (t[2] - '0') + (t[3] - '0') - 1;
   tms.tm_mday = 10 * (t[4] - '0') + (t[5] - '0');
   if (tms.tm_year < 90)
      tms.tm_year += 100;
   if (t[6] == '.') {
      tms.tm_hour = 10 * (t[7] - '0') + (t[8] - '0');
      tms.tm_min = 10 * (t[9] - '0') + (t[10] - '0');
      if (t[11])
         tms.tm_sec = 10 * (t[11] - '0') + (t[12] - '0');
   }

   time_t ltime = mktime(&tms);

   /* correct for dst */
   memcpy(&tms, localtime(&ltime), sizeof(tms));

   isdst = tms.tm_isdst;
   memset(&tms, 0, sizeof(tms));
   tms.tm_isdst = isdst;

   tms.tm_year = 10 * (t[0] - '0') + (t[1] - '0');
   tms.tm_mon = 10 * (t[2] - '0') + (t[3] - '0') - 1;
   tms.tm_mday = 10 * (t[4] - '0') + (t[5] - '0');
   if (tms.tm_year < 90)
      tms.tm_year += 100;
   if (t[6] == '.') {
      tms.tm_hour = 10 * (t[7] - '0') + (t[8] - '0');
      tms.tm_min = 10 * (t[9] - '0') + (t[10] - '0');
      if (t[11])
         tms.tm_sec = 10 * (t[11] - '0') + (t[12] - '0');
   }

   ltime = (DWORD) mktime(&tms);

   return ltime;
}

int main(int argc, char *argv[])
{
   DWORD status, start_time = 0, end_time, interval, index1 = 0, index2 = 0;
   INT i, var_n_data;
   BOOL list_query;
   DWORD var_type;
   char var_name[NAME_LENGTH], file_name[256];
   char *p, path_name[256], path1_name[256];
   char start_name[64];
   char *column;
   BOOL do_hst_file = false;
   std::string event_name;
   int debug = 0;

   /* turn off system message */
   cm_set_msg_print(0, MT_ALL, puts);

   var_name[0] = 0;
   file_name[0] = 0;
   list_query = FALSE;
   path_name[0] = 0;
   path1_name[0] = 0;
   start_name[0] = 0;

   HNDLE hDB;
   char host_name[256];
   char expt_name[256];
   host_name[0] = 0;
   expt_name[0] = 0;

   MidasHistoryInterface* mh = NULL;

   if (argc == 1) {
      status = cm_connect_experiment1(host_name, expt_name, "mhist", 0, DEFAULT_ODB_SIZE, 0);
      assert(status == CM_SUCCESS);

      status = cm_get_experiment_database(&hDB, NULL);
      assert(status == CM_SUCCESS);

      status = hs_get_history(hDB, 0, HS_GET_DEFAULT|HS_GET_READER|HS_GET_INACTIVE, debug, &mh);
      assert(status == HS_SUCCESS);

      status = query_params(mh, &event_name, &start_time, &end_time, &interval, var_name, &var_type, &var_n_data, &index1);
      if (status != HS_SUCCESS)
         return 1;
   } else {
      /* at least one argument */
      end_time = ss_time();
      start_time = end_time - 3600;
      interval = 1;
      index1 = 0;
      index2 = 0;
      var_type = 0;
      event_name = "";
      binary_time = FALSE;

      for (i = 1; i < argc; i++) {
         if (argv[i][0] == '-' && argv[i][1] == 'b')
            binary_time = TRUE;
         else if (argv[i][0] == '-' && argv[i][1] == 'l')
            list_query = TRUE;
         else if (argv[i][0] == '-') {
            if (i + 1 >= argc || argv[i + 1][0] == '-')
               goto usage;
            if (strncmp(argv[i], "-e", 2) == 0)
               event_name = argv[++i];
            else if (strncmp(argv[i], "-v", 2) == 0)
               strcpy(var_name, argv[++i]);
            else if (strncmp(argv[i], "-i", 2) == 0) {
               if ((column = strchr(argv[++i], ':')) == NULL) {
                  index1 = atoi(argv[i]);
                  index2 = 0;
               } else {
                  *column = 0;
                  index1 = atoi(argv[i]);
                  index2 = atoi(column + 1);
               }
            } else if (strncmp(argv[i], "-h", 2) == 0)
               start_time = ss_time() - atoi(argv[++i]) * 3600;
            else if (strncmp(argv[i], "-d", 2) == 0)
               start_time = ss_time() - atoi(argv[++i]) * 3600 * 24;
            else if (strncmp(argv[i], "-s", 2) == 0) {
               strcpy(start_name, argv[++i]);
               start_time = convert_time(argv[i]);
               do_hst_file = true;
            } else if (strncmp(argv[i], "-p", 2) == 0)
               end_time = convert_time(argv[++i]);
            else if (strncmp(argv[i], "-t", 2) == 0)
               interval = atoi(argv[++i]);
            else if (strncmp(argv[i], "-f", 2) == 0) {
               strcpy(path_name, argv[++i]);
               do_hst_file = true;
            } else if (strncmp(argv[i], "-z", 2) == 0)
               strcpy(path1_name, argv[++i]);
         } else {
          usage:
            printf("\nusage: mhist [-e Event ID] [-v Variable Name]\n");
            printf("         [-i Index] index of variables which are arrays\n");
            printf("         [-i Index1:Index2] index range of variables which are arrays (max 50)\n");
            printf("         [-t Interval] minimum interval in sec. between two displayed records\n");
            printf("         [-h Hours] display between some hours ago and now\n");
            printf("         [-d Days] display between some days ago and now\n");
            printf("         [-f File] specify history file explicitly\n");
            printf("         [-s Start date] specify start date YYMMDD[.HHMM[SS]]\n");
            printf("         [-p End date] specify end date YYMMDD[.HHMM[SS]]\n");
            printf("         [-l] list available events and variables\n");
            printf("         [-b] display time stamp in decimal format\n");
            printf("         [-z path] path to the location of the history files (def:cwd)\n");
            return 1;
         }
      }
   }

   /*
      -z is needed in case the mhist called by script

      -f /path/file.hst
      -f file.hst -z /path         =>   file_name = file.hst
      path1_name= /path/
      -f /path/file.hst -z path1   =>   file_name = file.hst
      path1_name = path1
    */
   p = strrchr(path_name, DIR_SEPARATOR);
   if (p != NULL) {
      strcpy(file_name, p + 1);
      *(p + 1) = '\0';
      if (path1_name[0] == '\0')
         strcpy(path1_name, path_name);
   } else {
      strcpy(file_name, path_name);
      path_name[0] = '\0';
   }

   if (do_hst_file) {
      /* Set path */
      if (path1_name[0])
         hs_set_path(path1_name);

      /* -l listing only */
      if (list_query) {
         /* Overwrite the file_name if -s is given with -l only
            in order to produce the -l on the specified date */
         if (start_name[0] != 0)
            strcpy(file_name, start_name);
         
         file_display_vars(file_name);
      } else if (file_name[0]) {
         hs_fdump(file_name, atoi(event_name.c_str()), binary_time);
      }
      return 0;
   }

   if (!mh) {
      status = cm_connect_experiment1(host_name, expt_name, "mhist", 0, DEFAULT_ODB_SIZE, 0);
      assert(status == CM_SUCCESS);

      status = cm_get_experiment_database(&hDB, NULL);
      assert(status == CM_SUCCESS);

      status = hs_get_history(hDB, 0, HS_GET_DEFAULT|HS_GET_READER|HS_GET_INACTIVE, debug, &mh);
      if (status != HS_SUCCESS) {
         printf("hs_connect() error %d\n", status);
         return 1;
      }
   }

   /* -l listing only */
   if (list_query) {
      display_vars(mh);
   }
   /* -v given takes -e -s -p -t -b */
   else if (var_name[0] == 0) {
      // hs_dump(event_id, start_time, end_time, interval, binary_time);
      display_all_hist(mh, event_name.c_str(), start_time, end_time, interval);
   }
   /* Interactive variable display */
   else if (index2 == 0)
      display_single_hist(mh, event_name.c_str(), start_time, end_time, interval, var_name, index1);
   else
      display_range_hist(mh, event_name.c_str(), start_time, end_time, interval, var_name, index1, index2);

   cm_disconnect_experiment();

   return 0;
}

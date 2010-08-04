/********************************************************************\
Name:         lazylogger.c
Created by:   Pierre-Andre Amaudruz

Contents:     Disk to Tape copier for background job.

$Id$

\********************************************************************/
#include "midas.h"
#include "msystem.h"
#ifdef HAVE_YBOS
#include "ybos.h"
#endif
#ifdef HAVE_FTPLIB
#include "ftplib.h"
#endif
#include <assert.h>

#include <vector>
#include <string>
#include <algorithm>

#define NOTHING_TODO  0
#define FORCE_EXIT    1
#define EXIT_REQUEST  2
#define NEW_FILE      1
#define REMOVE_FILE   2
#define REMOVE_ENTRY  3
#define MAX_LAZY_CHANNEL 100
#define TRACE

#define LOG_TYPE_SCRIPT (-1)

#define STRLCPY(dst, src) strlcpy((dst), (src), sizeof(dst))

BOOL debug = FALSE;
BOOL nodelete = FALSE;

typedef struct {
   std::string filename;
   INT runno;
   double size;
} DIRLOG;

typedef std::vector<DIRLOG> DIRLOGLIST;

bool cmp_dirlog1(const DIRLOG&a, const DIRLOG&b)
{
   if (a.runno < b.runno)
      return true;

   if (a.runno > b.runno)
      return false;

   const char* sa = a.filename.c_str();
   const char* sb = b.filename.c_str();

   while (1) {
      if (*sa == 0) // sa is shorter
         return true;

      if (*sb == 0) // sb is shorter
         return false;

      //printf("cmp char %c %c\n", *sa, *sb);

      if (*sa < *sb)
         return true;

      if (*sa > *sb)
         return false;

      // at this point, *sa == *sb

      if (isdigit(*sa)) {
         int ia = strtoul(sa, (char**)&sa, 10);
         int ib = strtoul(sb, (char**)&sb, 10);

         //printf("cmp int %d %d\n", ia, ib);
      
         if (ia < ib)
            return true;

         if (ia > ib)
            return false;

         // at this point, ia == ib
         continue;
      }

      sa++;
      sb++;
   }

}

bool cmp_dirlog(const DIRLOG&a, const DIRLOG&b)
{
   bool r = cmp_dirlog1(a,b);
   //printf("compare %s %s yields %d\n", a.filename.c_str(), b.filename.c_str(), r);
   return r;
}

#ifndef MAX_FILE_PATH
#define MAX_FILE_PATH 128 // must match value of 128 in LAZY_SETTINGS_STRING
#endif

#define LAZY_SETTINGS_STRING "\
Period = INT : 10\n\
Maintain free space (%) = INT : 0\n\
Stay behind = INT : 0\n\
Alarm Class = STRING : [32]\n\
Running condition = STRING : [128] ALWAYS\n\
Data dir = STRING : [256] \n\
Data format = STRING : [8] MIDAS\n\
Filename format = STRING : [128] run%05d.mid\n\
Backup type = STRING : [8] Tape\n\
Execute after rewind = STRING : [64]\n\
Path = STRING : [128] \n\
Capacity (Bytes) = FLOAT : 5e9\n\
List label= STRING : [128] \n\
Execute before writing file = STRING : [64]\n\
Execute after writing file = STRING : [64]\n\
Modulo.Position = STRING : [8]\n\
Tape Data Append = BOOL : y\n\
"
#define LAZY_STATISTICS_STRING "\
Backup file = STRING : [128] none \n\
File size (Bytes) = DOUBLE : 0.0\n\
KBytes copied = DOUBLE : 0.0\n\
Total Bytes copied = DOUBLE : 0.0\n\
Copy progress (%) = DOUBLE : 0\n\
Copy Rate (Bytes per s) = DOUBLE : 0\n\
Backup status (%) = DOUBLE : 0\n\
Number of Files = INT : 0\n\
Current Lazy run = INT : 0\n\
"

typedef struct {
   INT period;                  /* time dela in lazy_main */
   INT pupercent;               /* 0:100 % of disk space to keep free */
   INT staybehind;              /* keep x run file between current Acq and backup
                                   -x same as x but starting from oldest */
   char alarm[32];              /* Alarm Class */
   char condition[128];         /* key condition */
   char dir[256];               /* path to the data dir */
   char format[8];              /* Data format (YBOS, MIDAS) */
   char backfmt[MAX_FILE_PATH]; /* format for the run files run%05d.mid */
   char type[8];                /* Backup device type  (Disk, Tape, Ftp, Script) */
   char command[64];            /* command to run after rewind */
   char path[MAX_FILE_PATH];    /* backup device name */
   float capacity;              /* backup device max byte size */
   char backlabel[MAX_FILE_PATH];       /* label of the array in ~/list. if empty like active = 0 */
   char commandBefore[64];      /* command to run Before writing a file */
   char commandAfter[64];       /* command to run After writing a file */
   char modulo[8];              /* Modulo for multiple lazy client */
   BOOL tapeAppend;             /* Flag for appending data to the Tape */
} LAZY_SETTING;
LAZY_SETTING lazy;

typedef struct {
   char backfile[MAX_FILE_PATH];        /* current or last lazy file done (for info only) */
   double file_size;            /* file size in bytes */
   double cur_size;             /* current bytes copied */
   double cur_dev_size;         /* Total bytes backup on device */
   double progress;             /* copy % */
   double copy_rate;            /* copy rate Kb/s */
   double bckfill;              /* backup fill % */
   INT nfiles;                  /* # of backuped files */
   INT cur_run;                 /* current or last lazy run number done (for info only) */
} LAZY_STATISTICS;
LAZY_STATISTICS lazyst;


typedef struct {
   HNDLE hKey;
   BOOL active;
   char name[32];
} LAZY_INFO;

LAZY_INFO lazyinfo[MAX_LAZY_CHANNEL] = { {0, FALSE, "Tape"} };

INT channel = -1;

/* Globals */
INT lazy_semaphore;
HNDLE hDB, hKey, pcurrent_hKey;
double lastsz;
HNDLE hKeyst;
INT run_state, hDev;
BOOL msg_flag;
BOOL copy_continue = TRUE;
INT data_fmt, dev_type;
char lazylog[MAX_STRING_LENGTH];
BOOL full_bck_flag = FALSE, maintain_touched = FALSE;
INT blockn = 0;

#define WATCHDOG_TIMEOUT 60000  /* 60 sec for tape access */

/* prototypes */
INT moduloCheck(INT lModulo, INT lPosition, INT lrun);
BOOL lazy_file_exists(char *dir, char *file);
INT lazy_main(INT, LAZY_INFO *);
INT lazy_copy(char *dev, char *file);
INT lazy_load_params(HNDLE hDB, HNDLE hKey);
INT build_log_list(const char *fmt, const char *dir, DIRLOGLIST *plog);
INT build_done_list_odb(HNDLE, INT **);
void lazy_settings_hotlink(HNDLE hDB, HNDLE hKey, void *info);
void lazy_maintain_check(HNDLE hKey, LAZY_INFO * pLall);

void print_dirlog(const DIRLOGLIST* dirlog)
{
   for (unsigned i=0; i<dirlog->size(); i++)
      printf("%d: %s, run %d, size %12.0f\n",
             i,
             (*dirlog)[i].filename.c_str(),
             (*dirlog)[i].runno,
             (*dirlog)[i].size);
};

/*------------------------------------------------------------------*/
INT lazy_run_extract(const char *name)
/********************************************************************\
Routine: lazy_run_extract
Purpose: extract the contigious digit at the right side from the 
string to make up a run number.
Input:
char * name    string to search
Output:
Function value:
INT        run number
0 if no extraction
\********************************************************************/
{
#if 0
   char run_str[256];
   int j;

   strlcpy(run_str, name, sizeof(run_str));
   if (strlen(run_str) < 2)
      return 0;

   for (j = strlen(run_str) - 1; j >= 0 && !isdigit(run_str[j]); j--)
      run_str[j] = 0;

   for (j = strlen(run_str) - 1; j >= 0 && isdigit(run_str[j]); j--);

   return atoi(run_str + j + 1);
#endif

   while (*name && !isdigit(*name))
      name++;

   if (*name == 0)
      return 0;

   return atoi(name);
}

/*------------------------------------------------------------------*/
INT lazy_file_remove(const char *pufile)
{
   INT fHandle, status;

   if (nodelete) {
      printf("lazy_file_remove: running in nodelete mode (-n switch), will not remove \'%s\'\n", pufile);
      return !SS_SUCCESS;
   }

   /* open device */
   fHandle = open(pufile, O_RDONLY, 0644);
   if (fHandle == -1)
      return SS_INVALID_NAME;

   close(fHandle);

   status = ss_file_remove((char*)pufile);
   if (status != 0)
      return SS_FILE_ERROR;
   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT lazy_log_update(INT action, INT run, const char *label, const char *file, DWORD perf_time)
{
   char str[MAX_FILE_PATH];
   INT blocks;

   strcpy(str, "no action");

   /* log Lazy logger to midas.log only */
   if (action == NEW_FILE) {
      /* keep track of number of file on that channel */
      lazyst.nfiles++;

      if (equal_ustring(lazy.type, "FTP"))
         sprintf(str, "%s: (cp:%.1fs) %s %1.3lfMB file COPIED",
                 label, (double) perf_time / 1000., lazyst.backfile, lazyst.file_size / 1024.0 / 1024.0);
      else if (equal_ustring(lazy.type, "Script")) {
         sprintf(str, "%s[%i] (cp:%.1fs) %s %1.3lfMB  file NEW",
                 label, lazyst.nfiles, (double) perf_time / 1000., lazyst.backfile, lazyst.file_size / 1024.0 / 1024.0);
      } else if (equal_ustring(lazy.type, "Disk")) {
         if (lazy.path[0] != 0)
            if (lazy.path[strlen(lazy.path) - 1] != DIR_SEPARATOR)
               strcat(lazy.path, DIR_SEPARATOR_STR);
         sprintf(str, "%s[%i] (cp:%.1fs) %s%s %1.3lfMB  file NEW",
                 label, lazyst.nfiles, (double) perf_time / 1000.,
                 lazy.path, lazyst.backfile, lazyst.file_size / 1024.0 / 1024.0);
      } else if (equal_ustring(lazy.type, "Tape")) {
         blocks = (int) (lazyst.cur_dev_size / 32.0 / 1024.0) + lazyst.nfiles;
         /* June 2002, use variable blockn from the real tape position */
         sprintf(str, "%s[%i] (cp:%.1fs) %s/%s %1.3lfMB  file NEW (position at block %d)",
                 label, lazyst.nfiles, (double) perf_time / 1000.,
                 lazy.path, lazyst.backfile, lazyst.file_size / 1024.0 / 1024.0, blockn);
         if (lazy.commandAfter[0]) {
            char cmd[256];
            sprintf(cmd, "%s %s %i %s/%s %1.3lf %d", lazy.commandAfter,
                    lazy.backlabel, lazyst.nfiles, lazy.path, lazyst.backfile,
                    lazyst.file_size / 1024.0 / 1024.0, blockn);
            cm_msg(MINFO, "Lazy", "Exec post file write script:%s", cmd);
            ss_system(cmd);
         }
      }
   } else if (action == REMOVE_FILE)
      sprintf(str, "%i (rm:%dms) %s file REMOVED", run, perf_time, file);

   else if (action == REMOVE_ENTRY)
      sprintf(str, "%s run#%i entry REMOVED", label, run);

#ifdef WRITE_MIDAS_LOG
   cm_msg(MINFO, "lazy_log_update", str);
#endif
   /* Now add this info also to a special log file */
   cm_msg1(MINFO, "lazy", "lazy_log_update", str);

   return 0;
}

/*------------------------------------------------------------------*/
INT moduloCheck(INT lModulo, INT lPosition, INT lrun)
/********************************************************************\
Routine: moduleCheck
Purpose: return valid run number in case of Modulo function enabled
or zero if not.
Input:
lModulo   : number of lazy channel
lPosition : Position of the current channel
lrun      : current run to test
Function value:
valid run number (0=skip run)
\********************************************************************/
{
   if (lModulo) {
      if ((lrun % lModulo) == lPosition)
         return lrun;
      else
         return 0;
   } else
      return lrun;
}

/*------------------------------------------------------------------*/
INT build_log_list(const char *fmt, const char *xdir, DIRLOGLIST *dlist)
/********************************************************************\
Routine: build_log_list
Purpose: build an internal directory file list from the disk directory
Input:
* fmt     format of the file to search for (ie:run%05d.ybs)
* dir     path to the directory for the search
Output:
**plog     internal file list struct
Function value:
number of elements
\********************************************************************/
{
   char str[MAX_FILE_PATH];
   char dir[MAX_FILE_PATH];
   char *dot;
   int lModulo = 0, lPosition = 0;

   strlcpy(dir, xdir, sizeof(dir));

   while (fmt) {
      /* substitue %xx by * */
      STRLCPY(str, fmt);

      fmt = strchr(fmt, ',');
      if (fmt)
         fmt++;

      char*s = strchr(str, ',');
      if (s)
         *s = 0;

      if (strchr(str, '%')) {
         *strchr(str, '%') = '*';
         if (strchr(str, '.'))
            strcpy((strchr(str, '*') + 1), strchr(str, '.'));
      }

      char *list = NULL;

      /* create dir listing with given criteria */
      int nfile = ss_file_find(dir, str, &list);

      /* check */
      /*
        for (j=0;j<nfile;j++)
        printf ("list[%i]:%s\n",j, list+j*MAX_STRING_LENGTH);
      */

      std::vector<std::string> flist;
      for (int j=0;j<nfile;j++)
         flist.push_back(list+j*MAX_STRING_LENGTH);

      free(list);

      /* Check Modulo option */
      if (lazy.modulo[0]) {
         /* Modulo enabled, extract modulo and position */
         dot = strchr(lazy.modulo, '.');
         if (dot) {
            *dot = '\0';
            lModulo = atoi(lazy.modulo);
            lPosition = atoi(dot + 1);
            *dot = '.';
         }
      }

      /* fill structure */
      for (unsigned j = 0, l = 0; j < flist.size(); j++) {
         INT lrun;
         /* extract run number */
         lrun = lazy_run_extract((char*)flist[j].c_str());
         /* apply the modulo if enabled */
         lrun = moduloCheck(lModulo, lPosition, lrun);
         /* if modulo enable skip */
         if (lrun == 0)
            continue;

         std::string s = dir;
         s += flist[j];

         DIRLOG d;
         d.filename = flist[j];
         d.runno = lrun;
         d.size = ss_file_size((char*)s.c_str());

         dlist->push_back(d);

         l++;
      }
   }

   sort(dlist->begin(), dlist->end(), cmp_dirlog);

   return dlist->size();
}

/*------------------------------------------------------------------*/
INT build_done_list_odb(HNDLE hLch, INT ** pdo)
/********************************************************************\
Routine: build_done_list
Purpose: build a sorted internal /lazy/list list (pdo) tree.
Input:
HNDLE             Key of the Lazy channel
**pdo     /lazy_xxx/list run listing
Output:
**pdo     /lazy_xxx/list run listing
Function value:      number of elements
\********************************************************************/
{
   HNDLE hKey, hSubkey;
   KEY key;
   INT i, j, size, tot_nelement, nelement, temp;

   if (db_find_key(hDB, hLch, "List", &hKey) != DB_SUCCESS) {
      return 0;
   }

   tot_nelement = 0;
   for (i = 0;; i++) {
      db_enum_key(hDB, hKey, i, &hSubkey);
      if (!hSubkey)
         break;
      db_get_key(hDB, hSubkey, &key);
      nelement = key.num_values;
      *pdo = (INT*)realloc(*pdo, sizeof(INT)*(tot_nelement + nelement));
      size = nelement * sizeof(INT);
      db_get_data(hDB, hSubkey, (char *) (*pdo + tot_nelement), &size, TID_INT);
      tot_nelement += nelement;
   }

   if (0) {
      printf("read pdo: %d\n", tot_nelement);
      for (i=0; i<tot_nelement; i++)
         printf("%d: %d\n", i, (*pdo)[i]);
   }

   /* expand compressed run numbers */
   for (i=0; i<tot_nelement; i++)
      if ((*pdo)[i] < 0) {
         int first =  (*pdo)[i-1];
         int last = -(*pdo)[i];
         int nruns = last - first + 1;
         assert(nruns > 1);

         *pdo = (INT*)realloc(*pdo, sizeof(INT)*(tot_nelement + nruns - 2));
         assert(*pdo != NULL);

         memmove((*pdo) + i + nruns -1, (*pdo) + i + 1, sizeof(INT)*(tot_nelement - i - 1));

         for (j=1; j<nruns; j++)
            (*pdo)[i+j-1] = first + j;

         tot_nelement += nruns - 2;
      }

   if (0) {
      printf("uncompressed pdo: %d\n", tot_nelement);
      for (i=0; i<tot_nelement; i++)
         printf("%d: %d\n", i, (*pdo)[i]);
   }

   /* sort array of integers */
   for (j = 0; j < tot_nelement - 1; j++) {
      for (i = j + 1; i < tot_nelement; i++) {
         if (*(*pdo + j) > *(*pdo + i)) {
            memcpy(&temp, (*pdo + i), sizeof(INT));
            memcpy((*pdo + i), (*pdo + j), sizeof(INT));
            memcpy((*pdo + j), &temp, sizeof(INT));
         }
      }
   }

   if (0) {
      printf("sorted pdo: %d\n", tot_nelement);
      for (i=0; i<tot_nelement; i++)
         printf(" %d\n", (*pdo)[i]);
   }

   return tot_nelement;
}

std::string list_filename(const char* lazyname, const char* listname)
{
   char path[256];
   cm_get_path(path);

   std::string s;
   s += path;
   s += ".Lazy_";
   s += lazyname;
   s += ".";
   s += listname;
   return s;
}

void load_done_list(const char* lazyname, const DIRLOGLIST* dirlist, DIRLOGLIST* dlist)
{
   FILE *fp = fopen(list_filename(lazyname, "donelist").c_str(), "r");
   if (!fp)
      return;

   while (1) {
      char str[256];
      char* s = fgets(str, sizeof(str), fp);
      if (!s)
         break;

      DIRLOG d;

      char* p = strchr(s, ' ');

      if (p) {
         *p = 0;
         p++;
      }
      
      d.filename = s;
      d.runno = strtoul(p, &p, 0);
      d.size = strtod(p, &p);

      bool found = false;
      if (dirlist) {
         for (unsigned i=0; i<dirlist->size(); i++)
            if ((*dirlist)[i].filename == d.filename) {
               found = true;
               break;
            }
      }
      else
         found = true;

      if (found)
         dlist->push_back(d);
   }

   fclose(fp);

   sort(dlist->begin(), dlist->end(), cmp_dirlog);
}

int save_list(const char* lazyname, const char* listname, const DIRLOGLIST* dlist)
{
   std::string fname = list_filename(lazyname, listname);
   std::string tmpname = fname + ".tmp";

   FILE *fp = fopen(tmpname.c_str(), "w");
   if (!fp) {
      cm_msg(MERROR, "save_list", "Cannot write to \'%s\', errno %d (%s)",
             tmpname.c_str(),
             errno,
             strerror(errno));
      return !SUCCESS;
   }

   for (unsigned i=0; i<dlist->size(); i++) {
      const char* s = (*dlist)[i].filename.c_str();
      const char* p;
      while ((p = strchr(s, DIR_SEPARATOR)))
         s = p+1;
      fprintf(fp, "%s %d %12.0f\n",
              s,
              (*dlist)[i].runno,
              (*dlist)[i].size
              );
   }

   fclose(fp);

   int status = rename(tmpname.c_str(), fname.c_str());
   if (status) {
      cm_msg(MERROR, "save_list", "Cannot rename \'%s\' to \'%s\', errno %d (%s)",
             tmpname.c_str(),
             fname.c_str(),
             errno,
             strerror(errno));
      return !SUCCESS;
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/
void convert_done_list(HNDLE hLch)
/********************************************************************\
Routine: convert_done_list
Purpose: build a sorted internal /lazy/list list (pdo) tree.
Input:
HNDLE             Key of the Lazy channel
**pdo     /lazy_xxx/list run listing
Output:
**pdo     /lazy_xxx/list run listing
Function value:      number of elements
\********************************************************************/
{
   DIRLOGLIST dlist;
   build_log_list(lazy.backfmt, lazy.dir, &dlist);

   INT *pdo = NULL;
   int n = build_done_list_odb(hLch, &pdo);

   DIRLOGLIST done;

   for (int i=0; i<n; i++)
      for (unsigned j=0; j<dlist.size(); j++)
         if (dlist[j].runno == pdo[i])
            done.push_back(dlist[j]);

   free(pdo);

   KEY key;
   int status = db_get_key(hDB, hLch, &key);
   assert(status == DB_SUCCESS);

   save_list(key.name, "donelist", &done);
}

/*------------------------------------------------------------------*/
INT build_done_list(HNDLE hLch, const DIRLOGLIST *pdirlist, DIRLOGLIST *pdone)
/********************************************************************\
Routine: build_done_list
Purpose: build a sorted internal /lazy/list list (pdo) tree.
Input:
HNDLE             Key of the Lazy channel
**pdo     /lazy_xxx/list run listing
Output:
**pdo     /lazy_xxx/list run listing
Function value:      number of elements
\********************************************************************/
{
   KEY key;
   int status = db_get_key(hDB, hLch, &key);
   assert(status == DB_SUCCESS);

   load_done_list(key.name, pdirlist, pdone);

   if (pdone->size() == 0) {
      convert_done_list(hLch);
      load_done_list(key.name, pdirlist, pdone);
   }

   return pdone->size();
}

/*------------------------------------------------------------------*/
INT save_done_list(HNDLE hLch, DIRLOGLIST *pdone)
/********************************************************************\
Routine: save_done_list
Purpose: save a sorted internal /lazy/list list (pdo) tree.
Input:
HNDLE             Key of the Lazy channel
**pdo     /lazy_xxx/list run listing
Output:
**pdo     /lazy_xxx/list run listing
Function value:      number of elements
\********************************************************************/
{
   KEY key;
   int status = db_get_key(hDB, hLch, &key);
   assert(status == DB_SUCCESS);
   sort(pdone->begin(), pdone->end(), cmp_dirlog);
   save_list(key.name, "donelist", pdone);
   return SUCCESS;
}

/*------------------------------------------------------------------*/
int find_next_file(const DIRLOGLIST *plog, const DIRLOGLIST *pdone)
/********************************************************************\
Routine: find_next_file
Purpose: find next file to be backed up and return it's index in plog
Input:
*plog :   disk file listing
*pdone :  list of files already backed up
Output:
Function value:
index into plog
-1       : no files

\********************************************************************/
{
   for (unsigned j = 0; j < plog->size(); j++) {
      bool found = false;
      for (unsigned i = 0; i < pdone->size(); i++) {
         if ((*plog)[j].filename == (*pdone)[i].filename)
            found = true;
      }

      if (!found)
         if ((*plog)[j].size > 0)
            return j;
   }

   return -1;
}

/*------------------------------------------------------------------*/
INT lazy_select_purge(HNDLE hKey, INT channel, LAZY_INFO * pLall, const char* fmt, const char* dir, DIRLOG *f)
                      /********************************************************************\
                      Routine: lazy_select_purge
                      Purpose: Search oldest run number which can be purged
                      condition : oldest run# in (pdo AND present in plog)
                      AND scan all the other channels based on the following
                      conditions:
                      "data dir" && "filename format" are the same &&
                      the /list/run_number exists in all the above condition.
                      Input:
                      hKey      : Current channel key
                      channel   : Current channel number
                      *pLall    : Pointer to all channels
                      fmt       : Format string
                      dir       : Directory string
                      Output:
                      fpufile   : file to purge
                      run       : corresponding run# to be purged
                      Function value:
                      0          success
                      -1         run not found
                      \********************************************************************/
{
   int size;
   char ddir[256], ff[128], strmodulo[8];

   /* Scan donelist from first element (oldest)
      check if run exists in dirlog
      if yes return file and run number */

   while (1) {
      /* try to find the oldest matching run present in the list AND on disk */
      /* build current done list */

      DIRLOGLIST dirlists[MAX_LAZY_CHANNEL];
      DIRLOGLIST donelists[MAX_LAZY_CHANNEL];

      /* build matching dir and file format (ff) */
      for (int i = 0; i < MAX_LAZY_CHANNEL; i++) {
         /* Check if key present && key different than currrent
            and if modulo is off */
         if (((pLall + i)->hKey)) {
            /* extract dir and ff */
            size = sizeof(strmodulo);
            db_get_value(hDB, (pLall + i)->hKey, "Settings/Modulo.Position", strmodulo, &size, TID_STRING, TRUE);
            if (strmodulo[0]) {
               /* Modulo enabled, skip this channel */
               if (debug)
                  printf("purge: skipping channel \'%s\' which uses modulo \'%s\'\n", lazyinfo[i].name, strmodulo);
               continue;
            }

            size = sizeof(ddir);
            db_get_value(hDB, (pLall + i)->hKey, "Settings/Data dir", ddir, &size, TID_STRING, TRUE);
            size = sizeof(ff);
            db_get_value(hDB, (pLall + i)->hKey, "Settings/filename format", ff, &size, TID_STRING, TRUE);

            // monkey about with trailing '/' characters

            int len = strlen(ddir);
            if (ddir[len-1] != '/') {
               ddir[len] = '/';
               ddir[len+1] = 0;
            }

            /* if same dir && same ff => mark lazy channel */
            if (strcmp(ddir, dir) != 0) {
               if (debug)
                  printf("purge: skipping channel \'%s\' which uses different data dir \'%s\', we use \'%s\'\n", lazyinfo[i].name, ddir, dir);
               continue;
            }

            if (debug)
               printf("Loading file lists for channel \'%s\', data dir \'%s\', format \'%s\'\n", lazyinfo[i].name, ddir, ff);

            /* load file list and done list for matching channels */
            build_log_list(ff, ddir, &dirlists[i]);
            build_done_list(pLall[i].hKey, &dirlists[i], &donelists[i]);
         }
      } /* end of loop over channels */

      /* search for files to delete */
      for (unsigned k = 0; k < dirlists[channel].size(); k++) {
         bool can_delete = true;
      
         if (debug)
            printf("purge: consider file \'%s\'\n", dirlists[channel][k].filename.c_str());

         for (int i = 0; i < MAX_LAZY_CHANNEL; i++)
            if (((pLall + i)->hKey)) {
               bool in_dir_list = false;
               bool in_done_list = false;

               for (unsigned j=0; j<dirlists[i].size(); j++)
                  if (dirlists[i][j].filename == dirlists[channel][k].filename) {
                     in_dir_list = true;
                     break;
                  }

               if (!in_dir_list) {
                  if (debug)
                     printf("channel \'%s\': file is not in dir list, ok to delete\n", lazyinfo[i].name);
                  continue;
               }

               for (unsigned j=0; j<donelists[i].size(); j++)
                  if (donelists[i][j].filename == dirlists[channel][k].filename) {
                     in_done_list = true;
                     break;
                  }

               if (!in_done_list) {
                  if (debug)
                     printf("channel \'%s\': file is in dirlist but not in done list, cannot delete\n", lazyinfo[i].name);
                  can_delete = false;
                  break;
               }

               if (debug)
                  printf("channel \'%s\': file is in dirlist and in done list, ok to delete\n", lazyinfo[i].name);
            }

         if (can_delete) {
            if (debug)
               printf("purge: file \'%s\' is done by all channels\n", dirlists[channel][k].filename.c_str());
            *f = dirlists[channel][k];
            return SUCCESS;
         }
      }

      /* nothing to remove */
      return !SUCCESS;
   }
}

/*------------------------------------------------------------------*/
void lazy_settings_hotlink(HNDLE hDB, HNDLE hKey, void *info)
{
   INT size, maintain;

   /* check if Maintain has been touched */
   size = sizeof(maintain);
   db_get_value(hDB, hKey, "Maintain free space (%)", &maintain, &size, TID_INT, TRUE);
   if (maintain != 0)
      maintain_touched = TRUE;
   else
      maintain_touched = FALSE;
}

/*------------------------------------------------------------------*/
void lazy_maintain_check(HNDLE hKey, LAZY_INFO * pLall)
/********************************************************************\
Routine: lazy_maintain_check
Purpose: Check if the "maintain free space" of any of the other matching
channels is != 0. matching correspond to the condition:
"data dir" && "filename format"
if found != 0 then set to 0 and inform
Input:
hKey   :   Current lazy channel key
*pLall :   Pointer to all channels
\********************************************************************/
{
   INT size;
   INT i, maintain;
   char dir[256], ff[128];
   char cdir[256], cff[128];
   char cmodulostr[8], modulostr[8];

   /* do test only if maintain has been touched */
   if (!maintain_touched)
      return;
   maintain_touched = FALSE;

   /* check is Maintain free is enabled */
   size = sizeof(maintain);
   db_get_value(hDB, hKey, "Settings/Maintain free space (%)", &maintain, &size, TID_INT, TRUE);
   if (maintain != 0) {
      /* scan other channels */

      /* get current specification */
      size = sizeof(cdir);
      db_get_value(hDB, hKey, "Settings/Data dir", cdir, &size, TID_STRING, TRUE);
      size = sizeof(cff);
      db_get_value(hDB, hKey, "Settings/filename format", cff, &size, TID_STRING, TRUE);
      size = sizeof(modulostr);
      db_get_value(hDB, hKey, "Settings/Modulo.Position", cmodulostr, &size, TID_STRING, TRUE);

      /* build matching dir and ff */
      for (i = 0; i < MAX_LAZY_CHANNEL; i++) {
         if (((pLall + i)->hKey) && (hKey != (pLall + i)->hKey)) {      /* valid channel */
            size = sizeof(dir);
            db_get_value(hDB, (pLall + i)->hKey, "Settings/Data dir", dir, &size, TID_STRING, TRUE);
            size = sizeof(ff);
            db_get_value(hDB, (pLall + i)->hKey, "Settings/filename format", ff, &size, TID_STRING, TRUE);
            size = sizeof(modulostr);
            db_get_value(hDB, (pLall + i)->hKey, "Settings/Modulo.Position", modulostr, &size, TID_STRING, TRUE);


            if ((strcmp(dir, cdir) == 0) && (strcmp(ff, cff) == 0) && !cmodulostr[0]
                && !modulostr[0]) {
               /* check "maintain free space" */
               size = sizeof(maintain);
               db_get_value(hDB, (pLall + i)->hKey, "Settings/Maintain free space (%)",
                            &maintain, &size, TID_INT, TRUE);
               if (maintain) {
                  /* disable and inform */
                  size = sizeof(maintain);
                  maintain = 0;
                  db_set_value(hDB, (pLall + i)->hKey, "Settings/Maintain free space (%)", &maintain, size, 1, TID_INT);
                  cm_msg(MINFO, "lazy_maintain_check",
                         "Maintain free space on channel %s has been disable", (pLall + i)->name);
               }
            }
         }
      }
   }
}

/*------------------------------------------------------------------*/
void lazy_statistics_update(INT cploop_time)
/********************************************************************\
Routine: lazy_statistics_update
Purpose: update the /lazy/statistics
Input:
INT cploop_time : time of the last call
Output:
Function value:
0           success
\********************************************************************/
{
   /* update rate [kb/s] statistics */
   if (cploop_time == 0)        // special function: set copy_rate to zero at end of copy
      lazyst.copy_rate = 0.0;
   if ((ss_millitime() - cploop_time) > 100)
      lazyst.copy_rate = 1000.f * (lazyst.cur_size - lastsz) / (ss_millitime() - cploop_time);
   if (lazyst.copy_rate < 0.0)
      lazyst.copy_rate = 0.0;

   /* update % statistics */
   if (lazyst.file_size != 0.0f)
      lazyst.progress = (100.0f * (lazyst.cur_size / lazyst.file_size));
   else
      lazyst.progress = 0.0f;

   /* update backup % statistics */
   if (lazy.capacity > 0.0f)
      lazyst.bckfill = 100.0f * (lazyst.cur_dev_size / lazy.capacity);
   else
      lazyst.bckfill = 0.0f;

   lastsz = lazyst.cur_size;
   /* udate ODB statistics */
   db_send_changed_records();
}

/*------------------------------------------------------------------*/
BOOL condition_test(char *string)
/********************************************************************\
Routine: condition_check
Purpose: return TRUE or FALSE depending if condition is satisfied
I expect a statement of the following form:
<key> <operator> <value>
with <key> : either link or key[index] or key(index)
<operator> : either: = < >
<value> : [+/-][digits]
Input: char * string to decode and test
Output:
Function value:
TRUE           condition TRUE  (carry on the copy)
FALSE          condition FALSE (hold the copy)
\********************************************************************/
{
   KEY key;
   double value;
   double lcond_value;
   INT size, index, status;
   char str[128], left[64], right[64];
   char *p = NULL, *pp, *ppl, *pc, *lp;

   index = 0;
   p = string;
   if (p) {
      while (isspace(*p))
         p++;

      pc = strpbrk(p, "<=>");
      if (pc) {
         strncpy(left, p, pc - p);
         lp = left + (pc - p - 1);
         while (isspace(*lp))
            lp--;
         *(lp + 1) = '\0';
         strncpy(right, pc + 1, (strlen(p) - strlen(left)));
         right[strlen(p) - strlen(left) - 1] = '\0';
      }

      if ((pp = strpbrk(left, "[(")) != NULL) {
         if ((ppl = strpbrk(left, "])")) != NULL) {
            *pp = '\0';
            *ppl = '\0';
            index = atoi(pp + 1);
         }
         *pp = '\0';
      }

      /* convert value */
      value = (double) (atoi(right));

      status = db_find_key(hDB, 0, left, &hKey);
      if (status != DB_SUCCESS) {
         cm_msg(MINFO, "condition_check", "Key %s not found", left);
         return FALSE;
      }
      status = db_get_key(hDB, hKey, &key);
      if ((status == DB_SUCCESS) && (key.type <= TID_DOUBLE)) {
         /* get value of the condition */
         size = sizeof(lcond_value);
         db_get_data_index(hDB, hKey, &lcond_value, &size, index, key.type);
         db_sprintf(str, &lcond_value, key.item_size, 0, key.type);
         lcond_value = atof(str);
      }

      /*  printf("string:%s\n condition: %s %f %c %f \n"
         , string, left, lcond_value, *pc, value);
       */
      /*
         if (pv == NULL)
         return TRUE;
       */
      /* perform condition check */
      if (((*pc == '>') && ((double) lcond_value > value)) ||
          ((*pc == '=') && ((double) lcond_value == value)) || ((*pc == '<') && ((double) lcond_value < value)))
         return TRUE;
      else
         return FALSE;
   }
   /* catch wrong argument in the condition as TRUE */
   return TRUE;
}

/*------------------------------------------------------------------*/
BOOL lazy_condition_check(void)
/********************************************************************\
Routine: lazy_condition_check
Purpose: Check if the copy should continue.
The condition can have the following setting:
ALWAYS, NEVER, WHILE_NO_ACQ_RUNNING, key<=>value
Input:
Output:
Function value:
BOOL :  new copy condition
\********************************************************************/
{
   /* Get condition */
   if (equal_ustring(lazy.condition, "ALWAYS"))
      return TRUE;
   else if (equal_ustring(lazy.condition, "NEVER"))
      return FALSE;
   else if (equal_ustring(lazy.condition, "WHILE_ACQ_NOT_RUNNING")) {
      if (run_state == STATE_RUNNING)
         return FALSE;
      else
         return TRUE;
   } else
      return (condition_test(lazy.condition));
}

/*------------------------------------------------------------------*/
INT lazy_copy(char *outfile, char *infile)
/********************************************************************\
Routine: lazy_copy
Purpose: backup file to backup device
every 2 second will update the statistics and yield
if condition requires no copy, every 5 second will yield
Input:
char * outfile   backup destination file
char * infile    source file to be backed up
Output:
Function value:
0           success
\********************************************************************/
{
   void *plazy = NULL;
   DWORD szlazy;
   INT status, no_cpy_last_time = 0;
   INT last_time, cpy_loop_time;
   DWORD watchdog_timeout;
   static INT last_error = 0;
   //char *pext;
   BOOL watchdog_flag, exit_request = FALSE;
   char filename[256];

#ifndef HAVE_YBOS
   assert(!"YBOS support is not compiled in. Please use the \'SCRIPT\' backup type method");
#else

   // SR Nov 07, outcommented for "raw" .gz transfer
   //pext = malloc(strlen(infile));
   //strcpy(pext, infile);

   /* find out what format it is from the extension. */
   //if (strncmp(pext + strlen(pext) - 3, ".gz", 3) == 0) {
   /* extension .gz terminator on . */
   // *(pext + strlen(pext) - 3) = '\0'; SR Nov 07
   //}

   /* init copy variables */
   lazyst.cur_size = 0.0f;
   last_time = 0;

   /* open any logging file (output) */
   strlcpy(filename, outfile, sizeof(filename));        // ftp modifies filename
   if ((status = yb_any_file_wopen(dev_type, data_fmt, filename, &hDev)) != 1) {
      if ((ss_time() - last_error) > 60) {
         last_error = ss_time();
         cm_msg(MTALK, "Lazy_copy", "cannot open %s, error %d", outfile, status);
      }
      return (FORCE_EXIT);
   }

   /* New lazy copy if TAPE & append required force a mv to EOD */
   if ((dev_type == LOG_TYPE_TAPE) && lazy.tapeAppend) {
      /* Position Tape to end of Data */
      cm_msg(MINFO, "Lazy", "Positioning Tape to EOD");

      cm_get_watchdog_params(&watchdog_flag, &watchdog_timeout);
      cm_set_watchdog_params(watchdog_flag, 300000);    /* 5 min for tape rewind */
      status = ss_tape_spool(hDev);
      cm_set_watchdog_params(watchdog_flag, watchdog_timeout);
      if (status != SS_SUCCESS) {
         cm_msg(MINFO, "Lazy", "Error while Positioning Tape to EOD (%d)", status);
         ss_tape_close(hDev);
         return (FORCE_EXIT);
      }
   }

   /* reset error message */
   last_error = 0;

   /* open input data file */
   if (yb_any_file_ropen(infile, data_fmt) != YB_SUCCESS)
      return (FORCE_EXIT);

   /* run shell command if available */
   if (equal_ustring(lazy.type, "Tape")) {
      /* get the block number. If -1 it probably means that the tape
         is not ready. Wait for a while  */
      blockn = -1;
      while (blockn < 0) {
         blockn = ss_tape_get_blockn(hDev);
         if (blockn >= 0)
            break;
         cm_msg(MINFO, "Lazy", "Tape is not ready");
         cm_yield(3000);
      }
      if (lazy.commandBefore[0]) {
         char cmd[256];
         sprintf(cmd, "%s %s %s %d %s %i", lazy.commandBefore, infile, outfile, blockn, lazy.backlabel, lazyst.nfiles);
         cm_msg(MINFO, "Lazy", "Exec pre file write script:%s", cmd);
         ss_system(cmd);
      }
   }

   /* force a statistics update on the first loop */
   cpy_loop_time = -2000;
   if (dev_type == LOG_TYPE_TAPE) {
      char str[MAX_FILE_PATH];
      sprintf(str, "Starting lazy job on %s at block %d", lazyst.backfile, blockn);
      if (msg_flag)
         cm_msg(MTALK, "Lazy", str);
      cm_msg(MINFO, "Lazy", str);
      cm_msg1(MINFO, "lazy_log_update", "lazy", str);
   }

   /* infinite loop while copying */
   while (1) {
      if (copy_continue) {
         if (yb_any_physrec_get(data_fmt, &plazy, &szlazy) == YB_SUCCESS) {
            status = yb_any_log_write(hDev, data_fmt, dev_type, plazy, szlazy);
            if (status != SS_SUCCESS) {
               /* close source file */
               yb_any_file_rclose(dev_type);
               /* close output data file */
               yb_any_file_wclose(hDev, dev_type, data_fmt, outfile);
               /* szlazy is the requested block size. Why is it copied to cm_msg?
                  cm_msg(MERROR,"lazy_copy","Write error %i",szlazy); */
               cm_msg(MERROR, "lazy_copy", "Write error ");
               if (status == SS_NO_SPACE)
                  return status;
               return (FORCE_EXIT);
            }
            lazyst.cur_size += (double) szlazy;
            lazyst.cur_dev_size += (double) szlazy;
            if ((ss_millitime() - cpy_loop_time) > 2000) {
               /* update statistics */
               lazy_statistics_update(cpy_loop_time);

               /* check conditions */
               copy_continue = lazy_condition_check();

               /* update check loop */
               cpy_loop_time = ss_millitime();

               /* yield quickly */
               status = cm_yield(1);
               if (status == RPC_SHUTDOWN || status == SS_ABORT || exit_request) {
                  cm_msg(MINFO, "Lazy", "Abort postponed until end of copy of %s %1.0lf[%%]",
                         infile, (double) lazyst.progress);
                  exit_request = TRUE;
               }
            }
         } /* get physrec */
         else
            break;
      } /* copy_continue */
      else {                    /* !copy_continue */
         status = cm_yield(1000);
         if (status == RPC_SHUTDOWN || status == SS_ABORT)
            return (FORCE_EXIT);
         if ((ss_millitime() - no_cpy_last_time) > 5000) {
            copy_continue = lazy_condition_check();
            no_cpy_last_time = ss_millitime();
         }
      }                         /* !copy_continue */
   }                            /* while forever */

   /* update for last the statistics */
   lazy_statistics_update(0);

   /* close input log device */
   yb_any_file_rclose(dev_type);

   /* close output data file */
   if (equal_ustring(lazy.type, "Tape")) {
      blockn = ss_tape_get_blockn(hDev);
   }
   status = yb_any_file_wclose(hDev, dev_type, data_fmt, outfile);
   if (status != SS_SUCCESS) {
      if (status == SS_NO_SPACE)
         return status;
      return (FORCE_EXIT);
   }

   /* request exit */
   if (exit_request)
      return (EXIT_REQUEST);
   return 0;
#endif // HAVE_YBOS
}

/*------------------------------------------------------------------*/

#ifdef OS_LINUX                 // does not work under Windows because of missing popen

INT lazy_script_copy(char *infile)
/********************************************************************\
Routine: lazy_script_copy
Purpose: call user script to backup file to backup device
Input:
char * infile    source file to be backed up
Output:
Function value:
0           success
\********************************************************************/
{
   int status;
   FILE *fin;
   int cpy_loop_time = ss_millitime();
   char cmd[256];

   /* init copy variables */
   lazyst.cur_size = 0;

   /* run shell command if available */
   if (lazy.commandBefore[0]) {
      char cmd[256];
      sprintf(cmd, "%s %s %i", lazy.commandBefore, infile, lazyst.nfiles);
      cm_msg(MINFO, "Lazy", "Exec pre file write script:%s", cmd);
      ss_system(cmd);
   }

   /* create the command for the backup script */
   sprintf(cmd, "%s %s 2>&1", lazy.path, infile);

   {
      char str[MAX_FILE_PATH];
      sprintf(str, "Starting lazy job \'%s\'", cmd);
      if (msg_flag)
         cm_msg(MTALK, "Lazy", str);
      cm_msg(MINFO, "Lazy", str);
      cm_msg1(MINFO, "lazy_log_update", "lazy", str);
   }

   /* start the backup script */
   fin = popen(cmd, "r");

   if (fin == NULL) {
      cm_msg(MTALK, "Lazy_copy", "cannot start %s, errno %d (%s)", cmd, errno, strerror(errno));
      return FORCE_EXIT;
   }

   if (1) {
      int desc = fileno(fin);
      int flags = fcntl(desc, F_GETFL, 0);
      fcntl(desc, F_SETFL, flags | O_NONBLOCK);
   }

   /* infinite loop while copying */
   while (1) {
      char buf[1024];
      int rd = read(fileno(fin), buf, sizeof(buf));
      if (rd == 0) {
         /* file closed - backup script completed */
         break;
      } else if (rd < 0) {
         /* i/o error */

         if (errno == EAGAIN) {
            /* this is the main loop while waiting for the backup script */
            status = cm_yield(1000);
            continue;
         }

         cm_msg(MERROR, "lazy_copy", "Error reading output of the backup script, errno %d (%s)", errno,
                strerror(errno));
         break;
      } else {
         char *p;
         /* backup script printed something, write it to log file */

         /* make sure text is NUL-terminated */
         buf[rd] = 0;

         /* chop the output of the backup script into lines separated by '\n' */
         p = buf;
         while (p) {
            char *q = strchr(p, '\n');
            if (q) {
               *q = 0;          // replace '\n' with NUL
               q++;
               if (q[0] == 0)   // check for end of line
                  q = NULL;
            }
            cm_msg(MINFO, "lazy_copy", p);
            p = q;
         }
      }
   }

   /* update for last the statistics */
   lazy_statistics_update(cpy_loop_time);

   /* close input log device */
   status = pclose(fin);

   if (status != 0) {
      cm_msg(MERROR, "lazy_copy", "Backup script finished with exit code %d, status 0x%x", (status >> 8), status);
      return SS_NO_SPACE;
   }

   cm_msg(MINFO, "lazy_copy", "Backup script finished in %.1f sec with exit code %d",
          (ss_millitime() - cpy_loop_time) / 1000.0, (status >> 8));

   /* request exit */
   return 0;
}

#endif                          // OS_LINUX

/*------------------------------------------------------------------*/
BOOL lazy_file_exists(char *dir, char *file)
/********************************************************************\
Routine: lazy_file_exists
Purpose: check if file exists in dir by extracting its size
Input:
char * dir       data directory
char * file      file to be checked
Output:
Function value:
TRUE           file found
FALSE          file not found
\********************************************************************/
{
   char *list;
   char fullfile[MAX_FILE_PATH] = { '\0' };

   if (ss_file_find(dir, file, &list) == 1) {
      strcat(fullfile, dir);
      strcat(fullfile, DIR_SEPARATOR_STR);
      strcat(fullfile, file);
      if ((lazyst.file_size = (double) (ss_file_size(fullfile))) > 0) {
         free(list);
         return TRUE;
      }
   }
   free(list);
   return FALSE;
}

/*------------------------------------------------------------------*/

INT lazy_maintain_free_space(LAZY_INFO *pLch, LAZY_INFO *pLall)
{
   int status;

   /* update "maintain free space" */
   lazy_maintain_check(pLch->hKey, pLall);

   /* check SPACE and purge if necessary = % (1:99) */
   if (lazy.pupercent > 0) {

      // Compute initial disk space
      double freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);

      // Check for Disk full first
      if (freepercent < 5.0) {
         char buf[256];
         sprintf(buf, "Disk %s is almost full, free space: %.1f%%", lazy.dir, freepercent);
         al_trigger_alarm("Disk Full", buf, lazy.alarm, "Disk buffer full", AT_INTERNAL);
      } else {
         HNDLE hKey;
         if (db_find_key(hDB, 0, "/Alarms/Alarms/Disk Full", &hKey) == DB_SUCCESS)
            al_reset_alarm("Disk Full");
      }

      if (debug)
         printf("free space on %s: %.1f%%, lazy limit %d%%\n", lazy.dir, freepercent, lazy.pupercent);

      /* Lock other clients out of this following code as
         it may access the other client info tree */

      status = ss_semaphore_wait_for(lazy_semaphore, 5000);
      if (status != SS_SUCCESS) {
         /* exit for now and come back later */
         return NOTHING_TODO;
      }

      /* check purging action */
      while (freepercent <= (double) lazy.pupercent) {
         // flag for single purge
         BOOL donepurge = FALSE;
         /* search file to purge : run in donelist AND run in dirlog */
         /* synchronize donelist to dir log in case user has purged
            some file by hand. Basically remove the run in the list if the
            file is not anymore in the source dir. */
         DIRLOG f;
         if (lazy_select_purge(pLch->hKey, channel, pLall, lazy.backfmt, lazy.dir, &f) == SUCCESS) {
            /* check if beyond keep file + 1 */
            if (1 /*purun < (cur_acq_run - abs(lazy.staybehind) - 1)*/) {
               DWORD rm_time;
               /* remove file */
               rm_time = ss_millitime();
               std::string path = lazy.dir;
               path += f.filename;
               if (debug)
                  printf("file selected for removal %s [%s]\n", f.filename.c_str(), path.c_str());
               status = lazy_file_remove(path.c_str());
               rm_time = ss_millitime() - rm_time;
               if (status != SS_SUCCESS) {
                  cm_msg(MERROR, "Lazy", "lazy_file_remove failed on file %s", path.c_str());
                  // break out if can't delete files
                  break;
               } else {
                  status = lazy_log_update(REMOVE_FILE, f.runno, NULL, path.c_str(), rm_time);
                  donepurge = TRUE;
               }
            }                   // purun found

            // Re-compute free space
            freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);

            if (debug)
               printf("free space on %s: %.1f%%, lazy limit %d%%\n", lazy.dir, freepercent, lazy.pupercent);

         }                      // select_purge

         // donepurge = FALSE => nothing else to do 
         // donepurge = TRUE  => one purge done loop back once more
         if (!donepurge)
            break;
      }                         // while

      /* Release the semaphore  */
      status = ss_semaphore_release(lazy_semaphore);

   } /* end of if pupercent > 0 */

   return SUCCESS;
}


/*------------------------------------------------------------------*/
INT lazy_main(INT channel, LAZY_INFO * pLall)
/********************************************************************\
Routine: lazy_main
Purpose: check if backup is necessary...
Input:
channel: Current channel number
*pLall :   Pointer to all channels
Output:
Function value:
\********************************************************************/
{
   DWORD cp_time;
   INT size, status;
   char str[MAX_FILE_PATH], inffile[MAX_FILE_PATH], outffile[MAX_FILE_PATH];
   BOOL watchdog_flag, exit_request = FALSE;
   DWORD watchdog_timeout;
   LAZY_INFO *pLch;
   static BOOL eot_reached = FALSE;
   BOOL haveTape;

   /* current channel */
   pLch = &pLall[channel];

   /* extract Data format from the struct */
   if (equal_ustring(lazy.format, "YBOS"))
      data_fmt = FORMAT_YBOS;
   else if (equal_ustring(lazy.format, "MIDAS"))
      data_fmt = FORMAT_MIDAS;
   else {
      cm_msg(MERROR, "Lazy", "Unknown data format %s (MIDAS, YBOS)", lazy.format);
      return DB_NO_ACCESS;
   }

   /* extract Device type from the struct */
   if (equal_ustring(lazy.type, "DISK"))
      dev_type = LOG_TYPE_DISK;
   else if (equal_ustring(lazy.type, "TAPE"))
      dev_type = LOG_TYPE_TAPE;
   else if (equal_ustring(lazy.type, "FTP"))
      dev_type = LOG_TYPE_FTP;
   else if (equal_ustring(lazy.type, "SCRIPT"))
      dev_type = LOG_TYPE_SCRIPT;
   else {
      cm_msg(MERROR, "Lazy", "Unknown device type %s (Disk, Tape, FTP or SCRIPT)", lazy.type);
      return DB_NO_ACCESS;
   }

   if (dev_type == LOG_TYPE_SCRIPT)
      if (lazy.backlabel[0] == 0 || strcmp(lazy.backlabel, lazyinfo[channel].name) != 0) {
         strlcpy(lazy.backlabel, lazyinfo[channel].name, sizeof(lazy.backlabel));
         size = sizeof(lazy.backlabel);
         db_set_value(hDB, pLch->hKey, "Settings/List label", lazy.backlabel, size, 1, TID_STRING);
      }

   /* make sure that we don't operate on the current DAQ file if so set to oldest */
   //if (lazy.staybehind == 0) {
   //   cm_msg(MERROR, "Lazy", "Stay behind cannot be 0");
   //   return NOTHING_TODO;
   //}

   /* Check if Tape is OK */
   /* ... */

   haveTape = (lazy.backlabel[0] != '\0');

   /* check if space on device (not empty tape label) */
   if (lazy.backlabel[0] == '\0') {
      full_bck_flag = TRUE;
   } else {
      if (full_bck_flag) {
         full_bck_flag = FALSE;
         size = sizeof(lazyst);
         memset(&lazyst, 0, size);
         if (db_find_key(hDB, pLch->hKey, "Statistics", &hKeyst) == DB_SUCCESS) {
            status = db_set_record(hDB, hKeyst, &lazyst, size, 0);
            /* New session of lazy, apply append in case of Tape */
            /* DISK for debugging */
            if ((dev_type == LOG_TYPE_DISK) && lazy.tapeAppend) {
               /* Position Tape to end of Data */
               cm_msg(MINFO, "Lazy", "Positioning Tape to EOD");
            }
         } else
            cm_msg(MERROR, "lazy_main", "did not find /Lazy/Lazy_%s/Statistics for zapping", pLch->name);
         // INT al_reset_alarm(char *alarm_name)
         if (dev_type == LOG_TYPE_TAPE)
            al_reset_alarm("Tape");
      }
   }
   /* check if data dir is none empty */
   if (lazy.dir[0] == '\0') {
      cm_msg(MINFO, "Lazy", "Please setup Data dir for input source path!");
      return NOTHING_TODO;
   }

   if (lazy.dir[0] != 0)
      if (lazy.dir[strlen(lazy.dir)-1] != DIR_SEPARATOR)
         strcat(lazy.dir, DIR_SEPARATOR_STR);

   /* check if device path is set */
   if (lazy.path[0] == '\0') {
      cm_msg(MINFO, "Lazy", "Please setup backup device path too!");
      return NOTHING_TODO;
   }

   DIRLOGLIST dirlist;
   build_log_list(lazy.backfmt, lazy.dir, &dirlist);

   save_list(pLch->name, "dirlist", &dirlist);

   DIRLOGLIST donelist;
   build_done_list(pLch->hKey, &dirlist, &donelist);

   lazy_maintain_free_space(pLch, pLall);

   /* compare list : run NOT in donelist AND run in dirlog */
   int tobe_backup = find_next_file(&dirlist, &donelist);
   //const DIRLOG* tobe_backup = cmp_log2donelist(&dirlist, &donelist);

   //if (debug)
   //print_dirlog(&dirlist);

   //print_dirlog(&donelist);
   //printf("tobe_backup: %p\n", tobe_backup);

   if (tobe_backup < 0)
      return NOTHING_TODO;

   if (debug)
      printf("selected for backup: %s, run %d\n", dirlist[tobe_backup].filename.c_str(), dirlist[tobe_backup].runno);

   /* Get current run number */
   int cur_acq_run;
   size = sizeof(cur_acq_run);
   status = db_get_value(hDB, 0, "Runinfo/Run number", &cur_acq_run, &size, TID_INT, FALSE);
   assert(status == SUCCESS);

   lazyst.cur_run = dirlist[tobe_backup].runno;

   int behind_files = dirlist.size() - tobe_backup;
   int behind_runs  = cur_acq_run - dirlist[tobe_backup].runno;

   bool nothing_todo_files = (behind_files <= lazy.staybehind);
   bool nothing_todo_runs  = (behind_runs < abs(lazy.staybehind));

   bool nothing_todo = false;

   /* "stay behind by so many runs" mode */
   if (lazy.staybehind < 0)
      nothing_todo = nothing_todo_runs;

   /* "stay behind by so many files" mode */
   if (lazy.staybehind > 0)
      nothing_todo = nothing_todo_files;

   /* "no stay behind" mode */
   if (lazy.staybehind == 0) {
      if (dirlist[tobe_backup].runno != cur_acq_run)
         nothing_todo = false;
      else if (behind_files > 1)
         nothing_todo = false;
      else {
         nothing_todo = false;

         /* In case it is the current run make sure 
            1) no transition is in progress
            2) the run start has not been aborted
            3) the run has been ended
         */

         int flag;
         size = sizeof(flag);
         status = db_get_value(hDB, 0, "Runinfo/Transition in progress", &flag, &size, TID_INT, FALSE);
         assert(status == SUCCESS);
         if (flag) {
            if (debug)
               printf("transition in progress, cannot backup last file\n");
            nothing_todo = true;
         }
         
         size = sizeof(flag);
         status = db_get_value(hDB, 0, "Runinfo/Start abort", &flag, &size, TID_INT, FALSE);
         assert(status == SUCCESS);
         if (flag) {
            if (debug)
               printf("run start aborted, cannot backup last file\n");
            nothing_todo = true;
         }

         int cur_state_run;
         status = db_get_value(hDB, 0, "Runinfo/State", &cur_state_run, &size, TID_INT, FALSE);
         assert(status == SUCCESS);
         if ((cur_state_run != STATE_STOPPED)) {
            if (debug)
               printf("run still running, cannot backup last file\n");
            nothing_todo = true;
         }
      }
   }

   if (debug)
      printf("behind: %d files, %d runs, staybehind: %d, nothing_todo: files: %d, runs: %d, lazylogger: %d\n", behind_files, behind_runs, lazy.staybehind, nothing_todo_files, nothing_todo_runs, nothing_todo);

   if (nothing_todo) {
      return NOTHING_TODO;
   }

   if (dev_type != LOG_TYPE_SCRIPT)
      if (!haveTape)
         return NOTHING_TODO;

   strlcpy(lazyst.backfile, dirlist[tobe_backup].filename.c_str(), sizeof(lazyst.backfile));

   std::string xfile = lazy.dir;
   xfile += dirlist[tobe_backup].filename;
   strlcpy(inffile, xfile.c_str(), sizeof(inffile));

   /* Check again if the backup file is present in the logger dir */
   if (lazy_file_exists(lazy.dir, lazyst.backfile)) {
      /* compose the destination file name */
      if (dev_type == LOG_TYPE_DISK) {
         if (lazy.path[0] != 0)
            if (lazy.path[strlen(lazy.path) - 1] != DIR_SEPARATOR)
               strcat(lazy.path, DIR_SEPARATOR_STR);
         strcpy(outffile, lazy.path);
         strcat(outffile, lazyst.backfile);
      } else if (dev_type == LOG_TYPE_TAPE)
         strcpy(outffile, lazy.path);
      else if (dev_type == LOG_TYPE_FTP) {
         /* Format for FTP
            lazy.path=host,port,user,password,directory,filename[,umask]
            expect filename in format such as "bck%08d.mid" */

         /*
            if (lazy.path[0] != 0)
            if (lazy.path[strlen(lazy.path)-1] != DIR_SEPARATOR)
            strcat(lazy.path, DIR_SEPARATOR_STR);
          */


         strcpy(str, lazy.path);
         /* substitute "%d" for current run number */
         if (strchr(str, '%'))
            sprintf(outffile, str, lazyst.cur_run);
         else
            strcpy(outffile, str);

         /* substitute "#d" for current run number millenium */
         strlcpy(str, outffile, sizeof(str));
         if (strchr(str, '#')) {
            *strchr(str, '#') = '%';
            sprintf(outffile, str, lazyst.cur_run / 1000);
         }
      }

      /* check if space on backup device ONLY in the TAPE case */
      if (((dev_type == LOG_TYPE_TAPE)
           && (lazy.capacity < (lazyst.cur_dev_size + lazyst.file_size)))
          || eot_reached) {
         char pre_label[32];
         /* save the last label for shell script */
         strcpy(pre_label, lazy.backlabel);

         /* Reset EOT reached */
         eot_reached = FALSE;

         /* not enough space => reset list label */
         lazy.backlabel[0] = '\0';
         size = sizeof(lazy.backlabel);
         db_set_value(hDB, pLch->hKey, "Settings/List label", lazy.backlabel, size, 1, TID_STRING);
         full_bck_flag = TRUE;
         cm_msg(MINFO, "Lazy", "Not enough space for next copy on backup device!");

         /* rewind device if TAPE type */
         if (dev_type == LOG_TYPE_TAPE) {
            INT status, channel;
            char str[128];

            sprintf(str, "Tape %s is full with %d files", pre_label, lazyst.nfiles);
            cm_msg(MINFO, "Lazy", str);

            /* Setup alarm */
            lazy.alarm[0] = 0;
            size = sizeof(lazy.alarm);
            db_get_value(hDB, pLch->hKey, "Settings/Alarm Class", lazy.alarm, &size, TID_STRING, TRUE);

            /* trigger alarm if defined */
            if (lazy.alarm[0])
               al_trigger_alarm("Tape",
                                "Tape full, Please remove current tape and load new one!",
                                lazy.alarm, "Tape full", AT_INTERNAL);

            /* run shell command if available */
            if (lazy.command[0]) {
               char cmd[256];
               sprintf(cmd, "%s %s %s %s", lazy.command, lazy.path, pLch->name, pre_label);
               cm_msg(MINFO, "Lazy", "Exec post-rewind script:%s", cmd);
               ss_system(cmd);
            }

            cm_msg(MINFO, "Lazy", "backup device rewinding...");
            cm_get_watchdog_params(&watchdog_flag, &watchdog_timeout);
            cm_set_watchdog_params(watchdog_flag, 300000);      /* 5 min for tape rewind */
            status = ss_tape_open(outffile, O_RDONLY, &channel);
            if (channel < 0) {
               cm_msg(MERROR, "Lazy", "Cannot rewind tape %s - %d - %d", outffile, channel, status);
               return NOTHING_TODO;
            }
            //else
            //    cm_msg(MINFO,"Lazy", "After call ss_tape_open used to rewind tape %s - %d - %d", outffile, channel, status);
            cm_msg(MINFO, "Lazy", "Calling ss_tape_unmount");
            ss_tape_unmount(channel);
            ss_tape_close(channel);
            cm_set_watchdog_params(watchdog_flag, watchdog_timeout);
            return NOTHING_TODO;
         }                      // 1
      }                         // LOG_TYPE_TAPE 

      /* Finally do the copy */
      cp_time = ss_millitime();
      status = 0;
      if (dev_type == LOG_TYPE_SCRIPT) {
#ifdef OS_LINUX
         //sprintf(lazy.backlabel, "%06d", 100 * (lazyst.cur_run / 100));
         status = lazy_script_copy(inffile);
#else
         assert(!"lazy_script_copy not supported under Windows");
#endif
      } else {
         status = lazy_copy(outffile, inffile);
      }

      if ((status != 0) && (status != EXIT_REQUEST)) {
         if (status == SS_NO_SPACE) {
            /* Consider this case as EOT reached */
            eot_reached = TRUE;
            return status;
         } else if (status == FORCE_EXIT)
            return status;
         cm_msg(MERROR, "Lazy", "copy failed -%s-%s-%i", lazy.path, lazyst.backfile, status);
         return FORCE_EXIT;
      }
   } /* file exists */
   else {
      // If file not present it may be due to a run abort.
      // As the file is based on the "tobe_backup" run number
      // which is evaluated every lazy check, if a run is missing
      // it will be skiped properly.
      // No message is needed in this case.
      //    cm_msg(MERROR, "Lazy", "lazy_file_exists file %s doesn't exists", lazyst.backfile);
      return NOTHING_TODO;
   }

   if (status == EXIT_REQUEST)
      exit_request = TRUE;

   cp_time = ss_millitime() - cp_time;

   donelist.push_back(dirlist[tobe_backup]);

   save_done_list(pLch->hKey, &donelist);

   lazy_log_update(NEW_FILE, lazyst.cur_run, lazy.backlabel, lazyst.backfile, cp_time);

   if (msg_flag)
      cm_msg(MTALK, "Lazy", "         lazy job %s done!", lazyst.backfile);

   /* generate/update a <channel>_recover.odb file when everything is Ok
      after each file copy */
   {
      char str[128];
      /* leave "list label" as it is, as long as the _recover.odb is loaded before
         the lazylogger is started with NO -z things should be fine */
      /* save the recover with "List Label" empty */
      if (lazy.dir[strlen(lazy.dir) - 1] != DIR_SEPARATOR)
         sprintf(str, "%s%c%s_recover.odb", lazy.dir, DIR_SEPARATOR, pLch->name);
      else
         sprintf(str, "%s%s_recover.odb", lazy.dir, pLch->name);

      db_save(hDB, pLch->hKey, str, TRUE);
   }

   if (exit_request)
      return (FORCE_EXIT);
   return NOTHING_TODO;
}

/*------------------------------------------------------------------*/
int main(int argc, char **argv)
{
   char channel_name[32];
   char host_name[HOST_NAME_LENGTH];
   char expt_name[HOST_NAME_LENGTH];
   BOOL daemon;
   INT i, msg, ch, size, status, mainlast_time;

   setbuf(stdout, NULL);
   setbuf(stderr, NULL);

   /* set default */
   host_name[0] = 0;
   expt_name[0] = 0;
   channel_name[0] = 0;
   BOOL zap_flag = FALSE;
   msg_flag = FALSE;
   debug = daemon = FALSE;

   /* set default */
   cm_get_environment(host_name, sizeof(host_name), expt_name, sizeof(expt_name));

   /* get parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
         debug = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'n')
         nodelete = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'D')
         daemon = TRUE;
      else if (strncmp(argv[i], "-z", 2) == 0)
         zap_flag = TRUE;
      else if (strncmp(argv[i], "-t", 2) == 0)
         msg_flag = TRUE;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (strncmp(argv[i], "-e", 2) == 0)
            strcpy(expt_name, argv[++i]);
         else if (strncmp(argv[i], "-h", 2) == 0)
            strcpy(host_name, argv[++i]);
         else if (strncmp(argv[i], "-c", 2) == 0)
            strcpy(channel_name, argv[++i]);
      } else {
       usage:
         printf("Lazylogger: Multi channel background data copier\n");
         printf("\n");
         printf("Usage: lazylogger [-d] [-n] [-D] [-h <Hostname>] [-e <Experiment>] [-z] [-t] -c <channel name>\n");
         printf("\n");
         printf("Options:\n");
         printf(" -d - enable debug printout\n");
         printf(" -n - do not delete any files (for testing \"Maintain free space\")\n");
         printf(" -D - start as a daemon\n");
         printf(" -h - connect to experiment on another machine\n");
         printf(" -e - connect to non-default experiment\n");
         printf(" -z - clear /Lazy/channel/Statistics\n");
         printf(" -t - permit lazy logger to TALK (see mlxspeaker)\n");
         printf("\n");
         printf("Quick man :\n");
         printf("The Lazy/Settings tree is composed of the following parameters:\n");
         printf("Maintain free space (%%)(0): purge source device to maintain free space on the source directory\n");
         printf("                      (0) : no purge      \n");
         printf("Stay behind  (0)         : If negative number : lazylog runs starting from the OLDEST\n");
         printf("                             run file sitting in the 'Dir data' to the current acquisition\n");
         printf("                             run minus the 'Stay behind number'\n");
         printf("                            If positive number : lazylog starts from the current\n");
         printf("                             acquisition run minus 'Stay behind number' \n");
         printf("                            Zero : no stay-behind - files are saved as soon as they are closed\n");
         printf("Alarm Class               : Specify the Class to be used in case of Tape Full condition\n");
         printf("Running condition         : active/deactive lazylogger under given condition i.e:\n");
         printf("                           'ALWAYS' (default)     : Independent of the ACQ state ...\n");
         printf("                           'NEVER'                : ...\n");
         printf("                           'WHILE_ACQ_NOT_RUNNING': ...\n");
         printf("                           '/alias/max_rate < 200'    (max_rate is a link)\n");
         printf("                           '/equipment/scaler/variables/scal[4] < 23.45'\n");
         printf("                           '/equipment/trigger/statistics/events per sec. < 400'\n");
         printf("Data dir                  : MIDAS Data Directory (same as \"/Logger/Data Dir\")\n");
         printf("Data format               : Data format (YBOS/MIDAS)\n");
         printf("Filename format           : Run format i.e. \"run%%05d.mid\", or \"*.mid.gz\" or \"*.mid.gz,*.xml\" \n");
         printf("List label                : Label of destination save_set.\n");
         printf("                            Prevent lazylogger to run if not given.\n");
         printf("                            Will be reset if maximum capacity reached.\n");
         printf("Execute after rewind      : Execute the command <cmd> after rewind complete\n");
         printf("                          : args passed are: 'device path' 'channel name' 'list label'\n");
         printf("                          : The actual command will look like: <cmd> /dev/nst0 Tape Data_2000\n");
         printf("Backup type               : Destination device type (Disk, Tape, Script, FTP)\n");
         printf("Path                      : Destination path (file.ext, /dev/nst0, ftp...)\n");
         printf("                            in case of FTP type, the 'Path' entry should be:\n");
         printf("                            host, port, user, password, directory, run%%05d.mid\n");
         printf("Capacity (Bytes)          : Maximum capacity of the destination device.\n");
         printf("modulo                    : Enable multiple lazy on same source. Ex: 3ch : 3.0, 3.1, 3.2\n");
         printf("tapeAppend                : Enable positioning of the TAPE to EOD before each lazy copy\n");
         return 1;
      }
   }

   /* Handle SIGPIPE signals generated from errors on the pipe */
#ifdef SIGPIPE
   signal(SIGPIPE, SIG_IGN);
#endif
   if (daemon) {
      printf("Becoming a daemon...\n");
      ss_daemon_init(FALSE);
   }

   /* connect to experiment */
   status = cm_connect_experiment1(host_name, expt_name, "Lazy", 0, DEFAULT_ODB_SIZE, WATCHDOG_TIMEOUT);
   if (status != CM_SUCCESS)
      return 1;

   /* create a common semaphore for the independent lazylogger */
   status = ss_semaphore_create("LAZY", &lazy_semaphore);

   /* check lazy status for multiple channels */
   cm_get_experiment_database(&hDB, &hKey);
   if (db_find_key(hDB, 0, "/Lazy", &hKey) == DB_SUCCESS) {
      HNDLE hSubkey;
      KEY key;
      char strclient[32];
      INT j = 0;
      for (i = 0;; i++) {
         db_enum_key(hDB, hKey, i, &hSubkey);
         if (!hSubkey)
            break;
         db_get_key(hDB, hSubkey, &key);
         if (key.type == TID_KEY) {
            /* compose client name */
            sprintf(strclient, "Lazy_%s", key.name);
            if (cm_exist(strclient, TRUE) == CM_SUCCESS)
               lazyinfo[j].active = TRUE;
            else
               lazyinfo[j].active = FALSE;
            strcpy(lazyinfo[j].name, key.name);

            lazyinfo[j].hKey = hSubkey;
            j++;
         }
      }
   } else {
      /* create settings tree */
      char str[32];
      channel = 0;
      if (channel_name[0] != 0)
         strlcpy(lazyinfo[channel].name, channel_name, sizeof(lazyinfo[channel].name));
      sprintf(str, "/Lazy/%s/Settings", lazyinfo[channel].name);
      db_create_record(hDB, 0, str, LAZY_SETTINGS_STRING);
   }

   {                            /* Selection of client */
      INT i, j;
      char str[32];

      if (lazyinfo[0].hKey) {
         if (channel_name[0] == 0) {
            /* command prompt */
            printf(" Available Lazy channels to connect to:\n");
            i = 0;
            j = 1;
            while (lazyinfo[i].hKey) {
               if (!lazyinfo[i].active)
                  printf("%d) Lazy %s \n", j, lazyinfo[i].name);
               else
                  printf(".) Lazy %s already active\n", lazyinfo[i].name);
               j++;
               i++;
            }
            printf("Enter client number or new lazy client name: ");
            i = atoi(ss_gets(str, 32));
            if ((i == 0) && ((strlen(str) == 0) || (strncmp(str, " ", 1) == 0))) {
               cm_msg(MERROR, "Lazy", "Please specify a valid channel name (%s)", str);
               goto error;
            }
         } else {
            /* Skip the command prompt for serving the -c option */
            /*
               scan if channel_name already exists
               Yes : check if client is running
               Yes : get out (goto error)
               No  : connect (extract index)
               No  : new channel (i=0)
             */
            i = 0;
            j = -1;
            while (lazyinfo[i].hKey) {
               if (equal_ustring(channel_name, lazyinfo[i].name)) {
                  /* correct name => check active  */
                  if (lazyinfo[i].active) {
                     cm_msg(MERROR, "Lazy", "Lazy channel " "%s" " already running!", lazyinfo[i].name);
                     goto error;
                  }
                  j = i;
               }
               i++;
            }
            if (j == -1) {
               /* new entry */
               i = 0;
               sprintf(str, "%s", channel_name);
            } else {
               /* connect to */
               i = j + 1;
            }
         }
         if (i == 0) {          /* new entry */
            char strclient[32];
            for (j = 0; j < MAX_LAZY_CHANNEL; j++) {
               if (lazyinfo[j].hKey == 0) {
                  /* compose client name */
                  sprintf(strclient, "Lazy_%s", str);
                  if (cm_exist(strclient, TRUE) == CM_SUCCESS)
                     lazyinfo[j].active = TRUE;
                  else
                     lazyinfo[j].active = FALSE;
                  strcpy(lazyinfo[j].name, str);
                  lazyinfo[j].hKey = 0;
                  channel = j;
                  break;
               }
            }
         } else if (!lazyinfo[i - 1].active)
            channel = i - 1;
         else
            channel = -1;
      }

      if ((channel < 0) && (lazyinfo[channel].hKey != 0))
         goto error;
      if (channel < 0)
         channel = 0;

      {                         /* creation of the lazy channel */
         char str[128];

         if (lazyinfo[channel].hKey == 0)
            printf(" Creating Lazy channel %s\n", lazyinfo[channel].name);

         /* create/update settings */
         sprintf(str, "/Lazy/%s/Settings", lazyinfo[channel].name);
         db_create_record(hDB, 0, str, LAZY_SETTINGS_STRING);
         /* create/update statistics */
         sprintf(str, "/Lazy/%s/Statistics", lazyinfo[channel].name);
         db_create_record(hDB, 0, str, LAZY_STATISTICS_STRING);
         sprintf(str, "/Lazy/%s", lazyinfo[channel].name);
         db_find_key(hDB, 0, str, &lazyinfo[channel].hKey);
      }
   }
   /* disconnect  from expriment */
   cm_disconnect_experiment();

   {                            /* reconnect to experiment with proper name */
      char str[32];
      sprintf(str, "Lazy_%s", lazyinfo[channel].name);
      status = cm_connect_experiment1(host_name, expt_name, str, 0, DEFAULT_ODB_SIZE, WATCHDOG_TIMEOUT);
   }
   if (status != CM_SUCCESS)
      goto error;

   cm_get_experiment_database(&hDB, &hKey);

   /* Remove temporary Lazy entry */
   {
      HNDLE hPkey;

      status = db_find_key(hDB, 0, "Programs/Lazy", &hPkey);
      if (status == DB_SUCCESS) {
         status = db_delete_key(hDB, hPkey, FALSE);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "Lazy", "Cannot delete /Programs/Lazy");
         }
      }
   }

   /* turn on keepalive messages with increased timeout */
   if (debug)
      cm_set_watchdog_params(TRUE, 0);

#ifdef HAVE_FTPLIB
   if (debug)
      ftp_debug((int (*)(char *)) puts, (int (*)(char *)) puts);
#endif

   printf("Lazy_%s starting... " "!" " to exit \n", lazyinfo[channel].name);

   if (zap_flag) {
      /* reset the statistics */
      cm_msg(MINFO, "Lazy", "zapping %s/statistics content", lazyinfo[channel].name);
      size = sizeof(lazyst);
      memset(&lazyst, 0, size);
      if (db_find_key(hDB, lazyinfo[channel].hKey, "Statistics", &hKeyst) == DB_SUCCESS)
         status = db_set_record(hDB, hKeyst, &lazyst, size, 0);
      else
         cm_msg(MERROR, "Lazy", "did not find %s/Statistics for zapping", lazyinfo[channel].name);
   }

   /* get value once & hot links the run state */
   db_find_key(hDB, 0, "/runinfo/state", &hKey);
   size = sizeof(run_state);
   db_get_data(hDB, hKey, &run_state, &size, TID_INT);
   status = db_open_record(hDB, hKey, &run_state, sizeof(run_state), MODE_READ, NULL, NULL);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "Lazy", "cannot open /runinfo/state record");
   }
   /* hot link for statistics in write mode */
   size = sizeof(lazyst);
   if (db_find_key(hDB, lazyinfo[channel].hKey, "Statistics", &hKey) == DB_SUCCESS)
      db_get_record(hDB, hKey, &lazyst, &size, 0);
   status = db_open_record(hDB, hKey, &lazyst, sizeof(lazyst), MODE_WRITE, NULL, NULL);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "Lazy", "cannot open %s/Statistics record", lazyinfo[channel].name);
   }
   /* get /settings once & hot link settings in read mode */
   db_find_key(hDB, lazyinfo[channel].hKey, "Settings", &hKey);
   size = sizeof(lazy);
   status = db_open_record(hDB, hKey, &lazy, sizeof(lazy)
                           , MODE_READ, lazy_settings_hotlink, NULL);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "Lazy", "cannot open %s/Settings record", lazyinfo[channel].name);
   }

   /* set global key for that channel */
   pcurrent_hKey = lazyinfo[channel].hKey;

   /* set Data dir from /logger if local is empty & /logger exists */
   if ((lazy.dir[0] == '\0') && (db_find_key(hDB, 0, "/Logger/Data dir", &hKey) == DB_SUCCESS)) {
      size = sizeof(lazy.dir);
      db_get_data(hDB, hKey, lazy.dir, &size, TID_STRING);
      db_set_value(hDB, lazyinfo[channel].hKey, "Settings/Data dir", lazy.dir, size, 1, TID_STRING);
   }

   mainlast_time = 0;

   /* initialize ss_getchar() */
   ss_getchar(0);

   do {
      msg = cm_yield(2000);
      DWORD period = lazy.period*1000;
      if (period < 1)
         period = 1;
      if ((ss_millitime() - mainlast_time) > period) {
         status = lazy_main(channel, &lazyinfo[0]);
         if (status == FORCE_EXIT) {
            cm_msg(MERROR, "lazy", "Exit requested by program");
            break;
         }
         mainlast_time = ss_millitime();
      }
      ch = 0;
      while (ss_kbhit()) {
         ch = ss_getchar(0);
         if (ch == -1)
            ch = getchar();
         if ((char) ch == '!')
            break;
      }
   } while (msg != RPC_SHUTDOWN && msg != SS_ABORT && ch != '!');

 error:
   cm_disconnect_experiment();
   return 1;
}

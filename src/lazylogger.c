/********************************************************************\

  Name:         lazylogger.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Disk to Tape copier for background job

  $Log$
  Revision 1.15  1999/11/08 15:08:04  midas
  Added new parameters to al_trigger_alarm

  Revision 1.14  1999/10/22 10:58:56  midas
  Fixed compiler warnings

  Revision 1.13  1999/10/22 00:30:45  pierre
  - add hot link on settings for maintain space.

  Revision 1.12  1999/10/18 14:41:50  midas
  Use /programs/<name>/Watchdog timeout in all programs as timeout value. The
  default value can be submitted by calling cm_connect_experiment1(..., timeout)

  Revision 1.11  1999/10/18 11:40:53  midas
  Changed %9.2e[MB] to %1.3lfMB for NEWS message to be the same as for the COPIED msg.

  Revision 1.10  1999/10/18 11:31:47  midas
  - Fixed problem that lazy_main returned occasionally before releasing mutex
  - Changed ss_tape_rewind to ss_tape_unload, use increased timeout
  - Changed al_trigger_class to al_trigger_alarm

  Revision 1.9  1999/10/15 23:13:13  pierre
  - Added synchronization on purge between list and source dir
  - fix duplicated copy bug
  - move mutex outside purge

  Revision 1.8  1999/10/15 12:44:19  midas
  Increase timeout

  Revision 1.7  1999/10/13 19:31:28  pierre
  - Restore mod from version 1.2 and merge by hand! mod 'til 1.6
  - Implement single "Maintain free space" check.
  - change purge free space test to INT (+/- 1%)

  Revision 1.6  1999/10/13 00:29:26  pierre
  - Added -D for daemon
  - fix return code for ss_file_remove in lazy_main

  Revision 1.5  1999/10/08 08:01:09  midas
  Changed format from %lf to %1.0lf for postponed message

  Revision 1.4  1999/10/08 07:58:18  midas
  Fixed following bugs (hopefully):
  - Error message ss_remove_file was produced in a loop filling up log file,
    now it's only produced once and the file is nevertheless removed from
    the lazy list.
  - debug variable was not initialized properly
  - "abort postponed.." message used wrong format (%d instead %lf)

  Revision 1.3  1999/10/07 09:32:54  midas
  Fixed bug that '/' was added to the tape name which caused the lazylogger
  to crash since a device /dev/nst0/ does not exist (OS complains that the
  device is no directory). The additional '/' is now only added in the message.

  Revision 1.1  1999/10/06 07:05:10  midas
  Moved lazylogger from utils to src

  Revision 1.14  1999/10/06 00:42:28  pierre
  - added -c <lazy_channel> option
  - fix log message path and index

  Revision 1.13  1999/09/27 16:23:56  pierre
  - Changed KB to Bytes references

  Revision 1.12  1999/09/24 00:05:50  pierre
  - Modified for multiple lazy channel.
  - Remove lazy.log and log to midas.log

  Revision 1.11  1999/07/01 11:31:19  midas
  Change lazy_log_update for FTP mode

  Revision 1.10  1999/06/30 14:43:00  midas
  Increased str length in lazy_log_updata

  Revision 1.9  1999/06/23 09:48:42  midas
  Added FTP functionality

  Revision 1.8  1999/05/06 19:18:53  pierre
  - replace [%] by (%)

  Revision 1.7  1999/02/19 21:54:08  pierre
  - incorporate hot links on Settings

  Revision 1.6  1999/02/05 23:51:11  pierre
  - Cleanup /settings structure
  - Enable FTP channel
  - Add examples in -h switch

  Revision 1.5  1999/01/18 18:21:13  pierre
  - Changed the setting structure.
  - Added running condition test
  - Adapted to ybos prototypes.

  Revision 1.4  1998/11/20 15:02:01  pierre
  Added MIDAS fmt support
  No FTP channel support yet!

  Revision 1.3  1998/10/19 17:48:48  pierre
  - restructure of setting and statistics
  - add disk to disk support
  - add lazy.log log file and cleanup of List tree
  - NO support for MIDAS format yet

  Revision 1.2  1998/10/12 12:19:03  midas
  Added Log tag in header
\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "ybos.h"

#define NOTHING_TODO  0
#define FORCE_EXIT    1
#define NEW_FILE      1
#define REMOVE_FILE   2
#define REMOVE_ENTRY  3
#define MAX_LAZY_CHANNEL 4

typedef struct {
  INT run;
  double size;
} DIRLOG;

#define LAZY_SETTINGS_STRING "\
Maintain free space(%) = INT : 0\n\
Stay behind = INT : -3\n\
Alarm Class = STRING : [32]\n\
Running condition = STRING : [128] ALWAYS\n\
Data dir = STRING : [256] \n\
Data format = STRING : [8] MIDAS\n\
Filename format = STRING : [128] run%05d.mid\n\
Backup type = STRING : [8] Tape\n\
Path = STRING : [128] \n\
Capacity (Bytes) = FLOAT : 5e9\n\
List label= STRING : [128] \n\
"
#define LAZY_STATISTICS_STRING "\
Backup file = STRING : [128] none \n\
File size [Bytes] = FLOAT : 0.0\n\
KBytes copied = FLOAT : 0.0\n\
Total Bytes copied = FLOAT : 0.0\n\
Copy progress [%] = FLOAT : 0\n\
Copy Rate [bytes per s] = FLOAT : 0\n\
Backup status [%] = FLOAT : 0\n\
Number of Files = INT : 0\n\
Current Lazy run = INT : 0\n\
"

typedef struct {
  INT   pupercent;                /* 0:100 % of disk space to keep free */
  INT   staybehind;               /* keep x run file between current Acq and backup 
                                        -x same as x but starting from oldest */
  char  alarm[32];                /* Alarm Class */
  char  condition[128];           /* key condition */
  char  dir[256];                 /* path to the data dir */
  char  format[8];                /* Data format (YBOS, MIDAS) */
  char  backfmt[MAX_FILE_PATH];   /* format for the run files run%05d.mid */
  char  type[8];                  /* Backup device type  (Disk, Tape, Ftp) */
  char  path[MAX_FILE_PATH];      /* backup device name */
  float capacity;                 /* backup device max byte size */
  char  backlabel[MAX_FILE_PATH]; /* label of the array in ~/list. if empty like active = 0 */
} LAZY_SETTING;
LAZY_SETTING  lazy;

typedef struct {
char  backfile[MAX_FILE_PATH]; /* current or last lazy file done (for info only) */
float file_size;               /* file size in bytes*/
float cur_size;                /* current bytes copied */
float cur_dev_size;            /* Total bytes backup on device */
float progress;                /* copy % */
float copy_rate;               /* copy rate */
float bckfill;                 /* backup fill % */
INT nfiles;                    /* # of backuped files */
INT cur_run;                   /* current or last lazy run number done (for info only) */
} LAZY_STATISTICS;
LAZY_STATISTICS lazyst;


typedef struct {
  HNDLE hKey;
  BOOL  active;
  BOOL  match;
  char name[32];
} LAZY_INFO;

LAZY_INFO lazyinfo[MAX_LAZY_CHANNEL]={{0,FALSE,FALSE,"Tape"},{0,FALSE,FALSE,""}
                      ,{0,FALSE,FALSE,""},{0,FALSE,FALSE,""}};
INT channel = -1;

/* Globals */
INT       lazy_mutex;
HNDLE     hDB, hKey, pcurrent_hKey;
float     lastsz;
HNDLE     hKeyst;
INT       run_state, tot_do_size, tot_dirlog_size, hDev;
BOOL      zap_flag, msg_flag;
BOOL      copy_continue = TRUE;
INT       data_fmt, dev_type;
char      lazylog[MAX_STRING_LENGTH];
BOOL      full_bck_flag = FALSE, maintain_touched=FALSE;

#define WATCHDOG_TIMEOUT 60000 /* 60 sec for tape access */

/* prototypes */
BOOL lazy_file_exists(char * dir, char * file);
INT  lazy_main (INT, LAZY_INFO *);
INT  lazy_copy( char * dev, char * file);
INT  lazy_file_compose(char * fmt , char * dir, INT num, char * ffile, char * file);
INT  lazy_update_list(LAZY_INFO *);
INT  lazy_select_purge(HNDLE,  INT ch, LAZY_INFO *, char * fmt, char * dir, char * pufile, INT * run);
INT  lazy_load_params( HNDLE hDB, HNDLE hKey );
INT  build_log_list(char * fmt, char * dir, DIRLOG ** plog);
INT  build_done_list(HNDLE, INT **);
INT  cmp_log2donelist (DIRLOG * plog, INT * pdo);
INT  lazy_log_update(INT action, INT run, char * label, char * file);
int  lazy_remove_entry(INT ch, LAZY_INFO *, int run);
void lazy_settings_hotlink(HNDLE hDB, HNDLE hKey, void * info);
void lazy_maintain_check(HNDLE hKey, LAZY_INFO * pLall);
/*------------------------------------------------------------------*/
INT lazy_file_remove(char * pufile)
{
  INT  fHandle, status;

  /* open device */
  fHandle = open(pufile, O_RDONLY, 0644);
  if (fHandle == -1)
    return SS_INVALID_NAME;

  close(fHandle);

  status = ss_file_remove(pufile);
  if (status != 0)
    return SS_FILE_ERROR;
  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT lazy_log_update(INT action, INT run, char * label, char * file)
{
  char str[MAX_FILE_PATH];
  
  /* log Lazy logger to midas.log only */
  if (action == NEW_FILE)
    {
    /* keep track of number of file on that channel */
    lazyst.nfiles++;

    if (equal_ustring(lazy.type, "FTP"))
      sprintf(str, "%s: %s %1.3lfMB file COPIED",
              label, lazyst.backfile, lazyst.file_size/1024.0/1024.0);
    else
      sprintf(str,"%s[%i] %s%c%s %1.3lfMB file NEW",
	      label, lazyst.nfiles,
	      lazy.path, DIR_SEPARATOR, lazyst.backfile, 
              lazyst.file_size/1024.0/1024.0);
    }

  else if (action == REMOVE_FILE)
    sprintf(str,"%i  %s file REMOVED",
            run, file);
  
  else if (action == REMOVE_ENTRY)
    sprintf(str,"%s run#%i entry REMOVED", 
            label, run);
    
  cm_msg(MINFO, "lazy_log_update", str);
  
  return 0;
}

/*------------------------------------------------------------------*/
INT ss_run_extract(char * name)
/********************************************************************\
  Routine: ss_run_extract
  Purpose: extract the contigious digit from the string to make up a
            run number.
  Input:
   char * name    string to search
  Output:
  Function value:
   INT        run number 
              0 if no extraction
\********************************************************************/
{
  char run_str[32], *pb, *pe;
  int j=0, found=0;
  
  pb = pe = NULL;
  memset(run_str, 0, sizeof(run_str));
  for (j=0;j<(int)strlen(name);j++)
    {
      if (!found)
        {
          if (isdigit(name[j]))
            {
              found = 1;
              pb = name+j;
            }
        }
      else if (isdigit(name[j]) == 0)
        {
          pe = name+j;
          break;
        }
    }
  strncpy(run_str, pb, pe-pb); 
  return(atoi(run_str));
}

/*------------------------------------------------------------------*/
INT build_log_list(char * fmt, char * dir, DIRLOG ** plog)
/********************************************************************\
  Routine: build_log_list
  Purpose: build an internal directory file list from the disk directory
  Input:
    char     * fmt     format of the file to search for (ie:run%05d.ybs) 
    char     * dir     path to the directory for the search
  Output:
    INT     **plog     internal file list struct
  Function value:
    number of elements
\********************************************************************/
{
  INT nfile, i, j;
  char * list = NULL;
  char str[MAX_FILE_PATH];
  DIRLOG temp;
  
  /* substitue %xx by * */
  strcpy (str, fmt);
  if (strchr(str,'%'))
    {
      *strchr(str,'%') = '*';
      if (strchr(str, '.'))
        strcpy( (strchr(str,'*')+1), strchr(str,'.'));
    }
  
  /* create dir listing with given criteria */
  if (dir[0] != 0)
    if (dir[strlen(dir)-1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);
   nfile = ss_file_find(dir, str, &list);

  /* check */
  /*
    for (j=0;j<nfile;j++)
    printf ("list[%i]:%s\n",j, list+j*MAX_STRING_LENGTH);
  */

  /* allocate memory */
  tot_dirlog_size = (nfile*sizeof(DIRLOG));
  *plog = realloc(*plog, tot_dirlog_size);
  memset(*plog, 0, tot_dirlog_size);
  /* fill structure */
  for (j=0;j<nfile;j++)
    {
		  INT strl;
      (*plog+j)->run  = ss_run_extract(list+j*MAX_STRING_LENGTH);
      strcpy(str,dir);
			strl = strlen(list+j*MAX_STRING_LENGTH);
      strncat(str,list+j*MAX_STRING_LENGTH, strl);
      (*plog+j)->size = ss_file_size(str);
    }
  free(list);

  for (j=0;j<nfile;j++)
  {
    for (i=j; i<nfile ; i++)
    {
      if ((*plog+j)->run > (*plog+i)->run)
      {
        memcpy (&temp, (*plog+i), sizeof(DIRLOG));
        memcpy ((*plog+i), (*plog+j), sizeof(DIRLOG));
        memcpy ((*plog+j), &temp, sizeof(DIRLOG));
      }
    }
  }
  return nfile;
}

/*------------------------------------------------------------------*/
INT build_done_list(HNDLE hLch, INT **pdo)
/********************************************************************\
  Routine: build_done_list
  Purpose: build an internal /lazy/list list (pdo) tree
  Input:
    HNDLE             Key of the Lazy channel
    INT     **pdo     /lazy_xxx/list run listing
  Output:
    INT     **pdo     /lazy_xxx/list run listing
  Function value:      number of elements
\********************************************************************/
{
  HNDLE hKey, hSubkey;
  KEY    key;
  INT i, j, size, tot_nelement, nelement, temp;
  
  if (db_find_key(hDB, hLch, "List", &hKey) != DB_SUCCESS)
  {
    *pdo[0] = 0;
    return 0;
  }

  tot_nelement = tot_do_size = 0;
  for (i=0 ; ; i++)
  {
    db_enum_key(hDB, hKey, i, &hSubkey);
    if (!hSubkey)
      break;
    db_get_key(hDB, hSubkey, &key);
    nelement = key.num_values;
    tot_do_size += nelement*sizeof(INT);
    *pdo = realloc (*pdo,tot_do_size);
    size = nelement * sizeof(INT);
    db_get_data(hDB, hSubkey, (char *)(*pdo+tot_nelement), &size, TID_INT);
    tot_nelement += nelement;
  }

  for (j=0;j<tot_nelement-1;j++)
  {
    for (i=j+1; i<tot_nelement ; i++)
    {
      if (*(*pdo+j) > *(*pdo+i))
      {
        memcpy (&temp, (*pdo+i), sizeof(INT));
        memcpy ((*pdo+i), (*pdo+j), sizeof(INT));
        memcpy ((*pdo+j), &temp, sizeof(INT));
      }
    }
  }
  return tot_nelement;
}

/*------------------------------------------------------------------*/
INT cmp_log2donelist (DIRLOG * plog, INT * pdo)
/********************************************************************\
  Routine: cmp_log2donelist
  Purpose: return the run number to be backed up comparing the
           plog struct to the pdo struct
           the condition being : run# in (plog AND !pdo)
           marks the run present in both list by tagging them with *= -1
  Input:
    DIRLOG  *plog    disk file listing
    INT     *pdo     /lazy/list run listing
  Output:
  Function value:
    INT              Run number
          -1       : run not found
\********************************************************************/
{
  INT j, i, ndo, nlog, notfound=0;
  
  ndo = tot_do_size / sizeof(INT);
  nlog = tot_dirlog_size / sizeof(DIRLOG);
  
  for (j=0;j<nlog;j++)
  {
    for (i=0;i<ndo;i++)
    {
      if ((plog+j)->run == pdo[i])
        (plog+j)->run *= -1;
    }
  }
  for (j=0;j<nlog;j++)
  {
    if ((plog+j)->run > 0)
      return (plog+j)->run;
  }
return -1;
}

/*------------------------------------------------------------------*/
INT lazy_select_purge(HNDLE hKey, INT channel, LAZY_INFO * pLall, char * fmt, char * dir, char * fpufile, INT * run)
/********************************************************************\
  Routine: lazy_select_purge
  Purpose: Search oldest run number which can be purged
           condition : oldest run# in (pdo AND present in plog)
           AND scan all the other channels based on the following 
           conditions:
           "data dir" && "filename format" are the same &&
           the /list/run_number exists in all the above condition.
  Input:
   HNDLE          current lazy channel
   DIRLOG * plog  internal dir file listing
  Output:
   char * pufile  file to be purged
   INT * run      corresponding run# to be purged
  Function value:
    0          success
   -1          run not found
\********************************************************************/
{
  DIRLOG *pdirlog=NULL;
  INT *pdonelist=NULL, *potherlist=NULL, size;
  INT  i, j, k, status, ndone, nother, nfile;
  BOOL mark;
  char pufile[MAX_FILE_PATH];
  char ddir[256], ff[128];
  char cdir[256], cff[128];

  /* Scan donelist from first element (oldest)
     check if run exists in dirlog
     if yes return file and run number */

  /* get current specification */
  size = sizeof(cdir);
  db_get_value(hDB, hKey, "Settings/Data dir", &cdir, &size, TID_STRING);
  size = sizeof(cff);
  db_get_value(hDB, hKey, "Settings/filename format", &cff, &size, TID_STRING);
  while (1)
  {
    /* try to find the oldest matching run present in the list AND on disk */
    /* build current done list */
    if (pdonelist == NULL) pdonelist = malloc(sizeof(INT));
    if (potherlist == NULL) potherlist = malloc(sizeof(INT));
    ndone = build_done_list(hKey, &pdonelist);
  
   /* build matching dir and ff */
    for (i=0; i < MAX_LAZY_CHANNEL ; i++)
    {
      (pLall+i)->match = FALSE;
      if (((pLall+i)->hKey) && (hKey != (pLall+i)->hKey))
      { /* valid channel */
        size = sizeof(ddir);
        db_get_value(hDB, (pLall+i)->hKey, "Settings/Data dir", &ddir, &size, TID_STRING);
        size = sizeof(ff);
        db_get_value(hDB, (pLall+i)->hKey, "Settings/filename format", &ff, &size, TID_STRING);

        if ((strcmp(ddir, cdir) == 0) && (strcmp(ff, cff) == 0))
          (pLall+i)->match = TRUE;
      }  
    }

    /* scan matching run number */
    for (i=0; i < MAX_LAZY_CHANNEL ; i++)
    {
      if ((pLall+i)->match)
      {
        /* build done list for that channel */
        nother = build_done_list((pLall+i)->hKey, &potherlist);

        /* search for run match */
        for (k=0; k < ndone; k++)
        {
          mark = FALSE;
          for (j=0 ; j < nother ;j++)
          {
            if ((potherlist[j] == pdonelist[k]) && (pdonelist[k] != 0))
               mark = TRUE; 
          }
          if (!mark) pdonelist[k] = 0;
        }      
      }
    }

    /* Take the first run from pdonelist to purge */
    for (i=0 ; i < ndone ; i++)
    {
      if (pdonelist[i] != 0)
        break;
    }
    /* return -1 if no run found */
    if (i == ndone)
      return -1;
  
    /* pdonelist[i] is the oldest run common to all the valid channels */    
    *run = pdonelist[i];
    if (pdonelist) free (pdonelist); pdonelist=NULL;
    if (potherlist) free(potherlist); potherlist=NULL;

    /* check if file is in the dir log (exists) */
    if (pdirlog == NULL) pdirlog = malloc(sizeof(DIRLOG));
    nfile = build_log_list(fmt, dir, &pdirlog);
    for (i=0;i<nfile;i++)
    {
      if (pdirlog[i].run == *run)
      {
        status = lazy_file_compose(lazy.backfmt, lazy.dir, *run, fpufile, pufile);
        if (pdirlog) free (pdirlog); pdirlog = NULL;
        return 0;  /* found can be selected */
      }
    }
    if (pdirlog)   free (pdirlog); pdirlog=NULL;

    /* file not found synchronize listings and try again */
    status=lazy_remove_entry(channel, pLall, *run);
  }
}

/*------------------------------------------------------------------*/
void lazy_settings_hotlink(HNDLE hDB, HNDLE hKey, void * info)
{
  INT  size, maintain;

  /* check if Maintain has been touched */
  size = sizeof(maintain);
  db_get_value(hDB, hKey, "Maintain free space(%)", &maintain, &size, TID_INT);
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
   HNDLE          current lazy channel
   LAZY_INFO      
  Output:
  Function value:
\********************************************************************/
{
  INT  size;
  INT  i, maintain;
  char dir[256], ff[128];
  char cdir[256], cff[128];

  /* do test only if maintain has been touched */
  if (!maintain_touched) return;
  maintain_touched = FALSE;

  /* check is Maintain free is enabled */
  size = sizeof(maintain);
  db_get_value(hDB, hKey, "Settings/Maintain free space(%)", &maintain, &size, TID_INT);
  if (maintain != 0)
  {
    /* scan other channels */

    /* get current specification */
    size = sizeof(cdir);
    db_get_value(hDB, hKey, "Settings/Data dir", &cdir, &size, TID_STRING);
    size = sizeof(cff);
    db_get_value(hDB, hKey, "Settings/filename format", &cff, &size, TID_STRING);


    /* build matching dir and ff */
    for (i=0; i < MAX_LAZY_CHANNEL ; i++)
    {
      if (((pLall+i)->hKey) && (hKey != (pLall+i)->hKey))
      { /* valid channel */
        size = sizeof(dir);
        db_get_value(hDB, (pLall+i)->hKey, "Settings/Data dir", &dir, &size, TID_STRING);
        size = sizeof(ff);
        db_get_value(hDB, (pLall+i)->hKey, "Settings/filename format", &ff, &size, TID_STRING);

        if ((strcmp(dir, cdir) == 0) && (strcmp(ff, cff) == 0))
        {
          /* check "maintain free space" */
          size = sizeof(maintain);
          db_get_value(hDB, (pLall+i)->hKey, "Settings/Maintain free space(%)", &maintain, &size, TID_INT);
          if (maintain)
          {
            /* disable and inform */
            size = sizeof(maintain);
            maintain= 0;
            db_set_value(hDB, (pLall+i)->hKey, "Settings/Maintain free space(%)", &maintain, size, 1, TID_INT);
            cm_msg(MINFO,"lazy_maintain_check","Maintain free space on channel %s has been disable",(pLall+i)->name);
          }
        }
      }  
    }
  }
}

/*------------------------------------------------------------------*/
int lazy_remove_entry(INT channel, LAZY_INFO * pLall, int run)
/********************************************************************\
  Routine: lazy_remove_entry
  Purpose: remove run entry in all the /Lazy_.../list being marked by 
           lazy_select_purge
  
  Input:
  HNDLE            : Key to the current Lazy_xxx
  LAZY_INFO        : Full channel info
  int    run       : run number to be removed
  Output:

  Function value:
    0             success
    -1            run not found
\********************************************************************/
{
  INT    size, i, j, k;
  INT    *potherlist=NULL, nother;
  BOOL   found=FALSE;
  HNDLE  hSubkey;
  KEY    key;

  /* mark current channel for removing entry too */
  (pLall+channel)->match = TRUE;
  if (potherlist == NULL) potherlist = malloc(sizeof(INT));

  /* scan matching run number */
  for (k=0; k < MAX_LAZY_CHANNEL ; k++)
  { /* skip if no dir and no ff match */
    if ((pLall+k)->match)
    {
      /* search for the proper array to remove the entry */
      for (i=0 ; ; i++)
      {
        found = FALSE;
        if (db_find_key(hDB, (pLall+k)->hKey, "List", &hKey) != DB_SUCCESS)
        {
          cm_msg(MERROR,"lazy_entry_remove","Did not found %s/list", (pLall+k)->name);
          return -1;
        }

        db_enum_key(hDB, hKey, i, &hSubkey);
        if (!hSubkey)
          break;
        db_get_key(hDB, hSubkey, &key);
        size = key.total_size;
        nother = key.num_values;
        potherlist = (INT *)realloc(potherlist, size);
        db_get_record(hDB, hSubkey, (char *) potherlist, &size, 0);
        /* match run number */
        for (j=0; j<nother; j++)
        {
          if (*(potherlist+j) == run)
            found = TRUE;
	        if (found)
	          if (j+1 < nother)
	            *(potherlist+j) = *(potherlist+j+1);
        }
        /* delete label[] or update label[] */
        if (found)
        {
      	  nother--;
          if (nother > 0)
            db_set_data(hDB,hSubkey,potherlist,nother*sizeof(INT), nother, TID_INT);
          else
            db_delete_key(hDB,hSubkey,FALSE);
          lazy_log_update(REMOVE_ENTRY, run, (pLall+k)->name, NULL);
        }
      }
    }
  }
  if (potherlist) free (potherlist); potherlist=NULL;
  return 0;
}

/*------------------------------------------------------------------*/
INT lazy_update_list(LAZY_INFO * pLinfo)
/********************************************************************\
  Routine: lazy_update_list
  Purpose: update the /lazy/list tree with the info from lazy{}
  Input:
  HNDLE         : key to Lazy_xxx
  Output:
  Function value:
   DB_SUCCESS   : update ok
\********************************************************************/
{
  INT  size, ifiles;
  char * ptape = NULL, str[MAX_FILE_PATH];
  KEY Keylabel;
  INT  mone=-1;

  HNDLE hKeylabel;
  
  /* check if dir exists */
  if (db_find_key(hDB, pLinfo->hKey, "List", &hKey) != DB_SUCCESS)
    { /* list doesn't exists */
      sprintf(str,"List/%s", lazy.backlabel);
      db_set_value(hDB, pLinfo->hKey, str, &mone, sizeof(INT), 1, TID_INT);
      db_find_key(hDB, pLinfo->hKey, "List", &hKey);
    } 

  /*  record the saved run */
  if (db_find_key(hDB, hKey, lazy.backlabel, &hKeylabel) == DB_SUCCESS)
    { /* run array already present ==> append run */
      db_get_key(hDB, hKeylabel, &Keylabel);
      ifiles = Keylabel.num_values; 
      ptape = malloc(Keylabel.total_size);
      size = Keylabel.total_size;
      db_get_data(hDB, hKeylabel, ptape, &size, TID_INT);
      if ((size == sizeof(INT)) && (*ptape == -1))
      {
        ifiles = 1;
        size = sizeof(INT);
        memcpy((char *)ptape, (char *)&lazyst.cur_run, sizeof(INT));
      }
      else
      {
        ptape = realloc((char *)ptape, size+sizeof(INT));
        memcpy((char *)ptape+size, (char *)&lazyst.cur_run, sizeof(INT));
        ifiles += 1;
        size += sizeof(INT);
      }
      db_set_data (hDB, hKeylabel, (char *)ptape, size, ifiles, TID_INT);
    }
  else
    { /* new run array ==> */
      size = sizeof(lazyst.cur_run);
      db_set_value(hDB, hKey, lazy.backlabel, &lazyst.cur_run, size, 1, TID_INT);
    }

  lazy_log_update(NEW_FILE, lazyst.cur_run, lazy.backlabel, lazyst.backfile);
  
  if (msg_flag) cm_msg(MTALK,"Lazy","         lazy job %s done!",lazyst.backfile);
  
  if (ptape)
    free (ptape);

  db_send_changed_records();
  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT lazy_file_compose(char * fmt , char * dir, INT num, char * ffile, char * file)
/********************************************************************\
  Routine: lazy_file_compose
  Purpose: compose "file" with the args
  Input:
   char * fmt       file format 
   char * dir       data directory
   INT num          run number
  Output:
   char * ffile     composed full file name (include path)
   char * file      composed file name (translated with fmt only)
  Function value:
        0           success
\********************************************************************/
{
  char str[MAX_FILE_PATH];
  INT size;
  
  size = sizeof(dir);
  if (dir[0] != 0)
    if (dir[strlen(dir)-1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);
  strcpy(str, dir); 
  strcat(str, fmt);

  /* substitue "%d" by current run number [Full file name] */
  if (strchr(str, '%'))
    sprintf(ffile, str, num);
  else
    strcpy(ffile, str);

  /* substitue "%d" by current run number */
  strcpy(str, fmt);
  if (strchr(str, '%'))
    sprintf(file, str, num);
  else
    strcpy(file, str);
  return 0;
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
    lazyst.copy_rate = (lazyst.cur_size - lastsz) / (ss_millitime() - cploop_time);

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
BOOL condition_test(char * string)
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
  float value;
  double lcond_value;
  INT size, index, status;
  char str[128];
  char *p, *pp, *ppl, *pn, *pv, *pc;
        
  index = 0;
  p = strtok(string, " ");          
  
  while (p != NULL)
    {
      if (isdigit(*p) || (*p=='-') || (*p=='+'))
        pv = p; 
      else if ((*p == '<') || (*p == '>') ||(*p == '='))
        pc = p;
      else if (isalpha(*p) || (*p == '/'))
        {
          pn = p;
          if ((pp = strpbrk(pn,"[(")) != NULL)
            {
              if ((ppl = strpbrk(pn,"])")) != NULL)
                {
                  *pp = '\0';
                  *ppl = '\0';
                  index = atoi(pp+1);
                }
              *pp = '\0';
            }
        }
      else
        printf(" cannot decode string \n");
      p = strtok(NULL, " ");
    }

  /* catch empty condition as TRUE */
  if (pv == NULL)
    return TRUE;

  /* convert value */
  value = (float) (atoi(pv));

  db_find_key(hDB, 0, pn, &hKey);
  status = db_get_key (hDB, hKey, &key);
  if ((status == DB_SUCCESS) && (key.type <= TID_DOUBLE))
  { 
 	  /* get value of the condition */
	  size = sizeof(lcond_value);
	  db_get_data_index(hDB, hKey, &lcond_value, &size, index, key.type); 
	  db_sprintf(str, &lcond_value, key.item_size, 0, key.type);
	  lcond_value = atof(str);

    /* perform condition check */
 	  if (((*pc == '>') && ((float)lcond_value >  value)) ||
        ((*pc == '=') && ((float)lcond_value == value)) ||
        ((*pc == '<') && ((float)lcond_value < value)))
        return TRUE;
    else
        return FALSE;

    } /* TID_DOUBLE */

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
  else if (equal_ustring(lazy.condition, "WHILE_ACQ_NOT_RUNNING"))
    {
      if (run_state == STATE_RUNNING)
        return FALSE;
      else
        return TRUE;
    }
  else
    return (condition_test(lazy.condition));
}

/*------------------------------------------------------------------*/
INT lazy_copy( char * outfile, char * infile)
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

  void      *plazy = NULL;
  DWORD     szlazy;
  INT status, no_cpy_last_time=0;
  INT last_time, cpy_loop_time;
  static INT last_error = 0;
  char * pext;

  pext = malloc(strlen(infile));
  strcpy(pext, infile);

  /* find out what format it is from the extension. */
  if (strncmp(pext+strlen(pext)-3,".gz",3) == 0)
    {
      /* extension .gz terminator on .*/
      *(pext+strlen(pext)-3) = '\0';
    }

  /* init copy variables */
  lazyst.cur_size = 0.0f;
  last_time = 0;

  /* open any logging file (output) */
  if ((status = yb_any_file_wopen(dev_type, data_fmt, outfile, &hDev)) != 1) 
    {
      if ((ss_time() - last_error) > 15*60)
      {
	      last_error = ss_time();
	      cm_msg(MERROR,"copy","cannot open %s [%d]",outfile, status);
      }      
      return FORCE_EXIT;
    }
  /* reset error message */
  last_error = 0;
  
  /* open input data file */
  if (yb_any_file_ropen (infile, data_fmt) != YB_SUCCESS)
    return (FORCE_EXIT);

  /* force a statistics update on the first loop */
  cpy_loop_time = -2000;

  if (msg_flag) cm_msg(MTALK,"Lazy","Starting lazy job on %s",lazyst.backfile);
  /* infinite loop while copying */
  while (1)
  {  
    if (copy_continue)
    {
      if (yb_any_physrec_get(data_fmt, &plazy, &szlazy ) == YB_SUCCESS)
      {
	      status = yb_any_log_write(hDev, data_fmt, dev_type, plazy, szlazy);
	      if (status != SS_SUCCESS)
	      {
	        /* close source file */
	        yb_any_file_rclose(dev_type);
	        cm_msg(MERROR,"lazy_copy","Write error %i",szlazy);
	        return FORCE_EXIT; 
	      }
	      lazyst.cur_size += (float) szlazy;
	      lazyst.cur_dev_size += (float) szlazy;
	      if ((ss_millitime() - cpy_loop_time) > 2000)
	      {
	        /* update statistics */
	        lazy_statistics_update(cpy_loop_time);
	        
	        /* check conditions */
	        copy_continue = lazy_condition_check();
	        
	        /* update check loop */
	        cpy_loop_time = ss_millitime();
	        
	        /* yield quickly */
	        status = cm_yield(1);
	        if (status == RPC_SHUTDOWN || status == SS_ABORT)
	          cm_msg(MINFO,"Lazy","Abort postponed until end of copy %1.0lf[%%]",(double)lazyst.progress);
   	    }
      } /* get physrec */
      else
	      break;
    } /* copy_continue */
    else
    { /* !copy_continue */
      status = cm_yield(1000);
      if (status == RPC_SHUTDOWN || status == SS_ABORT)
      	return FORCE_EXIT;
      if ((ss_millitime() - no_cpy_last_time) > 5000)
      {
	      copy_continue = lazy_condition_check();
	      no_cpy_last_time = ss_millitime();
      }
    } /* !copy_continue */
  } /* while forever */
  
  /* update for last the statistics */
  lazy_statistics_update(cpy_loop_time);
  
  /* close log device */
  yb_any_file_rclose(dev_type);
  
  /* close input data file */   
  yb_any_file_wclose(hDev, dev_type, data_fmt);

  /* request exit */
  return 0;
}

/*------------------------------------------------------------------*/
BOOL lazy_file_exists(char * dir, char * file)
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
  char * list;
  char fullfile[MAX_FILE_PATH] = "\0";
  
  if (ss_file_find(dir, file, &list) == 1)
    {
      strcat(fullfile, dir);
      strcat(fullfile, file);
      if ((lazyst.file_size = (float)(ss_file_size(fullfile))) > 0)
      {
        free (list);
        return TRUE;
      }
    }
  free(list);
  return FALSE;
}

/*------------------------------------------------------------------*/
INT lazy_main (INT channel, LAZY_INFO * pLall)
/********************************************************************\
  Routine: lazy_main
  Purpose: check if backup is necessary...
  Input:
   HNDLE hDB   Database handle (not used here but could be hot linked)
   HNDLE hKey  Key handle of the current Lazy_xxx
  Output:
  Function value:
\********************************************************************/
{
  DIRLOG *pdirlog=NULL;
  INT *pdonelist=NULL;
  INT size,cur_acq_run, status, tobe_backup, purun;
  double freepercent, svfree;
  char pufile[MAX_FILE_PATH], inffile[MAX_FILE_PATH], outffile[MAX_FILE_PATH];
  BOOL donepurge, watchdog_flag;
  INT watchdog_timeout;
  LAZY_INFO * pLch;

  /* current channel */
  pLch = &pLall[channel];
  
  /* extract Data format from the struct */
  if (equal_ustring(lazy.format, "YBOS"))
    data_fmt = FORMAT_YBOS;
  else if (equal_ustring(lazy.format, "MIDAS"))
    data_fmt = FORMAT_MIDAS;
  else
  {
    cm_msg(MERROR,"Lazy","Unknown data format (MIDAS, YBOS)");
    return DB_NO_ACCESS;
  }

  /* extract Device type from the struct */
  if (equal_ustring(lazy.type, "DISK"))
    dev_type = LOG_TYPE_DISK;
  else if (equal_ustring(lazy.type, "TAPE"))
    dev_type = LOG_TYPE_TAPE;
  else if (equal_ustring(lazy.type, "FTP"))
    dev_type = LOG_TYPE_FTP;
  else
  {
    cm_msg(MERROR,"Lazy","Unknown device type (Disk, Tape, FTP)");
    return DB_NO_ACCESS;
  }

  /* make sure that we don't operate on the current DAQ file if so set to oldest */
  if (lazy.staybehind == 0)
  {
    cm_msg(MERROR,"Lazy","Stay behind cannot be 0");
    return NOTHING_TODO;
  }

  /* Check if Tape is OK */
  /* ... */
  
  /* check if space on device (none empty tape label) */
  if (lazy.backlabel[0] == '\0')
  {
    full_bck_flag = TRUE;
    return NOTHING_TODO;
  }
  else
  {
    if (full_bck_flag)
    {
      full_bck_flag = FALSE;
      size = sizeof(lazyst);
      memset(&lazyst,0,size);
      if (db_find_key(hDB, pLch->hKey, "Statistics",&hKeyst) == DB_SUCCESS)
      	status = db_set_record(hDB, hKeyst, &lazyst, size, 0);
      else
	      cm_msg(MERROR,"lazy_main","did not find /Lazy/Lazy_%s/Statistics for zapping", pLch->name);
    }
  }
  /* check if data dir is none empty */
  if (lazy.dir[0] == '\0')
  {
    cm_msg(MINFO,"Lazy","Please setup Dir data for input source path!");
    return NOTHING_TODO;
  }

  /* check if device path is set */
  if (lazy.path[0] == '\0')
  {
    cm_msg(MINFO,"Lazy","Please setup backup device path too!");
    return NOTHING_TODO;
  }
  
   /* build logger dir listing */
  if (pdirlog == NULL) pdirlog = malloc(sizeof(DIRLOG));
  build_log_list(lazy.backfmt, lazy.dir, &pdirlog);
  
  /* build donelist comparing pdirlog and /Lazy/List */
  if (pdonelist == NULL) pdonelist = malloc(sizeof(INT));
  build_done_list(pLch->hKey, &pdonelist);
  
  /* compare list : run NOT in donelist AND run in dirlog */
  tobe_backup = cmp_log2donelist (pdirlog, pdonelist);
  /* cleanup memory */
  if (pdirlog)   free (pdirlog); pdirlog = NULL;
  if (pdonelist) free (pdonelist); pdonelist = NULL;

  if (tobe_backup < 0)
    return NOTHING_TODO;
  
  /* ckeck if mode is on oldest (staybehind = -x)
     else take current run - keep file */
  if (lazy.staybehind < 0)
    lazyst.cur_run = tobe_backup;
  else
    lazyst.cur_run -= lazy.staybehind;
  
  /* Get current run number */
  size = sizeof(cur_acq_run);
  db_get_value (hDB, 0, "Runinfo/Run number", &cur_acq_run, &size, TID_INT);
  
  /* update "maintain free space */
  lazy_maintain_check(pLch->hKey, pLall);

  /***** Lock other clients out of this following code as
         it may accessing the other client info tree   *****/
  status = ss_mutex_wait_for(lazy_mutex, 5000);
  if (status != SS_SUCCESS)
  {
    /* exit for now and come back later */
    if (pdirlog != NULL)   free (pdirlog); pdirlog = NULL;
    if (pdonelist != NULL) free (pdonelist); pdonelist = NULL;

    return NOTHING_TODO;
  }

  /* check SPACE and purge if necessary = % (1:99) */
  if (lazy.pupercent > 0)
  {
    if (pdirlog == NULL) pdirlog = malloc(sizeof(DIRLOG));
    /* cleanup memory */
    donepurge = FALSE;
    svfree = freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);

    
    /* check purging action */
    while (freepercent <= (double)lazy.pupercent)
    {
      /* search file to purge : run in donelist AND run in dirlog */
      /* synchronize donelist to dir log in case user has purged
         some file by hand. Basically remove the run in the list if the
         file is not anymore in the source dir. */
      if (lazy_select_purge(pLch->hKey, channel, pLall, lazy.backfmt, lazy.dir, pufile, &purun) == 0)
      {
        /* check if beyond keep file + 1 */
        if (purun < (cur_acq_run - abs(lazy.staybehind) - 1 ))
        {
          /* remove file */
          status = lazy_file_remove(pufile);
          if (status == SS_FILE_ERROR)
              cm_msg(MERROR, "Lazy","lazy_file_remove failed on file %s",pufile);
          else
          {
            status = lazy_log_update(REMOVE_FILE, purun, NULL, pufile);
            donepurge = TRUE;

            /* update donelist (remove run entry as the file has been deleted */
            if ((status=lazy_remove_entry(channel, pLall, purun)) != 0)
              cm_msg(MERROR, "Lazy","remove_entry not performed %d",status);
          }
        }
        freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);
        if ((INT)svfree == (INT)freepercent)
          break;
      }
      else
      {
        if (donepurge)
          cm_msg(MINFO,"Lazy","Can't purge more for now!");
        break;
      }
    }
  } /* end of if pupercent > 0 */
  
  /* cleanup memory */
  if (pdirlog != NULL)   free (pdirlog); pdirlog = NULL;
  if (pdonelist != NULL) free (pdonelist); pdonelist = NULL;
  
  /***** Release the mutex  *****/
  status = ss_mutex_release(lazy_mutex);

  /* check if backup run is beyond keep */
  if (lazyst.cur_run >= (cur_acq_run - abs(lazy.staybehind)))
    return NOTHING_TODO;

  /* Compose the proper file name */
  status = lazy_file_compose(lazy.backfmt , lazy.dir , lazyst.cur_run , inffile, lazyst.backfile);
  if (status != 0)
    {
      cm_msg(MERROR,"Lazy","composition of file failed -%s-%s-%d-"
             ,lazy.backfmt, lazy.dir, lazyst.cur_run); 
      return status;
    }

  /* Check again if the backup file is present in the logger dir */
  if (lazy_file_exists(lazy.dir, lazyst.backfile))
  {
    /* compose the destination file name */
    if (dev_type == LOG_TYPE_DISK)
    {
      if (lazy.path[0] != 0)
        if (lazy.path[strlen(lazy.path)-1] != DIR_SEPARATOR)
          strcat(lazy.path, DIR_SEPARATOR_STR);
      strcpy(outffile, lazy.path); 
      strcat(outffile, lazyst.backfile);
    }
    else if(dev_type == LOG_TYPE_TAPE)
      strcpy(outffile, lazy.path); 
    else if(dev_type == LOG_TYPE_FTP)
    {
      if (lazy.path[0] != 0)
        if (lazy.path[strlen(lazy.path)-1] != DIR_SEPARATOR)
          strcat(lazy.path, DIR_SEPARATOR_STR);
      strcpy(outffile, lazy.path); 
      strcat(outffile, ",");
      strcat(outffile, lazyst.backfile);
    }

    /* check if space on backup device */
    if (lazy.capacity < (lazyst.cur_dev_size + lazyst.file_size))
    {
      /* not enough space => reset list label */
      lazy.backlabel[0]='\0';
      size = sizeof(lazy.backlabel);
      db_set_value (hDB, pLch->hKey, "Settings/List label"
                    , lazy.backlabel, size, 1, TID_STRING);
      full_bck_flag = TRUE;
      cm_msg(MINFO,"Lazy","Not enough space for next copy on backup device!");
      
      /* rewind device if TAPE type */
      if(dev_type == LOG_TYPE_TAPE)
      {
        int channel;

        cm_msg(MINFO,"Lazy","backup device rewinding...");
        ss_tape_open(outffile, O_RDONLY, &channel);
        cm_get_watchdog_params(&watchdog_flag, &watchdog_timeout);
        cm_set_watchdog_params(watchdog_flag, 300000);  /* 5 min for tape rewind */
        ss_tape_unmount(channel);
        ss_tape_close(channel);
        cm_set_watchdog_params(watchdog_flag, watchdog_timeout);

        /* Setup alarm */
        lazy.alarm[0] = 0;
        size = sizeof(lazy.alarm);
        db_get_value(hDB, pLch->hKey,  "Settings/Alarm Class", lazy.alarm, &size, TID_STRING);
      
        /* trigger alarm if defined */
        if (lazy.alarm[0])
          al_trigger_alarm("Tape", "Tape full, Please remove current tape and load new one!", lazy.alarm,
                           "Tape full", AT_INTERNAL);
        return NOTHING_TODO;
      }
    }
    
    /* Finally do the copy */
    if ((status = lazy_copy(outffile, inffile)) != 0)
    {
      if (status == FORCE_EXIT)
        return status;
      cm_msg(MERROR,"Lazy","copy failed -%s-%s-%i",lazy.path, lazyst.backfile, status);
      return NOTHING_TODO;
    }
  } /* file exists */
  else
  {
    cm_msg(MERROR,"Lazy","lazy_file_exists file %s doesn't exists",lazyst.backfile);
    return NOTHING_TODO;
  }              

  /* Update the list */
  if ((status = lazy_update_list(pLch)) != DB_SUCCESS)
  {
    cm_msg(MERROR,"Lazy","lazy_update failed");
    return NOTHING_TODO;
  }
  return NOTHING_TODO;
}

/*------------------------------------------------------------------*/
int main(unsigned int argc,char **argv)
{
  char      channel_name[32];
  char      host_name[HOST_NAME_LENGTH];
  char      expt_name[HOST_NAME_LENGTH];
  BOOL      debug, daemon;
  INT       msg, ch, size, status, mainlast_time;
  DWORD     i;
  
  /* set default */
  host_name[0] = 0;
  expt_name[0] = 0;
  channel_name[0] = 0;
  zap_flag = FALSE;
  msg_flag = FALSE;
  debug = daemon = FALSE;
  
  /* set default */
  cm_get_environment (host_name, expt_name);
  
  /* get parameters */
  for (i=1 ; i<argc ; i++)
  {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (strncmp(argv[i],"-z",2) == 0)
      zap_flag = TRUE;
    else if (strncmp(argv[i],"-t",2) == 0)
      msg_flag = TRUE;
    else if (argv[i][0] == '-')
    {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (strncmp(argv[i],"-e",2) == 0)
        strcpy(expt_name, argv[++i]);
      else if (strncmp(argv[i],"-h",2)==0)
        strcpy(host_name, argv[++i]);
      else if (strncmp(argv[i],"-c",2)==0)
        strcpy(channel_name, argv[++i]);
    }
    else
    {
    usage:
    printf("usage: lazylogger [-h <Hostname>] [-e <Experiment>]\n");
    printf("                  [-z zap statistics] [-t (talk msg)\n");
    printf("                  [-c channel name (Disk) -D to start as a daemon\n\n");
    printf(" Quick man :\n");
    printf(" The Lazy/Settings tree is composed of the following parameters:\n");
    printf(" Maintain free space [%](0): purge source device to maintain free space on the source directory\n");
    printf("                       (0) : no purge      \n");
    printf(" Stay behind  (-3)         : If negative number : lazylog runs starting from the OLDEST\n");
    printf("                              run file sitting in the 'Dir data' to the current acquisition\n");
    printf("                              run minus the 'Stay behind number'\n");
    printf("                             If positive number : lazylog starts from the current\n");
    printf("                              acquisition run minus 'Stay behind number' \n");
    printf(" Alarm Class               : Specify the Class to be used in case of Tape Full condition\n");
    printf(" Running condition         : active/deactive lazylogger under given condition i.e:\n");
    printf("                            'ALWAYS' (default)     : Independent of the ACQ state ...\n");
    printf("                            'NEVER'                : ...\n");
    printf("                            'WHILE_ACQ_NOT_RUNNING': ...\n");
    printf("                            '/alias/max_rate < 200'    (max_rate is a link)\n");
    printf("                            '/equipment/scaler/variables/scal[4] < 23.45'\n");
    printf("                            '/equipment/trigger/statistics/events per sec. < 400'\n");
    printf(" Data dir                  : Directory of the run to be lazylogged \n");
    printf(" Data format               : Data format (YBOS/MIDAS)\n");
    printf(" Filename format           : Run format i.e. run%05d.mid \n");
    printf(" List label                : Label of destination save_set.\n");
    printf("                             Prevent lazylogger to run if not given.\n");
    printf("                             Will be reset if maximum capacity reached.\n");
    printf(" Backup type               : Destination device type (Disk, Tape, FTP)\n");
    printf(" Path                      : Destination path (file.ext, /dev/nst0, ftp...)\n");
    printf("                             in case of FTP type, the 'Path' entry should be:\n");
    printf("                             host, port, user, password, directory, run%05d.mid\n");
    printf(" Capacity (Bytes)          : Maximum capacity of the destination device.\n");
     return 0;
    }
  }

  /* Handle SIGPIPE signals generated from errors on the pipe */
#ifdef SIGPIPE  
  signal( SIGPIPE, SIG_IGN );
#endif
  if (daemon)
  {
    printf("Becoming a daemon...\n");
    ss_daemon_init();
  }

  /* connect to experiment */
  status = cm_connect_experiment1(host_name, expt_name, "Lazy_Tape", 0, DEFAULT_ODB_SIZE, WATCHDOG_TIMEOUT);
  if (status != CM_SUCCESS)
    return 1;

  /* create a common mutex for the independent lazylogger */
  status = ss_mutex_create("LAZY", &lazy_mutex);

  /* check lazy status for multiple channels */
  cm_get_experiment_database(&hDB, &hKey);
  if (db_find_key(hDB, 0, "/Lazy", &hKey) == DB_SUCCESS)
  {
    HNDLE hSubkey;
    KEY key;
    char strclient[32];
    INT  j=0;
    for (i=0; ; i++)
    {
	    db_enum_key(hDB, hKey, i, &hSubkey);
	    if (!hSubkey)
	      break;
	    db_get_key(hDB, hSubkey, &key);
	    if (key.type == TID_KEY)
	    {
        /* compose client name */
        sprintf(strclient, "Lazy_%s", key.name);
        if (cm_exist(strclient,TRUE) == CM_SUCCESS)
         lazyinfo[j].active = TRUE;
        else
          lazyinfo[j].active = FALSE;
        strcpy(lazyinfo[j].name, key.name);

        lazyinfo[j].hKey = hSubkey;
        j++;
      }
    }
  }  
  else
  {
    /* create settings tree */	
    char str[32];
    channel = 0;
    if (channel_name[0] != 0) sprintf(lazyinfo[channel].name, channel_name);
    sprintf(str, "/Lazy/%s/Settings", lazyinfo[channel].name);
    db_create_record(hDB, 0, str, LAZY_SETTINGS_STRING);
  }

  {/* Selection of client */
    INT i, j;
    char str[32];

    if (lazyinfo[0].hKey)
    {
      if (channel_name[0] == 0)
      {
        /* command prompt */
        printf(" Available Lazy channels to connect to:\n");
        i=0;
        j=1;
        while (lazyinfo[i].hKey)
        {
          if (!lazyinfo[i].active)
            printf("%d) Lazy %s \n", j, lazyinfo[i].name);
          else 
            printf(".) Lazy %s already active\n", lazyinfo[i].name);
          j++; i++;
        }
        printf("Enter client number or new lazy client name: ");
        i = atoi(ss_gets(str, 32));
        if ((i==0) && ((strlen(str) == 0) || (strncmp(str," ",1) == 0)))
        {
          cm_msg(MERROR,"Lazy","Please specify a valid channel name (%s)",str);
          goto error;
        }
      }
      else
      {
        /* Skip the command prompt for serving the -c option */
        /* 
        scan if channel_name already exists 
        Yes : check if client is running
              Yes : get out (goto error)
              No  : connect (extract index)
        No  : new channel (i=0)
        */
        i=0; j=-1;
        while (lazyinfo[i].hKey)
        {
          if (equal_ustring(channel_name, lazyinfo[i].name))
          {
            /* correct name => check active  */
            if (lazyinfo[i].active)
            {
              cm_msg(MERROR,"Lazy","Lazy channel ""%s"" already running!", lazyinfo[i].name);
              goto error;
            }     
            j=i;
          }
          i++;
        }
        if (j==-1)
        {
          /* new entry */
          i = 0;
          sprintf(str, "%s", channel_name);
        }
        else
        {
          /* connect to */
          i = j+1;
        }
      }
      if (i == 0)
      { /* new entry */
        char strclient[32];
        for (j=0 ; j < MAX_LAZY_CHANNEL; j++)
        {
          if (lazyinfo[j].hKey == 0)
          {
            /* compose client name */
            sprintf(strclient, "Lazy_%s", str);
            if (cm_exist(strclient,TRUE) == CM_SUCCESS)
              lazyinfo[j].active = TRUE;
            else
              lazyinfo[j].active = FALSE;
            strcpy(lazyinfo[j].name, str);
            lazyinfo[j].hKey = 0;
            channel = j;
            break;
          }
        }
      }
      else if (!lazyinfo[i-1].active)
        channel = i-1;
      else
        channel = -1;
    } 

    if ((channel < 0) && (lazyinfo[channel].hKey != 0))
      goto error;
    if (channel < 0)
      channel = 0;

    { /* creation of the lazy channel */
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
  
  { /* reconnect to experiment with proper name */
    char str[32];
    sprintf(str, "Lazy_%s", lazyinfo[channel].name);
    status = cm_connect_experiment1(host_name, expt_name, str, 0, DEFAULT_ODB_SIZE, WATCHDOG_TIMEOUT);
  }
  if (status != CM_SUCCESS)
    goto error;

  cm_get_experiment_database(&hDB, &hKey);

  /* turn on keepalive messages with increased timeout */
  if (debug)
    cm_set_watchdog_params(TRUE, 0);
  
  printf("Lazy_%s starting... ""!"" to exit \n", lazyinfo[channel].name);

  if (zap_flag)
  {
    /* reset the statistics */
    cm_msg(MINFO,"lazy","zapping %s/statistics content", lazyinfo[channel].name);
    size = sizeof(lazyst);
    memset(&lazyst,0,size);
    if (db_find_key(hDB, lazyinfo[channel].hKey, "Statistics",&hKeyst) == DB_SUCCESS)
      status = db_set_record(hDB, hKeyst, &lazyst, size, 0);
    else
      cm_msg(MERROR,"Lazy","did not find %s/Statistics for zapping", lazyinfo[channel].name);
  }
  
  /* get value once & hot links the run state */
  db_find_key(hDB,0,"/runinfo/state",&hKey);
  size = sizeof(run_state);
  db_get_data(hDB, hKey, &run_state, &size, TID_INT);
  status = db_open_record(hDB, hKey, &run_state, sizeof(run_state), MODE_READ, NULL, NULL);
  if (status != DB_SUCCESS)
  {
    cm_msg(MERROR, "run_state", "cannot open variable record");
  }
  /* hot link for statistics in write mode */
  size = sizeof(lazyst);
  if (db_find_key(hDB,lazyinfo[channel].hKey,"Statistics",&hKey) == DB_SUCCESS)
    db_get_record(hDB, hKey, &lazyst, &size, 0);
  status = db_open_record(hDB, hKey, &lazyst, sizeof(lazyst), MODE_WRITE, NULL, NULL);
  if (status != DB_SUCCESS)
  {
    cm_msg(MERROR, "%s/Statistics", "cannot open variable record", lazyinfo[channel].name);
  }
  /* get /settings once & hot link settings in read mode */
  db_find_key(hDB,lazyinfo[channel].hKey,"Settings",&hKey);
  size = sizeof(lazy);
  status = db_open_record(hDB, hKey, &lazy, sizeof(lazy), MODE_READ, lazy_settings_hotlink, NULL);
  if (status != DB_SUCCESS)
  {
    cm_msg(MERROR, "%s/Settings", "cannot open variable record", lazyinfo[channel].name);
  }

  /* set global key for that channel */
  pcurrent_hKey = lazyinfo[channel].hKey;

  /* set Data dir from /logger if local is empty & /logger exists */
  if ((lazy.dir[0] == '\0') &&
      (db_find_key(hDB,0,"/Logger/Data dir", &hKey) == DB_SUCCESS))
  {
    size = sizeof(lazy.dir);
    db_get_data(hDB, hKey, lazy.dir, &size, TID_STRING);
    db_set_value(hDB, lazyinfo[channel].hKey, "Settings/Data dir", lazy.dir, size, 1, TID_STRING);
  }

  mainlast_time = 0;

  /* initialize ss_getchar() */
  ss_getchar(0);

  do
  {
    msg = cm_yield(2000);
    if ((ss_millitime() - mainlast_time) > 10000)
    {
      status = lazy_main(channel, &lazyinfo[0]);
      mainlast_time = ss_millitime();
    }      
    ch = 0;
    while (ss_kbhit())
    {
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


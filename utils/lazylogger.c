/********************************************************************\

  Name:         lazylogger.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Disk to Tape copier for background job

  $Log$
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

typedef struct {
  INT run;
  double size;
} DIRLOG;

DIRLOG *pdirlog;
INT    *pdonelist;

#define LAZY_SETTING_STRING "\
Active = BOOL : 1\n\
Maintain free [%] = INT : 0\n\
Copy while running = BOOL : 1\n\
Stay behind = INT : -3\n\
Lazy running condition = STRING : [128]\n\
Index condition = INT : 0\n\
comparison operator = STRING : [8]\n\
Threshold condition = FLOAT : 0.0\n\
Dir data  = STRING : [128] \n\
Data format = STRING : [8] MIDAS\n\
Filename format = STRING : [128] run%05d.mid\n\
List label= STRING : [128]\n\
Backup type = STRING : [8] Tape\n\
Path = STRING : [128] /dev/nst0\n\
Capacity (Bytes) = DOUBLE : 5e9\n\
"
#define LAZY_STATISTICS_STRING "\
File size [KB] = DOUBLE : 0.0\n\
KBytes copied = DOUBLE : 0.0\n\
Total KBytes copied = DOUBLE : 0.0\n\
Backup file = STRING : [128] none \n\
Copy progress [%] = FLOAT : 0\n\
Copy Rate [KB per sec] = FLOAT : 0\n\
Backup status [%] = FLOAT : 0\n\
Number of Files = INT : 0\n\
Current Lazy run = INT : 0\n\
Spare = INT : 0\n\
"

typedef struct {
  BOOL active;                   /* will prevent further copy (complete run anyway) */
  INT pupercent;                 /* 0:100 % of disk space to keep free */
  BOOL cpwhilerun;               /* 0: prevent copy while acq is running */
  INT staybehind;                /* keep x run file between current Acq and backup 
                                        -x same as x but starting from oldest */
  char condition[128];           /* key condition */
  INT  condition_index;          /* if the key condition is an element of an array */
  char condition_type[8];        /* condition operator: < or > or = */
  float condition_limit;         /* condition threshold */
  char dir[MAX_FILE_PATH];       /* path to the data dir */
  char format[8];                /* Data format (YBOS, MIDAS) */
  char backfmt[MAX_FILE_PATH];   /* format for the run files run%05d.mid */
  char backlabel[MAX_FILE_PATH]; /* label of the array in ~/list. if empty like active = 0 */
  char type[8];                  /* Backup device type  (Disk, Tape, Ftp) */
  char backdev[MAX_FILE_PATH];   /* backup device name */
  double bck_dev_size;           /* backup device max byte size */
} LAZY_SETTING;
LAZY_SETTING  lazy;

typedef struct {
double file_size;              /* file bytes size */
double tot_cp_size;            /* current bytes copied */
double dev_tot_cp_size;        /* Total bytes backup on device */
char backfile[MAX_FILE_PATH];  /* current or last lazy file done (for info only) */
float progress;                /* copy % */
float cprate;                  /* copy rate */
float bckfill;                 /* backup fill % */
INT nfiles;                    /* # of backuped files */
INT cur_run;                   /* current or last lazy run number done (for info only) */
INT spare;                     /* current or last lazy run number done (for info only) */
} LAZY_STATISTICS;
LAZY_STATISTICS lazyst;

/* Globals */
void      *plazy;
DWORD     szlazy;
HNDLE     hDB, hKey;
double    lastsz;
HNDLE     hKeyst;
INT       tot_do_size, tot_dirlog_size, hDev;
BOOL      zap_flag, msg_flag;
BOOL      copy_continue = TRUE;
INT       data_fmt, dev_type;
char      lazylog[MAX_STRING_LENGTH];

/* prototypes */
BOOL lazy_file_exists(char * dir, char * file);
INT  lazy_main (HNDLE hDB, HNDLE hKey);
INT  lazy_copy( char * dev, char * file);
INT  lazy_file_compose(char * fmt , char * dir, INT num, char * ffile, char * file);
INT  lazy_update_list(void);
INT  lazy_select_purge(DIRLOG * plog, INT * pdo, char * pufile, INT * run);
INT  lazy_load_params( HNDLE hDB, HNDLE hKey );
void build_log_list(char * fmt, char * dir, DIRLOG ** plog);
void sort_log_list(DIRLOG * plog);
void build_donelist(INT **pdo);
void sort_donelist(INT * pdo);
INT  cmp_log2donelist (DIRLOG * plog, INT * pdo);
INT lazy_log_update(INT action, INT index, INT run, char * label, char * file);

/*------------------------------------------------------------------*/
INT lazy_log_update(INT action, INT index, INT run, char * label, char * file)
{
  time_t   full_time;
  FILE *pF;
  char str[MAX_FILE_PATH];
  char strtime[25];
  BOOL addtitle=FALSE;

  /* update lazy.log */
    sprintf(lazylog, "%s/lazy.log", lazy.dir);
    if ((pF=fopen(lazylog, "r")) == NULL)
      addtitle = TRUE;
    if (pF != NULL) fclose (pF);        
      pF = fopen(lazylog, "a");
    if (pF)
    {
      if (addtitle)
        {         
        sprintf(str,"Date , List label[file#] , Device name , Run# , file name , file size[KB]\n");
          fputs (str,pF);
          sprintf(str,"------------------------------------------------------------------\n");
          fputs (str,pF);
        }
      time(&full_time);
      strcpy(strtime, ctime(&full_time));
      strtime[24] = 0;
      if (action == NEW_FILE)
        {
          sprintf(str,"%s , %s[%i] , %s , %i , %s , %9.2e , file  NEW\n"
            , strtime, label, lazyst.nfiles
            ,lazy.backdev, run, lazyst.backfile, lazyst.file_size/1024.);
          fputs (str,pF);
        }
      else if (action == REMOVE_FILE)
        {
          sprintf(str,"%s ,       ,     , %i , %s ,  , file  REMOVED\n"
            , strtime, run, file);
          fputs (str,pF);
        }
      else if (action == REMOVE_ENTRY)
        {
          sprintf(str,"%s , %s[%i],     ,  %i ,   ,  , entry REMOVED\n"
            , strtime, label, index,  run);
          fputs (str,pF);
        }
        fclose (pF);
      return 0;
    }
    else
    {
      cm_msg(MERROR,"Lazy","cannot open %s",lazylog);
      return -1;  
    }
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
INT lazy_load_params( HNDLE hDB, HNDLE hKey )
/********************************************************************\
  Routine: lazy_load_params
  Purpose: loads the /Lazy/settings into the global struct lazy.
  Input:
   HNDLE hDB   Database handle (not used here but could be hot linked)
   HNDLE hKey  Key handle (not used here but could be hot linked)
  Output:
  Function value:
   INT        DB_SUCCESS
              DB_NO_ACCESS
\********************************************************************/
{
  INT status, size;
  
  /* check if dir exists */
    if (db_find_key(hDB, 0, "Lazy/Settings", &hKey) != DB_SUCCESS)
    {
      if (db_find_key(hDB, 0, "Lazy", &hKey) == DB_SUCCESS)
      {
        status = db_create_record(hDB, hKey, "Settings", LAZY_SETTING_STRING);
        if (status != DB_SUCCESS)
        {
          cm_msg(MINFO, "Lazy", "cannot create Lazy structure");
          return DB_SUCCESS;
        }
      }
      else
      return DB_NO_KEY;
    }
  size = sizeof(lazy);
  db_find_key(hDB, 0, "/Lazy/Settings", &hKey);
  status = db_get_record(hDB, hKey, &lazy, &size, 0);
  if (status != DB_SUCCESS)
    {
      cm_msg(MINFO, "Lazy", "cannot read Lazy structure");
      return DB_NO_ACCESS;
    }
  
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
    lazy.staybehind = -1;
  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
void build_log_list(char * fmt, char * dir, DIRLOG ** plog)
/********************************************************************\
  Routine: build_log_list
  Purpose: build an internal directory file list from the disk directory
  Input:
    char     * fmt     format of the file to search for (ie:run%05d.ybs) 
    char     * dir     path to the directory for the search
  Output:
    INT     **plog     internal file list struct
  Function value:
\********************************************************************/
{
  INT nfile, j;
  char * list = NULL;
  char str[MAX_FILE_PATH];
  
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
}

/*------------------------------------------------------------------*/
void sort_log_list(DIRLOG * plog)
/********************************************************************\
  Routine: sort_log_list
  Purpose: sort plog struct by run number
  Input:
    INT     *pdo     /lazy/list run listing
  Output:
  Function value: void
\********************************************************************/
{
  DIRLOG temp;
  INT j, i, nelement;
  
  nelement = tot_dirlog_size / sizeof(DIRLOG);
  for (j=0;j<nelement;j++)
  {
    for (i=j; i<nelement ; i++)
    {
      if ((plog+j)->run > (plog+i)->run)
      {
        memcpy (&temp, (plog+i), sizeof(DIRLOG));
        memcpy ((plog+i), (plog+j), sizeof(DIRLOG));
        memcpy ((plog+j), &temp, sizeof(DIRLOG));
      }
    }
  }
}

/*------------------------------------------------------------------*/
void build_donelist(INT **pdo)
/********************************************************************\
  Routine: build_donelist
  Purpose: build an internal /lazy/list list (pdo) tree
  Input:
    INT     **pdo     /lazy/list run listing
  Output:
    INT     **pdo     /lazy/list run listing
  Function value:      void
\********************************************************************/
{
  HNDLE hKey, hSubkey;
  KEY    key;
  INT i, size, nelement, tot_nelement;
  
  if (db_find_key(hDB, 0, "/Lazy/List", &hKey) != DB_SUCCESS)
    {
      *pdo[0] = 0;
      return;
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
  return;
}

/*------------------------------------------------------------------*/
void sort_donelist(INT * pdo)
/********************************************************************\
  Routine: sort_donelist
  Purpose: sort the full /lazy/list run number in increasing order
  Input:
    INT     *pdo     /lazy/list run listing
  Output:
  Function value:
    void
\********************************************************************/
{
  INT temp;
  INT j, i, nelement;
  
  nelement = tot_do_size / sizeof(INT);
  for (j=0;j<nelement-1;j++)
  {
    for (i=j+1; i<nelement ; i++)
    {
      if (*(pdo+j) > *(pdo+i))
      {
        memcpy (&temp, (pdo+i), sizeof(INT));
        memcpy ((pdo+i), (pdo+j), sizeof(INT));
        memcpy ((pdo+j), &temp, sizeof(INT));
      }
    }
  }
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
INT lazy_select_purge(DIRLOG * plog, INT * pdo, char * fpufile, INT * run)
/********************************************************************\
  Routine: lazy_select_purge
  Purpose: Search oldest run number which can be purged
           condition : oldest run# in (pdo AND present in plog)
  Input:
   DIRLOG * plog  internal dir file listing
   INT * pdo      internal /lazy/list/ run listing
  Output:
   char * pufile  file to be purged
   INT * run      corresponding run# to be purged
  Function value:
    0          success
   -1          run not found
\********************************************************************/
{
  INT j, i, ndo, nlog, status;
  char pufile[MAX_FILE_PATH];

  /* Scan donelist from first element (oldest)
     check if run exists in dirlog
     if yes return file and run number */
  
  ndo = tot_do_size / sizeof(INT);
  nlog = tot_dirlog_size / sizeof(DIRLOG);
  for (i=0;i<ndo;i++)
    {
      for (j=0;j<nlog;j++)
        {
          if (pdo[i] == abs((plog+j)->run))
            {
              status = lazy_file_compose(lazy.backfmt, lazy.dir, abs((plog+j)->run), fpufile, pufile);
              *run = abs((plog+j)->run);
              return 0;
            }     
        }
    }
  return -1;
}

/*------------------------------------------------------------------*/
int lazy_remove_entry(int run)
/********************************************************************\
  Routine: lazy_remove_entry
  Purpose: remove run entry in the /lazy/list/<label>
  Input:
  char * label     : device label under which the run should be present
  int    run       : run number to be removed
  Output:

  Function value:
    0             success
    -1            run not found
\********************************************************************/
{
  KEY    key;
  HNDLE  hKey, hSubkey;
  INT    i, j, size, nelement, saveindex=0;
  INT    *prec;
  BOOL   found;
  
  
  /* check list */
  if (db_find_key(hDB, 0, "/Lazy/list", &hKey) != DB_SUCCESS)
      return 0;
  
  /* book tmp space */
  prec = (INT *)malloc(sizeof(INT));

  /* loop over all list entries */
  found = FALSE;
  for (i=0 ; ; i++)
  {
    db_enum_key(hDB, hKey, i, &hSubkey);
    if (!hSubkey)
      break;
    db_get_key(hDB, hSubkey, &key);
    size = key.total_size;
    nelement = key.num_values;
    prec = (INT *)realloc(prec, size);
    db_get_record(hDB, hSubkey, (char *) prec, &size, 0);
    /* match run number */
    for (j=0; j<nelement; j++)
      {
        if (*(prec+j) == run)
          {
            found = TRUE;
	    saveindex = j;
          }
	if (found)
	  if (j+1 < nelement)
	    *(prec+j) = *(prec+j+1);
      }
    /* delete label[] or update label[] */
    if (found)
      {
	nelement--;
        if (nelement > 0)
          db_set_data(hDB,hSubkey,prec,nelement*sizeof(INT), nelement, TID_INT);
        else
          db_delete_key(hDB,hSubkey,FALSE);
        free (prec);
        lazy_log_update(REMOVE_ENTRY, saveindex, run, key.name, NULL);
        return 0;
      }
  }
  free (prec);
  return -1;
}
/*------------------------------------------------------------------*/
INT lazy_update_list(void)
/********************************************************************\
  Routine: lazy_update_list
  Purpose: update the /lazy/list tree with the info from lazy{}
  Input:
  Output:
  Function value:
   DB_SUCCESS   : update ok
\********************************************************************/
{
  INT  size;
  char * ptape = NULL, str[MAX_FILE_PATH];
  KEY Keylabel;
  INT  mone=-1;

  HNDLE hKeylabel;
  
  /* check if dir exists */
  if (db_find_key(hDB, 0, "/Lazy/List", &hKey) != DB_SUCCESS)
    { /* list doesn't exists */
      sprintf(str,"/Lazy/List/%s",lazy.backlabel);
      db_set_value(hDB, 0, str, &mone, sizeof(INT), 1, TID_INT);
      db_find_key(hDB, 0, "/Lazy/List", &hKey);
    } 

  /*  record the saved run */
  if (db_find_key(hDB, hKey, lazy.backlabel, &hKeylabel) == DB_SUCCESS)
    { /* run array already present ==> append run */
      db_get_key(hDB, hKeylabel, &Keylabel);
      lazyst.nfiles = Keylabel.num_values; 
      ptape = malloc(Keylabel.total_size);
      size = Keylabel.total_size;
      db_get_data(hDB, hKeylabel, ptape, &size, TID_INT);
      if ((size == sizeof(INT)) && (*ptape == -1))
        {
          lazyst.nfiles = 1;
          size = sizeof(INT);
          memcpy((char *)ptape, (char *)&lazyst.cur_run, sizeof(INT));
        }
      else
        {
          ptape = realloc((char *)ptape, size+sizeof(INT));
          memcpy((char *)ptape+size, (char *)&lazyst.cur_run, sizeof(INT));
          lazyst.nfiles += 1;
          size += sizeof(INT);
        }
      db_set_data (hDB, hKeylabel, (char *)ptape, size, lazyst.nfiles, TID_INT);
      size = sizeof(lazyst.nfiles);
      db_set_value(hDB, 0,"/Lazy/Statistics/Number of Files", &lazyst.nfiles, size, 1, TID_INT);
    }
  else
    { /* new run array ==> */
      size = sizeof(lazyst.cur_run);
      db_set_value(hDB, hKey, lazy.backlabel, &lazyst.cur_run, size, 1, TID_INT);
    }
  lazy_log_update(NEW_FILE, 0, lazyst.cur_run, lazy.backlabel, lazyst.backfile);
  if (msg_flag) cm_msg(MUSER,"Lazy","         lazy job %s done!",lazyst.backfile);
  if (ptape)
    free (ptape);
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
void lazy_statistics_update(double *lastsz, INT cploop_time)
/********************************************************************\
  Routine: lazy_statistics_update
  Purpose: 
  Input:
  Output:
  Function value:
        0           success
\********************************************************************/
{
  INT size;
  double dtemp;

    /* update rate statistics */
    size = sizeof(float);
    lazyst.cprate = (float)(lazyst.tot_cp_size - *lastsz);
    lazyst.cprate /= (ss_millitime() - cploop_time);
    db_set_value (hDB, 0, "/Lazy/Statistics/Copy Rate [KB per sec]"
                  , &lazyst.cprate, size, 1, TID_FLOAT);
    *lastsz = lazyst.tot_cp_size;
    /* update KB copied statistics */
    size = sizeof(double);
    dtemp = lazyst.tot_cp_size / 1024.;
    db_set_value (hDB, 0, "/Lazy/Statistics/KBytes copied"
                  , &dtemp, size, 1, TID_DOUBLE);
    /* update Total KB copied statistics */
    size = sizeof(double);
    dtemp = lazyst.dev_tot_cp_size / 1024.;
    db_set_value (hDB, 0, "/Lazy/Statistics/Total KBytes copied"
                  , &dtemp, size, 1, TID_DOUBLE);
    /* update % statistics */
    if (lazyst.file_size != 0)
      {
        size = sizeof(float);
        lazyst.progress = (float) (100. * (lazyst.tot_cp_size / lazyst.file_size));
        db_set_value (hDB, 0, "/Lazy/Statistics/Copy progress [%]"
                      , &lazyst.progress, size, 1, TID_FLOAT);
      }

    /* update backup % statistics */
    if (lazy.bck_dev_size > 0)
      {
        lazyst.bckfill = (float) ( 100. * (lazyst.dev_tot_cp_size / lazy.bck_dev_size));
        db_set_value (hDB, 0, "/Lazy/Statistics/Backup status [%]"
                      , &lazyst.bckfill, size, 1, TID_FLOAT);
      }
}


/*------------------------------------------------------------------*/
void lazy_condition_check(INT msg, BOOL * copy_continue)
/********************************************************************\
  Routine: lazy_condition_check
  Purpose: Check if the copy should continue.
  !copy_continue if  : NO copy_while_running && RUNNING && NO user condition
                  || : copy_while_running && RUNNING && user condition OK
  Input:
  INT msg       : do cm_msg about chnge of state due to user condition
  BOOL * copy_continue : current copy condition
  Output:
  BOOL * copy_continue : new copy condition
  Function value:
\********************************************************************/
{
  INT status, size, cur_state;
  char str[128], condition_key[128];
  double lcond_value;
  KEY key;
  BOOL  previous_continue;
  static BOOL ucond=FALSE;

  /* Get current running state */
  size = sizeof(cur_state);
  db_get_value (hDB, 0, "Runinfo/State", &cur_state, &size, TID_INT);

  /* Get updated "copy while running" */
  size = sizeof(lazy.cpwhilerun);
  db_get_value (hDB, 0, "/Lazy/Settings/Copy while running", &lazy.cpwhilerun, &size, TID_BOOL);

  /* check if running */
  if ((lazy.cpwhilerun) && (cur_state == STATE_RUNNING))
  {
    /* Check if condition has to be verified */
    /* get the condition type */
    size = sizeof(lazy.condition_type);
    db_get_value(hDB, 0, "/Lazy/Settings/Comparision operator", lazy.condition_type, &size, TID_STRING);

    /* Get user condition based on comparaison between lcond_value and the condition_limit */
    if (lazy.condition_type[0] != '\0')
    { 
      /* get the condition key */
      size = sizeof(lazy.condition);
      db_get_value(hDB, 0, "/Lazy/Settings/Lazy running condition", condition_key, &size, TID_STRING);
      db_find_key(hDB, 0, condition_key, &hKey);
	    status = db_get_key (hDB, hKey, &key);
      if ((key.type <= TID_DOUBLE))
      { /* something to test */
        /* get the condition index */
        size = sizeof(lazy.condition_index);
        db_get_value (hDB, 0, "/Lazy/Settings/Index condition", &lazy.condition_index, &size, TID_INT);

 	      /* get value of the condition */
	      size = sizeof(lcond_value);
	      db_get_data_index(hDB, hKey, &lcond_value, &size, lazy.condition_index, key.type); 
	      db_sprintf(str, &lcond_value, key.item_size, 0, key.type);
	      lcond_value = atof(str);

 	      /* get value of the condition limit */
	      size = sizeof(lazy.condition_limit);
	      db_get_value(hDB, 0, "/Lazy/Settings/Threshold condition", &lazy.condition_limit, &size, TID_FLOAT); 

        previous_continue = *copy_continue;
        *copy_continue = FALSE;

        /* perform condition check */
 	      if ((lazy.condition_type[0] == '>') &&
            ((float)lcond_value > lazy.condition_limit))
              *copy_continue = TRUE;

        else if ((lazy.condition_type[0] == '=') &&
                 ((float)lcond_value == lazy.condition_limit))
              *copy_continue = TRUE;
	    
        else if ((lazy.condition_type[0] == '<') &&
                 ((float)lcond_value < lazy.condition_limit))
              *copy_continue = TRUE;

        if (msg && previous_continue && !*copy_continue)
          {
            ucond = TRUE;
            if (msg_flag) cm_msg(MINFO,"Lazy","Lazy suspended on user condition");
          }
        else if (!msg && !previous_continue && *copy_continue)
           if (msg_flag) cm_msg(MINFO,"Lazy","Lazy restarted on user condition");
      } /* TID_DOUBLE */
    } /* condition present */
    else
    {
     /* running but no condition */
      printf("running but no condition\n");
    }
  }
  else
  {
    /* Don't care so keep copying */
    if (ucond)
      {
        ucond = FALSE;
        *copy_continue = TRUE;
        if (msg_flag) cm_msg(MINFO,"Lazy","Lazy restarted on run condition");
      }
  }
}

/*------------------------------------------------------------------*/
INT lazy_copy( char * outfile, char * infile)
/********************************************************************\
  Routine: lazy_copy
  Purpose: backup file to backup device
  Input:
   char * outfile   backup destination file 
   char * infile    source file to be backed up
  Output:
  Function value:
  0           success
\********************************************************************/
{
  INT status, private_last_time=0;
  INT size, last_time, cploop_time;
  double dtemp;
  char * pext;

  pext = malloc(strlen(infile));
  strcpy(pext, infile);

  /* find out what format it is from the extension. */
  if (strncmp(pext+strlen(pext)-3,".gz",3) == 0)
    {
      /* extension .gz terminator on .*/
      *(pext+strlen(pext)-3) = '\0';
    }
  lazyst.tot_cp_size = 0;
  last_time = 0;
  size = sizeof(double);
  dtemp = lazyst.tot_cp_size / 1024.;
  db_set_value (hDB, 0, "/Lazy/Statistics/KBytes copied", &dtemp, size, 1, TID_DOUBLE);
  if (lazyst.file_size != 0)
    {
      size = sizeof(float);
      lazyst.progress = (float) (100 * (lazyst.tot_cp_size / lazyst.file_size));
      db_set_value (hDB, 0, "/Lazy/Statistics/Copy progress [%]"
                    , &lazyst.progress, size, 1, TID_FLOAT);
    }
  
  /* open any logging file (output) */
  if ((status = yb_any_file_wopen(dev_type, data_fmt, outfile, &hDev)) != 1) 
    {
      cm_msg(MERROR,"Lazy","cannot open %s [%d]",outfile, status);
      return FORCE_EXIT;
    }      
  
  /* open input data file */
  if (yb_any_file_ropen (infile, data_fmt) != YB_SUCCESS)
    return (-3);
  
  if (msg_flag) cm_msg(MUSER,"Lazy","Starting lazy job on %s",lazyst.backfile);
  cploop_time = -2000;

  /* increase RPC timeout to 60sec for logger with exabyte */
  rpc_set_option(-1, RPC_OTIMEOUT, 60000);

  cm_set_watchdog_params(TRUE, 60000);

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
                  cm_msg(MERROR,"Lazy","lazy_copy Write error %i",szlazy);
                  return -4; 
                }
                lazyst.tot_cp_size += szlazy;
                lazyst.dev_tot_cp_size += szlazy;
                if ((ss_millitime() - cploop_time) > 2000)
                {
                  /* update statistics */
                  lazy_statistics_update(&lastsz, cploop_time);

                  /* check conditions and cm_msg in case of changes */
                  lazy_condition_check(1, &copy_continue);

                  /* update check loop */
                  cploop_time = ss_millitime();

                  /* yield quickly */
                  status = cm_yield(1);
                  if (status == RPC_SHUTDOWN || status == SS_ABORT)
                    cm_msg(MINFO,"Lazy","Abort postponed until end of copy %d[%]"
                           ,lazyst.progress);
                }
            } /* get physrec */
          else
            break;
        } /* copy_continue */
      else
        {
          status = cm_yield(1000);
          if (status == RPC_SHUTDOWN || status == SS_ABORT)
            return FORCE_EXIT;
          if ((ss_millitime() - private_last_time) > 5000)
            {
              /* check conditions and NO cm_msg in any case */
              lazy_condition_check(0, &copy_continue);
            }
        }
    } /* while forever */

  /* update for last the statistics */
   lazy_statistics_update(&lastsz, cploop_time);

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
  double dtemp;
  int size;
  char fullfile[MAX_FILE_PATH] = "\0";
  
  if (ss_file_find(dir, file, &list) == 1)
    {
      strcat(fullfile, dir);
      strcat(fullfile, file);
      if ((lazyst.file_size = ss_file_size(fullfile)) > 0)
      {
        size = sizeof(double);
        dtemp = lazyst.file_size / 1024;
        db_set_value (hDB, 0, "/Lazy/Statistics/File size [KB]"
                    , &dtemp, size, 1, TID_DOUBLE);
        free (list);
        return TRUE;
      }
    }
  free(list);
  return FALSE;
}

/*------------------------------------------------------------------*/
INT lazy_main (HNDLE hDB, HNDLE hKey)
/********************************************************************\
  Routine: lazy_main
  Purpose: check if backup is necessary...
  Input:
   HNDLE hDB   Database handle (not used here but could be hot linked)
   HNDLE hKey  Key handle (not used here but could be hot linked)
  Output:
  Function value:
\********************************************************************/
{
  INT size, cur_state, cur_acq_run, status, tobe_backup, purun;
  double freepercent, svfree;
  char pufile[MAX_FILE_PATH], inffile[MAX_FILE_PATH], outffile[MAX_FILE_PATH];
  BOOL donepurge;

  /* load ODB parameters from /Lazy/Settings */
  if ((status = lazy_load_params(hDB, hKey)) != DB_SUCCESS)
    return status;
  
  /* skip if not active */
  if (!lazy.active)
    return NOTHING_TODO;
  
  /* Check if Tape is OK */
  /* ... */
  
  /* check if space on device (none empty tape label) */
  if (lazy.backlabel[0] == '\0')
    return NOTHING_TODO;
  
  /* check if data dir is none empty */
  if (lazy.dir[0] == '\0')
    {
      cm_msg(MINFO,"Lazy","Please setup Dir data for input source path!");
      return NOTHING_TODO;
    }

  /* check if device path is set */
  if (lazy.backdev[0] == '\0')
    {
      cm_msg(MINFO,"Lazy","Please setup backup device path too!");
      return NOTHING_TODO;
    }
  
  /* Get current run number */
  size = sizeof(cur_acq_run);
  db_get_value (hDB, 0, "Runinfo/Run number", &cur_acq_run, &size, TID_INT);
  
  /* Get current running state */
  size = sizeof(cur_state);
  db_get_value (hDB, 0, "Runinfo/State", &cur_state, &size, TID_INT);
  
   /* build logger dir listing */
  pdirlog = malloc(sizeof(DIRLOG));
  build_log_list(lazy.backfmt, lazy.dir, &pdirlog);
  
  /* sort logger list following the run number criteria */
  sort_log_list(pdirlog);
  
  /* build donelist comparing pdirlog and /Lazy/List */
  pdonelist = malloc(sizeof(INT));
  build_donelist(&pdonelist);

  /* sort /lazy/list run # criteria*/
  sort_donelist(pdonelist);
  
  /* compare list : run NOT in donelist AND run in dirlog */
  tobe_backup = cmp_log2donelist (pdirlog, pdonelist);
  if (tobe_backup < 0)
    return NOTHING_TODO;
  
  /* ckeck if mode is on oldest (keep = -x)
     else take current run - keep file */
  if (lazy.staybehind < 0)
    lazyst.cur_run = tobe_backup;
  else
    lazyst.cur_run = lazyst.cur_run - lazy.staybehind;
  
  /* check SPACE and purge if necessary = % (1:99) */
  if (lazy.pupercent > 0)
    {
      /* cleanup memory */
      donepurge = FALSE;
      svfree = freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);
      while (freepercent <= (double)lazy.pupercent)
        {
          /* rebuild dir listing */
          build_log_list(lazy.backfmt, lazy.dir, &pdirlog);
          
          /* rebuild donelist */
          build_donelist(&pdonelist);
          
          /* search file to purge : run in donelist AND run in dirlog */
          if (lazy_select_purge(pdirlog, pdonelist, pufile, &purun) == 0)
            {
                /* check if beyond keep file + 1 */
                if (purun < (cur_acq_run - abs(lazy.staybehind) - 1 ))
                  {
                    /* remove file */
                    if ((status = ss_file_remove(pufile)) == 0)
                      {
                        status = lazy_log_update(REMOVE_FILE, 0, purun, NULL, pufile);
                        donepurge = TRUE;

                        /* update donelist (remove run entry as the file has been deleted */
                        if ((status=lazy_remove_entry(purun)) != 0)
                          cm_msg(MERROR, "Lazy","remove_entry not performed %d",status);
                      }
                    else
                        cm_msg(MERROR, "Lazy","ss_file_remove not performed %d",status);
                  }
                freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);
                if (svfree == freepercent)
                  break;
            }
            else
            {
              if (donepurge)
                cm_msg(MINFO,"Lazy","Can't purge more for now!");
              break;
            }
        }
    }
  
  /* cleanup memory */
  free (pdirlog);
  free (pdonelist);
  
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
      /* update Lazy for current processing file */
      size = sizeof(lazyst.backfile);
      db_set_value (hDB, 0, "/Lazy/Statistics/Backup file", lazyst.backfile, size, 1, TID_STRING);
      
      /* check if space on backup device */
      if (lazy.bck_dev_size < (lazyst.dev_tot_cp_size + lazyst.file_size))
        {
          lazy.backlabel[0]='\0';
          size = sizeof(lazy.backlabel);
          db_set_value (hDB, 0, "/Lazy/Settings/List label"
                        , lazy.backlabel, size, 1, TID_STRING);
          /* reset the statistics */
          size = sizeof(lazyst);
          memset(&lazyst,0,size);
          if (db_find_key(hDB, 0, "/Lazy/Statistics",&hKeyst) == DB_SUCCESS)
            status = db_set_record(hDB, hKeyst, &lazyst, size, 0);
          else
            cm_msg(MERROR,"Lazy","did not find /Lazy/Statistics for zapping");
          cm_msg(MINFO,"Lazy","Not enough space for next copy on backup device!");
          return NOTHING_TODO;
        }
      
      /* Update the /Lazy for current file processing */
      size = sizeof(lazyst.cur_run);
      db_set_value (hDB, 0, "/Lazy/Statistics/Current Lazy run", &lazyst.cur_run, size, 1, TID_INT);
      
      /* compose the destination file name in case of lazy DISK to DISK */
      if (dev_type == LOG_TYPE_DISK)
        {
          if (lazy.backdev[0] != 0)
            if (lazy.backdev[strlen(lazy.backdev)-1] != DIR_SEPARATOR)
              strcat(lazy.backdev, DIR_SEPARATOR_STR);
          strcpy(outffile, lazy.backdev); 
          strcat(outffile, lazyst.backfile);
        }
      else if(dev_type == LOG_TYPE_TAPE)
          strcpy(outffile, lazy.backdev); 
      else 
          cm_msg(MINFO,"Lazy"," FTP not yet fully implemented");
            
      /* Do the copy */
      if ((status = lazy_copy(outffile, inffile)) != 0)
        {
          if (status == FORCE_EXIT)
            return status;
          cm_msg(MERROR,"Lazy","copy failed -%s-%s-%i",lazy.backdev, lazyst.backfile, status);
          return NOTHING_TODO;
        }
    }
  else
    {
      cm_msg(MERROR,"Lazy","lazy_file_exists file %s doesn't exists",lazyst.backfile);
      return NOTHING_TODO;
    }              
  
  /* Update the tape run list */
  if ((status = lazy_update_list()) != DB_SUCCESS)
    {
      cm_msg(MERROR,"Lazy","lazy_update failed");
      return NOTHING_TODO;
    }
  return NOTHING_TODO;
}

/*------------------------------------------------------------------*/
int main(unsigned int argc,char **argv)
{
  char      ch, host_name[HOST_NAME_LENGTH];
  char      expt_name[HOST_NAME_LENGTH];
  BOOL      debug;
  INT       size, status, mainlast_time;
  DWORD     i;
  
  /* set default */
  host_name[0] = 0;
  expt_name[0] = 0;
  zap_flag = FALSE;
  msg_flag = FALSE;
  
  /* set default */
  cm_get_environment (host_name, expt_name);
  
  /* get parameters */
  for (i=1 ; i<argc ; i++)
    {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
        debug = TRUE;
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
        }
      else
        {
        usage:
          printf("usage: lazylogger [-h <Hostname>] [-e <Experiment>]\n");
          printf("                  [-z zap statistics] [-t (talk msg)\n\n");
          printf("       in case of FTP type, the 'filename backup device' entry should be:\n");
          printf("       host, port, user, password, directory, run%05d.mid\n");
          return 0;
        }
    }
  
  /* connect to experiment */
  status = cm_connect_experiment(host_name, expt_name, "LazyLogger", 0);
  if (status != CM_SUCCESS)
    return 1;
  
  /* turn on keepalive messages with increased timeout */
  cm_set_watchdog_params(TRUE, 60000);
#ifdef _DEBUG
  cm_set_watchdog_params(TRUE, 0);
#endif
  
  cm_get_experiment_database(&hDB, &hKey);
  
  if (zap_flag)
    {
      /* reset the statistics */
      cm_msg(MINFO,"lazy","zapping /lazy/statistics content");
      size = sizeof(lazyst);
      memset(&lazyst,0,size);
      if (db_find_key(hDB, 0, "/Lazy/Statistics",&hKeyst) == DB_SUCCESS)
        status = db_set_record(hDB, hKeyst, &lazyst, size, 0);
      else
        cm_msg(MERROR,"Lazy","did not find /Lazy/Statistics for zapping");
    }
  else
    {
      /* restore globals from odb */
      if (db_find_key(hDB, 0, "/Lazy", &hKey) == DB_SUCCESS)
        {
          if (db_find_key(hDB, 0, "/Lazy/Statistics", &hKeyst) == DB_SUCCESS)
            {
              cm_msg(MINFO,"lazy","restoring /lazy/statistics content");
              size = sizeof(lazyst);
              status = db_get_record(hDB, hKeyst, &lazyst, &size, 0);
            }
          else
            db_create_record(hDB, hKey, "Statistics", LAZY_STATISTICS_STRING);
        }
      else
        {
          cm_msg(MINFO,"lazy","creating /lazy/statistics content");
          db_create_record(hDB, 0, "Lazy/Statistics", LAZY_STATISTICS_STRING);
        }
    }

  mainlast_time = 0;
  do
    {
      status = cm_yield(2000);
      if ((ss_millitime() - mainlast_time) > 10000)
        {
          status = lazy_main( hDB, hKey );
          if (status == FORCE_EXIT)
            break;
          mainlast_time = ss_millitime();
        }      
      if (ss_kbhit())
        {
#if defined(OS_MSDOS) || defined(OS_WINNT)
          ch = getch();
#else
          ch = getchar();
#endif
          if (ch == '!')
            break;
        }
    } while (status != RPC_SHUTDOWN && status != SS_ABORT);
  cm_disconnect_experiment();
  return 1;
}


/********************************************************************\

  Name:         lazylogger.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Disk to Tape copier for background job

  Revision history
  ------------------------------------------------------------------
  date        by    modification
  --------    ---   ------------------------------------------------
  17-JUN-98   PAA   created
\********************************************************************/
#include "midas.h"
#include "msystem.h"
#include "ybos.h"

#define NOTHING_TODO  0
#define FORCE_EXIT    5

typedef struct {
  INT run;
  double size;
} DIRLOG;

DIRLOG *pdirlog;
INT    *pdonelist;

#define LAZY_SETTING_STRING "\
Enable = INT : 1\n\
Backup device label = STRING : [128]\n\
Purge until percent free = INT : 50\n\
Copy while running = INT : 1\n\
Type = STRING : [8] Tape\n\
Format = STRING : [8] YBOS\n\
Keep file = INT : -3\n\
Max Run per Tape = INT : 0\n\
Data dir = STRING : [128] \n\
Backup fmt = STRING : [128] run*.ybs\n\
Backup device = STRING : [128] /dev/nst0\n\
Backup device size in bytes = DOUBLE : 5e9\n\
"
#define LAZY_STATISTICS_STRING "\
File size [KB] = DOUBLE : 0.0\n\
KBytes copied = DOUBLE : 0.0\n\
Total KBytes copied = DOUBLE : 0.0\n\
Copy status [%] = INT : 0\n\
Copy Rate [KB per sec] = FLOAT : 0\n\
Number of Files = INT : 0\n\
Backup status [%] = INT : 0\n\
Current acq run = INT : 0\n\
Current State = INT : 0\n\
Current Lazy run = INT : 0\n\
Backup file = STRING : [128] none \n\
"

typedef struct {
  INT enable;                    /* will prevent further copy (complete run anyway) */
  char backlabel[MAX_FILE_PATH]; /* label of the array in ~/list. if empty like enable = 0 */
  INT pupercent;                 /* 0:100 % of disk space to keep free */
  INT cpwhilerun;                /* 0: prevent copy while acq is running */
  char type[8];                  /* Backup device type  (Disk, Tape, Ftp) */
  char format[8];                /* Data format (YBOS, MIDAS) */
  INT keepfile;                  /* keep x run file between current Acq and backup 
				                            -x same as x but starting from oldest */
  INT max_run_tape;              /* limit of run per tape 0 : disable */
  char dir[MAX_FILE_PATH];       /* path to the data dir */
  char backfmt[MAX_FILE_PATH];   /* format for the run files */
  char backdev[MAX_FILE_PATH];   /* backup device name */
  double bck_dev_size;           /* backup device max byte size active only if max_run_tape<0 */
} LAZY_SETTING;

LAZY_SETTING  lazy;

typedef struct {
double file_size;              /* file bytes size */
double tot_cp_size;            /* current bytes copied */
double dev_tot_cp_size;        /* Total bytes backup on device */
INT progress;                  /* copy % */
float cprate;                  /* copy rate */
INT nfiles;                    /* # of backuped files */
float bckfill;                 /* backup fill % */
INT cur_acq_run;               /* current or last acq run (for info only) */
INT cur_state;                 /* current run state (for info only) */
INT cur_run;                   /* current or last lazy run number done (for info only) */
char backfile[MAX_FILE_PATH];  /* current or last lazy file done (for info only) */
} LAZY_STATISTICS;
LAZY_STATISTICS lazyst;

/* Globals */
extern    plazy, szlazy;
HNDLE     hDB, hKey;
double lastsz;
HNDLE hKeyst;
INT tot_do_size, tot_dirlog_size;
INT hDev, zap_flag=0; 
BOOL copy_continue = TRUE;
INT data_fmt, dev_type;
char lazylog[MAX_STRING_LENGTH];

/* prototypes */
BOOL lazy_file_exists(char * dir, char * file);
INT lazy_main (HNDLE hDB, HNDLE hKey);
INT lazy_copy( char * dev, char * file);
INT lazy_file_compose(char * fmt , char * dir, INT num, char * file);
INT lazy_update_list(void);
INT lazy_remove_file (char * pufile);
INT lazy_select_purge(DIRLOG * plog, INT * pdo, char * pufile, INT * run);
INT lazy_load_params( HNDLE hDB, HNDLE hKey );
void build_log_list(char * fmt, char * dir, DIRLOG ** plog);
void sort_log_list(DIRLOG * plog);
void build_donelist(INT **pdo);
void sort_donelist(INT * pdo);
INT cmp_log2donelist (DIRLOG * plog, INT * pdo);
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
INT ss_run_extract(char * name)
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
      -1      in case of error
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
  if (lazy.keepfile == 0)
    lazy.keepfile = -1;
  return DB_SUCCESS;
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
void sort_donelist(INT * pdo)
/********************************************************************\
  Routine: sort_donelist
  Purpose: sort the full /lazy/list run number in increasing order
  Input:
    INT     *pdo     /lazy/list run listing
  Output:
  Function value:
    INT              none
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
void build_donelist(INT **pdo)
/********************************************************************\
  Routine: build_donelist
  Purpose: build an internal /lazy/list list (pdo) tree
  Input:
    INT     **pdo     /lazy/list run listing
  Output:
    INT     **pdo     /lazy/list run listing
  Function value:
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
void sort_log_list(DIRLOG * plog)
/********************************************************************\
  Routine: sort_log_list
  Purpose: sort plog struct by run number
  Input:
    INT     *pdo     /lazy/list run listing
  Output:
  Function value:
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
void build_log_list(char * fmt, char * dir, DIRLOG ** plog)
/********************************************************************\
  Routine: build_log_list
  Purpose: build an internal directory file list (plog)
  Input:
    char     * fmt     format of the file to search for (ie:run%05d.ybs) 
    char     * dir     path to the directory for the search
  Output:
    INT     **plog     internal file list struct
  Function value:
\********************************************************************/
{
  INT nfile, j;
  char * list;
  char str[256];
  
  /* creat dir listing with given criteria */
  nfile = ss_file_find(dir, fmt, &list);
  /* check */
  for (j=0;j<nfile;j++)
    printf ("list[%i]:%s\n",j, list+j*MAX_STRING_LENGTH);
  /* allocate memory */
  tot_dirlog_size = (nfile*sizeof(DIRLOG));
  *plog = realloc(*plog, tot_dirlog_size);
  /* fill structure */
  for (j=0;j<nfile;j++)
    {
      (*plog+j)->run  = ss_run_extract(list+j*MAX_STRING_LENGTH);
      strcpy(str,dir);
      strcat(str,"\\");
      strcat(str,list+j*MAX_STRING_LENGTH);
      (*plog+j)->size = ss_file_size(str);
    }
  free(list);
}

/*------------------------------------------------------------------*/
INT lazy_select_purge(DIRLOG * plog, INT * pdo, char * pufile, INT * run)
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
  /* search file to purge : run in donelist AND run in dirlog */
  
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
	        status = lazy_file_compose(lazy.backfmt, lazy.dir, abs((plog+j)->run), pufile);
	        *run = abs((plog+j)->run);
	        return 0;
	      }	  
   	}
  }
  return -1;
}

/*------------------------------------------------------------------*/
int lazy_remove_entry(char * label, int run)
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
  HNDLE  hKey;
  INT    i, size, nelement;
  char   keystr[256];
  INT    *prec, found = 0;;

  /* compose key entry name */
  sprintf(keystr,"/Lazy/List/%s",label);
  if (db_find_key(hDB, 0, keystr, &hKey) != DB_SUCCESS)
      return 0;

  /* extract in prec the label[] */
  db_get_record_size(hDB, hKey, 0, &size);
  prec = (INT *)malloc(size);
  nelement = size / sizeof(INT);
  db_get_record(hDB, hKey, (char *) &prec, &size, 0);

  /* match run number */
  for (i=0 ;i<nelement; i++)
    {
      if (*(prec+i) == run)
	{
	  found = 1;
	  *(prec+i) = *(prec+i+1);
	  nelement -= 1;
	}
    }
  /* delete label[] and recreate label[] */
  if (found)
    {
      db_delete_key(hDB,hKey,FALSE);
      db_set_value(hDB,0,keystr,prec,nelement*sizeof(INT), nelement, TID_INT);
      return 0;
    }
  else
    return -1;
}

/*------------------------------------------------------------------*/
INT lazy_remove_file (char * pufile)
/********************************************************************\
  Routine: lazy_remove_file
  Purpose: remove (delete) file
  Input:
   char * pufile   file to be purged (removed)
  Output:
  Function value:
    0             success
    FORCE_EXIT    quit request 
\********************************************************************/
{
  /* remove file */
  ss_file_remove(pufile);
  return 0;
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
  time_t   full_time;
  FILE *pF;
  INT  size;
  char * ptape, str[128];
  KEY Keylabel;  
  HNDLE hKeylabel;
  
  /* check if dir exists */
  if (db_find_key(hDB, 0, "/Lazy/List", &hKey) == DB_SUCCESS)
    {
      /*  record the saved run */
      if (db_find_key(hDB, hKey, lazy.backlabel, &hKeylabel) == DB_SUCCESS)
	{
	  db_get_key(hDB, hKeylabel, &Keylabel);
	  lazyst.nfiles = Keylabel.num_values; 
	  ptape = malloc(Keylabel.total_size);
	  size = Keylabel.total_size;
	  db_get_data(hDB, hKeylabel, ptape, &size, TID_INT);
	  ptape = realloc((char *)ptape, size+sizeof(INT));
	  memcpy((char *)ptape+size, (char *)&lazyst.cur_run, sizeof(INT));
	  lazyst.nfiles += 1;
	  size += sizeof(INT);
	  db_set_data (hDB, hKeylabel, (char *)ptape, size, lazyst.nfiles, TID_INT);
	  size = sizeof(lazyst.nfiles);
	  db_set_value(hDB, 0,"/Lazy/Statistics/Number of Files", &lazyst.nfiles, size, 1, TID_INT);
	  /* update lazy.log */
	  sprintf(lazylog, "%s/lazy.log", lazy.dir);
	  pF = fopen(lazylog, "a");
	  if (pF)
	    {
	      time(&full_time);
	      strcpy(str, ctime(&full_time));
	      str[24] = 0;
	      strcat(str,"- Backup done");
	      fputs (str,pF);
	      fclose (pF);
	    }
	  else
	    cm_msg(MERROR,"Lazy","cannot open %s",lazylog);
	  cm_msg(MUSER,"Lazy","         lazy job %s done!",lazyst.backfile);
	}
      else
	{
	  size = sizeof(lazyst.cur_run);
	  db_set_value(hDB, hKey, lazy.backlabel, &lazyst.cur_run, size, 1, TID_INT);
	}
    }
  else
    { /* list doesn't exists */
      lazyst.nfiles = 1;
      size = sizeof(lazyst.cur_run);
      sprintf(str,"/Lazy/List/%s",lazy.backlabel);
      db_set_value(hDB, 0, str, &lazyst.cur_run, size, 1, TID_INT);
      /* upcate statistics */
      size = sizeof(lazyst);
      if (db_find_key(hDB, 0, "/Lazy/Statistics",&hKeyst) == DB_SUCCESS)
      	db_set_record(hDB, hKeyst, &lazyst, size, 0);
      cm_msg(MUSER,"Lazy","         lazy job %s done!",lazyst.backfile);
    } 
  
  /* reset the tape label as max run is reached */
  if (lazyst.nfiles >= lazy.max_run_tape && (lazy.max_run_tape > 0) )
    {
      lazy.backlabel[0]='\0';
      size = sizeof(lazy.backlabel);
      db_set_value (hDB, 0, "/Lazy/Settings/Backup device label", lazy.backlabel, size, 1, TID_STRING);
      lazyst.nfiles = 0;
      size = sizeof(lazyst.nfiles);
      db_set_value(hDB, 0,"/Lazy/Statistics/Number of Files", &lazyst.nfiles, size, 1, TID_INT);
      /*      ss_tape_unmount(hDev); can't do it as device closed */
      cm_msg(MINFO,"Lazy","Lazy job complete. Please remove tape!");
    }
  return DB_SUCCESS;;
}

/*------------------------------------------------------------------*/
INT lazy_file_compose(char * fmt , char * dir, INT num, char * file)
/********************************************************************\
  Routine: lazy_file_compose
  Purpose: compose "file" wit the args
  Input:
   char * fmt       file format 
   char * dir       data directory
   INT num          run number
  Output:
   char * file      composed file name
  Function value:
        0           success
\********************************************************************/
{
  char str[128];
  INT size;
  
  size = sizeof(dir);
  if (dir[0] != 0)
    if (dir[strlen(dir)-1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);
  strcpy(str, dir);
  strcat(str, fmt);
  
  /* substitue "%d" by current run number */
  if (strchr(str, '%'))
    sprintf(file, str, num);
  else
    strcpy(file, str);
  return 0;
}

/*------------------------------------------------------------------*/
INT lazy_copy( char * dev, char * file)
/********************************************************************\
  Routine: lazy_copy
  Purpose: backup file to backup device
  Input:
   char * dev       backup device 
   char * file      file to be backuped
  Output:
  Function value:
  0           success
\********************************************************************/
{
  INT status, written, private_last_time=0;
  INT size, last_time, cploop_time;
  double dtemp;
  char * pext;

  pext = malloc(strlen(file));
  strcpy(pext, file);

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
    lazyst.progress = (INT) (100 * (lazyst.tot_cp_size / lazyst.file_size));
    db_set_value (hDB, 0, "/Lazy/Statistics/Copy status [%]"
      , &lazyst.progress, size, 1, TID_INT);
  }
 
  /* open any logging file */
  if ((status = open_any_logfile(dev_type, data_fmt, dev, &hDev)) != 1) 
  {
    cm_msg(MERROR,"Lazy","cannot open %s [%d]",dev, status);
    return FORCE_EXIT;
  }      
  
  /* open input data file */
  if (open_any_datafile (data_fmt, file) != RDLOG_SUCCESS)
    return (-3);
  
  cm_msg(MINFO,"Lazy","Starting lazy job on %s",lazyst.backfile);
  cploop_time = 0;
  /* infinite loop while copying */
  while (1)
    {  
      if (copy_continue)
	{
	  if (get_any_physrec(data_fmt) == RDLOG_SUCCESS)
	    {
#ifdef OS_WINNT
	      WriteFile((HANDLE) hDev, (char *)plazy, szlazy, &written, NULL);
	      status = written == (INT) szlazy ? SS_SUCCESS : SS_FILE_ERROR;
#else
	      status = write(hDev, (char *)plazy, szlazy) == (INT) szlazy ? SS_SUCCESS : SS_FILE_ERROR;
#endif
	      lazyst.tot_cp_size += szlazy;
	      lazyst.dev_tot_cp_size += szlazy;
	      if (status != SS_SUCCESS)
		{
		  cm_msg(MERROR,"Lazy","lazy_copy Write error %i != %i",szlazy, written);
		  return -4; 
		}
	      if ((ss_millitime() - cploop_time) > 2000)
		{
		  /* update rate statistics */
		  size = sizeof(float);
		  lazyst.cprate = (float)(lazyst.tot_cp_size - lastsz);
		  lazyst.cprate /= (ss_millitime() - cploop_time);
		  db_set_value (hDB, 0, "/Lazy/Statistics/Copy Rate [KB per sec]"
				, &lazyst.cprate, size, 1, TID_FLOAT);
		  lastsz = lazyst.tot_cp_size;
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
		      lazyst.progress = (DWORD) (100 * (lazyst.tot_cp_size / lazyst.file_size));
		      db_set_value (hDB, 0, "/Lazy/Statistics/Copy status [%]"
				    , &lazyst.progress, size, 1, TID_INT);
		    }
		  /* Get current running state */
		  size = sizeof(lazyst.cur_state);
		  db_get_value (hDB, 0, "Runinfo/State", &lazyst.cur_state, &size, TID_INT);
		  
		  /* Get updated "copy while running" */
		  size = sizeof(lazy.cpwhilerun);
		  db_get_value (hDB, 0, "/Lazy/Settings/Copy while running", &lazy.cpwhilerun, &size, TID_INT);
		  
		  /* check if copy should be continuing */
		  if ((lazy.cpwhilerun == 0) && (lazyst.cur_state == STATE_RUNNING))
		    copy_continue = FALSE;
		  else
		    copy_continue = TRUE;
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
	      /* Get current running state */
	      size = sizeof(lazyst.cur_state);
	      db_get_value (hDB, 0, "Runinfo/State", &lazyst.cur_state, &size, TID_INT);
	      /* Get updated "copy while running" */
	      size = sizeof(lazy.cpwhilerun);
	      db_get_value (hDB, 0, "/Lazy/Settings/Copy while running", &lazy.cpwhilerun, &size, TID_INT);
	      private_last_time = ss_millitime();
	      /* check if copy should be continuing */
	      if ((lazy.cpwhilerun == 0) && (lazyst.cur_state == STATE_RUNNING))
		copy_continue = FALSE;
	      else
		copy_continue = TRUE;
	    }
	}
    } /* while forever */
  
  /* close log device */
  close_any_logfile(dev_type, hDev);
  
  /* close input data file */   
  close_any_datafile(data_fmt);
  
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
  
  if (ss_file_find(dir, file, &list) == 1)
    {
      lazyst.file_size = ss_file_size(list);
      dtemp = lazyst.file_size / 1024;
      size = sizeof(double);
      db_set_value (hDB, 0, "/Lazy/Statistics/File size [KB]"
		    , &dtemp, size, 1, TID_DOUBLE);
      return TRUE;
    }
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
  INT size, status, tobe_backup, purun;
  double freepercent, svfree;
  char pufile[MAX_FILE_PATH];
  
  /* load ODB parameters from /Lazy/Settings */
  if ((status = lazy_load_params(hDB, hKey)) != DB_SUCCESS)
    return status;
  
  /* skip if not enable */
  if (!lazy.enable)
    return NOTHING_TODO;;
  
  /* Check if Tape is OK */
  /* ... */
  
  /* check if space on device (none empty tape label) */
  if (lazy.backlabel[0] == '\0')
    return NOTHING_TODO;
  
  /* check if device path is set */
  if (lazy.backdev[0] == '\0')
    {
      cm_msg(MINFO,"Lazy","Please setup backup device path too!");
      return NOTHING_TODO;
    }
  
  /* Get current run number */
  size = sizeof(lazyst.cur_acq_run);
  db_get_value (hDB, 0, "Runinfo/Run number", &lazyst.cur_acq_run, &size, TID_INT);
  db_set_value (hDB, 0, "/Lazy/Statistics/Current acq run", &lazyst.cur_acq_run, size, 1, TID_INT);
  
  /* Get current running state */
  size = sizeof(lazyst.cur_state);
  db_get_value (hDB, 0, "Runinfo/State", &lazyst.cur_state, &size, TID_INT);
  db_set_value (hDB, 0, "/Lazy/Statistics/Current State", &lazyst.cur_state, size, 1, TID_INT);
  
   /* build logger dir listing */
  pdirlog = malloc(sizeof(DIRLOG));
  build_log_list(lazy.backfmt, lazy.dir, &pdirlog);
  
  /* sort logger list following the criteria */
  sort_log_list(pdirlog);
  
  /* build donelist compare pdirlog and /Lazy/List */
  pdonelist = malloc(sizeof(INT));
  build_donelist(&pdonelist);

  /* sort /lazy/list */
  sort_donelist(pdonelist);
  
  /* compare list : run NOT in donelist AND run in dirlog */
  tobe_backup = cmp_log2donelist (pdirlog, pdonelist);
  if (tobe_backup < 0)
    return NOTHING_TODO;
  
  /* ckeck if mode is on oldest (keep = -x)
     else take current run - keep file */
  if (lazy.keepfile < 0)
    lazyst.cur_run = tobe_backup;
  else
    lazyst.cur_run = lazyst.cur_run - lazy.keepfile;
  
  /* check SPACE and purge if necessary = % (1:99) */
  if (lazy.pupercent > 0)
    {
      svfree = freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);
      while (freepercent <= (double)lazy.pupercent)
	    {
	      /* rebuild dir listing */
	      build_log_list(lazy.backfmt, lazy.dir, &pdirlog);
	      
	      /* rebuild donelist */
	      build_donelist(&pdonelist);

	      /* search file to purge : run in donelist AND run in dirlog */
	      lazy_select_purge(pdirlog, pdonelist, pufile, &purun);
	      /* check if beyond keep file + 1 */
	      if (purun < (lazyst.cur_acq_run - abs(lazy.keepfile) - 1 ))
	      {
		/* remove file */
	        ss_file_remove(pufile);

		/* update donelist (remove run entry as the file has been deleted */
		lazy_remove_entry(lazy.backlabel, purun);
	      }
	      freepercent = 100. * ss_disk_free(lazy.dir) / ss_disk_size(lazy.dir);
	      if (svfree == freepercent)
	        break;
	    }
    }
  
  /* cleanup memory */
  free (pdirlog);
  free (pdonelist);
  
  /* check if backup run is beyond keep */
  if (lazyst.cur_run >= (lazyst.cur_acq_run - abs(lazy.keepfile)))
    return NOTHING_TODO;
  /* Compose the proper file name */
  status = lazy_file_compose(lazy.backfmt , lazy.dir , lazyst.cur_run , lazyst.backfile);
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
      if (lazy.max_run_tape <= 0)
	{
	  if (lazy.bck_dev_size < (lazyst.dev_tot_cp_size + lazyst.file_size))
	    {
	      lazy.backlabel[0]='\0';
	      size = sizeof(lazy.backlabel);
	      db_set_value (hDB, 0, "/Lazy/Settings/Backup device label"
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
	}
      
      /* Update the /Lazy for current file processing */
      size = sizeof(lazyst.cur_run);
      db_set_value (hDB, 0, "/Lazy/Statistics/Current Lazy run", &lazyst.cur_run, size, 1, TID_INT);
      
      /* Do the copy */
      if ((status = lazy_copy(lazy.backdev, lazyst.backfile)) != 0)
	{
	  if (status == FORCE_EXIT)
	    return status;
	  cm_msg(MERROR,"Lazy","copy failed -%s-%s-%i",lazy.backdev, lazyst.backfile, status);
	  return NOTHING_TODO;
	}
    }
  else
    {
      cm_msg(MERROR,"Lazy","lazy_file_exists file doesn't exists -%s-",lazyst.backfile);
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
  
  /* set default */
  cm_get_environment (host_name, expt_name);
  
  /* get parameters */
  for (i=1 ; i<argc ; i++)
  {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (strncmp(argv[i],"-z",2) == 0)
	    zap_flag = 1;
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
	    printf("                  [-z zap statistics]\n\n");
	    return 0;
    }
  }
  
  /* connect to experiment */
  status = cm_connect_experiment(host_name, expt_name, "LazyLogger", 0);
  if (status != CM_SUCCESS)
    return 1;
  
  cm_set_watchdog_params(TRUE, 60000);
#ifdef _DEBUG
  cm_set_watchdog_params(TRUE, 0);
#endif
  
  cm_get_experiment_database(&hDB, &hKey);
  
  if (zap_flag == 1)
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


/********************************************************************\

  Name:         lazylogger.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Disk to Tape copier for background job

  $Log$
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

typedef struct {
  INT run;
  double size;
} DIRLOG;

DIRLOG *pdirlog;
INT    *pdonelist;

#define LAZY_SETTINGS_STRING "\
Maintain free space[%] = INT : 0\n\
Stay behind = INT : -3\n\
Running condition = STRING : [128] ALWAYS\n\
Data dir = STRING : [256] \n\
Data format = STRING : [8] MIDAS\n\
Filename format = STRING : [128] run%05d.mid\n\
Backup type = STRING : [8] Disk\n\
Path = STRING : [128] \n\
Capacity (KBytes) = FLOAT : 5e6\n\
List label= STRING : [128] \n\
"
#define LAZY_STATISTICS_STRING "\
Backup file = STRING : [128] none \n\
File size [KB] = FLOAT : 0.0\n\
KBytes copied = FLOAT : 0.0\n\
Total KBytes copied = FLOAT : 0.0\n\
Copy progress [%] = FLOAT : 0\n\
Copy Rate [KB per sec] = FLOAT : 0\n\
Backup status [%] = FLOAT : 0\n\
Number of Files = INT : 0\n\
Current Lazy run = INT : 0\n\
"

typedef struct {
  INT   pupercent;                /* 0:100 % of disk space to keep free */
  INT   staybehind;               /* keep x run file between current Acq and backup 
                                        -x same as x but starting from oldest */
  char  condition[128];           /* key condition */
  char  dir[256];       /* path to the data dir */
  char  format[8];                /* Data format (YBOS, MIDAS) */
  char  backfmt[MAX_FILE_PATH];   /* format for the run files run%05d.mid */
  char  type[8];                  /* Backup device type  (Disk, Tape, Ftp) */
  char  path[MAX_FILE_PATH];   /* backup device name */
  float capacity;             /* backup device max byte size */
  char  backlabel[MAX_FILE_PATH]; /* label of the array in ~/list. if empty like active = 0 */
} LAZY_SETTING;
LAZY_SETTING  lazy;

typedef struct {
char  backfile[MAX_FILE_PATH]; /* current or last lazy file done (for info only) */
float file_size;               /* file Kbytes size */
float cur_size;                /* current Kbytes copied */
float cur_dev_size;            /* Total Kbytes backup on device */
float progress;                /* copy % */
float copy_rate;               /* copy rate */
float bckfill;                 /* backup fill % */
INT nfiles;                    /* # of backuped files */
INT cur_run;                   /* current or last lazy run number done (for info only) */
} LAZY_STATISTICS;
LAZY_STATISTICS lazyst;

/* Globals */
void      *plazy;
DWORD     szlazy;
HNDLE     hDB, hKey;
float     lastsz;
HNDLE     hKeyst;
INT       run_state, tot_do_size, tot_dirlog_size, hDev;
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
            ,lazy.path, run, lazyst.backfile, lazyst.file_size);
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
      (*plog+j)->size = ss_file_size(str)/1024;
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
    lazyst.copy_rate = 1000.f * (lazyst.cur_size - lastsz) / (ss_millitime() - cploop_time);

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
  INT status, no_cpy_last_time=0;
  INT last_time, cpy_loop_time;
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
      cm_msg(MERROR,"Lazy","cannot open %s [%d]",outfile, status);
      return FORCE_EXIT;
    }      
  
  /* open input data file */
  if (yb_any_file_ropen (infile, data_fmt) != YB_SUCCESS)
    return (-3);

  /* force a statistics update on the first loop */
  cpy_loop_time = -2000;

  if (msg_flag) cm_msg(MUSER,"Lazy","Starting lazy job on %s",lazyst.backfile);
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
                lazyst.cur_size += (float) szlazy / 1024;
                lazyst.cur_dev_size += (float) szlazy / 1024;
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
                    cm_msg(MINFO,"Lazy","Abort postponed until end of copy %d[%]"
                           ,lazyst.progress);
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
      if ((lazyst.file_size = (float)(ss_file_size(fullfile)/1024)) > 0)
      {
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
  INT size,cur_acq_run, status, tobe_backup, purun;
  double freepercent, svfree;
  char pufile[MAX_FILE_PATH], inffile[MAX_FILE_PATH], outffile[MAX_FILE_PATH];
  BOOL donepurge;

  
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
    return NOTHING_TODO;
  
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
  
  /* Get current run number */
  size = sizeof(cur_acq_run);
  db_get_value (hDB, 0, "Runinfo/Run number", &cur_acq_run, &size, TID_INT);
  
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
      /* check if space on backup device */
      if (lazy.capacity < (lazyst.cur_dev_size + lazyst.file_size))
        {
          /* not enough space => reset list label */
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
          strcpy(outffile, lazy.path); 
           
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
  char      host_name[HOST_NAME_LENGTH];
  char      expt_name[HOST_NAME_LENGTH];
  BOOL      debug;
  INT       msg, ch, size, status, mainlast_time;
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
        printf(" Quick man :\n");
        printf(" The Lazy/Settings tree is composed of the following parameters:\n");
        printf(" Maintain free space [%](0): purge source device to maintain free space on the source directory\n");
        printf("                       (0) : no purge      \n");
        printf(" Stay behind  (-3)         : If negative number : lazylog runs starting from the OLDEST\n");
        printf("                              run file sitting in the 'Dir data' to the current acquisition\n");
        printf("                              run minus the 'Stay behind number'\n");
        printf("                             If positive number : lazylog starts from the current\n");
        printf("                              acquisition run minus 'Stay behind number' \n");
        printf(" Running condition          : active/deactive lazylogger under given condition i.e:\n");
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
        printf(" Capacity (KBytes)         : Maximum capacity of the destination device.\n");
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
  
  /* need to cleanup previous lazy in order to make sure the open record
  is attached correctly */
  if (cm_exist("lazylogger",FALSE))
    {
      HNDLE hBuf;
      bm_open_buffer(EVENT_BUFFER_NAME, EVENT_BUFFER_SIZE, &hBuf);
      cm_cleanup();
      bm_close_buffer(hBuf);
    }

  printf("Lazylogger starting... ""!"" to exit \n");

  if (db_find_key(hDB,0,"/Lazy", &hKey) == DB_SUCCESS)
    {
      /* create/update settings */	
      db_create_record(hDB, hKey, "Settings", LAZY_SETTINGS_STRING);
      /* create/update statistics */
      db_create_record(hDB, hKey, "Statistics", LAZY_STATISTICS_STRING);
    }
  else
    {
      /* create/update settings */	
      db_create_record(hDB, 0, "/Lazy/Settings", LAZY_SETTINGS_STRING);
      /* create/update statistics */
      db_create_record(hDB, 0, "/Lazy/Statistics", LAZY_STATISTICS_STRING);
    }
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
  
  /* get value once & hot links the run state */
  db_find_key(hDB,0,"/runinfo/state",&hKey);
  size = sizeof(run_state);
  db_get_data(hDB, hKey, &run_state, &size, TID_INT);
  status = db_open_record(hDB, hKey, &run_state, sizeof(run_state), MODE_READ, NULL, NULL);
  if (status != DB_SUCCESS){
    cm_msg(MERROR, "run_state", "cannot open variable record");
  }
  /* hot link for statistics in write mode */
  size = sizeof(lazyst);
  if (db_find_key(hDB,0,"/Lazy/Statistics",&hKey) == DB_SUCCESS)
    db_get_record(hDB, hKey, &lazyst, &size, 0);
  status = db_open_record(hDB, hKey, &lazyst, sizeof(lazyst), MODE_WRITE, NULL, NULL);
  if (status != DB_SUCCESS){
    cm_msg(MERROR, "lazy/statistics", "cannot open variable record");
  }
  /* get /settings once & hot link settings in read mode */
  db_find_key(hDB,0,"/Lazy/Settings",&hKey);
  size = sizeof(lazy);
  status = db_open_record(hDB, hKey, &lazy, sizeof(lazy), MODE_READ, NULL, NULL);
  if (status != DB_SUCCESS){
    cm_msg(MERROR, "lazy/settings", "cannot open variable record");
  }

  /* set Data dir from /logger if local is empty & /logger exists */
  if ((lazy.dir[0] == '\0') &&
      (db_find_key(hDB,0,"/Logger/Data dir", &hKey) == DB_SUCCESS))
  {
    size = sizeof(lazy.dir);
    db_get_data(hDB, hKey, lazy.dir, &size, TID_STRING);
    db_set_value(hDB, 0, "/Lazy/Settings/Data dir", lazy.dir, size, 1, TID_STRING);
  }

  mainlast_time = 0;
  /* initialize ss_getchar() */
  ss_getchar(0);

  do
    {
      msg = cm_yield(2000);
      if ((ss_millitime() - mainlast_time) > 10000)
        {
          status = lazy_main( hDB, hKey );
          if (status == FORCE_EXIT)
            break;
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
  cm_disconnect_experiment();
  return 1;
}


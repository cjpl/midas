/********************************************************************\
  
  Name:         mstat.c
  Created by:   Pierre-Andre Amaudruz
  
  Contents:     Display/log some pertinent information of the ODB
  
  $Log$
  Revision 1.3  1998/10/23 14:21:52  midas
  - Modified version scheme from 1.06 to 1.6.0
  - cm_get_version() now returns versino as string

  Revision 1.2  1998/10/12 12:19:04  midas
  Added Log tag in header


  Previous revision history
  ------------------------------------------------------------------
  date        by    modification
  --------    ---   ------------------------------------------------
  30-JAN-97   PAA   created
  20-Oct-97   PAA   included malarm functionality

\********************************************************************/

#include <fcntl.h>
#include "midas.h"
#include "msystem.h"

#define MIDSTAT_MAX_LINE 13
#define MAX_LINE         64
#define LINE_LENGTH      85
#define ALINE_LENGTH    256

#if defined( OS_WINNT )
#define ESC_FLAG 0
#else
#define ESC_FLAG 1
#endif

static char data[10000];

struct         {
  INT      runstate;
  INT      runnumber;
  INT      equperiod;
  DWORD    equevtsend;
  DWORD    equevtpsec;
  DWORD    timebin;
  DWORD    stopbin;
  char     state[256];
  char     expt[256];
  char     equclient[256];
  char     equnode[256];
  char     starttime[256];
  char     stoptime[256];
  char     currenttime[256]; 
  char     ltype[64];
  char     lstate[64];
  char     lpath[256];
  char     datadir[256];
  char     mesfil[256];
  char     clientn[256];
  char     clienth[256];
  double   equkbpsec;
  double   levt;
  double   lbyt;
  BOOL     wd;
  BOOL     lactive;
  BOOL     equenabled;
} midstat;

#define ALARM_SETTINGS_STR "\
Current Client Checker = STRING : [32]\n\
Host = STRING : [256]\n\
Number of current local server = INT : 0\n\
Client checker time interval = DWORD : 10000\n\
"

#define ALARM_CLIENT_STR "\
Enable = BOOL : 0\n\
Always  = BOOL : 0\n\
Condition = STRING : [256] NONE\n\
Condition index = INT : 0\n\
Condition type = CHAR : =\n\
Condition value = DOUBLE : 0\n\
Condition message = STRING : [256]\n\
Condition file = STRING : [256]\n\
Client = STRING : [256] Aclient\n\
Start on start = BOOL : 0\n\
Start message = STRING : [256] Starting it, Jim\n\
Start command = STRING : [256] \n\
Remove on stop = BOOL : 0\n\
Remove message = STRING : [256] removing it, Jim\n\
Remove command = STRING : [256] \n\
Stop run on dead = BOOL : 0\n\
Alarm on dead  = BOOL : 0\n\
Alarm message = STRING : [256] Is dead, Jim\n\
Alarm file = STRING : [256] \n\
Message time interval = DWORD : 30000\n\
"
typedef struct {
  BOOL enable;
  BOOL always;
  char cond[ALINE_LENGTH];
  INT  cond_idx;
  char cond_typ;
  double cond_val;
  char cond_msg[ALINE_LENGTH];
  char cond_fil[ALINE_LENGTH];
  char client[ALINE_LENGTH];
  BOOL Sstart;
  char Startmsg[ALINE_LENGTH];
  char Startcommand[ALINE_LENGTH];
  BOOL Sremove;
  char Stopmsg[ALINE_LENGTH];
  char Stopcommand[ALINE_LENGTH];
  BOOL Dstop;
  BOOL Dalarm;
  char Amsg[ALINE_LENGTH];
  char Afile[ALINE_LENGTH];
  DWORD mes_time_interval;
} M_PROCS;

typedef struct {
  DWORD last_condition;
  DWORD last_start;
  DWORD last_dead;
  DWORD last_remove;
} M_LOCS;
/* Global variables 
 *  (required since no way to pass these inside callbacks) */
M_LOCS  *ploc;
M_PROCS *pmproc;
INT      numprocs;
DWORD    checktime, lastchecktime;
INT      nlocal;
HNDLE    hKeynlocal, hKeychktime;
BOOL     active_flag, esc_flag;

DWORD    loop, delta_time, cur_max_line;
char     ststr[MAX_LINE][LINE_LENGTH];

/* prototypes below have side effect of updating 
 * the global variables pmproc and numprocs */
INT scan_alarms( HNDLE hDB, HNDLE hKey );
INT setup_alarms( HNDLE hDB, HNDLE hKey );
INT cleanup_alarms( HNDLE hDB, HNDLE hKey );

/*------------------------------------------------------------------*/
char * strshfl (char * s)
/* Shift to the left by one char */
	{
		char *t;
		t = s;
		while (*t)
				*t++=*(t+1);
		return s;
	}

/*------------------------------------------------------------------*/
INT open_log_midstat(INT file_mode, INT runn, char * svpath)
{
  char srun[32];
  INT  fHandle;

  if (file_mode == 1)
    {  /* append run number */
      strcat (svpath,".");
      sprintf(srun,"Run%4.4i",runn);
      strncat (svpath,srun,strlen(srun));
      printf("output with run file:%s-\n",svpath);
    }
  /* open device */
#if defined (ULTRIX) || defined (LINUX)
  if( (fHandle = open(svpath ,O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1 )
#endif
#ifdef OSF1
    if( (fHandle = open(svpath ,O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1 )
#endif
#ifdef vxw
      if( (fHandle = open(svpath ,O_WRONLY | O_CREAT | O_TRUNC , 0644)) == -1 )
#endif
#ifdef _NT_
	if( (fHandle = _open(svpath ,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,  S_IWRITE)) == -1 )
	  
#endif
	  {
	    printf("File %s cannot be created\n",svpath);
	  }
  return (fHandle);
}

/*------------------------------------------------------------------*/
void compose_status(HNDLE hDB, HNDLE hKey)
{
  BOOL     atleastone_active;
  INT      savej, j, ii, i ,size;
  char     str[256], path[256];
  KEY      key;
  HNDLE    hSubkey;
  char     strtmp[256];
  time_t   full_time;
  DWORD    difftime;
  
  /* Clear string page */
  memset (ststr,' ',sizeof(ststr));
  
  for (j=0;j<MAX_LINE;j++)
    ststr[j][LINE_LENGTH-1] = '\0';
  
  /* Run info */
  size = sizeof(i);
  db_get_value(hDB, 0, "/Runinfo/State", &midstat.runstate, &size, TID_INT);
  if (midstat.runstate == STATE_RUNNING)
    strcpy (midstat.state,"Running");
  if (midstat.runstate == STATE_PAUSED)
    strcpy (midstat.state,"Paused ");
  if (midstat.runstate == STATE_STOPPED)
    strcpy (midstat.state,"Stopped");
  size = sizeof(i);
  db_get_value(hDB, 0, "/Runinfo/run number", &midstat.runnumber, &size, TID_INT);
  size = sizeof(str);
  db_get_value(hDB, 0, "/Runinfo/start time", midstat.starttime, &size, TID_STRING);
  size = sizeof(str);
  db_get_value(hDB, 0, "/Runinfo/stop time", midstat.stoptime, &size, TID_STRING);
  size = sizeof(DWORD);
  db_get_value(hDB, 0, "/runinfo/Start Time binary", &midstat.timebin, &size, TID_DWORD);
  size = sizeof(DWORD);
  db_get_value(hDB, 0, "/runinfo/Stop Time binary", &midstat.stopbin, &size, TID_DWORD);
  /* Experiment info */
  size = sizeof(str);
  db_get_value(hDB, 0, "/experiment/name", midstat.expt, &size, TID_STRING);
  
  j =0;
  time(&full_time);
  strcpy(str, ctime(&full_time));
  str[24] = 0;
  if (active_flag)
    sprintf(&(ststr[j++][0]),"*-v%s- MIDAS status -- Alarm Checker active-----%s--*"
	    ,cm_get_version(),str);
  else
    sprintf(&(ststr[j++][0]),"*-v%s- MIDAS status page ------------------------%s--*"
	    ,cm_get_version(),str);
  
  sprintf(&(ststr[j][0]),"Experiment:%s",midstat.expt);
  sprintf(&(ststr[j][23]), "Run#:%d",midstat.runnumber);

/* PAA revisit the /Runinfo for run time display */
  /* state */
  if (midstat.runstate == STATE_RUNNING)
    if (esc_flag)
      sprintf(&(ststr[j][35]), "State:\033[1m%s\033[m",midstat.state);
    else
      sprintf(&(ststr[j][36]), "State:%s",midstat.state);
  else
    sprintf(&(ststr[j][36]), "State:%s",midstat.state);
  
  /* time */  
  if (midstat.runstate != STATE_STOPPED)  
    {
      cm_time((DWORD *)(&full_time));
      difftime = full_time - midstat.timebin;
      if (esc_flag)
        sprintf(&(ststr[j++][66]),"Run time :%02d:%02d:%02d",
		difftime/3600,difftime%3600/60,difftime%60);
      else
        sprintf(&(ststr[j++][54]),"     Run time :%02d:%02d:%02d",
	        difftime/3600,difftime%3600/60,difftime%60);
    }         
  else if (midstat.runstate == STATE_STOPPED)
    {
      if (midstat.stopbin < midstat.timebin)
	difftime = 0;
      else
	difftime = midstat.stopbin - midstat.timebin;
      if (esc_flag)
	sprintf(&(ststr[j++][54]),"Full Run time :%02d:%02d:%02d",
		difftime/3600,difftime%3600/60,difftime%60);
      else
	sprintf(&(ststr[j++][54]),"Full Run time :%02d:%02d:%02d",
		difftime/3600,difftime%3600/60,difftime%60);
      
      sprintf(&(ststr[j][42])," Stop time:%s",midstat.stoptime);
    }
  
  /* Start Stop time */
  sprintf(&(ststr[j][0]),"Start time:%s",midstat.starttime);
  sprintf(&(ststr[++j][0]),"");
  sprintf(&(ststr[j++][0]),"");
  
  /* Equipment tree */
  size = sizeof(str);
  
  savej = j;
  atleastone_active = FALSE;
  /* check if dir exists */
  if (db_find_key(hDB, 0, "/equipment", &hKey) == DB_SUCCESS)
    {
      sprintf(&(ststr[j][0]),"FE Equip.");
      sprintf(&(ststr[j][12]),"Node");
      sprintf(&(ststr[j][30]),"Event Taken");
      sprintf(&(ststr[j][45]),"Event Rate[/s]");
      sprintf(&(ststr[j++][60]),"Data Rate[Kb/s]");
      for (i=0 ; ; i++)
	{
	  db_enum_key(hDB, hKey, i, &hSubkey);
	  if (!hSubkey)
	    break;
	  db_get_key(hDB, hSubkey, &key);
	  if ((key.type == TID_KEY) && 
	      ((strstr(key.name,"ODB")) == NULL) &&
	      ((strstr(key.name,"BOR")) == NULL) &&
	      ((strstr(key.name,"EOR")) == NULL))
	    {
	      /* check if client running this equipment is present */
	      /* extract client name from equipment */
	      size = sizeof(str);
	      sprintf(strtmp,"/equipment/%s/common/Frontend name",key.name);
 	      db_get_value(hDB, 0, strtmp, midstat.equclient, &size, TID_STRING);
	      /* search client name under /system/clients/xxx/name */
	      if (cm_exist(midstat.equclient,TRUE) == CM_SUCCESS)
		{
		  atleastone_active = TRUE;
		  size = sizeof(BOOL);
		  sprintf(strtmp,"/equipment/%s/common/enabled",key.name);
		  db_get_value(hDB, 0, strtmp, &midstat.equenabled, &size, TID_BOOL);
		  
		  size = sizeof(DWORD);
		  sprintf(strtmp,"/equipment/%s/statistics/events sent",key.name);
		  db_get_value(hDB, 0, strtmp, &midstat.equevtsend, &size, TID_DWORD);
		  
		  size = sizeof(DWORD);
		  sprintf(strtmp,"/equipment/%s/statistics/events per sec.",key.name);
		  db_get_value(hDB, 0, strtmp, &midstat.equevtpsec, &size, TID_DWORD);
		  
		  size = sizeof(double);
		  sprintf(strtmp,"/equipment/%s/statistics/kBytes per sec.",key.name);
		  db_get_value(hDB, 0, strtmp, &midstat.equkbpsec, &size, TID_DOUBLE);
		  
		  size = sizeof(str);
		  sprintf(strtmp,"/equipment/%s/common/Frontend host",key.name);
		  db_get_value(hDB, 0, strtmp, midstat.equnode, &size, TID_STRING);
		  {
		    char *pp, sdummy[64];
		    memset (sdummy,0,64);
		    sprintf(&(ststr[j][0]),"%s ",key.name);
		    pp = strchr(midstat.equnode,'.');
		    if (pp != NULL) 
		      sprintf(&(ststr[j][12]),"%s",strncpy(sdummy,midstat.equnode,pp-midstat.equnode));
		    else
		      sprintf(&(ststr[j][12]),"%s",strncpy(sdummy,midstat.equnode,strlen(midstat.equnode)));
		    sprintf(&(ststr[j][30]),"%i",midstat.equevtsend);
		    if (midstat.equenabled)
		      {
			if (esc_flag)
			  {
			    sprintf(&(ststr[j][45]),"\033[7m%i\033[m",midstat.equevtpsec);
			    sprintf(&(ststr[j++][67]),"%.1f",midstat.equkbpsec);
			  }
			else
			  {
			    sprintf(&(ststr[j][45]),"%i",midstat.equevtpsec);
			    sprintf(&(ststr[j++][60]),"%.1f",midstat.equkbpsec);
			  }
		      }
		    else
		      {
			sprintf(&(ststr[j][45]),"%i",midstat.equevtpsec);
			sprintf(&(ststr[j++][60]),"%.1f",midstat.equkbpsec);
		      }
		  } /* get value */
		} /* active */
	    } /* eor==NULL */
	} /* for equipment */
    }  
  /* Front-End message */
  if (!atleastone_active)
    {
      memset(&(ststr[savej][0]),0,LINE_LENGTH);
      sprintf(&(ststr[savej][0]),"... No Front-End currently running...");
    }
  
  /* search client name "Logger" under /system/clients/xxx/name */
  if (cm_exist("logger",FALSE) == CM_SUCCESS)
    {
      /* logger */
      sprintf(ststr[j++],"");
      size = sizeof(str);
      db_get_value(hDB, 0, "/logger/data dir", midstat.datadir, &size, TID_STRING);
      size = sizeof(str);
      db_get_value(hDB, 0, "/logger/message file", midstat.mesfil, &size, TID_STRING);
      size = sizeof(midstat.wd);
      db_get_value(hDB, 0, "/logger/write data", &midstat.wd, &size, TID_BOOL);
      sprintf(&ststr[j][0], "Logger Data dir: %s",midstat.datadir);
      sprintf(&ststr[j++][45], "Message File: %s",midstat.mesfil);
      
      /* check if dir exists */
      if (db_find_key(hDB, 0, "/logger/channels", &hKey) == DB_SUCCESS)
	{ 
	  sprintf(&(ststr[j][0]),"Chan.");
	  sprintf(&(ststr[j][8]),"Active");
	  sprintf(&(ststr[j][15]),"Type");
	  sprintf(&(ststr[j][25]),"Filename");
	  sprintf(&(ststr[j][45]),"Events Taken");
	  sprintf(&(ststr[j++][60]),"KBytes Taken");
	  for (i=0 ; ; i++)
	    {
	      db_enum_key(hDB, hKey, i, &hSubkey);
	      if (!hSubkey)
		break;
	      db_get_key(hDB, hSubkey, &key);
	      if (key.type == TID_KEY)
		{
		  size = sizeof(BOOL);
		  sprintf(strtmp,"/logger/channels/%s/settings/active",key.name);
		  db_get_value(hDB, 0, strtmp, &midstat.lactive, &size, TID_BOOL);
		  sprintf(midstat.lstate,"No");
		  if (midstat.lactive)
		    sprintf(midstat.lstate,"Yes");
		  
		  size = sizeof(str);
		  sprintf(strtmp,"/logger/channels/%s/settings/Filename",key.name);
		  db_get_value(hDB, 0, strtmp, midstat.lpath, &size, TID_STRING);
		  
		  /* substitue "%d" by current run number */
		  str[0] = 0;
		  strcat(str, midstat.lpath);
		  if (strchr(str, '%'))
		    sprintf(path, str, midstat.runnumber);
		  else
		    strcpy(path, str);
		  strcpy(midstat.lpath, path);
		  
		  size = sizeof(str);
		  sprintf(strtmp,"/logger/channels/%s/settings/type",key.name);
		  db_get_value(hDB, 0, strtmp, midstat.ltype, &size, TID_STRING);
		  
		  size = sizeof(double);
		  sprintf(strtmp,"/logger/channels/%s/statistics/Events written",key.name);
		  db_get_value(hDB, 0, strtmp, &midstat.levt, &size, TID_DOUBLE);
		  
		  size = sizeof(double);
		  sprintf(strtmp,"/logger/channels/%s/statistics/Bytes written",key.name);
		  db_get_value(hDB, 0, strtmp, &midstat.lbyt, &size, TID_DOUBLE);
      midstat.lbyt /= 1024;
		  if (midstat.lactive)
		    {
		      if (esc_flag)
			{	
			  sprintf(&(ststr[j][0]),"  \033[7m%s\033[m",key.name);
			  if (midstat.wd == 1)
			    sprintf(&(ststr[j][15]),"%s",midstat.lstate);
			  else
			    sprintf(&(ststr[j][15]),"(%s)",midstat.lstate);
			  sprintf(&(ststr[j][22]),"%s",midstat.ltype);
			  sprintf(&(ststr[j][32]), "%s",midstat.lpath);
			  sprintf(&(ststr[j][52]), "%.0f",midstat.levt);
			  sprintf(&(ststr[j++][67]), "%9.2e",midstat.lbyt);
			}
		      else	/* no esc */
			{
			  sprintf(&(ststr[j][0]),"  %s",key.name);
			  if (midstat.wd == 1)
			    sprintf(&(ststr[j][8]),"%s",midstat.lstate);
			  else
			    sprintf(&(ststr[j][8]),"(%s)",midstat.lstate);
			  sprintf(&(ststr[j][15]),"%s",midstat.ltype);
			  sprintf(&(ststr[j][25]), "%s",midstat.lpath);
			  sprintf(&(ststr[j][45]), "%.0f",midstat.levt);
			  sprintf(&(ststr[j++][60]), "%9.2e",midstat.lbyt);
		        }
		    }
		  else /* not active */
		    {		
		      sprintf(&(ststr[j][0]),"  %s",key.name);
		      if (midstat.wd == 1)
			sprintf(&(ststr[j][8]),"%s",midstat.lstate);
		      else
			sprintf(&(ststr[j][8]),"(%s)",midstat.lstate);
		      sprintf(&(ststr[j][15]),"%s",midstat.ltype);
		      sprintf(&(ststr[j][25]), "%s",midstat.lpath);
		      sprintf(&(ststr[j][45]), "%.0f",midstat.levt);
		      sprintf(&(ststr[j++][60]), "%9.2e",midstat.lbyt);
		    }
		}	 /* key */
	    }	/* for */
	} /* exists */
    }
  else
    {
      sprintf(&(ststr[j++][0]),"");
      sprintf(&(ststr[j++][0]),"... Logger currently not running...");
    }
  /* Get current Client listing */
  ii = 0;
  sprintf(ststr[j++],"");
  if (db_find_key(hDB, 0, "/system/clients", &hKey) == DB_SUCCESS)
    {
      char *pp, sdummy[64];
      sprintf(&(ststr[j][0]),"Clients:");
      for (i=0 ; ; i++)
	{
	  db_enum_key(hDB, hKey, i, &hSubkey);
	  if (!hSubkey)
	    break;
	  db_get_key(hDB, hSubkey, &key);
	  
	  memset (strtmp,0,sizeof(strtmp));
	  size = sizeof(str);
	  sprintf(strtmp,"name",key.name);
	  db_get_value(hDB, hSubkey, strtmp, midstat.clientn, &size, TID_STRING);
	  memset (strtmp,0,sizeof(strtmp));
	  size = sizeof(str);
	  sprintf(strtmp,"host",key.name);
	  db_get_value(hDB, hSubkey, strtmp, midstat.clienth, &size, TID_STRING);
	  if (ii>2)
	    {
	      ii=0;
	      sprintf(&(ststr[++j][0]),"");
	    }
	  memset (sdummy,0,64);
	  pp = strchr(midstat.clienth,'.');
	  if (pp != NULL) 
	    sprintf(&(ststr[j][10+23*ii]),"%s/%s",midstat.clientn,strncpy(sdummy,midstat.clienth,pp-midstat.clienth));
	  else
	    sprintf(&(ststr[j][10+23*ii]),"%s/%s",midstat.clientn,midstat.clienth);
	  ii++;
	}
      j++;
    }
  
  if (loop == 1)
    sprintf(ststr[j++],"*- [!<cr>] to Exit --- [R<cr>] to Refresh ------------------ Delay:%2.i [sec]--*"
	    ,delta_time/1000);
  else
    sprintf(ststr[j++],"*----------------------------------------------------------------------------*");
  
  cur_max_line = j;
  
  /* remove '/0' */
  for (j=0;j< MAX_LINE;j++)
      while (strlen(ststr[j]) < (LINE_LENGTH - 1))
				ststr[j][strlen(ststr[j])] = ' ';
  
  return;
}


/********************************************************************\

  Name:         malarms.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Monitor the Midas clients and generate Alarms 
                based on crateria from the ODB /Alarms

		/Alarms/<client>/ 
		                 /Enable          1 / 0
				 /Always          1 / 0
				 /Condition       "string"
				 /Condition_index 0
				 /Condition_type  char ('>','<','=')
				 /Condition_value DOUBLE        
				 /Condition_msg   "string"      
				 /Condition_file  "string"
		                 /Start_start     1 / 0
				 /Start_message   "string"
				 /Start_command   "string"
		                 /Stop_remove     1 / 0
                                 /Stop_message    "string"
				 /Stop_command    "string"
				 /Dead_stop_run   1 / 0
				 /Dead_alarm      1 / 0
				 /Alarm_message   "string"
				 /Alarm_file_name "string"

  Revision history
  ------------------------------------------------------------------
  date        by    modification
  --------    ---   ------------------------------------------------
  06-JUN-97   PAA   created
  11-JUN-97   ABJ   
  23-JUL-97   ABJ   add conditions to check that aren't actual
                    running clients. (want to be able to check
                    that data rate hasn't gone to zero when
                    running.

\********************************************************************/
/*------------------------------------------------------------------*/
INT load_alarm_params(HNDLE hDB, HNDLE hKey )
/********************************************************************\

  Routine: load_alarm_params

  Purpose: Loads alarm records into memory, and sets global varible
           pointer to records (pmproc).  Also sets global variable
	   number of records (numprocs).

  Input:
    HNDLE   hDB           Database handle
    HNDLE   hKey          Key handle

  Output:
    only error messages

  Function value:
    CM_SUCCESS         for successful completion
    0                  on error

\********************************************************************/
{
  KEY    key;
  INT    i, status, size;
  HNDLE  hprockey;
  BOOL   exit_flag;
  char   current_client[32];
  char  local_host_name[256];
  
  /* Create or Update /Alarms/Settings */
  status = db_create_record(hDB, 0, "/Alarms/Settings", ALARM_SETTINGS_STR);
  /*
  if (status != DB_SUCCESS)
    {
      cm_msg(MINFO,"malarm","Cannot create alarms/Settings key");
      return (0);
    }
    */  
  db_find_key(hDB, 0, "/Alarms/Settings/Number of current local server", &hKeynlocal);
  size = sizeof(nlocal);
  db_get_value(hDB,0,"/Alarms/Settings/Number of current local server",&nlocal,&size,TID_INT);
  
  /* check if it is yourself who is controlling the alarms */
  if (active_flag)
    return (CM_SUCCESS);
  if (nlocal == 1)
    {
      printf("client checker already running\n");
      active_flag = FALSE;
      db_open_record(hDB, hKeynlocal, &nlocal, sizeof(nlocal), MODE_READ, NULL, NULL);
      return 0;
    }

  /* active this current client */
  active_flag = TRUE;
  nlocal = 1;
  size = sizeof(nlocal);
  db_set_value(hDB,0,"/Alarms/Settings/Number of current local server",&nlocal,size,1,TID_INT);
  
  /* fill client info */
  cm_get_client_info(current_client);
  size = sizeof(current_client);
  db_set_value(hDB,0,"/Alarms/Settings/Current Client Checker",current_client,size,1,TID_STRING);
  size = sizeof(local_host_name);
  gethostname(local_host_name, size);
  db_set_value(hDB,0,"/Alarms/Settings/Host",local_host_name,size,1,TID_STRING);

  /* get the general client checker time interval */
  db_find_key(hDB, 0, "/Alarms/Settings/Client checker time interval", &hKeychktime);
  size = sizeof(checktime);
  db_get_value(hDB,0,"/Alarms/Settings/Client checker time interval",&checktime,&size,TID_DWORD);
  
  exit_flag = FALSE;
  db_create_record(hDB, 0, "/Alarms/Clients/Aclient", ALARM_CLIENT_STR);
  db_find_key(hDB, 0, "/Alarms/Clients", &hKey);
/* scan the process list for memory allocation space */
  for (i=0 ; ; i++)
    {
      db_enum_key(hDB, hKey, i, &hprockey);
      if (!hprockey)
	break;
      /* update record structure if necessary */
      db_create_record(hDB, hprockey, "" , ALARM_CLIENT_STR);
      /* check record size against definition */
      db_get_record_size(hDB, hprockey, 0, &size);
      if (size != sizeof(M_PROCS))
	{
	  cm_msg(MERROR,"malarm","/Alarms/Clients record size mismatch [%i] %d != %d",
		 i, size, sizeof(M_PROCS));
	  exit_flag = TRUE;
	}  
    }  
  if (exit_flag)
    return (0);
  
  /* allocate memory */
  if (pmproc != NULL )
    free(pmproc);
  (char *)pmproc = malloc ((i+1) * sizeof(M_PROCS) );
  memset ((char *)pmproc, 0, (i+1) * sizeof(M_PROCS));
  numprocs = i;
  /* local variables */
  if (ploc != NULL )
    free(ploc);
  (char *)ploc = malloc ((i+1) * sizeof(M_LOCS) );
  memset ((char *)ploc, 0, (i+1) * sizeof(M_LOCS));

  /* load process list */
  for (i=0 ; i < numprocs; i++)
    {
      db_enum_key(hDB, hKey, i, &hprockey);
      if (!hprockey)  
	break;

      /* extract client name from key name */
      db_get_key(hDB,hprockey,&key);
      
      /* extract record */
      size = sizeof(M_PROCS);
      status = db_get_record(hDB, hprockey, pmproc+i , &size, 0);
      if (status != DB_SUCCESS)
	{
	  cm_msg(MERROR,"malarm","cannot get record for /Alarm %i - error:%d",i,status);
	  exit_flag = TRUE;
	}  
      /* adjust time interval in second */
      pmproc[i].mes_time_interval *= 1000;

      /* save (overwrite) client name */
      sprintf(pmproc[i].client,"%s",key.name);
      size = sizeof(pmproc[i].client);
      /*
      db_set_value(hDB,hprockey,"client",pmproc[i].client,size,1,TID_STRING);
      */
    }  
  
  for (i=0; i<numprocs; i++)
    {
      printf("\nE:%d - Always:%d - Client:%s - Sstart:%d - Startmsg:%s"
	     "\n\tStartcommand:%s - Sremove:%d - Stopmsg:%s - Stopcommand:%s"
	     "\n\t - Dstop:%d - Dalarm:%d - Amsg:%s - Afile:%s"
	     "\n\t - Mtime:%d"
	     ,pmproc[i].enable
	     ,pmproc[i].always
	     ,pmproc[i].client
	     ,pmproc[i].Sstart
	     ,pmproc[i].Startmsg
	     ,pmproc[i].Startcommand
	     ,pmproc[i].Sremove 
	     ,pmproc[i].Stopmsg
	     ,pmproc[i].Stopmsg
	     ,pmproc[i].Dstop
	     ,pmproc[i].Dalarm
	     ,pmproc[i].Amsg
	     ,pmproc[i].Afile
	     ,pmproc[i].mes_time_interval);
    }
  return CM_SUCCESS;
}


/*------------------------------------------------------------------*/
INT tr_start_alarms( INT runnum, char * error )
{
  INT    runstate, active, size, i;
  HNDLE  hDB,hKey;

  /* connect to the database */
  cm_get_experiment_database(&hDB, &hKey);

  
  /* get run state STATE_RUNNING|STATE_PAUSED|STATE_STOPPED */
  size = sizeof(runstate);
  db_get_value(hDB, 0, "Runinfo/State", &runstate, &size, TID_INT);


  for ( i=0; i<numprocs; i++)
    {
      active = FALSE;
      if ( cm_exist( pmproc[i].client, FALSE ) == CM_SUCCESS )
	active = TRUE;

      /*
      printf("\nClient:%s  active=%d, enable=%d, Sstart=%d",
	     pmproc[i].client,active,pmproc[i].enable,pmproc[i].Sstart);
	     */
	
      if ( (active == FALSE) && 
	   (pmproc[i].enable == TRUE) &&
	   (pmproc[i].Sstart == TRUE) )
	{
	  /* we need to start the client */
	  /*
	  printf("\nClient:%s  need to start at start",pmproc[i].client);
	  */
	  cm_msg( MTALK, "malarm", pmproc[i].Startmsg );
	  system( pmproc[i].Startcommand );
	}      
    } /* end for */


 return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
void alarms_hotlink( HNDLE hDB, HNDLE hKey, void *info)
{  
  /* Load parameters */
  if ( load_alarm_params( hDB, hKey ) != CM_SUCCESS )  
    printf("\nCannot get alarm parameters");
      
  return;
} 

/*------------------------------------------------------------------*/
INT setup_alarms( HNDLE hDB, HNDLE hKey )
/********************************************************************\

  Routine: setup_alarms

  Purpose: Loads alarm information, registers for start transitions,
           and sets up hot link on alarms in database, so it gets 
	   updated when alarms are changed.
           
  Input:
    HNDLE   hDB           Database handle
    HNDLE   hKey          Key handle

  Function value:
    CM_SUCCESS            on success
    0                     error occured
\********************************************************************/
{
  INT status;


  /* Load parameters */
  if ( load_alarm_params(hDB, hKey ) != CM_SUCCESS )
      return CM_SUCCESS;

  /* register for start transition */
  if ( status=cm_register_transition( TR_START , tr_start_alarms )!= CM_SUCCESS )
    {
      printf("\nCannot register for start transition: Error#%d",status);
    }
  
  /* Set a hotlink on Alarms so can update records in pmproc 
     when they have been changed */
  db_find_key(hDB, 0, "Alarms/Clients", &hKey);
  db_open_record(hDB, hKeychktime, &checktime, sizeof(checktime), MODE_READ, NULL, NULL);
  db_open_record(hDB, hKey, pmproc, sizeof(M_PROCS)*numprocs, MODE_READ, alarms_hotlink, NULL);

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT scan_alarms( HNDLE hDB, HNDLE hKey )

/********************************************************************\

  Routine: scan_alarms

  Purpose: Scans through all alarm records in global variable pmproc
           to see if any alarms should be sounded.

  Input:
    HNDLE   hDB           Database handle
    HNDLE   hKey          Key handle

  Output:
    when alarms need to be sounded

  Function value:
  none
  
  \********************************************************************/
{
  INT    i, size, runstate;
  double cond_value, lcond_value;
  char   tmpmsg[ALINE_LENGTH], str[ALINE_LENGTH];
  char  *esc;
  BOOL   active;
  static INT speaker = TRUE;
  DWORD  msg_time;
  KEY    key;
  
  
  if (!active_flag)
    {
      if (nlocal != 1)
	{
	  db_close_record(hDB,hKeynlocal);
	  setup_alarms(hDB, hKey);
	}
      return 0;
    }
  
  /* See if speaker is present */
  if ( (speaker == TRUE) && ( cm_exist("Speaker",FALSE ) != CM_SUCCESS ) )
    {
      printf("\nMAlarms: No Speaker present!\n"); 
      speaker =  FALSE;
    }
  
  /* get run state STATE_RUNNING|STATE_PAUSED|STATE_STOPPED */
  size = sizeof(runstate);
  db_get_value(hDB, 0, "Runinfo/State", &runstate, &size, TID_INT);
  
  for ( i=0; i<numprocs; i++)
    {
      /*      printf("\nMidasAlarms: %i - cond=%s  - enable=%d - runstate=%d <%i>\n",
	      i, pmproc[i].cond, pmproc[i], runstate,strncmp(pmproc[i].cond,"NONE",4)); 
	      */
      if  ( ( strncmp(pmproc[i].cond,"NONE",4) != 0) && ( pmproc[i].enable == TRUE ) )
	{
	  /* this is not a client - this is a condition to check */
	  /* only check conditions if running */
	  if (runstate == STATE_RUNNING )
	    {
	      /* check condition */
	      db_find_key(hDB, 0, pmproc[i].cond, &hKey);
	      db_get_key (hDB, hKey, &key);
	      if (key.type > TID_DOUBLE)
		break;
	      
	      /* get index */
	      size = sizeof(cond_value);
	      db_get_data_index(hDB, hKey, &cond_value, &size, pmproc[i].cond_idx, key.type); 
	      db_sprintf(str, &cond_value, key.item_size, 0, key.type);
	      lcond_value = atof(str);
	      
	      if (pmproc[i].cond_typ == '>')
		{
		  if ( lcond_value >= pmproc[i].cond_val )
		    {
		      /* give alarm */
		      msg_time = ss_millitime();
		      if ((msg_time - ploc[i].last_condition) > pmproc[i].mes_time_interval)
			{
			  if (pmproc[i].cond_fil[0] != '\0')
			    sprintf(tmpmsg,"@%s@%s",pmproc[i].cond_fil,pmproc[i].cond_msg);
			  else
			    sprintf(tmpmsg,"%s",pmproc[i].cond_msg);
			  while (esc=strstr(tmpmsg,"\\a"))
					{
						strncpy(esc,"\a",1);
						strshfl(esc+1);
					}
			  cm_msg( MTALK, "malarm", tmpmsg );
			  ploc[i].last_condition = msg_time;
			}
		    }
		}
	      else if ( pmproc[i].cond_typ == '<')
		{
		  if ( lcond_value <= pmproc[i].cond_val )
		    {
		      /* give alarm */
		      msg_time = ss_millitime();
		      if ((msg_time - ploc[i].last_condition) > pmproc[i].mes_time_interval)
			{
			  if (pmproc[i].cond_fil[0] != '\0')
			    sprintf(tmpmsg,"@%s@%s",pmproc[i].cond_fil,pmproc[i].cond_msg);
			  else
			    sprintf(tmpmsg,"%s",pmproc[i].cond_msg);
			  while (esc=strstr(tmpmsg,"\\a"))
					{
						strncpy(esc,"\a",1);
						strshfl(esc+1);
					}
			  cm_msg( MTALK, "malarm", tmpmsg );
			  ploc[i].last_condition = msg_time;
			}
		    }
		}
	      else  if ( lcond_value == pmproc[i].cond_val )
		{
		  /* give alarm */
		  msg_time = ss_millitime();
		  if ((msg_time - ploc[i].last_condition) > pmproc[i].mes_time_interval)
		    {
		      if (pmproc[i].cond_fil[0] != '\0')
			sprintf(tmpmsg,"@%s@%s",pmproc[i].cond_fil,pmproc[i].cond_msg);
		      else
			sprintf(tmpmsg,"%s",pmproc[i].cond_msg);
			  while (esc=strstr(tmpmsg,"\\a"))
					{
						strncpy(esc,"\a",1);
						strshfl(esc+1);
					}
		      cm_msg( MTALK, "malarm", tmpmsg);
		      ploc[i].last_condition = msg_time;
		    }
		}
	    }
	}
      else
	{
	  active = FALSE;
	  if ( cm_exist( pmproc[i].client, FALSE ) == CM_SUCCESS )
	    active = TRUE;
	  
	  if ( pmproc[i].enable == TRUE )
	    {
	      if ( pmproc[i].always == TRUE )
		{
		  if ( active == FALSE )
		    {
		      msg_time = ss_millitime();
		      if ((msg_time - ploc[i].last_dead) > pmproc[i].mes_time_interval)
			{			  /* dead message */
			  if (pmproc[i].Afile[0] != '\0')
			    sprintf(tmpmsg,"@%s@%s",pmproc[i].Afile,pmproc[i].Amsg);
			  else
			    sprintf(tmpmsg,"%s",pmproc[i].Amsg);
			  while (esc=strstr(tmpmsg,"\\a"))
					{
						strncpy(esc,"\a",1);
						strshfl(esc+1);
					}
			  cm_msg( MTALK, "malarm", tmpmsg);
			  ploc[i].last_dead = msg_time;
			}
		    }
		  else
		    {
		      ploc[i].last_dead = 0;
		    }
		}
	      else if ( runstate == STATE_RUNNING )
		{
		  if ( (active == FALSE) && (pmproc[i].Dstop == TRUE) )
		    {
		      /* alarm request run stop since important client missing */
		      cm_transition(TR_STOP, 0, NULL, 0, ASYNC);
		      sprintf(tmpmsg, "Request run to stop by %s not running", pmproc[i].client );
		      cm_msg( MTALK, "malarm", tmpmsg );
		      
		    }
		  else if (active == FALSE)
		    {
		      /* give message that client is dead */
		      /*
			printf("\nClient:%s  is dead",pmproc[i].client);
			*/
		      sprintf(tmpmsg, "%s is not running", pmproc[i].client );
		      cm_msg( MTALK, "malarm", tmpmsg );
		    }
		}
	      else if ( runstate == STATE_STOPPED )
		{
		  if ( (active == TRUE) && (pmproc[i].Sremove == TRUE) ) 
		    {
		      /* stop client when run is stopped */
		      sprintf(tmpmsg,"%s",pmproc[i].Stopmsg);
			  while (esc=strstr(tmpmsg,"\\a"))
					{
						strncpy(esc,"\a",1);
						strshfl(esc+1);
					}
		      cm_msg( MTALK, "malarm", tmpmsg);
		      system( pmproc[i].Stopcommand );
		    }
		}
	      
	    }
	}
    } /* end for */
  return 1;
}

/*------------------------------------------------------------------*/
INT cleanup_alarms( HNDLE hDB, HNDLE hKey )
/********************************************************************\

  Routine: cleanup_alarms

  Purpose: remove any connection if necessary

  Input:
    HNDLE   hDB           Database handle
    HNDLE   hKey          Key handle

  Function value:
    CM_SUCCESS            on success
    0                     error occured
\********************************************************************/
{
  INT   size;
  char  current_client[32];
  char  local_host_name[256];
  if (active_flag)
    {
      if (nlocal > 0)
	{
	  nlocal--;
	  size = sizeof(nlocal);
	  db_set_value(hDB,0,"/Alarms/Settings/number of current local server",
		       &nlocal,size,1,TID_INT);
	  size = sizeof(current_client);
	  current_client[0] = 0;
	  db_set_value(hDB,0,"/Alarms/Settings/Current Client Checker",
		       current_client,size,1,TID_STRING);
	  size = sizeof(local_host_name);
	  local_host_name[0]=0;
	  db_set_value(hDB,0,"/Alarms/Settings/Host",local_host_name,size,1,TID_STRING);
	}
    }
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
int main(unsigned int argc,char **argv)
{
  INT    status, last_time, file_mode;
  DWORD  j, i, last_max_line=0;
  HNDLE  hDB, hKey;
  char   host_name[HOST_NAME_LENGTH], expt_name[HOST_NAME_LENGTH], str[32];
  char   ch, svpath[256];
  INT    fHandle;
  INT    msg;
	BOOL   debug;

  esc_flag = ESC_FLAG;
 
  /* set default */
  cm_get_environment (host_name, expt_name);
  svpath[0]=0;
  file_mode = 1;
  loop = 0;
  delta_time = 5000;

  /* get parameters */
  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (strncmp(argv[i],"-l",2) == 0)
			loop = 1;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (strncmp(argv[i],"-w",2) == 0)
	      delta_time = 1000 * (atoi (argv[++i]));
      else if (strncmp(argv[i],"-f",2) == 0)
      	strcpy(svpath, argv[++i]);
      else if (strncmp(argv[i],"-e",2) == 0)
      	strcpy(expt_name, argv[++i]);
      else if (strncmp(argv[i],"-h",2)==0)
        strcpy(host_name, argv[++i]);
      else if (strncmp(argv[i],"-c",2) == 0) {
        strcpy(str, argv[++i]);
	      if (strncmp(str,"n",1)==0 || strncmp(str,"N",1)==0)
           file_mode = 0;
	     }
      else
        {
usage:
        printf("usage: mstat  -l (loop) -w delay (5sec) -f filename (null)\n");
			  printf("              -c compose (Addrun#/norun#)\n");
				printf("             [-h Hostname] [-e Experiment]\n\n");
        return 0;
        }
      }
    }

  /* connect to experiment */
  status = cm_connect_experiment(host_name, expt_name, "MStatus", 0);
  if (status != CM_SUCCESS)
    return 1;
  
#ifdef _DEBUG
  cm_set_watchdog_params(TRUE, 0);
#endif
  
  /* reset page */
  memset (ststr,'\0',sizeof(ststr));

  /* turn off message display, turn on message logging */
  cm_set_msg_print(MT_ALL, 0, NULL);
  
  /* connect to the database */
  cm_get_experiment_database(&hDB, &hKey);

  /* generate status page */
  if (loop == 0)
    {
      j = 0;
      if (svpath[0] != 0)
    	{
				compose_status(hDB, hKey);
				fHandle = open_log_midstat(file_mode, midstat.runnumber, svpath);
				esc_flag = 0;
				compose_status(hDB, hKey);
				while ((j<cur_max_line ) && (ststr[j][0] != '\0'))
					{
						printf("\n%s",ststr[j]);
						write(fHandle,"\n",1);
						write(fHandle, ststr[j], strlen(ststr[j])); 
						j++;
					}
				close (fHandle);
			}
		else
			{
				compose_status(hDB, hKey);
				while ((j<cur_max_line) && (ststr[j][0] != '\0')) 
					printf("\n%s",ststr[j++]);
			}
    }
  else
    {
      /* start client checker if possible */
      setup_alarms( hDB, hKey );
      last_time = 0;
      ss_clear_screen();
      lastchecktime = ss_millitime();
      do 
     	{
	     if ((ss_millitime() - last_time) > delta_time)
	     {
	      last_time = ss_millitime();
	      compose_status(hDB, hKey);
	      j = 0;
	      if (cur_max_line < last_max_line)
      		ss_clear_screen();
	      last_max_line = cur_max_line;

	      while ((j<cur_max_line) && (ststr[j][0] != '\0')) 
	      	ss_printf(0, j, "%s",ststr[j++]);
	     }
	  if (ss_millitime() - lastchecktime > checktime)
	    {
	      status = scan_alarms( hDB, hKey );
	      lastchecktime = ss_millitime();
	    }
	  if (ss_kbhit())
	    {
#if defined(OS_MSDOS) || defined(OS_WINNT)
	      ch = getch();
#else
	      ch = getchar();
#endif
	      if (ch == 'R')
		ss_clear_screen();
	      if (ch == '!')
		break;
            }
	  msg = cm_yield(200);
	} while (msg != RPC_SHUTDOWN && msg != SS_ABORT);
    }      
  printf("\n");
  cleanup_alarms( hDB, hKey );
  cm_disconnect_experiment();
  return 1;
}

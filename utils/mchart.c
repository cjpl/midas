/********************************************************************\
  
  Name:         mchart.c
  Created by:   Pierre-Andre Amaudruz
  
  Contents:     Periodic Midas chart graph data generator.
                Generate the proper configuration and data files for
		the gstripchart application or the GH (Gertjan Hofman)
		stripchart.tcl
		standard running description is:
		mchart -f <file> -q <ODB tree> -c
	   ex:  mchart -f trigger -q /equipment/trigger/statistics -c
	        produce trigger.conf and trigger (data)

		mchart -f <file>.conf -g
	   ex:  mchart -f trigger.conf -gg
	        produce periodically trigger and spawn gstripchart for
		display of the data.
		See mchart -h for further info.
		
  $Log$
  Revision 1.3  2000/09/29 20:10:30  pierre
  fix warning messages

  Revision 1.2  2000/04/20 18:22:38  pierre
  - added "%" to "_" substitution for variable names (midas group, tcl parsing)

  Revision 1.1  2000/04/18 20:29:15  pierre
  - Periodic Midas chart graph data generator. (new application)

\********************************************************************/

#include <fcntl.h>
#include "midas.h"
#include "msystem.h"

#define  CONF_CREATE  1
#define  CONF_APPEND  2

char color[][16] = {"blue","red","brown","black"
		    ,"orange","cyan","yellow","beige"
		    ,"DarkCyan","DarkOliveGreen","goldenrod","DeepPink"};
INT      max_element = 8;
DWORD    delta_time;
BOOL     keep = FALSE, debug=FALSE, create=FALSE, config_given=FALSE;
BOOL     config_done = FALSE;
INT      graph=0, element=0;

FILE    *mchart_open(char * svpath);
INT      mchart_compose(HNDLE hDB, char * svapth, char * eqpstr);
INT      conf_compose(INT action, char * svpath, char * field, float maxval);
float    toplimit=0., botlimit=0.;

/*------------------------------------------------------------------*/
INT conf_compose(INT action, char * svpath, char * field, float val)
{
  FILE  *f;
  char  conpath[256];
  float minval, maxval;

  if (config_done) return 0;
  
  strcpy(conpath, svpath);
  strcat(conpath, ".conf");
  
  switch (action)
  {
  case CONF_CREATE :
    if (keep) return 1;
    /* open device */
    f = fopen(conpath, "w+");
    if (f == NULL)
    {
      printf("Cannot open configuration strip file.\n");
      return 0;
    }
    fprintf(f, "#Equipment:            >%s",field);
    fprintf(f, "\n");
    fprintf(f, "menu:                   on\n");
    fprintf(f, "slider:                 on\n");
    fprintf(f, "type:                   gtk\n");
    
    fprintf(f, "minor_ticks:            12\n");
    fprintf(f, "major_ticks:            6\n");
    
    fprintf(f, "chart-interval:         1.000\n");
    fprintf(f, "chart-filter:           0.500\n");
    fprintf(f, "slider-interval:        0.200\n");
    fprintf(f, "slider-filter:          0.200\n");
    fclose(f);
    break;
    
  case CONF_APPEND :
    if (keep) return 1;
    f = fopen(conpath, "a+");
    if (f == NULL)
    {
      printf("Cannot open configuration strip file.\n");
      return 0;
    }
    if ((element % max_element) == 0) element = 0;

    minval = maxval = val;
    /* check upper limits */
    if (toplimit == 0.)
    { /* use current value */
      if (val == 0.)
	maxval = 5.;
      else if (val > 0.)
	maxval *= 2.;
      else
	maxval /=2.;
    }
    else /* use arg value */
      maxval = toplimit;

    /* check lower limits */
    if (botlimit == 0.)
    { /* use current value */
      if (val == 0.)
	minval = -5.;
      else if (val > 0.)
	minval /= 2.;
      else
	minval *=2.;
    }
    else /* use arg value */
      minval = botlimit;

    fprintf(f, "begin:        %s\n", field );
    fprintf(f, "  filename:     %s\n", svpath);
    fprintf(f, "  fields:       2\n");
    fprintf(f, "  pattern:      %s\n", field);
    fprintf(f, "  equation:     $2\n");
    fprintf(f, "  color:        %s\n", color[element++]);
    fprintf(f, "  maximum:      %6.2f\n", maxval);
    fprintf(f, "  minimum:      %6.2f\n", minval);
    fprintf(f, "  id_char:      1\n");
    fprintf(f, "end:            %s\n", field);
    fclose(f);
    break;
  default:
    printf(" unknown command \n");
  }
  return 1;
}

/*------------------------------------------------------------------*/
FILE *mchart_open(char * svpath)
{
  FILE  *f;
  
  /* open device */
  f = fopen(svpath, "w+");
  if (f == NULL)
  {
    printf("Cannot open temporary strip file.\n");
    exit(1);
  }
  return (f);
}
/*------------------------------------------------------------------*/
INT mchart_get_names(HNDLE hDB, char * eqpstr, char * element,
		     char ** pname, INT * esize)
{
  char  *pslash, *p;
  char   strtmp[128];
  HNDLE  hKeyS, hSubKey;
  KEY    key;
  INT    i, size, status;
  BOOL   bslash=FALSE;
  
  /* convert to upper */
  p = eqpstr;
  for (i=0;i<128;i++) strtmp[i]=0;
  while (*p)  *p++ = (char) tolower(*p);
  if (strncmp(eqpstr,"equipment/",10) == 0) bslash = TRUE;
  if (strncmp(eqpstr,"/equipment/",11) == 0)
  {
    /* if the -q starts with /equipment and 2 more slash are following
       OR
       if the -q starts with equipment and 1 more slash are following
       then I search the Settings for the names of the variables */
    pslash = strchr(eqpstr,'/');
    pslash = strchr(pslash+1,'/');
    if (!bslash) pslash = strchr(pslash+1,'/');
    if (pslash)
    {
      strncpy(strtmp, eqpstr, pslash-eqpstr);
      strcat(strtmp,"/settings");
     
      if (db_find_key(hDB, 0, strtmp, &hKeyS) == DB_SUCCESS)
      {
	db_get_key(hDB, hKeyS, &key);
	for (i=0 ; ; i++)
	{ /* search first for "Names element" */
	  db_enum_key(hDB, hKeyS, i, &hSubKey);
	  if (!hSubKey)
	    break;
	  db_get_key(hDB, hSubKey, &key);
	  if (key.name[5] && key.type == TID_STRING)
	  { /* fmt Names xxxx */
	    p = &(key.name[6]);
	    if (strstr(p, element) == NULL)
	      continue;
	    *esize = key.item_size;
	    size   = *esize * key.num_values;
	    *pname = malloc(size);   
	    status = db_get_data(hDB, hSubKey, *pname, &size, key.type);
	    return key.num_values;
	  }
	}
	/* rescan dir for "Names" only now */
	db_get_key(hDB, hKeyS, &key);
	for (i=0 ; ; i++)
	{
	  db_enum_key(hDB, hKeyS, i, &hSubKey);
	  if (!hSubKey)
	    break;
	  db_get_key(hDB, hSubKey, &key);
	  if (strncmp(key.name, "Names", 5) == 0)
	  { /* fmt Names (only) */
	    *esize = key.item_size;
	    size   = *esize * key.num_values;
	    *pname = malloc(size);   
	    status = db_get_data(hDB, hSubKey, *pname, &size, key.type);
	    return key.num_values;
	  }
	}
      }
    }
  }
  return 0;
}
    
/*------------------------------------------------------------------*/
INT mchart_get_array(FILE * f, char * eqpstr, HNDLE hDB, HNDLE hSubkey, KEY key, char * svpath)
{
  char field[128];
  INT  nfound, status, i, size, esize;
  char *pspace, *pdata=NULL, *pname=NULL;
  float value;

  size = key.num_values * key.item_size;
  pdata = malloc(size);
  status = db_get_data(hDB, hSubkey, pdata, &size, key.type);
  if (status != DB_SUCCESS) return 0;
  if (debug) printf("%s : size:%d\n", key.name, size);
  nfound = mchart_get_names(hDB, eqpstr, key.name, &pname, &esize);
  if (debug) printf("#name:%d #Values:%d\n",nfound, key.num_values);
  /* if names found but no array then use variable names */
  if (key.num_values == 1) nfound = FALSE;
  for (i=0; i < key.num_values; i++)
  {
    if (nfound)
      strcpy(field, (pname+(i*esize)));
    else
    {
      if (key.num_values == 1)
	sprintf(field,"%s", key.name);
      else /* if no settings/Names ... found then compose names here */
	sprintf(field,"%s[%i]", key.name, i);
    }
    while (pspace = strstr(field," ")) *pspace = '_';
    while (pspace = strstr(field,"%")) *pspace = '_';
    if (key.type == TID_INT)
    {
      value = (float) *((INT *) (pdata+(i*key.item_size)));
      fprintf(f, "%s %d\n", field, *((INT *) (pdata+(i*key.item_size))));
    }
    else if (key.type == TID_WORD)
    {
      value = (float) *((WORD *) (pdata+(i*key.item_size)));
      fprintf(f, "%s %d\n", field, *((WORD *) (pdata+(i*key.item_size))));
    }
    else if (key.type == TID_DWORD)
    {
      value = (float) *((DWORD *) (pdata+(i*key.item_size)));
      fprintf(f, "%s %d\n", field, *((DWORD *) (pdata+(i*key.item_size))));
    }
    else if (key.type == TID_FLOAT)
    {
      value = (float) *(pdata+(i*key.item_size));
      fprintf(f, "%s %f\n", field, *((float *) (pdata+(i*key.item_size))));
    }
    else if (key.type == TID_DOUBLE)
    {
      value = (float) *((double *) (pdata+(i*key.item_size)));
      fprintf(f, "%s %e\n", field, *((double *) (pdata+(i*key.item_size))));
    }
    else
      continue;
    
    if (!config_given && !config_done)
      conf_compose(CONF_APPEND, svpath, field, value);
  }
  if (pdata) free (pdata);
  if (pname) free (pname);
  return 1;
}

/*------------------------------------------------------------------*/
INT mchart_compose(HNDLE hDB, char * svpath, char * eqpstr)
{
  FILE     *fHandle;
  INT      j, i ,size, status;
  char     str[256], path[256];
  KEY      key;
  HNDLE    hKey, hSubkey;
  char     strtmp[256], stmp[16], field[256];
  char    *pspace;
  
  size = sizeof(str);
  /* check if dir exists */
  if ((status = db_find_key(hDB, 0, eqpstr, &hKey)) == DB_SUCCESS)
  {
    if ((fHandle = mchart_open(svpath)) != NULL)
    {
      db_get_key(hDB, hKey, &key);
      if (key.type != TID_KEY)
      {
	status = mchart_get_array(fHandle, eqpstr, hDB, hKey, key, svpath);
	if (status < 1)
	{
	  printf("Error: cannot get array for %s\n", eqpstr);
	  return -2;
	}
      }
      else
	for (i=0 ; ; i++)
	{
	  db_enum_key(hDB, hKey, i, &hSubkey);
	  if (!hSubkey)
	    break;
	  db_get_key(hDB, hSubkey, &key);
	  mchart_get_array(fHandle, eqpstr, hDB, hSubkey, key, svpath);
	  if (status < 1)
	  {
	    printf("Error: cannot get array for %s\n", eqpstr);
	    return -3;
	  }
	} /* get value */
    } /* open */
    else
    {
      printf("Error: Cannot open file: %s\n",svpath);
      return -1;
    }
  } /* equ found */
  else
  {
    printf("Error: Equipment:%s not found\n",eqpstr);
    return 0;
  }
  fclose (fHandle);
  config_done = TRUE;
  return 1;
} 

/*------------------------------------------------------------------*/
int main(unsigned int argc,char **argv)
{
  char command[128];
  BOOL   daemon;
  INT    status, last_time;
  DWORD  j, i, last_max_line=0;
  HNDLE  hDB, hKey;
  char   host_name[HOST_NAME_LENGTH], expt_name[HOST_NAME_LENGTH], eqpstr[128];
  char   ch, svpath[256], cmdline[256];
  char   mchart_dir[128];
  INT    fHandle;
  INT    msg;
  
  /* set default */
  cm_get_environment (host_name, expt_name);
  delta_time = 5000;
  daemon = FALSE;
  
  /* get parameters */
  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
  {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'c')
      create = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 's')
      keep = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'g' && argv[i][2] == 'g')
      graph = 2;
    else if (argv[i][0] == '-' && argv[i][1] == 'g' && argv[i][2] == 'h')
      graph = 3;
    else if (argv[i][0] == '-' && argv[i][1] == 'g')
      graph = 1;
    else if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (argv[i][0] == '-')
    {
      if (i+1 >= argc || (argv[i+1][0] == '-' && !isdigit(argv[i+1][1])))
	goto usage;
      if (strncmp(argv[i],"-u",2) == 0)
	delta_time = 1000 * (atoi (argv[++i]));
      else if (strncmp(argv[i],"-f",2) == 0)
      {
	strcpy(svpath, argv[++i]);
	if (strstr(svpath, ".conf") != NULL)
	  config_given = TRUE;
      }
      else if (strncmp(argv[i],"-e",2) == 0)
	strcpy(expt_name, argv[++i]);
      else if (strncmp(argv[i],"-h",2)==0)
	strcpy(host_name, argv[++i]);
      else if (strncmp(argv[i],"-q",2) == 0)
	strcpy(eqpstr, argv[++i]);
      else if (strncmp(argv[i],"-b",2) == 0)
	botlimit = atof (argv[++i]);
      else if (strncmp(argv[i],"-t",2) == 0)
	toplimit = atof (argv[++i]);
    }
    else
    {
   usage:
      printf("usage:   mchart -u updates (5sec) -f filename\n");
      printf("                -f filename (+.conf: use existing file)\n");
      printf("                -q equipment -s (skip conf creation)\n");
      printf(" (override all) -b bot_limit -t top_limit (only with -q) \n");
      printf("                -c (create config only) \n");
      printf("                -g spawn Hofman stripchart or gstripchart if possible\n");
      printf("                -gg forces gstripchart spawning\n");
      printf("                -gh forces  stripchart spawning\n");
      printf("               [-h Hostname] [-e Experiment]\n\n");
      printf("ex: Creation : mchart -e myexpt -h myhost -c \n");
      printf("                      -f mydata -q /equipement/myeqp/variables\n");
      printf("    Running   : mchart -e myexpt -h myhost -f mydata.conf \n");
      printf("    Run/Graph : mchart -e myexpt -h myhost -f mydata.conf -g\n");
      printf("    MCHART_DIR: environment variable for mchar directory\n\n");
      return 0;
    }
  }
  
/* Daemon start */
  if (daemon)
  {
    printf("Becoming a daemon...\n");
    ss_daemon_init();
  }
  
/* connect to experiment */
  status = cm_connect_experiment(host_name, expt_name, "MChart", 0);
  if (status != CM_SUCCESS)
    return 1;
  
  if (debug)
    cm_set_watchdog_params(TRUE, 0);
  
  /* turn off message display, turn on message logging */
  cm_set_msg_print(MT_ALL, 0, NULL);
  
  /* connect to the database */
  cm_get_experiment_database(&hDB, &hKey);
  
  /* initialize ss_getchar() */
  ss_getchar(0);
  
  /* recompose command line */
  sprintf(cmdline, "%s ", argv[1]);
  for (i=2 ; i<argc ; i++)
  {
    strcat (cmdline, " ");
    strcat (cmdline, argv[i]);
  }

  /* retrieve environment */
  if (getenv("MCHART_DIR"))
  {
    strcpy(mchart_dir, getenv("MCHART_DIR"));
    if (mchart_dir[strlen(mchart_dir)-1] != DIR_SEPARATOR)
      strcat(mchart_dir, DIR_SEPARATOR_STR);
    strcat(mchart_dir, svpath);
  }
  else
    strcpy(mchart_dir, svpath);

  /* check if configuration given */
  if (config_given)
  {
    /* extract equipment key */
    FILE *f;
    char strtmp[128];
    char * peqp;
    
    create = FALSE;
    /* Overwrite the -q using the file.conf content */
    f = fopen(mchart_dir, "r");
    if (f == NULL)
    {
      printf("Error: Cannot open %s\n",mchart_dir);
      goto error;
    }
    fgets(strtmp, 128, f);
    if (strstr(strtmp,"#Equipment"))
    {
      peqp = strstr(strtmp,">");
      sprintf(eqpstr, "%s", peqp+1);
    }
    fclose(f);
    peqp = strstr(eqpstr,"\n");
    *peqp = 0;
    /* correct the mchart_dir file name (no extention) */
    peqp = strstr(mchart_dir,".");
    *peqp = 0;
  }
  else
  {
    status = conf_compose(CONF_CREATE, mchart_dir, eqpstr, argc);
    if (status != 1) goto error;
  }
  
  do 
  {
    if ((ss_millitime() - last_time) > delta_time)
    {
      last_time = ss_millitime();
      status = mchart_compose(hDB, mchart_dir, eqpstr);
      if (status != 1) goto error;
      if (create) goto error;
    }

    /* spawn graph once if possible */
    if (graph == 1)
    {
      char command[128];
      char * list=NULL;
      INT  i, nfile;
      char strip[][32]={"/usr/local/bin" ,"stripchart",
			"/usr/bin"       ,"gstripchart",
			"/home/midas/bin","stripchart",
			"/usr/sbin"      ,"gstripchart",
			"/home/chaos/bin","stripchart",
			"/sbin"          ,"gstripchart",
			""};
      
      i=0;
      while(strip[i][0])
      {
	nfile=ss_file_find(strip[i],strip[i+1], &list);
	free(list);
	if (nfile==1)
	  break;
	i += 2;
      }
      if (strip[i][0] == 0)
      {
	printf("No graphic package found in following dir:\n");
	i=0;
	while(strip[i][0])
	{
	  printf("Package : %s/%s \n",strip[i],strip[i+1]); 
	  i += 2;
	}
	break;
      }
      if (i%2)
	sprintf(command,"gstripchart -g 500x200-200-800 -f %s.conf", mchart_dir);
      else
      	sprintf(command,"stripchart %s.conf", mchart_dir);
      if (!daemon) printf("spawning graph with %s ...\n",command);
      ss_system(command);
    }
    else if (graph == 2)
    { /* Gnu graph */
      sprintf(command,"gstripchart -g 500x200-200-800 -f %s.conf", mchart_dir);
      if (!daemon) printf("spawning graph with %s ...\n",command);
      ss_system(command);
    }
    else if (graph == 3)
    { /* Hofman graph */
      sprintf(command,"stripchart %s.conf", mchart_dir);
      if (!daemon) printf("spawning graph with %s ...\n",command);
      ss_system(command);
    }
    
    graph = 0;
    ch = 0;
    while (ss_kbhit())
    {
      ch = ss_getchar(0);
      if (ch == -1)
	ch = getchar();
      if (ch == 'R')
	ss_clear_screen();
      if ((char) ch == '!')
	break;
    }
    msg = cm_yield(200);
  } while (msg != RPC_SHUTDOWN && msg != SS_ABORT && ch != '!');
  
 error:
  printf("\n");
  
  /* reset terminal */
  ss_getchar(TRUE);
  
  cm_disconnect_experiment();
  return 1;
}

/********************************************************************\

  Name:         scaler.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Display the equipment scaler 

  $Log$
  Revision 1.2  1998/10/12 12:19:04  midas
  Added Log tag in header


\********************************************************************/

#include <fcntl.h>
#include "midas.h"
#include "msystem.h"
#define MAX_LINE         256
#define LINE_LENGTH      256
#define stcolumn          11

char    svpath[256], equ_name[256], ststr[MAX_LINE][LINE_LENGTH];
char  * pNames;
DWORD * pValues;
DWORD   v_type, v_tags ;
char    parray[5]="SCAL";
DWORD   channels, name_length, ovrname_size, ovrval_size;
INT     loop, column, runnumber;
DWORD   delta_time;
HNDLE   hKeyNam, hKeySet;
HNDLE   hKeyVal;
HNDLE   hKeyVar;
time_t  full_time;

/*------------------------------------------------------------------*/
INT open_log(INT file_mode, INT runn, char * svpath)
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
INT setup_equipment_page (HNDLE hDB, HNDLE hKey)
{
  char     var_name[256];
  DWORD    l, j, status, n_var, vtot_tags, voff_tags, n_tags;
	INT      size;
  WORD     event_id;
  KEY      key;
  char     *pn;
  HNDLE    hKeyEq;
  
  /* reset page */
  memset (ststr,'\0',sizeof(ststr));
  
  /*  ## */
  /* get equipment name */
  sprintf(var_name,"/equipment/%s",equ_name);
  status = db_find_key(hDB, 0, var_name, &hKeyEq);
  if (status != DB_SUCCESS)
    {
      cm_msg(MERROR, "eqp_setup", "Cannot find /Equipment/%s entry in database", equ_name);
      return 0;
    }
  
  status = db_find_key(hDB, hKeyEq, "Variables", &hKeyVar);
  if (status != DB_SUCCESS)
    {
      cm_msg(MERROR, "eqp_setup", "Cannot find /Equipment/%s/Variables entry in database", equ_name);
      return 0;
    }
  
  status = db_find_key(hDB, hKeyEq, "Settings", &hKeySet);
  if (status != DB_SUCCESS)
    {
      cm_msg(MERROR, "eqp_setup", "Cannot find /Equipment/%s/Settings entry in database", equ_name);
      return 0;
    }
  
  size = sizeof(event_id);
  db_get_value(hDB, hKeyEq, "Common/Event ID", &event_id, &size, TID_WORD);
  
  hKeyVal = 0;
  vtot_tags = voff_tags = 0;
  /* count keys in variables tree */
  for (n_var=0,v_tags=0 ;; n_var++)
    {
      status = db_enum_key(hDB, hKeyVar, n_var, &hKey);
      if (status == DB_NO_MORE_SUBKEYS)
	       break;
      db_get_key(hDB, hKey, &key);
      if (equal_ustring(parray,key.name))
	     {
					voff_tags = vtot_tags;
					v_tags = key.num_values;
					v_type = key.type;
					hKeyVal = hKey;
			 }
      vtot_tags += key.num_values;
    }
    if (hKeyVal == 0)
    {
      cm_msg(MERROR, "eqp_setup", " bank [%s] not found under %s equipment in ODB",parray, equ_name);
      return 0;
    }

  if (n_var == 0)
    {
      cm_msg(MERROR, "eqp_setup", "defined event %d with no variables in ODB", event_id);
      return 0;
    }

   /* count keys in Settings  tree */
  for (n_var=0,n_tags=0 ;; n_var++)
    {
      status = db_enum_key(hDB, hKeySet, n_var, &hKey);
      if (status == DB_NO_MORE_SUBKEYS)
	break;
      db_get_key(hDB, hKey, &key);
      n_tags += key.num_values;
    }
  
  status = db_find_key(hDB, hKeySet, "names", &hKeyNam);
  if (status != DB_SUCCESS)
    {
      cm_msg(MERROR, "eqp_setup", "Cannot find /Equipment/%s/Settings/name entry in database", equ_name);
      return 0;
    }
 
  if (n_var == 0)
    {
      cm_msg(MERROR, "eqp_setup", "defined event %d with no settings in ODB", event_id);
      return 0;
    }
  
  /* check number of val and names */
  if (vtot_tags != n_tags)
    {
      cm_msg(MERROR, "eqp_setup", "number of var(%i) != number of names(%i)",vtot_tags, n_tags);
      return 0;
    }
  channels = vtot_tags;
  
  /* extract item length */
  db_get_key(hDB, hKeyNam, &key);
  name_length = key.item_size;
  
  /* Book space for these arrayes */
  ovrname_size = channels*name_length;
  pNames = malloc(ovrname_size);
  memset (pNames,0,ovrname_size);
  ovrval_size = v_tags*sizeof(DWORD);
  pValues = malloc(ovrval_size);
  memset (pValues,0,ovrval_size);

  /* Get Names record */
  size = ovrname_size;
  db_get_data(hDB, hKeyNam, pNames, &size, TID_STRING);
  
  /* Compose lines starting from 2 */
  l = 2;

  /* channel counter */
  for (j=0;j<v_tags;)
    {
      pn = strchr(pNames+voff_tags*name_length+j*name_length,'%');
      if (pn == NULL)
	pn = pNames+voff_tags*name_length+j*name_length-1;
      j++;
      if (j>= v_tags){
	sprintf(&(ststr[l][stcolumn]),"%s\n",pn+1);
	l++;
	break;
      }
      sprintf(&(ststr[l][stcolumn]),"%s",pn+1);
      
      pn = strchr(pNames+voff_tags*name_length+j*name_length,'%');
      if (pn == NULL)
	pn = pNames+voff_tags*name_length+j*name_length-1;
      j++;
      if (j>= v_tags) {
	sprintf(&(ststr[l][column + stcolumn]),"%s\n",pn+1);
	l++;
	break;
      }
      sprintf(&(ststr[l][column + stcolumn]),"%s",pn+1);

      pn = strchr(pNames+voff_tags*name_length+j*name_length,'%');
      if (pn == NULL)
	pn = pNames+voff_tags*name_length+j*name_length-1;
      j++;
      sprintf(&(ststr[l++][2*column + stcolumn]),"%s\n",pn+1);
    }
  if (loop==1)
    sprintf(ststr[l++],"*- [!<cr>] to Exit --- [R<cr>] to Refresh --------------------Loop :%2.i [sec]-*",delta_time/1000);
  else
    sprintf(ststr[l++],"*------------------------------------------------------------------------------*");

  return 1;
}

/*------------------------------------------------------------------*/
void get_var_values(HNDLE hDB, INT first)
{
  DWORD   l, j;
	INT		  size;
  char  str[256];
  BOOL  Fenabled;

  l=1;

  /* check if equipment enabled */ 
  sprintf(str,"/equipment/%s/common/enabled",equ_name);
  size = sizeof(Fenabled);
  db_get_value(hDB, 0, str, &Fenabled, &size, TID_BOOL);

  /* title */
  time(&full_time);
  strcpy(str, ctime(&full_time));
  str[24] = 0;
  if (!Fenabled)
    sprintf(&(ststr[l++][0]),"*-- Bank: %s --EQUIPMENT NOT ENABLED----------------%s-*\n",parray,str);
  else
    sprintf(&(ststr[l++][0]),"*-- Bank: %s ------ Run:%5d ----Last update time---%s-*\n",parray,runnumber,str);
    
  /* Get Values record */
  if (loop == 0 || first == 1)
    {
      size =  ovrval_size;
      db_get_data(hDB, hKeyVal, (char *)pValues, &size, v_type);
    }  

  /* channel counter */
  if (v_type == TID_FLOAT)
    for (j=0;j<v_tags;)
      {
	memset (&(ststr[l][0]),' ',10);
	sprintf(&(ststr[l][0]),"%10.3f",*((float *)(pValues+j++)));
	
	if (j>= v_tags) break;
	memset (&(ststr[l][column]),' ',10);
	sprintf(&(ststr[l][column]),"%10.3f",*((float *)(pValues+j++)));
	
	if (j>= v_tags) break;
	memset (&(ststr[l][2*column]),' ',10);
	sprintf(&(ststr[l++][2*column]),"%10.3f",*((float *)(pValues+j++)));
	if (j>= channels) break;
      }
  else
    for (j=0;j<v_tags;)
      {
	memset (&(ststr[l][0]),' ',10);
	sprintf(&(ststr[l][0]),"%10d",*(pValues+j++));
	if (j>= v_tags) break;
	memset (&(ststr[l][column]),' ',10);
	sprintf(&(ststr[l][column]),"%10d",*(pValues+j++));
	if (j>= v_tags) break;
	memset (&(ststr[l][2*column]),' ',10);
	  sprintf(&(ststr[l++][2*column]),"%10d",*(pValues+j++));
	if (j>= channels) break;
      }
  return;
}

/*------------------------------------------------------------------*/
void refresh_array(void)
{
  INT j,jj;

  /* remove '\0' */
  j = 0;
  while ((j<MAX_LINE-1) && (ststr[j][0] != '\0'))
    {
      jj=LINE_LENGTH-1;
      while(ststr[j][jj] == '\0')
	{
	  jj--;
	}
      while(jj>0)
	{
	  if (ststr[j][jj] == '\0')
	    memcpy(&(ststr[j][jj])," ",1);
	  jj--;
	}
      j++;
    }
  return;
}

/*------------------------------------------------------------------*/
void update_all(INT hDB, INT hKey)
{
  INT j;

  free(pNames);
  free(pValues);
  ss_clear_screen();
  if (!setup_equipment_page(hDB, hKey))
    return;
  get_var_values(hDB, 1);
  refresh_array();
  j = 0;
  while ((j<MAX_LINE) && (ststr[j][0] != '\0')) 
    ss_printf(0, j, "%s",ststr[j++]);
}

/*------------------------------------------------------------------*/
void update_value(INT hDB, INT pthKey)
{
  INT j;

  get_var_values(hDB, 0);
  refresh_array();
  j = 0;
  while ((j<MAX_LINE) && (ststr[j][0] != '\0')) 
    ss_printf(0, j, "%s",ststr[j++]);
}

/*------------------------------------------------------------------*/
int main(unsigned int argc,char **argv)
{
  INT    last_time, file_mode;
  INT    msg, j, status;                               
  char   host_name[30], expt_name[30], str[256];       
  char   var_name[256];
  char   ch;
  INT    fHandle;
  INT    size;
  HNDLE  hDB, hKey;
  BOOL   debug=FALSE;
	unsigned int i;
  
  /* set default */
  host_name[0] = '\0';
  expt_name[0] = '\0';
  sprintf(equ_name,"scaler");
  file_mode = 1;
  loop = 0;
  svpath[0]=0;
  delta_time = 5000;
  column = 25;
 
  /* get parameters */
  for (i=1 ; i<argc ; i++)
    {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
				debug = TRUE;
			else if (strncmp(argv[i],"-l",2) == 0)
				loop =1;
      else if (argv[i][0] == '-')
	    {
				if (i+1 >= argc || argv[i+1][0] == '-')
					goto usage;
				if (strncmp(argv[i],"-e",2) == 0)
					strcpy(expt_name, argv[++i]);
				else if (strncmp(argv[i],"-h",2)==0)
					strcpy(host_name, argv[++i]);
				else if (strncmp(argv[i],"-q",2)==0)
					strcpy(equ_name, argv[++i]);
				else if (strncmp(argv[i],"-b",2)==0)
					strcpy(parray, argv[++i]);
				else if (strncmp(argv[i],"-p",2)==0)
					column = atoi(argv[++i]);
				else if (strncmp(argv[i],"-w",2)==0)
					delta_time = 1000 * atoi(argv[++i]);
        else if (strncmp(argv[i],"-f",2) == 0)
      	  strcpy(svpath, argv[++i]);
        else if (strncmp(argv[i],"-c",2) == 0) {
          strcpy(str, argv[++i]);
		      if (strncmp(str,"n",1)==0 || strncmp(str,"N",1)==0)
		         file_mode = 0;
					}
			}
			else
			{
usage:
        printf("usage: showeqp  -q equipment (scaler) -w delay (5sec) -f filename (null)\n");
				printf("                -l (loop) -p # (column 25) -c compose (Addrun#/norun#)\n");
				printf("               [-h Hostname] [-e Experiment]\n\n");
        return 0;
			}
    }
  
  /* connect to experiment */
  sprintf(str,"Show%s",parray);
  status = cm_connect_experiment(host_name, expt_name, str, 0);
  if (status != CM_SUCCESS)
    return 1;

  if (debug)
    cm_set_watchdog_params(TRUE, 0);
  
  /* connect to the database */
  cm_get_experiment_database(&hDB, &hKey);
  
  /* check if equipment is running */
  sprintf(var_name,"/equipment/%s/common/Frontend name",equ_name);
  size = sizeof(str);
  status = db_get_value(hDB, 0, var_name, str, &size, TID_STRING);
  status = cm_exist(str,FALSE);
  if (status != CM_SUCCESS)
    {
      cm_msg(MERROR, "eqp_setup", "Equipment:%s not runnning", equ_name);
      goto error;
    }

  /* extract run number */
  size = sizeof(runnumber);
  db_get_value(hDB,0,"/runinfo/run number",&runnumber,&size,TID_INT);

  /* generate status page */
  if (loop == 0)
    {
      j = 0;
      if (svpath[0] != 0)
			 {
				if (!setup_equipment_page(hDB, hKey))
					goto error;
				fHandle = open_log(file_mode, runnumber, svpath);
				get_var_values(hDB, 1);
				ststr[0][0]='\n';
				refresh_array();
				while ((j<MAX_LINE-1) && (ststr[j][0] != '\0'))
					{
						printf("%s",ststr[j]);
						write (fHandle, ststr[j], strlen(ststr[j]));
						j++;
					}
				write(fHandle,"\n",1);
				close (fHandle);
			}
      else
			{
				if (!setup_equipment_page(hDB, hKey))
					goto error;
				get_var_values(hDB, 1); 
				/* title */
				time(&full_time);
				strcpy(str, ctime(&full_time));
				str[24] = 0;
				sprintf(&(ststr[0][0]),"*-v%1.2lf- Equip.: %s -------Current time--------%s-*\n"
					,cm_get_version()/100.0,equ_name,str);
				refresh_array();
				while ((j<MAX_LINE-1) && (ststr[j][0] != '\0')) 
					printf("%s",ststr[j++]);
			}
    }
  else
    {
      last_time = 0;
      j = 0;
      if (!setup_equipment_page(hDB, hKey))
				goto error;
      get_var_values(hDB, 1); 
      /* title */
      time(&full_time);
      strcpy(str, ctime(&full_time));
      str[24] = 0;
      sprintf(&(ststr[0][0]),"*-v%1.2lf- Equip.: %s -------Current time--------%s-*\n"
	      ,cm_get_version()/100.0,equ_name,str);
      refresh_array();
      while ((j<MAX_LINE-1) && (ststr[j][0] != '\0')) 
				printf("%s",ststr[j++]);
      
      /* set hot link to values */
      db_open_record(hDB, hKeyVal, pValues, ovrval_size, MODE_READ, update_value, NULL);
      db_open_record(hDB, hKeyNam, pNames, ovrname_size, MODE_READ, update_all, NULL);
      /* Midas event loop */
      do
			{
				msg = cm_yield(1000);

	  /* update time at least every loop */
				if (ss_millitime() - last_time > delta_time)
					{
						last_time = ss_millitime();
						/* title */
						time(&full_time);
						strcpy(str, ctime(&full_time));
						str[24] = 0;
						sprintf(&(ststr[0][0]),"*-v%1.2lf- Equip.: %s -------Current time--------%s-*\n"
							,cm_get_version()/100.0,equ_name,str);
						refresh_array();
						ss_printf(0, 0, "%s",ststr[0]);
					}
	  
	  /* check keyboard entry */
				if (ss_kbhit())
					{
#if defined(OS_MSDOS) || defined(OS_WINNT)
			      ch = getch();
#else
				    ch = getchar();
#endif
						if (ch == 'R')
							update_all(hDB, hKey );
						if (ch == '!')
							msg = RPC_SHUTDOWN;
					}
			} while (msg != RPC_SHUTDOWN && msg != SS_ABORT);
		}  
error: /* or exit */
  printf("\nThank you...\n");
  free(pNames);
  free(pValues);
  
  cm_disconnect_experiment();
  return 0;
}

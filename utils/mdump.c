/********************************************************************\

  Name:         mdump.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Dump event on screen with MIDAS or YBOS data format

  $Log$
  Revision 1.2  1998/10/12 12:19:03  midas
  Added Log tag in header


  Previous revision history
  ------------------------------------------------------------------
  date        by    modification
  --------    ---   ------------------------------------------------
  11-OCT-96   PAA   created
  20-NoV-97   PAA   add the Midas format

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "ybos.h"

#define  EVENT_SIZE (NET_BUFFER_SIZE-100)
#define  REP_HEADER    1
#define  REP_RECORD    2
#define  REP_LENGTH    3
#define  REP_EVENT     4

DWORD * prevent;

char   bank_name[4], sbank_name[4], svpath[128];
INT    hBufEvent;
INT    save_dsp, evt_display=0;
INT    speed=0, dsp_time=0, dsp_fmt=0, dsp_mode=0, file_mode, bl=-1;
BOOL   via_callback;
char   strtmp[128];
INT    i, data_fmt, count;
KEY    key;
HNDLE  hSubkey;

typedef struct {
  WORD id;
  WORD fmt;
  char Fmt[8];
} FMT_ID;

FMT_ID  eq[32];


/*-------- Check data format ------------*/
DWORD check_data_format(EVENT_HEADER * pevent, INT * i)
  {

		INT jj, ii;
		BOOL dupflag = FALSE;
	/* check in the active FE list for duplicate event ID */
   ii = 0;
     while (eq[ii].fmt)
     {
			jj = ii+1;
			 while (eq[jj].fmt)
				 {
					 if (eq[jj].id == eq[ii].id)
						 {
						 printf("Duplicate event ID [%d] for different equipment", eq[jj].id);
						 printf(" dumping event in raw format\n");
						 dupflag = TRUE;
						 }
					 jj++;
				 }
			 ii++;
			}


	  if (data_fmt != 0)
    {
      *i = 0;
		  strcpy(eq[*i].Fmt, "GIVEN");
      return data_fmt;
    }
  else
    {
      *i = 0;
      if (dupflag)
				strcpy(eq[*i].Fmt, "DUPLICATE");
			else
			{
        do
        {
          if (pevent->event_id == eq[*i].id)
              return eq[*i].fmt;
          (*i)++;
        } while (eq[*i].fmt);
			}
		}
    return 0;
  }

/*----- Replog function ----------------------------------------*/
int replog (int data_fmt, char * rep_file, int bl, int action)
{

  /* open data file */
  if (open_any_datafile (data_fmt, rep_file) != RDLOG_SUCCESS)
	  return (-1);
 
  if (action < REP_LENGTH)
  {
    /* get only physical record header */
    if (skip_any_physrec(data_fmt, bl) != RDLOG_SUCCESS)
      return (-1);
    do {
	      if (action == REP_HEADER)
		    display_any_physhead(data_fmt, dsp_fmt);
	      else if (action == REP_RECORD)
		    display_any_physrec(data_fmt, dsp_fmt);
        if (bl != -1)
          break;
       }
    while (get_any_physrec(data_fmt) == RDLOG_SUCCESS);

  }
  else 
  {
  /* get event */
    if (skip_any_physrec(data_fmt, bl) != RDLOG_SUCCESS)
      return (-1);
    while (get_any_event(data_fmt, bl, prevent) == RDLOG_SUCCESS)
	   {
	    swap_any_event(data_fmt, prevent);
	    if (action == REP_LENGTH)
		    display_any_evtlen(data_fmt, dsp_fmt);
	    if (action == REP_EVENT)
		    display_any_event(prevent, data_fmt, dsp_mode, dsp_fmt);
	   }
  }
  /* close data file */   
  close_any_datafile(data_fmt);
  return 0;
}

/*----- receive_event ----------------------------------------------*/
void process_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER *pheader, void *pevent)
{
  DWORD *pbk, bklen, bktyp;
  INT   internal_data_fmt, i, status, index, size;
  char banklist[512];
  BANK *pmbk;
  BANK_HEADER *pbh;
  char pdata[1000];
 
if (speed == 1)
  {
    /* accumulate received event size */
    size = pheader->data_size + sizeof(EVENT_HEADER);
    count += size;
    return;
  }
if (evt_display > 0)
  {
    evt_display--;

    internal_data_fmt = check_data_format(pheader, &index);

    /* pointer to data section */
    pevent = (DWORD *) ((EVENT_HEADER *)(pheader+1));

	  printf("------------------------ Event# %i --------------------------------\n"
    ,save_dsp-evt_display);
    printf("Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Fmt:%s- Dsize:%i/0x%x\n"
  	,pheader->event_id, pheader->trigger_mask ,pheader->serial_number
	  ,pheader->time_stamp, eq[index].Fmt, pheader->data_size, pheader->data_size);

    /* selection based on data format */
    if ((internal_data_fmt) == FORMAT_YBOS && (swap_any_event(FORMAT_YBOS,pevent) >= 0))
      { /* YBOS FMT swap byte if necessary based on the YBOS bank type */
        if (file_mode != NO_RECOVER)
          { 
	          status = ybos_file_compose(pevent, svpath, file_mode);  
	          printf("ybos_file_compose(status):%d\n",status);
          }
        if (sbank_name[0] != 0)
	        { /* bank name given through argument list */
	          if (ybk_find (pevent,sbank_name,&bklen,&bktyp,&pbk) == YBOS_SUCCESS)
	            {
	              status = ybk_list (pevent,banklist);
	              printf("#banks:%i Bank list:-%s-\n",status,banklist);
	              printf("Bank:%s - Length:%i - Type:%i - pBk:0x%p\n",sbank_name, bklen,bktyp,pbk);
	              display_any_bank(pbk,FORMAT_YBOS,dsp_mode,dsp_fmt);
	            }
        	  else
              printf("Bank -%s- not found\n",sbank_name);
	        }	  
        else
	        { /* Full event pevent -> to lrl */
	          status = ybk_list (pevent,banklist);
	          printf("#banks:%i - Bank list:-%s-\n",status,banklist);
	          if (dsp_mode == DSP_RAW)
	              display_any_event(pheader+1,FORMAT_YBOS,dsp_mode,dsp_fmt);
	          else
	            { /* DSP_BANK */
	              for (i=0;banklist[i]!=0;i+=4)
        	        {
		                strncpy(&bank_name[0],&banklist[i],4);
		                if (ybk_find (pevent,bank_name,&bklen,&bktyp,&pbk) == YBOS_SUCCESS)  
		                  display_any_bank(pbk,FORMAT_YBOS,dsp_mode,dsp_fmt);
		              }
	            }
	        }
      }
      else if (internal_data_fmt == FORMAT_MIDAS && 
              (swap_any_event(FORMAT_MIDAS,pevent) >= 0))
      { /* MIDAS FMT*/
        if (sbank_name[0] != 0)
	        { /* bank name given through argument list */
            pbh = (BANK_HEADER *)pevent;
            pmbk = NULL;

            /* compose bank list */
            banklist[0] = 0;
            i =0;
            do
              {
              /* scan all banks for bank name only */
              size = bk_iterate(pbh, &pmbk, pdata);
              if (pmbk == NULL)
                break;
                strncat (banklist,(char *) pmbk->name,4);
                i++;
              }
            while (1);

            /* search for given bank */
            i =0;
            do
              {
              /* scan all banks for bank name only */
              size = bk_iterate(pbh, &pmbk, pdata);
              if (pmbk == NULL)
                break;
                if (strncmp(sbank_name,(char *) pmbk->name,4) == 0)
                  {
                    i++;
                    break;
                  }
              }
            while (1);
            if (i == 0)
              printf("Bank -%s- not found\n",sbank_name);
            else
              {
                printf("#banks:%i Bank list:-%s-",i,banklist);
	              display_any_bank(pmbk,FORMAT_MIDAS,dsp_mode,dsp_fmt);
              }
	        }	  
        else
	        { /* Full event */
            pbh = (BANK_HEADER *)pevent;
            pmbk = NULL;

            /* compose bank list */
            banklist[0] = 0;
            i =0;
            do
              {
              /* scan all banks for bank name only */
              size = bk_iterate(pbh, &pmbk, pdata);
              if (pmbk == NULL)
                break;
                strncat (banklist,(char *) pmbk->name,4);
                i++;
              }
            while (1);
            printf("#banks:%i Bank list:-%s-",i,banklist);
            printf("\n");

              if (dsp_mode == DSP_RAW)
	                display_any_event(pheader,0,dsp_mode,dsp_fmt);
	            else
                  display_any_event(pheader,FORMAT_MIDAS,dsp_mode,dsp_fmt);
	        }
      }
      else 
      { /* unknown format just dump midas event */
      printf("Data format not supported: %s\n",eq[index].Fmt);
        display_any_event(pheader,0,DSP_RAW, dsp_fmt);  
      }
  if (evt_display == 0)
	  {
	    cm_disconnect_experiment();
	    exit(0);
	  }
  if (dsp_time != 0)
	  ss_sleep(dsp_time);
  } 
  return;
}  

/*------------------------------------------------------------------*/
 int main(unsigned int argc,char **argv)
{
  HNDLE         hDB, hKey;
  char          ch, host_name[HOST_NAME_LENGTH], expt_name[HOST_NAME_LENGTH], str[80];
  char			buf_name[32]=EVENT_BUFFER_NAME, rep_file[128];
  double        rate;
  unsigned int  i, status, start_time, stop_time;
  BOOL          debug, rep_flag;
  INT           event_id, request_id, size, get_flag, action;
  BUFFER_HEADER buffer_header;
  
  /* set default */
  host_name[0] = 0;
  expt_name[0] = 0;
  sbank_name[0]=0;
  svpath[0]=0;
  rep_file[0] = 0;
  event_id = -1; 
  evt_display = 1; 
  get_flag = GET_SOME;
  dsp_fmt = DSP_HEX;
  dsp_mode= DSP_BANK;
  file_mode = NO_RECOVER;
  via_callback = TRUE;
  rep_flag = FALSE;
  dsp_time = 0;
  speed = 0;

  /* Get if existing the pre-defined experiment */
  cm_get_environment (host_name, expt_name);

  /* scan arg list for -x which specify the replog configuration */
  for (i=1 ; i<argc ; i++)
  {
	  if (strncmp(argv[i],"-x",2) == 0)
    {
      if (i+1 == argc)
          goto repusage;
		  if (strncmp(argv[++i],"online",6) != 0)
		  {
			  rep_flag = TRUE;
			  break;
		  }
    }
	}
  if (rep_flag)
  {
  /* get Replay argument list */
  data_fmt = FORMAT_YBOS;
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto repusage;
      if (strncmp(argv[i],"-t",2) == 0) {
        sprintf (str,argv[++i]);
	      if (strncmp(str,"m",1)==0)
	        data_fmt = FORMAT_MIDAS;
	      if (strncmp(str,"y",1)==0)
	        data_fmt = FORMAT_YBOS;
	     }
      else if (strncmp(argv[i],"-m",2) == 0) {
	        sprintf (str,argv[++i]);
	        if (strncmp(str,"r",1)==0)
	          dsp_mode = DSP_RAW;
	        if (strncmp(str,"b",1)==0)
	          dsp_mode = DSP_BANK;
	     }
      else if (strncmp(argv[i],"-w",2) == 0) {
	        sprintf (str,argv[++i]);
	        if (strncmp(str,"h",1)==0)
	          action = REP_HEADER;
	        else if (strncmp(str,"r",1)==0)
			  action = REP_RECORD;
	        else if (strncmp(str,"l",1)==0)
			  action = REP_LENGTH;
	        else if (strncmp(str,"e",1)==0)
			  action = REP_EVENT;
	     }
      else if (strncmp(argv[i],"-f",2) == 0) {
	      sprintf (str,argv[++i]);
	      if (strncmp(str,"d",1)==0)
	        dsp_fmt = DSP_DEC;
	      if (strncmp(str,"x",1)==0)
	        dsp_fmt = DSP_HEX;
	      if (strncmp(str,"a",1)==0)
	        dsp_fmt = DSP_ASC;
	     }
      else if (strncmp(argv[i],"-r",2) == 0)
	      bl = atoi (argv[++i]);
	    else if (strncmp(argv[i],"-x",2) == 0)
      {
        if (i+1 == argc)
          goto repusage;
        strcpy(rep_file, argv[++i]);
      }
      else
      {
repusage:
		  printf("mdump for replay  -x file name \n");
		  printf("        -w what: Header,Record,Length,Event (Header)\n");
      printf("        -t type: Ybos/Midas (Y)  , -m mode: Bank/Raw (B)\n");
		  printf("        -r #: physical number (0) -f : format (X/d/a)\n");
          return 0;
        }
      }
	}		
  }
  else
  {
  /* get parameters for online */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (strncmp(argv[i],"-s",2) == 0)
	    speed = 1;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      else if (strncmp(argv[i],"-x",2) == 0)
		      strncpy(rep_file, argv[++i], 4);
      else if (strncmp(argv[i],"-b",2) == 0)
		      strncpy(sbank_name, argv[++i], 4);
      else if (strncmp(argv[i],"-l",2) == 0)
	      save_dsp = evt_display = atoi (argv[++i]);
      else if (strncmp(argv[i],"-w",2) == 0)
	      dsp_time = 1000 * (atoi (argv[++i]));
      else if (strncmp(argv[i],"-m",2) == 0) {
	        sprintf (str,argv[++i]);
	        if (strncmp(str,"r",1)==0)
	          dsp_mode = DSP_RAW;
	        if (strncmp(str,"y",1)==0)
	          dsp_mode = DSP_BANK;
	     }
      else if (strncmp(argv[i],"-g",2) == 0) {
	      sprintf (str,argv[++i]);
	      if (strncmp(str,"s",1)==0)
	        get_flag = GET_SOME;
	      if (strncmp(str,"a",1)==0)
	        get_flag = GET_ALL;
	     }
      else if (strncmp(argv[i],"-f",2) == 0) {
	      sprintf (str,argv[++i]);
	      if (strncmp(str,"d",1)==0)
	        dsp_fmt = DSP_DEC;
	      if (strncmp(str,"x",1)==0)
	        dsp_fmt = DSP_HEX;
	      if (strncmp(str,"a",1)==0)
	        dsp_fmt = DSP_ASC;
	     }
      else if (strncmp(argv[i],"-i",2) == 0)
  	      event_id = atoi (argv[++i]);
      else if (strncmp(argv[i],"-p",2) == 0)
	      strcpy(svpath, argv[++i]);
      else if (strncmp(argv[i],"-z",2) == 0)
	      strcpy(buf_name, argv[++i]);
      else if (strncmp(argv[i],"-t",2) == 0) {
          sprintf (str,argv[++i]);
	      if (strncmp(str,"m",1)==0)
	        data_fmt = FORMAT_MIDAS;
	      if (strncmp(str,"y",1)==0)
	        data_fmt = FORMAT_YBOS;
	     }
      else if (strncmp(argv[i],"-c",2) == 0) {
          strcpy(str, argv[++i]);
	      if (strncmp(str,"n",1)==0 || strncmp(str,"N",1)==0)
	        file_mode = NO_RUN;
        if (strncmp(str,"a",1)==0 || strncmp(str,"A",1)==0)
	        file_mode = ADD_RUN;
	     }
      else if (strncmp(argv[i],"-h",2)==0)
          strcpy(host_name, argv[++i]);
      else if (strncmp(argv[i],"-e",2) == 0)
      	  strcpy(expt_name, argv[++i]);
      else
        {
usage:
		  printf("mdump   -i evt_id (0=any) -b bank_name (none) -l # (look 1)\n");
		  printf("        -w # (wait 0sec)  -m mode (Bank/raw)  -g type (SOME/all)\n");
		  printf("        -f format (format X/d/a) -s (speed test) -p path (null)\n");
		  printf("        -t type (data type AUTO/Midas/Ybos) -c compose (Addrun#/Norun#)\n");
		  printf("        -z Midas buffer name(SYSTEM) -x Source stream\n");
      printf("       [-h Hostname] [-e Experiment]\n\n");
					return 0;
        }
      }
    }
  }


  /* steer to replog function */
  if (rep_flag)
  {
	 replog (data_fmt, rep_file, bl, action);
	 return 0;
  }
  else
  /* check parameters */
  if (evt_display < 1 && evt_display > 1000)
    {
      printf("mdump-F- <-display arg> out of range (1:1000)\n");
      return -1;
    }
    if(dsp_time < 0 && dsp_time > 100) 
    {
      printf("mdump-F- <-delay arg> out of range (1:100)\n");
      return -1;
    }
    if(dsp_fmt == 0)
      {
      printf("mdump-F- <-format arg> invalid (x,d)\n");
      return -1;
    }
  
  /* connect to experiment */

  status = cm_connect_experiment(host_name, expt_name, "mdump", 0);
  if (status != CM_SUCCESS)
    return 1;
  
#ifdef _DEBUG
  cm_set_watchdog_params(TRUE, 0);
#endif
  
  /* open the "system" buffer, 1M size */
  status = bm_open_buffer(buf_name, EVENT_BUFFER_SIZE, &hBufEvent);
  if (status != BM_SUCCESS) {
	  cm_msg(MERROR,"mdump","bm_open_buffer, unknown buffer");
	  goto error;
  }
  /* set the buffer cache size if requested */
  bm_set_cache_size(hBufEvent, 100000, 0);
  
  /* place a request for a specific event id */
  bm_request_event(hBufEvent, (WORD) event_id, TRIGGER_ALL,
		   get_flag, &request_id, process_event);
  
  start_time = 0;
  if (speed == 1)
    printf("-%1.2lf -- Enter <!> to Exit ------- Midas Dump in Speed test mode ---\n"
    ,cm_get_version()/100.0);
  else
    printf("-%1.2lf -- Enter <!> to Exit ------- Midas Dump ---\n"
    ,cm_get_version()/100.0);

    /* connect to the database */
  cm_get_experiment_database(&hDB, &hKey);

  memset ((char *) eq,0,32*sizeof(FMT_ID));

  /* check if dir exists */
  if (db_find_key(hDB, 0, "/equipment", &hKey) == DB_SUCCESS)
    {
      char strtmp[128], equclient[32];
			INT  l = 0;
      for (i=0 ; ; i++)
	     {
	        db_enum_key(hDB, hKey, i, &hSubkey);
	        if (!hSubkey)
	          break;
	        db_get_key(hDB, hSubkey, &key);
	      /* check if client running this equipment is present */
	      /* extract client name from equipment */
		      size = sizeof(strtmp);
		      sprintf(strtmp,"/equipment/%s/common/Frontend name",key.name);
 			    db_get_value(hDB, 0, strtmp, equclient, &size, TID_STRING);

				/* search client name under /system/clients/xxx/name */
          /* Outcommented 22 Dec 1997 SR because of problem when
             mdump is started before frontend 
          if (status = cm_exist(equclient,FALSE) != CM_SUCCESS)
						continue;
          */

					size = sizeof(WORD);
		      sprintf(strtmp,"/equipment/%s/common/event ID",key.name);
		      db_get_value(hDB, 0, strtmp, &(eq[l]).id, &size, TID_WORD);
		      
		      size = 8;
          sprintf(strtmp,"/equipment/%s/common/Format",key.name);
		      db_get_value(hDB, 0, strtmp, str, &size, TID_STRING);
          if (equal_ustring(str, "YBOS"))
            {
              eq[l].fmt = FORMAT_YBOS;
              strcpy(eq[l].Fmt,"YBOS");
            }
          else if (equal_ustring(str, "MIDAS"))
            {
              eq[l].fmt = FORMAT_MIDAS;
              strcpy(eq[l].Fmt,"MIDAS");
            }
          else if (equal_ustring(str, "DUMP"))
            {
              eq[l].fmt = FORMAT_MIDAS;
              strcpy(eq[l].Fmt,"DUMP");
            }
          else if (equal_ustring(str, "ASCII"))
            {
              eq[l].fmt = FORMAT_MIDAS;
              strcpy(eq[l].Fmt,"ASCII");
            }
          else if (equal_ustring(str, "HBOOK"))
            {
              eq[l].fmt = FORMAT_MIDAS;
              strcpy(eq[l].Fmt,"HBOOK");
            }
          else if (equal_ustring(str, "FIXED"))
            {
              eq[l].fmt = FORMAT_MIDAS;
              strcpy(eq[l].Fmt,"FIXED");
            }
					l++;
        }		  
  	} /* for equipment */

  do
    {
      if (via_callback)
    	{
	  status = cm_yield(1000);
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
	}
      if (speed == 1)
	{
	  /* calculate rates each second */
	  if (ss_millitime() - start_time > 1000)
	    {
	      stop_time = ss_millitime();
	      rate = count / 1024.0 / 1024.0 / ((stop_time-start_time)/1000.0);
	      
	      /* get information about filling level of the buffer */
	      bm_get_buffer_info(hBufEvent, &buffer_header);
	      size = buffer_header.read_pointer - buffer_header.write_pointer;
	      if (size <= 0)
		size += buffer_header.size;
	      printf("Level: %4.3lf %%, ", 100-100.0*size/buffer_header.size);
	      
	      printf("Rate: %1.3lf MB/sec\n", rate);
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
 	      start_time = stop_time;
	      count = 0;
	    }
	}      
    } while (status != RPC_SHUTDOWN && status != SS_ABORT);
error:  
  cm_disconnect_experiment();
  
  return 1;
}

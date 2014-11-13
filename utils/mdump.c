/********************************************************************\
Name:         mdump.c
Created by:   Pierre-Andre Amaudruz

Contents:     Dump event on screen with MIDAS or YBOS data format

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "mrpc.h"
#include "mdsupport.h"

#define  REP_HEADER    1
#define  REP_RECORD    2
#define  REP_LENGTH    3
#define  REP_EVENT     4
#define  REP_BANKLIST  5

char bank_name[4], sbank_name[4];
INT hBufEvent;
INT save_dsp = 1, evt_display = 0;
INT speed = 0, dsp_time = 0, dsp_fmt = 0, dsp_mode = 0, bl = -1;
INT consistency = 0, disp_bank_list = 0, openzip = 0;
BOOL via_callback;
INT i, data_fmt, count;
KEY key;
HNDLE hSubkey;
INT event_id, event_msk;
typedef struct {
   WORD id;
   WORD msk;
   WORD fmt;
  char Fmt[16];
  char Eqname[256];
} FMT_ID;

FMT_ID eq[32];

/*-------- Check data format ------------*/
DWORD data_format_check(EVENT_HEADER * pevent, INT * i)
{
  
  INT jj, ii;
  BOOL dupflag = FALSE;
  
  /* check in the active FE list for duplicate event ID */
  ii = 0;
  while (eq[ii].fmt) {
    jj = ii + 1;
    /* problem occur when duplicate ID with different data format */
    while (eq[jj].fmt) {
      if ((eq[jj].fmt != eq[ii].fmt)
	  && (eq[jj].id == eq[ii].id)
	  && (eq[jj].msk == eq[ii].msk)
	  && eq[ii].id != 0) {
	printf("Duplicate eventID[%d] between Eq:%s & %s  ", eq[jj].id, eq[jj].Eqname,
	       eq[ii].Eqname);
	printf("Dumping event in raw format\n");
	dupflag = TRUE;
      }
      jj++;
    }
    ii++;
  }
  if (data_fmt != 0) {
    *i = 0;
    strcpy(eq[*i].Fmt, "GIVEN");
    return data_fmt;
  } else {
    *i = 0;
    if (dupflag)
      strcpy(eq[*i].Fmt, "DUPLICATE");
    else {
      do {
	if (pevent->event_id == eq[*i].id)
	  return eq[*i].fmt;
	(*i)++;
      } while (eq[*i].fmt);
    }
  }
  return 0;
}

/*----- Replog function ----------------------------------------*/
int replog(int data_fmt, char *rep_file, int bl, int action)
{
  char banklist[STRING_BANKLIST_MAX];
  short int msk, id;
  static char bars[] = "|/-\\";
  static int i_bar;
  static EVENT_HEADER pevh;
  BOOL bank_found = FALSE;
  INT status = 0, i;
  DWORD physize, evtlen;
  void *physrec, *pbk;
  char *pmyevt;
  EVENT_HEADER *pme;
  BANK_HEADER *pmbkh;
  DWORD bklen, bktyp;
  
  if (data_fmt == FORMAT_YBOS)
    assert(!"YBOS not supported anymore");
  
  /* open data file */
  if (md_file_ropen(rep_file, data_fmt, openzip) != SS_SUCCESS)
    return (-1);
  
  switch (action) {
  case REP_HEADER:
  case REP_RECORD:
    /* get only physical record header */
    if (md_physrec_skip(data_fmt, bl) != MD_SUCCESS)
      return (-1);
    do {
      if (action == REP_HEADER)
	status = md_all_info_display(D_HEADER);
      else if (action == REP_RECORD)
	status = md_physrec_display(data_fmt);
      if ((status == MD_DONE) || (bl != -1))
	break;
    }
    while (md_physrec_get(data_fmt, &physrec, &physize) == MD_SUCCESS);
    break;
    
  case REP_LENGTH:
  case REP_EVENT:
  case REP_BANKLIST:
    /* skip will read atleast on record */
    if (md_physrec_skip(data_fmt, bl) != MD_SUCCESS)
      return (-1);
    i = 0;
    while (md_event_get(data_fmt, (void **) &pmyevt, &evtlen) == MD_SUCCESS) {
      status = md_event_swap(data_fmt, pmyevt);
      if ((consistency == 1) && (data_fmt == FORMAT_MIDAS)) {
	pme = (EVENT_HEADER *) pmyevt;
	if (pme->serial_number != pevh.serial_number + 1) {
	  /* event header */
	  printf
	    ("\nLast - Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Dsize:%i/0x%x\n",
	     pevh.event_id, pevh.trigger_mask, pevh.serial_number, pevh.time_stamp,
	     pevh.data_size, pevh.data_size);
	  printf
	    ("Now  - Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Dsize:%i/0x%x\n",
	     pme->event_id, pme->trigger_mask, pme->serial_number,
	     pme->time_stamp, pme->data_size, pme->data_size);
	} else {
	  printf("Consistency check: %c - %i (Data size:%i)\r", bars[i_bar++ % 4],
		 pme->serial_number, pme->data_size);
	  fflush(stdout);
	}
	memcpy((char *) &pevh, (char *) pme, sizeof(EVENT_HEADER));
	continue;
      }
      if (action == REP_LENGTH)
	status = md_all_info_display(D_EVTLEN);
      if ((action == REP_BANKLIST) || (disp_bank_list == 1)) {
	if (data_fmt == FORMAT_YBOS)
	  assert(!"YBOS not supported anymore");
	else if (data_fmt == FORMAT_MIDAS) {
	  pme = (EVENT_HEADER *) pmyevt;
               if (pme->event_id == EVENTID_BOR ||
                   pme->event_id == EVENTID_EOR || pme->event_id == EVENTID_MESSAGE)
		 continue;
               printf
		 ("Evid:%4.4x- Mask:%4.4x- Serial:%d- Time:0x%x- Dsize:%d/0x%x\n",
		  pme->event_id, pme->trigger_mask, pme->serial_number, pme->time_stamp,
		  pme->data_size, pme->data_size);
               pmbkh = (BANK_HEADER *) (((EVENT_HEADER *) pmyevt) + 1);
               status = bk_list(pmbkh, banklist);
	}
	printf("#banks:%i Bank list:-%s-\n", status, banklist);
      } else if ((action == REP_EVENT) && (event_id == EVENTID_ALL) && (event_msk == TRIGGER_ALL) && (sbank_name[0] == 0)) { /* a quick by-pass to prevent bank search if not necessary */
	printf
	  ("------------------------ Event# %i --------------------------------\n",
	   i++);
	md_event_display(pmyevt, data_fmt, dsp_mode, dsp_fmt, sbank_name);
      } else if (action == REP_EVENT) {
	id = EVENTID_ALL;
	msk = TRIGGER_ALL;
	if (data_fmt == FORMAT_MIDAS) {
	  pme = (EVENT_HEADER *) pmyevt;
	  id = pme->event_id;
	  msk = pme->trigger_mask;
	  /* Search bank if necessary */
	  if (sbank_name[0] != 0) {        /* bank name given through argument list */
	    bank_found = FALSE;
	    pmbkh = (BANK_HEADER *) (((EVENT_HEADER *) pmyevt) + 1);
	    if ((pmbkh->data_size + 8) == ((EVENT_HEADER *) pmyevt)->data_size) {
	      if ((status = bk_find(pmbkh, sbank_name, &bklen, &bktyp, (void **) &pbk)) == SS_SUCCESS) { /* given bank found in list */
		status = bk_list(pmbkh, banklist);
		bank_found = TRUE;
	      }
	    }
	  }
	} else if (data_fmt == FORMAT_YBOS) {
	  assert(!"YBOS not supported anymore");
	}
	/* check user request through switch setting (id, msk ,bank) */
	if ((event_msk != TRIGGER_ALL) || (event_id != EVENTID_ALL) || (sbank_name[0] != 0)) {      /* check request or skip event if not satisfied */
	  if (((event_id != EVENTID_ALL) && (id != event_id)) ||   /* id check ==> skip */
	      ((event_msk != TRIGGER_ALL) && (msk != event_msk)) ||        /* msk check ==> skip */
	      ((sbank_name[0] != 0) && !bank_found)) {     /* bk check ==> skip *//* skip event */
	    printf("Searching for Bank -%s- Skiping event...%i\r", sbank_name, i++);
	    fflush(stdout);
	  } else {         /* request match ==> display any event */
	    printf
	      ("------------------------ Event# %i --------------------------------\n",
	       i++);
	    md_event_display(pmyevt, data_fmt, dsp_mode, dsp_fmt, sbank_name);
	  }
	} else {            /* no user request ==> display any event */
	  printf
	    ("------------------------ Event# %i --------------------------------\n",
	     i++);
	  md_event_display(pmyevt, data_fmt, dsp_mode, dsp_fmt, sbank_name);
	}
      }
    }
    break;
  }                            /* switch */
  
  /* close data file */
  md_file_rclose(data_fmt);
  return 0;
}

/*----- receive_event ----------------------------------------------*/
void process_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER * pheader, void *pevent)
{
  static char bars[] = "|/-\\";
  static int i_bar;
  static EVENT_HEADER pevh;
  INT internal_data_fmt, status, index, size;
  DWORD *plrl;
  char banklist[STRING_BANKLIST_MAX];
  BANK_HEADER *pmbh;
  DWORD bklen, bktyp;
  
  if (speed == 1) {
    /* accumulate received event size */
    size = pheader->data_size + sizeof(EVENT_HEADER);
    count += size;
    return;
  }
  if (consistency == 1) {
    if (pheader->serial_number != pevh.serial_number + 1) {
      /* event header */
      printf
	("\nLast - Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Dsize:%i/0x%x\n",
	 pevh.event_id, pevh.trigger_mask, pevh.serial_number, pevh.time_stamp,
	 pevh.data_size, pevh.data_size);
      printf
	("Now  - Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Dsize:%i/0x%x\n",
	 pheader->event_id, pheader->trigger_mask, pheader->serial_number,
	 pheader->time_stamp, pheader->data_size, pheader->data_size);
    } else {
      printf("Consistency check: %c - %i (Data size:%i)\r", bars[i_bar++ % 4],
	     pheader->serial_number, pheader->data_size);
      fflush(stdout);
    }
    memcpy((char *) &pevh, (char *) pheader, sizeof(EVENT_HEADER));
    return;
  }
  if (evt_display > 0) {
    evt_display--;
    
    internal_data_fmt = data_format_check(pheader, &index);
    
    if (internal_data_fmt == FORMAT_YBOS)
      assert(!"YBOS not supported anymore");
    
    /* pointer to data section */
    plrl = (DWORD *) pevent;
    pmbh = (BANK_HEADER *) pevent;
    
    /* display header comes ALWAYS from MIDAS header */
    printf("------------------------ Event# %i ------------------------\n",
	   save_dsp - evt_display);
    /* selection based on data format */
    if (internal_data_fmt == FORMAT_YBOS) {
      assert(!"YBOS not supported anymore");
    } else if ((internal_data_fmt == FORMAT_MIDAS) && (md_event_swap(FORMAT_MIDAS, pheader) >= MD_SUCCESS)) {     /* ---- MIDAS FMT ---- */
      if (sbank_name[0] != 0) {
	BANK *pmbk;
	if (bk_find(pmbh, sbank_name, &bklen, &bktyp, (void **) &pmbk) == SS_SUCCESS) {      /* bank name given through argument list */
	  status = bk_list(pmbh, banklist);
	  printf("#banks:%i Bank list:-%s-", status, banklist);
	  if (bk_is32(pmbh))
	    pmbk = (BANK*)(((char*)pmbk) - sizeof(BANK32));
	  else
	    pmbk = (BANK*)(((char*)pmbk) - sizeof(BANK));
	  md_bank_display(pmbh, pmbk, FORMAT_MIDAS, dsp_mode, dsp_fmt);
	} else {
	  status = bk_list(pmbh, banklist);
	  printf("Bank -%s- not found (%i) in ", sbank_name, status);
	  printf("#banks:%i Bank list:-%s-\n", status, banklist);
	}
      } else {               /* Full event or bank list only */
	if (disp_bank_list) {
	  /* event header */
	  printf
	    ("Evid:%4.4x- Mask:%4.4x- Serial:%d- Time:0x%x- Dsize:%d/0x%x\n",
	     pheader->event_id, pheader->trigger_mask, pheader->serial_number,
	     pheader->time_stamp, pheader->data_size, pheader->data_size);
	  status = bk_list(pmbh, banklist);
	  printf("#banks:%i Bank list:-%s-\n", status, banklist);
	} else
	  md_event_display(pheader, FORMAT_MIDAS, dsp_mode, dsp_fmt, sbank_name);
      }
    } else {                  /* unknown format just dump midas event */
      printf("Data format not supported: %s\n", eq[index].Fmt);
      md_event_display(pheader, FORMAT_MIDAS, DSP_RAW, dsp_fmt, sbank_name);
    }
    if (evt_display == 0) {
      /* do not produce shutdown message */
      cm_set_msg_print(MT_ERROR, 0, NULL);
      cm_disconnect_experiment();
      exit(0);
    }
    if (dsp_time != 0)
      ss_sleep(dsp_time);
  }
  return;
}

/*------------------------------------------------------------------*/
int main(int argc, char **argv)
{
  HNDLE hDB, hKey;
  char host_name[HOST_NAME_LENGTH], expt_name[NAME_LENGTH], str[80];
  char buf_name[32] = EVENT_BUFFER_NAME, rep_file[128];
  double rate;
  unsigned int status, start_time, stop_time;
  BOOL debug = FALSE, rep_flag;
  INT ch, request_id, size, get_flag, action, single, i;
  BUFFER_HEADER buffer_header;
  
  /* set default */
  host_name[0] = 0;
  expt_name[0] = 0;
  sbank_name[0] = 0;
  rep_file[0] = 0;
  event_id = EVENTID_ALL;
  event_msk = TRIGGER_ALL;
  evt_display = 1;
  get_flag = GET_NONBLOCKING;
  dsp_fmt = DSP_UNK;
  dsp_mode = DSP_BANK;
  via_callback = TRUE;
  rep_flag = FALSE;
  dsp_time = 0;
  speed = 0;
  single = 0;
  consistency = 0;
  action = REP_EVENT;
  
  /* Get if existing the pre-defined experiment */
  cm_get_environment(host_name, sizeof(host_name), expt_name, sizeof(expt_name));
  
  /* scan arg list for -x which specify the replog configuration */
  for (i = 1; i < argc; i++) {
    if (strncmp(argv[i], "-x", 2) == 0) {
      if (i + 1 == argc)
	goto repusage;
      if (strncmp(argv[++i], "online", 6) != 0) {
	rep_flag = TRUE;
	break;
      }
    }
  }
  if (rep_flag) {
    /* get Replay argument list */
    data_fmt = 0;
    for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
	debug = TRUE;
      else if (strncmp(argv[i], "-single", 7) == 0)
	single = 1;
      else if (strncmp(argv[i], "-j", 2) == 0)
	disp_bank_list = 1;
      else if (strncmp(argv[i], "-y", 2) == 0)
	consistency = 1;
      else if (argv[i][0] == '-') {
	if (i + 1 >= argc || argv[i + 1][0] == '-')
	  goto repusage;
	if (strncmp(argv[i], "-t", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "m", 1) == 0)
	    data_fmt = FORMAT_MIDAS;
	  if (strncmp(str, "y", 1) == 0)
	    data_fmt = FORMAT_YBOS;
	} else if (strncmp(argv[i], "-b", 2) == 0)
	  strncpy(sbank_name, argv[++i], 4);
	else if (strncmp(argv[i], "-i", 2) == 0)
	  event_id = atoi(argv[++i]);
	else if (strncmp(argv[i], "-k", 2) == 0)
	  event_msk = atoi(argv[++i]);
	else if (strncmp(argv[i], "-m", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "r", 1) == 0)
	    dsp_mode = DSP_RAW;
	  if (strncmp(str, "b", 1) == 0)
	    dsp_mode = DSP_BANK;
	} else if (strncmp(argv[i], "-w", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "h", 1) == 0)
	    action = REP_HEADER;
	  else if (strncmp(str, "r", 1) == 0)
	    action = REP_RECORD;
	  else if (strncmp(str, "l", 1) == 0)
	    action = REP_LENGTH;
	  else if (strncmp(str, "e", 1) == 0)
	    action = REP_EVENT;
	  else if (strncmp(str, "j", 1) == 0)
	    action = REP_BANKLIST;
	} else if (strncmp(argv[i], "-f", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "d", 1) == 0)
	    dsp_fmt = DSP_DEC;
	  if (strncmp(str, "x", 1) == 0)
	    dsp_fmt = DSP_HEX;
	  if (strncmp(str, "a", 1) == 0)
	    dsp_fmt = DSP_ASC;
	} else if (strncmp(argv[i], "-r", 2) == 0)
	  bl = atoi(argv[++i]);
	else if (strncmp(argv[i], "-x", 2) == 0) {
	  if (i + 1 == argc)
	    goto repusage;
	  strcpy(rep_file, argv[++i]);
	} else {
	repusage:
	  printf
	    ("mdump for replay  -x file name    : file to inspect\n");
	  printf
	    ("                  -m mode         : Display mode either Bank or raw\n");
	  printf
	    ("                  -b bank name    : search for bank name (case sensitive)\n");
	  printf
	    ("                  -i evt_id (any) : event id from the FE\n");
	  printf
	    ("                  -[single]       : Request single bank only (to be used with -b)\n");
	  printf
	    ("                  -y              : Serial number consistency check\n");
	  printf
	    ("                  -j              : Display # of banks and bank name list only for all the event\n");
	  printf
	    ("                  -k mask (any)   : trigger_mask from FE setting\n");
	  printf
	    (">>> -i and -k are valid for YBOS ONLY if EVID bank is present in the event\n");
	  printf
	    ("                  -w what         : [h]eader, [r]ecord, [l]ength\n");
	  printf
	    ("                                    [e]vent, [j]bank_list (same as -j)\n");
	  printf
	    (">>> Header & Record are not supported for MIDAS as no physical record structure exists\n");
	  printf
	    ("                  -f format (auto): data representation ([x]/[d]/[a]scii) def:bank header content\n");
	  printf
	    ("                  -r #            : skip event(MIDAS) to #\n");
	  return 0;
	}
      }
    }
  } else {  // Online
    /* get parameters for online */
    for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
	debug = TRUE;
      else if (strncmp(argv[i], "-s", 2) == 0)
	speed = 1;
      else if (strncmp(argv[i], "-y", 2) == 0)
	consistency = 1;
      else if (strncmp(argv[i], "-j", 2) == 0)
	disp_bank_list = 1;
      else if (argv[i][0] == '-') {
	if (i + 1 >= argc || argv[i + 1][0] == '-')
	  goto usage;
	else if (strncmp(argv[i], "-x", 2) == 0)
	  strncpy(rep_file, argv[++i], 4);
	else if (strncmp(argv[i], "-b", 2) == 0)
	  strncpy(sbank_name, argv[++i], 4);
	else if (strncmp(argv[i], "-l", 2) == 0)
	  save_dsp = evt_display = atoi(argv[++i]);
	else if (strncmp(argv[i], "-w", 2) == 0)
	  dsp_time = 1000 * (atoi(argv[++i]));
	else if (strncmp(argv[i], "-m", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "r", 1) == 0)
	    dsp_mode = DSP_RAW;
	  if (strncmp(str, "y", 1) == 0)
	    dsp_mode = DSP_BANK;
	} else if (strncmp(argv[i], "-g", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "s", 1) == 0)
	    get_flag = GET_NONBLOCKING;
	  if (strncmp(str, "a", 1) == 0)
	    get_flag = GET_ALL;
	} else if (strncmp(argv[i], "-f", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "d", 1) == 0)
	    dsp_fmt = DSP_DEC;
	  if (strncmp(str, "x", 1) == 0)
	    dsp_fmt = DSP_HEX;
	  if (strncmp(str, "a", 1) == 0)
	    dsp_fmt = DSP_ASC;
	} else if (strncmp(argv[i], "-i", 2) == 0)
	  event_id = atoi(argv[++i]);
	else if (strncmp(argv[i], "-k", 2) == 0)
	  event_msk = atoi(argv[++i]);
	else if (strncmp(argv[i], "-z", 2) == 0)
	  strcpy(buf_name, argv[++i]);
	else if (strncmp(argv[i], "-t", 2) == 0) {
	  strlcpy(str, argv[++i], sizeof(str));
	  if (strncmp(str, "m", 1) == 0)
	    data_fmt = FORMAT_MIDAS;
	  if (strncmp(str, "y", 1) == 0)
	    data_fmt = FORMAT_YBOS;
	} else if (strncmp(argv[i], "-h", 2) == 0)
	  strcpy(host_name, argv[++i]);
	else if (strncmp(argv[i], "-e", 2) == 0)
	  strcpy(expt_name, argv[++i]);
	else {
	usage:
	  printf("mdump for online  -l #            : display # events (look 1)\n");
	  printf
	    ("                  -f format (auto): data representation ([x]/[d]/[a]scii) def:bank header content\n");
	  printf
	    ("                  -w time         : insert wait in [sec] between each display\n");
	  printf
	    ("                  -m mode         : Display mode either Bank or raw\n");
	  printf
	    ("                  -j              : Display # of banks and bank name list only for all the event\n");
	  printf
	    ("                  -b bank name    : search for bank name (case sensitive)\n");
	  printf
	    ("                  -i evt_id (any) : event id from the FE\n");
	  printf
	    ("                  -k mask (any)   : trigger_mask from FE setting\n");
	  printf
	    ("                  -g type         : sampling mode either SOME or all)\n");
	  printf
	    ("                  -s              : report buffer data rate and fill level\n");
	  printf
	    ("                  -s -d           : for use with -s: also report all buffer clients and requests\n");
	  printf
	    ("                  -t type (auto)  : Bank format (Midas/Ybos)\n");
	  printf
	    ("                  -x Source       : Data source selection def:online (see -x -h)\n");
	  printf
	    ("                  -y              : Serial number consistency check\n");
	  printf
	    (">>> in case of -y it is recommented to used -g all\n");
	  printf
	    ("                  -z buffer name  : Midas buffer name default:[SYSTEM]\n");
	  printf
	    ("                  [-h Hostname] [-e Experiment]\n\n");
	  return 0;
	}
      }
    }
  }
  
  // Set flag to inform that we're coming from mdump -> open zip  (.mid.gz)
  if ((strstr(argv[0], "mdump") == NULL)) openzip = 0; else  openzip = 1;

  if ((sbank_name[0] != 0) && single) dsp_mode += 1;
  
  if (rep_flag && data_fmt == 0) {
    char *pext;
    if ((pext = strrchr(rep_file, '.')) != 0) {
      if (equal_ustring(pext + 1, "mid"))
	data_fmt = FORMAT_MIDAS;
      else if (equal_ustring(pext + 1, "ybs"))
	data_fmt = FORMAT_YBOS;
      else if (equal_ustring(pext + 1, "gz")) {
	if ((pext = strchr(rep_file, '.')) != 0) {
	  if (strstr(pext + 1, "mid"))
	    data_fmt = FORMAT_MIDAS;
	  else if (strstr(pext + 1, "ybs"))
	    data_fmt = FORMAT_YBOS;
	} else {
	  printf
	    ("\n>>> data type (-t) should be set by hand in -x mode for tape <<< \n\n");
	  goto usage;
	}
      } else {
	printf
	  ("\n>>> data type (-t) should be set by hand in -x mode for tape <<< \n\n");
	goto usage;
      }
    }
  }
  
  /* steer to replog function */
  if (rep_flag) {
    replog(data_fmt, rep_file, bl, action);
    return 0;
  } else {
    /* check parameters */
    if (evt_display < 1 && evt_display > 1000) {
      printf("mdump-F- <-display arg> out of range (1:1000)\n");
      return -1;
    }
  }
  if (dsp_time < 0 && dsp_time > 100) {
    printf("mdump-F- <-delay arg> out of range (1:100)\n");
    return -1;
  }
  
  /* do not produce startup message */
  cm_set_msg_print(MT_ERROR, 0, NULL);
  
  /* connect to experiment */
  status = cm_connect_experiment(host_name, expt_name, "mdump", 0);
  if (status != CM_SUCCESS)
    return 1;
  
#ifdef _DEBUG
  cm_set_watchdog_params(TRUE, 0);
#endif
  
  /* open the shared memory buffer with proper size */
  status = bm_open_buffer(buf_name, 2*MAX_EVENT_SIZE, &hBufEvent);
  if (status != BM_SUCCESS && status != BM_CREATED) {
    cm_msg(MERROR, "mdump", "bm_open_buffer, unknown buffer");
    goto error;
  }
  /* set the buffer cache size if requested */
  bm_set_cache_size(hBufEvent, 100000, 0);
  
  /* place a request for a specific event id */
  bm_request_event(hBufEvent, (WORD) event_id, (WORD) event_msk,
		   get_flag, &request_id, process_event);
  
  start_time = 0;
  if (speed == 1)
    printf("-%d -- Enter <!> to Exit ------- Midas Dump in Speed test mode ---\n",
	   cm_get_revision());
  else
    printf("-%d -- Enter <!> to Exit ------- Midas Dump ---\n", cm_get_revision());
  
  /* connect to the database */
  cm_get_experiment_database(&hDB, &hKey);
  
  
  {   /* ID block */
    INT l = 0;
    memset((char *) eq, 0, 32 * sizeof(FMT_ID));
    /* check if dir exists */
    if (db_find_key(hDB, 0, "/equipment", &hKey) == DB_SUCCESS) {
      char strtmp[256], equclient[32];
      for (i = 0;; i++) {
	db_enum_key(hDB, hKey, i, &hSubkey);
	if (!hSubkey)
	  break;
	db_get_key(hDB, hSubkey, &key);
	strlcpy(eq[l].Eqname, key.name, sizeof(eq[l].Eqname));
	/* check if client running this equipment is present */
	/* extract client name from equipment */
	size = sizeof(strtmp);
	sprintf(strtmp, "/equipment/%s/common/Frontend name", key.name);
	db_get_value(hDB, 0, strtmp, equclient, &size, TID_STRING, TRUE);
	
	/* search client name under /system/clients/xxx/name */
	/* Outcommented 22 Dec 1997 SR because of problem when
	   mdump is started before frontend 
	   if (status = cm_exist(equclient,FALSE) != CM_SUCCESS)
	   continue;
	*/
	size = sizeof(WORD);
	sprintf(strtmp, "/equipment/%s/common/event ID", key.name);
	db_get_value(hDB, 0, strtmp, &(eq[l]).id, &size, TID_WORD, TRUE);
	
	size = sizeof(WORD);
	sprintf(strtmp, "/equipment/%s/common/Trigger mask", key.name);
	db_get_value(hDB, 0, strtmp, &(eq[l]).msk, &size, TID_WORD, TRUE);
	
	size = sizeof(str);
	sprintf(strtmp, "/equipment/%s/common/Format", key.name);
	db_get_value(hDB, 0, strtmp, str, &size, TID_STRING, TRUE);
	if (equal_ustring(str, "YBOS")) {
	  eq[l].fmt = FORMAT_YBOS;
	  strcpy(eq[l].Fmt, "YBOS");
	} else if (equal_ustring(str, "MIDAS")) {
	  eq[l].fmt = FORMAT_MIDAS;
	  strcpy(eq[l].Fmt, "MIDAS");
	} else if (equal_ustring(str, "DUMP")) {
	  eq[l].fmt = FORMAT_MIDAS;
	  strcpy(eq[l].Fmt, "DUMP");
	} else if (equal_ustring(str, "ASCII")) {
	  eq[l].fmt = FORMAT_MIDAS;
	  strcpy(eq[l].Fmt, "ASCII");
	} else if (equal_ustring(str, "HBOOK")) {
	  eq[l].fmt = FORMAT_MIDAS;
	  strcpy(eq[l].Fmt, "HBOOK");
	} else if (equal_ustring(str, "FIXED")) {
	  eq[l].fmt = FORMAT_MIDAS;
	  strcpy(eq[l].Fmt, "FIXED");
	}
	l++;
      }
    }
      
    /* for equipment */
    /* check for EBuilder */
    if (db_find_key(hDB, 0, "/EBuilder/Settings", &hKey) == DB_SUCCESS) {
      sprintf(eq[l].Eqname, "EBuilder");
      /* check if client running this equipment is present */
      /* search client name under /system/clients/xxx/name */
      /* Outcommented 22 Dec 1997 SR because of problem when
	 mdump is started before frontend 
	 if (status = cm_exist(equclient,FALSE) != CM_SUCCESS)
	 continue;
      */
      size = sizeof(WORD);
      db_get_value(hDB, hKey, "Event ID", &(eq[l]).id, &size, TID_WORD, TRUE);
      
      size = sizeof(WORD);
      db_get_value(hDB, hKey, "Trigger mask", &(eq[l]).msk, &size, TID_WORD, TRUE);
      
      size = sizeof(str);
      db_get_value(hDB, hKey, "Format", str, &size, TID_STRING, TRUE);
      if (equal_ustring(str, "YBOS")) {
	eq[l].fmt = FORMAT_YBOS;
	strcpy(eq[l].Fmt, "YBOS");
      } else if (equal_ustring(str, "MIDAS")) {
	eq[l].fmt = FORMAT_MIDAS;
	strcpy(eq[l].Fmt, "MIDAS");
      } else {
	printf("Format unknown for Event Builder (%s)\n", str);
	goto error;
      }
      l++;
    }

    /* Debug */
    if (debug) {
      i = 0;
      printf("ID\tMask\tFormat\tEq_name\n");
      while (eq[i].fmt) {
	printf("%d\t%d\t%s\t%s\n", eq[i].id, eq[i].msk, eq[i].Fmt, eq[i].Eqname);
	i++;
      }
    }
  }  /* ID block */
  
  do {
    if (via_callback)
      status = cm_yield(1000);
    if (speed == 1) {
      /* calculate rates each second */
      if (ss_millitime() - start_time > 1000) {
	stop_time = ss_millitime();
	rate = count / 1024.0 / 1024.0 / ((stop_time - start_time) / 1000.0);
	
	/* get information about filling level of the buffer */
	bm_get_buffer_info(hBufEvent, &buffer_header);
	size = buffer_header.read_pointer - buffer_header.write_pointer;
	if (size <= 0)
	  size += buffer_header.size;
	printf("Level: %4.3f %%, ", 100 - 100.0 * size / buffer_header.size);
	printf("Rate: %1.3f MB/sec\n", rate);
	
	if (debug) {
	  int i, j;
	  int now = ss_millitime();
	  
	  printf("buffer name [%s], clients: %d, max: %d, size: %d, rp: %d, wp: %d, ine: %d, oute: %d\n", 
		 buffer_header.name,
		 buffer_header.num_clients,
		 buffer_header.max_client_index,
		 buffer_header.size,
		 buffer_header.read_pointer,
		 buffer_header.write_pointer,
		 buffer_header.num_in_events,
		 buffer_header.num_out_events
		 );
	  
	  for (i=0; i<buffer_header.max_client_index; i++)
	    if (buffer_header.client[i].pid) {
	      printf("  client %d: name [%s], pid: %d, port: %d, rp: %d, max_req: %d, read_wait: %d, write_wait: %d, wake_up: %d, get_all: %d, active: %d, timeout: %d\n",
		     i,
		     buffer_header.client[i].name,
		     buffer_header.client[i].pid,
		     buffer_header.client[i].port,
		     buffer_header.client[i].read_pointer,
		     buffer_header.client[i].max_request_index,
		     buffer_header.client[i].read_wait,
		     buffer_header.client[i].write_wait,
		     buffer_header.client[i].wake_up,
		     buffer_header.client[i].all_flag,
		     now - buffer_header.client[i].last_activity,
		     buffer_header.client[i].watchdog_timeout);
	      
	      for (j=0; j<buffer_header.client[i].max_request_index; j++)
		if (buffer_header.client[i].event_request[j].valid)
		  printf("    request %d: id: %d, valid: %d, event_id: %d, trigger_mask: 0x%x, type: %d\n",
			 j,
			 buffer_header.client[i].event_request[j].id,
			 buffer_header.client[i].event_request[j].valid,
			 buffer_header.client[i].event_request[j].event_id,
			 buffer_header.client[i].event_request[j].trigger_mask,
			 buffer_header.client[i].event_request[j].sampling_type);
	    }
	}
	
	start_time = stop_time;
	count = 0;
      }
    } // Speed
  
    /*  speed */
    /* check keyboard */
    ch = 0;
    if (ss_kbhit()) {
      ch = ss_getchar(0);
      if (ch == -1)
	ch = getchar();
      if ((char) ch == '!')
	break;
    }
  } while (status != RPC_SHUTDOWN && status != SS_ABORT);

 error:
  /* do not produce shutdown message */
  cm_set_msg_print(MT_ERROR, 0, NULL);
  
  cm_disconnect_experiment();
  
  return 1;
}


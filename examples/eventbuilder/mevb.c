/********************************************************************\
Name:         mevb.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Main Event builder task.
  $Log$
  Revision 1.3  2002/05/08 20:51:41  midas
  Added extra parameter to function db_get_value()

  Revision 1.2  2002/01/17 23:34:14  pierre
  doc++ format

  Revision 1.1.1.1  2002/01/17 19:49:54  pierre
  Initial Version

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "mevb.h"
#include "ybos.h"

EBUILDER_SETTINGS    ebset;
EBUILDER_STATISTICS  ebstat;
EBUILDER_CHANNEL     ebch[MAX_CHANNELS];

HNDLE hDB, hKey, hSubkey, hEKey, hSetKey, hStatKey, hChKey;
HNDLE hFBC1stat, hFBC2stat;
KEY   key;
BOOL  daemon=FALSE, debug=FALSE, debug1=FALSE, debug2=FALSE;
BOOL  debug3=FALSE;
DWORD max_event_size;
BOOL  stop_requested = TRUE;
BOOL  stopped = TRUE;
BOOL  wheel = FALSE;
INT   run_state=0;
DWORD start_time = 0, stop_time=0, request_stop_time=0;
DWORD gbl_bytes_sent=0, gbl_events_sent=0;
DWORD cdemask=0;
INT   gbl_run=0;
BOOL  pulser1 = FALSE, pulser2 = FALSE;
BOOL  pulserBAD = FALSE, headerBAD = TRUE;
BOOL  next=FALSE, next2=FALSE;
INT   npulser1 = 0, npulser2 = 0;
DWORD hfbu1, hfbu2;
char  str1[128],str2[128],str3[128];
INT   (*meb_fragment_add)(char *, char *, INT *);

INT source_booking(INT nfrag);
INT eb_mfragment_add(char * pdest, char * psrce, INT *size);
INT eb_yfragment_add(char * pdest, char * psrce, INT *size);

INT eb_begin_of_run(INT, char *);
INT eb_end_of_run(INT, char *);
INT eb_user(INT, EBUILDER_CHANNEL *, EVENT_HEADER *, void *, INT *);


/*--------------------------------------------------------------------*/
/* eb_mfragment_add_()
@memo append one fragment to current destination event.
@param fmt Data format
@param pdest Destination pointer
@param psrce Fragment source pointer
@param size  Current destination event size (byte)
@return EB_SUCCESS
*/
INT eb_mfragment_add(char * pdest, char * psrce, INT *size)
{
  BANK_HEADER  *psbh, *pdbh;
  char         *psdata, *pddata;
  INT          bksize, status;

  /* Condition for new EVENT the data_size should be ZERO */
  *size = ((EVENT_HEADER *) pdest)->data_size;
  if (*size) {
    /* NOT the first fragment */
    
    /* destination pointer */
    pddata  = pdest + *size + sizeof(EVENT_HEADER);
    
    /* Swap event if necessary */
    psbh = (BANK_HEADER *) (((EVENT_HEADER *)psrce)+1);
    status = bk_swap(psbh, FALSE);
    
    /* source pointer */
    psbh    = (BANK_HEADER *)(((EVENT_HEADER *)psrce)+1);
    psdata  = (char *) (psbh+1);
    
    /* copy all banks without the bank header */
    bksize = psbh->data_size;
    
    /* copy */
    memcpy(pddata, psdata, bksize);
    
    /* update event size */
    ((EVENT_HEADER *) pdest)->data_size += bksize;
    
    /* update bank size */
    pdbh    = (BANK_HEADER *)(((EVENT_HEADER *)pdest)+1);
    pdbh->data_size += bksize;
    
    *size = ((EVENT_HEADER *) pdest)->data_size;
  }
  else {
    /* First event (without the event header) */
    *size = ((EVENT_HEADER *) psrce)->data_size;

    /* Swap event if necessary */
    psbh = (BANK_HEADER *) (((EVENT_HEADER *)psrce)+1);
    status = bk_swap(psbh, FALSE);
    
    /* copy */
    memcpy (pdest, psrce, *size + sizeof(EVENT_HEADER));
  }
  return CM_SUCCESS;
}

/*--------------------------------------------------------------------*/
INT eb_yfragment_add(char * pdest, char * psrce, INT *size)
{
  /* pdest : EVENT_HEADER pointer
     psrce : EVENT_HEADER pointer
     Keep pbkh for later incrementation
  */
  static DWORD yserial=0;
  char         *psdata, *pddata;
  DWORD        *pslrl, *pdlrl;
  INT          bksize, tmp_bksize, tmp2_bksize, status;

  /* Condition for new EVENT the data_size should be ZERO */
  *size = ((EVENT_HEADER *) pdest)->data_size;
  tmp_bksize = 0;
  if(debug2)
    printf("*size %d \n",*size);

  /* the Midas header is present for logger */
  if (*size)
  { /* already filled with fragment */
    if(debug2) printf("In YBOS format section\n");
    
    /* destination pointer (MIDAS control) */
    pddata  = pdest + *size + sizeof(EVENT_HEADER);
    
    /* source pointer */
    pslrl   = (DWORD *)(((EVENT_HEADER *)psrce)+1);
    /* Swap event if necessary */
    status = ybos_event_swap(pslrl);
//    printf("swap2 %d\n", status);
    if (debug) printf("Yser1:%d - Yser2:%d\n", yserial, pslrl[7]);
    if (yserial != pslrl[7])
    {
      printf("ERROR: Yser1:%d - Yser2:%d\n", yserial, pslrl[7]);
      cm_msg(MERROR, "eb_add_yevent",
	     "fragment mismatch at ev %d",yserial);
      return EB_ERROR;
    }
  
    /* Do not include LRL */
    psdata  = (char *) (pslrl+1);
    
    /* copy size (lrl included I*4, remove it) */
    bksize = (*pslrl);
    
    /* copy in bytes */
    if(debug2)
      printf("L1; bksize 0x%x; psdata 0x%x; pddata 0x%x\n", 
	     bksize,psdata,pddata);
    tmp_bksize = bksize;
    tmp_bksize *=4;            /* in bytes for memcpy */
    if(debug2)
      printf("L1;tmp_bksize 0x%x\n", tmp_bksize);
    
    /* append fragment */
    memcpy(pddata, psdata, tmp_bksize);
    
    /* update Midas header event size */
    ((EVENT_HEADER *) pdest)->data_size += tmp_bksize;
    
    /* update LRL size (I*4) */
    pdlrl  = (DWORD *)(((EVENT_HEADER *)pdest)+1);
    tmp_bksize = tmp_bksize/4;      /* back in DWORD */
    tmp2_bksize = *pdlrl;           /* current lrl content */
    tmp_bksize += tmp2_bksize;      /* add fragment size in DWORD */
    if(debug2)
      printf("LRL updated tmp_bksize before 0x%x\n", tmp_bksize);
    
    *pdlrl =tmp_bksize;             /* Save new lrl to destination */
    if(debug2)
      printf("LRL updated after 0x%x\n", *pdlrl);
    
    /* Return event size in bytes */
    *size = ((EVENT_HEADER *) pdest)->data_size;
    if(debug2)
      printf("*size 0x%x\n", *size);      
    {
      DWORD *pybk, *pdata,bklen, bktype;
      DWORD data;
      INT ii, istat, sdata;
      istat = ybk_find(pslrl, "FBU2", &bklen,&bktype,(void *)&pybk);
      if(istat == YB_SUCCESS)
	{
	  pdata = (DWORD *)((YBOS_BANK_HEADER *)pybk + 1);
	  /* 1st word should be header for TDC slot 24 = 0xC000xxss)
             where ss = size */
	  data = *pdata++;
	  hfbu2 = data;
	  if(next2){
	    //printf("EV %d; Data FBU2 %0x\n",ebch[0].serial,hfbu2);
	    next2 = FALSE;
	  } 
	  if(next){
	    //printf("EV %d; Data FBU2 %0x\n",ebch[0].serial,hfbu2);
	    next2 = TRUE;
	  } 
	  if ( (data & 0xffff0000) == 0xc0000000) {
	    sdata = data & 0x7ff;
            //printf("FBU2: In %0x, found slot 24, size %d\n",data,sdata);
	    for (ii = 1;ii<sdata;ii++)
	      {
		data = *pdata++;
		if((data & 0x00ff0000) == 0xa10000) {
		  /*printf("Ev %d, C2 found pulser 0x%x, at index %d, size %d\n",
		    ebch[0].serial,data,ii,sdata); */
		  pulser2 = TRUE;
		  break;
		}
	      }
	  }
	  else {
	    sprintf(str3,"EV %d; Data FBU2 is not header for TDC 0xC000 %0x",
		    ebch[0].serial,data);
            headerBAD=TRUE;
	  }
	  //if(pulser2)printf(" Pulser2 found in FBU2, event %d\n",yserial);
	}
      else
	printf("Error in ybk_find\n");
    }
  }
  else
  { /* new destination event */
    /* The composed event will have the MIDAS header anyway
       Will be striped by the logger.
       Copy the first full event ( no EVID supression )
       First event (without the event header) */
    
    /* source pointer */
    pslrl   = (DWORD *)(((EVENT_HEADER *)psrce)+1);

    /* Swap event if necessary */
    status = ybos_event_swap(pslrl);
//    printf("swap1 %d\n", status);
    yserial = pslrl[7];
    
    *size = ((EVENT_HEADER *) psrce)->data_size;
    if(debug2)
      printf("L0; size %d; pdest 0x%x; psrce 0x%x\n", 
	     *size,pdest, psrce);            
    /* copy */
    memcpy (pdest, psrce, *size + sizeof(EVENT_HEADER));
    /* find bank FBU1 and look for pulser */
    {
      DWORD *pybk, *pdata,bklen, bktype;
      DWORD data;
      INT ii, istat, sdata;
      istat = ybk_find(pslrl, "FBU1", &bklen,&bktype,(void *)&pybk);
      if(istat == YB_SUCCESS)
	{
	  pdata = (DWORD *)((YBOS_BANK_HEADER *)pybk + 1);
	  /* 1st word should be header for TDC slot 24 = 0xC000xxss)
             where ss = size */
	  data = *pdata++;
	  hfbu1 = data;
	  if(next){
	    //printf("EV %d; Data FBU1 %0x\n",ebch[0].serial,data);
	    next = FALSE;
	  } 
	  if ( (data & 0xffff0000) == 0xc0000000) {
	    sdata = data & 0x7ff;
	    //            printf("FBU1: In %0x, found slot 24, size %d\n",data,sdata);
	    for (ii = 1;ii<sdata;ii++)
	      {
		data = *pdata++;
		if((data & 0x00ff0000) == 0xa10000){
		  /*printf("Ev %d, C1 found pulser 0x%x, at index %d, size %d\n",
		    ebch[0].serial,data,ii,sdata);  */
		  pulser1 = TRUE;
		  break;
		}
	      }
	  }
	  else {
            sprintf(str3,"EV %d; Data FBU1 is not header for TDC 0xC000 %0x",
		   ebch[0].serial,data);
            headerBAD = TRUE;
	    next = TRUE;
	  }
	  //if(pulser1)printf(" Pulser1 found in FBU1, event %d\n",yserial);
	}
      else
	printf("Error in ybk_find\n");
    }
  }
  return CM_SUCCESS;
}

/*--------------------------------------------------------------------*/
INT tr_prestart(INT rn, char *error)
{
  INT i, status;
  
  gbl_run = rn;
  printf("\n New Run %d\n", rn);

  /* Reset Destination statistics */
  memset((char *)&ebstat, 0, sizeof(EBUILDER_STATISTICS));
  db_set_record(hDB, hStatKey, &ebstat, sizeof(EBUILDER_STATISTICS), 0);
  stop_requested = stopped = FALSE;
  gbl_bytes_sent = 0;
  gbl_events_sent = 0;
 
  /* Reset local Source staistics */
  for (i=0 ; ; i++)
  {
    if (ebch[i].name[0] == 0)
      break;
    memset(&(ebch[i].stat), 0, sizeof(EBUILDER_STATISTICS));
  }
  
  /* Call user function */
  status = eb_begin_of_run(gbl_run, error);
  if (status != EB_SUCCESS) {
    cm_msg(MERROR, "eb_prestart"
	   , "run start aborted due to eb_begin_of_run (%d)", status);
    return status;
  }
  
  /* Book all fragment */
  printf(" nfrag : %d\n", i);
  status = source_booking(i);
  if (status != SUCCESS)
    return status;
  if(debug)printf("tr_prestart: after booking:%d\n", status);

  /* Mark run start time for local purpose */
  start_time = ss_millitime();

  /* local run state */
  run_state = STATE_RUNNING;
  
  /* Reset global trigger mask */
  /* If you want to test event mismatch condition, comment out
     the following statement. Start a run, shutdown one producer
     during this run. End the run. Restart the producer. 
     Start a new run. That makes an event mismatch */
  cdemask = 0;
  return CM_SUCCESS;
}

/*--------------------------------------------------------------------*/
INT tr_stop(INT rn, char *error)
{
  if(debug)printf("Entering tr_stop\n");
  /* local stop */
  stop_requested = TRUE;

  /* local stop time */
  request_stop_time = ss_millitime();
  return CM_SUCCESS;
}

/*--------------------------------------------------------------------*/
void free_event_buffer(INT nfrag)
{
  INT i;
  for (i=0; i<nfrag; i++) {
    if (ebch[i].pfragment) {
      free(ebch[i].pfragment);
      ebch[i].pfragment = NULL;
    }
  }
}

/*--------------------------------------------------------------------*/
INT source_booking(INT nfrag)
{
  INT j, i, size, status, status1, status2;
  char strout[256];
  
  if(debug) printf("Entering booking\n");

  /* Book all the source channels */
  for (i=0; i<nfrag ; i++)
  {
    /* Book only the requested event mask */
    if (ebch[i].set.emask)
    {
      /* Connect channel to source buffer */
      status1 = bm_open_buffer(ebch[i].set.buffer
			       , EVENT_BUFFER_SIZE
			       , &(ebch[i].hBuf));

      if (debug)
	printf("bm_open_buffer frag:%d handle:%d stat:%d\n",
	       i, ebch[i].hBuf, status1);
      /* Register for specified channel event ID and Trigger mask */
      status2 = bm_request_event(ebch[i].hBuf
				 , ebch[i].set.event_id
				 , ebch[i].set.trigger_mask
				 , GET_ALL, &ebch[i].req_id, NULL);
      if (debug)
	printf("bm_request_event frag:%d req_id:%d stat:%d\n",
	       i, ebch[i].req_id, status1);
      if (((status1 != BM_SUCCESS) && (status1 != BM_CREATED)) ||
	  ((status2 != BM_SUCCESS) && (status2 != BM_CREATED)))
      {
	cm_msg(MERROR, "source_booking"
	       , "Open buffer/event request failure [%d %d %d]",
	       i, status1, status2 );
	return BM_CONFLICT;
      }

      /* Empty source buffer */
      printf("bm_empty_buffer:%d\n", bm_empty_buffers());
      
      /* allocate local source event buffer */
      if (ebch[i].pfragment)
	free(ebch[i].pfragment);
      ebch[i].pfragment = (char *) malloc(max_event_size + sizeof(EVENT_HEADER));
      if (debug)
	printf("malloc pevent frag:%d pevent:%p\n", i, ebch[i].pfragment);
      if (ebch[i].pfragment == NULL)
      {
	free_event_buffer(nfrag);
	cm_msg(MERROR, "source_booking", "Can't allocate space for buffer");
	return BM_NO_MEMORY;
      }

      do {
	size = max_event_size;
	status = bm_receive_event(ebch[i].hBuf, ebch[i].pfragment, &size, ASYNC);
	sprintf(strout
	,"booking: hand flush bm_receive_event[%d] hndle:%d stat:%d  Last Ser:%d"
		, i, ebch[i].hBuf, status
		, ((EVENT_HEADER *) ebch[i].pfragment)->serial_number);
	if(debug) printf("%s\n", strout);
      } while (status == BM_SUCCESS);
    }
  }

  /* Force to have a clean buffer for start */
  status = bm_empty_buffers();
  if (debug)
  {
    printf("bm_empty_buffers stat:%d\n",status);
    printf("Dest: mask:%x\n", ebset.emask);
    for (j=0; ; j++)
    {
      if (ebch[j].name[0] == 0)
	break;
      
      printf("%d name:%s",j , ebch[j].name);
      printf(" buff:%s", ebch[j].set.buffer);
      printf(" mask:%x", ebch[j].set.emask);
      printf(" seri:%d", ebch[j].serial);
      printf(" hbuf:%d", ebch[j].hBuf);
      printf(" reqi:%d", ebch[j].req_id);
      printf(" opst:%d", status1);
      printf(" rest:%d", status2);
      printf(" evid:%d", ebch[j].set.event_id);
      printf(" mask:0x%x\n", ebch[j].set.trigger_mask);
    }
  }

  return SUCCESS;
}

/*--------------------------------------------------------------------*/
INT source_unbooking(nfrag)
{
  INT i, status, size;
  char strout[256];
  
  /* Skip unbooking if already done */
  if (ebch[0].pfragment == NULL)
    return EB_SUCCESS;
  
  /* unbook all source channels */
  for (i=nfrag-1; i>=0 ; i--)
//  for (i=0; i<nfrag ; i++)
  {
    bm_empty_buffers();
//    status = bm_flush_cache(ebch[i].hBuf, SYNC);
//    printf("bm_flush_cache[%d] st:%d\n", i, status);
    /* flush source fragments */
/*
    do {
      size = max_event_size;
      status = bm_receive_event(ebch[i].hBuf, ebch[i].pfragment, &size, ASYNC);
      sprintf(strout
	      ,"unbooking: hand flush bm_receive_event[%d] hndle:%d stat:%d  Last Ser:%d"
	      , i, ebch[i].hBuf, status
	      , ((EVENT_HEADER *) ebch[i].pfragment)->serial_number);
      if(debug) printf("%s\n", strout);
    } while (status == BM_SUCCESS);
*/    
//    cm_msg(MINFO,"EBuilder",strout);

    /* Remove event ID registration */
    status = bm_delete_request(ebch[i].req_id);
    if (debug)
      printf("unbook: bm_delete_req[%d] req_id:%d stat:%d\n", i, ebch[i].req_id, status);

    /* Close source buffer */
    status = bm_close_buffer(ebch[i].hBuf);
    if (debug)
      printf("unbook: bm_close_buffer[%d] hndle:%d stat:%d\n", i, ebch[i].hBuf, status);
    if (status != BM_SUCCESS)
    {
      cm_msg(MERROR, "source_unbooking", "Close buffer[%d] stat:", i, status);
      return status;
    }
  }
  
  /* release local event buffer memory */
  free_event_buffer(nfrag);

  return EB_SUCCESS;
}

/*--------------------------------------------------------------------*/
/* source_scan()
  Scan all the fragment source once per call.
  1) This will retrieve the full midas event not swapped (except the
  MIDAS_HEADER) for each fragment if possible. The fragment will
  be stored in the channel event pointer.
  2a) if after a full nfrag path some frag are still not cellected, it
  returns with the frag# missing for timeout check.
  2b) If ALL fragments are present it will check the midas serial#
  for a full match across all the fragments.
  3a) If the serial check fails it returns with "event mismatch"
  and will abort the event builder but not stop the run for now.
  3b) If the serial check is passed, it will call the user_build function
  where the destination event is going to be composed.

@memo Scan all defined source and build a event if all fragment
are present.
@param fmt Fragment format type 
@param nfragment number of fragment to collect
@param dest_hBuf  Destination buffer handle
@param event destination point for built event 
@return   EB_NO_MORE_EVENT, EB_COMPOSE_TIMEOUT
          if different then SUCCESS (bm_compose, rpc_sent error)
*/
INT source_scan(INT fmt, INT nfragment, HNDLE dest_hBuf, char * dest_event)
{
  static char bars[] = "|/-\\";
  static int  i_bar;
  static DWORD  serial;
  INT    i, j, status, size; 
  INT    act_size;
  BOOL   found, event_mismatch;
  
  /* Scan all channels at least once */
  for(i=0 ; i<nfragment ; i++) {
    /* Check if current channel needs to be received */
    if ((ebset.emask & ebch[i].set.emask) & ~cdemask) {
      /* Get fragment and store it in ebch[i].pfragment */
      size = max_event_size;
      status = bm_receive_event(ebch[i].hBuf, ebch[i].pfragment, &size, ASYNC);
      if (status == BM_SUCCESS) { /* event received */
	/* mask event */
	cdemask |= ebch[i].set.emask;
	/* Keep local serial */
	ebch[i].serial = ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;
	
	/* update local source statistics */
	ebch[i].stat.events_sent++;
	
	if (debug1) {
	  printf("SUCC: ch:%d ser:%d Dest_emask:%d cdemask:%x emask:%x sz:%d\n"
		 , i, ebch[i].serial
		 , ebset.emask, cdemask, ebch[i].set.emask, size);
	}
      }
      else if (status == BM_ASYNC_RETURN) { /* timeout */
	ebch[i].timeout++;
	if (debug1) {
	  printf("ASYNC: ch:%d ser:%d Dest_emask:%d cdemask:%x emask:%x sz:%d\n"
		 , i,  ebch[i].serial
		 , ebset.emask, cdemask, ebch[i].set.emask, size);
	}
      }
      else { /* Error */
	cm_msg(MERROR, "event_scan", "bm_receive_event error %d", status);
	return status;
      }
    } /* ~cdemask */
  }
  
  /* Check if all fragments have been received */
  if (cdemask == ebset.emask) { /* All fragment in */
    /* Check if serial matches */
    found = event_mismatch = FALSE;
    /* Mark first serial */
    for (j=0; j<nfragment; j++) {
      if (ebch[j].set.emask && !found) {
	serial = ebch[j].serial;
	found = TRUE;
      }
      else {
	if (ebch[j].set.emask && (serial != ebch[j].serial)) {
	  /* Event mismatch */
	  event_mismatch = TRUE;
	}
      }
    }
    
    /* Global event mismatch */
    if (event_mismatch) {
      printf("\nevent_mismatch ");
      for (j=0;j<nfragment; j++) {
	printf("Ser[%d]:%d ", j, ebch[j].serial);
      }
      printf("\n");
      cm_msg(MERROR,"EBuilder","event mismatch at event %d",ebch[0].serial);
      return EB_ERROR;
    }
    else {
      /* serial number match */
      /* wheel display */
      if (wheel && (serial % 1024)==0) {
	printf("%c\r", bars[i_bar++ % 4]);
	fflush(stdout);
      }
      
      /* Inform this is a NEW destination event building procedure */
      memset(dest_event, 0, sizeof(EVENT_HEADER));
      act_size = 0;
      
      /* Fill reserved header space of destination event with
	 final header information */
      bm_compose_event((EVENT_HEADER *) dest_event
		       , ebset.event_id, ebset.trigger_mask,
		       act_size, ebch[0].serial);

      /* Pass fragments to user for final check before assembly */
      status = eb_user(nfragment, ebch, (EVENT_HEADER *) dest_event
		       , (void *) ((EVENT_HEADER *)dest_event+1)
		       , &act_size);
      if (status != SS_SUCCESS)
	return EB_ERROR;

      /* Allow bypass of fragment assembly if user wants to do it on its own */
      if (!ebset.user_build) {
	for (j=0 ; j<nfragment ; j++) {
	  status = meb_fragment_add(dest_event, ebch[j].pfragment, &act_size);
	  if (status != EB_SUCCESS) {
	    cm_msg(MERROR,"source_scan","compose fragment:%d current size:%d"
		   , j, act_size);
	    return status;
	  }
	}
      } /* skip user_build */
      
      /* Send event and wait for completion */
      status = rpc_send_event(dest_hBuf, dest_event, 
			      act_size+sizeof(EVENT_HEADER), SYNC);
      if (status != BM_SUCCESS) {
	if (debug) 
	  printf("rpc_send_event returned error %d, event_size %d\n", 
		 status, act_size);
	cm_msg(MERROR,"EBuilder","rpc_send_event returned error %d",status);
	return status;
      }
      
      if(debug)
//	printf("rpc_send_event: status 0x%x; serial %d; size %d\n",
//	       status,serial,act_size+sizeof(EVENT_HEADER));
      
      /* Keep track of the total byte count */
      gbl_bytes_sent += act_size+sizeof(EVENT_HEADER);
      
      /* update destination event count */
      ebstat.events_sent++;
      gbl_events_sent++;
      
      /* Reset mask and timeouts */
      cdemask = 0;
    } /* serial match */
  } /* cdemask == ebset.emask */ 
  return status;
}

/*--------------------------------------------------------------------*/
int main(unsigned int argc,char **argv)
{
  char   host_name[HOST_NAME_LENGTH], expt_name[HOST_NAME_LENGTH];
  INT    size, status;
  DWORD  nfragment, fragn;
  char   *dest_event;
  DWORD  last_time, actual_millitime=0, previous_event_sent=0;
  DWORD  i, j;
  BOOL   flag = TRUE;
  INT    state, fmt;
  HNDLE  hBuf;
  EBUILDER(ebuilder_str);
  EBUILDER_CHANNEL(ebuilder_channel_str);
  char   strout[128];

  /* init structure */
  memset (&ebch[0], 0, sizeof(ebch));
  max_event_size = MAX_EVENT_SIZE;

  /* set default */
  cm_get_environment (host_name, expt_name);

  /* get parameters */
  for (i=1 ; i<argc ; i++)
  {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'v')
      wheel = TRUE;
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
    printf("usage: mevb [-h <Hostname>] [-e <Experiment>] [-d debug]\n");
    printf("             -v show wheel -D to start as a daemon\n\n");
    return 0;
    }
  }

  printf("Program mevb/EBuilder version 2 started\n\n");
  if (daemon)
  {
    printf("Becoming a daemon...\n");
    ss_daemon_init();
  }

 /* Connect to experiment */
  status = cm_connect_experiment(host_name, expt_name, "EBuilder", NULL);
  if (status != CM_SUCCESS)
    return 1;

  /* Connect to ODB */
  cm_get_experiment_database(&hDB, &hKey);

  /* Setup tree */
  if (db_find_key(hDB, 0, "EBuilder", &hEKey) != DB_SUCCESS)
    db_create_record(hDB, 0, "EBuilder", strcomb(ebuilder_str));
  db_find_key(hDB, 0, "EBuilder", &hEKey);
  
  /* EB setting handle */
  db_find_key(hDB, hEKey, "Settings", &hSetKey);
  size = sizeof(EBUILDER_SETTINGS);
  status = db_get_record(hDB, hSetKey, &ebset, &size, 0); 

  /* Get hostname for status page */
  gethostname(ebset.hostname, sizeof(ebset.hostname));
  size =  sizeof(ebset.hostname);
  db_set_value(hDB, hSetKey, "hostname", ebset.hostname, size, 1, TID_STRING);

  /* Get EB statistics */
  db_find_key(hDB, hEKey, "Statistics", &hStatKey);

  /* extract format */
  if (equal_ustring(ebset.format, "YBOS"))
    fmt = FORMAT_YBOS;
  else if (equal_ustring(ebset.format, "MIDAS"))
    fmt = FORMAT_MIDAS;
  else /* default format is MIDAS */
  {
    cm_msg(MERROR,"EBuilder", "Format not permitted");
    goto error;
  }

  /* Check for run condition */
  size = sizeof(state);
  db_get_value(hDB,0,"/Runinfo/state", &state, &size, TID_INT, TRUE);
  if (state != STATE_STOPPED)
  {
    cm_msg(MERROR,"EBuilder","Run must be stopped before starting EBuilder");
    goto error;
  }
    
  /* Scan EB Channels */
  if (db_find_key(hDB, hEKey, "Channels", &hChKey) != DB_SUCCESS)
  {
    db_create_record(hDB, hEKey, "Channels", strcomb(ebuilder_channel_str));
    db_find_key(hDB, hEKey, "Channels", &hChKey);
  }
  
  for (i=0, j=0, nfragment=0; i<MAX_CHANNELS ; i++)
  {
    db_enum_key(hDB, hChKey, i, &hSubkey);
    if (!hSubkey)
      break;
    db_get_key(hDB, hSubkey, &key);
    if (key.type == TID_KEY)
    {
      /* read channel record */
      sprintf(ebch[j].name, "%s", key.name);
      status = db_find_key(hDB, hSubkey, "Statistics", &(ebch[j].hStat));
      status = db_find_key(hDB, hSubkey, "Settings", &hKey);
      size = sizeof(EBUILDER_SETTINGS_CH);
      status = db_get_record(hDB, hKey, &(ebch[j].set), &size, 0);
      j++;
      nfragment++;
    }
  }

  /* Register transition for reset counters */
  if (cm_register_transition(TR_PRESTART, tr_prestart) != CM_SUCCESS)
    goto error;
  if (cm_register_transition(TR_STOP, tr_stop) != CM_SUCCESS)
    goto error;
  
  if (debug)
    cm_set_watchdog_params(TRUE, 0);

  /* Destination buffer */
  status = bm_open_buffer(ebset.buffer, EVENT_BUFFER_SIZE, &hBuf);
  if(debug)printf("bm_open_buffer dest returns %d\n",status);
  if (status != BM_SUCCESS && status != BM_CREATED) {
    printf("Error return from bm_open_buffer\n");
    goto error;
  }
    
  /* set the buffer write cache size */
  status = bm_set_cache_size(hBuf, 0, 200000);
  if(debug)printf("bm_set_cache_size dest returns %d\n",status);
  
  /* allocate destination event buffer */
  dest_event = (char *) malloc(nfragment*(max_event_size + sizeof(EVENT_HEADER)));
  memset(dest_event, 0, nfragment*(max_event_size + sizeof(EVENT_HEADER)));
  if (dest_event == NULL) {
    cm_msg(MERROR,"EBuilder","Not enough memory for event buffer\n");
    goto error;
  }
  
  /* Set fragment_add function based on the fornat */
  if (fmt = FORMAT_MIDAS)
    meb_fragment_add = eb_mfragment_add;
  else if (fmt = FORMAT_YBOS)
    meb_fragment_add = eb_yfragment_add;
  else {
    cm_msg(MERROR,"mevb","Unknown data format :%d", fmt);
    goto error;
  }
  
  /* Main event loop */
  do {
    if (run_state != STATE_RUNNING) {
      /* skip the source scan and yield */
      goto skip;
    }
    
    /* scan source buffer and send event to destination
       The source_scan() serves one event at the time.
       The status returns:
       EB_SUCCESS, BM_ASYNC_RETURN, EB_ERROR
       In the case of no fragment found(timeout), a watchdog would
       kick in for a fix amount of time. If timeout occur,
       the run state is checked and memory channels are freed
    */
    status = source_scan(fmt, nfragment, hBuf, dest_event);
    if (status == BM_ASYNC_RETURN) {
      for (fragn=0; fragn<nfragment ;fragn++) {
	if (ebch[fragn].timeout > TIMEOUT) {
	  /* Check for stop */
	  if (stop_requested) {
	    stopped = TRUE;
	    run_state = STATE_STOPPED;
	    
	    /* Flush local destination cache */
	    bm_flush_cache(hBuf, SYNC);

	    /* Call user function */
	    status = eb_end_of_run(gbl_run, strout);
	    if (status != EB_SUCCESS)
	      cm_msg(MERROR, "eb_stop"
		     , "eb_end_of_run returs:%d", status);

	    /* Detach all source from midas */
	    status = source_unbooking(nfragment);
	    
	    if(debug) printf("main: stop unbooking:%d\n", status);
	    
	    stop_time = ss_millitime() - request_stop_time;
	    sprintf(strout,"Run %d Stop on frag#%d; events_sent %1.0lf; npulser %d",
		    gbl_run, fragn, ebstat.events_sent,npulser1);
	    cm_msg(MINFO,"EBuilder","%s",strout);
	    printf("Time between request and actual stop: %d ms\n",stop_time);
	  }
	  else {
	  ebch[fragn].timeout = 0;
	  status = cm_yield(50);
	  }
	}
	else {
	  status = cm_yield(50);
	  if(debug3) printf("timeout frag#%d [%d]\n", fragn, ebch[fragn].timeout);
	}
      }
    }
    else if (status == EB_ERROR) {
      cm_msg(MTALK,"EBuilder","Event mismatch - Stop the run");
      cm_yield(1000);
      status = SS_ABORT;
    }
    else if (status == EB_SUCCESS) {
      for (i=0;i<nfragment;i++)
	ebch[i].timeout = 0;
    }
    else {
      cm_msg(MERROR, "Source_scan", "unexpected return %d", status);
      status = SS_ABORT;
    }
    
 skip: /* EB job done, update statistics */
    
    /* Check if it's time to do statistics job */
    if ((actual_millitime = ss_millitime()) - last_time > 1000) {
      /* Force event ot appear at the destination if Ebuilder is remote */
      rpc_flush_event();
      /* Force event ot appear at the destination if Ebuilder is local */
      bm_flush_cache(hBuf, ASYNC);
      
      /* update all source statistics */
      for (j=0 ; j<nfragment ; j++) {
	
	/* Compute statistics */
	if ((actual_millitime > start_time) && ebch[j].stat.events_sent) {
	  ebch[j].stat.events_per_sec_ = ebch[j].stat.events_sent
	    / ((actual_millitime-last_time)/1000.0);
	  
	  /* Update ODB channel statistics */
	  db_set_record(hDB, ebch[j].hStat
			, &(ebch[j].stat)
			, sizeof(EBUILDER_STATISTICS), 0);
	}
      }
      
      /* Compute destination statistics */
      if ((actual_millitime > start_time) && ebstat.events_sent) {
	ebstat.events_per_sec_ = gbl_events_sent
	  / ((actual_millitime-last_time)/1000.0) ;
	
	ebstat.kbytes_per_sec_ = gbl_bytes_sent
	  /1024.0/((actual_millitime-last_time)/1000.0);
	
	/* update destination statistics */
	db_set_record(hDB, hStatKey
		      , &ebstat
		      , sizeof(EBUILDER_STATISTICS), 0);
      }

      /* Keep track of last ODB update */
      last_time = ss_millitime();
      
      /* Reset local rate counters */
      gbl_events_sent = 0;
      gbl_bytes_sent = 0;
      
      /* Yield for system messages */
      status = cm_yield(10);
    }
  } while (status != RPC_SHUTDOWN && status != SS_ABORT);
  if (status == SS_ABORT)
    goto error;
  else
    goto exit;
  
 error: 
  cm_msg(MTALK,"EBuilder","Event builder error. Check messages"); 

 exit:
  /* Detach all source from midas */
  printf("exit: unbook\n");
  source_unbooking(nfragment);

  /* Free local memory */
//  if(!stopped) {
  free_event_buffer(nfragment);
//  } 
  
  /* Clean disconnect from midas */
  cm_disconnect_experiment();
  return 0;
}

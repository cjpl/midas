/********************************************************************\
Name:         mevb.c
Created by:   Pierre-Andre Amaudruz

Contents:     Main Event builder task.
$Log$
Revision 1.14  2004/09/29 17:55:34  pierre
fix speed problem

Revision 1.13  2004/09/29 16:20:31  pierre
change Ebuilder structure

Revision 1.12  2004/01/08 08:40:08  midas
Implemented standard indentation

Revision 1.11  2004/01/08 06:48:26  pierre
Doxygen the file

Revision 1.10  2003/08/19 23:26:36  pierre
fix cm_get_environment arg

Revision 1.9  2002/10/07 17:04:01  pierre
fix tr_stop request

Revision 1.8  2002/09/28 00:48:33  pierre
Add EB_USER_ERROR handling, handFlush()

Revision 1.7  2002/09/25 18:37:37  pierre
correct: header passing, user field, abort run

Revision 1.6  2002/08/29 22:07:47  pierre
fix event header, double task, EOR

Revision 1.5  2002/07/13 05:45:49  pierre
added swap before user function

Revision 1.4  2002/06/14 04:59:08  pierre
revised for ybos 

Revision 1.3  2002/05/08 20:51:41  midas
Added extra parameter to function db_get_value()

Revision 1.2  2002/01/17 23:34:14  pierre
doc++ format

Revision 1.1.1.1  2002/01/17 19:49:54  pierre
Initial Version

\********************************************************************/

/**dox***************************************************************/
/* @file mevb.c
The Event builder main file 
*/

#include <stdio.h>
#include "midas.h"
#include "mevb.h"
#include "msystem.h"
#include "ybos.h"

EBUILDER_SETTINGS ebset;
EBUILDER_STATISTICS ebstat;
EBUILDER_CHANNEL ebch[MAX_CHANNELS];

DWORD max_event_size = MAX_EVENT_SIZE;
char  ebname[32];
HNDLE hDB, hKey, hStatKey;
BOOL debug = FALSE, debug1 = FALSE;

BOOL abort_requested = FALSE, stop_requested = TRUE;
BOOL stopped = TRUE;
BOOL wheel = FALSE;
INT run_state = 0;
DWORD start_time = 0, stop_time = 0, request_stop_time = 0;
DWORD gbl_bytes_sent = 0, gbl_events_sent = 0;
DWORD cdemask = 0;
INT gbl_run = 0;

INT(*meb_fragment_add) (char *, char *, INT *);
INT handFlush(INT);
INT source_booking(INT nfrag);
INT eb_mfragment_add(char *pdest, char *psrce, INT * size);
INT eb_yfragment_add(char *pdest, char *psrce, INT * size);

INT eb_begin_of_run(INT, char *, char *);
INT eb_end_of_run(INT, char *);
INT eb_user(INT, EBUILDER_CHANNEL *, EVENT_HEADER *, void *, INT *);
INT my_get_record(HNDLE hDB, HNDLE hkey, EBUILDER_SETTINGS *rec);

extern INT ybos_event_swap(DWORD * pevt);

INT my_get_record(HNDLE hDB, HNDLE hkey, EBUILDER_SETTINGS *rec)
{
  INT size, status;
  size = sizeof(INT);
  status = db_get_value(hDB, hkey, "Number of Fragment", &rec->nfragment, &size, TID_INT, 0);
  size = sizeof(WORD);
  status = db_get_value(hDB, hkey, "Event ID", &rec->event_id, &size, TID_WORD, 0);
  size = sizeof(WORD);
  status = db_get_value(hDB, hkey, "Trigger Mask", &rec->trigger_mask, &size, TID_WORD, 0);
  size = sizeof(rec->buffer);
  status = db_get_value(hDB, hkey, "Buffer", rec->buffer, &size, TID_STRING, 0);
  size = sizeof(rec->format);
  status = db_get_value(hDB, hkey, "Format", rec->format, &size, TID_STRING, 0);
  size = sizeof(rec->user_build);
  status = db_get_value(hDB, hkey, "User build", &rec->user_build, &size, TID_BOOL, 0);
  size = sizeof(rec->user_field);
  status = db_get_value(hDB, hkey, "User Field", rec->user_field, &size, TID_STRING, 0);
  return status;
}


/********************************************************************/
INT eb_mfragment_add(char *pdest, char *psrce, INT * size)
{
   BANK_HEADER *psbh, *pdbh;
   char *psdata, *pddata;
   INT bksize;

   /* Condition for new EVENT the data_size should be ZERO */
   *size = ((EVENT_HEADER *) pdest)->data_size;

   /* destination pointer */
   pddata = pdest + *size + sizeof(EVENT_HEADER);

   if (*size) {
      /* NOT the first fragment */

      /* Swap event source if necessary */
      psbh = (BANK_HEADER *) (((EVENT_HEADER *) psrce) + 1);
      bk_swap(psbh, FALSE);

      /* source pointer */
      psbh = (BANK_HEADER *) (((EVENT_HEADER *) psrce) + 1);
      psdata = (char *) (psbh + 1);

      /* copy all banks without the bank header */
      bksize = psbh->data_size;

      /* copy */
      memcpy(pddata, psdata, bksize);

      /* update event size */
      ((EVENT_HEADER *) pdest)->data_size += bksize;

      /* update bank size */
      pdbh = (BANK_HEADER *) (((EVENT_HEADER *) pdest) + 1);
      pdbh->data_size += bksize;

      *size = ((EVENT_HEADER *) pdest)->data_size;
   } else {
      /* First event without the event header but with the 
         bank header as the size is zero */
      *size = ((EVENT_HEADER *) psrce)->data_size;

      /* Swap event if necessary */
      psbh = (BANK_HEADER *) (((EVENT_HEADER *) psrce) + 1);
      bk_swap(psbh, FALSE);

      /* copy first fragment */
      memcpy(pddata, psbh, *size);

      /* update destination event size */
      ((EVENT_HEADER *) pdest)->data_size = *size;
   }
   return CM_SUCCESS;
}

/*--------------------------------------------------------------------*/
INT eb_yfragment_add(char *pdest, char *psrce, INT * size)
{
   /* pdest : EVENT_HEADER pointer
      psrce : EVENT_HEADER pointer
      Keep pbkh for later incrementation
    */
   char *psdata, *pddata;
   DWORD *pslrl, *pdlrl;
   INT i4frgsize, i1frgsize, status;

   /* Condition for new EVENT the data_size should be ZERO */
   *size = ((EVENT_HEADER *) pdest)->data_size;

   /* destination pointer skip the header as it has been already
      composed and the usere may have modified it on purpose (Midas Control) */
   pddata = pdest + *size + sizeof(EVENT_HEADER);

   /* the Midas header is present for logger */
   if (*size) {                 /* already filled with a fragment */

      /* source pointer: number of DWORD (lrl included) */
      pslrl = (DWORD *) (((EVENT_HEADER *) psrce) + 1);

      /* Swap event if necessary */
      status = ybos_event_swap(pslrl);

      /* copy done in bytes, do not include LRL */
      psdata = (char *) (pslrl + 1);

      /* copy size in I*4 (lrl included, remove it) */
      i4frgsize = (*pslrl);
      i1frgsize = 4 * i4frgsize;

      /* append fragment */
      memcpy(pddata, psdata, i1frgsize);

      /* update Midas header event size */
      ((EVENT_HEADER *) pdest)->data_size += i1frgsize;

      /* update LRL size (I*4) */
      pdlrl = (DWORD *) (((EVENT_HEADER *) pdest) + 1);
      *pdlrl += i4frgsize;

      /* Return event size in bytes */
      *size = ((EVENT_HEADER *) pdest)->data_size;
   } else {                     /* new destination event */
      /* The composed event has already the MIDAS header.
         which may have been modified by the user in ebuser.c
         Will be stripped by the logger (YBOS).
         Copy the first full event ( no EVID suppression )
         First event (without the event header) */

      /* source pointer */
      pslrl = (DWORD *) (((EVENT_HEADER *) psrce) + 1);

      /* Swap event if necessary */
      status = ybos_event_swap(pslrl);

      /* size in byte from the source midas header */
      *size = ((EVENT_HEADER *) psrce)->data_size;

      /* copy first fragment */
      memcpy(pddata, (char *) pslrl, *size);

      /* update destination Midas header event size */
      ((EVENT_HEADER *) pdest)->data_size += *size;

   }
   return CM_SUCCESS;
}

/*--------------------------------------------------------------------*/
INT tr_prestart(INT rn, char *error)
{
   INT status, size, i;
   HNDLE  hkey;
   KEY    key;
   char   str[128];

   abort_requested = FALSE;
   gbl_run = rn;
   printf("%s-Starting New Run: %d\n", ebname, rn);

   /* Reset Destination statistics */
   memset((char *) &ebstat, 0, sizeof(EBUILDER_STATISTICS));
   db_set_record(hDB, hStatKey, &ebstat, sizeof(EBUILDER_STATISTICS), 0);
   gbl_bytes_sent = 0;
   gbl_events_sent = 0;

   /* Update the user_field */
   size = sizeof(ebset.user_field);
   sprintf(str,"%s/Settings/User Field", ebname);
   db_get_value(hDB, 0, str, ebset.user_field, &size,
                TID_STRING, FALSE);

   /* update from ODB field */
   size = sizeof(ebset.event_id);
   sprintf(str,"%s/Settings/Event ID", ebname);
   db_get_value(hDB, 0, str, &ebset.event_id, &size, TID_WORD, 0);

   size = sizeof(ebset.trigger_mask);
   sprintf(str,"%s/Settings/Trigger Mask", ebname);
   db_get_value(hDB, 0, str, &ebset.trigger_mask, &size, TID_WORD, 0);

   size = sizeof(ebset.buffer);
   sprintf(str,"%s/Settings/Buffer", ebname);
   db_get_value(hDB, 0, str, ebset.buffer, &size, TID_STRING, 0);

   /* Get the Request flags */
   sprintf(str,"%s/Settings/Requested Fragment", ebname);
   db_find_key(hDB, 0, str, &hkey); 
   db_get_key (hDB, hkey, &key);
   free (ebset.preqfrag);
   ebset.preqfrag = malloc(key.total_size);
   size = key.total_size;
   free (ebset.received);
   ebset.received = malloc(key.total_size);
   size = key.total_size;
   status = db_get_data(hDB, hkey, ebset.preqfrag, &size, TID_BOOL);
   for (i=0;i<ebset.nfragment;i++) 
     ebset.received[i] = FALSE;

   /* Call BOR user function */
   status = eb_begin_of_run(gbl_run, ebset.user_field, error);
   if (status != EB_SUCCESS) {
      cm_msg(MERROR, "eb_prestart", "run start aborted due to eb_begin_of_run (%d)",
             status);
      return status;
   }

   /* Book all fragment */
   status = source_booking(ebset.nfragment);
   if (status != SUCCESS)
      return status;

   /* Mark run start time for local purpose */
   start_time = ss_millitime();

   /* local run state */
   run_state = STATE_RUNNING;
   stopped = FALSE;
   stop_requested = FALSE;

   /* Reset global trigger mask */
   cdemask = 0;
   return CM_SUCCESS;
}

/*--------------------------------------------------------------------*/
INT tr_stop(INT rn, char *error)
{
   printf("\n%s-Stopping Run: %d detected\n", ebname, rn);

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
   for (i = 0; i < nfrag; i++) {
      if (ebch[i].pfragment) {
         free(ebch[i].pfragment);
         ebch[i].pfragment = NULL;
      }
   }
}

/*--------------------------------------------------------------------*/
INT handFlush(INT nfrag)
{
   int i, size, status;
   char strout[256];

   /* Do Hand flush until better way to  garantee the input buffer to be empty */
   if (debug)
      printf("Hand flushing system buffer... \n");
   for (i = 0; i < nfrag; i++) {
      do {
         size = max_event_size;
         status = bm_receive_event(ebch[i].hBuf, ebch[i].pfragment, &size, ASYNC);
         if (debug1) {
            sprintf(strout,
                    "booking:Hand flush bm_receive_event[%d] hndle:%d stat:%d  Last Ser:%d",
                    i, ebch[i].hBuf, status,
                    ((EVENT_HEADER *) ebch[i].pfragment)->serial_number);
            printf("%s\n", strout);
         }
      } while (status == BM_SUCCESS);
   }

   /* Empty source buffer */
   status = bm_empty_buffers();
   if (status != BM_SUCCESS)
      cm_msg(MERROR, "source_booking", "bm_empty_buffers failure [%d]", status);
   stopped = TRUE;
   run_state = STATE_STOPPED;
   return status;
}


/*--------------------------------------------------------------------*/
INT source_booking(INT nfrag)
{
   INT j, i, status, status1, status2;

   if (debug)
      printf("Entering booking\n");

   /* Book all the source channels */
   for (i = 0; i < nfrag; i++) {
      /* Book only the requested event mask */
     if (ebset.preqfrag[i]) {
         /* Connect channel to source buffer */
         status1 = bm_open_buffer(ebch[i].buffer, EVENT_BUFFER_SIZE, &(ebch[i].hBuf));

         if (debug)
           printf("bm_open_buffer frag:%d buf:%s handle:%d stat:%d\n",
                   i, ebch[i].buffer, ebch[i].hBuf, status1);
         /* Register for specified channel event ID and Trigger mask */
         status2 =
             bm_request_event(ebch[i].hBuf, ebch[i].event_id,
                              ebch[i].trigger_mask, GET_ALL, &ebch[i].req_id, NULL);
         if (debug)
           printf("bm_request_event frag:%d id:%d msk:%d req_id:%d stat:%d\n",
                   i, ebch[i].event_id, ebch[i].trigger_mask, ebch[i].req_id, status2);
         if (((status1 != BM_SUCCESS) && (status1 != BM_CREATED)) ||
             ((status2 != BM_SUCCESS) && (status2 != BM_CREATED))) {
            cm_msg(MERROR, "source_booking",
                   "Open buffer/event request failure [%d %d %d]", i, status1, status2);
            return BM_CONFLICT;
         }

         /* allocate local source event buffer */
         if (ebch[i].pfragment)
            free(ebch[i].pfragment);
         ebch[i].pfragment = (char *) malloc(max_event_size + sizeof(EVENT_HEADER));
         if (debug)
            printf("malloc pevent frag:%d pevent:%p\n", i, ebch[i].pfragment);
         if (ebch[i].pfragment == NULL) {
            free_event_buffer(nfrag);
            cm_msg(MERROR, "source_booking", "Can't allocate space for buffer");
            return BM_NO_MEMORY;
         }
      }
   }

   /* Empty source buffer */
   status = bm_empty_buffers();
   if (status != BM_SUCCESS) {
      cm_msg(MERROR, "source_booking", "bm_empty_buffers failure [%d]", status);
      return status;
   }

   if (debug) {
      printf("bm_empty_buffers stat:%d\n", status);
      for (j = 0; j < ebset.nfragment; j++) {
         printf(" buff:%s", ebch[j].buffer);
         printf(" ser#:%d", ebch[j].serial);
         printf(" hbuf:%2d", ebch[j].hBuf);
         printf(" rqid:%2d", ebch[j].req_id);
         printf(" opst:%d", status1);
         printf(" rqst:%d", status2);
         printf(" evid:%2d", ebch[j].event_id);
         printf(" tmsk:0x%4.4x\n", ebch[j].trigger_mask);
      }
   }

   return SUCCESS;
}

/*--------------------------------------------------------------------*/
INT source_unbooking(nfrag)
{
   INT i, status;

   /* Skip unbooking if already done */
   if (ebch[0].pfragment == NULL)
      return EB_SUCCESS;

   /* unbook all source channels */
   for (i = nfrag - 1; i >= 0; i--) {
      bm_empty_buffers();

      /* Remove event ID registration */
      status = bm_delete_request(ebch[i].req_id);
      if (debug)
         printf("unbook: bm_delete_req[%d] req_id:%d stat:%d\n", i, ebch[i].req_id,
                status);

      /* Close source buffer */
      status = bm_close_buffer(ebch[i].hBuf);
      if (debug)
         printf("unbook: bm_close_buffer[%d] hndle:%d stat:%d\n", i, ebch[i].hBuf,
                status);
      if (status != BM_SUCCESS) {
         cm_msg(MERROR, "source_unbooking", "Close buffer[%d] stat:", i, status);
         return status;
      }
   }

   /* release local event buffer memory */
   free_event_buffer(nfrag);

   return EB_SUCCESS;
}

/********************************************************************/
/**
Scan all the fragment source once per call.

-# This will retrieve the full midas event not swapped (except the
MIDAS_HEADER) for each fragment if possible. The fragment will
be stored in the channel event pointer.
-# if after a full nfrag path some frag are still not cellected, it
returns with the frag# missing for timeout check.
-# If ALL fragments are present it will check the midas serial#
for a full match across all the fragments.
-# If the serial check fails it returns with "event mismatch"
and will abort the event builder but not stop the run for now.
-# If the serial check is passed, it will call the user_build function
where the destination event is going to be composed.

@param fmt Fragment format type 
@param nfragment number of fragment to collect
@param dest_hBuf  Destination buffer handle
@param dest_event destination point for built event 
@return   EB_NO_MORE_EVENT, EB_COMPOSE_TIMEOUT
if different then SUCCESS (bm_compose, rpc_sent error)
*/
INT source_scan(INT fmt, INT nfrag, HNDLE dest_hBuf, char *dest_event)
{
   static char bars[] = "|/-\\";
   static int i_bar;
   static DWORD serial;
   DWORD *plrl;
   BOOL   complete;
   INT i, j, status, size;
   INT act_size;
   BOOL found, event_mismatch;
   BANK_HEADER *psbh;

   /* Scan all channels at least once */
   for (i = 0; i < nfrag; i++) {
      /* Check if current channel needs to be received */
     if (ebset.preqfrag[i] && !ebset.received[i]) {
         /* Get fragment and store it in ebch[i].pfragment */
         size = max_event_size;
         status = bm_receive_event(ebch[i].hBuf, ebch[i].pfragment, &size, ASYNC);
         switch (status) {
         case BM_SUCCESS:      /* event received */
            /* Mask event */
            ebset.received[i] = TRUE;
            /* Keep local serial */
            ebch[i].serial = ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;

            /* Swap event depending on data format */
            switch (fmt) {
            case FORMAT_YBOS:
               plrl = (DWORD *) (((EVENT_HEADER *) ebch[i].pfragment) + 1);
               ybos_event_swap(plrl);
               break;
            case FORMAT_MIDAS:
               psbh = (BANK_HEADER *) (((EVENT_HEADER *) ebch[i].pfragment) + 1);
               bk_swap(psbh, FALSE);
               break;
            }

            if (debug1) {
              printf("SUCC: ch:%d ser:%d rec:%d sz:%d\n", i,
                      ebch[i].serial, ebset.received[i], size);
            }
            break;
         case BM_ASYNC_RETURN: /* timeout */
            ebch[i].timeout++;
            if (debug1) {
              printf("ASYNC: ch:%d ser:%d rec:%d sz:%d\n", i,
                      ebch[i].serial, ebset.received[i], size);
            }
            break;
         default:              /* Error */
            cm_msg(MERROR, "event_scan", "bm_receive_event error %d", status);
            return status;
            break;
         }
      }                         /* ~cdemask => next channel */
   }

   /* Check if all fragments have been received */
   complete = FALSE;
   for (i = 0; i < nfrag;i++) {
     if (ebset.preqfrag[i] && !ebset.received[i])
       break;
   }
   if (i == nfrag) {
     complete = TRUE;
     /* Check if serial matches */
     found = event_mismatch = FALSE;
     /* Check Serial, mark first serial */
     for (i = 0; i < nfrag; i++) {
         if (ebset.preqfrag[i] && ebset.received[i] && !found) {
            serial = ebch[i].serial;
            found = TRUE;
         } else {
            if (ebset.preqfrag[i] && ebset.received[i] && (serial != ebch[i].serial)) {
               /* Event mismatch */
               event_mismatch = TRUE;
            }
         }
      }

      if (abort_requested) {
         return EB_SKIP;
      }

      /* Global event mismatch */
      if (event_mismatch) {
         char str[256];
         char strsub[128];
         strcpy(str, "event mismatch: ");
         for (j = 0; j < nfrag; j++) {
            sprintf(strsub, "Ser[%d]:%d ", j, ebch[j].serial);
            strcat(str, strsub);
         }
      } else {                  /* serial number match */

         /* wheel display */
         if (wheel && (serial % 1024) == 0) {
            printf("...%c ..Going on %1.0lf\r", bars[i_bar++ % 4], ebstat.events_sent);
            fflush(stdout);
         }

         /* Inform this is a NEW destination event building procedure */
         memset(dest_event, 0, sizeof(EVENT_HEADER));
         act_size = 0;

         /* Fill reserved header space of destination event with
            final header information */
         bm_compose_event((EVENT_HEADER *) dest_event, ebset.event_id, ebset.trigger_mask,
                          act_size, ebch[0].serial);

         /* Pass fragments to user for final check before assembly */
         status =
             eb_user(nfrag, ebch, (EVENT_HEADER *) dest_event,
                     (void *) ((EVENT_HEADER *) dest_event + 1), &act_size);
         if (status != SS_SUCCESS)
            return status;

         /* Allow bypass of fragment assembly if user wants to do it on its own */
         if (!ebset.user_build) {
            for (j = 0; j < nfrag; j++) {
               status = meb_fragment_add(dest_event, ebch[j].pfragment, &act_size);
               if (status != EB_SUCCESS) {
                  cm_msg(MERROR, "source_scan",
                         "compose fragment:%d current size:%d (%d)", j, act_size, status);
                  return EB_ERROR;
               }
            }
         }

         /* skip user_build */
         /* Overall event to be sent */
         act_size = ((EVENT_HEADER *) dest_event)->data_size + sizeof(EVENT_HEADER);

         /* Send event and wait for completion */
         status = rpc_send_event(dest_hBuf, dest_event, act_size, SYNC);
         if (status != BM_SUCCESS) {
            if (debug)
               printf("rpc_send_event returned error %d, event_size %d\n",
                      status, act_size);
            cm_msg(MERROR, "EBuilder", "%s: rpc_send_event returned error %d", ebname, status);
            return EB_ERROR;
         }

         /* Keep track of the total byte count */
         gbl_bytes_sent += act_size;

         /* update destination event count */
         ebstat.events_sent++;
         gbl_events_sent++;

         /* Reset mask and timeouts */
         for (i = 0; i < nfrag; i++) {
            ebch[i].timeout = 0;
            ebset.received[i] = FALSE;
         }
      }    /* serial match */
      return EB_SUCCESS;
   }
   return status;
}

/*--------------------------------------------------------------------*/
int main(unsigned int argc, char **argv)
{
  static char bars[] = "|\\-/";
  static int i_bar;
  char host_name[HOST_NAME_LENGTH], expt_name[HOST_NAME_LENGTH];
  BOOL   changed=FALSE;
  INT size, status;
  DWORD nfragment, fragn;
  char *dest_event, buffer_name[32];
  DWORD last_time = 0, actual_millitime = 0, previous_event_sent = 0;
  DWORD i;
  BOOL daemon = FALSE, flag = TRUE, neweb = FALSE;
  INT state, fmt, type, ev_id=0, odb_ev_id=0;
  HNDLE hBuf, hSubkey, hEqKey, hEKey, hESetKey;
  EBUILDER(ebuilder_str);
  char strout[128];
  KEY key;
  /* init structure */
  memset(&ebch[0], 0, sizeof(ebch));

  /* set default */
  cm_get_environment(host_name, sizeof(host_name), expt_name, sizeof(expt_name));

  /* get parameters */
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'w')
      wheel = TRUE;
    else if (argv[i][0] == '-') {
      if (i + 1 >= argc || argv[i + 1][0] == '-')
        goto usage;
      if (strncmp(argv[i], "-e", 2) == 0)
        strcpy(expt_name, argv[++i]);
      else if (strncmp(argv[i], "-h", 2) == 0)
        strcpy(host_name, argv[++i]);
      else if (strncmp(argv[i], "-b", 2) == 0)
        strcpy(buffer_name, argv[++i]);
      else if (strncmp(argv[i], "-i", 2) == 0)
        ev_id = atoi(argv[++i]);
    } else {
usage:
      printf("usage: mevb [-h <Hostname>] [-e <Experiment>] -b <buffername> [-d debug]\n");
      printf("             -i event_id -w show wheel -D to start as a daemon\n\n");
      return 0;
    }
  }

  printf("Program mevb version 4 started\n\n");
  if (daemon) {
    printf("Becoming a daemon...\n");
    ss_daemon_init(FALSE);
  }
  
  /* Check flag Event ID */
  if (ev_id != 0)
    sprintf(ebname, "EBuilder%02d", ev_id);

  if (debug)
    cm_set_watchdog_params(TRUE, 0);

  /* Connect to experiment */
  status = cm_connect_experiment(host_name, expt_name, ebname, NULL);
  if (status != CM_SUCCESS)
    return 1;

  /* check if Ebuilder is already running */
  status = cm_exist(ebname, TRUE);
  if (status == CM_SUCCESS) {
    cm_msg(MERROR, "Ebuilder", "%s running already!.", ebname);
    cm_disconnect_experiment();
    return 1;
  }

  /* Connect to ODB */
  cm_get_experiment_database(&hDB, &hKey);

  /* Setup default tree or find existing one */
  if (db_find_key(hDB, 0, ebname, &hEKey) != DB_SUCCESS) {
    db_create_record(hDB, 0, ebname, strcomb(ebuilder_str));
    neweb = TRUE;
  }

  /* Keep Key on Ebuilder */
  db_find_key(hDB, 0, ebname, &hEKey);

  /* Get copy of current ODB EB setting handle */
  db_find_key(hDB, hEKey, "Settings", &hESetKey);
  size = sizeof(EBUILDER_SETTINGS);
  status = my_get_record(hDB, hESetKey, &ebset);

  /* Get hostname for status page */
  gethostname(ebset.hostname, sizeof(ebset.hostname));
  size = sizeof(ebset.hostname);
  db_set_value(hDB, hESetKey, "hostname", ebset.hostname, size, 1, TID_STRING);

  /* Get EB statistics */
  db_find_key(hDB, hEKey, "Statistics", &hStatKey);

  /* Check for run condition */
  size = sizeof(state);
  db_get_value(hDB, 0, "/Runinfo/state", &state, &size, TID_INT, TRUE);
  if (state != STATE_STOPPED) {
    cm_msg(MTALK, "EBuilder", "Run must be stopped before starting %s", ebname);
    goto error1;
  }

  /* Scan Equipment/Common listing */
  if (db_find_key(hDB, 0, "Equipment", &hEqKey) != DB_SUCCESS) {
    cm_msg(MINFO, "mevb", "Equipment listing not found");
    goto error1;
  }
  for (i = 0, nfragment=0 ; ; i++) {
    db_enum_key(hDB, hEqKey, i, &hSubkey);
    if (!hSubkey)
      break;
    db_get_key(hDB, hSubkey, &key);
    if (key.type == TID_KEY) {
      /* Equipment name */
      printf("Equipment name:%s\n", key.name);
      /* Check if equipment is EQ_EB */
      size = sizeof(INT);
      db_get_value(hDB, hSubkey, "common/type", &type, &size, TID_INT, 0);
      size = sizeof(WORD);
      db_get_value(hDB, hSubkey, "common/Event ID", &odb_ev_id, &size, TID_WORD, 0);
      if ((type & EQ_EB) && (ev_id == odb_ev_id)) {
        ebch[nfragment].type = type;
        ebch[nfragment].event_id = odb_ev_id;
        size = sizeof(WORD);
        db_get_value(hDB, hSubkey, "common/Trigger Mask", &ebch[nfragment].trigger_mask, &size, TID_WORD, 0);
        size = sizeof(ebch[nfragment].buffer);
        db_get_value(hDB, hSubkey, "common/Buffer", ebch[nfragment].buffer, &size, TID_STRING, 0);
        size = sizeof(ebch[nfragment].format);
        db_get_value(hDB, hSubkey, "common/Format", ebch[nfragment].format, &size, TID_STRING, 0);
        nfragment++;
      }
    }
  }
  
  /* Got all Equipment info for the Event Builder
     Do some checks on the content */
  if (ebset.nfragment != nfragment) {
    cm_msg(MINFO, "mevb", "Number of Fragment mismatch ODB:%d/%d", ebset.nfragment, nfragment);
    changed = TRUE;
  }
  for (i=0;i<nfragment;i++) {
    if ((ebset.event_id != ebch[i].event_id) && neweb) {
      cm_msg(MINFO, "mevb", "Event ID mismatch on Fragment:%s", ebch[i].buffer);
      ebset.event_id = ebch[i].event_id;
      changed = TRUE;
    }
    if ((ebset.trigger_mask != ebch[i].trigger_mask) && neweb) {
      cm_msg(MINFO, "mevb", "Trigger Mask mismatch on Fragment:%s", ebch[i].buffer);
      ebset.trigger_mask = ebch[i].trigger_mask;
      changed = TRUE;
    }
    if (strcmp(ebset.format, ebch[i].format) != 0) {
      cm_msg(MINFO, "mevb", "Format mismatch on Fragment:%s", ebch[i].buffer);
      changed = TRUE;
    }
  }
  if (changed) {
    /* Create the ReqFrag array and put it in ODB */
    ebset.nfragment = nfragment;
    ebset.preqfrag = malloc (sizeof(BOOL) * ebset.nfragment);
    for (i=0;i < nfragment;i++)
      ebset.preqfrag[i] = 1;
    status = db_set_value(hDB, hESetKey, "Requested Fragment", ebset.preqfrag , sizeof(BOOL) * nfragment, nfragment, TID_BOOL);
    status = db_set_value(hDB, hESetKey, "Number of Fragment", &ebset.nfragment, sizeof(INT), 1, TID_INT);
    status = db_set_value(hDB, hESetKey, "Trigger Mask", &ebset.trigger_mask, sizeof(WORD), 1, TID_WORD);
    status = db_set_value(hDB, hESetKey, "Event ID", &ebset.event_id, sizeof(WORD), 1, TID_WORD);
    sprintf(ebset.buffer, "SYSTEM");
    status = db_set_value(hDB, hESetKey, "Buffer", ebset.buffer, sizeof(ebset.buffer), 1, TID_STRING);
    cm_msg(MINFO, "mevb", "Overwriting /%s/Settings from Equipment scan for ev_id:%d", ebname, ebset.event_id);
  }

  /* extract format */
  if (equal_ustring(ebch[0].format, "YBOS"))
     fmt = FORMAT_YBOS;
  else if (equal_ustring(ebch[0].format, "MIDAS"))
     fmt = FORMAT_MIDAS;
  else {                       /* default format is MIDAS */

    cm_msg(MERROR, "EBuilder", "%s: Format not permitted", ebname);
     goto error;
  }

  /* Register transition for reset counters */
  if (cm_register_transition(TR_PRESTART, tr_prestart) != CM_SUCCESS)
    goto error;
  if (cm_register_transition(TR_STOP, tr_stop) != CM_SUCCESS)
    goto error;

  /* Destination buffer */
  status = bm_open_buffer(ebset.buffer, EVENT_BUFFER_SIZE, &hBuf);
  if (debug)
    printf("bm_open_buffer dest returns %d\n", status);
  if (status != BM_SUCCESS && status != BM_CREATED) {
    printf("Error return from bm_open_buffer\n");
    goto error;
  }

  /* set the buffer write cache size */
  status = bm_set_cache_size(hBuf, 0, 100000);
  if (debug)
    printf("bm_set_cache_size dest returns %d\n", status);

  /* allocate destination event buffer */
  dest_event = (char *) malloc(nfragment * (max_event_size + sizeof(EVENT_HEADER)));
  memset(dest_event, 0, nfragment * (max_event_size + sizeof(EVENT_HEADER)));
  if (dest_event == NULL) {
    cm_msg(MERROR, "EBuilder", "%s: Not enough memory for event buffer", ebname);
    goto error;
  }

  /* Set fragment_add function based on the format */
  if (fmt == FORMAT_MIDAS)
    meb_fragment_add = eb_mfragment_add;
  else if (fmt == FORMAT_YBOS)
    meb_fragment_add = eb_yfragment_add;
  else {
    cm_msg(MERROR, "mevb", "Unknown data format :%d", fmt);
    goto error;
  }

  /* Main event loop */
  do {
    if (run_state != STATE_RUNNING) {
      /* skip the source scan and yield */
      status = cm_yield(500);
      if (wheel) {
        printf("...%c Snoring on %1.0lf\r", bars[i_bar++ % 4], ebstat.events_sent);
        fflush(stdout);
      }
      continue;
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
    switch (status) {
      case BM_ASYNC_RETURN:
        // No event found for now:
        // Check for timeout 
        for (fragn = 0; fragn < nfragment; fragn++) {
          if (ebch[fragn].timeout > TIMEOUT) {        /* Timeout */
            if (stop_requested) {    /* Stop */
              if (debug)
                printf("Stop requested on timeout %d\n", status);

              /* Flush local destination cache */
              bm_flush_cache(hBuf, SYNC);

              /* Call user function */
              eb_end_of_run(gbl_run, strout);

              /* Cleanup buffers */
              handFlush(nfragment);

              /* Detach all source from midas */
              source_unbooking(nfragment);

              /* Compose message */
              stop_time = ss_millitime() - request_stop_time;
              sprintf(strout, "Run %d Stop on frag#%d; events_sent %1.0lf DT:%d[ms]",
                gbl_run, fragn, ebstat.events_sent, stop_time);

              /* Send message */
              cm_msg(MINFO, "EBuilder", "%s", strout);

              run_state = STATE_STOPPED;
              abort_requested = FALSE;
              break;
            } else {         /* No stop requested */
              ebch[fragn].timeout = 0;
              status = cm_yield(10);
              if (wheel) {
                printf("...%c Timoing on %1.0lf\r", bars[i_bar++ % 4],
                  ebstat.events_sent);
                fflush(stdout);
              }
            }
          }
          //else { /* No timeout */
          //  status = cm_yield(50);
          //}
        }                      /* do loop */
        break;
      case EB_ERROR:
      case EB_USER_ERROR:
        abort_requested = TRUE;
        if (status == EB_USER_ERROR)
          cm_msg(MTALK, "EBuilder", "%s: Error signaled by user code - stopping run...", ebname);
        else
          cm_msg(MTALK, "EBuilder", "%s: Event mismatch - Stopping run...", ebname);
        cdemask = 0;
        if (cm_transition(TR_STOP, 0, NULL, 0, ASYNC, 0) != CM_SUCCESS) {
          cm_msg(MERROR, "EBuilder", "%s: Stop Transition request failed", ebname);
          goto error;
        }
        break;
      case EB_SUCCESS:
      case EB_SKIP:
        //   Normal path if event has been assembled
        //   No yield in this case.
        break;
      default:
        cm_msg(MERROR, "Source_scan", "unexpected return %d", status);
        status = SS_ABORT;
    }

    /* EB job done, update statistics if its time */

    /* Check if it's time to do statistics job */
    if ((actual_millitime = ss_millitime()) - last_time > 1000) {
      /* Force event to appear at the destination if Ebuilder is remote */
      rpc_flush_event();
      /* Force event ot appear at the destination if Ebuilder is local */
      bm_flush_cache(hBuf, ASYNC);

      /* Compute destination statistics */
      if ((actual_millitime > start_time) && ebstat.events_sent) {
        ebstat.events_per_sec_ = gbl_events_sent
          / ((actual_millitime - last_time) / 1000.0);

        ebstat.kbytes_per_sec_ = gbl_bytes_sent
          / 1024.0 / ((actual_millitime - last_time) / 1000.0);

        /* update destination statistics */
        db_set_record(hDB, hStatKey, &ebstat, sizeof(EBUILDER_STATISTICS), 0);
      }


      /* Keep track of last ODB update */
      last_time = ss_millitime();

      /* Reset local rate counters */
      gbl_events_sent = 0;
      gbl_bytes_sent = 0;
    }

    /* Yield for system messages */
    status = cm_yield(0);
    if (wheel && (run_state != STATE_RUNNING)) {
      printf("...%c Idleing on %1.0lf\r", bars[i_bar++ % 4], ebstat.events_sent);
      fflush(stdout);
    }
  } while (status != RPC_SHUTDOWN && status != SS_ABORT);
if (status == SS_ABORT)
goto error;
else
goto exit;

error:
cm_msg(MTALK, "EBuilder", "%s: Event builder error. Check messages", ebname);

exit:
/* Detach all source from midas */
printf("%s-Unbooking\n", ebname);
source_unbooking(nfragment);

/* Free local memory */
free_event_buffer(nfragment);
error1:
/* Clean disconnect from midas */
cm_disconnect_experiment();
return 0;
  }

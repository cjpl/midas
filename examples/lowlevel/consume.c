/********************************************************************\

  Name:         consume.c
  Created by:   Stefan Ritt

  Contents:     Buffer manager test program. Simple consumer connec-
                ting to a SYSTEM buffer and receiving some data.

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "midas.h"

INT all_flag;
INT hBufEvent;
INT event_byte_count;
INT count_mismatches = 0;
char event_buffer[MAX_EVENT_SIZE];

/*----- Print any system message -----------------------------------*/

void process_message(HNDLE hBuf, HNDLE id, EVENT_HEADER * pheader, void *message)
{
   /* just print message text which comes after event header */
   printf("%s\n", (char *) message);
}

/*----- Print any run transition -----------------------------------*/

INT transition(INT run_number, char *error)
{
   printf("Transition, run #%d\n", run_number);
   return CM_SUCCESS;
}

/*----- process_event ----------------------------------------------*/

void process_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER * pheader, void *pevent)
{
   static int ser[10], jam = 0;

   int size, *pdata, id;

   /* accumulate received event size */
   size = pheader->data_size;
   id = pheader->event_id;
   if (id > 9)
      id = 9;
   event_byte_count += size + sizeof(EVENT_HEADER);

   /* check if first and last word inside event is equal
      to size to check that nothing was overwritten... */

   if (!jam) {
      /* only test once */
      pdata = (INT *) (pheader + 1);
      if (pdata[0] != size || pdata[size / 4 - 1] != size)
         cm_msg(MERROR, "process_event", "--> data jam <--");
      jam = 1;
   }

   /* if only some events are requested, sleep a little bit
      to simulate a random event consumer */
   if (!all_flag)
      ss_sleep(10);

   /* if all events are requested, now check the serial number
      if no events are missing */
   if (all_flag && (INT) pheader->serial_number != ser[id] + 1) {
      cm_msg(MERROR, "process_event",
             "Serial number mismatch: Ser: %d, OldSer: %d, ID: %d, size: %d",
             pheader->serial_number, ser[id], pheader->event_id, pheader->data_size);
      count_mismatches ++;
   }

   ser[id] = pheader->serial_number;
}

/*------------------------------------------------------------------*/

int main()
{
   INT start_time, stop_time;
   double rate;
   BUFFER_HEADER buffer_header;
   INT status, size, trans, run_number;
   char host_name[256], str[32];
   INT event_id, request_id;
   DWORD last_time;
   BOOL via_callback;

   /* get parameters */

   printf("ID of event to request: ");
   ss_gets(str, 32);
   event_id = atoi(str);

   printf("Host to connect: ");
   ss_gets(host_name, 256);

   printf("Get all events (0/1): ");
   ss_gets(str, 32);
   all_flag = atoi(str);

   printf("Receive via callback ([y]/n): ");
   ss_gets(str, 32);
   via_callback = str[0] != 'n';

   /* connect to experiment */
   status = cm_connect_experiment(host_name, "",
                                  all_flag ? "Power Consumer" : "Consumer", NULL);
   if (status != CM_SUCCESS)
      return 1;

   /* open the "system" buffer, 1M size */
   bm_open_buffer("SYSTEM", 2*MAX_EVENT_SIZE, &hBufEvent);

   /* set the buffer cache size */
   bm_set_cache_size(hBufEvent, 100000, 0);

   /* place a request for a specific event id */
   bm_request_event(hBufEvent, (WORD) event_id, TRIGGER_ALL,
                    all_flag ? GET_ALL : GET_NONBLOCKING, &request_id,
                    via_callback ? process_event : NULL);

   /* place a request for system messages */
   cm_msg_register(process_message);

   /* place a request for transition notification */
   cm_register_transition(TR_START, via_callback ? transition : NULL, 500);

   last_time = 0;
   start_time = 0;
   event_byte_count = 0;

   do {
      if (via_callback)
         status = cm_yield(1000);
      else {
         /* receive event "manually" and call receive_event */
         size = sizeof(event_buffer);
         status = bm_receive_event(hBufEvent, event_buffer, &size, BM_NO_WAIT);
         if (status == BM_SUCCESS)
            process_event(hBufEvent, request_id, (EVENT_HEADER *) event_buffer,
                          (void *) (((EVENT_HEADER *) event_buffer) + 1));

         /* receive transitions "manually" */
         if (cm_query_transition(&trans, &run_number, NULL))
            transition(run_number, NULL);

         /* call yield once every 100 ms */
         if (ss_millitime() - last_time > 100) {
            last_time = ss_millitime();
            status = cm_yield(0);
         }
      }

      /* calculate rates each second */
      if (ss_millitime() - start_time > 1000) {
         stop_time = ss_millitime();
         rate = event_byte_count / 1024.0 / 1024.0 / ((stop_time - start_time) / 1000.0);

         /* get information about filling level of the buffer */
         bm_get_buffer_info(hBufEvent, &buffer_header);
         size = buffer_header.read_pointer - buffer_header.write_pointer;
         if (size <= 0)
            size += buffer_header.size;
         printf("Level: %5.1lf %%, ", 100 - 100.0 * size / buffer_header.size);

         printf("Rate: %1.2lf MB/sec, ser mismatches: %d\n", rate, count_mismatches);
         start_time = stop_time;
         event_byte_count = 0;
      }

   } while (status != RPC_SHUTDOWN && status != SS_ABORT);

   cm_disconnect_experiment();

   return 1;
}

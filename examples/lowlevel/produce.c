/********************************************************************\

  Name:         produce.c
  Created by:   Stefan Ritt

  Contents:     Buffer manager test program. Simple producer connec-
                ting to a SYSTEM buffer and sending some data.

  $Id:$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "midas.h"

/*------------------------------------------------------------------*/

#ifdef OS_VXWORKS
produce()
#else
main()
#endif
{
   INT hBuf, status, i;
   char *event, str[256];
   INT *pdata, count;
   INT start, stop;
   double rate;
   int id, size, act_size, variable_size, flush = 0;
   char host_name[256];

   /* get parameters */

   printf("ID of event to produce: ");
   ss_gets(str, 256);
   id = atoi(str);

   printf("Host to connect: ");
   ss_gets(host_name, 256);

   printf("Event size: ");
   ss_gets(str, 256);
   size = atoi(str);
   if (size < 0) {
      variable_size = 1;
      size = -size;
   } else
      variable_size = 0;

   /* connect to experiment */
   status = cm_connect_experiment(host_name, "", "Producer", NULL);
   if (status != CM_SUCCESS)
      return 1;

   /* open the event buffer with default size */
   bm_open_buffer(EVENT_BUFFER_NAME, EVENT_BUFFER_SIZE, &hBuf);

   /* set the buffer write cache size */
   bm_set_cache_size(hBuf, 0, 100000);

   /* allocate event buffer */
   event = (char *) malloc(size + sizeof(EVENT_HEADER));
   memset(event, 0, size + sizeof(EVENT_HEADER));
   if (event == NULL) {
      printf("Not enough memory for event buffer\n");
      goto error;
   }
   pdata = (INT *) (event + sizeof(EVENT_HEADER));

   do {
      start = ss_millitime();
      count = 0;

      do {
         for (i = 0; i < 10; i++) {
            if (variable_size)
               act_size = (rand() % (size - 10)) + 10;
            else
               act_size = size;

            /* place the event size in the first and last word of
               the event to check later if data has been overwritten
               accidentally */
            pdata[0] = act_size;
            pdata[act_size / 4 - 1] = act_size;

            /* compose an event header with serial number */
            bm_compose_event((EVENT_HEADER *) event, (short) id, 1,
                             act_size, ((EVENT_HEADER *) (event))->serial_number + 1);

            if (act_size < 0)
               printf("Error: act_size = %d, size = %d\n", act_size, size);

            /* now send event */
            status = rpc_send_event(hBuf, event, act_size + sizeof(EVENT_HEADER), SYNC);
            /* status = bm_send_event(hBuf, event, 
               act_size+sizeof(EVENT_HEADER), SYNC); */

            if (status != BM_SUCCESS) {
               printf("rpc_send_event returned error %d, event_size %d\n",
                      status, act_size);
               goto error;
            }

/*
        printf(".");
        getchar();
*/

            count += act_size;
         }

         /* repeat this loop for 1s */
      } while (ss_millitime() - start < 1000);

      /* Now calculate the rate */
      stop = ss_millitime();
      if (stop != start)
         rate = count / 1024.0 / 1024.0 / (stop / 1000.0 - start / 1000.0);
      else
         rate = 0;

      printf("Rate: %1.2lf MB/sec\n", rate);

      /* flush buffers every 10 seconds */
      if ((flush++) % 10 == 0) {
         rpc_flush_event();
         bm_flush_cache(hBuf, SYNC);
         printf("flush\n");
      }

      status = cm_yield(0);

   } while (status != RPC_SHUTDOWN && status != SS_ABORT);

 error:

   cm_disconnect_experiment();
   return 1;
}

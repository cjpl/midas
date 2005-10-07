/********************************************************************\

  Name:         miniana.c
  Created by:   Stefan Ritt

  Contents:     Minimum analyzer program to receive an event

  $Id:$

\********************************************************************/

#include <stdio.h>
#include "midas.h"


void process_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER * pheader, void *pevent)
{
   printf("Received event #%d\r", pheader->serial_number);
}

main()
{
   INT status, request_id;
   HNDLE hBufEvent;

   status = cm_connect_experiment("", "sample", "Simple Analyzer", NULL);
   if (status != CM_SUCCESS)
      return 1;

   bm_open_buffer("SYSTEM", EVENT_BUFFER_SIZE, &hBufEvent);
   bm_request_event(hBufEvent, 1, TRIGGER_ALL, GET_ALL, &request_id, process_event);

   do {
      status = cm_yield(1000);
   } while (status != RPC_SHUTDOWN && status != SS_ABORT);

   cm_disconnect_experiment();

   return 0;
}

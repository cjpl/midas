/********************************************************************\

  Name:         produce.c
  Created by:   Stefan Ritt

  Contents:     Buffer manager test program. Simple producer connec-
                ting to a SYSTEM buffer and sending some data.

  $Log$
  Revision 1.3  1999/04/30 13:19:53  midas
  Changed inter-process communication (ss_resume, bm_notify_clients, etc)
  to strings so that server process can receive it's own watchdog produced
  messages (pass buffer name insteas buffer handle)

  Revision 1.2  1998/10/12 12:18:59  midas
  Added Log tag in header


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
INT           hBuf, status, i;
char          *event, str[256];
INT           *pdata, count;
INT           start, stop;
double        rate;
int           id, size;
char          host_name[256];

  /* get parameters */

  printf("ID of event to produce: ");
  ss_gets(str, 256);
  id = atoi(str);

  printf("Host to connect: ");
  ss_gets(host_name, 256);

  printf("Event size: ");
  ss_gets(str, 256);
  size = atoi(str);

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
  if (event == NULL)
    {
    printf("Not enough memory for event buffer\n");
    goto error;
    }
  pdata = (INT *) (event + sizeof(EVENT_HEADER));

  do
    {
    start = ss_millitime();
    count = 0;

    do
      {
      for (i=0 ; i<10 ; i++)
        {
        /* place the event size in the first and last word of
           the event to check later if data has been overwritten
           accidentally */
        pdata[0] = size;
        pdata[size/4-1] = size;

        /* compose an event header with serial number */
        bm_compose_event((EVENT_HEADER *) event, (short) id, 1,
                         size, ((EVENT_HEADER*) (event))->serial_number+1);

        /* now send event */
   	    status = rpc_send_event(hBuf, event, size+sizeof(EVENT_HEADER), SYNC);
//   	    status = bm_send_event(hBuf, event, size+sizeof(EVENT_HEADER), SYNC);

        if (status != BM_SUCCESS)
          {
          printf("rpc_send_event returns %d\n", status);
          printf("Error sending event");
	        goto error;
          }

/*
        printf(".");
        getchar();
*/
        
        count += size;
        }

      /* repeat this loop for 1s */
      } while (ss_millitime() - start < 1000);

    /* Now calculate the rate */
    stop = ss_millitime();
    if (stop != start)
      rate =  count / 1024.0 / 1024.0 / (stop/1000.0 - start/1000.0);
    else
      rate = 0;

    printf("Rate: %1.2lf MB/sec\n", rate);
    } while (1);

error:

  cm_disconnect_experiment();
  return 1;
}

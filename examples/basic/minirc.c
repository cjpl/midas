/********************************************************************\

  Name:         minirc.c
  Created by:   Stefan Ritt

  Contents:     A "Mini-Run Control" program showing the basic concept
                of starting/stopping runs in the MIDAS system

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "midas.h"

/*------------------------------------------------------------------*/

int main()
{
   INT run_number, status;
   char host_name[256];
   char str[256];

   printf("Host to connect: ");
   ss_gets(host_name, 256);

   /* connect to experiment */
   status = cm_connect_experiment(host_name, "", "MiniRC", NULL);
   if (status != CM_SUCCESS)
      return 1;

   printf("Enter run number: ");
   ss_gets(str, 256);
   run_number = atoi(str);

   printf("Start run\n");

   /* start run */
   if (cm_transition(TR_START, run_number, str, sizeof(str), SYNC, MT_INFO) != CM_SUCCESS)
      printf(str);

   printf("Hit RETURN to stop run");
   getchar();

   /* stop run */
   if (cm_transition(TR_STOP, run_number, str, sizeof(str), SYNC, MT_INFO) != CM_SUCCESS)
      printf(str);

   cm_disconnect_experiment();

   return 1;
}

/********************************************************************\

  Name:         odb_test.c
  Created by:   Stefan Ritt

  Contents:     Small demo program to show how to connect to the ODB

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "midas.h"

int run_number;

void run_number_changed(HNDLE hDB, HNDLE hKey, void *info)
{
   printf("New run number: %d\n", run_number);
}

int main()
{
   int status, size;
   HNDLE hDB, hKey;

   /* connect to experiment */
   status = cm_connect_experiment("", "", "ODBTest", NULL);
   if (status != CM_SUCCESS)
      return 1;

   /* get handle to online database */
   cm_get_experiment_database(&hDB, &hKey);

   /* read key "runinfo/run number" */
   size = sizeof(run_number);
   status =
       db_get_value(hDB, 0, "/runinfo/run number", &run_number, &size, TID_INT, TRUE);
   if (status != DB_SUCCESS) {
      printf("Cannot read run number");
      return 0;
   }

   printf("Current run number is %d\n", run_number);

   /* set new run number */
   run_number++;
   db_set_value(hDB, 0, "/runinfo/run number", &run_number, size, 1, TID_INT);


   /* now open run_number with automatic updates */
   db_find_key(hDB, 0, "/runinfo/run number", &hKey);
   db_open_record(hDB, hKey, &run_number, sizeof(run_number),
                  MODE_READ, run_number_changed, NULL);

   printf("Waiting for run number to change. Hit RETURN to abort\n");

   do {
      cm_yield(1000);
   } while (!ss_kbhit());

   db_close_record(hDB, hKey);

   /* disconnect from experiment */
   cm_disconnect_experiment();

   return 1;
}

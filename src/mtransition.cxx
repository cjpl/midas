/********************************************************************\

  Name:         mtransition.cxx
  Created by:   Stefan Ritt and Konstantin Olchanski

  Contents:     Command-line interface to run state transitions

  $Id: odbedit.c 4345 2008-10-13 06:45:19Z ritt@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "midas.h"

/*------------------------------------------------------------------*/

int read_state(HNDLE hDB)
{
  /* check if run is stopped */
  int state = STATE_STOPPED;
  int size = sizeof(state);
  int status = db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
  assert(status == DB_SUCCESS);
  return state;
}

void usage()
{
   fprintf(stderr, "Usage: mtransition [-v] [-h Hostname] [-e Experiment] commands...\n");
   fprintf(stderr, "Commands:\n");
   fprintf(stderr, "  START - start a new run\n");
   fprintf(stderr, "  STOP - stop the run\n");
   fprintf(stderr, "  PAUSE - pause the run\n");
   fprintf(stderr, "  RESUME - resume the run\n");
   fprintf(stderr, "  DELAY 100 - sleep for 100 seconds\n");
   fprintf(stderr, "  DELAY \"/logger/Auto restart delay\" - sleep for time specified in ODB variable\n");
   fprintf(stderr, "  IF \"/logger/Auto restart\" - continue only if ODB variable is set to TRUE\n");
   cm_disconnect_experiment();
   exit (1);
}

int main(int argc, char *argv[])
{
   int status;
   bool verbose = false;
   char host_name[HOST_NAME_LENGTH], exp_name[NAME_LENGTH];

   /* get default from environment */
   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

   /* parse command line parameters */

   if (argc < 2)
      usage(); // does not return

   for (int i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
         if (argv[i][1] == 'v')
            verbose = true;
         else if (argv[i][1] == 'e' && i < argc-1)
            strlcpy(exp_name, argv[++i], sizeof(exp_name));
         else if (argv[i][1] == 'h' && i < argc-1)
            strlcpy(host_name, argv[++i], sizeof(host_name));
         else
            usage(); // does not return
         }
      }

   status = cm_connect_experiment1(host_name, exp_name, "mtransition", NULL,
                                   DEFAULT_ODB_SIZE,
                                   DEFAULT_WATCHDOG_TIMEOUT);
   if (status != CM_SUCCESS) {
      fprintf(stderr,"Error: Cannot connect to experiment \'%s\' on host \'%s\', status %d\n",
              exp_name,
              host_name,
              status);
      exit(1);
   }

   if (verbose)
      printf("Connected to experiment \'%s\' on host \'%s\'\n", exp_name, host_name);

   HNDLE hDB;

   status = cm_get_experiment_database(&hDB, NULL);
   assert(status == CM_SUCCESS);

   for (int i=1; i<argc; i++) {

      if (argv[i][0] == '-') {
        
         // skip command line switches
         continue;

      } else if (strcmp(argv[i], "START") == 0) {

         /* start */

         /* check if run is already started */
         int state = read_state(hDB);

         if (state == STATE_RUNNING) {
            printf("START: Run is already started\n");
            cm_disconnect_experiment();
            exit(1);
         } else if (state == STATE_PAUSED) {
            printf("START: Run is paused, please use \"RESUME\"\n");
            cm_disconnect_experiment();
            exit(1);
         }

         /* get present run number */
         int old_run_number = 0;
         int size = sizeof(old_run_number);
         status = db_get_value(hDB, 0, "/Runinfo/Run number", &old_run_number, &size, TID_INT, FALSE);
         assert(status == DB_SUCCESS);
         assert(old_run_number >= 0);

         int new_run_number = old_run_number + 1;

         if (verbose)
            printf("Starting run %d\n", new_run_number);

         char str[256];
         status = cm_transition(TR_START, new_run_number, str, sizeof(str), SYNC, verbose);
         if (status != CM_SUCCESS) {
            /* in case of error, reset run number */
            status = db_set_value(hDB, 0, "/Runinfo/Run number", &old_run_number, sizeof(old_run_number), 1, TID_INT);
            assert(status == DB_SUCCESS);

            printf("START: cm_transition status %d, message \'%s\'\n", status, str);
         }

      } else if (strcmp(argv[i], "STOP") == 0) {

         /* check if run is stopped */

         int state = read_state(hDB);

         if (state == STATE_STOPPED) {
            printf("Run is already stopped, stopping again.\n");
         }

         char str[256];
         status = cm_transition(TR_STOP, 0, str, sizeof(str), SYNC, verbose);

         if (status != CM_SUCCESS)
            printf("STOP: cm_transition status %d, message \'%s\'\n", status, str);

      } else if (strcmp(argv[i], "PAUSE") == 0) {

         /* check if run is started */

         int state = read_state(hDB);

         if (state == STATE_PAUSED) {
            printf("PAUSE: Run is already paused\n");
            continue;
         }

         if (state != STATE_RUNNING) {
            printf("PAUSE: Run is not started\n");
            cm_disconnect_experiment();
            exit(1);
         }

         char str[256];
         status = cm_transition(TR_PAUSE, 0, str, sizeof(str), SYNC, verbose);

         if (status != CM_SUCCESS)
            printf("PAUSE: cm_transition status %d, message \'%s\'\n", status, str);

      } else if (strcmp(argv[i], "RESUME") == 0) {

         int state = read_state(hDB);

         if (state == STATE_RUNNING) {
            printf("RESUME: Run is already running\n");
            continue;
         } else if (state != STATE_PAUSED) {
            printf("RESUME: Run is not paused\n");
            cm_disconnect_experiment();
            exit(1);
         }

         char str[256];
         status = cm_transition(TR_RESUME, 0, str, sizeof(str), SYNC, verbose);
         if (status != CM_SUCCESS)
            printf("RESUME: cm_transition status %d, message \'%s\'\n", status, str);

      } else if (strcmp(argv[i], "DELAY") == 0) {

         if (argv[i+1] == NULL) {
            fprintf(stderr,"Command DELAY requires an argument\n");
            usage(); // does not return
         }

         const char* arg = argv[++i];
         int delay = 0;

         if (isdigit(arg[0])) {
            delay = atoi(arg);
         } else if (arg[0] == '/') {
            int size = sizeof(delay);
            status = db_get_value(hDB, 0, arg, &delay, &size, TID_INT, FALSE);
            if (status !=  DB_SUCCESS) {
               fprintf(stderr,"DELAY: Cannot read ODB variable \'%s\', status %d\n", arg, status);
               cm_disconnect_experiment();
               exit(1);
            }
         }

         if (verbose)
            printf("DELAY \'%s\' %d sec\n", arg, delay);

         status = ss_sleep(delay*1000);
         assert(status == SS_SUCCESS);

      } else if (strcmp(argv[i], "IF") == 0) {

         if (argv[i+1] == NULL) {
            fprintf(stderr,"Command IF requires an argument\n");
            usage(); // does not return
         }

         const char* arg = argv[++i];
         int value = 0;

         if (isdigit(arg[0])) {
            value = atoi(arg);
         } else if (arg[0] == '/') {
            int size = sizeof(value);
            status = db_get_value(hDB, 0, arg, &value, &size, TID_BOOL, FALSE);
            if (status ==  DB_TYPE_MISMATCH) {
               status = db_get_value(hDB, 0, arg, &value, &size, TID_INT, FALSE);
            }
            if (status !=  DB_SUCCESS) {
               fprintf(stderr,"IF: Cannot read ODB variable \'%s\', status %d\n", arg, status);
               cm_disconnect_experiment();
               exit(1);
            }
         }

         if (verbose)
            printf("IF \'%s\' value %d\n", arg, value);

         if (!value) {
            cm_disconnect_experiment();
            exit(0);
         }

      } else {
        
         fprintf(stderr,"Unknown command \'%s\'\n", argv[i]);
         usage(); // does not return

      }
   }

   cm_disconnect_experiment();

   return 0;
}

// end

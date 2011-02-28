/*******************************************************************\

  Name:         fedummy.c
  Created by:   K.Olchanski

  Contents:     Front end for creating dummy data

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#include "midas.h"

#define FE_NAME "fejrpc"
#define EQ_NAME "RpcExample"
#define EQ_EVID 1

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = FE_NAME;

/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms */
//INT display_period = 3000;
INT display_period = 0;

/* maximum event size produced by this frontend */
INT max_event_size      = 2000;
INT max_event_size_frag = 0;

/* buffer size to hold events */
INT event_buffer_size = 10*2000;

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

HNDLE hDB;
INT gbl_run_number;

int read_slow_event(char *pevent, int off);

/*-- Equipment list ------------------------------------------------*/

EQUIPMENT equipment[] = {

  { EQ_NAME,         /* equipment name */
    {
      EQ_EVID, (1<<EQ_EVID), /* event ID, trigger mask */
      "SYSTEM",              /* event buffer */
      EQ_PERIODIC,          /* equipment type */
      0,                    /* event source */
      "MIDAS",              /* format */
      TRUE,                 /* enabled */
      RO_RUNNING|RO_STOPPED|RO_PAUSED|RO_ODB, /* When to read */
      1000,                 /* poll every so milliseconds */
      0,                    /* stop run after this event limit */
      0,                    /* number of sub events */
      1,                    /* history period*/
      "", "", ""
    },
    read_slow_event, /* readout routine */
  },
  { "" }
};

#ifdef __cplusplus
}
#endif

/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.
  
  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send 
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.

\********************************************************************/

int count_slow = 0;

static int configure()
{
   int size, status;
   int example_int = 0;

   size = sizeof(int);
   status = db_get_value(hDB, 0, "/Equipment/" EQ_NAME "/Settings/example_int", &example_int, &size, TID_INT, TRUE);
   printf("Example_int: %d\n", example_int);
   
   return SUCCESS;
}

// RPC handlers

RPC_LIST rpc_list[] =
{
   { 101, "rpc_101", {
         {TID_STRING, RPC_IN}, // arg0
         {TID_STRING, RPC_IN}, // arg1
         {TID_STRING, RPC_IN}, // arg2
         {TID_STRING, RPC_IN}, // arg3
         {TID_STRING, RPC_IN}, // arg4
         {TID_STRING, RPC_IN}, // arg5
         {TID_STRING, RPC_IN}, // arg6
         {TID_STRING, RPC_IN}, // arg7
         {TID_STRING, RPC_IN}, // arg8
         {TID_STRING, RPC_IN}, // arg9
         {0}} },
   { 102, "rpc_102", {
         {TID_STRING, RPC_OUT}, // return string
         {TID_INT,    RPC_IN},  // return string max length
         {TID_STRING, RPC_IN}, // arg0
         {TID_STRING, RPC_IN}, // arg1
         {TID_STRING, RPC_IN}, // arg2
         {TID_STRING, RPC_IN}, // arg3
         {TID_STRING, RPC_IN}, // arg4
         {TID_STRING, RPC_IN}, // arg5
         {TID_STRING, RPC_IN}, // arg6
         {TID_STRING, RPC_IN}, // arg7
         {TID_STRING, RPC_IN}, // arg8
         {TID_STRING, RPC_IN}, // arg9
         {0}} },
   { 0 }
};

INT rpc101_callback(INT index, void *prpc_param[])
{
   const char* arg0 = CSTRING(0);
   const char* arg1 = CSTRING(1);
   const char* arg2 = CSTRING(2);

   cm_msg(MINFO, "rpc_callback", "--------> rpc101_callback: index %d, args [%s] [%s] [%s]", index, arg0, arg1, arg2);

   int example_int = strtol(arg0, NULL, 0);
   int size = sizeof(int);
   int status = db_set_value(hDB, 0, "/Equipment/" EQ_NAME "/Settings/example_int", &example_int, size, 1, TID_INT);

   return RPC_SUCCESS;
}

INT rpc102_callback(INT index, void *prpc_param[])
{
   char* return_buf = CSTRING(0);
   int   return_max_length = CINT(1);
   const char* arg0 = CSTRING(2);
   const char* arg1 = CSTRING(3);
   const char* arg2 = CSTRING(4);

   cm_msg(MINFO, "rpc_callback", "--------> rpc102_callback: index %d, max_length %d, args [%s] [%s] [%s]", index, return_max_length, arg0, arg1, arg2);

   int example_int = strtol(arg0, NULL, 0);
   int size = sizeof(int);
   int status = db_set_value(hDB, 0, "/Equipment/" EQ_NAME "/Settings/example_int", &example_int, size, 1, TID_INT);

   char tmp[256];
   time_t now = time(NULL);
   sprintf(tmp, "current time is %d %s", (int)now, ctime(&now));

   strlcpy(return_buf, tmp, return_max_length);

   return RPC_SUCCESS;
}

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
   int status;

   //cm_set_watchdog_params (FALSE, 0);

   status = rpc_register_functions(rpc_list, NULL);
   assert(status == SUCCESS);

   status = cm_register_function(101, rpc101_callback);
   assert(status == SUCCESS);

   status = cm_register_function(102, rpc102_callback);
   assert(status == SUCCESS);

   configure();

   return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  printf("Begin run %d\n", run_number);
  gbl_run_number = run_number;

  configure();

  count_slow = 0;

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
  printf("End run %d!\n", run_number);

  cm_msg(MINFO, frontend_name, "read %d slow events", count_slow);

  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  //printf("frontend_loop!\n");
  ss_sleep(10);
  return SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

extern "C" INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
  if (test) {
    ss_sleep (count);
  }
  return (0);
}

/*-- Interrupt configuration ---------------------------------------*/

extern "C" INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
  switch(cmd)
    {
    case CMD_INTERRUPT_ENABLE:
      break;
    case CMD_INTERRUPT_DISABLE:
      break;
    case CMD_INTERRUPT_ATTACH:
      break;
    case CMD_INTERRUPT_DETACH:
      break;
    }
  return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

double get_time()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}

int read_slow_event(char *pevent, int off)
{
  bk_init32(pevent);

  count_slow++;

  double* pdatad;
  bk_create(pevent, "SLOW", TID_DOUBLE, &pdatad);

  time_t t = time(NULL);
  pdatad[0] = count_slow;
  pdatad[1] = t;
  pdatad[2] = 100.0*sin(M_PI*t/60);
  printf("time %d, data %f\n", (int)t, pdatad[2]);

  bk_close(pevent, pdatad + 3);

  return bk_size(pevent);
}

// end file

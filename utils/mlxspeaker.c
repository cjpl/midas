/********************************************************************\

  Name:         mlxspeaker.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Speaks midas messages (UNIX version)

  $Log$
  Revision 1.3  1999/05/06 19:09:49  pierre
  - Fix empty trailing char on the message which would hold festival

  Revision 1.2  1998/10/12 12:19:03  midas
  Added Log tag in header


  Previous revision history
  ------------------------------------------------------------------
  date        by    modification
  --------    ---   ------------------------------------------------
  30-JAN-97   PAA   created
  05-JUN-98   Glenn Ported to "festival speech system"

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "midas.h"
#include "msystem.h"

#define SPEECH_PROGRAM "festival --tts -"

static FILE *fp = NULL;
BOOL   debug=FALSE;

/*------------------------------------------------------------------*/

void sigpipehandler( int sig )
{
	cm_msg(MERROR, "Speaker", "No speech synthesizer attached" );
	cm_disconnect_experiment();
	pclose( fp );
	exit( 2 );
}

void siginthandler( int sig )
{
	cm_msg(MINFO, "Speaker", "Speaker interrupted" );
	cm_disconnect_experiment();
	pclose( fp );
	exit( 0 );
}

/*----- receive_message --------------------------------------------*/

void receive_message(HNDLE hBuf, HNDLE id, EVENT_HEADER *header, void *message)
{
char str[256], *pc, *sp;

  /* print message */
  printf("%s\n", (char *)(message));

  if (fp == NULL) {
	  fputs( "Speech synthesizer not enabled - terminating\n", stderr );
	  cm_disconnect_experiment();
	  exit( 2 );
  }

  if (debug)
    {
      printf("evID:%x Mask:%x Serial:%i Size:%d\n"
	     ,header->event_id
	     ,header->trigger_mask
	     ,header->serial_number
	     ,header->data_size);
      pc = strchr((char *)(message),']')+2;
    }

  /* skip none talking message */
  if (header->trigger_mask == MT_TALK ||
      header->trigger_mask == MT_USER)
    {
      pc = strchr((char *)(message),']')+2;
      sp = pc + strlen(pc) - 1;
      while (isblank(*sp))
      	sp--;
      *sp ='\0';
      if (debug) 
	{
	  printf("<%s>", pc );
	  printf(" sending msg to festival\n");
	}
      fprintf( fp, "%s.\n.\n", pc );
      fflush( fp );
    }
  
  return;
}

/*------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  INT    status, i, ch;
  char   host_name[NAME_LENGTH];
  char   exp_name[NAME_LENGTH];
  char *speech_program = SPEECH_PROGRAM;
  
  /* get default from environment */
  cm_get_environment(host_name, exp_name);
  
  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
	debug = TRUE;
      else if (argv[i][0] == '-')
	{
	  if (i+1 >= argc || argv[i+1][0] == '-')
	    goto usage;
	  if (argv[i][1] == 'e')
	    strcpy(exp_name, argv[++i]);
	  else if (argv[i][1] == 'h')
	    strcpy(host_name, argv[++i]);
	  else if (argv[i][1] == 'c')
	    speech_program = argv[++i];
	  else
        {
usage:
        printf("usage: mlxspeaker [-h Hostname] [-e Experiment] [-c command]\n");
	printf("  where `command' is used to start the speech synthesizer,\n" );
	printf("  which should read text from it's standard input.\n" );
	printf("   eg: mlxspeaker -c 'festival --tts -'\n\n" );
        return 0;
        }
      }
    }

  /* now connect to server */
  status = cm_connect_experiment(host_name, exp_name, "Speaker", NULL);
  if (status != CM_SUCCESS)
    return 1;

  cm_msg_register(receive_message);

  /* Handle SIGPIPE signals generated from errors on the pipe */
  signal( SIGPIPE, sigpipehandler );
  signal( SIGINT, siginthandler );
  if (NULL == (fp = popen( speech_program, "w" ))) {
	  cm_msg( MERROR, "Speaker", "Unable to start \"%s\": %s\n",
		  speech_program, strerror( errno ) );
	  cm_disconnect_experiment();
	  exit( 2 );
  }

  printf("Midas Message Speaker connected to %s.\n", 
          host_name[0] ? host_name : "local host");

  do
    {
    status = cm_yield(1000);
    } while (status != RPC_SHUTDOWN && status != SS_ABORT);
  
  pclose( fp );
  
  cm_disconnect_experiment();
  return 1;
}

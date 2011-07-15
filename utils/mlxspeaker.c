/********************************************************************\

  Name:         mlxspeaker.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Speaks midas messages (UNIX version)

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "midas.h"
#include "msystem.h"

#ifdef OS_DARWIN
#define SPEECH_PROGRAM "say"
#else
#define SPEECH_PROGRAM "festival --tts -"
#endif

static FILE *fp = NULL;
char mtUserStr[128], mtTalkStr[128];
DWORD shutupTime = 10;

/*------------------------------------------------------------------*/

void sigpipehandler(int sig)
{
   cm_msg(MERROR, "Speaker", "No speech synthesizer attached");
   cm_disconnect_experiment();
   pclose(fp);
   exit(2);
}

void siginthandler(int sig)
{
   cm_msg(MINFO, "Speaker", "Speaker interrupted");
   cm_disconnect_experiment();
   pclose(fp);
   exit(0);
}

/*----- receive_message --------------------------------------------*/

void receive_message(HNDLE hBuf, HNDLE id, EVENT_HEADER * header, void *message)
{
   char str[256], *pc, *sp;
   static DWORD last_beep = 0;

   /* print message */
   printf("%s\n", (char *) (message));

   if (fp == NULL) {
      fputs("Speech synthesizer not enabled - terminating\n", stderr);
      cm_disconnect_experiment();
      exit(2);
   }

   /* skip none talking message */
   if (header->trigger_mask == MT_TALK || header->trigger_mask == MT_USER) {
      pc = strchr((char *) (message), ']') + 2;
      sp = pc + strlen(pc) - 1;
      while (*sp == ' ' || *sp == '\t')
         sp--;
      *(++sp) = '\0';

      /* Send beep first */
      // "play --volume=0.3 /etc/mt_talk.wav"
      if ((ss_time() - last_beep) > shutupTime) {
         switch (header->trigger_mask) {
         case MT_TALK:
            if (mtTalkStr[0])
               sprintf(str, "%s", mtTalkStr);
            break;
         case MT_USER:
            if (mtUserStr[0])
               sprintf(str, "%s", mtUserStr);
            break;
         }
         ss_system(str);
         last_beep = ss_time();
         ss_sleep(1000);
      }

#ifdef OS_DARWIN      
      sprintf(str, "say %s.", pc);
      ss_system(str);
#else
      fprintf(fp, "%s.\n", pc);
      fflush(fp);
#endif      
   }

   return;
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   BOOL daemon = FALSE;
   INT status, i, ch;
   char host_name[HOST_NAME_LENGTH];
   char exp_name[NAME_LENGTH];
   char *speech_program = SPEECH_PROGRAM;

   /* get default from environment */
   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

#ifdef OS_DARWIN
   strlcpy(mtTalkStr, "afplay $MIDASSYS/utils/notify.wav", sizeof(mtTalkStr));
   strlcpy(mtUserStr, "afplay $MIDASSYS/utils/notify.wav", sizeof(mtTalkStr));
#endif   

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'D')
         daemon = TRUE;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'e')
            strcpy(exp_name, argv[++i]);
         else if (argv[i][1] == 'h')
            strcpy(host_name, argv[++i]);
         else if (argv[i][1] == 'c')
            speech_program = argv[++i];
         else if (argv[i][1] == 't')
            strcpy(mtTalkStr, argv[++i]);
         else if (argv[i][1] == 'u')
            strcpy(mtUserStr, argv[++i]);
         else if (argv[i][1] == 's')
            shutupTime = atoi(argv[++i]);
         else {
          usage:
            printf
                ("usage: mlxspeaker [-h Hostname] [-e Experiment] [-c command] [-D] daemon\n");
            printf("                  [-t mt_talk] Specify the mt_talk alert command\n");
            printf("                  [-u mt_user] Specify the mt_user alert command\n");
            printf("                  [-s shut up time] Specify the min time interval between alert [s]\n");
            printf("                  The -t & -u switch require a command equivalent to:\n");
            printf("                  '-t play --volume=0.3 file.wav'\n");
            printf("                  [-c command] Used to start the speech synthesizer,\n");
            printf("                              which should read text from it's standard input.\n");
            printf("                              eg: mlxspeaker -c 'festival --tts -'\n\n");
            return 0;
         }
      }
   }

   if (daemon) {
      printf("Becoming a daemon...\n");
      ss_daemon_init(FALSE);
   }

   /* Handle SIGPIPE signals generated from errors on the pipe */
   signal(SIGPIPE, sigpipehandler);
   signal(SIGINT, siginthandler);
   
   fp = popen(speech_program, "w");
   if (fp == NULL) {
      cm_msg(MERROR, "Speaker", "Unable to start \"%s\": %s\n",
             speech_program, strerror(errno));
      cm_disconnect_experiment();
      exit(2);
   }
   
   /* now connect to server */
   status = cm_connect_experiment(host_name, exp_name, "Speaker", NULL);
   if (status != CM_SUCCESS)
      return 1;

   cm_msg_register(receive_message);

   printf("Midas Message Speaker connected to %s. Press \"!\" to exit.\n", host_name[0] ? host_name : "local host");

   /* initialize terminal */
   ss_getchar(0);
   
   do {
      status = cm_yield(1000);
      while (ss_kbhit()) {
         ch = ss_getchar(0);
         if (ch == -1)
            ch = getchar();
         if (ch == '!')
            status = RPC_SHUTDOWN;
      }
            
   } while (status != SS_ABORT && status != RPC_SHUTDOWN);

   /* reset terminal */
   ss_getchar(TRUE);
   
   pclose(fp);

   cm_disconnect_experiment();
   return 1;
}

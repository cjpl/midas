/********************************************************************\

  Name:         mspeaker.c
  Created by:   Pierre-Andre Amaudruz, Stefan Ritt

  Contents:     Speaks midas messages

  Requires Microsoft Speech SDK 5.1 to be installed under

  \Program Files\Microsoft Speech SDK 5.1\

  $Id$

\********************************************************************/

#include <stdlib.h>
#include <sapi.h>
#include "midas.h"
#include "msystem.h"

char mtUserStr[128], mtTalkStr[128];
DWORD shutupTime = 10;

ISpVoice* Voice = NULL;				// The voice interface

char *type_name[] = {
   "ERROR",
   "INFO",
   "DEBUG" "USER",
   "LOG",
   "TALK",
   "CALL",
};

/*----- receive_message --------------------------------------------*/

void receive_message(HNDLE hBuf, HNDLE id, EVENT_HEADER * header, void *message)
{
   char str[256], *pc, *sp;
   static DWORD last_beep = 0;

   /* print message */
   printf("%s\n", (char *) (message));

   /* skip none talking message */
   if (header->trigger_mask == MT_TALK || header->trigger_mask == MT_USER) {
      
      /* strip [<username.] */
      pc = strchr((char *) (message), ']') + 2;
      sp = pc + strlen(pc) - 1;
      while (*sp == ' ' || *sp == '\t')
         sp--;
      *(++sp) = '\0';

      if ((ss_time() - last_beep) > shutupTime) {
         switch (header->trigger_mask) {
         case MT_TALK:
            if (mtTalkStr[0])
               sprintf(str, mtTalkStr);
            break;
         case MT_USER:
            if (mtUserStr[0])
               sprintf(str, mtUserStr);
            break;
         }

         // beep first
         PlaySound(str, NULL, SND_SYNC);
         last_beep = ss_time();

         ss_sleep(500);

         wchar_t wcstring[1000];
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)pc, -1, wcstring, 1000); 
         
         // Speak!
         Voice->Speak(wcstring, SPF_DEFAULT, NULL );
      }
   }

   return;
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   INT status, i, ch;
   char host_name[NAME_LENGTH];
   char exp_name[NAME_LENGTH];

   /* get default from environment */
   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

   /* default wave files */
   strcpy(mtTalkStr, "\\Windows\\Media\\notify.wav");
   strcpy(mtUserStr, "\\Windows\\Media\\notify.wav");

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'e')
            strcpy(exp_name, argv[++i]);
         else if (argv[i][1] == 'h')
            strcpy(host_name, argv[++i]);
         else if (argv[i][1] == 't')
            strcpy(mtTalkStr, argv[++i]);
         else if (argv[i][1] == 'u')
            strcpy(mtUserStr, argv[++i]);
         else if (argv[i][1] == 's')
            shutupTime = atoi(argv[++i]);
         else {
          usage:
            printf("usage: mspeaker [-h <hostname>] [-e <experiment>]\n\n");
            printf("  [-t <file>]  Specify the alert wave file for MT_TALK messages\n");
            printf("  [-u <file>]  Specify the alert wave file for MT_USER messages\n");
            printf("  [-s <sec>]   Specify the min time interval between alert [s]\n");
            return 0;
         }
      }
   }

	// Initialize COM
	CoInitialize ( NULL );

	// Create the voice interface object
	CoCreateInstance ( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&Voice );
   if (Voice == NULL) {
      printf("Cannot initialize speech system\n");
      return 1;
   }

 reconnect:

   /* now connect to server */
   status = cm_connect_experiment(host_name, exp_name, "Speaker", NULL);
   if (status != CM_SUCCESS)
      return 1;

   cm_msg_register(receive_message);

   /* increased watchdog timeout for long messages */
   cm_set_watchdog_params(TRUE, 60000);

   printf("Midas Message Talker connected to %s. Press \"!\" to exit\n",
          host_name[0] ? host_name : "local host");

   do {
      status = cm_yield(1000);
      if (ss_kbhit()) {
         ch = getch();
         if (ch == 'r')
            ss_clear_screen();
         if (ch == '!')
            break;
      }

      if (status == SS_ABORT) {
         cm_disconnect_experiment();
         printf("Trying to reconnect...\n");
         ss_sleep(3000);
         goto reconnect;
      }

   } while (status != RPC_SHUTDOWN);

	// Shutdown the voice
	if (Voice != NULL) 
      Voice->Release();

	// Shutdown COM
	CoUninitialize();

   cm_disconnect_experiment();
   return 1;
}

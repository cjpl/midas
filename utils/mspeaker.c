/********************************************************************\

  Name:         mspeaker.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     Speaks midas messages

  $Log$
  Revision 1.6  2000/04/10 13:16:14  midas
  Added reconnect

  Revision 1.5  2000/04/05 14:59:24  midas
  Added time display

  Revision 1.4  2000/04/05 08:24:26  midas
  Changed speech font which crashed program

  Revision 1.3  1999/07/06 09:03:16  midas
  Changed include directory for provoice to c:

  Revision 1.2  1998/10/12 12:19:04  midas
  Added Log tag in header


\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "c:\provoice\include\fbvspch.h"

LPSTR           lpPhonetics;
LPSPEECHBLOCK   lpSCB;

char *type_name[] = {
  "ERROR",
  "INFO",
  "DEBUG"
  "USER",
  "LOG",
  "TALK",
  "CALL",
};

/*------------------------------------------------------------------*/

/*----- receive_message --------------------------------------------*/

void receive_message(HNDLE hBuf, HNDLE id, EVENT_HEADER *header, void *message)
{
char str[256], *pc;

  /* print message */
  cm_asctime(str, 256);
  str[19] = 0;
  printf("%s %s\n", str+4, (char *)(message));

  /* skip none talking message */
  if (header->trigger_mask == MT_TALK ||
      header->trigger_mask == MT_USER)
    {
    pc = strchr((char *)(message),']')+2;

    while(*pc)
      {
      if (*pc != '@')
        {
        strcpy(str, pc);
        if (strchr(str, '@'))
          *strchr(str, '@') = 0;
        pc += strlen(str);

        while (SpeechStatus(lpSCB) != 0)
          ss_sleep(1000);
        ss_sleep(500);

        lpPhonetics = TextToPhonetics(lpSCB, str, 0);
        SpeakPhonetics(lpSCB, lpPhonetics);
        //  save phonetics to .wav file
        //  WritePhonetics(lpSCB, lpPhonetics, "hello.wav");
        }
      
      if (*pc == '@')
        {
        strcpy(str, pc+1);
        *strchr(str, '@') = 0;
        pc += 2+strlen(str);

        while (SpeechStatus(lpSCB) != 0)
          ss_sleep(1000);
        ss_sleep(500);

        PlaySound(str, NULL, SND_SYNC);
	      }
      }

    ss_sleep(1000);
    }

  return;
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
INT    status, i, ch;
char   host_name[NAME_LENGTH];
char   exp_name[NAME_LENGTH];

  /* get default from environment */
  cm_get_environment(host_name, exp_name);

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'e')
        strcpy(exp_name, argv[++i]);
      else if (argv[i][1] == 'h')
        strcpy(host_name, argv[++i]);
      else
        {
usage:
        printf("usage: mspeaker [-h Hostname] [-e Experiment]\n\n");
        return 0;
        }
      }
    }

  //  register to SB
  // lpSCB = OpenSpeech(0, 0, "Esnb1k8"); 
  lpSCB = OpenSpeech(0, 0, NULL); 
	if (lpSCB == NULL)
	  {
		printf("Cannot allocate Speech Control Block\n");
		goto out;
	  }
	else
	  {
	  status = lpSCB->speechStatus;
   	if ((status <= E_SPEECH_ERROR) && (status > W_SPEECH_WARNING))
	    {
		  printf("Open Speech error:%d\n", status);
		  goto out;
	    }
	  }
   
  lpPhonetics = NULL;

reconnect:

  /* now connect to server */
  status = cm_connect_experiment(host_name, exp_name, "Speaker", NULL);
  if (status != CM_SUCCESS)
    return 1;

  cm_msg_register(receive_message);

  printf("Midas Message Talker connected to %s. Press \"!\" to exit\n", 
          host_name[0] ? host_name : "local host");

  do
    {
    status = cm_yield(1000);
	  if (ss_kbhit())
      {
	    ch = getch();
	    if (ch == 'r')
	      ss_clear_screen();
	    if (ch == '!')
	      break;
	    }

    if (status == SS_ABORT)
      {
      cm_disconnect_experiment();
      printf("Trying to reconnect...\n");
      ss_sleep(3000);
      goto reconnect;
      }

    } while (status != RPC_SHUTDOWN);
  
out:
  
  CloseSpeech(lpSCB);
  if (lpPhonetics)
	  FreePhoneticsBuffer(lpPhonetics);

  cm_disconnect_experiment();
  return 1;
}

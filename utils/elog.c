 /********************************************************************\

  Name:         elog.c
  Created by:   Stefan Ritt

  Contents:     Electronic logbook utility   

  $Log$
  Revision 1.11  2000/05/08 14:29:39  midas
  Added delete option in ELog

  Revision 1.10  1999/10/08 22:00:30  midas
  Finished editing of elog messages

  Revision 1.9  1999/10/06 06:56:32  midas
  Added "modification" to message type

  Revision 1.8  1999/09/28 11:05:50  midas
  Use external editor for composing text

  Revision 1.7  1999/09/27 12:54:28  midas
  Added -m flag

  Revision 1.6  1999/09/17 11:48:08  midas
  Alarm system half finished

  Revision 1.5  1999/09/16 07:36:10  midas
  Added automatic host name in author field

  Revision 1.4  1999/09/15 13:33:36  midas
  Added remote el_submit functionality

  Revision 1.3  1999/09/15 08:06:58  midas
  Use own Ctrl-C handler

  Revision 1.1  1999/09/14 15:15:27  midas
  Initial revision


\********************************************************************/

#include <stdio.h>
#include <signal.h>
#include "midas.h"
#include "msystem.h"

char type_list[20][NAME_LENGTH] = {
  "Routine",
  "Shift summary",
  "Minor error",
  "Severe error",
  "Fix",
  "Info",
  "Modification",
  "Complaints",
  "Reply",
  "Alarm",
  "Test",
  "Other"
};

char system_list[20][NAME_LENGTH] = {
  "General",
  "DAQ",
  "Detector",
  "Electronics",
  "Target",
  "Beamline"
};

HNDLE hDB;

/*------------------------------------------------------------------*/

INT query_params(char *author, char *type, char *syst, char *subject, 
                 char *text, char *textfile, char attachment[3][256])
{
char str[1000], tmpfile[256];
FILE *f;
int  i;

  while (!author[0])
    {
    printf("Author: ");
    ss_gets(author, 80);
    if (!author[0])
      printf("Author must be supplied!\n");
    }

  if (!type[0])
    {
    do
      {
      printf("Select a message type from following list:\n<");
      for (i=0 ; i<20 && type_list[i][0] ; i++)
        {
        if (i % 8 == 7)
          printf("\n");
        if (type_list[i+1][0])
          printf("%s,", type_list[i]);
        else
          printf("%s>\n", type_list[i]);
        }

      ss_gets(type, 80);

      /* check if valid type */
      for (i=0 ; i<20 && type_list[i][0] ; i++)
        if (equal_ustring(type_list[i], type))
          break;
      if (!type_list[i][0])
        printf("Not a valid message type!\n");
      else
        break;

      } while (1);
    }

  if (!syst[0])
    {
    do
      {
      printf("Select a system from following list:\n<");
      for (i=0 ; i<20 && system_list[i][0] ; i++)
        {
        if (i % 8 == 7)
          printf("\n");
        if (system_list[i+1][0])
          printf("%s,", system_list[i]);
        else
          printf("%s>\n", system_list[i]);
        }

      ss_gets(syst, 80);

      /* check if valid system */
      for (i=0 ; i<20 && system_list[i][0] ; i++)
        if (equal_ustring(system_list[i], syst))
          break;
      if (!system_list[i][0])
        printf("Not a valid system!\n");
      else
        break;

      } while (1);
    }

  if (!subject[0])
    {
    printf("Subject: ");
    ss_gets(subject, 256);
    }

  if (!text[0] && !textfile[0])
    {
    if (getenv("EDITOR"))
      {
      sprintf(tmpfile, "tmp%d.txt", ss_getpid());
      f = fopen(tmpfile, "wt");
      if (f == NULL)
        {
        printf("Cannot open temporary file for editor.\n");
        exit(1);
        }

      fprintf(f, "Author: %s\n", author);
      fprintf(f, "Type: %s\n", type);
      fprintf(f, "System: %s\n", syst);
      fprintf(f, "Subject: %s\n", subject);
      fprintf(f, "--------------------------------\n");
      fclose(f);

      sprintf(str, "%s %s", getenv("EDITOR"), tmpfile);
      system(str);

      f = fopen(tmpfile, "rt");
      if (f == NULL)
        {
        printf("Cannot open temporary file for editor.\n");
        exit(1);
        }

      text[0] = 0;
      while (!feof(f))
        {
        str[0] = 0;
        fgets(str, sizeof(str), f);
        if (strncmp(str, "Author: ", 8) == 0)
          {
          strcpy(author, str+8);
          if (strchr(author, '\n'))
            *strchr(author, '\n') = 0;
          }
        else if (strncmp(str, "Type: ", 6) == 0)
          {
          strcpy(type, str+6);
          if (strchr(type, '\n'))
            *strchr(type, '\n') = 0;
          }
        else if (strncmp(str, "System: ", 8) == 0)
          {
          strcpy(syst, str+8);
          if (strchr(syst, '\n'))
            *strchr(syst, '\n') = 0;
          }
        else if (strncmp(str, "Subject: ", 9) == 0)
          {
          strcpy(subject, str+9);
          if (strchr(subject, '\n'))
            *strchr(subject, '\n') = 0;
          }
        else if (strcmp(str, "--------------------------------\n") == 0)
          {
          }
        else
          strcat(text, str);
        }
      fclose(f);
      remove(tmpfile);
      }
    else
      {
      printf("Finish message text with empty line!\nMessage text: ");
      do
        {
        ss_gets(str, 1000);
        if (str[0])
          {
          strcat(text, str);
          strcat(text, "\n");
          }
        } while (str[0]);
      }
    }

  if (!attachment[0][0] && !author[0])
    {
    for (i=0 ; i<3 ; i++)
      {
      do
        {
        printf("Optional file attachment %d: ", i+1);
        ss_gets(attachment[i], 256);
        if (!attachment[i][0])
          break;
        f = fopen(attachment[i], "r");
        if (f == NULL)
          printf("File does not exist!\n");
        else
          fclose(f);
        } while (f == NULL);

      if (!attachment[i][0])
        break;
      }
    }

  return SUCCESS;
}

/*------------------------------------------------------------------*/

void ctrlc_handler(int sig)
{
  cm_disconnect_experiment();
  ss_ctrlc_handler(NULL);
  raise(sig);
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
char      author[80], type[80], system[80], subject[256], text[10000];
char      host_name[256], exp_name[NAME_LENGTH], str[256], lhost_name[256], textfile[256];
char      *buffer[3], attachment[3][256];
INT       att_size[3];
struct hostent *phe;
INT       i, n, status, fh, n_att, size;
HNDLE     hkey;

  /* turn off system message */
  cm_set_msg_print(0, MT_ALL, puts);

  author[0] = type[0] = system[0] = subject[0] = text[0] = textfile[0] = 0;
  host_name[0] = exp_name[0] = n_att = 0;
  for (i=0 ; i<3 ; i++)
    {
    attachment[i][0] = 0;
    buffer[i] = NULL;
    att_size[i] = 0;
    }

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'h')
        strcpy(host_name, argv[++i]);
      else if (argv[i][1] == 'e')
        strcpy(exp_name, argv[++i]);
      else if (argv[i][1] == 'a')
        strcpy(author, argv[++i]);
      else if (argv[i][1] == 't')
        strcpy(type, argv[++i]);
      else if (argv[i][1] == 's')
        strcpy(system, argv[++i]);
      else if (argv[i][1] == 'b')
        strcpy(subject, argv[++i]);
      else if (argv[i][1] == 'f')
        strcpy(attachment[n_att++], argv[++i]);
      else if (argv[i][1] == 'm')
        strcpy(textfile, argv[++i]);
      else
        {
usage:
        printf("\nusage: elog [-h hostname] [-e experiment]\n");
        printf("          -a <author> -t <type> -s <system> -b <subject>\n");
        printf("          -f <attachment> -m <textfile> <text>\n");
        printf("\nArguments with blanks must be enclosed in quotes\n");
        printf("The elog message can either be submitted on the command line\n");
        printf("or in a file with the -m flag.\n");
        return 0;
        }
      }
    else
      strcpy(text, argv[i]);
    }

  /* connect to experiment */
  status = cm_connect_experiment(host_name, exp_name, "ELog", NULL);
  if (status != CM_SUCCESS)
    {
    cm_get_error(status, str);
    puts(str);
    return 1;
    }

  cm_get_experiment_database(&hDB, NULL);

  /* re-establish Ctrl-C handler */
  ss_ctrlc_handler(ctrlc_handler);

  /* get type list from ODB */
  size = 20*NAME_LENGTH;
  if (db_find_key(hDB, 0, "/Elog/Types", &hkey) != DB_SUCCESS)
    db_set_value(hDB, 0, "/Elog/Types", type_list, NAME_LENGTH*20, 20, TID_STRING);
  db_find_key(hDB, 0, "/Elog/Types", &hkey);
  if (hkey)
    db_get_data(hDB, hkey, type_list, &size, TID_STRING);

  /* get system list from ODB */
  size = 20*NAME_LENGTH;
  if (db_find_key(hDB, 0, "/Elog/Systems", &hkey) != DB_SUCCESS)
    db_set_value(hDB, 0, "/Elog/Systems", system_list, NAME_LENGTH*20, 20, TID_STRING);
  db_find_key(hDB, 0, "/Elog/Systems", &hkey);
  if (hkey)
    db_get_data(hDB, hkey, system_list, &size, TID_STRING);

  /* complete missing parameters */
  status = query_params(author, type, system, subject, text, textfile, attachment);
  if (status != EL_SUCCESS)
    return 0;

  /* check valid type and system */
  for (i=0 ; i<20 && type_list[i][0] ; i++)
    if (equal_ustring(type_list[i], type))
      break;

  if (i == 20 || !type_list[i][0])
    {
    printf("Type \"%s\" is invalid.\n", type);
    cm_disconnect_experiment();
    return 0;
    }

  for (i=0 ; i<20 && system_list[i][0] ; i++)
    if (equal_ustring(system_list[i], system))
      break;

  if (i == 20 || !system_list[i][0])
    {
    printf("System \"%s\" is invalid.\n", system);
    cm_disconnect_experiment();
    return 0;
    }

  /*---- open text file ----*/

  if (textfile[0])
    {
    fh = open(textfile, O_RDONLY | O_BINARY);
    if (fh < 0)
      {
      printf("Message file \"%s\" does not exist.\n", textfile);
      cm_disconnect_experiment();
      return 0;
      }

    size = lseek(fh, 0, SEEK_END);
    lseek(fh, 0, SEEK_SET);

    if (size > sizeof(text))
      {
      printf("Message file \"%s\" is too long (%d bytes max).\n", sizeof(text));
      cm_disconnect_experiment();
      return 0;
      }

    i = read(fh, text, size);
    if (i < size)
      {
      printf("Cannot fully read message file \"%s\".\n", textfile);
      cm_disconnect_experiment();
      return 0;
      }

    close(fh);
    }

  /*---- open attachment file ----*/

  for (i = 0 ; i<3 ; i++)
    {
    if (!attachment[i][0])
      break;

    fh = open(attachment[i], O_RDONLY | O_BINARY);
    if (fh < 0)
      {
      printf("Attachment file \"%s\" does not exist.\n", attachment[i]);
      cm_disconnect_experiment();
      return 0;
      }

    att_size[i] = lseek(fh, 0, SEEK_END);
    lseek(fh, 0, SEEK_SET);

    buffer[i] = malloc(att_size[i]);
    if (buffer[i] == NULL || att_size[i] > 500*1024)
      {
      printf("Attachment file \"%s\" is too long (500k max).\n", attachment[i]);
      cm_disconnect_experiment();
      return 0;
      }

    n = read(fh, buffer[i], att_size[i]);
    if (n < att_size[i])
      {
      printf("Cannot fully read attachment file \"%s\".\n", attachment[i]);
      cm_disconnect_experiment();
      return 0;
      }

    close(fh);
    }

  /* add local host name to author */
  gethostname(lhost_name, sizeof(host_name));

  phe = gethostbyname(lhost_name);
  if (phe == NULL)
    {
    printf("Cannot retrieve local host name\n");
    cm_disconnect_experiment();
    return 0;
    }
  phe = gethostbyaddr(phe->h_addr, sizeof(int), AF_INET);
  if (phe == NULL)
    {
    printf("Cannot retrieve local host name\n");
    cm_disconnect_experiment();
    return 0;
    }
  strcpy(lhost_name, phe->h_name);
  strcat(author, "@");
  strcat(author, lhost_name);

  /* now submit message */
  str[0] = 0;
  el_submit(0, author, type, system, subject, text, "", "plain", 
            attachment[0], buffer[0], att_size[0], 
            attachment[1], buffer[1], att_size[1], 
            attachment[2], buffer[2], att_size[2], 
            str, sizeof(str));

  for (i=0 ; i<3 ; i++)
    if (buffer[i])
      free(buffer[i]);

  cm_disconnect_experiment();

  return 1;
}

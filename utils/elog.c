 /********************************************************************\

  Name:         elog.c
  Created by:   Stefan Ritt

  Contents:     Electronic logbook utility   

  $Log$
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
  "Complaints",
  "Reply",
  "Test",
  "Other"
};

char system_list[20][NAME_LENGTH] = {
  "General",
  "DAQ",
  "Detector",
  "Electronics",
  "Target",
};

HNDLE hDB;

/*------------------------------------------------------------------*/

INT query_params(char *author, char *type, char *system, char *subject, 
                 char *text, char *attachment)
{
char str[1000];
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
    printf("Select a message type from following list:\n");
    for (i=0 ; i<20 && type_list[i][0] ; i++)
      if (type_list[i+1][0])
        printf("%s,", type_list[i]);
      else
        printf("%s\n", type_list[i]);

    ss_gets(type, 80);
    }

  if (!system[0])
    {
    printf("Select a system from following list:\n");
    for (i=0 ; i<20 && system_list[i][0] ; i++)
      if (system_list[i+1][0])
        printf("%s,", system_list[i]);
      else
        printf("%s\n", system_list[i]);

    ss_gets(system, 80);
    }

  if (!subject[0])
    {
    printf("Subject: ");
    ss_gets(subject, 256);
    }

  if (!text[0])
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

  if (!attachment[0])
    {
    do
      {
      printf("Optional file attachment: ");
      ss_gets(attachment, 256);
      if (!attachment[0])
        break;
      f = fopen(attachment, "r");
      if (f == NULL)
        printf("File does not exist!\n");
      else
        fclose(f);
      } while (f == NULL);
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
char      author[80], type[80], system[80], subject[256], text[10000], attachment[256];
char      host_name[256], exp_name[NAME_LENGTH], str[256], dir[256], file_name[256], *p;
char      buffer[10000];
INT       i, size, status, run_number, read, written;
DWORD     now;
struct tm *tms;
HNDLE     hkey;
FILE      *f1, *f2;

  /* turn off system message */
  cm_set_msg_print(0, MT_ALL, puts);

  author[0] = type[0] = system[0] = subject[0] = text[0] = attachment[0] = 0;
  host_name[0] = exp_name[0] = 0;

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
        strcpy(attachment, argv[++i]);
      else
        {
usage:
        printf("\nusage: elog [-h hostname] [-e experiment]\n");
        printf("          -a <author> -t <type> -s <system> -b <subject>\n");
        printf("          -f <attachment> <text>\n");
        printf("\nArguments with blanks must be enclosed in quotes\n");
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
  status = query_params(author, type, system, subject, text, attachment);
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

  /* check if attachment exists */
  if (attachment[0])
    {
    f1 = fopen(attachment, "r");
    if (f1 == NULL)
      {
      printf("Attachment file \"%s\" does not exist.\n", attachment);
      cm_disconnect_experiment();
      return 0;
      }
    else
      fclose(f1);
    }

  /* get run number */
  size = sizeof(run_number);
  db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT);

  /*---- copy attachment file ----*/

  /* strip path from filename */
  strcpy(str, attachment);
  p = str;
  while (strchr(p, '\\'))
    p = strchr(p, '\\')+1; /* NT */
  while (strchr(p, '/'))
    p = strchr(p, '/')+1;  /* Unix */
  while (strchr(p, ']'))
    p = strchr(p, ']')+1;  /* VMS */

  /* assemble ELog filename */
  if (p[0])
    {
    f1 = fopen(attachment, "rb");
    
    dir[0] = 0;
    size = sizeof(dir);
    memset(dir, 0, size);
    db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
    if (dir[0] != 0)
      if (dir[strlen(dir)-1] != DIR_SEPARATOR)
        strcat(dir, DIR_SEPARATOR_STR);

#if !defined(OS_VXWORKS) 
#if !defined(OS_VMS)
    tzset();
#endif
#endif
  
    time(&now);
    tms = localtime(&now);

    sprintf(file_name, "%02d%02d%02d_%02d%02d%02d_%s",
            tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
            tms->tm_hour, tms->tm_min, tms->tm_sec, p);
    strcpy(attachment, file_name);
    sprintf(file_name, "%s%02d%02d%02d_%02d%02d%02d_%s", dir, 
            tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
            tms->tm_hour, tms->tm_min, tms->tm_sec, p);

    /* write to destination */
    f2 = fopen(file_name, "wb");

    while (!feof(f1))
      {
      read = fread(buffer, 1, sizeof(buffer), f1);
      written = fwrite(buffer, 1, read, f2);

      if (written != read)
        {
        printf("File \"%s\" could not be written.\n", file_name);
        cm_disconnect_experiment();
        return 0;
        }
      }

    fclose(f1);
    fclose(f2);
    }

  /* now submit message */
  el_submit(run_number, author, type, system, subject, text, "", "plain", attachment, NULL);

  cm_disconnect_experiment();

  return 1;
}

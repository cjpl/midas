/********************************************************************\

  Name:         elog.c
  Created by:   Stefan Ritt

  Contents:     Electronic logbook utility   

  $Log$
  Revision 1.12  2000/09/01 15:49:26  midas
  Elog is now submitted via mhttpd

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
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <windows.h>
#include <io.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#define closesocket(s) close(s)
#define O_BINARY 0
#endif

typedef int INT;

char type_list[20][32] = {
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

char system_list[20][32] = {
  "General",
  "DAQ",
  "Detector",
  "Electronics",
  "Target",
  "Beamline"
};

/*------------------------------------------------------------------*/

void sgets(char *string, int size)
{
char *p;

  do
    {
    p = fgets(string, size, stdin);
    } while (p == NULL);

  if (strlen(p) > 0 && p[strlen(p)-1] == '\n')
    p[strlen(p)-1] = 0;
}

/*------------------------------------------------------------------*/

INT query_params(char *host_name, int *port, char *author, char *type, char *syst, 
                 char *subject, char *text, char *textfile, char attachment[3][256])
{
char str[1000], tmpfile[256];
FILE *f;
int  i, query_attachment;

  if (!host_name[0])
    {
    while (!host_name[0])
      {
      printf("Host name: ");
      sgets(host_name, 80);
      if (!host_name[0])
        printf("Author must be supplied!\n");
      }

    printf("Port [80]: ");
    sgets(str, 80);
    if (str[0])
      *port = atoi(str);
    }

  query_attachment = (author[0] == 0 && attachment[0][0] == 0);

  while (!author[0])
    {
    printf("Author: ");
    sgets(author, 80);
    if (!author[0])
      printf("Author must be supplied!\n");
    }

  if (!type[0])
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

    sgets(type, 80);
    }

  if (!syst[0])
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

    sgets(syst, 80);
    }

  if (!subject[0])
    {
    printf("Subject: ");
    sgets(subject, 256);
    }

  if (!text[0] && !textfile[0])
    {
    if (getenv("EDITOR"))
      {
      sprintf(tmpfile, "tmp.txt");
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
        sgets(str, 1000);
        if (str[0])
          {
          strcat(text, str);
          strcat(text, "\n");
          }
        } while (str[0]);
      }
    }

  if (query_attachment)
    {
    for (i=0 ; i<3 ; i++)
      {
      do
        {
        printf("Optional file attachment %d: ", i+1);
        sgets(attachment[i], 256);
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

  return 1;
}

/*------------------------------------------------------------------*/

char request[600000], response[10000], content[600000];

INT submit_elog(char *host, int port, char *experiment, char *passwd,
                char *author, char *type, char *system, char *subject, 
                char *text,
                char *afilename1, char *buffer1, INT buffer_size1, 
                char *afilename2, char *buffer2, INT buffer_size2, 
                char *afilename3, char *buffer3, INT buffer_size3)
/********************************************************************\

  Routine: submit_elog

  Purpose: Submit an ELog entry

  Input:
    char   *host            Host name where ELog server runs
    in     port             ELog server port number
    char   *passwd          Web password
    int    run              Run number
    char   *author          Message author
    char   *type            Message type
    char   *system          Message system
    char   *subject         Subject
    char   *text            Message text

    char   *afilename1/2/3  File name of attachment
    char   *buffer1/2/3     File contents
    INT    *buffer_size1/2/3 Size of buffer in bytes

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
int                  status, sock, i, n, header_length, content_length;
struct hostent       *phe;
struct sockaddr_in   bind_addr;
char                 host_name[256], boundary[80], *p;

#if defined( _MSC_VER )
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return -1;
  }
#endif

  /* get local host name */
  gethostname(host_name, sizeof(host_name));

  phe = gethostbyname(host_name);
  if (phe == NULL)
    {
    perror("Cannot retrieve host name");
    return -1;
    }
  phe = gethostbyaddr(phe->h_addr, sizeof(int), AF_INET);
  if (phe == NULL)
    {
    perror("Cannot retrieve host name");
    return -1;
    }

  /* if domain name is not in host name, hope to get it from phe */
  if (strchr(host_name, '.') == NULL)
    strcpy(host_name, phe->h_name);

  /* create socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
    perror("cannot create socket");
    return -1;
    }

  /* compose remote address */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = htons((unsigned short)port);

  phe = gethostbyname(host);
  if (phe == NULL)
    {
    perror("cannot get host name");
    return -1;
    }
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);

  /* connect to server */
  status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));
  if (status != 0)
    {
    printf("Cannot connect to host %s, port %d\n", host, port);
    return -1;
    }

  /* compose content */
  strcpy(boundary, "---------------------------7d0bf1a60904bc");
  strcpy(content, boundary);
  strcat(content, "\r\nContent-Disposition: form-data; name=\"cmd\"\r\n\r\nSubmit\r\n");

  if (experiment[0])
    sprintf(content+strlen(content), 
            "%s\r\nContent-Disposition: form-data; name=\"exp\"\r\n\r\n%s\r\n", boundary, experiment);
  
  sprintf(content+strlen(content), 
          "%s\r\nContent-Disposition: form-data; name=\"Author\"\r\n\r\n%s\r\n", boundary, author);
  sprintf(content+strlen(content), 
          "%s\r\nContent-Disposition: form-data; name=\"type\"\r\n\r\n%s\r\n", boundary, type);
  sprintf(content+strlen(content), 
          "%s\r\nContent-Disposition: form-data; name=\"system\"\r\n\r\n%s\r\n", boundary, system);
  sprintf(content+strlen(content), 
          "%s\r\nContent-Disposition: form-data; name=\"Subject\"\r\n\r\n%s\r\n", boundary, subject);
  sprintf(content+strlen(content), 
          "%s\r\nContent-Disposition: form-data; name=\"Text\"\r\n\r\n%s\r\n%s\r\n", boundary, text, boundary);

  content_length = strlen(content);
  p = content+content_length;

  if (afilename1[0])
    {
    sprintf(p, "Content-Disposition: form-data; name=\"attfile1\"; filename=\"%s\"\r\n\r\n", 
            afilename1);

    content_length += strlen(p);
    p += strlen(p);
    memcpy(p, buffer1, buffer_size1);
    p += buffer_size1;
    strcpy(p, boundary);
    strcat(p, "\r\n");

    content_length += buffer_size1 + strlen(p);
    p += strlen(p);
    }
  
  if (afilename2[0])
    {
    sprintf(p, "Content-Disposition: form-data; name=\"attfile2\"; filename=\"%s\"\r\n\r\n", 
            afilename2);

    content_length += strlen(p);
    p += strlen(p);
    memcpy(p, buffer2, buffer_size2);
    p += buffer_size2;
    strcpy(p, boundary);
    strcat(p, "\r\n");

    content_length += buffer_size2 + strlen(p);
    p += strlen(p);
    }

  if (afilename3[0])
    {
    sprintf(p, "Content-Disposition: form-data; name=\"attfile3\"; filename=\"%s\"\r\n\r\n", 
            afilename3);

    content_length += strlen(p);
    p += strlen(p);
    memcpy(p, buffer3, buffer_size3);
    p += buffer_size3;
    strcpy(p, boundary);
    strcat(p, "\r\n");

    content_length += buffer_size3 + strlen(p);
    p += strlen(p);
    }

  /* compose request */
  sprintf(request, "POST / HTTP/1.0\r\n");
  sprintf(request+strlen(request), "Content-Type: multipart/form-data; boundary=%s\r\n", boundary);
  sprintf(request+strlen(request), "Host: %s\r\n", host_name);
  sprintf(request+strlen(request), "Content-Length: %d\r\n", content_length);

  if (passwd[0])
    sprintf(request+strlen(request), "Cookie: midas_wpwd: %s\r\n", content_length);

  strcat(request, "\r\n");

  header_length = strlen(request);
  memcpy(request+header_length, content, content_length);

    {
    FILE *f;
    f = fopen("elog.log", "w");
    fwrite(request, header_length+content_length, 1, f);
    fclose(f);
    }

  /* send request */
  send(sock, request, header_length+content_length, 0);

  /* receive response */
  i = recv(sock, response, 10000, 0);
  if (i < 0)
    {
    perror("Cannot receive response");
    return -1;
    }  

  /* discard remainder of response */
  n = i;
  while (i > 0)
    {
    i = recv(sock, response+n, 10000, 0);
    if (i > 0)
      n += i;
    }
  response[n] = 0;

  closesocket(sock);

  /* check response status */
  if (strstr(response, "302 Found"))
    printf("Message successfully transmitted\n");
  else
    printf("Error transmitting message\n");

  return 1;
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
char      author[80], type[80], system[80], subject[256], text[10000];
char      host_name[256], exp_name[32], textfile[256];
char      *buffer[3], attachment[3][256];
INT       att_size[3];
INT       i, n, status, fh, n_att, size, port;

  author[0] = type[0] = system[0] = subject[0] = text[0] = textfile[0] = 0;
  host_name[0] = exp_name[0] = n_att = 0;
  port = 80;
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
      else if (argv[i][1] == 'p')
        port = atoi(argv[++i]);
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
        printf("\nusage: elog -h <hostname> [-p port] [-e experiment]\n");
        printf("          -a <author> -t <type> -s <system> -b <subject>\n");
        printf("          [-f <attachment>] [-m <textfile>|<text>]\n");
        printf("\nArguments with blanks must be enclosed in quotes\n");
        printf("The elog message can either be submitted on the command line\n");
        printf("or in a file with the -m flag.\n");
        return 0;
        }
      }
    else
      strcpy(text, argv[i]);
    }

  /* complete missing parameters */
  status = query_params(host_name, &port, author, type, system, subject, 
                        text, textfile, attachment);
  if (status != 1)
    return 0;

  /*---- open text file ----*/

  if (textfile[0])
    {
    fh = open(textfile, O_RDONLY | O_BINARY);
    if (fh < 0)
      {
      printf("Message file \"%s\" does not exist.\n", textfile);
      return 0;
      }

    size = lseek(fh, 0, SEEK_END);
    lseek(fh, 0, SEEK_SET);

    if (size > sizeof(text))
      {
      printf("Message file \"%s\" is too long (%d bytes max).\n", sizeof(text));
      return 0;
      }

    i = read(fh, text, size);
    if (i < size)
      {
      printf("Cannot fully read message file \"%s\".\n", textfile);
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
      return 0;
      }

    att_size[i] = lseek(fh, 0, SEEK_END);
    lseek(fh, 0, SEEK_SET);

    buffer[i] = malloc(att_size[i]);
    if (buffer[i] == NULL || att_size[i] > 500*1024)
      {
      printf("Attachment file \"%s\" is too long (500k max).\n", attachment[i]);
      return 0;
      }

    n = read(fh, buffer[i], att_size[i]);
    if (n < att_size[i])
      {
      printf("Cannot fully read attachment file \"%s\".\n", attachment[i]);
      return 0;
      }

    close(fh);
    }

  /* now submit message */
  submit_elog(host_name, port, exp_name, "", author, type, system, subject, text,
             attachment[0], buffer[0], att_size[0], 
             attachment[1], buffer[1], att_size[1], 
             attachment[2], buffer[2], att_size[2]);

  for (i=0 ; i<3 ; i++)
    if (buffer[i])
      free(buffer[i]);

  return 1;
}

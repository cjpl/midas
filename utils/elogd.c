/********************************************************************\

  Name:         elogd.c
  Created by:   Stefan Ritt

  Contents:     Web server program for Electronic Logbook ELOG

  $Log$
  Revision 1.10  2001/06/15 09:13:30  midas
  Display "<" and ">" correctly

  Revision 1.9  2001/06/15 08:49:19  midas
  Fixed bug when query gave no results if no message from yesterday

  Revision 1.8  2001/05/23 13:15:34  midas
  Fixed bug with category

  Revision 1.7  2001/05/23 13:06:03  midas
  Added "allow edit" functionality

  Revision 1.6  2001/05/23 10:48:48  midas
  Added version

  Revision 1.5  2001/05/23 08:16:41  midas
  Fixed bug when POST request comes in two blocks

  Revision 1.4  2001/05/22 11:32:59  midas
  Added Help button

  Revision 1.3  2001/05/22 09:13:47  midas
  Rearranged configuration file

  Revision 1.2  2001/05/21 15:28:45  midas
  Implemented multiple logbooks

  Revision 1.1  2001/05/18 13:31:33  midas
  Initial revision

\********************************************************************/

/* Version of ELOG */
#define VERSION "1.0.0"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define OS_WINNT

#define DIR_SEPARATOR '\\'

#include <windows.h>
#include <io.h>
#include <time.h>
#else

#define OS_UNIX

#define TRUE 1
#define FALSE 0

#define DIR_SEPARATOR '/'

typedef int BOOL;
typedef unsigned long int DWORD;

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define closesocket(s) close(s)
#define O_BINARY 0
#endif

typedef int INT;

#define TELL(fh) lseek(fh, 0, SEEK_CUR)

#define NAME_LENGTH  32
#define SUCCESS       1

#define EL_SUCCESS    1
#define EL_FIRST_MSG  2
#define EL_LAST_MSG   3
#define EL_FILE_ERROR 4

#define WEB_BUFFER_SIZE 1000000

char return_buffer[WEB_BUFFER_SIZE];
int  return_length;
char elogd_url[256];
char loogbook[32];
char logbook[32];
char data_dir[256];
char cfg_file[256];

#define MAX_GROUPS    32
#define MAX_PARAM    100
#define VALUE_SIZE   256
#define PARAM_LENGTH 256
#define TEXT_SIZE   4096

char _param[MAX_PARAM][PARAM_LENGTH];
char _value[MAX_PARAM][VALUE_SIZE];
char _text[TEXT_SIZE];
char *_attachment_buffer[3];
INT  _attachment_size[3];
struct in_addr remote_addr;
INT  _sock;
BOOL verbose;

char *mname[] = {
  "January",
  "February",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
};

char type_list[50][NAME_LENGTH] = {
  "Routine",
  "Other"
};

char category_list[50][NAME_LENGTH] = {
  "General",
  "Other",
};


struct {
  char ext[32];
  char type[32];
  } filetype[] = {

  ".JPG",   "image/jpeg",     
  ".GIF",   "image/gif",
  ".PS",    "application/postscript",
  ".EPS",   "application/postscript",
  ".HTML",  "text/html",
  ".HTM",   "text/html",
  ".XLS",   "application/x-msexcel",
  ".DOC",   "application/msword",
  ".PDF",   "application/pdf",
  ".TXT",   "text/plain",
  ".ASC",   "text/plain",
  ".ZIP",   "application/x-zip-compressed",
  ""
};

/*---- Funcions from the MIDAS library -----------------------------*/

BOOL equal_ustring(char *str1, char *str2)
{
  if (str1 == NULL && str2 != NULL)
    return FALSE;
  if (str1 != NULL && str2 == NULL)
    return FALSE;
  if (str1 == NULL && str2 == NULL)
    return TRUE;

  while (*str1)
    if (toupper(*str1++) != toupper(*str2++))
      return FALSE;

  if (*str2)
    return FALSE;

  return TRUE;
}

/*-------------------------------------------------------------------*/

char *map= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int cind(char c)
{
int i;

  if (c == '=')
    return 0;

  for (i=0 ; i<64 ; i++)
    if (map[i] == c)
      return i;

  return -1;
}

void base64_decode(char *s, char *d)
{
unsigned int t;

  while (*s)
    {
    t = (cind(*s++) << 18) |
        (cind(*s++) << 12) |
        (cind(*s++) << 6)  |
        (cind(*s++) << 0);
    
    *(d+2) = (char) (t & 0xFF);
    t >>= 8;
    *(d+1) = (char) (t & 0xFF);
    t >>= 8;
    *d = (char) (t & 0xFF);

    d += 3;
    }
  *d = 0;
}

void base64_encode(char *s, char *d)
{
unsigned int t, pad;

  pad = 3 - strlen(s) % 3;
  while (*s)
    {
    t =   (*s++) << 16;
    if (*s)
      t |=  (*s++) << 8;
    if (*s)
      t |=  (*s++) << 0;
    
    *(d+3) = map[t & 63];
    t >>= 6;
    *(d+2) = map[t & 63];
    t >>= 6;
    *(d+1) = map[t & 63];
    t >>= 6;
    *(d+0) = map[t & 63];

    d += 4;
    }
  *d = 0;
  while (pad--)
    *(--d) = '=';
}

/*-------------------------------------------------------------------*/

INT ss_daemon_init()
{
#ifdef OS_UNIX

  /* only implemented for UNIX */
  int i, fd, pid;

  if ( (pid = fork()) < 0)
    return 0;
  else if (pid != 0)
    exit(0); /* parent finished */

  /* child continues here */

  /* try and use up stdin, stdout and stderr, so other
     routines writing to stdout etc won't cause havoc. Copied from smbd */
  for (i=0 ; i<3 ; i++) 
    {
    close(i);
    fd = open("/dev/null", O_RDWR, 0);
    if (fd < 0) 
      fd = open("/dev/null", O_WRONLY, 0);
    if (fd < 0) 
      {
      printf("Can't open /dev/null");
      return;
      }
    if (fd != i)
      {
      printf("Did not get file descriptor");
      return;
      }
    }
  
  setsid();               /* become session leader */
  chdir("/");             /* change working direcotry (not on NFS!) */
  umask(0);               /* clear our file mode createion mask */

#endif

  return SUCCESS;
}

/*------------------------------------------------------------------*/

/* Parameter extraction from configuration file similar to win.ini */

char *cfgbuffer;

int getcfg(char *group, char *param, char *value)
{
char str[256], *p, *pstr;
int  length;
int  fh;

  value[0] = 0;

  /* read configuration file on init */
  if (!cfgbuffer)
    {
    if (cfgbuffer)
      free(cfgbuffer);

    fh = open(cfg_file, O_RDONLY | O_BINARY);
    if (fh < 0)
      return 0;
    length = lseek(fh, 0, SEEK_END);
    lseek(fh, 0, SEEK_SET);
    cfgbuffer = malloc(length+1);
    if (cfgbuffer == NULL)
      {
      close(fh);
      return 0;
      }
    read(fh, cfgbuffer, length);
    cfgbuffer[length] = 0;
    close(fh);
    }

  /* search group */
  p = cfgbuffer;
  do
    {
    if (*p == '[')
      {
      p++;
      pstr = str;
      while (*p && *p != ']' && *p != '\n')
        *pstr++ = *p++;
      *pstr = 0;
      if (equal_ustring(str, group))
        {
        /* search parameter */
        p = strchr(p, '\n');
        if (p)
          p++;
        while (p && *p && *p != '[')
          {
          pstr = str;
          while (*p && *p != '=' && *p != '\n')
            *pstr++ = *p++;
          *pstr-- = 0;
          while (pstr > str && (*pstr == ' ' || *pstr == '='))
            *pstr-- = 0;

          if (equal_ustring(str, param))
            {
            if (*p == '=')
              {
              p++;
              while (*p == ' ')
                p++;
              pstr = str;
              while (*p && *p != '\n' && *p != '\r')
                *pstr++ = *p++;
              *pstr-- = 0;
              while (*pstr == ' ')
                *pstr-- = 0;

              strcpy(value, str);
              return 1;
              }
            }

          if (p)
            p = strchr(p, '\n');
          if (p)
            p++;
          }
        }
      }
    if (p)
      p = strchr(p, '\n');
    if (p)
      p++;
    } while (p);

  return 0;
}

/*------------------------------------------------------------------*/

int enumcfg(char *group, char *param, char *value, int index)
{
char str[256], *p, *pstr;
int  i;

  /* open configuration file */
  if (!cfgbuffer)
    getcfg("dummy", "dummy", str);
  if (!cfgbuffer)
    return 0;

  /* search group */
  p = cfgbuffer;
  do
    {
    if (*p == '[')
      {
      p++;
      pstr = str;
      while (*p && *p != ']' && *p != '\n' && *p != '\r')
        *pstr++ = *p++;
      *pstr = 0;
      if (equal_ustring(str, group))
        {
        /* enumerate parameters */
        i = 0;
        p = strchr(p, '\n');
        if (p)
          p++;
        while (p && *p && *p != '[')
          {
          pstr = str;
          while (*p && *p != '=' && *p != '\n' && *p != '\r')
            *pstr++ = *p++;
          *pstr-- = 0;
          while (pstr > str && (*pstr == ' ' || *pstr == '='))
            *pstr-- = 0;

          if (i == index)
            {
            strcpy(param, str);
            if (*p == '=')
              {
              p++;
              while (*p == ' ')
                p++;
              pstr = str;
              while (*p && *p != '\n' && *p != '\r')
                *pstr++ = *p++;
              *pstr-- = 0;
              while (*pstr == ' ')
                *pstr-- = 0;

              if (value)
                strcpy(value, str);
              }
            return 1;
            }

          if (p)
            p = strchr(p, '\n');
          if (p)
            p++;
          i++;
          }
        }
      }
    if (p)
      p = strchr(p, '\n');
    if (p)
      p++;
    } while (p);

  return 0;
}

/*------------------------------------------------------------------*/

int enumgrp(int index, char *group)
{
char str[256], *p, *pstr;
int  i;

  /* open configuration file */
  if (!cfgbuffer)
    getcfg("dummy", "dummy", str);
  if (!cfgbuffer)
    return 0;

  /* search group */
  p = cfgbuffer;
  i = 0;
  do
    {
    if (*p == '[')
      {
      p++;
      pstr = str;
      while (*p && *p != ']' && *p != '\n' && *p != '\r')
        *pstr++ = *p++;
      *pstr = 0;

      if (i == index)
        {
        strcpy(group, str);
        return 1;
        }

      i++;
      }

    if (p)
      p = strchr(p, '\n');
    if (p)
      p++;
    } while (p);

  return 0;
}

/*------------------------------------------------------------------*/

void el_decode(char *message, char *key, char *result)
{
char *pc;

  if (result == NULL)
    return;

  *result = 0;

  if (strstr(message, key))
    {
    for (pc=strstr(message, key)+strlen(key) ; *pc != '\n' ; )
      *result++ = *pc++;
    *result = 0;
    }
}

/*------------------------------------------------------------------*/

INT el_search_message(char *tag, int *fh, BOOL walk)
{
int    i, size, offset, direction, last, status;
struct tm *tms, ltms;
DWORD  lt, ltime, lact;
char   str[256], file_name[256], dir[256];

  tzset();

  /* open file */
  strcpy(dir, data_dir);

  /* check tag for direction */
  direction = 0;
  if (strpbrk(tag, "+-"))
    {
    direction = atoi(strpbrk(tag, "+-"));
    *strpbrk(tag, "+-") = 0;
    }

  /* if tag is given, open file directly */
  if (tag[0])
    {
    /* extract time structure from tag */
    tms = &ltms;
    memset(tms, 0, sizeof(struct tm));
    tms->tm_year = (tag[0]-'0')*10 + (tag[1]-'0');
    tms->tm_mon  = (tag[2]-'0')*10 + (tag[3]-'0') -1;
    tms->tm_mday = (tag[4]-'0')*10 + (tag[5]-'0');
    tms->tm_hour = 12;

    if (tms->tm_year < 90)
      tms->tm_year += 100;
    ltime = lt = mktime(tms);

    strcpy(str, tag);
    if (strchr(str, '.'))
      {
      offset = atoi(strchr(str, '.')+1);
      *strchr(str, '.') = 0;
      }
    else
      return -1;

    do
      {
      tms = localtime(&ltime);

      sprintf(file_name, "%s%02d%02d%02d.log", dir,
              tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);
      *fh = open(file_name, O_RDWR | O_BINARY, 0644);

      if (*fh < 0)
        {
        if (!walk)
          return EL_FILE_ERROR;

        if (direction == -1)
          ltime -= 3600*24; /* one day back */
        else
          ltime += 3600*24; /* go forward one day */

        /* set new tag */
        tms = localtime(&ltime);
        sprintf(tag, "%02d%02d%02d.0", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);
        }

      /* in forward direction, stop today */
      if (direction != -1 && ltime > (DWORD)time(NULL)+3600*24)
        break;

      /* in backward direction, goe back 10 years */
      if (direction == -1 && abs((INT)lt-(INT)ltime) > 3600*24*365*10)
        break;

      } while (*fh < 0);

    if (*fh < 0)
      return EL_FILE_ERROR;

    lseek(*fh, offset, SEEK_SET);

    /* check if start of message */
    i = read(*fh, str, 15);
    if (i <= 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    if (strncmp(str, "$Start$: ", 9) != 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    lseek(*fh, offset, SEEK_SET);
    }

  /* open most recent file if no tag given */
  if (tag[0] == 0)
    {
    time(&lt);
    ltime = lt;
    do
      {
      tms = localtime(&ltime);

      sprintf(file_name, "%s%02d%02d%02d.log", dir,
              tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);
      *fh = open(file_name, O_RDWR | O_BINARY, 0644);

      if (*fh < 0)
        ltime -= 3600*24; /* one day back */

      } while (*fh < 0 && (INT)lt-(INT)ltime < 3600*24*365);

    if (*fh < 0)
      return EL_FILE_ERROR;

    /* remember tag */
    sprintf(tag, "%02d%02d%02d", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

    lseek(*fh, 0, SEEK_END);

    sprintf(tag+strlen(tag), ".%d", TELL(*fh));
    }


  if (direction == -1)
    {
    /* seek previous message */

    if (TELL(*fh) == 0)
      {
      /* go back one day */
      close(*fh);

      lt = ltime;
      do
        {
        lt -= 3600*24;
        tms = localtime(&lt);
        sprintf(str, "%02d%02d%02d.0",
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

        status = el_search_message(str, fh, FALSE);

        } while (status != SUCCESS &&
                 (INT)ltime-(INT)lt < 3600*24*365);

      if (status != EL_SUCCESS)
        return EL_FIRST_MSG;

      /* adjust tag */
      strcpy(tag, str);

      /* go to end of current file */
      lseek(*fh, 0, SEEK_END);
      }

    /* read previous message size */
    lseek(*fh, -17, SEEK_CUR);
    i = read(*fh, str, 17);
    if (i <= 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    if (strncmp(str, "$End$: ", 7) == 0)
      {
      size = atoi(str+7);
      lseek(*fh, -size, SEEK_CUR);
      }
    else
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    /* adjust tag */
    sprintf(strchr(tag, '.')+1, "%d", TELL(*fh));
    }

  if (direction == 1)
    {
    /* seek next message */

    /* read current message size */
    last = TELL(*fh);

    i = read(*fh, str, 15);
    if (i <= 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }
    lseek(*fh, -15, SEEK_CUR);

    if (strncmp(str, "$Start$: ", 9) == 0)
      {
      size = atoi(str+9);
      lseek(*fh, size, SEEK_CUR);
      }
    else
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    /* if EOF, goto next day */
    i = read(*fh, str, 15);
    if (i < 15)
      {
      close(*fh);
      time(&lact);

      lt = ltime;
      do
        {
        lt += 3600*24;
        tms = localtime(&lt);
        sprintf(str, "%02d%02d%02d.0",
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

        status = el_search_message(str, fh, FALSE);

        } while (status != EL_SUCCESS &&
                 (INT)lt-(INT)lact < 3600*24);

      if (status != EL_SUCCESS)
        return EL_LAST_MSG;

      /* adjust tag */
      strcpy(tag, str);

      /* go to beginning of current file */
      lseek(*fh, 0, SEEK_SET);
      }
    else
      lseek(*fh, -15, SEEK_CUR);

    /* adjust tag */
    sprintf(strchr(tag, '.')+1, "%d", TELL(*fh));
    }

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_submit(int run, char *author, char *type, char *category, char *subject,
              char *text, char *reply_to, char *encoding,
              char *afilename1, char *buffer1, INT buffer_size1,
              char *afilename2, char *buffer2, INT buffer_size2,
              char *afilename3, char *buffer3, INT buffer_size3,
              char *tag, INT tag_size)
/********************************************************************\

  Routine: el_submit

  Purpose: Submit an ELog entry

  Input:
    int    run              Run number
    char   *author          Message author
    char   *type            Message type
    char   *category        Message category
    char   *subject         Subject
    char   *text            Message text
    char   *reply_to        In reply to this message
    char   *encoding        Text encoding, either HTML or plain

    char   *afilename1/2/3  File name of attachment
    char   *buffer1/2/3     File contents
    INT    *buffer_size1/2/3 Size of buffer in bytes
    char   *tag             If given, edit existing message
    INT    *tag_size        Maximum size of tag

  Output:
    char   *tag             Message tag in the form YYMMDD.offset
    INT    *tag_size        Size of returned tag

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
INT     n, size, fh, status, run_number, buffer_size, index, offset, tail_size;
struct  tm *tms;
char    afilename[256], file_name[256], afile_name[3][256], dir[256], str[256],
        start_str[80], end_str[80], last[80], date[80], thread[80], attachment[256];
time_t  now;
char    message[10000], *p, *buffer;
BOOL    bedit;

  bedit = (tag[0] != 0);

  /* ignore run number */
  run_number = 0;

  for (index = 0 ; index < 3 ; index++)
    {
    /* generate filename for attachment */
    afile_name[index][0] = file_name[0] = 0;

    if (index == 0)
      {
      strcpy(afilename, afilename1);
      buffer = buffer1;
      buffer_size = buffer_size1;
      }
    else if (index == 1)
      {
      strcpy(afilename, afilename2);
      buffer = buffer2;
      buffer_size = buffer_size2;
      }
    else if (index == 2)
      {
      strcpy(afilename, afilename3);
      buffer = buffer3;
      buffer_size = buffer_size3;
      }

    if (afilename[0])
      {
      strcpy(file_name, afilename);
      p = file_name;
      while (strchr(p, ':'))
        p = strchr(p, ':')+1;
      while (strchr(p, '\\'))
        p = strchr(p, '\\')+1; /* NT */
      while (strchr(p, '/'))
        p = strchr(p, '/')+1;  /* Unix */
      while (strchr(p, ']'))
        p = strchr(p, ']')+1;  /* VMS */

      /* assemble ELog filename */
      if (p[0])
        {
        strcpy(dir, data_dir);

#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
        tzset();
#endif
#endif

        time(&now);
        tms = localtime(&now);

        strcpy(str, p);
        sprintf(afile_name[index], "%02d%02d%02d_%02d%02d%02d_%s",
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
                tms->tm_hour, tms->tm_min, tms->tm_sec, str);
        sprintf(file_name, "%s%02d%02d%02d_%02d%02d%02d_%s", dir,
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
                tms->tm_hour, tms->tm_min, tms->tm_sec, str);

        /* save attachment */
        fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
        if (fh < 0)
          {
          printf("Cannot write attachment file \"%s\"", file_name);
          }
        else
          {
          write(fh, buffer, buffer_size);
          close(fh);
          }
        }
      }
    }

  /* generate new file name YYMMDD.log in data directory */
  strcpy(dir, data_dir);

#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
  tzset();
#endif
#endif

  if (bedit)
    {
    /* edit existing message */
    strcpy(str, tag);
    if (strchr(str, '.'))
      {
      offset = atoi(strchr(str, '.')+1);
      *strchr(str, '.') = 0;
      }
    sprintf(file_name, "%s%s.log", dir, str);
    fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
    if (fh < 0)
      return -1;

    lseek(fh, offset, SEEK_SET);
    read(fh, str, 16);
    size = atoi(str+9);
    read(fh, message, size);

    el_decode(message, "Date: ", date);
    el_decode(message, "Thread: ", thread);
    el_decode(message, "Attachment: ", attachment);

    /* buffer tail of logfile */
    lseek(fh, 0, SEEK_END);
    tail_size = TELL(fh) - (offset+size);

    if (tail_size > 0)
      {
      buffer = malloc(tail_size);
      if (buffer == NULL)
        {
        close(fh);
        return -1;
        }

      lseek(fh, offset+size, SEEK_SET);
      n = read(fh, buffer, tail_size);
      }
    lseek(fh, offset, SEEK_SET);
    }
  else
    {
    /* create new message */
    time(&now);
    tms = localtime(&now);

    sprintf(file_name, "%s%02d%02d%02d.log", dir,
            tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

    fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
    if (fh < 0)
      return -1;

    strcpy(date, ctime(&now));
    date[24] = 0;

    if (reply_to[0])
      sprintf(thread, "%16s %16s", reply_to, "0");
    else
      sprintf(thread, "%16s %16s", "0", "0");

    lseek(fh, 0, SEEK_END);
    }

  /* compose message */

  sprintf(message, "Date: %s\n", date);
  sprintf(message+strlen(message), "Thread: %s\n", thread);
  sprintf(message+strlen(message), "Run: %d\n", run_number);
  sprintf(message+strlen(message), "Author: %s\n", author);
  sprintf(message+strlen(message), "Type: %s\n", type);
  sprintf(message+strlen(message), "Category: %s\n", category);
  sprintf(message+strlen(message), "Subject: %s\n", subject);

  /* keep original attachment if edit and no new attachment */
  if (bedit && afile_name[0][0] == 0 && afile_name[1][0] == 0 &&
                afile_name[2][0] == 0)
    sprintf(message+strlen(message), "Attachment: %s", attachment);
  else
    {
    sprintf(message+strlen(message), "Attachment: %s", afile_name[0]);
    if (afile_name[1][0])
      sprintf(message+strlen(message), ",%s", afile_name[1]);
    if (afile_name[2][0])
      sprintf(message+strlen(message), ",%s", afile_name[2]);
    }
  sprintf(message+strlen(message), "\n");

  sprintf(message+strlen(message), "Encoding: %s\n", encoding);
  sprintf(message+strlen(message), "========================================\n");
  strcat(message, text);

  size = 0;
  sprintf(start_str, "$Start$: %6d\n", size);
  sprintf(end_str,   "$End$:   %6d\n\f", size);

  size = strlen(message)+strlen(start_str)+strlen(end_str);

  if (tag != NULL && !bedit)
    sprintf(tag, "%02d%02d%02d.%d", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday, TELL(fh));

  sprintf(start_str, "$Start$: %6d\n", size);
  sprintf(end_str,   "$End$:   %6d\n\f", size);

  write(fh, start_str, strlen(start_str));
  write(fh, message, strlen(message));
  write(fh, end_str, strlen(end_str));

  if (bedit)
    {
    if (tail_size > 0)
      {
      n = write(fh, buffer, tail_size);
      free(buffer);
      }

    /* truncate file here */
#ifdef _MSC_VER
    chsize(fh, TELL(fh));
#else
    ftruncate(fh, TELL(fh));
#endif
    }

  close(fh);

  /* if reply, mark original message */
  if (reply_to[0] && !bedit)
    {
    strcpy(last, reply_to);
    do
      {
      status = el_search_message(last, &fh, FALSE);
      if (status == EL_SUCCESS)
        {
        /* position to next thread location */
        lseek(fh, 72, SEEK_CUR);
        memset(str, 0, sizeof(str));
        read(fh, str, 16);
        lseek(fh, -16, SEEK_CUR);

        /* if no reply yet, set it */
        if (atoi(str) == 0)
          {
          sprintf(str, "%16s", tag);
          write(fh, str, 16);
          close(fh);
          break;
          }
        else
          {
          /* if reply set, find last one in chain */
          strcpy(last, strtok(str, " "));
          close(fh);
          }
        }
      else
        /* stop on error */
        break;

      } while (TRUE);
    }

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_retrieve(char *tag, char *date, int *run, char *author, char *type,
                char *category, char *subject, char *text, int *textsize,
                char *orig_tag, char *reply_tag,
                char *attachment1, char *attachment2, char *attachment3,
                char *encoding)
/********************************************************************\

  Routine: el_retrieve

  Purpose: Retrieve an ELog entry by its message tab

  Input:
    char   *tag             tag in the form YYMMDD.offset
    int    *size            Size of text buffer

  Output:
    char   *tag             tag of retrieved message
    char   *date            Date/time of message recording
    int    *run             Run number
    char   *author          Message author
    char   *type            Message type
    char   *category        Message category
    char   *subject         Subject
    char   *text            Message text
    char   *orig_tag        Original message if this one is a reply
    char   *reply_tag       Reply for current message
    char   *attachment1/2/3 File attachment
    char   *encoding        Encoding of message
    int    *size            Actual message text size

  Function value:
    EL_SUCCESS              Successful completion
    EL_LAST_MSG             Last message in log

\********************************************************************/
{
int     size, fh, offset, search_status;
char    str[256], *p;
char    message[10000], thread[256], attachment_all[256];

  if (tag[0])
    {
    search_status = el_search_message(tag, &fh, TRUE);
    if (search_status != EL_SUCCESS)
      return search_status;
    }
  else
    {
    /* open most recent message */
    strcpy(tag, "-1");
    search_status = el_search_message(tag, &fh, TRUE);
    if (search_status != EL_SUCCESS)
      return search_status;
    }

  /* extract message size */
  offset = TELL(fh);
  read(fh, str, 16);
  size = atoi(str+9);
  read(fh, message, size);

  close(fh);

  /* decode message */
  if (strstr(message, "Run: ") && run)
    *run = atoi(strstr(message, "Run: ")+5);

  el_decode(message, "Date: ", date);
  el_decode(message, "Thread: ", thread);
  el_decode(message, "Author: ", author);
  el_decode(message, "Type: ", type);
  el_decode(message, "Category: ", category);
  el_decode(message, "Subject: ", subject);
  el_decode(message, "Attachment: ", attachment_all);
  el_decode(message, "Encoding: ", encoding);

  /* break apart attachements */
  if (attachment1 && attachment2 && attachment3)
    {
    attachment1[0] = attachment2[0] = attachment3[0] = 0;
    p = strtok(attachment_all, ",");
    if (p != NULL)
      strcpy(attachment1, p);
    p = strtok(NULL, ",");
    if (p != NULL)
      strcpy(attachment2, p);
    p = strtok(NULL, ",");
    if (p != NULL)
      strcpy(attachment3, p);
    }

  /* conver thread in reply-to and reply-from */
  if (orig_tag != NULL && reply_tag != NULL)
    {
    p = strtok(thread, " \r");
    if (p != NULL)
      strcpy(orig_tag, p);
    else
      strcpy(orig_tag, "");
    p = strtok(NULL, " \r");
    if (p != NULL)
      strcpy(reply_tag, p);
    else
      strcpy(reply_tag, "");
    if (atoi(orig_tag) == 0)
      orig_tag[0] = 0;
    if (atoi(reply_tag) == 0)
      reply_tag[0] = 0;
    }

  p = strstr(message, "========================================\n");

  if (text != NULL)
    {
    if (p != NULL)
      {
      p += 41;
      if ((int) strlen(p) >= *textsize)
        {
        strncpy(text, p, *textsize-1);
        text[*textsize-1] = 0;
        return -1;
        }
      else
        {
        strcpy(text, p);

        /* strip end tag */
        if (strstr(text, "$End$"))
          *strstr(text, "$End$") = 0;

        *textsize = strlen(text);
        }
      }
    else
      {
      text[0] = 0;
      *textsize = 0;
      }
    }

  if (search_status == EL_LAST_MSG)
    return EL_LAST_MSG;

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_search_run(int run, char *return_tag)
/********************************************************************\

  Routine: el_search_run

  Purpose: Find first message belonging to a specific run

  Input:
    int    run              Run number

  Output:
    char   *tag             tag of retrieved message

  Function value:
    EL_SUCCESS              Successful completion
    EL_LAST_MSG             Last message in log

\********************************************************************/
{
int     actual_run, fh, status;
char    tag[256];

  tag[0] = return_tag[0] = 0;

  do
    {
    /* open first message in file */
    strcat(tag, "-1");
    status = el_search_message(tag, &fh, TRUE);
    if (status == EL_FIRST_MSG)
      break;
    if (status != EL_SUCCESS)
      return status;
    close(fh);

    if (strchr(tag, '.') != NULL)
      strcpy(strchr(tag, '.'), ".0");

    el_retrieve(tag, NULL, &actual_run, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, NULL, NULL, NULL);
    } while (actual_run >= run);

  while (actual_run < run)
    {
    strcat(tag, "+1");
    status = el_search_message(tag, &fh, TRUE);
    if (status == EL_LAST_MSG)
      break;
    if (status != EL_SUCCESS)
      return status;
    close(fh);

    el_retrieve(tag, NULL, &actual_run, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, NULL, NULL, NULL);
    }

  strcpy(return_tag, tag);

  if (status == EL_LAST_MSG || status == EL_FIRST_MSG)
    return status;

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_delete_message(char *tag)
/********************************************************************\

  Routine: el_submit

  Purpose: Submit an ELog entry

  Input:
    char   *tag             Message tage

  Output:
    <none>

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
INT     n, size, fh, offset, tail_size;
char    dir[256], str[256], file_name[256];
char    *buffer;

  /* generate file name YYMMDD.log in data directory */
  strcpy(dir, data_dir);

  strcpy(str, tag);
  if (strchr(str, '.'))
    {
    offset = atoi(strchr(str, '.')+1);
    *strchr(str, '.') = 0;
    }
  sprintf(file_name, "%s%s.log", dir, str);
  fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
  if (fh < 0)
    return EL_FILE_ERROR;
  lseek(fh, offset, SEEK_SET);
  read(fh, str, 16);
  size = atoi(str+9);

  /* buffer tail of logfile */
  lseek(fh, 0, SEEK_END);
  tail_size = TELL(fh) - (offset+size);

  if (tail_size > 0)
    {
    buffer = malloc(tail_size);
    if (buffer == NULL)
      {
      close(fh);
      return EL_FILE_ERROR;
      }

    lseek(fh, offset+size, SEEK_SET);
    n = read(fh, buffer, tail_size);
    }
  lseek(fh, offset, SEEK_SET);

  if (tail_size > 0)
    {
    n = write(fh, buffer, tail_size);
    free(buffer);
    }

  /* truncate file here */
#ifdef OS_WINNT
  chsize(fh, TELL(fh));
#else
  ftruncate(fh, TELL(fh));
#endif

  /* if file length gets zero, delete file */
  tail_size = lseek(fh, 0, SEEK_END);
  close(fh);

  if (tail_size == 0)
    remove(file_name);

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

void rsputs(const char *str)
{
  if (strlen(return_buffer) + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    strcat(return_buffer, str);
}

/*------------------------------------------------------------------*/

void rsputs2(const char *str)
{
int i, j;

  if (strlen(return_buffer) + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    {
    j = strlen(return_buffer);
    for (i=0 ; i<(int)strlen(str) ; i++)
      switch (str[i])
        {
        case '<': strcat(return_buffer, "&lt;"); j+=4; break;
        case '>': strcat(return_buffer, "&gt;"); j+=4; break;
        default: return_buffer[j++] = str[i];
        }

    return_buffer[j] = 0;
    }  
}

/*------------------------------------------------------------------*/

void rsprintf(const char *format, ...)
{
va_list argptr;
char    str[10000];

  va_start(argptr, format);
  vsprintf(str, (char *) format, argptr);
  va_end(argptr);

  if (strlen(return_buffer) + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    strcat(return_buffer, str);
}

/*------------------------------------------------------------------*/

/* Parameter handling functions similar to setenv/getenv */

void initparam()
{
  memset(_param, 0, sizeof(_param));
  memset(_value, 0, sizeof(_value));
  _text[0] = 0;
}

void setparam(char *param, char *value)
{
int i;

  if (equal_ustring(param, "text"))
    {
    if (strlen(value) >= TEXT_SIZE)
      printf("Error: parameter value too big\n");

    strncpy(_text, value, TEXT_SIZE);
    _text[TEXT_SIZE-1] = 0;
    return;
    }

  for (i=0 ; i<MAX_PARAM ; i++)
    if (_param[i][0] == 0)
      break;

  if (i<MAX_PARAM)
    {
    strcpy(_param[i], param);

    if (strlen(value) >= VALUE_SIZE)
      printf("Error: parameter value too big\n");

    strncpy(_value[i], value, VALUE_SIZE);
    _value[i][VALUE_SIZE-1] = 0;
    }
  else
    {
    printf("Error: parameter array too small\n");
    }
}

char *getparam(char *param)
{
int i;

  if (equal_ustring(param, "text"))
    return _text;

  for (i=0 ; i<MAX_PARAM && _param[i][0]; i++)
    if (equal_ustring(param, _param[i]))
      break;

  if (i<MAX_PARAM)
    return _value[i];

  return NULL;
}

BOOL isparam(char *param)
{
int i;

  for (i=0 ; i<MAX_PARAM && _param[i][0]; i++)
    if (equal_ustring(param, _param[i]))
      break;

  if (i<MAX_PARAM && _param[i][0])
    return TRUE;

  return FALSE;
}

void unsetparam(char *param)
{
int i;

  for (i=0 ; i<MAX_PARAM ; i++)
    if (equal_ustring(param, _param[i]))
      break;

  if (i<MAX_PARAM)
    {
    _param[i][0] = 0;
    _value[i][0] = 0;
    }
}

/*------------------------------------------------------------------*/

void urlDecode(char *p) 
/********************************************************************\
   Decode the given string in-place by expanding %XX escapes
\********************************************************************/
{
char *pD, str[3];
int  i;

  pD = p;
  while (*p) 
    {
    if (*p=='%') 
      {
      /* Escape: next 2 chars are hex representation of the actual character */
      p++;
      if (isxdigit(p[0]) && isxdigit(p[1])) 
        {
        str[0] = p[0];
        str[1] = p[1];
        str[2] = 0;
        sscanf(p, "%02X", &i);
        
        *pD++ = (char) i;
        p += 2;
        }
      else
        *pD++ = '%';
      } 
    else if (*p == '+')
      {
      /* convert '+' to ' ' */
      *pD++ = ' ';
      p++;
      }
    else
      {
      *pD++ = *p++;
      }
    }
   *pD = '\0';
}

void urlEncode(char *ps) 
/********************************************************************\
   Encode the given string in-place by adding %XX escapes
\********************************************************************/
{
char *pd, *p, str[256];

  pd = str;
  p  = ps;
  while (*p) 
    {
    if (strchr(" %&=", *p))
      {
      sprintf(pd, "%%%02X", *p);
      pd += 3;
      p++;
      } 
    else 
      {
      *pd++ = *p++;
      }
    }
   *pd = '\0';
   strcpy(ps, str);
}

/*------------------------------------------------------------------*/

void redirect(char *path)
{
  /* redirect */
  rsprintf("HTTP/1.0 302 Found\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);

  rsprintf("Location: %s%s/%s\n\n<html>redir</html>\r\n", elogd_url, logbook, path);
}

void redirect2(char *path)
{
  redirect(path);
  send(_sock, return_buffer, strlen(return_buffer)+1, 0);
  closesocket(_sock);
  return_length = -1;
}

/*------------------------------------------------------------------*/

void show_help_page()
{
  rsprintf("<html><head>\n");
  rsprintf("<title>ELOG Electronic Logbook Help</title>\n");
  rsprintf("</head>\n\n");

  rsprintf("<body>\n");
  rsprintf("<center><h1>ELog Electronic Logbook Help</h1></center>\n");
  rsprintf("The Electronic Logbook (<i>ELOG</i>) can be used to store and retrieve messages\n");
  rsprintf("through a Web interface. Use the <b>New</b> button to create a new message\n");
  rsprintf("and use the <b>Next/Previous/Last</b> and <b>Query</b> buttons to view\n");
  rsprintf("messages.<p>\n\n");

  rsprintf("For more information, refer to the\n"); 
  rsprintf("<A HREF=\"http://midas.psi.ch/elog\">ELOG home page</A>.<P>\n\n");

  rsprintf("<hr>\n");
  rsprintf("<address>\n");
  rsprintf("<a href=\"http://pibeta.psi.ch/~stefan\">S. Ritt</a>, 17 May 2001");
  rsprintf("</address>");

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_header(char *title, char *path, int colspan)
{
time_t now;

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>%s</title></head>\n", title);
  rsprintf("<body><form method=\"GET\" action=\"%s%s/%s\">\n\n",
            elogd_url, logbook, path);

  /* title row */

  time(&now);

  rsprintf("<table border=3 cellpadding=1>\n");
  rsprintf("<tr><th colspan=%d bgcolor=#A0A0FF>Electronic Logbook \"%s\"", colspan, logbook);
  rsprintf("<th colspan=%d bgcolor=#A0A0FF>%s</tr>\n", colspan, ctime(&now));
}

/*------------------------------------------------------------------*/

void show_error(char *error)
{
  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG error</title></head>\n");
  rsprintf("<body><H1>%s</H1></body></html>\n", error);
}

/*------------------------------------------------------------------*/

void strencode(char *text)
{
int i;

  for (i=0 ; i<(int) strlen(text) ; i++)
    {
    switch (text[i])
      {
      case '\n': rsprintf("<br>\n"); break;
      case '<': rsprintf("&lt;"); break;
      case '>': rsprintf("&gt;"); break;
      case '&': rsprintf("&amp;"); break;
      case '\"': rsprintf("&quot;"); break;
      default: rsprintf("%c", text[i]);
      }
    }
}

/*------------------------------------------------------------------*/

void strencode2(char *b, char *text)
{
int i;

  for (i=0 ; i<(int) strlen(text) ; b++,i++)
    {
    switch (text[i])
      {
      case '\n': sprintf(b, "<br>\n"); break;
      case '<': sprintf(b, "&lt;"); break;
      case '>': sprintf(b, "&gt;"); break;
      case '&': sprintf(b, "&amp;"); break;
      case '\"': sprintf(b, "&quot;"); break;
      default: sprintf(b, "%c", text[i]);
      }
    }
  *b = 0;
}

/*------------------------------------------------------------------*/

void el_format(char *text, char *encoding)
{
  if (equal_ustring(encoding, "HTML"))
    rsputs(text);
  else
    strencode(text);
}

void show_elog_new(char *path, BOOL bedit, char *attachment)
{
int    i, size, run_number, wrap;
char   str[256], ref[256], *p, list[1000];
char   date[80], author[80], type[80], category[80], subject[256], text[10000], 
       orig_tag[80], reply_tag[80], att1[256], att2[256], att3[256], encoding[80];
time_t now;
BOOL   allow_edit;

  allow_edit = TRUE;

  /* get flag from configuration file */
  if (getcfg(logbook, "Allow edit", str))
    allow_edit= atoi(str);

  /* get message for reply */
  type[0] = category[0] = 0;
  att1[0] = att2[0] = att3[0] = 0;
  subject[0] = 0;

  if (path)
    {
    strcpy(str, path);
    size = sizeof(text);
    el_retrieve(str, date, &run_number, author, type, category, subject, 
                text, &size, orig_tag, reply_tag, att1, att2, att3, encoding);
    }

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG</title></head>\n");
  rsprintf("<body><form method=\"POST\" action=\"%s%s\" enctype=\"multipart/form-data\">\n", 
            elogd_url, logbook);

  rsprintf("<table border=3 cellpadding=5>\n");

  /*---- title row ----*/

  rsprintf("<tr><th bgcolor=#A0A0FF>ELOG");
  rsprintf("<th bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", logbook);

  if (bedit && !allow_edit)
    {
    rsprintf("<tr><td colspan=2 bgcolor=#FF8080 align=center><h1>Message editing disabled</h1>\n");
    rsprintf("</table>\n");
    rsprintf("</body></html>\r\n");
    return;
    }
  
  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=Submit>\n");
  rsprintf("</tr>\n\n");

  /*---- entry form ----*/

  if (bedit)
    {
    rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: %s<br>", date);
    time(&now);                 
    rsprintf("Revision date: %s", ctime(&now));
    }
  else
    {
    time(&now);
    rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: %s", ctime(&now));
    }

  if (bedit)
    {
    strcpy(str, author);
    if (strchr(str, '@'))
      *strchr(str, '@') = 0;
    }
  else
    str[0] = 0;

  rsprintf("<tr><td bgcolor=#FFA0A0>Author: <input type=\"text\" size=\"15\" maxlength=\"80\" name=\"Author\" value=\"%s\">\n", str);

  /* get type list from configuration file */
  if (getcfg(logbook, "Types", list))
    {
    p = strtok(list, ",");
    for (i=0 ; p ; i++)
      {
      strcpy(type_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  /* get category list from configuration file */
  if (getcfg(logbook, "Categories", list))
    {
    p = strtok(list, ",");
    for (i=0 ; p ; i++)
      {
      strcpy(category_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  rsprintf("<td bgcolor=#FFA0A0>Type: <select name=\"type\">\n", ref);
  for (i=0 ; i<20 && type_list[i][0] ; i++)
    if ((path && !bedit && equal_ustring(type_list[i], "reply")) ||
        (bedit && equal_ustring(type_list[i], type)))
      rsprintf("<option selected value=\"%s\">%s\n", type_list[i], type_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
  rsprintf("</select></tr>\n");

  rsprintf("<tr><td bgcolor=#A0FFA0>Category: <select name=\"category\">\n");
  for (i=0 ; i<20 && category_list[i][0] ; i++)
    if (path && equal_ustring(category_list[i], category))
      rsprintf("<option selected value=\"%s\">%s\n", category_list[i], category_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", category_list[i], category_list[i]);
  rsprintf("</select>\n");

  str[0] = 0;
  if (path && !bedit)
    sprintf(str, "Re: %s", subject);
  else
    sprintf(str, "%s", subject);
  rsprintf("<td bgcolor=#A0FFA0>Subject: <input type=text size=20 maxlength=\"80\" name=Subject value=\"%s\"></tr>\n", str);

  if (path)
    {
    /* hidden text for original message */
    rsprintf("<input type=hidden name=orig value=\"%s\">\n", path);

    if (bedit)
      rsprintf("<input type=hidden name=edit value=1>\n");
    }
  
  /* increased wrapping for replys (leave space for '> ' */
  wrap = (path && !bedit) ? 78 : 76;
  
  rsprintf("<tr><td colspan=2>Text:<br>\n");
  rsprintf("<textarea rows=10 cols=%d wrap=hard name=Text>", wrap);

  if (path)
    {
    if (bedit)
      {
      rsputs(text);
      }
    else
      {
      p = text;
      do
        {
        if (strchr(p, '\r'))
          {
          *strchr(p, '\r') = 0;
          rsprintf("> %s\n", p);
          p += strlen(p)+1;
          if (*p == '\n')
            p++;
          }
        else
          {
          rsprintf("> %s\n\n", p);
          break;
          }

        } while (TRUE);
      }
    }

  rsprintf("</textarea><br>\n");

  /* HTML check box */
  if (bedit && encoding[0] == 'H')
    rsprintf("<input type=checkbox checked name=html value=1>Submit as HTML text</tr>\n");
  else
    rsprintf("<input type=checkbox name=html value=1>Submit as HTML text</tr>\n");

  if (bedit && att1[0])
    rsprintf("<tr><td colspan=2 align=center bgcolor=#8080FF>If no attachment are resubmitted, the original ones are kept</tr>\n");

  /* attachment */
  rsprintf("<tr><td colspan=2 align=center>Enter attachment filename(s):</tr>");

  if (attachment)
    {
    str[0] = 0;
    if (attachment[0] != '\\' && attachment[0] != '/')
      strcpy(str, "\\");
    strcat(str, attachment);
    rsprintf("<tr><td colspan=2>Attachment1: <input type=hidden name=attachment0 value=\"%s\"><b>%s</b></tr>\n", str, str);
    }
  else
    rsprintf("<tr><td colspan=2>Attachment1: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile1\" value=\"%s\" accept=\"filetype/*\"></tr>\n", att1);

  rsprintf("<tr><td colspan=2>Attachment2: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile2\" value=\"%s\" accept=\"filetype/*\"></tr>\n", att2);
  rsprintf("<tr><td colspan=2>Attachment3: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile3\" value=\"%s\" accept=\"filetype/*\"></tr>\n", att3);

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_query()
{
int    i;
time_t now;
struct tm *tms;
char   *p, list[1000];

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s%s\">\n", elogd_url, logbook);

  rsprintf("<table border=3 cellpadding=5>\n");

  /*---- title row ----*/

  rsprintf("<tr><th colspan=2 bgcolor=#A0A0FF>ELOG");
  rsprintf("<th colspan=2 bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", logbook);
  
  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=4 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=\"Submit Query\">\n");
  rsprintf("<input type=reset value=\"Reset Form\">\n");
  rsprintf("</tr>\n\n");

  /*---- entry form ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C000>");
  rsprintf("<input type=checkbox name=mode value=\"summary\">Summary only\n");
  rsprintf("<td colspan=2 bgcolor=#C0C000>");
  rsprintf("<input type=checkbox name=attach value=1>Show attachments</tr>\n");

  time(&now);
  now -= 3600*24;
  tms = localtime(&now);
  tms->tm_year += 1900;

  rsprintf("<tr><td bgcolor=#FFFF00>Start date: ");
  rsprintf("<td colspan=3 bgcolor=#FFFF00><select name=\"m1\">\n");

  for (i=0 ; i<12 ; i++)
    if (i == tms->tm_mon)
      rsprintf("<option selected value=\"%s\">%s\n", mname[i], mname[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
  rsprintf("</select>\n");

  rsprintf("<select name=\"d1\">");
  for (i=0 ; i<31 ; i++)
    if (i+1 == tms->tm_mday)
      rsprintf("<option selected value=%d>%d\n", i+1, i+1);
    else
      rsprintf("<option value=%d>%d\n", i+1, i+1);
  rsprintf("</select>\n");

  rsprintf(" <input type=\"text\" size=5 maxlength=5 name=\"y1\" value=\"%d\">", tms->tm_year);
  rsprintf("</tr>\n");

  rsprintf("<tr><td bgcolor=#FFFF00>End date: ");
  rsprintf("<td colspan=3 bgcolor=#FFFF00><select name=\"m2\" value=\"%s\">\n", mname[tms->tm_mon]);

  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<12 ; i++)
    rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
  rsprintf("</select>\n");

  rsprintf("<select name=\"d2\">");
  rsprintf("<option selected value=\"\">\n");
  for (i=0 ; i<31 ; i++)
    rsprintf("<option value=%d>%d\n", i+1, i+1);
  rsprintf("</select>\n");

  rsprintf(" <input type=\"text\" size=5 maxlength=5 name=\"y2\">");
  rsprintf("</tr>\n");

  /* get type list from configuration file */
  if (getcfg(logbook, "Types", list))
    {
    p = strtok(list, ",");
    for (i=0 ; p ; i++)
      {
      strcpy(type_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  /* get category list from configuration file */
  if (getcfg(logbook, "Categories", list))
    {
    p = strtok(list, ",");
    for (i=0 ; p ; i++)
      {
      strcpy(category_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  rsprintf("<tr><td colspan=2 bgcolor=#FFA0A0>Author: ");
  rsprintf("<input type=\"test\" size=\"15\" maxlength=\"80\" name=\"author\">\n");

  rsprintf("<td colspan=2 bgcolor=#FFA0A0>Type: ");
  rsprintf("<select name=\"type\">\n");
  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<20 && type_list[i][0] ; i++)
    rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
  
  rsprintf("</select></tr>\n");

  rsprintf("<tr><td colspan=2 bgcolor=#A0FFA0>Category: ");
  rsprintf("<select name=\"category\">\n");
  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<20 && category_list[i][0] ; i++)
    rsprintf("<option value=\"%s\">%s\n", category_list[i], category_list[i]);
  rsprintf("</select>\n");

  rsprintf("<td colspan=2 bgcolor=#A0FFA0>Subject: ");
  rsprintf("<input type=\"text\" size=\"15\" maxlength=\"80\" name=\"subject\"></tr>\n");

  rsprintf("<tr><td colspan=4 bgcolor=#FFA0FF>Text: ");
  rsprintf("<input type=\"text\" size=\"15\" maxlength=\"80\" name=\"subtext\">\n");
  rsprintf("<i>(case insensitive substring)</i><tr>\n");

  rsprintf("</tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_delete(char *path)
{
int    status;
char   str[256];
BOOL   allow_delete;

  allow_delete = FALSE;

  /* get flag from configuration file */
  if (getcfg(logbook, "Allow delete", str))
    allow_delete = atoi(str);

  /* redirect if confirm = NO */
  if (getparam("confirm") && *getparam("confirm") && 
      strcmp(getparam("confirm"), "No") == 0)
    {
    sprintf(str, "%s", path);
    redirect(str);
    return;
    }

  /* header */
  sprintf(str, "%s", path);
  show_header("Delete ELog entry", str, 1);

  if (!allow_delete)
    {
    rsprintf("<tr><td colspan=2 bgcolor=#FF8080 align=center><h1>Message deletion disabled</h1>\n");
    }
  else
    {
    if (getparam("confirm") && *getparam("confirm"))
      {
      if (strcmp(getparam("confirm"), "Yes") == 0)
        {
        /* delete message */
        status = el_delete_message(path);
        rsprintf("<tr><td colspan=2 bgcolor=#80FF80 align=center>");
        if (status == EL_SUCCESS)
          rsprintf("<b>Message successfully deleted</b></tr>\n");
        else
          rsprintf("<b>Error deleting message: status = %d</b></tr>\n", status);

        rsprintf("<input type=hidden name=cmd value=last>\n");
        rsprintf("<tr><td colspan=2 align=center><input type=submit value=\"Goto last message\"></tr>\n");
        }
      }
    else
      {
      /* define hidden field for command */
      rsprintf("<input type=hidden name=cmd value=delete>\n");

      rsprintf("<tr><td colspan=2 bgcolor=#FF8080 align=center>");
      rsprintf("<b>Are you sure to delete this message?</b></tr>\n");

      rsprintf("<tr><td align=center><input type=submit name=confirm value=Yes>\n");
      rsprintf("<td align=center><input type=submit name=confirm value=No>\n");
      rsprintf("</tr>\n\n");
      }
    }

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_submit_query(INT past_n, INT last_n)
{
int    i, size, run, status, m1, d2, m2, y2, index, colspan, current_year, fh;
char   date[80], author[80], type[80], category[80], subject[256], text[10000], 
       orig_tag[80], reply_tag[80], attachment[3][256], encoding[80];
char   str[256], str2[10000], tag[256], ref[80], file_name[256];
BOOL   full, show_attachments;
DWORD  ltime_start, ltime_end, ltime_current, now;
struct tm tms, *ptms;
FILE   *f;

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s%s\">\n", elogd_url, logbook);

  rsprintf("<table border=3 cellpadding=2 width=\"100%%\">\n");

  /* get mode */
  if (past_n || last_n)
    {
    full = TRUE;
    show_attachments = FALSE;
    }
  else
    {
    full = !(*getparam("mode"));
    show_attachments = (*getparam("attach") > 0);
    }

  time(&now);
  ptms = localtime(&now);
  current_year = ptms->tm_year+1900;
  
  /*---- title row ----*/

  colspan = full ? 2 : 3;

  rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>ELOG");
  rsprintf("<th colspan=%d bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", colspan, logbook);

  /*---- menu buttons ----*/

  if (!full)
    {
    rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");

    rsprintf("<input type=submit name=cmd value=\"Query\">\n");
    rsprintf("<input type=submit name=cmd value=\"ELog\">\n");
    rsprintf("<input type=submit name=cmd value=\"Status\">\n");
    rsprintf("</tr>\n\n");
    }

  /*---- convert end date to ltime ----*/

  ltime_end = ltime_start = 0;

  if (!past_n && !last_n)
    {
    strcpy(str, getparam("m1"));
    for (m1=0 ; m1<12 ; m1++)
      if (equal_ustring(str, mname[m1]))
        break;
    if (m1 == 12)
      m1 = 0;

    if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
      {
      if (*getparam("m2"))
        {
        strcpy(str, getparam("m2"));
        for (m2=0 ; m2<12 ; m2++)
          if (equal_ustring(str, mname[m2]))
            break;
        if (m2 == 12)
          m2 = 0;
        }
      else
        m2 = m1;

      if (*getparam("y2"))
        y2 = atoi(getparam("y2"));
      else
        y2 = atoi(getparam("y1"));

      if (y2 < 1990 || y2 > current_year)
        {
        rsprintf("</table>\r\n");
        rsprintf("<h1>Year %d out of range</h1>", y2);
        rsprintf("</body></html>\r\n");
        return;
        }
      
      if (*getparam("d2"))
        d2 = atoi(getparam("d2"));
      else
        d2 = atoi(getparam("d1"));

      memset(&tms, 0, sizeof(struct tm));
      tms.tm_year = y2 % 100;
      tms.tm_mon  = m2;
      tms.tm_mday = d2;
      tms.tm_hour = 12;

      if (tms.tm_year < 90)
        tms.tm_year += 100;
      ltime_end = mktime(&tms);
      }
    }

  /*---- title row ----*/

  colspan = full ? 5 : 6;

  if (*getparam("r1"))
    {
    if (*getparam("r2"))
      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between runs %s and %s</b></tr>\n",
                colspan, getparam("r1"), getparam("r2"));
    else
      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between run %s and today</b></tr>\n",
                colspan, getparam("r1"));
    }
  else 
    {
    if (past_n)
      {
      rsprintf("<tr><td colspan=6><a href=\"past%d\">Last %d days</a>\n", 
                past_n+1, past_n+1);
      rsprintf("<a href=\"\">Last message</a></tr>\n");

      if (past_n == 1)
        rsprintf("<tr><td colspan=6 bgcolor=#FFFF00><b>Last day</b></tr>\n");
      else
        rsprintf("<tr><td colspan=6 bgcolor=#FFFF00><b>Last %d days</b></tr>\n", past_n);
      }

    else if (last_n)
      {
      rsprintf("<tr><td colspan=6><a href=\"last%d\">Last %d messages</a>\n", 
                last_n+10, last_n+10);
      rsprintf("<a href=\"\">Last message</a></tr>\n");

      rsprintf("<tr><td colspan=6 bgcolor=#FFFF00><b>Last %d messages</b></tr>\n", last_n);
      }

    else if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between %s %s %s and %s %d %d</b></tr>\n",
                colspan, getparam("m1"), getparam("d1"), getparam("y1"),
                mname[m2], d2, y2);
    else
      {
      time(&now);
      ptms = localtime(&now);
      ptms->tm_year += 1900;

      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between %s %s %s and %s %d %d</b></tr>\n",
                colspan, getparam("m1"), getparam("d1"), getparam("y1"),
                mname[ptms->tm_mon], ptms->tm_mday, ptms->tm_year);
      }
    }

  rsprintf("</tr>\n<tr>");

  rsprintf("<td colspan=%d bgcolor=#FFA0A0>\n", colspan);

  if (*getparam("author"))
    rsprintf("Author: <b>%s</b>   ", getparam("author"));

  if (*getparam("type"))
    rsprintf("Type: <b>%s</b>   ", getparam("type"));

  if (*getparam("category"))
    rsprintf("Category: <b>%s</b>   ", getparam("category"));

  if (*getparam("subject"))
    rsprintf("Subject: <b>%s</b>   ", getparam("subject"));

  if (*getparam("subtext"))
    rsprintf("Text: <b>%s</b>   ", getparam("subtext"));

  rsprintf("</tr>\n");

  /*---- table titles ----*/

  if (full)
    rsprintf("<tr><th>Date<th>Author<th>Type<th>Category<th>Subject</tr>\n");
  else
    rsprintf("<tr><th>Date<th>Author<th>Type<th>Category<th>Subject<th>Text</tr>\n");

  /*---- do query ----*/

  if (past_n)
    {
    time(&now);
    ltime_start = now-3600*24*past_n;
    ptms = localtime(&ltime_start);

    sprintf(tag, "%02d%02d%02d.0", ptms->tm_year % 100, ptms->tm_mon+1, ptms->tm_mday);
    }
  else if (last_n)
    {
    strcpy(tag, "-1");
    el_search_message(tag, &fh, TRUE);
    close(fh);
    for (i=1 ; i<last_n ; i++)   
      {
      strcat(tag, "-1");   
      el_search_message(tag, &fh, TRUE);   
      close(fh);   
      } 
    }
  else if (*getparam("r1"))
    {
    /* do run query */
    el_search_run(atoi(getparam("r1")), tag);
    }
  else
    {
    /* do date-date query */
    i = atoi(getparam("y1"));
    if (i < 1990 || i > current_year)
      {
      rsprintf("</table>\r\n");
      rsprintf("<h1>Year %s out of range</h1>", getparam("y1"));
      rsprintf("</body></html>\r\n");
      return;
      }

    i = i% 100;
    sprintf(tag, "%02d%02d%02d.0", i, m1+1, atoi(getparam("d1")));
    }

  do
    {
    size = sizeof(text);
    status = el_retrieve(tag, date, &run, author, type, category, subject, 
                         text, &size, orig_tag, reply_tag, 
                         attachment[0], attachment[1], attachment[2],
                         encoding);
    strcat(tag, "+1");

    /* check for end run */
    if (*getparam("r2") && atoi(getparam("r2")) < run)
      break;

    /* convert date to unix format */
    memset(&tms, 0, sizeof(struct tm));
    tms.tm_year = (tag[0]-'0')*10 + (tag[1]-'0');
    tms.tm_mon  = (tag[2]-'0')*10 + (tag[3]-'0') -1;
    tms.tm_mday = (tag[4]-'0')*10 + (tag[5]-'0');
    tms.tm_hour = (date[11]-'0')*10 + (date[12]-'0');
    tms.tm_min  = (date[14]-'0')*10 + (date[15]-'0');
    tms.tm_sec  = (date[17]-'0')*10 + (date[18]-'0');

    if (tms.tm_year < 90)
      tms.tm_year += 100;
    ltime_current = mktime(&tms);
    
    /* check for start date */
    if (ltime_start > 0)
      if (ltime_current < ltime_start)
        continue;

    /* check for end date */
    if (ltime_end > 0)
      {
      if (ltime_current > ltime_end)
        break;
      }

    if (status == EL_SUCCESS)
      {
      /* do filtering */
      if (*getparam("type") && !equal_ustring(getparam("type"), type))
        continue;
      if (*getparam("category") && !equal_ustring(getparam("category"), category))
        continue;

      if (*getparam("author"))
        {
        strcpy(str, getparam("author"));
        for (i=0 ; i<(int)strlen(str) ; i++)
          str[i] = toupper(str[i]);
        str[i] = 0;
        for (i=0 ; i<(int)strlen(author) && author[i] != '@'; i++)
          str2[i] = toupper(author[i]);
        str2[i] = 0;

        if (strstr(str2, str) == NULL)
          continue;
        }
      
      if (*getparam("subject"))
        {
        strcpy(str, getparam("subject"));
        for (i=0 ; i<(int)strlen(str) ; i++)
          str[i] = toupper(str[i]);
        str[i] = 0;
        for (i=0 ; i<(int)strlen(subject) ; i++)
          str2[i] = toupper(subject[i]);
        str2[i] = 0;

        if (strstr(str2, str) == NULL)
          continue;
        }

      if (*getparam("subtext"))
        {
        strcpy(str, getparam("subtext"));
        for (i=0 ; i<(int)strlen(str) ; i++)
          str[i] = toupper(str[i]);
        str[i] = 0;
        for (i=0 ; i<(int)strlen(text) ; i++)
          str2[i] = toupper(text[i]);
        str2[i] = 0;

        if (strstr(str2, str) == NULL)
          continue;
        }

      /* filter passed: display line */

      strcpy(str, tag);
      if (strchr(str, '+'))
        *strchr(str, '+') = 0;
      sprintf(ref, "%s%s/%s", elogd_url, logbook, str);

      strncpy(str, text, 80);
      str[80] = 0;

      if (full)
        {
        rsprintf("<tr><td><a href=%s>%s</a><td>%s<td>%s<td>%s<td>%s</tr>\n", ref, date, author, type, category, subject);
        rsprintf("<tr><td colspan=5>");
        
        if (equal_ustring(encoding, "plain"))
          {
          rsputs("<pre>");
          rsputs2(text);
          rsputs("</pre>");
          }
        else
          rsputs(text);

        if (!show_attachments && attachment[0][0])
          {
          if (attachment[1][0])
            rsprintf("Attachments: ");
          else
            rsprintf("Attachment: ");
          }
        else
          rsprintf("</tr>\n");

        for (index = 0 ; index < 3 ; index++)
          {
          if (attachment[index][0])
            {
            for (i=0 ; i<(int)strlen(attachment[index]) ; i++)
              str[i] = toupper(attachment[index][i]);
            str[i] = 0;
    
            sprintf(ref, "%s%s/%s", elogd_url, logbook, attachment[index]);

            if (!show_attachments)
              {
              rsprintf("<a href=\"%s\"><b>%s</b></a> ", 
                        ref, attachment[index]+14);
              }
            else
              {
              colspan = 5;
              if (strstr(str, ".GIF") ||
                  strstr(str, ".JPG"))
                {
                rsprintf("<tr><td colspan=%d>Attachment: <a href=\"%s\"><b>%s</b></a><br>\n", 
                          colspan, ref, attachment[index]+14);
                if (show_attachments)
                  rsprintf("<img src=\"%s\"></tr>", ref);
                }
              else
                {
                rsprintf("<tr><td colspan=%d bgcolor=#C0C0FF>Attachment: <a href=\"%s\"><b>%s</b></a>\n", 
                          colspan, ref, attachment[index]+14);

                if ((strstr(str, ".TXT") ||
                     strstr(str, ".ASC") ||
                     strchr(str, '.') == NULL) && show_attachments)
                  {
                  /* display attachment */
                  rsprintf("<br><pre>");

                  strcpy(file_name, data_dir);

                  strcat(file_name, attachment[index]);

                  f = fopen(file_name, "rt");
                  if (f != NULL)
                    {
                    while (!feof(f))
                      {
                      str[0] = 0;
                      fgets(str, sizeof(str), f);
                      rsputs2(str);
                      }
                    fclose(f);
                    }

                  rsprintf("</pre>\n");
                  }
                rsprintf("</tr>\n");
                }
              }
            }
          }
 
        if (!show_attachments && attachment[0][0])
          rsprintf("</tr>\n");

        }
      else
        {
        rsprintf("<tr><td>%s<td>%s<td>%s<td>%s<td>%s\n", date, author, type, category, subject);
        rsprintf("<td><a href=%s>", ref);
      
        el_format(str, encoding);
      
        rsprintf("</a></tr>\n");
        }
      }

    } while (status == EL_SUCCESS);
    
  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_rawfile(char *path)
{
FILE   *f;
char   file_name[256], line[1000];

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG File Display</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s%s\">\n", elogd_url, logbook);

  rsprintf("<table border=3 cellpadding=1 width=\"100%%\">\n");

  /*---- title row ----*/


  rsprintf("<tr><th bgcolor=#A0A0FF>ELOG File Display");
  rsprintf("<th bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", logbook);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");
  rsprintf("<input type=submit name=cmd value=\"ELog\">\n");
  rsprintf("</tr></table>\n\n");

  /*---- open file ----*/

  strcpy(file_name, data_dir);
  strcat(file_name, path);

  f = fopen(file_name, "r");
  if (f == NULL)
    {
    strcat(file_name, ".txt");
    f = fopen(file_name, "r");
    if (f == NULL)
      {
      rsprintf("<h3>File \"%s\" not available for this experiment</h3>\n", path);
      rsprintf("</body></html>\n");
      return;
      }
    }

  /*---- file contents ----*/

  rsprintf("<pre>\n");
  while (!feof(f))
    {
    memset(line, 0, sizeof(line));
    fgets(line, sizeof(line), f);
    rsputs2(line);
    }
  rsprintf("</pre>\n");
  fclose(f);

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void submit_elog()
{
char   str[80], author[256], path[256], path1[256];
char   *buffer[3];
char   att_file[3][256];
int    i, fh, size;
struct hostent *phe;

  strcpy(att_file[0], getparam("attachment0"));
  strcpy(att_file[1], getparam("attachment1"));
  strcpy(att_file[2], getparam("attachment2"));

  /* check for author */
  if (*getparam("author") == 0)
    {
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n");
    rsprintf("Content-Type: text/html\r\n\r\n");

    rsprintf("<html><head><title>ELog Error</title></head>\n");
    rsprintf("<i>Error: No author supplied.</i><p>\n");
    rsprintf("Please go back and enter your name in the <i>author</i> field.\n");
    rsprintf("<body></body></html>\n");
    return;
    }

  /* check for valid attachment files */
  for (i=0 ; i<3 ; i++)
    {
    buffer[i] = NULL;
    sprintf(str, "attachment%d", i);
    if (getparam(str) && *getparam(str) && _attachment_size[i] == 0)
      {
      /* replace '\' by '/' */
      strcpy(path, getparam(str));  
      strcpy(path1, path);
      while (strchr(path, '\\'))    
        *strchr(path, '\\') = '/';

      if ((fh = open(path1, O_RDONLY | O_BINARY)) >= 0)
        {
        size = lseek(fh, 0, SEEK_END);
        buffer[i] = malloc(size);
        lseek(fh, 0, SEEK_SET);
        read(fh, buffer[i], size);
        close(fh);
        strcpy(att_file[i], path);
        _attachment_buffer[i] = buffer[i];
        _attachment_size[i] = size;
        }
      else
        {
        rsprintf("HTTP/1.0 200 Document follows\r\n");
        rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
        rsprintf("Content-Type: text/html\r\n\r\n");

        rsprintf("<html><head><title>ELog Error</title></head>\n");
        rsprintf("<i>Error: Attachment file <i>%s</i> not valid.</i><p>\n", getparam(str));
        rsprintf("Please go back and enter a proper filename (use the <b>Browse</b> button).\n");
        rsprintf("<body></body></html>\n");
        return;
        }
      }
    }

  /* add remote host name to author */
  phe = gethostbyaddr((char *) &remote_addr, 4, PF_INET);
  if (phe == NULL)
    {
    /* use IP number instead */
    strcpy(str, (char *)inet_ntoa(remote_addr));
    }
  else
    strcpy(str, phe->h_name);
      

  strcpy(author, getparam("author"));
  strcat(author, "@");
  strcat(author, str);

  str[0] = 0;
  if (*getparam("edit"))
    strcpy(str, getparam("orig"));

  el_submit(atoi(getparam("run")), author, getparam("type"),
            getparam("category"), getparam("subject"), getparam("text"), 
            getparam("orig"), *getparam("html") ? "HTML" : "plain", 
            att_file[0], _attachment_buffer[0], _attachment_size[0], 
            att_file[1], _attachment_buffer[1], _attachment_size[1], 
            att_file[2], _attachment_buffer[2], _attachment_size[2], 
            str, sizeof(str));

  for (i=0 ; i<3 ; i++)
    if (buffer[i])
      free(buffer[i]);

  rsprintf("HTTP/1.0 302 Found\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);

  rsprintf("Location: %s%s/%s\n\n<html>redir</html>\r\n", elogd_url, logbook, str);
}

/*------------------------------------------------------------------*/

void show_elog_page(char *logbook, char *path)
{
int   size, i, run, msg_status, status, fh, length, first_message, last_message, index;
char  str[256], orig_path[256], command[80], ref[256], file_name[256];
char  date[80], author[80], type[80], category[80], subject[256], text[10000], 
      orig_tag[80], reply_tag[80], attachment[3][256], encoding[80], att[256];
FILE  *f;
BOOL  allow_delete, allow_edit;

  allow_delete = FALSE;
  allow_edit = TRUE;

  /* get flags from configuration file */
  if (getcfg(logbook, "Allow delete", str))
    allow_delete = atoi(str);
  if (getcfg(logbook, "Allow edit", str))
    allow_edit = atoi(str);
  
  /*---- interprete commands ---------------------------------------*/

  strcpy(command, getparam("cmd"));

  if (equal_ustring(command, "help"))
    {
    show_help_page();
    return;
    }

  if (equal_ustring(command, "new"))
    {
    if (*getparam("file"))
      show_elog_new(NULL, FALSE, getparam("file"));
    else
      show_elog_new(NULL, FALSE, NULL);
    return;
    }

  if (equal_ustring(command, "edit"))
    {
    show_elog_new(path, TRUE, NULL);
    return;
    }

  if (equal_ustring(command, "reply"))
    {
    show_elog_new(path, FALSE, NULL);
    return;
    }

  if (equal_ustring(command, "submit"))
    {
    submit_elog();
    return;
    }

  if (equal_ustring(command, "query"))
    {
    show_elog_query();
    return;
    }

  if (equal_ustring(command, "submit query"))
    {
    show_elog_submit_query(0, 0);
    return;
    }

  if (equal_ustring(command, "last day"))
    {
    redirect("past1");
    return;
    }

  if (equal_ustring(command, "last 10"))
    {
    redirect("last10");
    return;
    }

  if (equal_ustring(command, "delete"))
    {
    show_elog_delete(path);
    return;
    }

  if (strncmp(path, "past", 4) == 0)
    {
    show_elog_submit_query(atoi(path+4), 0);
    return;
    }

  if (strncmp(path, "last", 4) == 0)
    {
    show_elog_submit_query(0, atoi(path+4));
    return;
    }

  /*---- check if file requested -----------------------------------*/

  if (strlen(path) > 13 && path[6] == '_' && path[13] == '_')
    {
    strcpy(file_name, data_dir);
    strcat(file_name, path);

    fh = open(file_name, O_RDONLY | O_BINARY);
    if (fh > 0)
      {
      lseek(fh, 0, SEEK_END);
      length = TELL(fh);
      lseek(fh, 0, SEEK_SET);

      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
      rsprintf("Accept-Ranges: bytes\r\n");

      /* return proper header for file type */
      for (i=0 ; i<(int)strlen(path) ; i++)
        str[i] = toupper(path[i]);
      str[i] = 0;

      for (i=0 ; filetype[i].ext[0] ; i++)
        if (strstr(str, filetype[i].ext))
          break;

      if (filetype[i].ext[0])
        rsprintf("Content-Type: %s\r\n", filetype[i].type);
      else if (strchr(str, '.') == NULL)
        rsprintf("Content-Type: text/plain\r\n");
      else
        rsprintf("Content-Type: application/octet-stream\r\n");

      rsprintf("Content-Length: %d\r\n\r\n", length);

      /* return if file too big */
      if (length > (int) (sizeof(return_buffer) - strlen(return_buffer)))
        {
        printf("return buffer too small\n");
        close(fh);
        return;
        }

      return_length = strlen(return_buffer)+length;
      read(fh, return_buffer+strlen(return_buffer), length);

      close(fh);
      }

    return;
    }

  /*---- check if runlog is requested ------------------------------*/

  if (path[0] > '9')
    {
    show_rawfile(path);
    return;
    }
  
  /*---- check next/previous message -------------------------------*/

  last_message = first_message = FALSE;
  if (equal_ustring(command, "next") || equal_ustring(command, "previous") ||
      equal_ustring(command, "last"))
    {
    strcpy(orig_path, path);

    if (equal_ustring(command, "last"))
      path[0] = 0;

    do
      {
      strcat(path, equal_ustring(command, "next") ? "+1" : "-1");
      status = el_search_message(path, &fh, TRUE);
      close(fh);
      if (status != EL_SUCCESS)
        {
        if (equal_ustring(command, "next"))
          last_message = TRUE;
        else
          first_message = TRUE;
        strcpy(path, orig_path);
        break;
        }

      size = sizeof(text);
      el_retrieve(path, date, &run, author, type, category, subject, 
                  text, &size, orig_tag, reply_tag, 
                  attachment[0], attachment[1], attachment[2], 
                  encoding);
      
      if (strchr(author, '@'))
        *strchr(author, '@') = 0;
      if (*getparam("lauthor")  == '1' && !equal_ustring(getparam("author"),  author ))
        continue;
      if (*getparam("ltype")    == '1' && !equal_ustring(getparam("type"),    type   ))
        continue;
      if (*getparam("lcategory")  == '1' && !equal_ustring(getparam("category"),  category ))
        continue;
      if (*getparam("lsubject") == '1')
        {
        strcpy(str, getparam("subject"));
        for (i=0 ; i<(int)strlen(str) ; i++)
          str[i] = toupper(str[i]);
        for (i=0 ; i<(int)strlen(subject) ; i++)
          subject[i] = toupper(subject[i]);

        if (strstr(subject, str) == NULL)
          continue;
        }
      
      sprintf(str, "%s", path);

      if (*getparam("lauthor") == '1')
        if (strchr(str, '?') == NULL)
          strcat(str, "?lauthor=1");
        else
          strcat(str, "&lauthor=1");

      if (*getparam("ltype") == '1')
        if (strchr(str, '?') == NULL)
          strcat(str, "?ltype=1");
        else
          strcat(str, "&ltype=1");

      if (*getparam("lcategory") == '1')
        if (strchr(str, '?') == NULL)
          strcat(str, "?lcategory=1");
        else
          strcat(str, "&lcategory=1");

      if (*getparam("lsubject") == '1')
        if (strchr(str, '?') == NULL)
          strcat(str, "?lsubject=1");
        else
          strcat(str, "&lsubject=1");

      redirect(str);
      return;

      } while (TRUE);
    }

  /*---- get current message ---------------------------------------*/

  size = sizeof(text);
  strcpy(str, path);
  msg_status = el_retrieve(str, date, &run, author, type, category, subject, 
                           text, &size, orig_tag, reply_tag, 
                           attachment[0], attachment[1], attachment[2],
                           encoding);

  /*---- header ----*/

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s%s/%s\">\n", elogd_url, logbook, str);

  rsprintf("<table cols=2 border=2 cellpadding=2>\n");

  /*---- title row ----*/

  rsprintf("<tr><th bgcolor=#A0A0FF>ELOG");
  rsprintf("<th bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", logbook);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");
  rsprintf("<input type=submit name=cmd value=New>\n");
  if (allow_edit)
    rsprintf("<input type=submit name=cmd value=Edit>\n");
  if (allow_delete)
    rsprintf("<input type=submit name=cmd value=Delete>\n");
  rsprintf("<input type=submit name=cmd value=Reply>\n");
  rsprintf("<input type=submit name=cmd value=Query>\n");
  rsprintf("<input type=submit name=cmd value=\"Last day\">\n");
  rsprintf("<input type=submit name=cmd value=\"Last 10\">\n");
  rsprintf("<input type=submit name=cmd value=\"Help\">\n");

  rsprintf("</tr>\n");

  rsprintf("<tr><td colspan=2 bgcolor=#E0E0E0>");
  rsprintf("<input type=submit name=cmd value=Next>\n");
  rsprintf("<input type=submit name=cmd value=Previous>\n");
  rsprintf("<input type=submit name=cmd value=Last>\n");
  rsprintf("<i>Check a box to browse only entries of that type</i>\n");
  rsprintf("</tr>\n\n");

  if (msg_status != EL_FILE_ERROR && (reply_tag[0] || orig_tag[0]))
    {
    rsprintf("<tr><td colspan=2 bgcolor=#F0F0F0>");
    if (orig_tag[0])
      {
      sprintf(ref, "%s%s/%s", elogd_url, logbook, orig_tag);
      rsprintf("  <a href=\"%s\">Original message</a>  ", ref);
      }
    if (reply_tag[0])
      {
      sprintf(ref, "%s%s/%s", elogd_url, logbook, reply_tag);
      rsprintf("  <a href=\"%s\">Reply to this message</a>  ", ref);
      }
    rsprintf("</tr>\n");
    }

  /*---- message ----*/

  if (msg_status == EL_FILE_ERROR)
    rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><h1>No message available</h1></tr>\n");
  else
    {
    if (last_message)
      rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>This is the last message in the ELog</b></tr>\n");

    if (first_message)
      rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>This is the first message in the ELog</b></tr>\n");

    rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: <b>%s</b></tr>\n\n", date);

    /* define hidded fields */
    strcpy(str, author);
    if (strchr(str, '@'))
      *strchr(str, '@') = 0;
    rsprintf("<input type=hidden name=author  value=\"%s\">\n", str); 
    rsprintf("<input type=hidden name=type    value=\"%s\">\n", type); 
    rsprintf("<input type=hidden name=category  value=\"%s\">\n", category); 
    rsprintf("<input type=hidden name=subject value=\"%s\">\n\n", subject); 

    if (*getparam("lauthor") == '1')
      rsprintf("<tr><td bgcolor=#FFA0A0><input type=\"checkbox\" checked name=\"lauthor\" value=\"1\">");
    else
      rsprintf("<tr><td bgcolor=#FFA0A0><input type=\"checkbox\" name=\"lauthor\" value=\"1\">");
    rsprintf("  Author: <b>%s</b>\n", author);

    if (*getparam("ltype") == '1')
      rsprintf("<td bgcolor=#FFA0A0><input type=\"checkbox\" checked name=\"ltype\" value=\"1\">");
    else
      rsprintf("<td bgcolor=#FFA0A0><input type=\"checkbox\" name=\"ltype\" value=\"1\">");
    rsprintf("  Type: <b>%s</b></tr>\n", type);

    if (*getparam("lcategory") == '1')
      rsprintf("<tr><td bgcolor=#A0FFA0><input type=\"checkbox\" checked name=\"lcategory\" value=\"1\">");
    else
      rsprintf("<tr><td bgcolor=#A0FFA0><input type=\"checkbox\" name=\"lcategory\" value=\"1\">");

    rsprintf("  Category: <b>%s</b>\n", category);

    if (*getparam("lsubject") == '1')
      rsprintf("<td bgcolor=#A0FFA0><input type=\"checkbox\" checked name=\"lsubject\" value=\"1\">");
    else
      rsprintf("<td bgcolor=#A0FFA0><input type=\"checkbox\" name=\"lsubject\" value=\"1\">");
    rsprintf("  Subject: <b>%s</b></tr>\n", subject);

    
    /* message text */
    rsprintf("<tr><td colspan=2>\n");
    if (equal_ustring(encoding, "plain"))
      {
      rsputs("<pre>");
      rsputs2(text);
      rsputs("</pre>");
      }
    else
      rsputs(text);
    rsputs("</tr>\n");

    for (index = 0 ; index < 3 ; index++)
      {
      if (attachment[index][0])
        {
        for (i=0 ; i<(int)strlen(attachment[index]) ; i++)
          att[i] = toupper(attachment[index][i]);
        att[i] = 0;
      
        sprintf(ref, "%s%s/%s", elogd_url, logbook, attachment[index]);

        if (strstr(att, ".GIF") ||
            strstr(att, ".JPG"))
          {
          rsprintf("<tr><td colspan=2>Attachment: <a href=\"%s\"><b>%s</b></a><br>\n", 
                    ref, attachment[index]+14);
          rsprintf("<img src=\"%s\"></tr>", ref);
          }
        else
          {
          rsprintf("<tr><td colspan=2 bgcolor=#C0C0FF>Attachment: <a href=\"%s\"><b>%s</b></a>\n", 
                    ref, attachment[index]+14);
          if (strstr(att, ".TXT") ||
              strstr(att, ".ASC") ||
              strchr(att, '.') == NULL)
            {
            /* display attachment */
            rsprintf("<br>");
            if (!strstr(att, ".HTML"))
              rsprintf("<pre>");

            strcpy(file_name, data_dir);
            strcat(file_name, attachment[index]);

            f = fopen(file_name, "rt");
            if (f != NULL)
              {
              while (!feof(f))
                {
                str[0] = 0;
                fgets(str, sizeof(str), f);

                if (!strstr(att, ".HTML"))
                  rsputs2(str);
                else
                  rsputs(str);
                }
              fclose(f);
              }

            if (!strstr(att, ".HTML"))
              rsprintf("</pre>");
            rsprintf("\n");
            }
          rsprintf("</tr>\n");
          }
        }
      }
    }

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_password_page(char *password, char *experiment)
{
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>Enter password</title></head><body>\n\n");

  rsprintf("<form method=\"GET\" action=\"%s%s\">\n\n", elogd_url, logbook);

  rsprintf("<table border=1 cellpadding=5>");

  if (password[0])
    rsprintf("<tr><th bgcolor=#FF0000>Wrong password!</tr>\n");

  rsprintf("<tr><th bgcolor=#A0A0FF>Please enter password</tr>\n");
  rsprintf("<tr><td align=center><input type=password name=pwd></tr>\n");
  rsprintf("<tr><td align=center><input type=submit value=Submit></tr>");

  rsprintf("</table>\n");

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

BOOL check_web_password(char *logbook, char *password, char *redir)
{
char  str[256];

  /* get write password from configuration file */
  if (getcfg(logbook, "Write password", str))
    {
    if (strcmp(password, str) == 0)
      return TRUE;

    /* show web password page */
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
    rsprintf("Content-Type: text/html\r\n\r\n");

    rsprintf("<html><head><title>Enter password</title></head><body>\n\n");

    rsprintf("<form method=\"GET\" action=\"%s%s\">\n\n", elogd_url, logbook);

    /* define hidden fields for current experiment and destination */
    if (redir[0])
      rsprintf("<input type=hidden name=redir value=\"%s\">\n", redir);

    rsprintf("<table border=1 cellpadding=5>");

    if (password[0])
      rsprintf("<tr><th bgcolor=#FF0000>Wrong password!</tr>\n");

    rsprintf("<tr><th bgcolor=#A0A0FF>Please enter password to obtain write access</tr>\n");
    rsprintf("<tr><td align=center><input type=password name=wpwd></tr>\n");
    rsprintf("<tr><td align=center><input type=submit value=Submit></tr>");

    rsprintf("</table>\n");

    rsprintf("</body></html>\r\n");

    return FALSE;
    }
  else
    return TRUE;
}

/*------------------------------------------------------------------*/

void show_selection_page()
{
int  i;
char str[80], logbook[80];

  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html>\n");
  rsprintf("<head>\n");
  rsprintf("<title>ELOG Logbook Selection</title>\n");
  rsprintf("</head>\n\n");

  rsprintf("<body>\n\n");

  rsprintf("<table border=3 cellpadding=5><tr><td colspan=2 bgcolor=#80FF80>\n");
  rsprintf("Several logbooks are defined on this host.<BR>\n");
  rsprintf("Please select the one to connect to:</td><tr>\n");

  for (i=0 ;  ; i++)
    {
    if (!enumgrp(i, logbook))
      break;

    rsprintf("<tr><td bgcolor=#FFFF00><a href=\"%s%s\">%s</a>", elogd_url, logbook, logbook);

    str[0] = 0;
    getcfg(logbook, "Comment", str);
    rsprintf("<td>%s</td></tr>\n", str);
    }

  rsprintf("</table></body>\n");
  rsprintf("</html>\r\n");
  
}

/*------------------------------------------------------------------*/

void get_password(char *password)
{
static char last_password[32];

  if (strncmp(password, "set=", 4) == 0)
    strcpy(last_password, password+4);
  else
    strcpy(password, last_password);
}

/*------------------------------------------------------------------*/

void interprete(char *cookie_wpwd, char *path)
/********************************************************************\

  Routine: interprete

  Purpose: Interprete parametersand generate HTML output.

  Input:
    char *cookie_pwd        Cookie containing encrypted password
    char *path              Message path

  <implicit>
    _param/_value array accessible via getparam()

\********************************************************************/
{
int    index;
char   str[256], enc_pwd[80];
char   enc_path[256], dec_path[256];
char   *experiment, *wpassword, *command, *value, *group;
time_t now;
struct tm *gmt;

  /* encode path for further usage */
  strcpy(dec_path, path);
  urlDecode(dec_path);
  urlDecode(dec_path); /* necessary for %2520 -> %20 -> ' ' */
  strcpy(enc_path, dec_path);
  urlEncode(enc_path);

  experiment = getparam("exp");
  wpassword = getparam("wpwd");
  command = getparam("cmd");
  value = getparam("value");
  group = getparam("group");
  index = atoi(getparam("index"));

  /* if experiment give, use it as logbook */
  if (experiment && experiment[0])
    strcpy(logbook, experiment);
  
  /* if no logbook given, display logbook selection page */
  if (!logbook[0])
    {
    enumgrp(0, logbook);

    if (enumgrp(1, logbook))
      {
      show_selection_page();
      return;
      }
    }

  /* get data dir from configuration file */
  getcfg(logbook, "Data dir", data_dir);
  if (data_dir[strlen(data_dir)-1] != DIR_SEPARATOR)
    {
    data_dir[strlen(data_dir)+1] = 0;
    data_dir[strlen(data_dir)] = DIR_SEPARATOR;
    }
    
  if (wpassword[0])
    {
    /* check if password correct */
    base64_encode(wpassword, enc_pwd);

    if (!check_web_password(logbook, enc_pwd, getparam("redir")))
      return;
    
    rsprintf("HTTP/1.0 302 Found\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n", VERSION);

    time(&now);
    now += 3600;
    gmt = gmtime(&now);
    strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

    rsprintf("Set-Cookie: elog_wpwd=%s; path=/%s; expires=%s\r\n", enc_pwd, logbook, str);

    sprintf(str, "%s%s/%s", elogd_url, logbook, getparam("redir"));
    rsprintf("Location: %s\n\n<html>redir</html>\r\n", str);
    return;
    }

  /*---- show ELog page --------------------------------------------*/

  if (equal_ustring(command, "new") ||
      equal_ustring(command, "edit") ||
      equal_ustring(command, "reply") ||
      equal_ustring(command, "submit"))
    {
    sprintf(str, "%s?cmd=%s", path, command);
    if (!check_web_password(logbook, cookie_wpwd, str))
      return;
    }

  show_elog_page(logbook, dec_path);
  return;
}

/*------------------------------------------------------------------*/

void decode_get(char *string, char *cookie_wpwd)
{
char path[256];
char *p, *pitem;

  initparam();

  strncpy(path, string, sizeof(path));
  path[255] = 0;
  if (strchr(path, '?'))
    *strchr(path, '?') = 0;
  setparam("path", path);

  if (strchr(string, '?'))
    {
    p = strchr(string, '?')+1;

    /* cut trailing "/" from netscape */
    if (p[strlen(p)-1] == '/')
      p[strlen(p)-1] = 0;

    p = strtok(p,"&");
    while (p != NULL) 
      {
      pitem = p;
      p = strchr(p,'=');
      if (p != NULL)
        {
        *p++ = 0;
        urlDecode(pitem);
        urlDecode(p);

        setparam(pitem, p);

        p = strtok(NULL,"&");
        }
      }
    }
  
  interprete(cookie_wpwd, path);
}

/*------------------------------------------------------------------*/

void decode_post(char *string, char *boundary, int length, char *cookie_wpwd)
{
char *pinit, *p, *pitem, *ptmp, file_name[256], str[256];
int  n;

  initparam();
  _attachment_size[0] = _attachment_size[1] = _attachment_size[2] = 0;

  pinit = string;

  /* return if no boundary defined */
  if (!boundary[0])
    return;

  if (strstr(string, boundary))
    string = strstr(string, boundary) + strlen(boundary);

  do
    {
    if (strstr(string, "name="))
      {
      pitem = strstr(string, "name=")+5;
      if (*pitem == '\"')
        pitem++;

      if (strncmp(pitem, "attfile", 7) == 0)
        {
        n = pitem[7] - '1';

        /* evaluate file attachment */
        if (strstr(pitem, "filename="))
          p = strstr(pitem, "filename=")+9;
        if (*p == '\"')
          p++;
        if (strstr(p, "\r\n\r\n"))
          string = strstr(p, "\r\n\r\n")+4;
        else if (strstr(p, "\r\r\n\r\r\n"))
          string = strstr(p, "\r\r\n\r\r\n")+6;
        if (strchr(p, '\"'))
          *strchr(p, '\"') = 0;

        /* set attachment filename */
        strcpy(file_name, p);
        sprintf(str, "attachment%d", n);
        setparam(str, file_name);

        /* find next boundary */
        ptmp = string;
        do
          {
          while (*ptmp != '-')
            ptmp++;

          if ((p = strstr(ptmp, boundary)) != NULL)
            {
            while (*p == '-')
              p--;
            if (*p == 10)
              p--;
            if (*p == 13)
              p--;
            p++;
            break;
            }
          else
            ptmp += strlen(ptmp);

          } while (TRUE);

        /* save pointer to file */
        if (file_name[0])
          {
          _attachment_buffer[n] = string;
          _attachment_size[n] = (INT)p - (INT) string; 
          }

        string = strstr(p, boundary) + strlen(boundary);
        }
      else
        {
        p = pitem;
        if (strstr(p, "\r\n\r\n"))
          p = strstr(p, "\r\n\r\n")+4;
        else if (strstr(p, "\r\r\n\r\r\n"))
          p = strstr(p, "\r\r\n\r\r\n")+6;

        if (strchr(pitem, '\"'))
          *strchr(pitem, '\"') = 0;

        if (strstr(p, boundary))
          {
          string = strstr(p, boundary) + strlen(boundary);
          *strstr(p, boundary) = 0;
          ptmp = p + (strlen(p) - 1);
          while (*ptmp == '-' || *ptmp == '\n' || *ptmp == '\r')
            *ptmp-- = 0;
          }
        setparam(pitem, p);
        }

      while (*string == '-' || *string == '\n' || *string == '\r')
        string++;
      }

    } while ((INT)string - (INT)pinit < length);
  
  interprete(cookie_wpwd, "");
}

/*------------------------------------------------------------------*/

BOOL _abort = FALSE;

void ctrlc_handler(int sig)
{
  _abort = TRUE;
}

/*------------------------------------------------------------------*/

char net_buffer[WEB_BUFFER_SIZE];

void server_loop(int tcp_port, int daemon)
{
int                  status, i, n_error, authorized;
struct sockaddr_in   bind_addr, acc_addr;
char                 host_name[256], pwd[256], str[256], cl_pwd[256], *p;
char                 cookie_wpwd[256], boundary[256];
int                  lsock, len, flag, content_length, header_length;
struct hostent       *phe;
struct linger        ling;
fd_set               readfds;
struct timeval       timeout;
INT                  last_time=0;

#ifdef OS_WINNT
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return;
  }
#endif

  /* create a new socket */
  lsock = socket(AF_INET, SOCK_STREAM, 0);

  if (lsock == -1)
    {
    printf("Cannot create socket\n");
    return;
    }

  /* bind local node name and port to socket */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = htons((short) tcp_port);

  gethostname(host_name, sizeof(host_name));

  phe = gethostbyname(host_name);
  if (phe == NULL)
    {
    printf("Cannot retrieve host name\n");
    return;
    }
  phe = gethostbyaddr(phe->h_addr, sizeof(int), AF_INET);
  if (phe == NULL)
    {
    printf("Cannot retrieve host name\n");
    return;
    }

  /* if domain name is not in host name, hope to get it from phe */
  if (strchr(host_name, '.') == NULL)
    strcpy(host_name, phe->h_name);

  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);

  status = bind(lsock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    /* try reusing address */
    flag = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR,
               (char *) &flag, sizeof(INT));
    status = bind(lsock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

    if (status < 0)
      {
      printf("Cannot bind to port %d.\nPlease try later or use the \"-p\" flag to specify a different port\n", tcp_port);
      return;
      }
    else
      printf("Warning: port %d already in use\n", tcp_port);
    }


  /* open configuration file */
  getcfg("dummy", "dummy", str);

#ifdef OS_UNIX
  /* give up root privilege */
  setuid(getuid());
  setgid(getgid());
#endif

  if (daemon)
    {
    printf("Becoming a daemon...\n");
    ss_daemon_init();
    }

  /* listen for connection */
  status = listen(lsock, SOMAXCONN);
  if (status < 0)
    {
    printf("Cannot listen\n");
    return;
    }

  /* set my own URL */
  if (tcp_port == 80)
    sprintf(elogd_url, "http://%s/", host_name);
  else
    sprintf(elogd_url, "http://%s:%d/", host_name, tcp_port);

  printf("Server listening...\n");
  do
    {
    FD_ZERO(&readfds);
    FD_SET(lsock, &readfds);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 100000;

    status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

    if (FD_ISSET(lsock, &readfds))
      {
      len = sizeof(acc_addr);
      _sock = accept( lsock,(struct sockaddr *) &acc_addr, &len);

      last_time = (INT) time(NULL);

      /* turn on lingering (borrowed from NCSA httpd code) */
      ling.l_onoff = 1;
      ling.l_linger = 600;
      setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));

      /* save remote host address */
      memcpy(&remote_addr, &(acc_addr.sin_addr), sizeof(remote_addr));

      memset(net_buffer, 0, sizeof(net_buffer));
      len = 0;
      header_length = 0;
      n_error = 0;
      do
        {
        FD_ZERO(&readfds);
        FD_SET(_sock, &readfds);

        timeout.tv_sec  = 6;
        timeout.tv_usec = 0;

        status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

        if (FD_ISSET(_sock, &readfds))
          i = recv(_sock, net_buffer+len, sizeof(net_buffer)-len, 0);
        else
          goto error;
      
        /* abort if connection got broken */
        if (i<0)
          goto error;

        if (i>0)
          len += i;

        if (i == 0)
          {
          n_error++;
          if (n_error == 100)
            goto error;
          }

        /* finish when empty line received */
        if (strstr(net_buffer, "GET") != NULL && strstr(net_buffer, "POST") == NULL)
          {
          if (len>4 && strcmp(&net_buffer[len-4], "\r\n\r\n") == 0)
            break;
          if (len>6 && strcmp(&net_buffer[len-6], "\r\r\n\r\r\n") == 0)
            break;
          }
        else if (strstr(net_buffer, "POST") != NULL)
          {
          if (header_length == 0)
            {
            /* extract logbook */
            strncpy(str, net_buffer+6, 32);
            if (strstr(str, "HTTP"))
              *(strstr(str, "HTTP")-1) = 0;
            strcpy(logbook, str);
            
            /* extract header and content length */
            if (strstr(net_buffer, "Content-Length:"))
              content_length = atoi(strstr(net_buffer, "Content-Length:") + 15);
            boundary[0] = 0;
            if (strstr(net_buffer, "boundary="))
              {
              strncpy(boundary, strstr(net_buffer, "boundary=")+9, sizeof(boundary));
              if (strchr(boundary, '\r'))
                *strchr(boundary, '\r') = 0;
              }

            if (strstr(net_buffer, "\r\n\r\n"))
              header_length = (INT)strstr(net_buffer, "\r\n\r\n") - (INT)net_buffer + 4;

            if (strstr(net_buffer, "\r\r\n\r\r\n"))
              header_length = (INT)strstr(net_buffer, "\r\r\n\r\r\n") - (INT)net_buffer + 6;

            if (header_length)
              net_buffer[header_length-1] = 0;
            }

          if (header_length > 0 && len >= header_length+content_length)
            break;
          }
        else if (strstr(net_buffer, "OPTIONS") != NULL)
          goto error;
        else
          {
          printf(net_buffer);
          goto error;
          }

        } while (1);

      if (!strchr(net_buffer, '\r'))
        goto error;

      /* extract cookies */
      cookie_wpwd[0] = 0;
      if (strstr(net_buffer, "elog_wpwd=") != NULL)
        {
        strcpy(cookie_wpwd, strstr(net_buffer, "elog_wpwd=")+10);
        cookie_wpwd[strcspn(cookie_wpwd, " ;\r\n")] = 0;
        }

      memset(return_buffer, 0, sizeof(return_buffer));

      if (strncmp(net_buffer, "GET", 3) != 0 &&
          strncmp(net_buffer, "POST", 4) != 0)
        goto error;
      
      return_length = 0;

      /* extract logbook */
      if (strncmp(net_buffer, "GET", 3) == 0)
        {
        p = net_buffer+5;
        logbook[0] = 0;
        for (i=0 ; *p && *p != '/' && *p != '?' && *p != ' '; i++)
          logbook[i] = *p++;
        logbook[i] = 0;
        }
      
      /* ask for password if configured */
      authorized = 1;
      if (getcfg(logbook, "Read Password", pwd))
        {
        authorized = 0;

        /* decode authorization */
        if (strstr(net_buffer, "Authorization:"))
          {
          p = strstr(net_buffer, "Authorization:")+14;
          if (strstr(p, "Basic"))
            {
            p = strstr(p, "Basic")+6;
            while (*p == ' ')
              p++;
            i = 0;
            while (*p && *p != ' ' && *p != '\r')
              str[i++] = *p++;
            str[i] = 0;
            }
          base64_decode(str, cl_pwd);
          if (strchr(cl_pwd, ':'))
            {
            p = strchr(cl_pwd, ':')+1;
            base64_encode(p, str);
            strcpy(cl_pwd, str);

            /* check authorization */
            if (strcmp(str, pwd) == 0)
              authorized = 1;
            }
          }
        }

      if (!authorized)
        {
        /* return request for authorization */
        rsprintf("HTTP/1.1 401 Authorization Required\r\n");
        rsprintf("Server: ELOG HTTPD 1.0\r\n");
        rsprintf("WWW-Authenticate: Basic realm=\"%s\"\r\n", logbook);
        rsprintf("Connection: close\r\n");
        rsprintf("Content-Type: text/html\r\n\r\n");

        rsprintf("<HTML><HEAD>\r\n");
        rsprintf("<TITLE>401 Authorization Required</TITLE>\r\n");
        rsprintf("</HEAD><BODY>\r\n");
        rsprintf("<H1>Authorization Required</H1>\r\n");
        rsprintf("This server could not verify that you\r\n");
        rsprintf("are authorized to access the document\r\n");
        rsprintf("requested. Either you supplied the wrong\r\n");
        rsprintf("credentials (e.g., bad password), or your\r\n");
        rsprintf("browser doesn't understand how to supply\r\n");
        rsprintf("the credentials required.<P>\r\n");
        rsprintf("</BODY></HTML>\r\n");
        }
      else
        {

        if (verbose)
          printf("\n\n\n%s\n", net_buffer);

        if (strncmp(net_buffer, "GET", 3) == 0)
          {
          /* extract path and commands */
          *strchr(net_buffer, '\r') = 0;
  
          if (!strstr(net_buffer, "HTTP"))
            goto error;
          *(strstr(net_buffer, "HTTP")-1)=0;

          /* skip logbook from path */
          p = net_buffer+5;
          for (i=0 ; *p && *p != '/' && *p != '?'; p++);
          while (*p && *p == '/')
            p++;

          /* decode command and return answer */
          decode_get(p, cookie_wpwd);
          }
        else if (strncmp(net_buffer, "POST", 4) == 0)
          {
          decode_post(net_buffer+header_length, boundary, content_length, cookie_wpwd);
          }
        else
          {
          net_buffer[50] = 0;
          sprintf(str, "Unknown request:<p>%s", net_buffer);
          show_error(str);
          }
        }

      if (return_length != -1)
        {
        if (return_length == 0)
          return_length = strlen(return_buffer)+1;

        send(_sock, return_buffer, return_length, 0);

  error:

        closesocket(_sock);
        }
      }

    } while (!_abort);
}

/*------------------------------------------------------------------*/

void create_password(char *logbook, char *name, char *pwd)
{
int  fh, length, i;
char *cfgbuffer, str[256], *p;

  fh = open(cfg_file, O_RDONLY);
  if (fh < 0)
    {
    /* create new file */
    fh = open(cfg_file, O_CREAT | O_WRONLY, 0640);
    if (fh < 0)
      {
      printf("Cannot create \"%d\".\n", cfg_file);
      return;
      }
    sprintf(str, "[%s]\n%s=%s\n", logbook, name, pwd);
    write(fh, str, strlen(str));
    close(fh);
    printf("File \"%s\" created with password in loogbook \"%s\".\n", cfg_file, logbook);
    return;
    }

  /* read existing file and add password */
  length = lseek(fh, 0, SEEK_END);
  lseek(fh, 0, SEEK_SET);
  cfgbuffer = malloc(length+1);
  if (cfgbuffer == NULL)
    {
    close(fh);
    return;
    }
  length = read(fh, cfgbuffer, length);
  cfgbuffer[length] = 0;
  close(fh);

  fh = open(cfg_file, O_TRUNC | O_WRONLY, 0640);

  sprintf(str, "[%s]", logbook);
  
  /* check if logbook exists already */
  if (strstr(cfgbuffer, str))
    {
    p = strstr(cfgbuffer, str);

    /* search password in current logbook */
    do 
      {
      while (*p && *p != '\n')
        p++;
      if (*p && *p == '\n')
        p++;

      if (strncmp(p, name, strlen(name)) == 0)
        {
        /* replace existing password */
        i = (int) p - (int) cfgbuffer;
        write(fh, cfgbuffer, i);
        sprintf(str, "%s=%s\n", name, pwd);
        write(fh, str, strlen(str));

        printf("Password replaced in loogbook \"%s\".\n", logbook);

        while (*p && *p != '\n')
          p++;
        if (*p && *p == '\n')
          p++;

        /* write remainder of file */
        write(fh, p, strlen(p));

        free(cfgbuffer);
        close(fh);
        return;
        }

      } while (*p && *p != '[');

    if (!*p || *p == '[')
      {
      /* enter password into current logbook */
      p = strstr(cfgbuffer, str);
      while (*p && *p != '\n')
        p++;
      if (*p && *p == '\n')
        p++;
      
      i = (int) p - (int) cfgbuffer;
      write(fh, cfgbuffer, i);
      sprintf(str, "%s=%s\n", name, pwd);
      write(fh, str, strlen(str));

      printf("Password added to loogbook \"%s\".\n", logbook);

      /* write remainder of file */
      write(fh, p, strlen(p));

      free(cfgbuffer);
      close(fh);
      return;
      }
    }
  else /* write new logbook entry */
    {
    write(fh, cfgbuffer, strlen(cfgbuffer));
    sprintf(str, "\n[%s]\n%s=%s\n\n", logbook, name, pwd);
    write(fh, str, strlen(str));

    printf("Password added to new loogbook \"%s\".\n", logbook);
    }

  free(cfgbuffer);
  close(fh);
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
int i;
int tcp_port = 80, daemon = FALSE;
char read_pwd[80], write_pwd[80], str[80];

  read_pwd[0] = write_pwd[0] = logbook[0] = 0;

  strcpy(cfg_file, "elogd.cfg");

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'v')
      verbose = TRUE;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'p')
        tcp_port = atoi(argv[++i]);
      else if (argv[i][1] == 'c')
        strcpy(cfg_file, argv[++i]);
      else if (argv[i][1] == 'r')
        strcpy(read_pwd, argv[++i]);
      else if (argv[i][1] == 'w')
        strcpy(write_pwd, argv[++i]);
      else if (argv[i][1] == 'l')
        strcpy(logbook, argv[++i]);
      else
        {
usage:
        printf("usage: %s [-p port] [-D] [-c file] [-r pwd] [-w pwd] [-l loggbook]\n\n", argv[0]);
        printf("       -p <port> TCP/IP port\n");
        printf("       -D become a daemon\n");
        printf("       -c <file> specify configuration file\n");
        printf("       -v debugging output\n");
        printf("       -r create/overwrite read password in config file\n");
        printf("       -w create/overwrite write password in config file\n");
        printf("       -l <loogbook> specify logbook for -r and -w commands\n\n");
        return 0;
        }
      }
    }
  
  if (read_pwd[0])
    {
    if (!logbook[0])
      {
      printf("Must specify a lookbook via the -l parameter.\n");
      return 0;
      }
    base64_encode(read_pwd, str);
    create_password(logbook, "Read Password", str);
    return 0;
    }

  if (write_pwd[0])
    {
    if (!logbook[0])
      {
      printf("Must specify a lookbook via the -l parameter.\n");
      return 0;
      }
    base64_encode(write_pwd, str);
    create_password(logbook, "Write Password", str);
    return 0;
    }

  server_loop(tcp_port, daemon);

  return 0;
}

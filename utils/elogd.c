/********************************************************************\

  Name:         elogd.c
  Created by:   Stefan Ritt

  Contents:     Web server program for Electronic Logbook ELOG

  $Log$
  Revision 1.43  2001/10/12 10:46:05  midas
  Fixed bug which caused problems with the "find" function

  Revision 1.42  2001/10/12 07:18:47  midas
  - Fixed CRLF problem with sendmail, thanks to Michael Jones
  - Fixed problem that first logbook page can be viewed even with read PW
  - Fixed problem that passwords only worked with -k flag

  Revision 1.41  2001/10/09 10:39:55  midas
  Version 1.1.2

  Revision 1.40  2001/10/05 13:35:50  midas
  Implemented keep-alive

  Revision 1.39  2001/08/30 10:58:02  midas
  Fixed bugs with Opera browser

  Revision 1.38  2001/08/29 07:53:49  midas
  Use smaller font for printable search result display

  Revision 1.37  2001/08/28 11:41:49  midas
  Optimized attachment display

  Revision 1.36  2001/08/28 11:36:18  midas
  Treat configuration directory correctly

  Revision 1.35  2001/08/28 10:42:58  midas
  Extract path for theme file from configuration file

  Revision 1.34  2001/08/28 10:20:59  midas
  Changed default colors

  Revision 1.33  2001/08/28 08:32:00  midas
  Major changes for revision 1.1.0

  Revision 1.32  2001/08/14 09:37:22  midas
  Mail notification did not contain host name. Fixed.

  Revision 1.31  2001/08/09 13:51:42  midas
  Added "suppress default" flag

  Revision 1.30  2001/08/09 07:44:31  midas
  Changed from absolute pathnames to relative pathnames

  Revision 1.29  2001/08/08 14:26:44  midas
  Fixed bug which crashes server on long (>80 chars) attachments

  Revision 1.28  2001/08/08 11:02:11  midas
  Added separate password for message deletion
  Authors are displayed in a drop-down box on the query page
  A button "suppress Email notification" has been added

  Revision 1.27  2001/08/07 13:26:27  midas
  Increase size of author, type and cat. list to 100

  Revision 1.26  2001/08/07 11:01:44  midas
  Fixed bugs with strtok

  Revision 1.25  2001/08/07 08:03:40  midas
  Fixed bug in el_retrieve in decoding attachments

  Revision 1.24  2001/08/07 07:10:03  midas
  Fixed problem with emails

  Revision 1.23  2001/08/06 12:44:15  midas
  Fixed small bug preventing the selection list for multiple logbooks

  Revision 1.22  2001/08/03 12:06:12  midas
  Fixed bug where categories were wrong when multiple logbooks are used

  Revision 1.21  2001/08/03 06:44:48  midas
  Changed mail notification author from "name@host" to "name from host"

  Revision 1.20  2001/08/02 12:30:00  midas
  Fixed unix bug

  Revision 1.19  2001/08/02 12:26:43  midas
  Fine-tunes mail facility, fixed a few bugs

  Revision 1.18  2001/08/02 11:45:47  midas
  Added mail facility and author list

  Revision 1.17  2001/07/26 10:37:32  midas
  Removed "status" button on query result

  Revision 1.16  2001/07/26 10:25:54  midas
  Fixed bug which omitted logbook name if only one is defined in elogd.cfg

  Revision 1.15  2001/07/26 09:31:07  midas
  Added URL = entry in elogd.cfg to support secure connections over stunnel

  Revision 1.14  2001/07/24 10:46:09  midas
  Use subject as title, make http://xxx text aktive links

  Revision 1.13  2001/07/06 12:34:13  midas
  Incresed cookie expiration from 1h to 1d

  Revision 1.12  2001/06/20 08:33:14  midas
  Added "summary on last"

  Revision 1.11  2001/06/19 10:55:40  midas
  Variable number of attachments, revised attachment editing

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
#define VERSION "1.1.2"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define OS_WINNT

#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_STR "\\"

#define snprintf _snprintf

#include <windows.h>
#include <io.h>
#include <time.h>
#else

#define OS_UNIX

#define TRUE 1
#define FALSE 0

#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_STR "/"

typedef int BOOL;
typedef unsigned long int DWORD;

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>

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
char header_buffer[1000];
int  return_length;
char host_name[256];
char elogd_url[256];
char elogd_full_url[256];
char loogbook[32];
char logbook[32];
char logbook_enc[40];
char data_dir[256];
char cfg_file[256];
char cfg_dir[256];

#define MAX_GROUPS       32
#define MAX_PARAM       100
#define MAX_ATTACHMENTS   5
#define MAX_N_LIST      100
#define VALUE_SIZE      256
#define PARAM_LENGTH    256
#define TEXT_SIZE     50000
#define MAX_PATH_LENGTH 256

char _param[MAX_PARAM][PARAM_LENGTH];
char _value[MAX_PARAM][VALUE_SIZE];
char _text[TEXT_SIZE];
char *_attachment_buffer[MAX_ATTACHMENTS];
INT  _attachment_size[MAX_ATTACHMENTS];
struct in_addr rem_addr;
INT  _sock;
BOOL verbose, use_keepalive;

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

char type_list[MAX_N_LIST][NAME_LENGTH] = {
  "Routine",
  "Other"
};

char category_list[MAX_N_LIST][NAME_LENGTH] = {
  "General",
  "Other",
};

char author_list[MAX_N_LIST][NAME_LENGTH] = {
  ""
};

struct {
  char ext[32];
  char type[32];
  } filetype[] = {

  ".JPG",   "image/jpeg",     
  ".GIF",   "image/gif",
  ".PNG",   "image/png",
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

INT recv_string(int sock, char *buffer, DWORD buffer_size, INT millisec)
{
INT            i;
DWORD          n;
fd_set         readfds;
struct timeval timeout;

  n = 0;
  memset(buffer, 0, buffer_size);

  do
    {
    if (millisec > 0)
      {
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec  = millisec / 1000;
      timeout.tv_usec = (millisec % 1000) * 1000;

      select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

      if (!FD_ISSET(sock, &readfds))
        break;
      }

    i = recv(sock, buffer+n, 1, 0);

    if (i<=0)
      break;

    n++;

    if (n >= buffer_size)
      break;

    } while (buffer[n-1] && buffer[n-1] != 10);

  return n-1;
}


INT sendmail(char *smtp_host, char *from, char *to, char *subject, char *text)
{
struct sockaddr_in   bind_addr;
struct hostent       *phe;
int                  s;
char                 buf[80];
char                 str[10000];
time_t               now;

  /* create a new socket for connecting to remote server */
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1)
    return -1;

  /* connect to remote node port 25 */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_port        = htons((short) 25);

  phe = gethostbyname(smtp_host); 
  if (phe == NULL)
    return -1;
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);

  if (connect(s, (void *) &bind_addr, sizeof(bind_addr)) < 0)
    {
    closesocket(s);
    return -1;
    }

  recv_string(s, str, sizeof(str), 3000);

  snprintf(str, sizeof(str) - 1, "MAIL FROM: <%s>\r\n", from);
  send(s, str, strlen(str), 0);
  recv_string(s, str, sizeof(str), 3000);

  snprintf(str, sizeof(str) - 1, "RCPT TO: <%s>\r\n", to);
  send(s, str, strlen(str), 0);
  recv_string(s, str, sizeof(str), 3000);

  snprintf(str, sizeof(str) - 1, "DATA\r\n");
  send(s, str, strlen(str), 0);
  recv_string(s, str, sizeof(str), 3000);

  snprintf(str, sizeof(str) - 1, "To: %s\r\nFrom: %s\r\nSubject: %s\r\n", to, from, subject);
  send(s, str, strlen(str), 0);

  time(&now);
  snprintf(buf, sizeof(buf) - 1, "%s", ctime(&now));
  buf[strlen(buf)-1] = '\0';
  snprintf(str, sizeof(str) - 1, "Date: %s\r\n\r\n", buf);
  send(s, str, strlen(str), 0);

  snprintf(str, sizeof(str) - 1, "%s\r\n.\r\n", text);
  send(s, str, strlen(str), 0);
  recv_string(s, str, sizeof(str), 3000);

  snprintf(str, sizeof(str) - 1, "QUIT\r\n");
  send(s, str, strlen(str), 0);
  recv_string(s, str, sizeof(str), 3000);

  closesocket(s);
  
  return 1;
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
char str[10000], *p, *pstr;
int  length;
int  fh;

  value[0] = 0;

  /* read configuration file on init */
  if (!cfgbuffer)
    {
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
char str[10000], *p, *pstr;
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
char str[10000], *p, *pstr;
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

/* Parameter extraction from themes file  */

typedef struct {
  char name[32];
  char value[256];
  } THEME;

THEME default_theme [] = {

  "Table width",            "100%",      
  "Frame color",            "#486090",   
  "Cell BGColor",           "#E6E6E6",   
  "Border width",           "0",         
                                         
  "Title BGColor",          "#486090",   
  "Title Fontcolor",        "#FFFFFF",   
  "Title Cellpadding",      "0",         
  "Title Image",            "elog.gif",          
                                         
  "Merge menus",            "1",
  "Use buttons",            "0",

  "Menu1 BGColor",          "#E0E0E0",   
  "Menu1 Align",            "left",      
  "Menu1 Cellpadding",      "0",         
                                         
  "Menu2 BGColor",          "#FFFFB0",   
  "Menu2 Align",            "center",      
  "Menu2 Cellpadding",      "0",         
  "Menu2 use images",       "0",
  
  "Categories cellpadding", "3",
  "Categories border",      "0",
  "Categories BGColor1",    "#CCCCFF",
  "Categories BGColor2",    "#DDEEBB",

  "Text BGColor",           "#FFFFFF",

  "List bgcolor1",          "#DDEEBB",
  "List bgcolor2",          "#FFFFB0",

  ""

};

THEME *theme = NULL;
char  theme_name[80];

/*------------------------------------------------------------------*/

void loadtheme(char *tn)
{
FILE *f;
char *p, line[256], item[256], value[256], file_name[256];
int  i;

  if (theme == NULL)
    theme = malloc(sizeof(default_theme));

  /* copy default theme */
  memcpy(theme, default_theme, sizeof(default_theme));

  /* use default if no name is given */
  if (tn == NULL)
    return;

  memset(file_name, 0, sizeof(file_name));

  strcpy(file_name, cfg_dir);
  strcat(file_name, tn);
  file_name[strlen(file_name)] = DIR_SEPARATOR;
  strcat(file_name, "theme.cfg");

  f = fopen(file_name, "r");
  if (f == NULL)
    return;
 
  strcpy(theme_name, tn);

  do
    {
    line[0] = 0;
    fgets(line, sizeof(line), f);

    /* ignore comments */
    if (line[0] == ';' || line[0] == '#')
      continue;

    if (strchr(line, '='))
      {
      /* split line into item and value, strop blanks */
      strcpy(item, line);
      p = strchr(item, '=');
      while ((*p == '=' || *p == ' ') && p > item)
        *p-- = 0;
      p = strchr(line, '=');
      while (*p && (*p == '=' || *p == ' '))
        p++;
      strcpy(value, p);
      if (strlen(value) > 0)
        {
        p = value+strlen(value)-1;
        while (p >= value && (*p == ' ' || *p == '\r' || *p == '\n'))
          *(p--) = 0;
        }

      /* find field in _theme and set it */
      for (i=0 ; theme[i].name[0] ; i++)
        if (equal_ustring(item, theme[i].name))
          {
          strcpy(theme[i].value, value);
          break;
          }
      }

    } while (!feof(f));

  fclose(f);

  return; 
}

/*------------------------------------------------------------------*/

/* get a certain theme attribute from currently loaded file */

char *gt(char *name)
{
int i;

  for (i=0 ; theme[i].name[0] ; i++)
    if (equal_ustring(theme[i].name, name))
      return theme[i].value;

  return "";
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

INT ss_file_find(char * path, char * pattern, char **plist)
/********************************************************************\

  Routine: ss_file_find

  Purpose: Return list of files matching 'pattern' from the 'path' location 

  Input:
    char  *path             Name of a file in file system to check
    char  *pattern          pattern string (wildcard allowed)

  Output:
    char  **plist           pointer to the lfile list 

  Function value:
    int                     Number of files matching request

\********************************************************************/
{
  int i, first;
  char str[255];

#ifdef OS_UNIX
  DIR *dir_pointer;
  struct dirent *dp;
  
  if ((dir_pointer = opendir(path)) == NULL)
    return 0;
  *plist = (char *) malloc(MAX_PATH_LENGTH);
  i = 0;
  for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer))
    {
    if (fnmatch (pattern, dp->d_name, 0) == 0)
      {
      *plist = (char *)realloc(*plist, (i+1)*MAX_PATH_LENGTH);
      strncpy(*plist+(i*MAX_PATH_LENGTH), dp->d_name, strlen(dp->d_name)); 
      *(*plist+(i*MAX_PATH_LENGTH)+strlen(dp->d_name)) = '\0';
      i++;
      seekdir(dir_pointer, telldir(dir_pointer));
      }
    }
  closedir(dir_pointer); 
#endif

#ifdef OS_WINNT
  HANDLE pffile;
  LPWIN32_FIND_DATA lpfdata;

  strcpy(str,path);
  strcat(str,"\\");
  strcat(str,pattern);
  first = 1;
  i = 0;
  lpfdata = malloc(sizeof(WIN32_FIND_DATA));
  *plist = (char *) malloc(MAX_PATH_LENGTH);
  pffile = FindFirstFile(str, lpfdata);
  if (pffile == INVALID_HANDLE_VALUE)
    return 0;
  first = 0;
	*plist = (char *)realloc(*plist, (i+1)*MAX_PATH_LENGTH);
	strncpy(*plist+(i*MAX_PATH_LENGTH), lpfdata->cFileName, strlen(lpfdata->cFileName));
  *(*plist+(i*MAX_PATH_LENGTH)+strlen(lpfdata->cFileName)) = '\0';
	i++;
  while (FindNextFile(pffile, lpfdata))
    {
	  *plist = (char *)realloc(*plist, (i+1)*MAX_PATH_LENGTH);
	  strncpy(*plist+(i*MAX_PATH_LENGTH), lpfdata->cFileName, strlen(lpfdata->cFileName));
    *(*plist+(i*MAX_PATH_LENGTH)+strlen(lpfdata->cFileName)) = '\0';
	  i++;
    }
  free(lpfdata);
#endif
  return i;
}

/*------------------------------------------------------------------*/

INT el_search_message(char *tag, int *fh, BOOL walk, BOOL first)
{
int    lfh, i, n, d, min, max, size, offset, direction, last, status;
struct tm *tms, ltms;
DWORD  lt, ltime, lact;
char   str[256], file_name[256], dir[256];
char   *file_list;

  tzset();

  /* get data directory */
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
      lfh = open(file_name, O_RDWR | O_BINARY, 0644);

      if (lfh < 0)
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

      /* in backward direction, go back 10 years */
      if (direction == -1 && abs((INT)lt-(INT)ltime) > 3600*24*365*10)
        break;

      } while (lfh < 0);

    if (lfh < 0)
      return EL_FILE_ERROR;

    lseek(lfh, offset, SEEK_SET);

    /* check if start of message */
    i = read(lfh, str, 15);
    if (i <= 0)
      {
      close(lfh);
      return EL_FILE_ERROR;
      }

    if (strncmp(str, "$Start$: ", 9) != 0)
      {
      close(lfh);
      return EL_FILE_ERROR;
      }

    lseek(lfh, offset, SEEK_SET);
    }

  /* open most recent file if no tag given */
  if (tag[0] == 0)
    {
    if (first)
      {
      /* go through whole directory, find first file */

      n = ss_file_find(dir, "??????.log", &file_list);
      if (n == 0)
        return EL_FILE_ERROR;

      for (i=0,min=9999999 ; i<n ; i++)
        {
        d = atoi(file_list+i*MAX_PATH_LENGTH);
        if (d == 0)
          continue;

        if (d < 900000)
          d += 1000000; /* last century */

        if (d < min)
          min = d;
        }
      free(file_list);
      sprintf(file_name, "%s%06d.log", dir, min % 1000000);
      lfh = open(file_name, O_RDWR | O_BINARY, 0644);
      if (lfh < 0)
        return EL_FILE_ERROR;

      /* remember tag */
      sprintf(tag, "%06d.0", min % 1000000);

      /* on tag "+1", don't go to next message */
      if (direction == 1)
        direction = 0;
      }
    else
      {
      /* go through whole directory, find last file */

      n = ss_file_find(dir, "??????.log", &file_list);
      if (n == 0)
        return EL_FILE_ERROR;

      for (i=0,max=0 ; i<n ; i++)
        {
        d = atoi(file_list+i*MAX_PATH_LENGTH);
        if (d < 900000)
          d += 1000000; /* last century */

        if (d > max)
          max = d;
        }
      free(file_list);
      sprintf(file_name, "%s%06d.log", dir, max % 1000000);
      lfh = open(file_name, O_RDWR | O_BINARY, 0644);
      if (lfh < 0)
        return EL_FILE_ERROR;

      lseek(lfh, 0, SEEK_END);

      /* remember tag */
      sprintf(tag, "%06d.%d", max % 1000000, TELL(lfh));
      }
    }

  if (direction == -1)
    {
    /* seek previous message */

    if (TELL(lfh) == 0)
      {
      /* go back one day */
      close(lfh);

      lt = ltime;
      do
        {
        lt -= 3600*24;
        tms = localtime(&lt);
        sprintf(str, "%02d%02d%02d.0",
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

        status = el_search_message(str, &lfh, FALSE, FALSE);

        } while (status != SUCCESS &&
                 (INT)ltime-(INT)lt < 3600*24*365);

      if (status != EL_SUCCESS)
        {
        if (fh)
          *fh = lfh;
        else
          close(lfh);
        return EL_FIRST_MSG;
        }

      /* adjust tag */
      strcpy(tag, str);

      /* go to end of current file */
      lseek(lfh, 0, SEEK_END);
      }

    /* read previous message size */
    lseek(lfh, -17, SEEK_CUR);
    i = read(lfh, str, 17);
    if (i <= 0)
      {
      close(lfh);
      return EL_FILE_ERROR;
      }

    if (strncmp(str, "$End$: ", 7) == 0)
      {
      size = atoi(str+7);
      lseek(lfh, -size, SEEK_CUR);
      }
    else
      {
      close(lfh);
      return EL_FILE_ERROR;
      }

    /* adjust tag */
    sprintf(strchr(tag, '.')+1, "%d", TELL(lfh));
    }

  if (direction == 1)
    {
    /* seek next message */

    /* read current message size */
    last = TELL(lfh);

    i = read(lfh, str, 15);
    if (i <= 0)
      {
      close(lfh);
      return EL_FILE_ERROR;
      }
    lseek(lfh, -15, SEEK_CUR);

    if (strncmp(str, "$Start$: ", 9) == 0)
      {
      size = atoi(str+9);
      lseek(lfh, size, SEEK_CUR);
      }
    else
      {
      close(lfh);
      return EL_FILE_ERROR;
      }

    /* if EOF, goto next day */
    i = read(lfh, str, 15);
    if (i < 15)
      {
      close(lfh);
      time(&lact);

      lt = ltime;
      do
        {
        lt += 3600*24;
        tms = localtime(&lt);
        sprintf(str, "%02d%02d%02d.0",
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

        status = el_search_message(str, &lfh, FALSE, FALSE);

        } while (status != EL_SUCCESS &&
                 (INT)lt-(INT)lact < 3600*24);

      if (status != EL_SUCCESS)
        {
        if (fh)
          *fh = lfh;
        else
          close(lfh);
        return EL_LAST_MSG;
        }

      /* adjust tag */
      strcpy(tag, str);

      /* go to beginning of current file */
      lseek(lfh, 0, SEEK_SET);
      }
    else
      lseek(lfh, -15, SEEK_CUR);

    /* adjust tag */
    sprintf(strchr(tag, '.')+1, "%d", TELL(lfh));
    }

  if (fh)
    *fh = lfh;
  else
    close(lfh);

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_submit(int run, char *author, char *type, char *category, char *subject,
              char *text, char *reply_to, char *encoding,
              char afilename[MAX_ATTACHMENTS][256],
              char *buffer[MAX_ATTACHMENTS], 
              INT buffer_size[MAX_ATTACHMENTS],
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

    char   *afilename[]     File name of attachments
    char   *buffer[]        Attachment contents
    INT    *buffer_size[]   Size of attachment in bytes
    char   *tag             If given, edit existing message
    INT    *tag_size        Maximum size of tag

  Output:
    char   *tag             Message tag in the form YYMMDD.offset
    INT    *tag_size        Size of returned tag

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
INT     n, i, size, fh, status, run_number, index, offset, tail_size;
struct  tm *tms;
char    file_name[256], afile_name[MAX_ATTACHMENTS][256], dir[256], str[256],
        start_str[80], end_str[80], last[80], date[80], thread[80], attachment[256];
time_t  now;
char    message[TEXT_SIZE+100], *p;
BOOL    bedit;

  bedit = (tag[0] != 0);

  /* ignore run number */
  run_number = 0;

  for (index = 0 ; index < MAX_ATTACHMENTS ; index++)
    {
    /* generate filename for attachment */
    afile_name[index][0] = file_name[0] = 0;

    if (equal_ustring(afilename[index], "<delete>"))
      {
      strcpy(afile_name[index], "<delete>");
      }
    else
      {
      if (afilename[index][0])
        {
        /* strip directory, add date and time to filename */
  
        strcpy(file_name, afilename[index]);
        p = file_name;
        while (strchr(p, ':'))
          p = strchr(p, ':')+1;
        while (strchr(p, '\\'))
          p = strchr(p, '\\')+1; /* NT */
        while (strchr(p, '/'))
          p = strchr(p, '/')+1;  /* Unix */

        /* assemble ELog filename */
        if (p[0])
          {
          strcpy(dir, data_dir);

          tzset();

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
            write(fh, buffer[index], buffer_size[index]);
            close(fh);
            }
          }
        }
      }
    }

  /* generate new file name YYMMDD.log in data directory */
  strcpy(dir, data_dir);

  tzset();

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

  if (bedit)
    {
    for (i=n=0 ; i<MAX_ATTACHMENTS ; i++)
      {
      if (i == 0)
        p = strtok(attachment, ",");
      else
        if (p != NULL)
          p = strtok(NULL, ",");

      if (p && (afile_name[i][0] || equal_ustring(afile_name[i], "<delete>")))
        {
        /* delete old attachment */
        strcpy(str, data_dir);
        strcat(str, p);
        remove(str);
        p = NULL;
        }

      if (afile_name[i][0] && !equal_ustring(afile_name[i], "<delete>"))
        {
        if (n == 0)
          {
          sprintf(message+strlen(message), "Attachment: %s", afile_name[i]);
          n++;
          }
        else
          sprintf(message+strlen(message), ",%s", afile_name[i]);
        }

      if (!afile_name[i][0] && p && !equal_ustring(afile_name[i], "<delete>"))
        {
        if (n == 0)
          {
          sprintf(message+strlen(message), "Attachment: %s", p);
          n++;
          }
        else
          sprintf(message+strlen(message), ",%s", p);
        }

      }
    }
  else
    {
    sprintf(message+strlen(message), "Attachment: %s", afile_name[0]);
    for (i=1 ; i<MAX_ATTACHMENTS ; i++)
      if (afile_name[i][0])
        sprintf(message+strlen(message), ",%s", afile_name[i]);
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
      status = el_search_message(last, &fh, FALSE, FALSE);
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
                char attachment[MAX_ATTACHMENTS][256],
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
    char   *attachment[]    File attachments
    char   *encoding        Encoding of message
    int    *size            Actual message text size

  Function value:
    EL_SUCCESS              Successful completion
    EL_LAST_MSG             Last message in log

\********************************************************************/
{
int     i, size, fh, offset, search_status;
char    str[256], *p;
char    message[TEXT_SIZE+100], thread[256], attachment_all[256];

  if (tag[0])
    {
    search_status = el_search_message(tag, &fh, TRUE, FALSE);
    if (search_status != EL_SUCCESS)
      return search_status;
    }
  else
    {
    /* open most recent message */
    strcpy(tag, "-1");
    search_status = el_search_message(tag, &fh, TRUE, FALSE);
    if (search_status != EL_SUCCESS)
      return search_status;
    }

  /* extract message size */
  offset = TELL(fh);
  read(fh, str, 16);
  size = atoi(str+9);

  /* read message */
  memset(message, 0, sizeof(message));
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
  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    if (attachment[i] != NULL)
      attachment[i][0] = 0;

  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    {
    if (attachment[i] != NULL)
      {
      if (i == 0)
        p = strtok(attachment_all, ",");
      else
        p = strtok(NULL, ",");

      if (p != NULL)
        strcpy(attachment[i], p);
      else
        break;
      }
    }

  /* conver thread in reply-to and reply-from */
  if (orig_tag != NULL && reply_tag != NULL)
    {
    p = strtok(thread, " \r");
    if (p != NULL)
      strcpy(orig_tag, p);
    else
      strcpy(orig_tag, "");

    if (p != NULL)
      {
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
    status = el_search_message(tag, &fh, TRUE, FALSE);
    if (status == EL_FIRST_MSG)
      break;
    if (status != EL_SUCCESS)
      return status;
    close(fh);

    if (strchr(tag, '.') != NULL)
      strcpy(strchr(tag, '.'), ".0");

    el_retrieve(tag, NULL, &actual_run, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, NULL);
    } while (actual_run >= run);

  while (actual_run < run)
    {
    strcat(tag, "+1");
    status = el_search_message(tag, &fh, TRUE, FALSE);
    if (status == EL_LAST_MSG)
      break;
    if (status != EL_SUCCESS)
      return status;
    close(fh);

    el_retrieve(tag, NULL, &actual_run, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, NULL);
    }

  strcpy(return_tag, tag);

  if (status == EL_LAST_MSG || status == EL_FIRST_MSG)
    return status;

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_delete_message(char *tag)
/********************************************************************\

  Routine: el_delete_message

  Purpose: Submit an ELog entry

  Input:
    char   *tag             Message tage

  Output:
    <none>

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
INT  i, n, size, fh, offset, tail_size, run, status;
char dir[256], str[256], file_name[256];
char *buffer;
char date[80], author[80], type[80], category[80], subject[256], text[TEXT_SIZE], 
     orig_tag[80], reply_tag[80], attachment[MAX_ATTACHMENTS][256], encoding[80];

  /* get attachments */
  size = sizeof(text);
  status = el_retrieve(tag, date, &run, author, type, category, subject, 
                       text, &size, orig_tag, reply_tag, 
                       attachment,
                       encoding);
  if (status != SUCCESS)
    return EL_FILE_ERROR;

  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    if (attachment[i][0])
      {
      strcpy(str, data_dir);
      strcat(str, attachment[i]);
      remove(str);
      }

  /* generate file name YYMMDD.log in data directory */
  strcpy(dir, data_dir);

  strcpy(str, tag);
  if (strchr(str, '.'))
    {
    offset = atoi(strchr(str, '.')+1);
    *strchr(str, '.') = 0;
    }
  sprintf(file_name, "%s%s.log", dir, str);
  fh = open(file_name, O_RDWR | O_BINARY, 0644);
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
int i, j, k;
char *p, link[256];

  if (strlen(return_buffer) + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    {
    j = strlen(return_buffer);
    for (i=0 ; i<(int)strlen(str) ; i++)
      {
      if (strncmp(str+i, "http://", 7) == 0)
        {
        p = (char *) (str+i+7);
        i += 7;
        for (k=0 ; *p && *p != ' ' && *p != '\n' ; k++,i++)
          link[k] = *p++;
        link[k] = 0;

        sprintf(return_buffer+j, "<a href=\"http://%s\">http://%s</a>", link, link);
        j = strlen(return_buffer);
        }
      else
        switch (str[i])
          {
          case '<': strcat(return_buffer, "&lt;"); j+=4; break;
          case '>': strcat(return_buffer, "&gt;"); j+=4; break;
          default: return_buffer[j++] = str[i];
          }
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

void url_decode(char *p) 
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

void url_encode(char *ps) 
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
  rsprintf("HTTP/1.1 302 Found\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }

  rsprintf("Location: %s%s/%s\r\n\r\n<html>redir</html>\r\n", elogd_url, logbook_enc, path);
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
  rsprintf("HTTP/1.1 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n");
  rsprintf("Pragma: no-cache\r\n");
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }
  rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n\r\n");

  rsprintf("<html><head>\n");
  rsprintf("<title>ELOG Electronic Logbook Help</title>\n");
  rsprintf("</head>\n\n");

  rsprintf("<body>\n");

  rsprintf("<table border=%s width=100%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>", 
            gt("Border width"), gt("Frame color"));
  rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

  rsprintf("<tr><td align=center bgcolor=%s><font size=5 color=%s>ELog Electronic Logbook Help<br>Version %s</font></td></tr>\n",
            gt("Title bgcolor"), gt("Title fontcolor"), VERSION);

  rsprintf("<tr><td bgcolor=#FFFFFF><br>The Electronic Logbook (<i>ELog</i>) can be used to store and retrieve messages\n");
  rsprintf("through a Web interface. Depending on the configuration, the ELog system can host one or more <i>logbooks</i>,\n");
  rsprintf("which are stored in separate sections on the server.<p><br></td></tr>\n");
    
  rsprintf("<tr><td bgcolor=%s><font color=%s><b>Quick overview</b></font></td></tr>\n", gt("Title bgcolor"), gt("Title fontcolor"));

  rsprintf("<tr><td bgcolor=#FFFFFF>Per default, the last entry in a logbook is displayed. One can use the browse buttons\n");
  rsprintf("to display the first, previous, next and last message.<p>\n");

  rsprintf("If one of the checkboxes next to the <b>Author, Type, Category</b> or <b>Subject</b> fields is checked, only\n");
  rsprintf("messages of that type are displayed with the browse buttons. This can be used as a quick filter to display\n");
  rsprintf("only pages from a certain author or type.<p>\n");

  rsprintf("The <b>New</b> button creates a new entry. With the <b>Edit</b> button one can edit an existing message,\n");
  rsprintf("if this is allowed in the configuration file. The <b>Reply</b> button creates a reply to an existing message,\n");
  rsprintf("similar like a reply to an email.<p>\n");

  rsprintf("The <b>Find</b> button opens a query page, where messages from a logbook can be displayed based\n");
  rsprintf("on filter rules. Each non-empty field works like an additional filter, which is and-ed with the other rules.\n");
  rsprintf("If no filter is selected, all messages from a logbook are displayed. The <b>Last day</b> and <b>Last 10</b>\n");
  rsprintf("buttons display all messages from the last 24 hours and the last ten messages, respectively.<p><br>\n");

  rsprintf("</td></tr>\n");
  rsprintf("<tr><td bgcolor=%s><font color=%s><b>More information</b></font></td></tr>\n", gt("Title bgcolor"), gt("Title fontcolor"));

  rsprintf("<tr><td bgcolor=#FFFFFF>For more information, especially about the configuration of ELog, refer to the\n"); 
  rsprintf("<A HREF=\"http://midas.psi.ch/elog\">ELOG home page</A>.<P><br>\n\n");

  rsprintf("</td></tr></table></td></tr></table>\n\n");

  rsprintf("<hr>\n");
  rsprintf("<address>\n");
  rsprintf("<a href=\"http://midas.psi.ch/~stefan\">S. Ritt</a>, 28 August 2001");
  rsprintf("</address>");

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_standard_header(char *title, char *path)
{
  rsprintf("HTTP/1.1 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  rsprintf("Content-Type: text/html\r\n");
  rsprintf("Pragma: no-cache\r\n");
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }
  rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n\r\n");

  rsprintf("<html><head><title>%s</title></head>\n", title);
  if (path)
    rsprintf("<body bgcolor=#FFFFFF><form method=\"GET\" action=\"%s%s/%s\">\n\n", elogd_url, logbook_enc, path);
  else
    rsprintf("<body bgcolor=#FFFFFF><form method=\"GET\" action=\"%s%s\">\n\n", elogd_url, logbook_enc);
}

/*------------------------------------------------------------------*/

void show_standard_title(char *logbook, char *text, int printable)
{
char str[256], ref[256];
int  i;

  /* overall table, containing title, menu and message body */
  if (printable)
    rsprintf("<table width=600 border=%s cellpadding=0 cellspacing=0 bgcolor=%s>\n\n",
             gt("Border width"), gt("Frame color"));
  else
    rsprintf("<table width=%s border=%s cellpadding=0 cellspacing=0 bgcolor=%s>\n\n",
             gt("Table width"), gt("Border width"), gt("Frame color"));

  /*---- loogbook selection row ----*/

  if (!printable && getcfg("global", "logbook tabs", str) && atoi(str) == 1)
    {
    if (!getcfg("global", "tab cellpadding", str))
      strcpy(str, "5");
    rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=0 bgcolor=#FFFFFF><tr>\n", str);

    for (i=0 ;  ; i++)
      {
      if (!enumgrp(i, str))
        break;

      if (equal_ustring(str, "global"))
        continue;

      strcpy(ref, str);
      url_encode(ref);

      if (equal_ustring(str, logbook))
        rsprintf("<td nowrap bgcolor=%s><font color=%s>%s</font></td>\n", gt("Title BGColor"), gt("Title fontcolor"), str);
      else
        rsprintf("<td nowrap bgcolor=#E0E0E0><a href=\"%s%s\">%s</a></td>\n", elogd_url, ref, str);
      rsprintf("<td width=10 bgcolor=#FFFFFF>&nbsp;</td>\n"); 
      }
    rsprintf("<td width=100%% bgcolor=#FFFFFF>&nbsp;</td>\n"); 
    rsprintf("</tr></table></td></tr>\n\n");
    }
  
  /*---- title row ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=0 bgcolor=#FFFFFF>\n",
            gt("Title cellpadding"));

  /* left cell */
  rsprintf("<tr><td bgcolor=%s align=left>", gt("Title BGColor"));
  rsprintf("<font size=3 face=verdana,arial,helvetica color=%s><b>&nbsp;&nbsp;Logbook \"%s\"%s<b></font>\n", 
            gt("Title fontcolor"), logbook, text);
  rsprintf("&nbsp;</td>\n");
  
  /* right cell */
  rsprintf("<td bgcolor=%s align=right>", gt("Title BGColor"));
  if (*gt("Title image"))
    rsprintf("<img src=\"%s%s/%s\">", elogd_url, logbook_enc, gt("Title image"));
  else
    rsprintf("<font size=3 face=verdana,arial,helvetica color=%s><b>ELOG V%s&nbsp;&nbsp</b></font>", 
              gt("Title fontcolor"), VERSION);
  rsprintf("</td>\n");

  rsprintf("</tr></table></td></tr>\n\n");
}

/*------------------------------------------------------------------*/

void show_error(char *error)
{
  /* header */
  rsprintf("HTTP/1.1 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG error</title></head>\n");
  rsprintf("<body><i>%s</i></body></html>\n", error);
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
      case ' ': rsprintf("&nbsp;"); break;
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

void show_elog_new(char *path, BOOL bedit)
{
int    i, size, run_number, wrap;
char   str[256], *p, list[10000];
char   date[80], author[80], type[80], category[80], subject[256], text[TEXT_SIZE], 
       orig_tag[80], reply_tag[80], att[MAX_ATTACHMENTS][256], encoding[80];
time_t now;
BOOL   allow_edit;

  allow_edit = TRUE;

  /* get flag from configuration file */
  if (getcfg(logbook, "Allow edit", str))
    allow_edit= atoi(str);

  /* get message for reply */
  type[0] = category[0] = 0;
  
  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    att[i][0] = 0;
  subject[0] = 0;
  author[0] = 0;
  type[0] = 0;
  category[0] = 0;

  if (path)
    {
    strcpy(str, path);
    size = sizeof(text);
    el_retrieve(str, date, &run_number, author, type, category, subject, 
                text, &size, orig_tag, reply_tag, att, encoding);
    }
  else
    {
    strcpy(author, getparam("pauthor"));
    strcpy(type, getparam("ptype"));
    strcpy(category , getparam("pcategory"));
    strcpy(subject, getparam("psubject"));
    }

  /* remove author for replies */
  if (path && !bedit)
    author[0] = 0;

  /* header */
  rsprintf("HTTP/1.1 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG</title></head>\n");
  rsprintf("<body><form method=\"POST\" action=\"%s%s\" enctype=\"multipart/form-data\">\n", 
            elogd_url, logbook_enc);

  /*---- title row ----*/

  show_standard_title(logbook, "", 0);

  if (bedit && !allow_edit)
    {
    rsprintf("<tr><td bgcolor=#FF8080 align=center><br><h1>Message editing disabled</h1></td></tr>\n");
    rsprintf("</table>\n");
    rsprintf("</body></html>\r\n");
    return;
    }
  
  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  rsprintf("<input type=submit name=cmd value=Submit>\n");
  rsprintf("<input type=submit name=cmd value=Back>\n");
  rsprintf("</td></tr></table></td></tr>\n\n");

  /*---- entry form ----*/

  /* overall table for message giving blue frame */
  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
           gt("Categories border"), gt("Categories cellpadding"), gt("Frame color"));
  
  time(&now);
  if (bedit)
    {
    rsprintf("<tr><td nowrap bgcolor=%s width=1%%><b>Entry date:</b></td><td bgcolor=%s>%s</td></tr>\n\n", 
             gt("Categories bgcolor1"), gt("Categories bgcolor2"), date);
    rsprintf("<tr><td nowrap bgcolor=%s width=1%%><b>Revision date:</b></td><td bgcolor=%s>%s</td></tr>\n\n", 
             gt("Categories bgcolor1"), gt("Categories bgcolor2"), ctime(&now));
    }
  else
    {
    rsprintf("<tr><td nowrap bgcolor=%s width=1%%><b>Entry date:</b></td><td bgcolor=%s>%s</td></tr>\n\n", 
             gt("Categories bgcolor1"), gt("Categories bgcolor2"), ctime(&now));
    }

  /* get optional author list from configuration file */
  if (getcfg(logbook, "Authors", list))
    {
    memset(author_list, 0, sizeof(author_list));
    p = strtok(list, ",");
    for (i=0 ; p && i<MAX_N_LIST ; i++)
      {
      strcpy(author_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  /* get type list from configuration file */
  if (getcfg(logbook, "Types", list))
    {
    memset(type_list, 0, sizeof(type_list));
    p = strtok(list, ",");
    for (i=0 ; p && i<MAX_N_LIST ; i++)
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
    memset(category_list, 0, sizeof(category_list));
    p = strtok(list, ",");
    for (i=0 ; p && i<MAX_N_LIST ; i++)
      {
      strcpy(category_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  if (bedit)
    {
    strcpy(str, author);
    if (strchr(str, '@'))
      *strchr(str, '@') = 0;

    rsprintf("<tr><td bgcolor=%s><b>Author:</b></td>", gt("Categories bgcolor1"));
    rsprintf("<td bgcolor=%s><input type=\"text\" size=\"30\" maxlength=\"80\" name=\"Author\" value=\"%s\"></td></tr>\n", 
              gt("Categories bgcolor2"), str);
    }
  else
    {
    strcpy(str, author);
    if (author_list[0][0] == 0)
      {
      rsprintf("<tr><td bgcolor=%s><b>Author:</b></td>", gt("Categories bgcolor1"));
      rsprintf("<td bgcolor=%s><input type=\"text\" size=\"30\" maxlength=\"80\" name=\"Author\" value=\"%s\"></td></tr>\n", 
                gt("Categories bgcolor2"), str);
      }
    else
      {
      rsprintf("<tr><td bgcolor=%s><b>Author:</b></td><td bgcolor=%s><select name=\"author\">\n", 
               gt("Categories bgcolor1"), gt("Categories bgcolor2"));
      for (i=0 ; i<MAX_N_LIST && author_list[i][0] ; i++)
        if (equal_ustring(author_list[i], author))
          rsprintf("<option selected value=\"%s\">%s\n", author_list[i], author_list[i]);
        else
          rsprintf("<option value=\"%s\">%s\n", author_list[i], author_list[i]);
      rsprintf("</select></td></tr>\n");
      }
    }

  rsprintf("<tr><td bgcolor=%s><b>Type:</b></td><td bgcolor=%s><select name=\"type\">\n",
           gt("Categories bgcolor1"), gt("Categories bgcolor2"));
  for (i=0 ; i<MAX_N_LIST && type_list[i][0] ; i++)
    if ((path && !bedit && equal_ustring(type_list[i], "reply")) ||
        (equal_ustring(type_list[i], type)))
      rsprintf("<option selected value=\"%s\">%s\n", type_list[i], type_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
  rsprintf("</select></td></tr>\n");

  rsprintf("<tr><td bgcolor=%s><b>Category:</b></td><td bgcolor=%s><select name=\"category\">\n",
           gt("Categories bgcolor1"), gt("Categories bgcolor2"));
  for (i=0 ; i<MAX_N_LIST && category_list[i][0] ; i++)
    if (equal_ustring(category_list[i], category))
      rsprintf("<option selected value=\"%s\">%s\n", category_list[i], category_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", category_list[i], category_list[i]);
  rsprintf("</select></td></tr>\n");

  str[0] = 0;
  if (path && !bedit)
    sprintf(str, "Re: %s", subject);
  else
    sprintf(str, "%s", subject);
  rsprintf("<tr><td bgcolor=%s><b>Subject:</b></td><td bgcolor=%s><input type=text size=80 maxlength=\"80\" name=Subject value=\"%s\">\n", 
            gt("Categories bgcolor1"), gt("Categories bgcolor2"), str);

  if (path)
    {
    /* hidden text for original message */
    rsprintf("<input type=hidden name=orig value=\"%s\">\n", path);

    if (bedit)
      rsprintf("<input type=hidden name=edit value=1>\n");
    }
  
  rsprintf("</td></tr>\n");

  /* increased wrapping for replys (leave space for '> ' */
  wrap = (path && !bedit) ? 78 : 76;
  
  rsprintf("<tr><td colspan=2 bgcolor=#FFFFFF>\n");
  rsprintf("<textarea rows=20 cols=%d wrap=hard name=Text>", wrap);

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
    rsprintf("<input type=checkbox name=html value=1>Submit as HTML text\n");

  rsprintf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");

  if (getcfg(logbook, "Suppress default", str) && atoi(str) == 1)
    rsprintf("<input type=checkbox checked name=suppress value=1>Suppress Email notification</tr>\n");
  else
    rsprintf("<input type=checkbox name=suppress value=1>Suppress Email notification</tr>\n");

  if (bedit)
    {
    rsprintf("<tr><td colspan=2 align=center bgcolor=%s>If no attachments are resubmitted, the original ones are kept.<br>\n",
              gt("Categories bgcolor1"));
    rsprintf("To delete an old attachment, enter <code>&lt;delete&gt;</code> in the new attachment field.</td></tr>\n");

    for (i=0 ; i<MAX_ATTACHMENTS ; i++)
      {
      if (att[i][0])
        {
        rsprintf("<tr><td align=right nowrap bgcolor=%s>Original attachment %d:<br>New attachment %d:</td>", 
                  gt("Categories bgcolor1"), i+1, i+1);

        rsprintf("<td bgcolor=%s>%s<br>", gt("Categories bgcolor2"), att[i]+14);
        rsprintf("<input type=\"file\" size=\"60\" maxlength=\"200\" name=\"attfile%d\" accept=\"filetype/*\"></td></tr>\n", 
                  i+1);
        }
      else
        rsprintf("<tr><td bgcolor=%s>Attachment %d:</td><td bgcolor=%s><input type=\"file\" size=\"60\" maxlength=\"200\" name=\"attfile%d\" accept=\"filetype/*\"></td></tr>\n", 
                  gt("Categories bgcolor1"), i+1, gt("Categories bgcolor2"), i+1);
      }

    }
  else
    {
    /* attachment */
    for (i=0 ; i<MAX_ATTACHMENTS ; i++)
      rsprintf("<tr><td nowrap bgcolor=%s><b>Attachment %d:</b></td><td bgcolor=%s><input type=\"file\" size=\"60\" maxlength=\"200\" name=\"attfile%d\" accept=\"filetype/*\"></td></tr>\n", 
               gt("Categories bgcolor1"), i+1, gt("Categories bgcolor2"), i+1);
    }

  rsprintf("</td></tr></table></td></tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_find()
{
int    i;
char   *p, list[10000], str[256];

  /*---- header ----*/

  show_standard_header("ELOG find", NULL);

  /*---- title ----*/
  
  show_standard_title(logbook, "", 0);

  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  rsprintf("<input type=submit name=cmd value=Search>\n");
  rsprintf("<input type=reset value=\"Reset Form\">\n");
  rsprintf("<input type=submit name=cmd value=Back>\n");
  rsprintf("</td></tr></table></td></tr>\n\n");

  /*---- entry form ----*/

  /* overall table for message giving blue frame */
  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
           gt("Categories border"), gt("Categories cellpadding"), gt("Frame color"));

  rsprintf("<tr><td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));

  if (getcfg(logbook, "Summary on default", str) && atoi(str) == 1)
    rsprintf("<input type=checkbox checked name=mode value=\"summary\">Summary only\n");
  else
    rsprintf("<input type=checkbox name=mode value=\"summary\">Summary only\n");
  rsprintf("</td></tr><td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));
  rsprintf("<input type=checkbox name=attach value=1>Show attachments</td></tr>\n");

  rsprintf("<td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));
  rsprintf("<input type=checkbox name=printable value=1>Printable output</td></tr>\n");

  /* count logbooks */
  for (i=0 ;  ; i++)
    {
    if (!enumgrp(i, str))
      break;

    if (equal_ustring(str, "global"))
      continue;
    }

  if (i > 2)
    {
    rsprintf("<td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));
    rsprintf("<input type=checkbox name=all value=1>Search all logbooks</td></tr>\n");
    }

  rsprintf("<tr><td nowrap width=1%% bgcolor=%s>Start date:</td>", gt("Categories bgcolor1"));
  rsprintf("<td bgcolor=%s><select name=\"m1\">\n", gt("Categories bgcolor2"));

  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<12 ; i++)
    rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
  rsprintf("</select>\n");

  rsprintf("<select name=\"d1\">");
  rsprintf("<option selected value=\"\">\n");
  for (i=0 ; i<31 ; i++)
    rsprintf("<option value=%d>%d\n", i+1, i+1);
  rsprintf("</select>\n");

  rsprintf("&nbsp;Year: <input type=\"text\" size=5 maxlength=5 name=\"y1\">");
  rsprintf("</td></tr>\n");

  rsprintf("<tr><td bgcolor=%s>End date:</td>", gt("Categories bgcolor1"));
  rsprintf("<td bgcolor=%s><select name=\"m2\">\n", gt("Categories bgcolor2"));

  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<12 ; i++)
    rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
  rsprintf("</select>\n");

  rsprintf("<select name=\"d2\">");
  rsprintf("<option selected value=\"\">\n");
  for (i=0 ; i<31 ; i++)
    rsprintf("<option value=%d>%d\n", i+1, i+1);
  rsprintf("</select>\n");

  rsprintf("&nbsp;Year: <input type=\"text\" size=5 maxlength=5 name=\"y2\">");
  rsprintf("</td></tr>\n");

  /* get optional author list from configuration file */
  if (getcfg(logbook, "Authors", list))
    {
    memset(author_list, 0, sizeof(author_list));
    p = strtok(list, ",");
    for (i=0 ; p && i<MAX_N_LIST ; i++)
      {
      strcpy(author_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  /* get type list from configuration file */
  if (getcfg(logbook, "Types", list))
    {
    memset(type_list, 0, sizeof(type_list));
    p = strtok(list, ",");
    for (i=0 ; p && i<MAX_N_LIST ; i++)
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
    memset(category_list, 0, sizeof(category_list));
    p = strtok(list, ",");
    for (i=0 ; p && i<MAX_N_LIST ; i++)
      {
      strcpy(category_list[i], p);
      p = strtok(NULL, ",");
      if (!p)
        break;
      while (*p == ' ')
        p++;
      }
    }

  rsprintf("</td></tr>\n");

  rsprintf("<tr><td bgcolor=%s>Author:</td>", gt("Categories bgcolor1"));
  rsprintf("<td bgcolor=%s>", gt("Categories bgcolor2"));
  if (author_list[0][0] == 0)
    rsprintf("<input type=\"text\" size=\"30\" maxlength=\"80\" name=\"author\">\n");
  else
    {
    rsprintf("<select name=\"author\">\n");
    rsprintf("<option value=\"\">\n");
    for (i=0 ; i<MAX_N_LIST && author_list[i][0] ; i++)
      rsprintf("<option value=\"%s\">%s\n", author_list[i], author_list[i]);
    rsprintf("</select>\n");
    }
  rsprintf("</td></tr>\n");

  rsprintf("<td bgcolor=%s>Type:</td>", gt("Categories bgcolor1"));
  rsprintf("<td bgcolor=%s><select name=\"type\">\n", gt("Categories bgcolor2"));
  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<MAX_N_LIST && type_list[i][0] ; i++)
    rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
  
  rsprintf("</select></td></tr>\n");

  rsprintf("<tr><td bgcolor=%s>Category:</td>",  gt("Categories bgcolor1"));
  rsprintf("<td bgcolor=%s><select name=\"category\">\n",  gt("Categories bgcolor2"));
  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<MAX_N_LIST && category_list[i][0] ; i++)
    rsprintf("<option value=\"%s\">%s\n", category_list[i], category_list[i]);
  rsprintf("</select></td></tr>\n");

  rsprintf("<tr><td bgcolor=%s>Subject:</td>", gt("Categories bgcolor1"));
  rsprintf("<td bgcolor=%s><input type=\"text\" size=\"30\" maxlength=\"80\" name=\"subject\">\n",
              gt("Categories bgcolor2"));
  rsprintf("<i>(case insensitive substring)</i></td></tr>\n");

  rsprintf("<tr><td bgcolor=%s>Text:</td>",  gt("Categories bgcolor1"));
  rsprintf("<td bgcolor=%s><input type=\"text\" size=\"30\" maxlength=\"80\" name=\"subtext\">\n",
             gt("Categories bgcolor2"));
  rsprintf("<i>(case insensitive substring)</i></td></tr>\n");

  rsprintf("</table></td></tr></table>\n");
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
  show_standard_header("Delete ELog entry", str);

  rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>", 
            gt("Border width"), gt("Frame color"));
  rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));


  if (!allow_delete)
    {
    rsprintf("<tr><td colspan=2 bgcolor=#FF8080 align=center><h1>Message deletion disabled</h1></td></tr>\n");
    }
  else
    {
    if (getparam("confirm") && *getparam("confirm"))
      {
      if (strcmp(getparam("confirm"), "Yes") == 0)
        {
        /* delete message */
        status = el_delete_message(path);
        rsprintf("<tr><td bgcolor=#80FF80 align=center>");
        if (status == EL_SUCCESS)
          rsprintf("<b>Message successfully deleted</b></tr>\n");
        else
          rsprintf("<b>Error deleting message: status = %d</b></tr>\n", status);

        rsprintf("<input type=hidden name=cmd value=last>\n");
        rsprintf("<tr><td bgcolor=%s align=center><input type=submit value=\"Goto last message\"></td></tr>\n", 
                  gt("Cell BGColor"));
        }
      }
    else
      {
      /* define hidden field for command */
      rsprintf("<input type=hidden name=cmd value=delete>\n");

      rsprintf("<tr><td bgcolor=%s align=center>", gt("Title bgcolor"));
      rsprintf("<font color=%s><b>Are you sure to delete this message?</b></font></td></tr>\n", gt("Title fontcolor"));

      rsprintf("<tr><td bgcolor=%s align=center>%s</td></tr>\n", gt("Cell BGColor"), path);

      rsprintf("<tr><td bgcolor=%s align=center><input type=submit name=confirm value=Yes>\n", gt("Cell BGColor"));
      rsprintf("<input type=submit name=confirm value=No>\n", gt("Cell BGColor"));
      rsprintf("</td></tr>\n\n");
      }
    }

  rsprintf("</table></td></tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_submit_find(INT past_n, INT last_n)
{
int    i, j, size, run, status, d1, m1, y1, d2, m2, y2, index, colspan, i_line, n_line, i_col;
int    current_year, current_month, current_day, printable, n_logbook, lindex;
char   date[80], author[80], type[80], category[80], subject[256], text[TEXT_SIZE], 
       orig_tag[80], reply_tag[80], attachment[MAX_ATTACHMENTS][256], encoding[80];
char   str[256], str2[10000], tag[256], ref[256], file_name[256], col[80], old_data_dir[256];
char   logbook_list[100][32], lb_enc[32], *nowrap;
BOOL   full, show_attachments;
DWORD  ltime, ltime_start, ltime_end, ltime_current, now;
struct tm tms, *ptms;
FILE   *f;

  printable = atoi(getparam("Printable"));

  /*---- header ----*/

  show_standard_header("ELOG search result", NULL);

  /*---- title ----*/
  
  str[0] = 0;
  if (past_n == 1)
    strcpy(str, ", Last day");
  else if (past_n > 1)
    sprintf(str, ", Last %d days", past_n);
  else if (last_n)
    sprintf(str, ", Last %d messages", last_n);

  if (printable)
    show_standard_title(logbook, str, 1);
  else
    show_standard_title(logbook, str, 0);

  /* get mode */
  if (past_n || last_n)
    {
    if (getcfg(logbook, "Summary on default", str))
      full = !atoi(str);
    else
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
  current_year  = ptms->tm_year+1900;
  current_month = ptms->tm_mon+1;
  current_day   = ptms->tm_mday;
  
  /*---- menu buttons ----*/

  if (!printable)
    {
    rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
             gt("Menu1 cellpadding"), gt("Frame color"));

    rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

    if (past_n)
      rsprintf("<input type=submit name=past value=\"Last %d days\">\n", past_n+1);

    if (last_n)
      rsprintf("<input type=submit name=last value=\"Last %d entries\">\n", last_n+10);
    
    rsprintf("<input type=submit name=cmd value=Find>\n");
    rsprintf("<input type=submit name=cmd value=Back>\n");
    rsprintf("</td></tr></table></td></tr>\n\n");
    }

  /*---- convert end date to ltime ----*/

  ltime_end = ltime_start = 0;

  if (!past_n && !last_n)
    {
    if (*getparam("m1") || *getparam("y1") || *getparam("d1"))
      {
      /* if year not given, use current year */
      if (!*getparam("y1"))
        y1 = current_year;
      else
        y1 = atoi(getparam("y1"));
      if (y1 < 1990 || y1 > current_year)
        {
        rsprintf("</table>\r\n");
        rsprintf("<h1>Error: year %s out of range</h1>", getparam("y1"));
        rsprintf("</body></html>\r\n");
        return;
        }

      /* if month not given, use current month */
      if (*getparam("m1"))
        {
        strcpy(str, getparam("m1"));
        for (m1=0 ; m1<12 ; m1++)
          if (equal_ustring(str, mname[m1]))
            break;
        if (m1 == 12)
          m1 = 0;
        m1++;
        }
      else
        m1 = current_month;

      /* if day not given, use 1 */
      if (*getparam("d1"))
        d1 = atoi(getparam("d1"));
      else
        d1 = 1;

      memset(&tms, 0, sizeof(struct tm));
      tms.tm_year = y1 % 100;
      tms.tm_mon  = m1-1;
      tms.tm_mday = d1;
      tms.tm_hour = 12;

      if (tms.tm_year < 90)
        tms.tm_year += 100;
      ltime_start = mktime(&tms);
      }
    
    if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
      {
      /* if year not give, use current year */
      if (*getparam("y2"))
        y2 = atoi(getparam("y2"));
      else
        y2 = current_year;

      if (y2 < 1990 || y2 > current_year)
        {
        rsprintf("</table>\r\n");
        rsprintf("<h1>Error: year %d out of range</h1>", y2);
        rsprintf("</body></html>\r\n");
        return;
        }

      /* if month not given, use current month */
      if (*getparam("m2"))
        {
        strcpy(str, getparam("m2"));
        for (m2=0 ; m2<12 ; m2++)
          if (equal_ustring(str, mname[m2]))
            break;
        if (m2 == 12)
          m2 = 0;
        m2++;
        }
      else
        m2 = current_month;

      /* if day not given, use last day of month */
      if (*getparam("d2"))
        d2 = atoi(getparam("d2"));
      else
        {
        memset(&tms, 0, sizeof(struct tm));
        tms.tm_year = y2 % 100;
        tms.tm_mon  = m2-1+1;
        tms.tm_mday = 1;
        tms.tm_hour = 12;

        if (tms.tm_year < 90)
          tms.tm_year += 100;
        ltime = mktime(&tms);
        ltime -= 3600*24;
        memcpy(&tms, localtime(&ltime), sizeof(struct tm));
        d2 = tms.tm_mday;
        }

      memset(&tms, 0, sizeof(struct tm));
      tms.tm_year = y2 % 100;
      tms.tm_mon  = m2-1;
      tms.tm_mday = d2;
      tms.tm_hour = 12;

      if (tms.tm_year < 90)
        tms.tm_year += 100;
      ltime_end = mktime(&tms);
      }
    }


  if (ltime_start && ltime_end && ltime_start > ltime_end)
    {
    rsprintf("</table>\r\n");
    rsprintf("<h1>Error: start time after end time</h1>", y2);
    rsprintf("</body></html>\r\n");
    return;
    }

  /*---- display filters ----*/

  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1>\n",
            printable ? "1" : gt("Border width"), gt("Categories cellpadding"));

  if (*getparam("m1") || *getparam("y1") || *getparam("d1"))
    rsprintf("<tr><td nowrap width=1%% bgcolor=%s><b>Start date:</b></td><td bgcolor=%s>%s %d, %d</td></tr>", 
              gt("Categories bgcolor1"), gt("Categories bgcolor2"), mname[m1-1], d1, y1);

  if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
    rsprintf("<tr><td nowrap width=1%% bgcolor=%s><b>End date:</b></td><td bgcolor=%s>%s %d, %d</td></tr>", 
              gt("Categories bgcolor1"), gt("Categories bgcolor2"), mname[m2-1], d2, y2);

  if (*getparam("author"))
    rsprintf("<tr><td nowrap width=1%% bgcolor=%s><b>Author:</b></td><td bgcolor=%s>%s</td></tr>", 
              gt("Categories bgcolor1"), gt("Categories bgcolor2"), getparam("author"));

  if (*getparam("type"))
    rsprintf("<tr><td nowrap width=1%% bgcolor=%s><b>Type:</b></td><td bgcolor=%s>%s</td></tr>", 
              gt("Categories bgcolor1"), gt("Categories bgcolor2"), getparam("type"));

  if (*getparam("category"))
    rsprintf("<tr><td nowrap width=1%% bgcolor=%s><b>Category:</b></td><td bgcolor=%s>%s</td></tr>", 
              gt("Categories bgcolor1"), gt("Categories bgcolor2"), getparam("category"));

  if (*getparam("subject"))
    rsprintf("<tr><td nowrap width=1%% bgcolor=%s><b>Subject:</b></td><td bgcolor=%s>%s</td></tr>", 
              gt("Categories bgcolor1"), gt("Categories bgcolor2"), getparam("subject"));

  if (*getparam("subtext"))
    rsprintf("<tr><td nowrap width=1%% bgcolor=%s><b>Text:</b></td><td bgcolor=%s>%s</td></tr>", 
              gt("Categories bgcolor1"), gt("Categories bgcolor2"), getparam("subtext"));

  rsprintf("</td></tr></table></td></tr>\n\n");
  
  
  /* get number of summary lines */
  n_line = 3;
  if (getcfg(logbook, "Summary lines", str))
    n_line = atoi(str);

  /*---- table titles ----*/

  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           printable ? "1" : gt("Border width"), gt("Categories cellpadding"), gt("Frame color"));

  strcpy(col, gt("Categories bgcolor1"));
  rsprintf("<tr>\n");

  size = printable ? 2 : 3;

  if (atoi(getparam("all")) == 1)
    rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica><b>Logbook</b></td>", col, size);
  
  rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica><b>Date</b></td>", col, size);
  rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica><b>Author</b></td>", col, size);
  rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica><b>Type</b></td>", col, size);
  rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica><b>Category</b></td>", col, size);
  rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica><b>Subject</b></td>", col, size);
    
  if (!full && n_line > 0)
    rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica><b>Text</b></td>", col, size);

  rsprintf("</tr>\n\n");
  
  /*---- do find ----*/

  i_col = 0;

  old_data_dir[0] = 0;
  if (atoi(getparam("all")) == 1)
    {
    /* count logbooks */
    for (i=n_logbook=0 ;  ; i++)
      {
      if (!enumgrp(i, str))
        break;

      if (equal_ustring(str, "global"))
        continue;

      strcpy(logbook_list[n_logbook++], str);
      }

    strcpy(old_data_dir, data_dir);
    }
  else
    {
    strcpy(logbook_list[0], logbook);
    n_logbook = 1;
    }

  /* loop through all logbooks */
  for (lindex=0 ; lindex<n_logbook ; lindex++)
    {
    if (n_logbook > 1)
      {
      /* set data_dir from logbook */
      getcfg(logbook_list[lindex], "Data dir", data_dir);
      if (data_dir[strlen(data_dir)-1] != DIR_SEPARATOR)
        {
        data_dir[strlen(data_dir)+1] = 0;
        data_dir[strlen(data_dir)] = DIR_SEPARATOR;
        }
      }

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
      el_search_message(tag, NULL, TRUE, FALSE);
      for (i=1 ; i<last_n ; i++)   
        {
        strcat(tag, "-1");   
        el_search_message(tag, NULL, TRUE, FALSE);
        } 
      }
    else if (*getparam("r1"))
      {
      /* do run find */
      el_search_run(atoi(getparam("r1")), tag);
      }
    else
      {
      /* do date-date find */

      if (!*getparam("y1") && !*getparam("m1") && !*getparam("d1"))
        {
        /* search first entry */
        tag[0] = 0;
        el_search_message(tag, NULL, TRUE, TRUE);
        }
      else
        {
        /* if y, m or d given, assemble start date */
        sprintf(tag, "%02d%02d%02d.0", y1 % 100, m1, d1);
        }
      }

    do
      {
      size = sizeof(text);
      status = el_retrieve(tag, date, &run, author, type, category, subject, 
                           text, &size, orig_tag, reply_tag, 
                           attachment,
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

        strcpy(lb_enc, logbook_list[lindex]);
        url_encode(lb_enc);
        sprintf(ref, "%s%s/%s", elogd_url, lb_enc, str);

        if (full)
          {
          strcpy(col, gt("List bgcolor1"));
          rsprintf("<tr>");

          size = printable ? 2 : 3;
          nowrap = printable ? "" : "nowrap";

          if (atoi(getparam("all")) == 1)
            rsprintf("<td %s bgcolor=%s><font size=%d>%s</font></td>", nowrap, col, size, logbook_list[lindex]);

          rsprintf("<td %s bgcolor=%s><font size=%d><a href=\"%s\">%s</a></font></td>", nowrap, col, size, ref, date);
          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp</font></td>", col, size, author);
          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp</font></td>", col, size, type);
          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp</font></td>", col, size, category);
          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp;</font></td>", col, size, subject);

          if (atoi(getparam("all")) == 1)
            colspan = 6;
          else
            colspan = 5;

          rsprintf("</tr><tr><td bgcolor=#FFFFFF colspan=%d><font size=%d>", colspan, size);
        
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
            rsprintf("&nbsp;</font></td></tr>\n");

          for (index = 0 ; index < MAX_ATTACHMENTS ; index++)
            {
            if (attachment[index][0])
              {
              for (i=0 ; i<(int)strlen(attachment[index]) ; i++)
                str[i] = toupper(attachment[index][i]);
              str[i] = 0;
    
              sprintf(ref, "%s%s/%s", elogd_url, logbook_enc, attachment[index]);

              if (!show_attachments)
                {
                rsprintf("<a href=\"%s\"><b>%s</b></a>&nbsp;", 
                          ref, attachment[index]+14);
                }
              else
                {
                if (strstr(str, ".GIF") ||
                    strstr(str, ".JPG") ||
                    strstr(str, ".PNG"))
                  {
                  rsprintf("<tr><td colspan=%d bgcolor=%s><b>Attachment %d:</b> <a href=\"%s\">%s</a>\n", 
                            colspan, gt("List bgcolor2"), index+1, ref, attachment[index]+14);
                  if (show_attachments)
                    rsprintf("</td></tr><tr><td colspan=%d bgcolor=#FFFFFF><img src=\"%s\"></td></tr>", colspan, ref);
                  }
                else
                  {
                  rsprintf("<tr><td colspan=%d bgcolor=%s><b>Attachment %d:</b> <a href=\"%s\">%s</a>\n", 
                            colspan, gt("List bgcolor2"), index+1, ref, attachment[index]+14);

                  if ((strstr(str, ".TXT") ||
                       strstr(str, ".ASC") ||
                       strchr(str, '.') == NULL) && show_attachments)
                    {
                    /* display attachment */
                    rsprintf("<br><font size=%d><pre>", size);

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
                  rsprintf("</font></td></tr>\n");
                  }
                }
              }
            }
 
          if (!show_attachments && attachment[0][0])
            rsprintf("</font></td></tr>\n");

          }
        else
          {
          if (i_col % 2 == 0)
            strcpy(col, gt("List bgcolor1"));
          else
            strcpy(col, gt("List bgcolor2"));
          i_col++;

          rsprintf("<tr>");
          
          size = printable ? 2 : 3;
          nowrap = printable ? "" : "nowrap";

          if (atoi(getparam("all")) == 1)
            rsprintf("<td %s bgcolor=%s>%s</td>", nowrap, col, logbook_list[lindex]);

          rsprintf("<td %s bgcolor=%s><font size=%d><a href=\"%s\">%s</a></font></td>", nowrap, col, size, ref, date);
          
          /* add spaces for line wrap */
          if (printable)
            {
            for (i=j=0 ; i<(int)strlen(author) ; i++)
              {
              str[j++] = author[i];
              if (author[i] == '.' || author[i] == '@')
                str[j++] = ' ';
              }
            str[j] = 0;
            }
          else
            strcpy(str, author);

          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp</font></td>", col, size, str);
          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp</font></td>", col, size, type);
          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp</font></td>", col, size, category);
          rsprintf("<td bgcolor=%s><font size=%d>%s&nbsp;</font></td>", col, size, subject);

          if (n_line > 0)
            {
            rsprintf("<td bgcolor=%s><font size=%d>", col, size);
            for (i=i_line=0 ; i<sizeof(str)-1 ; i++)
              {
              str[i] = text[i];
              if (str[i] == '\n')
                i_line++;

              if (i_line == n_line)
                break;
              }
            str[i] = 0;

            if (equal_ustring(encoding, "HTML"))
              rsputs(str);
            else
              strencode(str);
            rsprintf("&nbsp;</font></td>\n");
            }

          rsprintf("</tr>\n");
          }
        }

      } while (status == EL_SUCCESS);
    } /* for () */

  rsprintf("</table></td></tr></table>\n");
  rsprintf("</body></html>\r\n");

  /* restor old data_dir */
  if (old_data_dir[0])
    strcpy(data_dir, old_data_dir);
}

/*------------------------------------------------------------------*/

void show_rawfile(char *path)
{
FILE   *f;
char   file_name[256], line[1000];

  /* header */
  show_standard_header(path, NULL);
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
char   str[256], str1[256], str2[256], author[256], mail_to[256], mail_from[256],
       mail_text[256], mail_list[256], smtp_host[256], tag[80], *p;
char   *buffer[MAX_ATTACHMENTS], mail_param[1000];
char   att_file[MAX_ATTACHMENTS][256];
int    i, index, n_mail, suppress, status;
struct hostent *phe;

  /* check for author */
  if (*getparam("author") == 0)
    {
    rsprintf("HTTP/1.1 200 Document follows\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n");
    if (use_keepalive)
      {
      rsprintf("Connection: Keep-Alive\r\n");
      rsprintf("Keep-Alive: timeout=60, max=10\r\n");
      }
    rsprintf("Content-Type: text/html\r\n\r\n");

    rsprintf("<html><head><title>ELog Error</title></head>\n");
    rsprintf("<i>Error: No author supplied.</i><p>\n");
    rsprintf("Please go back and enter your name in the <i>author</i> field.\n");
    rsprintf("<body></body></html>\n");
    return;
    }

  /* check for valid attachment files */
  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    {
    buffer[i] = NULL;
    sprintf(str, "attachment%d", i);
    strcpy(att_file[i], getparam(str));
    if (att_file[i][0] && _attachment_size[i] == 0 && !equal_ustring(att_file[i], "<delete>"))
      {
      rsprintf("HTTP/1.1 200 Document follows\r\n");
      rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
      if (use_keepalive)
        {
        rsprintf("Connection: Keep-Alive\r\n");
        rsprintf("Keep-Alive: timeout=60, max=10\r\n");
        }
      rsprintf("Content-Type: text/html\r\n\r\n");

      rsprintf("<html><head><title>ELog Error</title></head>\n");
      rsprintf("<i>Error: Attachment file <i>%s</i> not valid.</i><p>\n", getparam(str));
      rsprintf("Please go back and enter a proper filename (use the <b>Browse</b> button).\n");
      rsprintf("<body></body></html>\n");
      return;
      }
    }

  /* add remote host name to author */
  phe = gethostbyaddr((char *) &rem_addr, 4, PF_INET);
  if (phe == NULL)
    {
    /* use IP number instead */
    strcpy(str, (char *)inet_ntoa(rem_addr));
    }
  else
    strcpy(str, phe->h_name);
      
  strcpy(author, getparam("author"));
  strcat(author, "@");
  strcat(author, str);

  tag[0] = 0;
  if (*getparam("edit"))
    strcpy(tag, getparam("orig"));

  status = el_submit(atoi(getparam("run")), author, getparam("type"),
                     getparam("category"), getparam("subject"), getparam("text"), 
                     getparam("orig"), *getparam("html") ? "HTML" : "plain", 
                     att_file, 
                     _attachment_buffer, 
                     _attachment_size, 
                     tag, sizeof(tag));

  if (status != EL_SUCCESS)
    {
    sprintf(str, "New message cannot be written to directory \"%s\"<p>", data_dir);
    strcat(str, "Please check that it exists and elogd has write access.");
    show_error(str);
    return;
    }

  suppress = atoi(getparam("suppress"));

  /* check for mail submissions */
  mail_param[0] = 0;
  n_mail = 0;

  if (suppress)
    {
    strcpy(mail_param, "?suppress=1");
    }
  else
    {
    for (index=0 ; index < 3 ; index++)
      {
      if (index == 0)
        sprintf(str, "Email %s", getparam("type"));
      else if (index == 1)
        sprintf(str, "Email %s", getparam("category"));
      else
        sprintf(str, "Email ALL");

      if (getcfg(logbook, str, mail_list))
        {
        if (verbose)
          printf("\n%s to %s\n\n", str, mail_list);
    
        if (!getcfg(logbook, "SMTP host", smtp_host))
          if (!getcfg(logbook, "SMTP host", smtp_host))
            {
            show_error("No SMTP host defined in configuration file");
            return;
            }
    
        p = strtok(mail_list, ",");
        for (i=0 ; p ; i++)
          {
          strcpy(mail_to, p);
          sprintf(mail_from, "ELog@%s", host_name);

          strcpy(str1, author);
          if (strchr(str1, '@'))
            {
            strcpy(str2, strchr(str1, '@')+1);
            *strchr(str1, '@') = 0;
            }

          if (strchr(author, '@'))
            sprintf(mail_text, "A new entry has been submitted by %s from %s:\r\n\r\n", str1, str2);
          else
            sprintf(mail_text, "A new entry has been submitted by %s:\r\n\r\n", author);

          sprintf(mail_text+strlen(mail_text), "Logbook  : %s\r\n", logbook);
          sprintf(mail_text+strlen(mail_text), "Type     : %s\r\n", getparam("type"));
          sprintf(mail_text+strlen(mail_text), "Category : %s\r\n", getparam("category"));
          sprintf(mail_text+strlen(mail_text), "Subject  : %s\r\n", getparam("subject"));
          sprintf(mail_text+strlen(mail_text), "Link     : %s%s/%s\r\n", elogd_full_url, logbook_enc, tag);

          sendmail(smtp_host, mail_from, mail_to, 
            index == 0 ? getparam("type") : getparam("category"), mail_text);

          if (mail_param[0] == 0)
            strcpy(mail_param, "?");
          else
            strcat(mail_param, "&");
          sprintf(mail_param+strlen(mail_param), "mail%d=%s", n_mail++, mail_to);

          p = strtok(NULL, ",");
          if (!p)
            break;
          while (*p == ' ')
            p++;
          }
        }
      }
    }

  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    if (buffer[i])
      free(buffer[i]);

  rsprintf("HTTP/1.1 302 Found\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }

  rsprintf("Location: %s%s/%s%s\r\n\r\n<html>redir</html>\r\n", 
            elogd_url, logbook_enc, tag, mail_param);
}

/*------------------------------------------------------------------*/

void show_elog_page(char *logbook, char *path)
{
int    size, i, run, msg_status, status, fh, length, first_message, last_message, index;
char   str[256], orig_path[256], command[80], ref[256], file_name[256];
char   date[80], author[80], type[80], category[80], subject[256], text[TEXT_SIZE],
       orig_tag[80], reply_tag[80], attachment[MAX_ATTACHMENTS][256], encoding[80], att[256];
FILE   *f;
BOOL   allow_delete, allow_edit;
time_t now;
struct tm *gmt;

  allow_delete = FALSE;
  allow_edit = TRUE;

  /* get flags from configuration file */
  if (getcfg(logbook, "Allow delete", str))
    allow_delete = atoi(str);
  if (getcfg(logbook, "Allow edit", str))
    allow_edit = atoi(str);
  
  /*---- interprete commands ---------------------------------------*/

  strcpy(command, getparam("cmd"));

  /* correct for image buttons */
  if (*getparam("cmd_first.x"))
    strcpy(command, "first");
  if (*getparam("cmd_previous.x"))
    strcpy(command, "previous");
  if (*getparam("cmd_next.x"))
    strcpy(command, "next");
  if (*getparam("cmd_last.x"))
    strcpy(command, "last");

  if (equal_ustring(command, "help"))
    {
    show_help_page();
    return;
    }

  if (equal_ustring(command, "new"))
    {
    show_elog_new(NULL, FALSE);
    return;
    }

  if (equal_ustring(command, "edit"))
    {
    show_elog_new(path, TRUE);
    return;
    }

  if (equal_ustring(command, "reply"))
    {
    show_elog_new(path, FALSE);
    return;
    }

  if (equal_ustring(command, "submit"))
    {
    submit_elog();
    return;
    }

  if (equal_ustring(command, "find"))
    {
    show_elog_find();
    return;
    }

  if (equal_ustring(command, "search"))
    {
    show_elog_submit_find(0, 0);
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

  strcpy(str, getparam("last"));
  if (strchr(str, ' '))
    {
    i = atoi(strchr(str, ' '));
    sprintf(str, "last%d", i);
    redirect(str);
    return;
    }

  strcpy(str, getparam("past"));
  if (strchr(str, ' '))
    {
    i = atoi(strchr(str, ' '));
    sprintf(str, "past%d", i);
    redirect(str);
    return;
    }

  if (equal_ustring(command, "delete"))
    {
    show_elog_delete(path);
    return;
    }

  if (strncmp(path, "past", 4) == 0)
    {
    show_elog_submit_find(atoi(path+4), 0);
    return;
    }

  if (strncmp(path, "last", 4) == 0 && strstr(path, ".gif") == NULL)
    {
    show_elog_submit_find(0, atoi(path+4));
    return;
    }

  /*---- check if file requested -----------------------------------*/

  if ((strlen(path) > 13 && path[6] == '_' && path[13] == '_') ||
      strstr(path, ".gif") || strstr(path, ".jpg") || strstr(path, ".png"))
    {
    if (strlen(path) > 13 && path[6] == '_' && path[13] == '_')
      {
      /* file from data directory requested */
      strcpy(file_name, data_dir);
      strcat(file_name, path);
      }
    else
      {
      /* file from theme directory requested */
      strcpy(file_name, cfg_dir);
      strcat(file_name, theme_name);
      strcat(file_name, DIR_SEPARATOR_STR);
      strcat(file_name, path);
      }

    fh = open(file_name, O_RDONLY | O_BINARY);
    if (fh > 0)
      {
      lseek(fh, 0, SEEK_END);
      length = TELL(fh);
      lseek(fh, 0, SEEK_SET);

      rsprintf("HTTP/1.1 200 Document follows\r\n");
      rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
      rsprintf("Accept-Ranges: bytes\r\n");

      time(&now);
      now += (int) (3600*3);
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);
      rsprintf("Expires: %s\r\n", str);

      if (use_keepalive)
        {
        rsprintf("Connection: Keep-Alive\r\n");
        rsprintf("Keep-Alive: timeout=60, max=10\r\n");
        }

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
    else
      {
      /* return empty buffer */
      rsprintf("HTTP/1.1 404 Not Found\r\n");
      rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
      rsprintf("Connection: close\r\n");
      rsprintf("Content-Type: text/html\r\n\r\n");
      rsprintf("<html><head><title>404 Not Found</title></head>\r\n");
      rsprintf("<body><h1>Not Found</h1>\r\n");
      rsprintf("The requested file <b>%s</b> was not found on this server<p>\r\n", file_name);
      rsprintf("<hr><address>ELOG version %s</address></body></html>\r\n\r\n", VERSION);
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
      equal_ustring(command, "last") || equal_ustring(command, "first"))
    {
    strcpy(orig_path, path);

    if (equal_ustring(command, "last") || equal_ustring(command, "first"))
      path[0] = 0;

    do
      {
      if (equal_ustring(command, "next") || equal_ustring(command, "first"))
        strcat(path, "+1");
      if (equal_ustring(command, "previous") || equal_ustring(command, "last"))
        strcat(path, "-1");

      status = el_search_message(path, NULL, TRUE, equal_ustring(command, "first"));
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
                  attachment, encoding);
      
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
  msg_status = el_retrieve(path, date, &run, author, type, category, subject, 
                           text, &size, orig_tag, reply_tag, 
                           attachment, encoding);

  /*---- header ----*/

  /* header */
  if (msg_status == EL_SUCCESS)
    show_standard_header(subject, path);
  else
    show_standard_header("", "");

  /*---- title ----*/
  
  show_standard_title(logbook, "", 0);

  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));
  
  if (atoi(gt("Use buttons")) == 1)
    {
    rsprintf("<input type=submit name=cmd value=New>\n");
    if (allow_edit)
      rsprintf("<input type=submit name=cmd value=Edit>\n");
    if (allow_delete)
      rsprintf("<input type=submit name=cmd value=Delete>\n");
    rsprintf("<input type=submit name=cmd value=Reply>\n");
    rsprintf("<input type=submit name=cmd value=Find>\n");
    rsprintf("<input type=submit name=cmd value=\"Last day\">\n");
    rsprintf("<input type=submit name=cmd value=\"Last 10\">\n");
    rsprintf("<input type=submit name=cmd value=\"Help\">\n");
    }
  else
    {
    rsprintf("<small>\n");
    rsprintf("&nbsp;<a href=\"/%s?cmd=New\">New</a>&nbsp;|\n", logbook_enc);
    if (allow_edit)
      rsprintf("&nbsp;<a href=\"/%s/%s?cmd=Edit\">Edit</a>&nbsp|\n", logbook_enc, path);
    if (allow_delete)
      rsprintf("&nbsp;<a href=\"/%s/%s?cmd=Delete\">Delete</a>&nbsp|\n", logbook_enc, path);
    rsprintf("&nbsp;<a href=\"/%s/%s?cmd=Reply\">Reply</a>&nbsp|\n", logbook_enc, path);
    rsprintf("&nbsp;<a href=\"/%s?cmd=Find\">Find</a>&nbsp|\n", logbook_enc);
    rsprintf("&nbsp;<a href=\"/%s?cmd=Last+day\">Last day</a>&nbsp|\n", logbook_enc);
    rsprintf("&nbsp;<a href=\"/%s?cmd=Last+10\">Last 10</a>&nbsp|\n", logbook_enc);
    rsprintf("&nbsp;<a href=\"/%s?cmd=Help\">Help</a>&nbsp\n", logbook_enc);

    rsprintf("</small>\n");
    }

  rsprintf("</td>\n");

  if (atoi(gt("Merge menus")) != 1)
    {
    rsprintf("</tr>\n");
    rsprintf("</table></td></tr>\n\n");
    }

  /*---- next/previous buttons ----*/

  if (atoi(gt("Merge menus")) != 1)
    {
    rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
             gt("Menu2 cellpadding"), gt("Frame color"));
    rsprintf("<tr><td valign=bottom align=%s bgcolor=%s>\n", gt("Menu2 Align"), gt("Menu2 BGColor"));
    }
  else
    rsprintf("<td width=10%% nowrap align=%s bgcolor=%s>\n", gt("Menu2 Align"), gt("Menu2 BGColor"));
  
  if (atoi(gt("Menu2 use images")) == 1)
    {
    rsprintf("<input type=image name=cmd_first border=0 alt=\"First entry\" src=\"%s%s/first.gif\">\n", 
             elogd_url, logbook_enc);
    rsprintf("<input type=image name=cmd_previous border=0 alt=\"Previous entry\" src=\"%s%s/previous.gif\">\n", 
             elogd_url, logbook_enc);
    rsprintf("<input type=image name=cmd_next border=0 alt=\"Next entry\" src=\"%s%s/next.gif\">\n", 
             elogd_url, logbook_enc);
    rsprintf("<input type=image name=cmd_last border=0 alt=\"Last entry\" src=\"%s%s/last.gif\">\n", 
             elogd_url, logbook_enc);
    }
  else
    {
    rsprintf("<input type=submit name=cmd value=First>\n");
    rsprintf("<input type=submit name=cmd value=Previous>\n");
    rsprintf("<input type=submit name=cmd value=Next>\n");
    rsprintf("<input type=submit name=cmd value=Last>\n");
    }

  rsprintf("</td></tr>\n");
  rsprintf("</table></td></tr>\n\n");

  /*---- message ----*/

  /* overall table for message giving blue frame */
  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n", 
           gt("Categories border"), gt("Categories cellpadding"), gt("Frame color"));

  if (msg_status == EL_FILE_ERROR)
    rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><h1>No message available</h1></tr>\n");
  else
    {
    if (last_message)
      rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>This is the last message in the ELog</b></tr>\n");

    if (first_message)
      rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>This is the first message in the ELog</b></tr>\n");

    /* check for mail submissions */
    if (*getparam("suppress"))
      {
      if (getcfg(logbook, "Suppress default", str) && atoi(str) == 1)
        rsprintf("<tr><td colspan=2 bgcolor=#FFFFFF>Email notification suppressed</tr>\n");
      else
        rsprintf("<tr><td colspan=2 bgcolor=#FF0000><b>Email notification suppressed</b></tr>\n");
      i = 1;
      }
    else
      {
      for (i=0 ; ; i++)
        {
        sprintf(str, "mail%d", i);
        if (*getparam(str))
          {
          if (i==0)
            rsprintf("<tr><td colspan=2 bgcolor=#FFC020>");
          rsprintf("Mail sent to <b>%s</b><br>\n", getparam(str));
          }
        else
          break;
        }
      }

    if (i>0)
      rsprintf("</tr>\n");

    /*---- display date ----*/

    rsprintf("<tr><td nowrap bgcolor=%s width=10%%><b>Entry date:</b></td><td bgcolor=%s>%s\n\n", 
             gt("Categories bgcolor1"), gt("Categories bgcolor2"), date);

    /* define hidded fields */
    strcpy(str, author);
    if (strchr(str, '@'))
      *strchr(str, '@') = 0;
    rsprintf("<input type=hidden name=author value=\"%s\">\n", str); 
    rsprintf("<input type=hidden name=type value=\"%s\">\n", type); 
    rsprintf("<input type=hidden name=category value=\"%s\">\n", category); 
    rsprintf("<input type=hidden name=subject value=\"%s\">\n\n", subject); 

    rsprintf("</td></tr>\n\n");

    /*---- link to original message or reply ----*/

    if (msg_status != EL_FILE_ERROR && (reply_tag[0] || orig_tag[0]))
      {
      rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));

      if (orig_tag[0])
        {
        sprintf(ref, "%s%s/%s", elogd_url, logbook_enc, orig_tag);
        rsprintf("<b>In reply to:</b></td><td bgcolor=%s>", gt("Menu2 bgcolor"));
        rsprintf("<a href=\"%s\">%s</a></td>", ref, orig_tag);
        }
      if (reply_tag[0])
        {
        sprintf(ref, "%s%s/%s", elogd_url, logbook_enc, reply_tag);
        rsprintf("<b>Reply:</b></td><td bgcolor=%s>", gt("Menu2 bgcolor"));
        rsprintf("<a href=\"%s\">%s</a></td>", ref, reply_tag);
        }
      rsprintf("</tr>\n");
      }

    /*---- display author, categories, subject ----*/

    rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));
    if (*getparam("lauthor") == '1')
      rsprintf("<input type=\"checkbox\" checked name=\"lauthor\" value=\"1\">");
    else
      rsprintf("<input alt=\"text\" type=\"checkbox\" name=\"lauthor\" value=\"1\">");
    rsprintf("&nbsp;<b>Author:</b></td><td bgcolor=%s>%s&nbsp</td></tr>\n", gt("Categories bgcolor2"), author);

    rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));
    if (*getparam("ltype") == '1')
      rsprintf("<input type=\"checkbox\" checked name=\"ltype\" value=\"1\">");
    else
      rsprintf("<input type=\"checkbox\" name=\"ltype\" value=\"1\">");
    rsprintf("&nbsp;<b>Type:</b></td><td bgcolor=%s>%s&nbsp</td></tr>\n", gt("Categories bgcolor2"), type);
    
    rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));
    if (*getparam("lcategory") == '1')
      rsprintf("<input type=\"checkbox\" checked name=\"lcategory\" value=\"1\">");
    else
      rsprintf("<input type=\"checkbox\" name=\"lcategory\" value=\"1\">");
    rsprintf("&nbsp;<b>Category:</b></td><td bgcolor=%s>%s&nbsp</td></tr>\n", gt("Categories bgcolor2"), category);

    rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));
    if (*getparam("lsubject") == '1')
      rsprintf("<input type=\"checkbox\" checked name=\"lsubject\" value=\"1\">");
    else
      rsprintf("<input type=\"checkbox\" name=\"lsubject\" value=\"1\">");
    rsprintf("&nbsp;<b>Subject:</b></td><td bgcolor=%s>%s&nbsp</td></tr>\n", gt("Categories bgcolor2"), subject);

    rsprintf("</td></tr>\n");
    rsputs("</table></td></tr>\n");
    
    /*---- message text ----*/

    rsprintf("<tr><td><table width=100%% border=0 cellpadding=1 cellspacing=1 bgcolor=%s>\n", gt("Frame color"));
    rsprintf("<tr><td bgcolor=%s><br>\n", gt("Text BGColor"));
    
    if (equal_ustring(encoding, "plain"))
      {
      rsputs("<pre>");
      rsputs2(text);
      rsputs("</pre>");
      }
    else
      rsputs(text);

    rsputs("</td></tr>\n");
    rsputs("</table></td></tr>\n");

    for (index = 0 ; index < MAX_ATTACHMENTS ; index++)
      {
      if (attachment[index][0])
        {
        for (i=0 ; i<(int)strlen(attachment[index]) ; i++)
          att[i] = toupper(attachment[index][i]);
        att[i] = 0;
      
        /* determine size of attachment */
        strcpy(file_name, data_dir);
        strcat(file_name, attachment[index]);

        length = 0;
        fh = open(file_name, O_RDONLY | O_BINARY);
        if (fh > 0)
          {
          lseek(fh, 0, SEEK_END);
          length = TELL(fh);
          close(fh);
          }

        sprintf(ref, "%s%s/%s", elogd_url, logbook_enc, attachment[index]);

        rsprintf("<tr><td><table width=100%% border=0 cellpadding=0 cellspacing=1 bgcolor=%s>\n", gt("Frame color"));

        rsprintf("<tr><td nowrap width=10%% bgcolor=%s><b>&nbsp;Attachment %d:</b></td>", 
                 gt("Categories bgcolor1"), index+1);
        rsprintf("<td bgcolor=%s>&nbsp;<a href=\"%s\">%s</a>\n", 
                 gt("Categories bgcolor2"), ref, attachment[index]+14);

        if (length < 1024)
          rsprintf("&nbsp;<small><b>%d Bytes</b></small>", length);
        else if (length < 1024*1024)
          rsprintf("&nbsp;<small><b>%d kB</b></small>", length/1024);
        else
          rsprintf("&nbsp;<small><b>%1.3lf MB</b></small>", length/1024.0/1024.0);

        rsprintf("</td></tr></table></td></tr>\n");

        if (strstr(att, ".GIF") ||
            strstr(att, ".JPG") ||
            strstr(att, ".PNG"))
          {
          rsprintf("<tr><td><table width=100%% border=0 cellpadding=0 cellspacing=1 bgcolor=%s>\n", gt("Frame color"));
          rsprintf("<tr><td bgcolor=%s>", gt("Text bgcolor"));
          rsprintf("<img src=\"%s\"></td></tr>", ref);
          rsprintf("</table></td></tr>\n\n");
          }
        else
          {
          if (strstr(att, ".TXT") ||
              strstr(att, ".ASC") ||
              strchr(att, '.') == NULL)
            {
            /* display attachment */
            rsprintf("<tr><td><table width=100%% border=0 cellpadding=0 cellspacing=1 bgcolor=%s>\n", gt("Frame color"));
            rsprintf("<tr><td bgcolor=%s>", gt("Text bgcolor"));
            if (!strstr(att, ".HTML"))
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
            rsprintf("</td></tr></table></td></tr>\n");
            }
          }
        }
      }
    }

  /* table for message body */
  rsprintf("</td></tr></table></td></tr>\n\n");

  /* overall table */
  rsprintf("</td></tr></table></td></tr>\n");

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

BOOL check_write_password(char *logbook, char *password, char *redir)
{
char  str[256];

  /* get write password from configuration file */
  if (getcfg(logbook, "Write password", str))
    {
    if (strcmp(password, str) == 0)
      return TRUE;

    /* show web password page */
    show_standard_header("ELOG password", NULL);

    /* define hidden fields for current destination */
    if (redir[0])
      rsprintf("<input type=hidden name=redir value=\"%s\">\n", redir);

    rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>", 
              gt("Border width"), gt("Frame color"));
    rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

    if (password[0])
      rsprintf("<tr><th bgcolor=#FF0000>Wrong password!</th></tr>\n");

    rsprintf("<tr><td align=center bgcolor=%s>\n", gt("Title bgcolor"));
    rsprintf("<font color=%s>Please enter password to obtain write access</font></td></tr>\n", 
             gt("Title fontcolor"));
    
    rsprintf("<tr><td align=center bgcolor=%s><input type=password name=wpwd></td></tr>\n", gt("Cell BGColor"));
    rsprintf("<tr><td align=center bgcolor=%s><input type=submit value=Submit></td></tr>", gt("Cell BGColor"));

    rsprintf("</table></td></tr></table>\n");

    rsprintf("</body></html>\r\n");

    return FALSE;
    }
  else
    return TRUE;
}

/*------------------------------------------------------------------*/

BOOL check_delete_password(char *logbook, char *password, char *redir)
{
char  str[256];

  /* get write password from configuration file */
  if (getcfg(logbook, "Delete password", str))
    {
    if (strcmp(password, str) == 0)
      return TRUE;

    /* show web password page */
    show_standard_header("ELOG password", NULL);

    /* define hidden fields for current destination */
    if (redir[0])
      rsprintf("<input type=hidden name=redir value=\"%s\">\n", redir);

    rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>", 
              gt("Border width"), gt("Frame color"));
    rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

    if (password[0])
      rsprintf("<tr><th bgcolor=#FF0000>Wrong password!</th></tr>\n");

    rsprintf("<tr><td align=center bgcolor=%s>\n", gt("Title bgcolor"));
    rsprintf("<font color=%s>Please enter password to obtain delete access</font></td></tr>\n", 
             gt("Title fontcolor"));
    
    rsprintf("<tr><td align=center bgcolor=%s><input type=password name=dpwd></td></tr>\n", gt("Cell BGColor"));
    rsprintf("<tr><td align=center bgcolor=%s><input type=submit value=Submit></td></tr>", gt("Cell BGColor"));

    rsprintf("</table></td></tr></table>\n");

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

  rsprintf("HTTP/1.1 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html>\n");
  rsprintf("<head>\n");
  rsprintf("<title>ELOG Logbook Selection</title>\n");
  rsprintf("</head>\n\n");

  rsprintf("<body>\n\n");

  rsprintf("<p><p><p><table border=0 width=50%% bgcolor=#486090 cellpadding=0 cellspacing=0 align=center>");
  rsprintf("<tr><td><table cellpadding=5 cellspacing=1 border=0 width=100%% bgcolor=#486090>\n");
  
  rsprintf("<tr><td align=center colspan=2 bgcolor=#486090><font size=5 color=#FFFFFF>\n");
  rsprintf("Several logbooks are defined on this host.<BR>\n");
  rsprintf("Please select the one to connect to:</font></td></tr>\n");

  for (i=0 ;  ; i++)
    {
    if (!enumgrp(i, logbook))
      break;

    if (equal_ustring(logbook, "global"))
      continue;

    strcpy(str, logbook);
    url_encode(str);
    rsprintf("<tr><td bgcolor=#CCCCFF><a href=\"%s%s\">%s</a></td>", elogd_url, str, logbook);

    str[0] = 0;
    getcfg(logbook, "Comment", str);
    rsprintf("<td bgcolor=#DDEEBB>%s&nbsp;</td></tr>\n", str);
    }

  rsprintf("</table></td></tr></table></body>\n");
  rsprintf("</html>\r\n\r\n");
  
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

void interprete(char *cookie_wpwd, char *cookie_dpwd, char *path)
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
int    i, n, index;
double exp;
char   str[256], enc_pwd[80];
char   enc_path[256], dec_path[256];
char   *experiment, *wpassword, *dpassword, *command, *value, *group;
time_t now;
struct tm *gmt;

  /* encode path for further usage */
  strcpy(dec_path, path);
  url_decode(dec_path);
  url_decode(dec_path); /* necessary for %2520 -> %20 -> ' ' */
  strcpy(enc_path, dec_path);
  url_encode(enc_path);

  experiment = getparam("exp");
  wpassword = getparam("wpwd");
  dpassword = getparam("dpwd");
  command = getparam("cmd");
  value = getparam("value");
  group = getparam("group");
  index = atoi(getparam("index"));

  /* if experiment given, use it as logbook (for elog!) */
  if (experiment && experiment[0])
    {
    strcpy(logbook_enc, experiment);
    strcpy(logbook, experiment);
    url_decode(logbook);
    }

  /* if no logbook given, display logbook selection page */
  if (!logbook[0])
    {
    for (i=n=0 ; ; i++)
      {
      if (!enumgrp(i, str))
        break;
      if (!equal_ustring(str, "global"))
        {
        strcpy(logbook, str);
        n++;
        }
      }

    if (n > 1)
      {
      show_selection_page();
      return;
      }

    strcpy(logbook_enc, logbook);
    url_encode(logbook_enc);
    }

  /* get theme for logbook */
  if (getcfg(logbook, "Theme", str))
    loadtheme(str);
  else
    loadtheme(NULL); /* get default values */

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

    if (!check_write_password(logbook, enc_pwd, getparam("redir")))
      return;
    
    rsprintf("HTTP/1.1 302 Found\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
    if (use_keepalive)
      {
      rsprintf("Connection: Keep-Alive\r\n");
      rsprintf("Keep-Alive: timeout=60, max=10\r\n");
      }

    /* get optional expriation from configuration file */
    exp = 24;
    if (getcfg(logbook, "Write password expiration", str))
      exp = atof(str);

    tzset();
    time(&now);
    now += (int) (3600*exp);
    gmt = gmtime(&now);
    strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

    rsprintf("Set-Cookie: elog_wpwd=%s; path=/%s; expires=%s\r\n", enc_pwd, logbook_enc, str);

    sprintf(str, "%s%s/%s", elogd_url, logbook_enc, getparam("redir"));
    rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
    return;
    }

  if (dpassword[0])
    {
    /* check if password correct */
    base64_encode(dpassword, enc_pwd);

    if (!check_delete_password(logbook, enc_pwd, getparam("redir")))
      return;
    
    rsprintf("HTTP/1.1 302 Found\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
    if (use_keepalive)
      {
      rsprintf("Connection: Keep-Alive\r\n");
      rsprintf("Keep-Alive: timeout=60, max=10\r\n");
      }

    /* get optional expriation from configuration file */
    exp = 24;
    if (getcfg(logbook, "Delete password expiration", str))
      exp = atof(str);

    time(&now);
    now += (int) (3600*exp);
    gmt = gmtime(&now);
    strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

    rsprintf("Set-Cookie: elog_dpwd=%s; path=/%s; expires=%s\r\n", enc_pwd, logbook_enc, str);

    sprintf(str, "%s%s/%s", elogd_url, logbook_enc, getparam("redir"));
    rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
    return;
    }

  /*---- show ELog page --------------------------------------------*/

  if (equal_ustring(command, "new") ||
      equal_ustring(command, "edit") ||
      equal_ustring(command, "reply") ||
      equal_ustring(command, "submit") ||
      equal_ustring(command, "delete"))
    {
    sprintf(str, "%s?cmd=%s", path, command);
    if (!check_write_password(logbook, cookie_wpwd, str))
      return;
    }

  if (equal_ustring(command, "delete"))
    {
    sprintf(str, "%s?cmd=%s", path, command);
    if (!check_delete_password(logbook, cookie_dpwd, str))
      return;
    }

  show_elog_page(logbook, dec_path);
  return;
}

/*------------------------------------------------------------------*/

void decode_get(char *string, char *cookie_wpwd, char *cookie_dpwd)
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
        url_decode(pitem);
        url_decode(p);

        setparam(pitem, p);

        p = strtok(NULL,"&");
        }
      }
    }
  
  interprete(cookie_wpwd, cookie_dpwd, path);
}

/*------------------------------------------------------------------*/

void decode_post(char *string, char *boundary, int length, char *cookie_wpwd, char *cookie_dpwd)
{
char *pinit, *p, *pitem, *ptmp, file_name[256], str[256];
int  i, n;

  initparam();
  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    _attachment_size[i]=  0;

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
          {
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

          if (verbose)
            printf("decode_post: Found attachment %s\n", file_name);

          /* find next boundary */
          ptmp = string;
          do
            {
            while (*ptmp != '-')
              ptmp++;

            if ((p = strstr(ptmp, boundary)) != NULL)
              {
              if (*(p-1) == '-')
                p--;
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
          string = strstr(string, boundary) + strlen(boundary);

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

  interprete(cookie_wpwd, cookie_dpwd, "");
}

/*------------------------------------------------------------------*/

BOOL _abort = FALSE;

void ctrlc_handler(int sig)
{
  _abort = TRUE;
}

/*------------------------------------------------------------------*/

char net_buffer[WEB_BUFFER_SIZE];

#define N_MAX_CONNECTION 10
#define KEEP_ALIVE_TIME  60

int ka_sock[N_MAX_CONNECTION];
int ka_time[N_MAX_CONNECTION];
struct in_addr remote_addr[N_MAX_CONNECTION];

void server_loop(int tcp_port, int daemon)
{
int                  status, i, n, n_error, authorized, min, i_min, i_conn, length;
struct sockaddr_in   serv_addr, acc_addr;
char                 pwd[256], str[256], cl_pwd[256], *p;
char                 cookie_wpwd[256], cookie_dpwd[256], boundary[256];
int                  lsock, len, flag, content_length, header_length;
struct hostent       *phe;
struct linger        ling;
fd_set               readfds;
struct timeval       timeout;
INT                  keep_alive;

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
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port        = htons((short) tcp_port);

  status = bind(lsock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (status < 0)
    {
    /* try reusing address */
    flag = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR,
               (char *) &flag, sizeof(INT));
    status = bind(lsock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (status < 0)
      {
      printf("Cannot bind to port %d.\nPlease try later or use the \"-p\" flag to specify a different port\n", tcp_port);
      return;
      }
    else
      printf("Warning: port %d already in use\n", tcp_port);
    }

  /* get host name for mail notification */
  gethostname(host_name, sizeof(host_name));

  phe = gethostbyname(host_name);
  if (phe != NULL)
    phe = gethostbyaddr(phe->h_addr, sizeof(int), AF_INET);

  /* if domain name is not in host name, hope to get it from phe */
  if (strchr(host_name, '.') == NULL && phe != NULL)
    strcpy(host_name, phe->h_name);

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

  printf("Server listening...\n");
  do
    {
    FD_ZERO(&readfds);
    FD_SET(lsock, &readfds);

    for (i=0 ; i<N_MAX_CONNECTION ; i++)
      if (ka_sock[i] > 0)
        FD_SET(ka_sock[i], &readfds);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 100000;

    status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

    if (FD_ISSET(lsock, &readfds))
      {
      len = sizeof(acc_addr);
      _sock = accept( lsock,(struct sockaddr *) &acc_addr, &len);

      /* turn on lingering (borrowed from NCSA httpd code) */
      ling.l_onoff = 1;
      ling.l_linger = 600;
      setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));

      /* find new entry in socket table */
      for (i=0 ; i<N_MAX_CONNECTION ; i++)
        if (ka_sock[i] == 0)
          break;

      /* recycle last connection */
      if (i == N_MAX_CONNECTION)
        {
        for (i=i_min=0,min=ka_time[0] ; i<N_MAX_CONNECTION ; i++)
          if (ka_time[i] < min)
            {
            min = ka_time[i];
            i_min = i;
            }

        closesocket(ka_sock[i_min]);
        ka_sock[i_min] = 0;
        i = i_min;

#ifdef DEBUG_CONN        
        printf("## close connection %d\n", i_min);
#endif
        }

      i_conn = i;
      ka_sock[i_conn] = _sock;
      ka_time[i_conn] = (int) time(NULL);

      /* save remote host address */
      memcpy(&remote_addr[i_conn], &(acc_addr.sin_addr), sizeof(remote_addr));
      memcpy(&rem_addr, &(acc_addr.sin_addr), sizeof(remote_addr));

#ifdef DEBUG_CONN        
      printf("## open new connection %d\n", i_conn);
#endif
      }

    else
      {
      /* check if open connection received data */
      for (i= 0 ; i<N_MAX_CONNECTION ; i++)
        if (ka_sock[i] > 0 && FD_ISSET(ka_sock[i], &readfds))
          break;

      if (i == N_MAX_CONNECTION)
        {
        _sock = 0;
        }
      else
        {
        i_conn = i;
        _sock = ka_sock[i_conn];
        ka_time[i_conn] = (int) time(NULL);
        memcpy(&rem_addr, &remote_addr[i_conn], sizeof(remote_addr));

#ifdef DEBUG_CONN        
        printf("## received request on connection %d\n", i_conn);
#endif
        }
      }

    /* turn off keep_alive by default */
    keep_alive = FALSE;
    
    if (_sock > 0)
      {
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
            strcpy(logbook_enc, str);
            url_decode(logbook);
            
            /* extract header and content length */
            if (strstr(net_buffer, "Content-Length:"))
              content_length = atoi(strstr(net_buffer, "Content-Length:") + 15);
            else if (strstr(net_buffer, "Content-length:"))
              content_length = atoi(strstr(net_buffer, "Content-length:") + 15);

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
      cookie_dpwd[0] = 0;
      if (strstr(net_buffer, "elog_dpwd=") != NULL)
        {
        strcpy(cookie_dpwd, strstr(net_buffer, "elog_dpwd=")+10);
        cookie_dpwd[strcspn(cookie_dpwd, " ;\r\n")] = 0;
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
        strcpy(logbook_enc, logbook);
        url_decode(logbook);

        /* check if logbook exists */
        for (i=0 ; ; i++)
          {
          if (!enumgrp(i, str))
            break;
          if (equal_ustring(logbook, str))
            break;
          }

        if (!equal_ustring(logbook, str) && logbook[0])
          {
          sprintf(str, "Error: logbook \"%s\" not defined in elogd.cfg", logbook);
          show_error(str);
          send(_sock, return_buffer, strlen(return_buffer), 0);
          goto error;
          }
        }

      /* if no logbook is given and only one logbook defined, use this one */
      if (!logbook[0])
        {
        for (i=n=0 ; ; i++)
          {
          if (!enumgrp(i, str))
            break;
          if (!equal_ustring(str, "global"))
            n++;
          }

        if (n == 1)
          {
          strcpy(logbook, str);
          strcpy(logbook_enc, logbook);
          url_encode(logbook_enc);
          }
        }

      /* force re-read configuration file */
      if (cfgbuffer)
        {
        free(cfgbuffer);
        cfgbuffer = NULL;
        }
      
      /* set my own URL */
      getcfg("global", "URL", str);
      if (str[0])
        {
        if (str[strlen(str)-1] != '/')
          strcat(str, "/");
        strcpy(elogd_url, str);
        strcpy(elogd_full_url, str);
        }
      else
        {
        /* use relative pathnames */
        sprintf(elogd_url, "/");

        if (tcp_port == 80)
          sprintf(elogd_full_url, "http://%s/", host_name);
        else
          sprintf(elogd_full_url, "http://%s:%d/", host_name, tcp_port);
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

      /* check for Keep-alive */
      if (strstr(net_buffer, "Keep-Alive") != NULL && use_keepalive)
        keep_alive = TRUE;

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

        keep_alive = FALSE;
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
          decode_get(p, cookie_wpwd, cookie_dpwd);
          }
        else if (strncmp(net_buffer, "POST", 4) == 0)
          {
          if (verbose)
            printf("%s\n", net_buffer+header_length);
          decode_post(net_buffer+header_length, boundary, content_length, cookie_wpwd, cookie_dpwd);
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

        if (keep_alive && strstr(return_buffer, "Content-Length") == NULL)
          {
          /*---- add content-length ----*/

          p = strstr(return_buffer, "\r\n\r\n");

          if (p != NULL)
            {
            length = strlen(p+4);

            header_length = (int) (p) - (int) return_buffer;
            memcpy(header_buffer, return_buffer, header_length);
            
            sprintf(header_buffer+header_length, "\r\nContent-Length: %d\r\n\r\n", length);

            send(_sock, header_buffer, strlen(header_buffer), 0);
            send(_sock, p+2, length, 0);

            if (verbose)
              {
              printf("==== Return ================================\n");
              puts(header_buffer);
              puts(p+2);
              printf("\n");
              }
            }
          else
            printf("Internal error, no valid header!\n");
          }
        else
          {
          send(_sock, return_buffer, return_length, 0);

          if (verbose)
            {
            printf("==== Return ================================\n");
            puts(return_buffer);
            printf("\n\n");
            }
          }
  error:

        if (!keep_alive)
          {
          closesocket(_sock);
          ka_sock[i_conn] = 0;

#ifdef DEBUG_CONN        
          printf("## close connection %d\n", i_conn);
#endif
          }
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
char read_pwd[80], write_pwd[80], delete_pwd[80], str[80];
time_t now;
struct tm *tms;

  read_pwd[0] = write_pwd[0] = delete_pwd[0] = logbook[0] = 0;

  strcpy(cfg_file, "elogd.cfg");

  use_keepalive = TRUE;
  
  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'v')
      verbose = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'k')
      use_keepalive = FALSE;
    else if (argv[i][1] == 't')
      {
      tzset();
      time(&now);
      tms = localtime(&now);
      printf("Acutal date/time: %02d%02d%02d_%02d%02d%02d\n",
             tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
             tms->tm_hour, tms->tm_min, tms->tm_sec);
      return 0;
      }
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
      else if (argv[i][1] == 'd')
        strcpy(delete_pwd, argv[++i]);
      else if (argv[i][1] == 'l')
        strcpy(logbook, argv[++i]);
      else
        {
usage:
        printf("usage: %s [-p port] [-D] [-c file] [-r pwd] [-w pwd] [-d pwd] [-l loggbook]\n\n", argv[0]);
        printf("       -p <port> TCP/IP port\n");
        printf("       -D become a daemon\n");
        printf("       -c <file> specify configuration file\n");
        printf("       -v debugging output\n");
        printf("       -r create/overwrite read password in config file\n");
        printf("       -w create/overwrite write password in config file\n");
        printf("       -d create/overwrite delete password in config file\n");
        printf("       -l <loogbook> specify logbook for -r and -w commands\n\n");
        printf("       -k do not use keep-alive\n\n");
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

  if (delete_pwd[0])
    {
    if (!logbook[0])
      {
      printf("Must specify a lookbook via the -l parameter.\n");
      return 0;
      }
    base64_encode(delete_pwd, str);
    create_password(logbook, "Delete Password", str);
    return 0;
    }

  /* extract directory from configuration file */
  if (cfg_file[0] && strchr(cfg_file, DIR_SEPARATOR))
    {
    strcpy(cfg_dir, cfg_file);
    for (i=strlen(cfg_dir)-1 ; i>0 ; i--)
      {
      if (cfg_dir[i] == DIR_SEPARATOR)
        break;
      cfg_dir[i] = 0;
      }
    }

  server_loop(tcp_port, daemon);

  return 0;
}

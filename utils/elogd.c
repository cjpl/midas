/********************************************************************\

  Name:         elogd.c
  Created by:   Stefan Ritt

  Contents:     Web server program for Electronic Logbook ELOG

  $Log$
  Revision 1.98  2001/12/21 13:28:08  midas
  Adjusted spaces in find page display for link with message number

  Revision 1.97  2001/12/17 12:11:22  midas
  Added "Preset <attrib> = %03d"

  Revision 1.96  2001/12/14 14:24:45  midas
  Couple of changes, see CHANGELOG

  Revision 1.95  2001/12/14 12:48:44  midas
  Fixed concatenation of lines in password file, thanks to Michael Buselli

  Revision 1.94  2001/12/14 11:47:01  midas
  Display active URLs also in find page

  Revision 1.93  2001/12/13 13:05:18  midas
  Added missing "reply" etc. translations

  Revision 1.92  2001/12/12 16:22:25  midas
  Added "HTML default" option, fixed "reply" link display

  Revision 1.91  2001/12/12 16:01:18  midas
  Fixed wrong help file name and preceeding " | " for MOptions

  Revision 1.90  2001/12/11 13:41:31  midas
  Added "MOptions" multiple options

  Revision 1.89  2001/12/11 10:20:58  midas
  Changed the way "language" is interpreted, no need to restart elogd after
  language change any more.

  Revision 1.88  2001/12/11 09:39:28  midas
  Format "entry date" according to "date format"

  Revision 1.87  2001/12/11 09:14:51  midas
  Added elog language files

  Revision 1.86  2001/12/10 09:18:09  midas
  Call tzset() only once

  Revision 1.85  2001/12/10 09:06:57  midas
  Fixed memeory leak with ss_file_find

  Revision 1.84  2001/12/07 13:02:45  midas
  Fixed crash with language file in Unix format

  Revision 1.83  2001/12/07 11:41:19  midas
  Made "move to" and "copy to" work with other languages

  Revision 1.82  2001/12/07 10:05:30  midas
  Fixed major bug with memcpy(rem_addr...), many thanks to Frank Schnell!

  Revision 1.81  2001/12/07 09:15:37  midas
  Added Per-Session cookies (when expriation == 0)

  Revision 1.80  2001/12/06 15:47:24  midas
  First version of localization

  Revision 1.79  2001/12/06 09:22:36  midas
  Fixed bug with "full_name"

  Revision 1.78  2001/12/05 16:01:05  midas
  Removed DWORD, changed (int)p1-(int)p2 to (int)(p1-p2) for True64

  Revision 1.77  2001/12/05 10:37:09  midas
  Removed most -Wall compiler warnings

  Revision 1.76  2001/12/04 14:43:53  midas
  Changed global "Welcome page" into "Selection page"

  Revision 1.75  2001/12/04 12:13:21  midas
  Fixed "Date format" problem in short listings

  Revision 1.74  2001/12/04 08:53:03  midas
  Fixed bugs with resubmit, display locked attributes as text only, added
  -h flag for multi-homes hosts thanks to Scott Erickson

  Revision 1.73  2001/11/21 16:26:29  midas
  Removed "Revision date:" from edit form (should better be done via an
  attribute, preset to $date)

  Revision 1.72  2001/11/21 15:23:48  midas
  Fixed bug with "Welcome page"

  Revision 1.71  2001/11/21 15:03:35  midas
  Added "resubmit as new entry" checkbox

  Revision 1.70  2001/11/21 13:31:47  midas
  Made "date format" also work in find result page

  Revision 1.69  2001/11/21 08:21:42  midas
  Specify %20 in menu commands explicitly

  Revision 1.68  2001/11/20 16:14:37  midas
  Corrected "data:" format in sendmail

  Revision 1.67  2001/11/20 12:24:36  midas
  Add more spaces between attachments in find result page

  Revision 1.66  2001/11/20 09:16:33  midas
  Added "Last x" in "find menu commands"

  Revision 1.65  2001/11/19 17:14:25  midas
  Made "bottom text" work with find page and added "find menu commands"

  Revision 1.64  2001/11/19 11:14:45  midas
  Added "default" options, "use email from" and fixed a few small bugs

  Revision 1.63  2001/11/16 15:29:36  midas
  First fine-tuning for GNU software directory

  Revision 1.62  2001/11/15 16:08:07  midas
  Logout takes you back to the logbook selection page

  Revision 1.61  2001/11/15 11:54:25  midas
  Version 1.2.3

  Revision 1.60  2001/11/14 10:51:02  midas
  Fixed bug with resubmission of attachments

  Revision 1.59  2001/11/14 08:58:56  midas
  Version 1.2.2

  Revision 1.58  2001/11/13 14:17:09  midas
  More new features

  Revision 1.57  2001/11/12 15:22:27  midas
  Fixed bug with admin password and checking of allowed commands

  Revision 1.56  2001/11/12 15:08:58  midas
  Changes made in hospital

  Revision 1.55  2001/11/06 15:52:02  midas
  Fixed small bug

  Revision 1.54  2001/11/05 16:31:27  midas
  Added "page title" and changed "use email subject"

  Revision 1.53  2001/11/05 14:44:17  midas
  Fixed problem with long configuration files

  Revision 1.52  2001/11/05 12:37:36  midas
  Version 1.2.1

  Revision 1.51  2001/11/05 12:36:15  midas
  Added <errno.h> for Unix include files

  Revision 1.50  2001/11/05 12:33:30  midas
  Fixed bugs concerning edit/reply/delete and admin password

  Revision 1.49  2001/10/31 11:12:02  midas
  Changes made at home...

  Revision 1.48  2001/10/19 14:36:59  midas
  Added "Display email recipients", "suppress default = 2", moved SMTP host to
  [global]

  Revision 1.47  2001/10/19 11:34:07  midas
  Added "Welcome title" for logbook selection page

  Revision 1.46  2001/10/19 10:33:02  midas
  Added "boolean" attributes, "filtered browsing" and "display search"

  Revision 1.45  2001/10/18 13:06:32  midas
  Added attributes, IP checking, reverse search, copy/move, highlight in search

  Revision 1.44  2001/10/12 10:48:27  midas
  Version 1.1.3

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
#define VERSION "1.3.2"

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

#define __USE_XOPEN /* needed for crypt() */

typedef int BOOL;

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define closesocket(s) close(s)
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

typedef int INT;

#define TELL(fh) lseek(fh, 0, SEEK_CUR)

#define NAME_LENGTH 256
#define SUCCESS       1

#define EL_SUCCESS    1
#define EL_FIRST_MSG  2
#define EL_LAST_MSG   3
#define EL_FILE_ERROR 4

#define WEB_BUFFER_SIZE 2000000

char return_buffer[WEB_BUFFER_SIZE];
int  strlen_retbuf;
int  keep_alive;
char header_buffer[1000];
int  return_length;
char host_name[256];
char elogd_url[256];
char elogd_full_url[256];
char logbook[256];
char logbook_enc[256];
char data_dir[256];
char cfg_file[256];
char cfg_dir[256];
char tcp_hostname[256];

#define MAX_GROUPS       32
#define MAX_PARAM       100
#define MAX_ATTACHMENTS  10
#define MAX_N_LIST      100
#define MAX_N_ATTR       50
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

/* attribute flags */
#define AF_REQUIRED           (1<<0)
#define AF_LOCKED             (1<<1)
#define AF_MULTI              (1<<2)
#define AF_FIXED              (1<<3)

char attr_list[MAX_N_ATTR][NAME_LENGTH];
char attr_options[MAX_N_ATTR][MAX_N_LIST][NAME_LENGTH];
int  attr_flags[MAX_N_ATTR];

char attr_list_default[][NAME_LENGTH] = {
  "Author",
  "Type",
  "Category",
  "Subject",
  ""
};

char attr_options_default[][MAX_N_LIST][NAME_LENGTH] = {
  { "" },
  { "Routine", "Other" },
  { "General", "Other" },
  { "" }
};

int attr_flags_default[] = {
  AF_REQUIRED,
  0,
  0,
  0
};


struct {
  char ext[32];
  char type[32];
  } filetype[] = {

  { ".JPG",   "image/jpeg" },
  { ".GIF",   "image/gif" },
  { ".PNG",   "image/png" },
  { ".PS",    "application/postscript" },
  { ".EPS",   "application/postscript" },
  { ".HTML",  "text/html" },
  { ".HTM",   "text/html" },
  { ".XLS",   "application/x-msexcel" },
  { ".DOC",   "application/msword" },
  { ".PDF",   "application/pdf" },
  { ".TXT",   "text/plain" },
  { ".ASC",   "text/plain" },
  { ".ZIP",   "application/x-zip-compressed" },
  { "" },
};

int scan_attributes(char *logbook);

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

void strsubst(char *string, char name[][NAME_LENGTH], char value[][NAME_LENGTH], int n)
/* subsitute "$name" with value corresponding to name */
{
int  i, j;
char tmp[1000], str[256], uattr[256], *ps, *pt, *p;

  pt = tmp;
  ps = string;
  for (p = strchr(ps, '$') ; p != NULL ; p = strchr(ps, '$'))
    {
    /* copy leading characters */
    j = (int)(p-ps);
    memcpy(pt, ps, j);
    pt += j;
    p++;

    /* extract name */
    strcpy(str, p);
    for (j=0 ; j<(int)strlen(str) ; j++)
      str[j] = toupper(str[j]);

    /* search name */
    for (i=0 ; i<n ; i++)
      {
      strcpy(uattr, name[i]);
      for (j=0 ; j<(int)strlen(uattr) ; j++)
        uattr[j] = toupper(uattr[j]);

      if (strncmp(str, uattr, strlen(uattr)) == 0)
        break;
      }

    /* copy value */
    if (i<n)
      {
      strcpy(pt, value[i]);
      pt += strlen(pt);
      ps = p + strlen(uattr);
      }
    else
      {
      *pt++ = '$';
      ps = p;
      }
    }

  /* copy remainder */
  strcpy(pt, ps);

  /* return result */
  strcpy(string, tmp);
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

void do_crypt(char *s, char *d)
{
#ifdef USE_CRYPT
  strcpy(d, crypt(s, "el"));
#else
  base64_encode(s, d);
#endif
}

/*-------------------------------------------------------------------*/

INT recv_string(int sock, char *buffer, INT buffer_size, INT millisec)
{
INT            i, n;
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
struct tm            *ts;

  if (verbose)
    printf("\n\nEmail from %s to %s, SMTP host %s:\n", from, to, smtp_host);

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

  recv_string(s, str, sizeof(str), 10000);
  if (verbose) puts(str);

  snprintf(str, sizeof(str) - 1, "HELO %s\r\n", host_name);
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);
  recv_string(s, str, sizeof(str), 3000);
  if (verbose) puts(str);

  snprintf(str, sizeof(str) - 1, "MAIL FROM: <%s>\r\n", from);
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);
  recv_string(s, str, sizeof(str), 3000);
  if (verbose) puts(str);

  snprintf(str, sizeof(str) - 1, "RCPT TO: <%s>\r\n", to);
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);
  recv_string(s, str, sizeof(str), 3000);
  if (verbose) puts(str);

  snprintf(str, sizeof(str) - 1, "DATA\r\n");
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);
  recv_string(s, str, sizeof(str), 3000);
  if (verbose) puts(str);

  snprintf(str, sizeof(str) - 1, "To: %s\r\nFrom: %s\r\nSubject: %s\r\n", to, from, subject);
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);

  time(&now);
  ts = localtime(&now);
  strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S", ts);
  snprintf(str, sizeof(str) - 1, "Date: %s %+03d%02d\r\n\r\n", buf, (int) (-timezone/3600), (int) ((abs(timezone)/60) % 60));
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);

  snprintf(str, sizeof(str) - 1, "%s\r\n.\r\n", text);
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);
  recv_string(s, str, sizeof(str), 3000);
  if (verbose) puts(str);

  snprintf(str, sizeof(str) - 1, "QUIT\r\n");
  send(s, str, strlen(str), 0);
  if (verbose) puts(str);
  recv_string(s, str, sizeof(str), 3000);
  if (verbose) puts(str);

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
      return 0;
      }
    if (fd != i)
      {
      printf("Did not get file descriptor");
      return 0;
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

  /* if parameter not found in logbook, look in [global] section */
  if (!equal_ustring(group, "global"))
    return getcfg("global", param, value);

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

    while (*p && *p != '\r' && *p != '\n')
      p++;
    while (*p && (*p == '\r' || *p == '\n'))
      p++;
    } while (*p);

  return 0;
}

/*-------------------------------------------------------------------*/

char *locbuffer = NULL;
char **porig, **ptrans;

/* localization support */
char *loc(char *orig)
{
char language[256], file_name[256], *p;
int  fh, length, n;
static char old_language[256];

  getcfg("global", "Language", language);

  if (!equal_ustring(language, old_language))
    {
    if (equal_ustring(language, "english") ||
        language[0] == 0)
      {
      if (locbuffer)
        {
        free(locbuffer);
        locbuffer = NULL;
        }
      }
    else
      {
      strcpy(file_name, cfg_dir);
      strcat(file_name, "eloglang.");
      strcat(file_name, language);

      fh = open(file_name, O_RDONLY | O_BINARY);
      if (fh < 0)
        return orig;

      length = lseek(fh, 0, SEEK_END);
      lseek(fh, 0, SEEK_SET);
      locbuffer = malloc(length+1);
      if (locbuffer == NULL)
        {
        close(fh);
        return orig;
        }
      read(fh, locbuffer, length);
      locbuffer[length] = 0;
      close(fh);

      /* scan lines, setup orig-translated pointers */
      p = locbuffer;
      n = 0;
      do
        {
        while (*p && (*p == '\r' || *p == '\n'))
          p++;

        if (*p && (*p == ';' || *p == '#' || *p == ' ' || *p == '\t'))
          {
          while (*p && *p != '\n' && *p != '\r')
            p++;
          continue;
          }

        if (n == 0)
          {
          porig = malloc(sizeof(char *) * 2);
          ptrans = malloc(sizeof(char *) * 2);
          }
        else
          {
          porig = realloc(porig, sizeof(char *) * (n + 2));
          ptrans = realloc(ptrans, sizeof(char *) * (n + 2));
          }

        porig[n] = p;
        while (*p && (*p != '=' && *p != '\r' && *p != '\n'))
          p++;

        if (*p && *p != '=')
          continue;

        ptrans[n] = p + 1;
        while (*ptrans[n] == ' ')
          ptrans[n] ++;

        /* remove '=' and surrounding blanks */
        while (*p == '=' || *p == ' ')
          *p-- = 0;

        p = ptrans[n];
        while (*p && *p != '\n' && *p != '\r')
          p++;

        if (p)
          *p++ = 0;

        n++;
        } while (p && *p);

      porig[n] = NULL;
      }

    strcpy(old_language, language);
    }

  if (!locbuffer)
    return orig;

  /* search string and return translation */
  for (n = 0; porig[n] ; n++)
    if (strcmp(orig, porig[n]) == 0)
      {
      if (*ptrans[n])
        return ptrans[n];
      return orig;
      }
   
  return orig;
}

/*------------------------------------------------------------------*/

/* Parameter extraction from themes file  */

typedef struct {
  char name[32];
  char value[256];
  } THEME;

THEME default_theme [] = {

  { "Table width",            "100%"    },
  { "Frame color",            "#486090" },
  { "Cell BGColor",           "#E6E6E6" },
  { "Border width",           "0"       },

  { "Title BGColor",          "#486090" },
  { "Title Fontcolor",        "#FFFFFF" },
  { "Title Cellpadding",      "0"       },
  { "Title Image",            ""        },

  { "Merge menus",            "1"       },
  { "Use buttons",            "0"       },

  { "Menu1 BGColor",          "#E0E0E0" },
  { "Menu1 Align",            "left"    },
  { "Menu1 Cellpadding",      "0"       },

  { "Menu2 BGColor",          "#FFFFB0" },
  { "Menu2 Align",            "center"  },
  { "Menu2 Cellpadding",      "0"       },
  { "Menu2 use images",       "0"       },

  { "Categories cellpadding", "3"       },
  { "Categories border",      "0"       },
  { "Categories BGColor1",    "#CCCCFF" },
  { "Categories BGColor2",    "#DDEEBB" },

  { "Text BGColor",           "#FFFFFF" },

  { "List bgcolor1",          "#DDEEBB" },
  { "List bgcolor2",          "#FFFFB0" },

  { "" }

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

  if (theme == NULL)
    loadtheme(NULL);

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
    for (pc=strstr(message, key)+strlen(key) ; *pc != '\n' && *pc != '\r' ; )
      *result++ = *pc++;
    *result = 0;
    }
}

/*------------------------------------------------------------------*/

/* Simplified copy of fnmatch() for Cygwin where fnmatch is not defined */

#define EOS '\0'

int fnmatch1(const char *pattern, const char *string)
{
const char *stringstart;
char c, test;

  for (stringstart = string;;)
    switch (c = *pattern++) {
    case EOS:
      return (*string == EOS ? 0 : 1);
    case '?':
      if (*string == EOS)
        return (1);
      ++string;
      break;
    case '*':
      c = *pattern;
      /* Collapse multiple stars. */
      while (c == '*')
        c = *++pattern;

      /* Optimize for pattern with * at end or before /. */
      if (c == EOS)
        return (0);

      /* General case, use recursion. */
      while ((test = *string) != EOS)
        {
        if (!fnmatch1(pattern, string))
          return (0);
        ++string;
        }
      return (1);
      /* FALLTHROUGH */
    default:
      if (c != *string)
        return (1);
      string++;
      break;
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
  int i;

#ifdef OS_UNIX
  DIR *dir_pointer;
  struct dirent *dp;

  if ((dir_pointer = opendir(path)) == NULL)
    return 0;
  *plist = (char *) malloc(MAX_PATH_LENGTH);
  i = 0;
  for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer))
    {
    if (fnmatch1(pattern, dp->d_name) == 0)
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
  char str[255];
  int first;

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
int    lfh, i, n, d, min, max, size, offset, direction, last, status, did_walk;
struct tm *tms, ltms;
time_t lt, ltime, lact;
char   str[256], file_name[256], dir[256];
char   *file_list;

  did_walk = 0;
  ltime = lfh = 0;

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

    if (ltime < 0)
      return -1;

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

        did_walk = 1;

        if (direction == -1)
          ltime -= 3600*24; /* one day back */
        else
          ltime += 3600*24; /* go forward one day */

        /* set new tag */
        tms = localtime(&ltime);
        sprintf(tag, "%02d%02d%02d.0", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);
        }

      /* in forward direction, stop today */
      if (direction != -1 && ltime > (time_t)(time(NULL)+3600*24))
        break;

      /* in backward direction, go back 10 years */
      if (direction == -1 && abs((int)(lt-ltime)) > 3600*24*365*10)
        break;

      } while (lfh < 0);

    if (lfh < 0)
      return EL_FILE_ERROR;

    if (direction == -1 && did_walk)
      lseek(lfh, 0, SEEK_END);
    else
      lseek(lfh, offset, SEEK_SET);
    }

  /* open most recent file if no tag given */
  if (tag[0] == 0)
    {
    if (first)
      {
      /* go through whole directory, find first file */

      file_list = NULL;
      n = ss_file_find(dir, "??????.log", &file_list);
      if (n == 0)
        {
        if (file_list)
          free(file_list);
        return EL_FILE_ERROR;
        }

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

      file_list = NULL;
      n = ss_file_find(dir, "??????.log", &file_list);
      if (n == 0)
        {
        if (file_list)
          free(file_list);
        return EL_FILE_ERROR;
        }

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
      sprintf(tag, "%06d.%d", (int) (max % 1000000), (int) (TELL(lfh)));
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
    sprintf(strchr(tag, '.')+1, "%d", (int) (TELL(lfh)));
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
    sprintf(strchr(tag, '.')+1, "%d", (int) (TELL(lfh)));
    }

  if (fh)
    *fh = lfh;
  else
    close(lfh);

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_submit(char attr_name[MAX_N_ATTR][NAME_LENGTH],
              char attr_value[MAX_N_ATTR][NAME_LENGTH],
              int n_attr, char *text, char *reply_to, char *encoding,
              char afilename[MAX_ATTACHMENTS][256],
              char *buffer[MAX_ATTACHMENTS],
              INT buffer_size[MAX_ATTACHMENTS],
              char *tag, INT tag_size)
/********************************************************************\

  Routine: el_submit

  Purpose: Submit an ELog entry

  Input:
    char   attr_name[][]    Name of attributes
    char   attr_value[][]   Value of attributes
    int    n_attr           Number of attributes

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
INT     n, i, size, fh, status, index, offset, tail_size;
struct  tm *tms;
char    file_name[256], afile_name[MAX_ATTACHMENTS][256], dir[256], str[256],
        start_str[80], end_str[80], last[80], date[80], thread[80], attachment_all[64*MAX_ATTACHMENTS];
time_t  now;
char    message[TEXT_SIZE+100], *p;
BOOL    bedit;

  bedit = (tag[0] != 0);

  for (index = 0 ; index < MAX_ATTACHMENTS ; index++)
    {
    /* generate filename for attachment */
    afile_name[index][0] = file_name[0] = 0;

    if (equal_ustring(afilename[index], loc("<delete>")))
      {
      strcpy(afile_name[index], loc("<delete>"));
      }
    else if (afilename[index][0] && buffer_size[index] == 0)
      {
      /* resubmission of existing attachment */
      strcpy(afile_name[index], afilename[index]);
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
    el_decode(message, "Attachment: ", attachment_all);

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

  for (i=0 ; i<n_attr ; i++)
    sprintf(message+strlen(message), "%s: %s\n", attr_name[i], attr_value[i]);

  /* keep original attachment if edit and no new attachment */

  if (bedit)
    {
    for (i=n=0 ; i<MAX_ATTACHMENTS ; i++)
      {
      if (i == 0)
        p = strtok(attachment_all, ",");
      else
        if (p != NULL)
          p = strtok(NULL, ",");

      if (p && (afile_name[i][0] || equal_ustring(afile_name[i], loc("<delete>"))))
        {
        /* delete old attachment */
        strcpy(str, data_dir);
        strcat(str, p);
        remove(str);
        }

      if (afile_name[i][0] && !equal_ustring(afile_name[i], loc("<delete>")))
        {
        if (n == 0)
          {
          sprintf(message+strlen(message), "Attachment: %s", afile_name[i]);
          n++;
          }
        else
          sprintf(message+strlen(message), ",%s", afile_name[i]);
        }

      else if (p && !equal_ustring(afile_name[i], loc("<delete>")))
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
    sprintf(tag, "%02d%02d%02d.%d", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday, (int) (TELL(fh)));

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

INT el_retrieve(char *tag, char *date, char attr_list[MAX_N_ATTR][NAME_LENGTH],
                char attrib[MAX_N_ATTR][NAME_LENGTH], int n_attr,
                char *text, int *textsize,
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
    char   attr_list        Names of attributes
    char   attrib           Values of attributes
    int    n_attr           Number of attributes
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
char    message[TEXT_SIZE+100], thread[256], attachment_all[64*MAX_ATTACHMENTS];

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
  i = read(fh, str, 16);
  if (i <= 0)
    {
    close(fh);
    return EL_FILE_ERROR;
    }

  if (strncmp(str, "$Start$: ", 9) == 0)
    size = atoi(str+9);
  else
    {
    close(fh);
    return EL_FILE_ERROR;
    }

  /* read message */
  memset(message, 0, sizeof(message));
  read(fh, message, size);
  close(fh);

  /* decode message */
  el_decode(message, "Date: ", date);
  el_decode(message, "Thread: ", thread);

  for (i=0 ; i<n_attr ; i++)
    {
    sprintf(str, "%s: ", attr_list[i]);
    el_decode(message, str, attrib[i]);
    }

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

  /* check for \n -> \r conversion (e.g. zipping/unzipping) */
  if (p == NULL)
    p = strstr(message, "========================================\r");

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

INT el_delete_message(char *tag)
/********************************************************************\

  Routine: el_delete_message

  Purpose: Delete an ELog entry including attachments

  Input:
    char   *tag             Message tage

  Output:
    <none>

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
INT  i, n, size, fh, offset, tail_size, status, n_attr;
char dir[256], str[256], file_name[256];
char *buffer, attrib[MAX_N_ATTR][NAME_LENGTH];
char date[80], text[TEXT_SIZE], orig_tag[80], reply_tag[80],
     attachment[MAX_ATTACHMENTS][256], encoding[80];

  n_attr = scan_attributes(logbook);

  /* get attachments */
  size = sizeof(text);
  status = el_retrieve(tag, date, attr_list, attrib, n_attr,
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

INT el_delete_message_only(char *tag, char afilename[MAX_ATTACHMENTS][256])
/********************************************************************\

  Routine: el_delete_message_only

  Purpose: Delete an ELog entry, but keeping (and returning) attachments

  Input:
    char   *tag             Message tage

  Output:
    <none>

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
INT  i, n, size, fh, offset, tail_size, status, n_attr;
char dir[256], str[256], file_name[256];
char *buffer, attrib[MAX_N_ATTR][NAME_LENGTH];
char date[80], text[TEXT_SIZE], orig_tag[80], reply_tag[80],
     attachment[MAX_ATTACHMENTS][256], encoding[80];

  n_attr = scan_attributes(logbook);

  /* get attachments */
  size = sizeof(text);
  status = el_retrieve(tag, date, attr_list, attrib, n_attr,
                       text, &size, orig_tag, reply_tag,
                       attachment,
                       encoding);
  if (status != SUCCESS)
    return EL_FILE_ERROR;

  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    {
    if (attachment[i][0] && afilename[i][0])
      {
      /* delete old attachment if new one exists */
      strcpy(str, data_dir);
      strcat(str, attachment[i]);
      remove(str);
      }

    /* return old attachment if no new one */
    if (attachment[i][0] && !afilename[i][0])
      strcpy(afilename[i], attachment[i]);
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
  if (strlen_retbuf + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    strcpy(return_buffer+strlen_retbuf, str);

  strlen_retbuf += strlen(str);
}

/*------------------------------------------------------------------*/

void rsputs2(const char *str)
{
int i, j, k;
char *p, link[256];

  if (strlen_retbuf + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    {
    j = strlen_retbuf;
    for (i=0 ; i<(int)strlen(str) ; i++)
      {
      if (strncmp(str+i, "http://", 7) == 0)
        {
        p = (char *) (str+i+7);
        i += 7;
        for (k=0 ; *p && *p != ' ' && *p != ',' && *p != '\t' && *p != '\n' && *p != '\r'; k++,i++)
          link[k] = *p++;
        link[k] = 0;
        i--;

        sprintf(return_buffer+j, "<a href=\"http://%s\">http://%s</a>", link, link);
        j += strlen(return_buffer+j);
        }
      else if (strncmp(str+i, "https://", 8) == 0)
        {
        p = (char *) (str+i+8);
        i += 8;
        for (k=0 ; *p && *p != ' ' && *p != ',' && *p != '\t' && *p != '\n' && *p != '\r'; k++,i++)
          link[k] = *p++;
        link[k] = 0;
        i--;

        sprintf(return_buffer+j, "<a href=\"https://%s\">https://%s</a>", link, link);
        j += strlen(return_buffer+j);
        }
      else if (strncmp(str+i, "ftp://", 6) == 0)
        {
        p = (char *) (str+i+6);
        i += 6;
        for (k=0 ; *p && *p != ' ' && *p != ',' && *p != '\t' && *p != '\n' && *p != '\r'; k++,i++)
          link[k] = *p++;
        link[k] = 0;
        i--;

        sprintf(return_buffer+j, "<a href=\"ftp://%s\">ftp://%s</a>", link, link);
        j += strlen(return_buffer+j);
        }
      else if (strncmp(str+i, "mailto:", 7) == 0)
        {
        p = (char *) (str+i+7);
        i += 7;
        for (k=0 ; *p && *p != '>' && *p != ' ' && *p != ',' && *p != '\t' && *p != '\n' && *p != '\r'; k++,i++)
          link[k] = *p++;
        link[k] = 0;
        i--;

        sprintf(return_buffer+j, "<a href=\"mailto:%s\">%s</a>", link, link);
        j += strlen(return_buffer+j);
        }
      else
        switch (str[i])
          {
          case 1  : strcat(return_buffer, "<");    j++;  break;
          case 2  : strcat(return_buffer, ">");    j++;  break;
          case '<': strcat(return_buffer, "&lt;"); j+=4; break;
          case '>': strcat(return_buffer, "&gt;"); j+=4; break;
          default: return_buffer[j++] = str[i];
          }
      }

    return_buffer[j] = 0;
    strlen_retbuf = j;
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

  if (strlen_retbuf + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    strcpy(return_buffer+strlen_retbuf, str);

  strlen_retbuf += strlen(str);
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
    for ( ; i<MAX_PARAM-1 ; i++)
      {
      strcpy(_param[i], _param[i+1]);
      strcpy(_value[i], _value[i+1]);
      }
    _param[MAX_PARAM-1][0] = 0;
    _value[MAX_PARAM-1][0] = 0;
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

  rsprintf("Location: /%s/%s\r\n\r\n<html>redir</html>\r\n", logbook_enc, path);
}

void redirect2(char *path)
{
  redirect(path);
  send(_sock, return_buffer, strlen(return_buffer)+1, 0);
  closesocket(_sock);
  return_length = -1;
}

void redirect3(char *path)
{
  /* redirect */
  rsprintf("HTTP/1.1 302 Found\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }

  rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", path);
}

/*------------------------------------------------------------------*/

int strbreak(char *str, char list[][NAME_LENGTH], int size)
/* break comma-separated list into char array, stripping leading
   and trailing blanks */
{
int i;
char *p;

  memset(list, 0, size*NAME_LENGTH);
  p = strtok(str, ",");
  if (!p)
    return 0;
  while (*p == ' ')
    p++;

  for (i=0 ; p && i<size ; i++)
    {
    strcpy(list[i], p);
    while (list[i][strlen(list[i])-1] == ' ')
      list[i][strlen(list[i])-1] = 0;

    p = strtok(NULL, ",");
    if (!p)
      break;
    while (*p == ' ')
      p++;
    }

  if (i == size)
    return size;

  return i+1;
}

/*------------------------------------------------------------------*/

int scan_attributes(char *logbook)
/* scan configuration file for attributes and fill attr_list, attr_options
   and attr_flags arrays */
{
char list[10000], str[256], tmp_list[MAX_N_ATTR][NAME_LENGTH];
int  i, j, n, m;

  if (getcfg(logbook, "Attributes", list))
    {
    /* reset attribute flags */
    memset(attr_flags, 0, sizeof(attr_flags));

    /* get attribute list */
    memset(attr_list, 0, sizeof(attr_list));
    n = strbreak(list, attr_list, MAX_N_ATTR);

    /* get options lists for attributes */
    memset(attr_options, 0, sizeof(attr_options));
    for (i=0 ; i<n ; i++)
      {
      sprintf(str, "Options %s", attr_list[i]);
      if (getcfg(logbook, str, list))
        strbreak(list, attr_options[i], MAX_N_LIST);

      sprintf(str, "MOptions %s", attr_list[i]);
      if (getcfg(logbook, str, list))
        {
        strbreak(list, attr_options[i], MAX_N_LIST);
        attr_flags[i] |= AF_MULTI;
        }
      }

    /* check if attribut required */
    getcfg(logbook, "Required Attributes", list);
    m = strbreak(list, tmp_list, MAX_N_ATTR);
    for (i=0 ; i<m ; i++)
      {
      for (j=0 ; j<n ; j++)
        if (equal_ustring(attr_list[j], tmp_list[i]))
          attr_flags[j] |= AF_REQUIRED;
      }

    /* check if locked attribut */
    getcfg(logbook, "Locked Attributes", list);
    m = strbreak(list, tmp_list, MAX_N_ATTR);
    for (i=0 ; i<m ; i++)
      {
      for (j=0 ; j<n ; j++)
        if (equal_ustring(attr_list[j], tmp_list[i]))
          attr_flags[j] |= AF_LOCKED;
      }

    /* check if fixed attribut */
    getcfg(logbook, "Fixed Attributes", list);
    m = strbreak(list, tmp_list, MAX_N_ATTR);
    for (i=0 ; i<m ; i++)
      {
      for (j=0 ; j<n ; j++)
        if (equal_ustring(attr_list[j], tmp_list[i]))
          attr_flags[j] |= AF_FIXED;
      }
    }
  else
    {
    memcpy(attr_list, attr_list_default, sizeof(attr_list_default));
    memcpy(attr_options, attr_options_default, sizeof(attr_options_default));
    memcpy(attr_flags, attr_flags_default, sizeof(attr_flags_default));
    n = 4;
    }

  return n;
}

/*------------------------------------------------------------------*/

void show_upgrade_page()
{
char str[1000];

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
  rsprintf("<title>ELOG Electronic Logbook Upgrade Information</title>\n");
  rsprintf("</head>\n\n");

  rsprintf("<body>\n");

  rsprintf("<table border=%s width=100%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>",
            gt("Border width"), gt("Frame color"));
  rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

  rsprintf("<tr><td align=center bgcolor=%s><font size=5 color=%s>ELog Electronic Logbook Upgrade Information</font></td></tr>\n",
            gt("Title bgcolor"), gt("Title fontcolor"));

  rsprintf("<tr><td bgcolor=#FFFFFF><br>\n");

  rsprintf("You probably use an <b>elogd.cfg</b> configuration file for a ELOG version\n");
  rsprintf("1.1.x, since it contains a <b><code>\"Types = ...\"</code></b> entry. From version\n");
  rsprintf("1.2.0 on, the fixed attributes <b>Type</b> and <b>Category</b> have been\n");
  rsprintf("replaced by arbitrary attributes. Please replace these two lines with the\n");
  rsprintf("following entries:<p>\n");
  rsprintf("<pre>\n");
  rsprintf("Attributes = Author, Type, Category, Subject\n");
  rsprintf("Required Attributes = Author\n");
  getcfg(logbook, "Types", str);
  rsprintf("Options Type = %s\n", str);
  getcfg(logbook, "Categories", str);
  rsprintf("Options Category = %s\n", str);
  rsprintf("Page title = $subject\n");
  rsprintf("</pre>\n");
  rsprintf("<p>\n");

  rsprintf("It is of course possible to change the attributes or add new ones. The new\n");
  rsprintf("options in the configuration file are described under <a href=\"\n");
  rsprintf("http://midas.psi.ch/elog/config.html\">http://midas.psi.ch/elog/config.html\n");
  rsprintf("</a>.\n");

  rsprintf("</td></tr>\n");
  rsprintf("</td></tr></table></td></tr></table>\n\n");

  rsprintf("<hr>\n");
  rsprintf("<address>\n");
  rsprintf("<a href=\"http://midas.psi.ch/~stefan\">S. Ritt</a>, 18 October 2001");
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
    rsprintf("<body bgcolor=#FFFFFF><form method=\"GET\" action=\"/%s/%s\">\n\n", logbook_enc, path);
  else
    rsprintf("<body bgcolor=#FFFFFF><form method=\"GET\" action=\"/%s\">\n\n", logbook_enc);
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

  /*---- logbook selection row ----*/

  if (!printable && getcfg("global", "logbook tabs", str) && atoi(str) == 1)
    {
    if (!getcfg("global", "tab cellpadding", str))
      strcpy(str, "5");
    rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=0 bgcolor=#FFFFFF><tr>\n", str);

    if (getcfg("global", "main tab", str))
      {
      rsprintf("<td nowrap bgcolor=#E0E0E0><a href=\"/\">%s</a></td>\n", str);
      rsprintf("<td width=10 bgcolor=#FFFFFF>&nbsp;</td>\n");
      }

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
        rsprintf("<td nowrap bgcolor=#E0E0E0><a href=\"/%s\">%s</a></td>\n", ref, str);
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

  /* use comment as title if available, else logbook name */
  if (!getcfg(logbook, "Comment", str))
    strcpy(str, logbook);

  rsprintf("<font size=3 face=verdana,arial,helvetica,sans-serif color=%s><b>&nbsp;&nbsp;%s%s<b></font>\n",
            gt("Title fontcolor"), str, text);
  rsprintf("&nbsp;</td>\n");

  /* middle cell */
  if (*getparam("full_name"))
    {
    rsprintf("<td bgcolor=%s align=center>", gt("Title BGColor"));
    rsprintf("<font size=1 face=verdana,arial,helvetica,sans-serif color=%s><b>&nbsp;&nbsp;%s \"%s\"<b></font></td>\n",
              gt("Title fontcolor"), loc("Logged in as"), getparam("full_name"));
    }

  /* right cell */
  rsprintf("<td bgcolor=%s align=right>", gt("Title BGColor"));
  if (*gt("Title image"))
    rsprintf("<img src=\"/%s/%s\">", logbook_enc, gt("Title image"));
  else
    rsprintf("<font size=3 face=verdana,arial,helvetica,sans-serif color=%s><b>ELOG V%s&nbsp;&nbsp</b></font>",
              gt("Title fontcolor"), VERSION);
  rsprintf("</td>\n");

  rsprintf("</tr></table></td></tr>\n\n");
}

/*------------------------------------------------------------------*/

void show_error(char *error)
{
  /* header */
  show_standard_header("ELOG error", "");

  rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>",
            gt("Border width"), gt("Frame color"));
  rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));
  rsprintf("<tr><td bgcolor=#FFB0B0 align=center>");

  rsprintf("%s</tr>\n", error);

  rsprintf("<tr><td bgcolor=%s align=center>", gt("Cell BGColor"));

  rsprintf("<button type=button onClick=history.back()>%s</button>\n", loc("Back"));
  /*
  rsprintf("<input type=submit value=\"%s\">", loc("Back"));
  */

  rsprintf("</td></tr>\n</table></td></tr></table>\n");
  rsprintf("</body></html>\n");
}

/*------------------------------------------------------------------*/

void send_file(char *file_name)
{
int    fh, i, length;
char   str[256];
time_t now;
struct tm *gmt;

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
    for (i=0 ; i<(int)strlen(file_name) ; i++)
      str[i] = toupper(file_name[i]);
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
    rsprintf("HTTP/1.1 404 Not Found\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
    rsprintf("Connection: close\r\n");
    rsprintf("Content-Type: text/html\r\n\r\n");
    rsprintf("<html><head><title>404 Not Found</title></head>\r\n");
    rsprintf("<body><h1>Not Found</h1>\r\n");
    rsprintf("The requested file <b>%s</b> was not found on this server<p>\r\n", file_name);
    rsprintf("<hr><address>ELOG version %s</address></body></html>\r\n\r\n", VERSION);
    return_length = strlen_retbuf;
    keep_alive = 0;
    }
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

int build_subst_list(char list[][NAME_LENGTH], char value[][NAME_LENGTH],
                     char attrib[][NAME_LENGTH])
{
int            i;
char           str[256], format[256];
time_t         now;
struct tm      *ts;
struct hostent *phe;

  /* copy attribute list */
  for (i=0 ; i<scan_attributes(logbook) ; i++)
    {
    strcpy(list[i], attr_list[i]);
    if (attrib)
      strcpy(value[i], attrib[i]);
    else
      strcpy(value[i], getparam(attr_list[i]));
    }

  /* add remote host */
  strcpy(list[i], "remote_host");

  phe = gethostbyaddr((char *) &rem_addr, 4, PF_INET);
  if (phe != NULL)
    strcpy(str, phe->h_name);
  else
    strcpy(str, (char *)inet_ntoa(rem_addr));

  strcpy(value[i++], str);

  /* add local host */
  strcpy(list[i], "host");
  strcpy(value[i++], host_name);

  /* add user names */
  strcpy(list[i], "short_name");
  strcpy(value[i++], getparam("unm"));
  strcpy(list[i], "long_name");
  strcpy(value[i++], getparam("full_name"));

  /* add logbook */
  strcpy(list[i], "logbook");
  strcpy(value[i++], logbook);

  /* add date */
  strcpy(list[i], "date");
  time(&now);
  ts = localtime(&now);
  if (getcfg(logbook, "Date format", format))
    strftime(str, sizeof(str), format, ts);
  else
    strcpy(str, ctime(&now));
  strcpy(value[i++], str);

  return i;
}

/*------------------------------------------------------------------*/

void show_change_pwd_page()
{
char   str[256], line[256], *p, *pl, old_pwd[32], new_pwd[32];
char   *buf;
FILE   *f;
int    i, wrong_pwd, size;
double exp;
time_t now;
struct tm *gmt;


  do_crypt(getparam("oldpwd"), old_pwd);
  do_crypt(getparam("newpwd"), new_pwd);

  getcfg(logbook, "Password file", str);
  wrong_pwd = FALSE;

  if (old_pwd[0] || new_pwd[0])
    {
    f = fopen(str, "r+b");
    if (f != NULL)
      {
      fseek(f, 0, SEEK_END);
      size = TELL(fileno(f));
      fseek(f, 0, SEEK_SET);

      buf = malloc(size+1);
      fread(buf, 1, size, f);
      buf[size] = 0;
      pl = buf;

      while (pl < buf+size)
        {
        for (i=0 ; pl[i] && pl[i] != '\r' && pl[i] != '\n' ; i++)
          line[i] = pl[i];
        line[i] = 0;

        if (line[0] == ';' || line[0] == '#' || line[0] == 0)
          {
          pl += strlen(line);
          while (*pl && (*pl == '\r' || *pl == '\n'))
            pl++;
          continue;
          }

        strcpy(str, line);
        if (strchr(str, ':'))
          *strchr(str, ':') = 0;
        if (strcmp(str, getparam("unm")) == 0)
          break;

        pl += strlen(line);
        while (*pl && (*pl == '\r' || *pl == '\n'))
          pl++;
        }

      /* if user found, check old password */
      if (*getparam("unm") && (strcmp(str, getparam("unm")) == 0))
        {
        p = line+strlen(str);
        if (*p)
          p++;

        strcpy(str, p);
        if (strchr(str, ':'))
          *strchr(str, ':') = 0;

        if (strcmp(old_pwd, str) != 0)
          wrong_pwd = TRUE;
        }

      /* replace password */
      if (!wrong_pwd)
        {
        fseek(f, 0, SEEK_SET);
        fwrite(buf, 1, pl-buf, f);

        fprintf(f, "%s:%s:%s\n", getparam("unm"), new_pwd, getparam("full_name"));

        pl += strlen(line);
        while (*pl && (*pl == '\r' || *pl == '\n'))
          pl++;

        fwrite(pl, 1, strlen(pl), f);

#ifdef _MSC_VER
        chsize(fileno(f), TELL(fileno(f)));
#else
        ftruncate(fileno(f), TELL(fileno(f)));
#endif
        }

      free(buf);
      fclose(f);

      if (!wrong_pwd)
        {
        rsprintf("HTTP/1.1 302 Found\r\n");
        rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
        if (use_keepalive)
          {
          rsprintf("Connection: Keep-Alive\r\n");
          rsprintf("Keep-Alive: timeout=60, max=10\r\n");
          }

        /* get optional expriation from configuration file */
        exp = 24;
        if (getcfg(logbook, "Login expiration", str))
          exp = atof(str);

        if (exp == 0)
          {
          rsprintf("Set-Cookie: upwd=%s; path=/%s\r\n", new_pwd, logbook_enc);
          }
        else
          {
          time(&now);
          now += (int) (3600*exp);
          gmt = gmtime(&now);
          strftime(str, sizeof(str), "%A, %d-%b-%Y %H:%M:%S GMT", gmt);

          rsprintf("Set-Cookie: upwd=%s; path=/%s; expires=%s\r\n", new_pwd, logbook_enc, str);
          }

        sprintf(str, "/%s/", logbook_enc);
        rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
        return;
        }
      }
    }

  show_standard_header(loc("ELOG change password"), NULL);

  rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>",
            gt("Border width"), gt("Frame color"));
  rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

  if (wrong_pwd)
    rsprintf("<tr><th bgcolor=#FF0000>%s!</th></tr>\n", loc("Wrong password"));

  rsprintf("<tr><td align=center bgcolor=%s>\n", gt("Title bgcolor"));

  rsprintf("<font color=%s>%s \"%s\"</font></td></tr>\n",
           gt("Title fontcolor"), loc("Change password for user"), getparam("full_name"));

  rsprintf("<tr><td align=center bgcolor=%s>%s:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type=password name=oldpwd></td></tr>\n", 
           gt("Cell BGColor"), loc("Old Password"));
  rsprintf("<tr><td align=center bgcolor=%s>%s:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type=password name=newpwd></td></tr>\n", 
           gt("Cell BGColor"), loc("New Password"));

  rsprintf("<tr><td align=center bgcolor=%s><input type=submit value=\"%s\"></td></tr>", 
           gt("Cell BGColor"), loc("Submit"));

  rsprintf("</table></td></tr></table>\n");

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

BOOL allow_user(char *command)
{
char str[1000], users[2000];
char list[MAX_N_LIST][NAME_LENGTH];
int  i, n;

  /* check for user level access */
  if (!getcfg(logbook, "Password file", str))
    return TRUE;

  /* allow by default */
  sprintf(str, "Allow %s", command);
  if (!getcfg(logbook, str, users))
    return TRUE;

  /* check if current user in list */
  n = strbreak(users, list, MAX_N_LIST);
  for (i=0 ; i<n ; i++)
    if (equal_ustring(list[i], getparam("unm")))
      return TRUE;

  sprintf(str, loc("Error: Command \"<b>%s</b>\" is not allowed for user \"<b>%s</b>\""),
          command, getparam("full_name"));
  show_error(str);

  return FALSE;
}

/*------------------------------------------------------------------*/

int get_last_index(int index)
{
int    i, n_attr;
char   str[80], date[80], attrib[MAX_N_ATTR][NAME_LENGTH],
       orig_tag[80], reply_tag[80], att[MAX_ATTACHMENTS][256], encoding[80];

  str[0] = 0;
  n_attr = scan_attributes(logbook);
  el_retrieve(str, date, attr_list, attrib, n_attr,
              NULL, 0, orig_tag, reply_tag, att, encoding);

  strcpy(str, attrib[index]);

  /* look for first digit, return value */
  for (i=0 ; i<(int)strlen(str) ; i++)
    if (isdigit(str[i]))
      break;

  return atoi(str+i);
}

/*------------------------------------------------------------------*/

void show_elog_new(char *path, BOOL bedit)
{
int    i, n, n_attr, index, size, wrap, fh, length;
char   str[1000], preset[1000], *p, star[80], comment[10000];
char   list[MAX_N_ATTR][NAME_LENGTH], file_name[256], *buffer, format[256];
char   date[80], attrib[MAX_N_ATTR][NAME_LENGTH], text[TEXT_SIZE],
       orig_tag[80], reply_tag[80], att[MAX_ATTACHMENTS][256], encoding[80],
       slist[MAX_N_ATTR+10][NAME_LENGTH], svalue[MAX_N_ATTR+10][NAME_LENGTH];
time_t now;

  n_attr = scan_attributes(logbook);

  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    att[i][0] = 0;

  for (i=0 ; i<n_attr ; i++)
    attrib[i][0] = 0;

  if (path)
    {
    /* get message for reply */

    strcpy(str, path);
    size = sizeof(text);
    el_retrieve(str, date, attr_list, attrib, n_attr,
                text, &size, orig_tag, reply_tag, att, encoding);
    }
  else
    {
    for (i=0 ; i<n_attr ; i++)
      {
      sprintf(str, "p%s", attr_list[i]);
      strcpy(attrib[i], getparam(str));
      }
    }

  /* remove attributes for replies */
  if (path && !bedit)
    {
    getcfg(logbook, "Remove on reply", str);
    strbreak(str, list, MAX_N_ATTR);
    for (i=0 ; i<n_attr ; i++)
      {
      if (equal_ustring(attr_list[i], list[i]))
        attrib[i][0] = 0;
      }
    }

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
  rsprintf("<body><form method=\"POST\" action=\"/%s\" enctype=\"multipart/form-data\">\n",
            logbook_enc);

  /*---- title row ----*/

  show_standard_title(logbook, "", 0);

  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Submit"));
  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Back"));
  rsprintf("</td></tr></table></td></tr>\n\n");

  /*---- entry form ----*/

  /* overall table for message giving blue frame */
  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Categories border"), gt("Categories cellpadding"), gt("Frame color"));

  /* print required message if one of the attributes has it set */
  for (i= 0 ; i < n_attr ; i++)
    {
    if (attr_flags[i] & AF_REQUIRED)
      {
      rsprintf("<tr><td colspan=2 bgcolor=%s>%s <font color=red>*</font> %s</td></tr>\n",
               gt("Categories bgcolor1"), loc("Fields marked with"), loc("are required"));
      break;
      }
    }

  time(&now);
  if (bedit)
    {
    if (getcfg(logbook, "Date format", format))
      {
      struct tm ts;

      memset(&ts, 0, sizeof(ts));

      for (i=0 ; i<12 ; i++)
        if (strncmp(date+4, mname[i], 3) == 0)
          break;
      ts.tm_mon = i;

      ts.tm_mday = atoi(date+8);
      ts.tm_hour = atoi(date+11);
      ts.tm_min  = atoi(date+14);
      ts.tm_sec  = atoi(date+17);
      ts.tm_year = atoi(date+20)-1900;

      mktime(&ts);
      strftime(str, sizeof(str), format, &ts);
      }
    else
      strcpy(str, date);

    rsprintf("<tr><td nowrap bgcolor=%s width=10%%><b>%s:</b></td><td bgcolor=%s>%s</td></tr>\n\n",
             gt("Categories bgcolor1"), loc("Entry date"), gt("Categories bgcolor2"), str);
    }
  else
    {
    if (getcfg(logbook, "Date format", format))
      strftime(str, sizeof(str), format, localtime(&now));
    else
      strcpy(str, ctime(&now));
    
    rsprintf("<tr><td nowrap bgcolor=%s width=10%%><b>%s:</b></td><td bgcolor=%s>%s</td></tr>\n\n",
             gt("Categories bgcolor1"), loc("Entry date"), gt("Categories bgcolor2"), str);
    }

  /* display attributes */
  for (index = 0 ; index < n_attr ; index++)
    {
    strcpy(star, (attr_flags[index] & AF_REQUIRED) ? "<font color=red>*</font>" : "");

    /* check for preset string */
    sprintf(str, "Preset %s", attr_list[index]);
    if (getcfg(logbook, str, preset))
      {
      if (!bedit || (attr_flags[index] & AF_FIXED) == 0)
        {
        i = build_subst_list(slist, svalue, NULL);
        strsubst(preset, slist, svalue, i);

        /* check for index substitution */
        if (!bedit && strchr(preset, '%'))
          {
          /* get index */
          i = get_last_index(index);
          
          strcpy(str, preset);
          sprintf(preset, str, i+1);
          }

        if (!strchr(preset, '%'))
          strcpy(attrib[index], preset);
        }
      }

    if (attr_options[index][0][0] == 0)
      {
      /* display text box */
      rsprintf("<tr><td nowrap bgcolor=%s><b>%s%s:</b></td>", gt("Categories bgcolor1"), attr_list[index], star);

      if (attr_flags[index] & AF_LOCKED)
        {
        rsprintf("<td bgcolor=%s>%s\n", gt("Categories bgcolor2"), attrib[index]);
        rsprintf("<input type=\"hidden\" name=\"%s\" value=\"%s\"></td></tr>\n",
                  attr_list[index], attrib[index]);
        }
      else
        rsprintf("<td bgcolor=%s><input type=\"text\" size=80 maxlength=%d name=\"%s\" value=\"%s\"></td></tr>\n",
                  gt("Categories bgcolor2"), NAME_LENGTH, attr_list[index], attrib[index]);
      }
    else
      {
      if (equal_ustring(attr_options[index][0], "boolean"))
        {
        /* display checkbox */
        rsprintf("<tr><td nowrap bgcolor=%s><b>%s%s:</b></td><td bgcolor=%s><input type=checkbox name=\"%s\" value=1>\n",
                 gt("Categories bgcolor1"), attr_list[index], star, gt("Categories bgcolor2"), attr_list[index]);
        }
      else
        {
        if (attr_flags[index] & AF_MULTI)
          {
          /* display multiple check boxes */
          rsprintf("<tr><td nowrap bgcolor=%s><b>%s%s:</b></td><td bgcolor=%s>\n",
                   gt("Categories bgcolor1"), attr_list[index], star, gt("Categories bgcolor2"));

          for (i=0 ; i<MAX_N_LIST && attr_options[index][i][0] ; i++)
            {
            sprintf(str, "%s%d", attr_list[index], i);

            if (strstr(attrib[index], attr_options[index][i]))
              rsprintf("<input type=checkbox checked name=\"%s\" value=\"%s\">%s&nbsp;\n", str, attr_options[index][i], attr_options[index][i]);
            else
              rsprintf("<input type=checkbox name=\"%s\" value=\"%s\">%s&nbsp;\n", str, attr_options[index][i], attr_options[index][i]);
            }

          rsprintf("</td></tr>\n");
          }
        else
          {
          /* display drop-down box */
          rsprintf("<tr><td nowrap bgcolor=%s><b>%s%s:</b></td><td bgcolor=%s><select name=\"%s\">\n",
                   gt("Categories bgcolor1"), attr_list[index], star, gt("Categories bgcolor2"), attr_list[index]);

          /* display emtpy option */
          rsprintf("<option value=\"\">- %s -\n", loc("please select"));

          for (i=0 ; i<MAX_N_LIST && attr_options[index][i][0] ; i++)
            {
            if (equal_ustring(attr_options[index][i], attrib[index]))
              rsprintf("<option selected value=\"%s\">%s\n", attr_options[index][i], attr_options[index][i]);
            else
              rsprintf("<option value=\"%s\">%s\n", attr_options[index][i], attr_options[index][i]);
            }

          rsprintf("</select></td></tr>\n");
          }
        }
      }
    }

  if (path)
    {
    /* hidden text for original message */
    rsprintf("<input type=hidden name=orig value=\"%s\">\n", path);

    if (bedit)
      rsprintf("<input type=hidden name=edit value=1>\n");
    }

  rsprintf("</td></tr>\n");

  /* increased wrapping for replies (leave space for '> ' */
  wrap = (path && !bedit) ? 78 : 76;

  rsprintf("<tr><td colspan=2 bgcolor=#FFFFFF>\n");

  if (getcfg(logbook, "Message comment", comment) && !bedit && !path)
    {
    rsputs(comment);
    rsputs("<br>\n");
    }

  if (!getcfg(logbook, "Show text", str) || atoi(str) == 1)
    {
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

    if (!path && getcfg(logbook, "Preset text", str))
      {
      /* check if file */
      strcpy(file_name, cfg_dir);
      strcat(file_name, str);

      fh = open(file_name, O_RDONLY | O_BINARY);
      if (fh > 0)
        {
        length = lseek(fh, 0, SEEK_END);
        lseek(fh, 0, SEEK_SET);
        buffer = malloc(length+1);
        read(fh, buffer, length);
        buffer[length] = 0;
        close(fh);
        rsputs(buffer);
        free(buffer);
        }
      else
        rsputs(str);
      }

    rsprintf("</textarea><br>\n");

    /* HTML check box */
    if (bedit)
      {
      if (encoding[0] == 'H')
        rsprintf("<input type=checkbox checked name=html value=1>%s\n", loc("Submit as HTML text"));
      else
        rsprintf("<input type=checkbox name=html value=1>%s\n", loc("Submit as HTML text"));
      }
    else
      {
      if (getcfg(logbook, "HTML default", str))
        {
        if (atoi(str) == 0)
          {
          rsprintf("<input type=checkbox name=html value=1>%s\n", loc("Submit as HTML text"));
          }
        else if (atoi(str) == 1)
          {
          rsprintf("<input type=checkbox checked name=html value=1>%s\n", loc("Submit as HTML text"));
          }
        }
      else
        rsprintf("<input type=checkbox name=html value=1>%s\n", loc("Submit as HTML text"));
      }
    }

  /* Suppress check box */
  if (getcfg(logbook, "Suppress default", str))
    {
    if (atoi(str) == 0)
      {
      rsprintf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");
      rsprintf("<input type=checkbox name=suppress value=1>%s>\n", loc("Suppress Email notification"));
      }
    else if (atoi(str) == 1)
      {
      rsprintf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");
      rsprintf("<input type=checkbox checked name=suppress value=1>%s\n", loc("Suppress Email notification"));
      }
    }
  else
    {
    rsprintf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");
    rsprintf("<input type=checkbox name=suppress value=1>%s\n", loc("Suppress Email notification"));
    }

  if (bedit)
    {
    /* Resubmit check box */
    if (getcfg(logbook, "Resubmit default", str))
      {
      if (atoi(str) == 0)
        {
        rsprintf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");
        rsprintf("<input type=checkbox name=resubmit value=1>%s\n", loc("Resubmit as new entry"));
        }
      else if (atoi(str) == 1)
        {
        rsprintf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");
        rsprintf("<input type=checkbox checked name=resubmit value=1>%s\n", loc("Resubmit as new entry"));
        }
      }
    else
      {
      rsprintf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");
      rsprintf("<input type=checkbox name=resubmit value=1>%s\n", loc("Resubmit as new entry"));
      }
    }

  rsprintf("</tr>\n");

  if (getcfg(logbook, "Number attachments", str))
    n = atoi(str);
  else
    n = 5;

  if (n > MAX_ATTACHMENTS)
    n = MAX_ATTACHMENTS;

  if (n > 0)
    {
    if (bedit)
      {
      rsprintf("<tr><td colspan=2 align=left bgcolor=%s>%s.<br>\n",
                gt("Categories bgcolor1"), loc("If no attachments are resubmitted, the original ones are kept"));
      rsprintf("%s.</td></tr>\n", loc("To delete an old attachment, enter <code>&lt;delete&gt;</code> in the new attachment field"));

      for (i=0 ; i<n ; i++)
        {
        if (att[i][0])
          {
          rsprintf("<tr><td align=right nowrap bgcolor=%s>%s %d:<br>%s %d:</td>",
                    gt("Categories bgcolor1"), loc("Original attachment"), i+1, loc("New attachment"), i+1);

          rsprintf("<td bgcolor=%s>%s<br>", gt("Categories bgcolor2"), att[i]+14);
          rsprintf("<input type=\"file\" size=\"60\" maxlength=\"200\" name=\"attfile%d\" accept=\"filetype/*\"></td></tr>\n",
                    i+1);
          }
        else
          rsprintf("<tr><td bgcolor=%s>%s %d:</td><td bgcolor=%s><input type=\"file\" size=\"60\" maxlength=\"200\" name=\"attfile%d\" accept=\"filetype/*\"></td></tr>\n",
                    gt("Categories bgcolor1"), loc("Attachment"), i+1, gt("Categories bgcolor2"), i+1);
        }

      }
    else
      {
      /* attachment */
      for (i=0 ; i<n ; i++)
        rsprintf("<tr><td nowrap bgcolor=%s><b>%s %d:</b></td><td bgcolor=%s><input type=\"file\" size=\"60\" maxlength=\"200\" name=\"attfile%d\" accept=\"filetype/*\"></td></tr>\n",
                 gt("Categories bgcolor1"), loc("Attachment"), i+1, gt("Categories bgcolor2"), i+1);
      }
    }

  rsprintf("</td></tr></table>\n");

  /*---- menu buttons again ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Submit"));
  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Back"));
  rsprintf("</td></tr></table></td></tr>\n\n");

  rsprintf("</td></tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_find()
{
int    i, j, n_attr;
char   str[256];

  n_attr = scan_attributes(logbook);

  /*---- header ----*/

  show_standard_header(loc("ELOG find"), NULL);

  /*---- title ----*/

  show_standard_title(logbook, "", 0);

  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Search"));
  rsprintf("<input type=reset value=\"%s\">\n", loc("Reset Form"));
  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Back"));
  rsprintf("</td></tr></table></td></tr>\n\n");

  /*---- entry form ----*/

  /* overall table for message giving blue frame */
  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Categories border"), gt("Categories cellpadding"), gt("Frame color"));

  if (!getcfg(logbook, "Show text", str) || atoi(str) == 1)
    {
    rsprintf("<tr><td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));

    if (getcfg(logbook, "Summary on default", str) && atoi(str) == 1)
      rsprintf("<input type=checkbox checked name=mode value=\"summary\">%s\n", loc("Summary only"));
    else
      rsprintf("<input type=checkbox name=mode value=\"summary\">%s\n", loc("Summary only"));
    rsprintf("</td></tr>\n");
    }
  rsprintf("<td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));
  rsprintf("<input type=checkbox name=attach value=1>%s</td></tr>\n", loc("Show attachments"));

  rsprintf("<td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));
  rsprintf("<input type=checkbox name=printable value=1>%s</td></tr>\n", loc("Printable output"));

  rsprintf("<td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));
  rsprintf("<input type=checkbox name=reverse value=1>%s</td></tr>\n", loc("Sort in reverse order"));

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
    rsprintf("<input type=checkbox name=all value=1>%s</td></tr>\n", loc("Search all logbooks"));
    }

  rsprintf("<tr><td nowrap width=10%% bgcolor=%s>%s:</td>", gt("Categories bgcolor1"), loc("Start date"));
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

  rsprintf("&nbsp;%s: <input type=\"text\" size=5 maxlength=5 name=\"y1\">", loc("Year"));
  rsprintf("</td></tr>\n");

  rsprintf("<tr><td bgcolor=%s>%s:</td>", gt("Categories bgcolor1"), loc("End date"));
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

  rsprintf("&nbsp;%s: <input type=\"text\" size=5 maxlength=5 name=\"y2\">", loc("Year"));
  rsprintf("</td></tr>\n");

  rsprintf("</td></tr>\n");

  for (i=0 ; i<n_attr ; i++)
    {
    rsprintf("<tr><td nowrap bgcolor=%s>%s:</td>", gt("Categories bgcolor1"), attr_list[i]);
    rsprintf("<td bgcolor=%s>", gt("Categories bgcolor2"));
    if (attr_options[i][0][0] == 0)
      {
      rsprintf("<input type=\"text\" size=\"30\" maxlength=\"80\" name=\"%s\">\n", attr_list[i]);
      rsprintf("<i>%s</i>\n", loc("(case insensitive substring)"));
      }
    else
      {
      if (equal_ustring(attr_options[i][0], "boolean"))
        rsprintf("<input type=checkbox name=\"%s\" value=1>\n", attr_list[i]);
      else
        {
        rsprintf("<select name=\"%s\">\n", attr_list[i]);
        rsprintf("<option value=\"\">\n");
        for (j=0 ; j<MAX_N_LIST && attr_options[i][j][0] ; j++)
          rsprintf("<option value=\"%s\">%s\n", attr_options[i][j], attr_options[i][j]);
        rsprintf("</select>\n");
        }
      }
    rsprintf("</td></tr>\n");
    }

  if (!getcfg(logbook, "Show text", str) || atoi(str) == 1)
    {
    rsprintf("<tr><td bgcolor=%s>%s:</td>",  gt("Categories bgcolor1"), loc("Text"));
    rsprintf("<td bgcolor=%s><input type=\"text\" size=\"30\" maxlength=\"80\" name=\"subtext\">\n",
               gt("Categories bgcolor2"));
    rsprintf("<i>%s</i></td></tr>\n", loc("(case insensitive substring)"));
    }

  rsprintf("</table></td></tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_config_page()
{
int  fh, length;
char *buffer;

  /*---- header ----*/

  rsprintf("HTTP/1.1 200 Document follows\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>ELOG config</title></head>\n");
  rsprintf("<body><form method=\"POST\" action=\"/%s\" enctype=\"multipart/form-data\">\n",
            logbook_enc);

  /*---- title ----*/

  show_standard_title(logbook, "", 0);

  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Save"));
  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Cancel"));
  rsprintf("</td></tr></table></td></tr>\n\n");

  /*---- entry form ----*/

  /* overall table for message giving blue frame */
  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Categories border"), gt("Categories cellpadding"), gt("Frame color"));

  rsprintf("<tr><td colspan=2 bgcolor=%s>", gt("Categories bgcolor2"));

  fh = open(cfg_file, O_RDONLY | O_BINARY);
  if (fh < 0)
    {
    rsprintf("Cannot read configuration file <b>\"%s\"</b>", cfg_file);
    rsprintf("</table></td></tr></table>\n");
    rsprintf("</body></html>\r\n");
    return;
    }
  length = lseek(fh, 0, SEEK_END);
  lseek(fh, 0, SEEK_SET);
  buffer = malloc(length+1);
  if (buffer == NULL)
    {
    close(fh);
    rsprintf("Error: out of memory<p>");
    rsprintf("</table></td></tr></table>\n");
    rsprintf("</body></html>\r\n");
    return;
    }
  read(fh, buffer, length);
  buffer[length] = 0;
  close(fh);

  rsprintf("<textarea rows=40 cols=80 wrap=virtual name=Text>");

  rsputs(buffer);
  free(buffer);

  rsprintf("</textarea><br>\n");

  rsprintf("</table></td></tr>\n");

  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Save"));
  rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Cancel"));
  rsprintf("</td></tr></table>\n\n");

  rsprintf("</td></tr></table>\n\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

int save_config()
{
int  fh, i;
char str[80];

  fh = open(cfg_file, O_RDWR | O_BINARY | O_TRUNC, 644);
  if (fh < 0)
    {
    sprintf(str, "%s: %s", loc("Cannot open file <b>%s</b>"), cfg_file, strerror(errno));
    show_error(str);
    return 0;
    }
  i = write(fh, _text, strlen(_text));
  if (i < (int)strlen(_text))
    {
    sprintf(str, "%s: %s", loc("Cannot write to <b>%s</b>"), cfg_file, strerror(errno));
    show_error(str);
    close(fh);
    return 0;
    }

  close(fh);
  return 1;
}

/*------------------------------------------------------------------*/

void show_elog_delete(char *path)
{
int    status;
char   str[256];

  /* redirect if confirm = NO */
  if (getparam("confirm") && *getparam("confirm") &&
      strcmp(getparam("confirm"), loc("No")) == 0)
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

  if (getparam("confirm") && *getparam("confirm"))
    {
    if (strcmp(getparam("confirm"), loc("Yes")) == 0)
      {
      /* delete message */
      status = el_delete_message(path);
      rsprintf("<tr><td bgcolor=#80FF80 align=center>");
      if (status == EL_SUCCESS)
        rsprintf("<b>%s</b></tr>\n", loc("Message successfully deleted"));
      else
        rsprintf("<b>%s = %d</b></tr>\n", loc("Error deleting message: status"), status);

      rsprintf("<input type=hidden name=cmd value=\"%s\">\n", loc("Last"));
      rsprintf("<tr><td bgcolor=%s align=center><input type=submit value=\"%s\"></td></tr>\n",
                gt("Cell BGColor"), loc("Goto last message"));
      }
    }
  else
    {
    /* define hidden field for command */
    rsprintf("<input type=hidden name=cmd value=\"%s\">\n", loc("Delete"));

    rsprintf("<tr><td bgcolor=%s align=center>", gt("Title bgcolor"));
    rsprintf("<font color=%s><b>%s</b></font></td></tr>\n", gt("Title fontcolor"), 
            loc("Are you sure to delete this message?"));

    rsprintf("<tr><td bgcolor=%s align=center>%s</td></tr>\n", gt("Cell BGColor"), path);

    rsprintf("<tr><td bgcolor=%s align=center><input type=submit name=confirm value=\"%s\">\n", 
              gt("Cell BGColor"), loc("Yes"));
    rsprintf("<input type=submit name=confirm value=\"%s\">\n", loc("No"));
    rsprintf("</td></tr>\n\n");
    }

  rsprintf("</table></td></tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_submit_find(INT past_n, INT last_n)
{
int    i, j, n, size, status, d1, m1, y1, d2, m2, y2, index, colspan, i_line, n_line, i_col;
int    current_year, current_month, current_day, printable, n_logbook, lindex, n_attr,
       reverse, n_attr_disp, n_found;
char   date[80], attrib[MAX_N_ATTR][NAME_LENGTH], disp_attr[MAX_N_ATTR][NAME_LENGTH],
       list[10000], text[TEXT_SIZE], text1[TEXT_SIZE], text2[TEXT_SIZE],
       orig_tag[80], reply_tag[80], attachment[MAX_ATTACHMENTS][256], encoding[80];
char   str[256], tag[256], ref[256], file_name[256], col[80], old_data_dir[256];
char   logbook_list[100][256], lb_enc[256], *nowrap, format[80];
char   menu_str[1000], menu_item[MAX_N_LIST][NAME_LENGTH];
char   *p , *pt, *pt1, *pt2;
BOOL   full, show_attachments;
time_t ltime, ltime_start, ltime_end, ltime_current, now;
struct tm tms, *ptms;
FILE   *f;

  n_attr = scan_attributes(logbook);

  printable = atoi(getparam("Printable"));

  if (*getparam("Reverse"))
    reverse = atoi(getparam("Reverse"));
  else
    {
    reverse = 0;
    if (getcfg(logbook, "Reverse sort", str))
      reverse = atoi(str);
    }

  /*---- header ----*/

  show_standard_header(loc("ELOG search result"), NULL);

  /*---- title ----*/

  strcpy(str, ", ");
  if (past_n == 1)
    strcat(str, loc("Last day"));
  else if (past_n > 1)
    sprintf(str+strlen(str), loc("Last %d days"), past_n);
  else if (last_n)
    sprintf(str+strlen(str), loc("Last %d entries"), last_n);

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

    if (getcfg(logbook, "Find menu commands", menu_str))
      {
      n = strbreak(menu_str, menu_item, MAX_N_LIST);

      if (atoi(gt("Use buttons")) == 1)
        {
        for (i=0 ; i<n ; i++)
          {
          if (equal_ustring(menu_item[i], "Last x"))
            {
            if (past_n)
              {
              sprintf(str, loc("Last %d days"), past_n*2);
              rsprintf("<input type=submit name=past value=\"%s\">\n", str);
              }

            if (last_n)
              {
              sprintf(str, loc("Last %d entries"), last_n*2);
              rsprintf("<input type=submit name=last value=\"%s\">\n", str);
              }
            }
          else
            rsprintf("<input type=submit name=cmd value=\"%s\">\n", menu_item[i]);
          }
        }
      else
        {
        rsprintf("<small>\n");

        for (i=0 ; i<n ; i++)
          {
          if (equal_ustring(menu_item[i], "Last x"))
            {
            if (past_n)
              {
              sprintf(str, loc("Last %d days"), past_n*2);
              rsprintf("&nbsp;<a href=\"/%s/past%d\">%s</a>&nbsp;|\n", logbook_enc, past_n*2, str);
              }

            if (last_n)
              {
              sprintf(str, loc("Last %d entries"), last_n*2);
              rsprintf("&nbsp;<a href=\"/%s/last%d\">%s</a>&nbsp;|\n", logbook_enc, last_n*2, str);
              }
            }
          else
            {
            strcpy(str, menu_item[i]);
            url_encode(str);

            if (i < n-1)
              rsprintf("&nbsp;<a href=\"/%s?cmd=%s\">%s</a>&nbsp;|\n", logbook_enc, str, menu_item[i]);
            else
              rsprintf("&nbsp;<a href=\"/%s?cmd=%s\">%s</a>&nbsp;\n", logbook_enc, str, menu_item[i]);
            }
          }

        rsprintf("</small>\n");
        }
      }
    else
      {
      if (atoi(gt("Use buttons")) == 1)
        {
        if (past_n)
          {
          sprintf(str, loc("Last %d days"), past_n*2);
          rsprintf("<input type=submit name=past value=\"%s\">\n", str);
          }

        if (last_n)
          {
          sprintf(str, loc("Last %d entries"), last_n*2);
          rsprintf("<input type=submit name=last value=\"%s\">\n", str);
          }

        rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Find"));
        rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Back"));
        }
      else
        {
        rsprintf("<small>\n");

        if (past_n)
          {
          sprintf(str, loc("Last %d days"), past_n*2);
          rsprintf("&nbsp;<a href=\"/%s/past%d\">%s</a>&nbsp;|\n", logbook_enc, past_n*2, str);
          }

        if (last_n)
          {
          sprintf(str, loc("Last %d entries"), last_n*2);
          rsprintf("&nbsp;<a href=\"/%s/last%d\">%s</a>&nbsp;|\n", logbook_enc, last_n*2, str);
          }

        rsprintf("&nbsp;<a href=\"/%s?cmd=%s\">%s</a>&nbsp;|\n", logbook_enc, loc("Find"), loc("Find"));
        rsprintf("&nbsp;<a href=\"/%s?cmd=%s\">%s</a>&nbsp;\n", logbook_enc, loc("Back"), loc("Back"));

        rsprintf("</small>\n");
        }

      }

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
      tms.tm_hour = 0;

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
      tms.tm_hour = 0;
      tms.tm_min  = 0;
      tms.tm_sec  = 0;

      if (tms.tm_year < 90)
        tms.tm_year += 100;
      ltime_end = mktime(&tms);

      /* end time is first second of next day */
      ltime_end += 3600*24;

      memcpy(&tms, localtime(&ltime_end), sizeof(struct tm));
      y2 = tms.tm_year + 1900;
      m2 = tms.tm_mon +1;
      d2 = tms.tm_mday;
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
    rsprintf("<tr><td nowrap width=10%% bgcolor=%s><b>%s:</b></td><td bgcolor=%s>%s %d, %d</td></tr>",
              gt("Categories bgcolor1"), loc("Start date"), gt("Categories bgcolor2"), mname[m1-1], d1, y1);

  if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
    {
    /* calculate previous day */
    memset(&tms, 0, sizeof(struct tm));
    tms.tm_year = y2 % 100;
    tms.tm_mon  = m2-1;
    tms.tm_mday = d2;
    tms.tm_hour = 12;

    if (tms.tm_year < 90)
      tms.tm_year += 100;
    ltime = mktime(&tms);
    ltime -= 3600*24;
    memcpy(&tms, localtime(&ltime), sizeof(struct tm));

    rsprintf("<tr><td nowrap width=10%% bgcolor=%s><b>%s:</b></td><td bgcolor=%s>%s %d, %d</td></tr>",
              gt("Categories bgcolor1"), loc("End date"), gt("Categories bgcolor2"),
              mname[tms.tm_mon], tms.tm_mday, tms.tm_year + 1900);
    }

  for (i=0 ; i<n_attr ; i++)
    {
    if (*getparam(attr_list[i]))
      rsprintf("<tr><td nowrap width=10%% bgcolor=%s><b>%s:</b></td><td bgcolor=%s>%s</td></tr>",
                gt("Categories bgcolor1"), attr_list[i], gt("Categories bgcolor2"), getparam(attr_list[i]));
    }

  if (*getparam("subtext"))
    {
    rsprintf("<tr><td nowrap width=10%% bgcolor=%s><b>%s:</b></td>", gt("Categories bgcolor1"), loc("Text"));
    rsprintf("<td bgcolor=%s><B style=\"color:black;background-color:#ffff66\">%s</B></td></tr>",
              gt("Categories bgcolor2"), getparam("subtext"));
    }

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

  rsprintf("<td width=10%% align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica,sans-serif><b>#</b></td>", col, size);

  if (atoi(getparam("all")) == 1)
    rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica,sans-serif><b>Logbook</b></td>", col, size);

  rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica,sans-serif><b>Date</b></td>", col, size);

  if (getcfg(logbook, "Display search", list))
    n_attr_disp = strbreak(list, disp_attr, MAX_N_ATTR);
  else
    {
    n_attr_disp = n_attr;
    memcpy(disp_attr, attr_list, sizeof(attr_list));
    }

  for (i=0 ; i<n_attr ; i++)
    {
    for (j=0 ; j<n_attr_disp ; j++)
      if (equal_ustring(disp_attr[j], attr_list[i]))
        break;

    if (j == n_attr_disp)
      continue;

    rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica,sans-serif><b>%s</b></td>",
              col, size, attr_list[i]);
    }

  if (!full && n_line > 0)
    rsprintf("<td align=center bgcolor=%s><font size=%d face=verdana,arial,helvetica,sans-serif><b>Text</b></td>",
             col, size);

  rsprintf("</tr>\n\n");

  /*---- do find ----*/

  i_col = 0;
  n_found = 0;

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
      getcfg(logbook_list[lindex], "Data dir", str);
      if (str[0] == DIR_SEPARATOR || str[1] == ':')
        strcpy(data_dir, str);
      else
        {
        strcpy(data_dir, cfg_dir);
        strcat(data_dir, str);
        }
      
      if (data_dir[strlen(data_dir)-1] != DIR_SEPARATOR)
        strcat(data_dir, DIR_SEPARATOR_STR);
      }

    if (past_n)
      {
      if (reverse)
        {
        strcpy(tag, "-1");
        el_search_message(tag, NULL, TRUE, FALSE);

        time(&now);
        ltime_start = now-3600*24*past_n;
        }
      else
        {
        time(&now);
        ltime_start = now-3600*24*past_n;
        ptms = localtime(&ltime_start);

        sprintf(tag, "%02d%02d%02d.0", ptms->tm_year % 100, ptms->tm_mon+1, ptms->tm_mday);
        }
      }
    else if (last_n)
      {
      strcpy(tag, "-1");
      el_search_message(tag, NULL, TRUE, FALSE);

      if (!reverse)
        for (i=1 ; i<last_n ; i++)
          {
          strcat(tag, "-1");
          el_search_message(tag, NULL, TRUE, FALSE);
          }
      }
    else
      {
      /* do date-date find */

      if (reverse)
        {
        if (!*getparam("y2") && !*getparam("m2") && !*getparam("d2"))
          {
          strcpy(tag, "-1");

          /* search last message */
          el_search_message(tag, NULL, TRUE, FALSE);
          }
        else
          {
          /* if y, m or d given, assemble start date */
          sprintf(tag, "%02d%02d%02d.0-1", y2 % 100, m2, d2);
          }
        }
      else
        {
        if (!*getparam("y1") && !*getparam("m1") && !*getparam("d1"))
          {
          tag[0] = 0;

          /* search first message */
          el_search_message(tag, NULL, TRUE, TRUE);
          }
        else
          {
          /* if y, m or d given, assemble start date */
          sprintf(tag, "%02d%02d%02d.0", y1 % 100, m1, d1);
          }
        }
      }

    do
      {
      size = sizeof(text);
      status = el_retrieve(tag, date, attr_list, attrib, n_attr,
                           text, &size, orig_tag, reply_tag,
                           attachment,
                           encoding);

      if (reverse)
        strcat(tag, "-1");
      else
        strcat(tag, "+1");

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

      if (reverse)
        {
        /* check for start date */
        if (ltime_start > 0)
          if (ltime_current < ltime_start)
            break;

        /* check for end date */
        if (ltime_end > 0)
          {
          if (ltime_current > ltime_end)
            continue;
          }
        }
      else
        {
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
        }

      if (status == EL_SUCCESS)
        {
        /* do filtering */
        for (i=0 ; i<n_attr ; i++)
          {
          if (*getparam(attr_list[i]))
            {
            strcpy(str, getparam(attr_list[i]));
            for (j=0 ; j<(int)strlen(str) ; j++)
              str[j] = toupper(str[j]);
            str[j] = 0;
            for (j=0 ; j<(int)strlen(attrib[i]) ; j++)
              text1[j] = toupper(attrib[i][j]);
            text1[j] = 0;

            if (strstr(text1, str) == NULL)
              break;
            }
          }
        if (i < n_attr)
          continue;

        if (*getparam("subtext"))
          {
          strcpy(str, getparam("subtext"));
          for (i=0 ; i<(int)strlen(str) ; i++)
            str[i] = toupper(str[i]);
          str[i] = 0;
          for (i=0 ; i<(int)strlen(text) ; i++)
            text1[i] = toupper(text[i]);
          text1[i] = 0;

          if (strstr(text1, str) == NULL)
            continue;

          text2[0] = 0;
          pt = text;   /* original text */
          pt1 = text1; /* upper-case text */
          pt2 = text2; /* text with inserted coloring */
          do
            {
            p = strstr(pt1, str);
            size = (int)(p - pt1);
            if (p != NULL)
              {
              pt1 = p+strlen(str);

              /* copy first part original text */
              memcpy(pt2, pt, size);
              pt2 += size;
              pt += size;

              /* add coloring 1st part */
              if (equal_ustring(encoding, "plain"))
                strcpy(pt2, "\001B style=\"color:black;background-color:#ffff66\"\002");
              else
                strcpy(pt2, "<B style=\"color:black;background-color:#ffff66\">");

              pt2 += strlen(pt2);

              /* copy origial search text */
              memcpy(pt2, pt, strlen(str));
              pt2 += strlen(str);
              pt += strlen(str);

              /* add coloring 2nd part */
              if (equal_ustring(encoding, "plain"))
                strcpy(pt2, "\001/B\002");
              else
                strcpy(pt2, "</B>");
              pt2 += strlen(pt2);
              }
            } while (p != NULL);

            strcpy(pt2, pt);
            strcpy(text, text2);
          }

        /* filter passed: display line */

        n_found++;

        strcpy(str, tag);
        if (strchr(str, '+'))
          *strchr(str, '+') = 0;
        if (strchr(str, '-'))
          *strchr(str, '-') = 0;

        strcpy(lb_enc, logbook_list[lindex]);
        url_encode(lb_enc);
        sprintf(ref, "/%s/%s", lb_enc, str);

        if (full)
          {
          strcpy(col, gt("List bgcolor1"));
          rsprintf("<tr>");

          size = printable ? 2 : 3;
          nowrap = printable ? "" : "nowrap";

          rsprintf("<td align=center bgcolor=%s><font size=%d><a href=\"%s\">&nbsp;&nbsp;%d&nbsp;&nbsp;</a></font></td>", col, size, ref, n_found);

          if (atoi(getparam("all")) == 1)
            rsprintf("<td align=center %s bgcolor=%s><font size=%d>%s</font></td>", nowrap, col, size, logbook_list[lindex]);

          if (getcfg(logbook, "Date format", format))
            {
            struct tm ts;

            memset(&ts, 0, sizeof(ts));

            for (i=0 ; i<12 ; i++)
              if (strncmp(date+4, mname[i], 3) == 0)
                break;
            ts.tm_mon = i;

            ts.tm_mday = atoi(date+8);
            ts.tm_hour = atoi(date+11);
            ts.tm_min  = atoi(date+14);
            ts.tm_sec  = atoi(date+17);
            ts.tm_year = atoi(date+20)-1900;

            mktime(&ts);
            strftime(str, sizeof(str), format, &ts);
            }
          else
            strcpy(str, date);

          rsprintf("<td align=center %s bgcolor=%s><font size=%d>%s</font></td>", nowrap, col, size, str);

          for (i=0 ; i<n_attr ; i++)
            {
            for (j=0 ; j<n_attr_disp ; j++)
              if (equal_ustring(disp_attr[j], attr_list[i]))
                break;

            if (j == n_attr_disp)
              continue;

            if (equal_ustring(attr_options[i][0], "boolean"))
              {
              if (atoi(attrib[i]) == 1)
                rsprintf("<td align=center bgcolor=%s><input type=checkbox checked disabled></td>\n", col);
              else
                rsprintf("<td align=center bgcolor=%s><input type=checkbox disabled></td>\n", col);
              }
            else
              {
              rsprintf("<td align=center bgcolor=%s><font size=%d>", col, size);
              rsputs2(attrib[i]);
              rsprintf("&nbsp</font></td>");
              }
            }

          rsprintf("</tr>\n");

          if (atoi(getparam("all")) == 1)
            colspan = 3+n_attr_disp;
          else
            colspan = 2+n_attr_disp;

          if (!getcfg(logbook, "Show text", str) || atoi(str) == 1)
            {
            rsprintf("<tr><td bgcolor=#FFFFFF colspan=%d><font size=%d>", colspan, size);

            if (equal_ustring(encoding, "plain"))
              {
              rsputs("<pre>");
              rsputs2(text);
              rsputs("</pre>");
              }
            else
              rsputs(text);

            rsprintf("</font></td></tr>\n");
            }

          if (!show_attachments && attachment[0][0])
            {
            rsprintf("<tr><td bgcolor=#FFFFFF colspan=%d><font size=%d>", colspan, size);

            if (attachment[1][0])
              rsprintf("%s: ", loc("Attachments"));
            else
              rsprintf("%s: ", loc("Attachment"));
            }

          for (index = 0 ; index < MAX_ATTACHMENTS ; index++)
            {
            if (attachment[index][0])
              {
              strcpy(str, attachment[index]);
              str[13] = 0;
              sprintf(ref, "/%s/%s/%s", logbook_enc, str, attachment[index]+14);

              for (i=0 ; i<(int)strlen(attachment[index]) ; i++)
                str[i] = toupper(attachment[index][i]);
              str[i] = 0;

              if (!show_attachments)
                {
                rsprintf("<a href=\"%s\"><b>%s</b></a>&nbsp;&nbsp;&nbsp;&nbsp; ",
                          ref, attachment[index]+14);
                }
              else
                {
                if (strstr(str, ".GIF") ||
                    strstr(str, ".JPG") ||
                    strstr(str, ".PNG"))
                  {
                  rsprintf("<tr><td colspan=%d bgcolor=%s><b>%s %d:</b> <a href=\"%s\">%s</a>\n",
                            colspan, gt("List bgcolor2"), loc("Attachment"), index+1, ref, attachment[index]+14);
                  if (show_attachments)
                    rsprintf("</td></tr><tr><td colspan=%d bgcolor=#FFFFFF><img src=\"%s\"></td></tr>", colspan, ref);
                  }
                else
                  {
                  rsprintf("<tr><td colspan=%d bgcolor=%s><b>%s %d:</b> <a href=\"%s\">%s</a>\n",
                            colspan, gt("List bgcolor2"), loc("Attachment"), index+1, ref, attachment[index]+14);

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

          rsprintf("<td align=center bgcolor=%s><font size=%d><a href=\"%s\">&nbsp;&nbsp;%d&nbsp;&nbsp;</a></font></td>", col, size, ref, n_found);

          if (atoi(getparam("all")) == 1)
            rsprintf("<td align=center %s bgcolor=%s>%s</td>", nowrap, col, logbook_list[lindex]);

          if (getcfg(logbook, "Date format", format))
            {
            struct tm ts;

            memset(&ts, 0, sizeof(ts));

            for (i=0 ; i<12 ; i++)
              if (strncmp(date+4, mname[i], 3) == 0)
                break;
            ts.tm_mon = i;

            ts.tm_mday = atoi(date+8);
            ts.tm_hour = atoi(date+11);
            ts.tm_min  = atoi(date+14);
            ts.tm_sec  = atoi(date+17);
            ts.tm_year = atoi(date+20)-1900;

            mktime(&ts);
            strftime(str, sizeof(str), format, &ts);
            }
          else
            strcpy(str, date);

          rsprintf("<td align=center %s bgcolor=%s><font size=%d>%s</font></td>", nowrap, col, size, str);

          for (i=0 ; i<n_attr ; i++)
            {
            for (j=0 ; j<n_attr_disp ; j++)
              if (equal_ustring(disp_attr[j], attr_list[i]))
                break;

            if (j == n_attr_disp)
              continue;

            if (equal_ustring(attr_options[i][0], "boolean"))
              {
              if (atoi(attrib[i]) == 1)
                rsprintf("<td align=center bgcolor=%s><input type=checkbox checked disabled></td>\n", col);
              else
                rsprintf("<td align=center bgcolor=%s><input type=checkbox disabled></td>\n", col);
              }
            else
              rsprintf("<td align=center bgcolor=%s><font size=%d>%s&nbsp</font></td>", col, size, attrib[i]);
            }

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

      /* stop after all found (needed for reverse search) */
      if (last_n && n_found >= last_n)
        break;

      } while (status == EL_SUCCESS);
    } /* for () */

  rsprintf("</table></td></tr>\n");

  if (n_found == 0)
    {
    rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           printable ? "1" : gt("Border width"), gt("Categories cellpadding"), gt("Frame color"));

    rsprintf("<tr><td bgcolor=#FFB0B0 align=center><b>%s<b></td></tr>", loc("No entries found"));

    rsprintf("</td></tr></table>\n");
    }

  rsprintf("</table>\n");

  if (getcfg(logbook, "bottom text", str))
    {
    FILE *f;
    char file_name[256], *buf;

    strcpy(file_name, cfg_dir);
    strcat(file_name, str);

    f = fopen(file_name, "r");
    if (f != NULL)
      {
      fseek(f, 0, SEEK_END);
      size = TELL(fileno(f));
      fseek(f, 0, SEEK_SET);

      buf = malloc(size+1);
      fread(buf, 1, size, f);
      buf[size] = 0;
      fclose(f);

      rsputs(buf);
      }
    }
  else
    /* add little logo */
    rsprintf("<center><font size=1 color=#A0A0A0><a href=\"http://midas.psi.ch/elog/\">ELOG V%s</a></font></center>", VERSION);

  rsprintf("</body></html>\r\n");

  /* restor old data_dir */
  if (old_data_dir[0])
    strcpy(data_dir, old_data_dir);
}

/*------------------------------------------------------------------*/

void submit_elog()
{
char   str[256], mail_to[256], mail_from[256], file_name[256], error[1000],
       mail_text[10000], mail_list[MAX_N_LIST][NAME_LENGTH], list[10000], smtp_host[256],
       tag[80], subject[256], attrib[MAX_N_ATTR][NAME_LENGTH], subst_str[256];
char   *buffer[MAX_ATTACHMENTS], mail_param[1000];
char   att_file[MAX_ATTACHMENTS][256];
char   slist[MAX_N_ATTR+10][NAME_LENGTH], svalue[MAX_N_ATTR+10][NAME_LENGTH];
int    i, j, n, missing, first, index, n_attr, n_mail, suppress, status;

  n_attr = scan_attributes(logbook);

  /* check for required attributs */
  missing = 0;
  for (i=0 ; i<n_attr ; i++)
    if (attr_flags[i] & AF_REQUIRED)
      {
      if ((attr_flags[i] & AF_MULTI) == 0 && *getparam(attr_list[i]) == 0)
        {
        missing = 1;
        break;
        }
      if ((attr_flags[i] & AF_MULTI))
        {
        for (j=0 ; j<MAX_N_LIST ; j++)
          {
          sprintf(str, "%s%d", attr_list[i], j);
          if (getparam(str) && *getparam(str))
            break;
          }

        if (j == MAX_N_LIST)
          {
          missing = 1;
          break;
          }
        }
      }

  if (missing)
    {
    show_standard_header("ELOG error", "");

    rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>",
              gt("Border width"), gt("Frame color"));
    rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));
    rsprintf("<tr><td bgcolor=#FFB0B0 align=center>");

    rsprintf("<i>");
    rsprintf(loc("Error: Attribute <b>%s</b> not supplied"), attr_list[i]);
    rsprintf(".</i><p>\n");
    rsprintf(loc("Please go back and enter the <b>%s</b> field"), attr_list[i]);
    rsprintf(".</tr>\n");

    rsprintf("<tr><td bgcolor=%s align=center>", gt("Cell BGColor"));
    rsprintf("<button type=button onClick=history.back()>%s</button></td></tr>\n", loc("Back"));


    rsprintf("</table></td></tr></table>\n");
    rsprintf("</body></html>\n");
    return;
    }

  /* check for valid attachment files */
  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    {
    buffer[i] = NULL;
    sprintf(str, "attachment%d", i);
    strcpy(att_file[i], getparam(str));
    if (att_file[i][0] && _attachment_size[i] == 0 && !equal_ustring(att_file[i], loc("<delete>")))
      {
      sprintf(error, loc("Error: Attachment file <i>%s</i> invalid, please bo back and enter a proper file name"), getparam(str));
      show_error(error);
      return;
      }
    }

  /* compile substitution list */
  n = build_subst_list(slist, svalue, NULL);

  /* retrieve attributes */
  for (i=0 ; i<n_attr ; i++)
    {
    if (attr_flags[i] & AF_MULTI)
      {
      attrib[i][0] = 0;
      first = 1;
      for (j=0 ; j<MAX_N_LIST ; j++)
        {
        sprintf(str, "%s%d", attr_list[i], j);
        if (getparam(str))
          {
          if (*getparam(str))
            {
            if (first)
              first = 0;
            else
              strcat(attrib[i], " | ");
            if (strlen(attrib[i]) + strlen(getparam(str)) < NAME_LENGTH-2)
              strcat(attrib[i], getparam(str));
            else
              break;
            }
          }
        else
          break;
        }
      }
    else
      {
      strncpy(attrib[i], getparam(attr_list[i]), NAME_LENGTH);
      attrib[i][NAME_LENGTH-1] = 0;
      }

    if (!*getparam("edit"))
      {
      sprintf(str, "Subst %s", attr_list[i]);
      if (getcfg(logbook, str, subst_str))
        {
        strsubst(subst_str, slist, svalue, n);
        strcpy(attrib[i], subst_str);
        }
      }
    }

  if (*getparam("edit") &&
      *getparam("resubmit") &&
      atoi(getparam("resubmit")) == 1)
    {
    /* delete old message */
    el_delete_message_only(getparam("orig"), att_file);
    unsetparam("orig");

    tag[0] = 0;
    }
  else
    {
    tag[0] = 0;
    if (*getparam("edit"))
      strcpy(tag, getparam("orig"));
    }

  status = el_submit(attr_list, attrib, n_attr, getparam("text"),
                     getparam("orig"), *getparam("html") ? "HTML" : "plain",
                     att_file,
                     _attachment_buffer,
                     _attachment_size,
                     tag, sizeof(tag));

  if (status != EL_SUCCESS)
    {
    sprintf(str, loc("New message cannot be written to directory \"%s\""), data_dir);
    strcat(str, "\n<p>");
    strcat(str, loc("Please check that it exists and elogd has write access"));
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
    for (index=0 ; index <= n_attr ; index++)
      {
      if (index < n_attr)
        {
        strcpy(str, "Email ");
        if (strchr(attr_list[index], ' '))
          sprintf(str+strlen(str), "\"%s\"", attr_list[index]);
        else
          strcat(str, attr_list[index]);
        strcat(str, " ");

        if (strchr(getparam(attr_list[index]), ' '))
          sprintf(str+strlen(str), "\"%s\"", getparam(attr_list[index]));
        else
          strcat(str, getparam(attr_list[index]));
        }
      else
        sprintf(str, "Email ALL");

      if (getcfg(logbook, str, list))
        {
        n = strbreak(list, mail_list, MAX_N_LIST);

        if (verbose)
          printf("\n%s to %s\n\n", str, list);

        if (!getcfg("global", "SMTP host", smtp_host))
          if (!getcfg("global", "SMTP host", smtp_host))
            {
            show_error(loc("No SMTP host defined in [global] section of configuration file"));
            return;
            }

        for (i=0 ; i<n ; i++)
          {
          strcpy(mail_to, mail_list[i]);

          if (getcfg(logbook, "Use Email from", mail_from))
            {
            j = build_subst_list(slist, svalue, attrib);
            strsubst(mail_from, slist, svalue, j);
            }
          else
            sprintf(mail_from, "ELog@%s", host_name);

          sprintf(mail_text, loc("A new entry has been submitted on %s"), host_name);
          sprintf(mail_text+strlen(mail_text), "\r\n\r\n");

          sprintf(mail_text+strlen(mail_text), "%s             : %s\r\n", loc("Logbook"), logbook);

          for (j=0 ; j<n_attr ; j++)
            {
            strcpy(str, "                                    ");
            memcpy(str, attr_list[j], strlen(attr_list[j]));
            sprintf(str+20, ": %s\r\n", getparam(attr_list[j]));

            strcpy(mail_text+strlen(mail_text), str);
            }

          /* compose subject from attributes */
          if (getcfg(logbook, "Use Email Subject", subject))
            {
            j = build_subst_list(slist, svalue, attrib);
            strsubst(subject, slist, svalue, j);
            }
          else
            strcpy(subject, "New ELOG entry");

          sprintf(mail_text+strlen(mail_text), "\r\n%s URL         : %s%s/%s\r\n", 
                  elogd_full_url, loc("Logbook"), logbook_enc, tag);

          if (getcfg(logbook, "Email message body", str) &&
              atoi(str) == 1)
            {
            sprintf(mail_text+strlen(mail_text), "\r\n=================================\r\n\r\n%s",
              getparam("text"));
            }

          sendmail(smtp_host, mail_from, mail_to, subject, mail_text);

          if (!getcfg(logbook, "Display email recipients", str) ||
               atoi(str) == 1)
            {
            if (mail_param[0] == 0)
              strcpy(mail_param, "?");
            else
              strcat(mail_param, "&");
            sprintf(mail_param+strlen(mail_param), "mail%d=%s", n_mail++, mail_to);
            }
          }
        }
      }
    }

  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    if (buffer[i])
      free(buffer[i]);

  if (getcfg(logbook, "Submit page", str))
    {
    strcpy(file_name, cfg_dir);
    strcat(file_name, str);
    send_file(file_name);
    return;
    }

  rsprintf("HTTP/1.1 302 Found\r\n");
  rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
  if (use_keepalive)
    {
    rsprintf("Connection: Keep-Alive\r\n");
    rsprintf("Keep-Alive: timeout=60, max=10\r\n");
    }

  rsprintf("Location: /%s/%s%s\r\n\r\n<html>redir</html>\r\n",
            logbook_enc, tag, mail_param);
}

/*------------------------------------------------------------------*/

void copy_to(char *src_path, char *dst_logbook, int move)
{
int    size, i, status, fh, n_attr;
char   str[256], file_name[256], attrib[MAX_N_ATTR][NAME_LENGTH];
char   date[80], text[TEXT_SIZE],  old_data_dir[256], tag[32],
       orig_tag[80], reply_tag[80], attachment[MAX_ATTACHMENTS][256], encoding[80];

  n_attr = scan_attributes(logbook);

  /* get message */
  size = sizeof(text);
  status = el_retrieve(src_path, date, attr_list, attrib, n_attr,
                       text, &size, orig_tag, reply_tag,
                       attachment, encoding);

  if (status != EL_SUCCESS)
    {
    sprintf(str, loc("Message %s cannot be read from logbook \"%s\""), src_path, logbook);
    show_error(str);
    return;
    }

  /* read attachments */
  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    if (attachment[i][0])
      {
      strcpy(file_name, data_dir);
      strcat(file_name, attachment[i]);

      fh = open(file_name, O_RDONLY | O_BINARY);
      if (fh > 0)
        {
        lseek(fh, 0, SEEK_END);
        _attachment_size[i] = TELL(fh);
        lseek(fh, 0, SEEK_SET);

        _attachment_buffer[i] = malloc(_attachment_size[i]);

        if (_attachment_buffer[i])
          read(fh, _attachment_buffer[i], _attachment_size[i]);

        close(fh);
        }

      /* stip date/time from file name */
      strcpy(str, attachment[i]);
      strcpy(attachment[i], str+14);
      }

  /* switch to destination logbook data directory */

  strcpy(old_data_dir, data_dir);
  getcfg(dst_logbook, "Data dir", str);
  if (str[0] == DIR_SEPARATOR || str[1] == ':')
    strcpy(data_dir, str);
  else
    {
    strcpy(data_dir, cfg_dir);
    strcat(data_dir, str);
    }

  if (data_dir[strlen(data_dir)-1] != DIR_SEPARATOR)
    strcat(data_dir, DIR_SEPARATOR_STR);

  tag[0] = 0;
  status = el_submit(attr_list, attrib, n_attr, text,
                     orig_tag, encoding,
                     attachment,
                     _attachment_buffer,
                     _attachment_size,
                     tag, sizeof(tag));

  for (i=0 ; i<MAX_ATTACHMENTS ; i++)
    if (attachment[i][0])
      free(_attachment_buffer[i]);

  /* restore old data_dir */
  strcpy(data_dir, old_data_dir);

  if (status != EL_SUCCESS)
    {
    sprintf(str, loc("New message cannot be written to directory \"%s\""), data_dir);
    strcat(str, "\n<p>");
    strcat(str, loc("Please check that it exists and elogd has write access"));
    show_error(str);
    return;
    }

  /* delete original message for move */
  if (move)
    {
    el_delete_message(src_path);

    /* check if this was the last message */
    size = sizeof(text);
    status = el_retrieve(src_path, date, attr_list, attrib, n_attr,
                         text, &size, orig_tag, reply_tag,
                         attachment, encoding);

    /* if yes, force display of new last message */
    if (status != EL_SUCCESS)
      src_path[0] = 0;
    }

  /* display status message */
  show_standard_header(loc("Copy ELog entry"), src_path);

  rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>",
            gt("Border width"), gt("Frame color"));
  rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

  rsprintf("<tr><td bgcolor=#80FF80 align=center><b>");
  if (move)
    rsprintf(loc("Message moved successfully from \"%s\" to \"%s\""), logbook, dst_logbook);
  else
    rsprintf(loc("Message copied successfully from \"%s\" to \"%s\""), logbook, dst_logbook);

  rsprintf("</b></tr>\n");
  rsprintf("<tr><td bgcolor=%s align=center>%s <a href=\"/%s/%s\">%s</td></tr>\n",
            gt("Cell BGColor"), loc("Go to"), logbook, src_path, logbook);
  rsprintf("<tr><td bgcolor=%s align=center>%s <a href=\"/%s\">%s</td></tr>\n",
            gt("Cell BGColor"), loc("Go to"), dst_logbook, dst_logbook);

  rsprintf("</table></td></tr></table>\n");
  rsprintf("</body></html>\r\n");

  return;
}

/*------------------------------------------------------------------*/

void show_elog_page(char *logbook, char *path)
{
int    size, i, j, len, n, msg_status, status, fh, length, first_message, last_message, index, n_attr;
char   str[256], orig_path[256], command[80], ref[256], file_name[256], attrib[MAX_N_ATTR][NAME_LENGTH];
char   date[80], text[TEXT_SIZE], menu_str[1000], other_str[1000], cmd[256],
       orig_tag[80], reply_tag[80], attachment[MAX_ATTACHMENTS][256], encoding[80], att[256], lattr[256];
char   menu_item[MAX_N_LIST][NAME_LENGTH], format[80],
       slist[MAX_N_ATTR+10][NAME_LENGTH], svalue[MAX_N_ATTR+10][NAME_LENGTH];
FILE   *f;

  n_attr = scan_attributes(logbook);

  if (getcfg(logbook, "Types", str))
    {
    show_upgrade_page();
    return;
    }

  /*---- interprete commands ---------------------------------------*/

  strcpy(command, getparam("cmd"));

  /* correct for image buttons */
  if (*getparam("cmd_first.x"))
    strcpy(command, loc("First"));
  if (*getparam("cmd_previous.x"))
    strcpy(command, loc("Previous"));
  if (*getparam("cmd_next.x"))
    strcpy(command, loc("Next"));
  if (*getparam("cmd_last.x"))
    strcpy(command, loc("Last"));

  /* check if command allowed for current user */
  if (!allow_user(command))
    return;

  /* get menu command */
  getcfg(logbook, "Menu commands", menu_str);
  if (menu_str[0] == 0)
    {
    strcpy(menu_str, loc("New"));
    strcat(menu_str, ", ");
    strcat(menu_str, loc("Edit"));
    strcat(menu_str, ", ");
    strcat(menu_str, loc("Delete"));
    strcat(menu_str, ", ");
    strcat(menu_str, loc("Reply"));
    strcat(menu_str, ", ");
    strcat(menu_str, loc("Find"));
    strcat(menu_str, ", ");
    strcat(menu_str, loc("Last day"));
    strcat(menu_str, ", ");
    strcat(menu_str, loc("Last 10"));
    strcat(menu_str, ", ");
    strcat(menu_str, loc("Config"));
    strcat(menu_str, ", ");

    if (getcfg(logbook, "Password file", str))
      {
      strcat(menu_str, loc("Change Password"));
      strcat(menu_str, ", ");
      strcat(menu_str, loc("Logout"));
      strcat(menu_str, ", ");
      }

    strcat(menu_str, loc("Help"));
    }

  strcpy(other_str, loc("Submit"));
  strcat(other_str, " ");
  strcat(other_str, loc("Back"));
  strcat(other_str, " ");
  strcat(other_str, loc("Search"));
  strcat(other_str, " ");
  strcat(other_str, loc("Save"));
  strcat(other_str, " ");
  strcat(other_str, loc("Cancel"));
  strcat(other_str, " ");
  strcat(other_str, loc("First"));
  strcat(other_str, " ");
  strcat(other_str, loc("Last"));
  strcat(other_str, " ");
  strcat(other_str, loc("Previous"));
  strcat(other_str, " ");
  strcat(other_str, loc("Next"));

  /* check if command is present in the menu list */
  if (command[0] && strstr(menu_str, command) == NULL &&
      strstr(other_str, command) == NULL)
    {
    sprintf(str, loc("Error: Command \"<b>%s</b>\" not allowed"), command);
    show_error(str);
    return;
    }

  if (equal_ustring(command, loc("Help")))
    {
    if (getcfg(logbook, "Help URL", str))
      {
      /* if file is given, add '/' to make absolute path */
      if (strchr(str, '/') == NULL)
        sprintf(ref, "/%s", str);
      else
        strcpy(ref, str);

      redirect3(ref);
      return;
      }

    /* send local help file */
    strcpy(file_name, cfg_dir);
    strcat(file_name, "eloghelp_");

    if (getcfg("global", "Language", str))
      {
      str[2] = 0;
      strcat(file_name, str);
      }
    else
      strcat(file_name, "en");
    strcat(file_name, ".html");

    f = fopen(file_name, "r");
    if (f == NULL)
      redirect3("http://midas.psi.ch/elog/eloghelp_en.html");
    else
      {
      fclose(f);
      send_file(file_name);
      }
    return;
    }

  if (equal_ustring(command, loc("New")))
    {
    show_elog_new(NULL, FALSE);
    return;
    }

  if (equal_ustring(command, loc("Edit")))
    {
    if (path[0])
      {
      show_elog_new(path, TRUE);
      return;
      }
    }

  if (equal_ustring(command, loc("Reply")))
    {
    show_elog_new(path, FALSE);
    return;
    }

  if (equal_ustring(command, loc("Submit")))
    {
    submit_elog();
    return;
    }

  if (equal_ustring(command, loc("Find")))
    {
    show_elog_find();
    return;
    }

  if (equal_ustring(command, loc("Search")))
    {
    show_elog_submit_find(0, 0);
    return;
    }

  if (equal_ustring(command, loc("Last day")))
    {
    redirect("past1");
    return;
    }

  if (equal_ustring(command, loc("Last 10")))
    {
    redirect("last10");
    return;
    }

  if (equal_ustring(command, loc("Copy to")))
    {
    copy_to(path, getparam("destc"), 0);
    return;
    }

  if (equal_ustring(command, loc("Move to")))
    {
    copy_to(path, getparam("destm"), 1);
    return;
    }

  if (equal_ustring(command, loc("Config")))
    {
    show_config_page();
    return;
    }

  if (equal_ustring(command, loc("Change Password")) ||
      isparam("newpwd"))
    {
    show_change_pwd_page();
    return;
    }

  if (equal_ustring(command, loc("Save")))
    {
    if (!save_config())
      return;
    redirect("");
    return;
    }

  if (equal_ustring(command, loc("Logout")))
    {
    rsprintf("HTTP/1.1 302 Found\r\n");
    rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
    if (use_keepalive)
      {
      rsprintf("Connection: Keep-Alive\r\n");
      rsprintf("Keep-Alive: timeout=60, max=10\r\n");
      }

    /* delete user cookies */
    rsprintf("Set-Cookie: upwd=; path=/%s; expires=Fri, 01 Jan 1983 00:00:00 GMT\r\n", logbook_enc, str);
    rsprintf("Set-Cookie: unm=; path=/%s; expires=Fri, 01 Jan 1983 00:00:00 GMT\r\n", logbook_enc, str);

    rsprintf("Location: /\r\n\r\n<html>redir</html>\r\n");
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

  if (equal_ustring(command, loc("Delete")))
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

  if (equal_ustring(command, loc("Back")) &&
      getcfg(logbook, "Back to main", str) &&
      atoi(str) == 1)
    {
    redirect3("/");
    return;
    }

  /*---- check if file requested -----------------------------------*/

  if ((strlen(path) > 13 && path[6] == '_' && path[13] == '_') ||
      (strlen(path) > 13 && path[6] == '_' && path[13] == '/') ||
      strstr(path, ".gif") || strstr(path, ".jpg") || strstr(path, ".png"))
    {
    if ((strlen(path) > 13 && path[6] == '_' && path[13] == '_') ||
        (strlen(path) > 13 && path[6] == '_' && path[13] == '/'))
      {
      if (path[13] == '/')
        path[13] = '_';

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

    send_file(file_name);
    return;
    }

  /*---- check next/previous message -------------------------------*/

  last_message = first_message = FALSE;
  if (equal_ustring(command, loc("Next")) || equal_ustring(command, loc("Previous")) ||
      equal_ustring(command, loc("Last")) || equal_ustring(command, loc("First")))
    {
    strcpy(orig_path, path);

    if (equal_ustring(command, loc("Last")) || equal_ustring(command, loc("First")))
      path[0] = 0;

    do
      {
      if (equal_ustring(command, loc("Next")) || equal_ustring(command, loc("First")))
        strcat(path, "+1");
      if (equal_ustring(command, loc("Previous")) || equal_ustring(command, loc("Last")))
        strcat(path, "-1");

      status = el_search_message(path, NULL, TRUE, equal_ustring(command, loc("First")));
      if (status != EL_SUCCESS)
        {
        if (equal_ustring(command, loc("Next")))
          last_message = TRUE;
        else
          first_message = TRUE;
        strcpy(path, orig_path);
        break;
        }

      size = sizeof(text);
      el_retrieve(path, date, attr_list, attrib, n_attr,
                  text, &size, orig_tag, reply_tag,
                  attachment, encoding);

      /* check for locked attributes */
      for (i=0 ; i<n_attr ; i++)
        {
        sprintf(lattr, "l%s", attr_list[i]);
        if (*getparam(lattr) == '1' && !equal_ustring(getparam(attr_list[i]), attrib[i]))
          break;
        }
      if (i < n_attr)
        continue;

      /* check for attribute filter if not browsing */
      if (!*getparam("browsing"))
        {
        for (i=0 ; i<n_attr ; i++)
          {
          if (*getparam(attr_list[i]) && !equal_ustring(getparam(attr_list[i]), attrib[i]))
            break;
          }
        if (i < n_attr)
          continue;
        }

      sprintf(str, "%s", path);

      for (i=0 ; i<n_attr ; i++)
        {
        sprintf(lattr, "l%s", attr_list[i]);
        if (*getparam(lattr) == '1')
          {
          if (strchr(str, '?') == NULL)
            sprintf(str+strlen(str), "?%s=1", lattr);
          else
            sprintf(str+strlen(str), "&%s=1", lattr);
          }
        }

      redirect(str);
      return;

      } while (TRUE);
    }

  /*---- check for welcome page ------------------------------------*/

  if (!path[0] && getcfg(logbook, "Welcome page", str) && str[0])
    {
    strcpy(file_name, cfg_dir);
    strcat(file_name, str);
    send_file(file_name);
    return;
    }

  /*---- get current message ---------------------------------------*/

  size = sizeof(text);
  msg_status = el_retrieve(path, date, attr_list, attrib, n_attr,
                           text, &size, orig_tag, reply_tag,
                           attachment, encoding);

  /*---- header ----*/

  /* header */
  if (msg_status == EL_SUCCESS)
    {
    str[0] = 0;

    if (getcfg(logbook, "Page Title", str))
      {
      i = build_subst_list(slist, svalue, attrib);
      strsubst(str, slist, svalue, i);
      }
    else
      strcpy(str, "ELOG");

    if (str[0])
      show_standard_header(str, path);
    else
      show_standard_header(logbook, path);
    }
  else
    show_standard_header("", "");

  /*---- title ----*/

  show_standard_title(logbook, "", 0);

  /*---- menu buttons ----*/

  rsprintf("<tr><td><table width=100%% border=0 cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Menu1 cellpadding"), gt("Frame color"));

  rsprintf("<tr><td align=%s bgcolor=%s>\n", gt("Menu1 Align"), gt("Menu1 BGColor"));

  n = strbreak(menu_str, menu_item, MAX_N_LIST);

  if (atoi(gt("Use buttons")) == 1)
    {
    for (i=0 ; i<n ; i++)
      {
      /* strip "logbook" from "move to / copy to" commands */
      strcpy(cmd, menu_item[i]);
      if (strchr(cmd, '\"'))
        *strchr(cmd, '\"') = 0;

      /* strip trailing blanks */
      while (cmd[strlen(cmd)-1] == ' ')
        cmd[strlen(cmd)-1] = 0;

      /* display menu item */
      rsprintf("<input type=submit name=cmd value=\"%s\">\n", cmd);

      if (equal_ustring(cmd, loc("Copy to")) ||
          equal_ustring(cmd, loc("Move to")))
        {
        /* put one link for each logbook except current one */
        if (equal_ustring(cmd, loc("Copy to")))
          rsprintf("<select name=destc>\n");
        else
          rsprintf("<select name=destm>\n");

        for (j=0 ;  ; j++)
          {
          if (!enumgrp(j, str))
            break;

          if (equal_ustring(str, "global"))
            continue;

          if (equal_ustring(str, logbook))
            continue;

          rsprintf("<option value=\"%s\">%s\n", str, str);
          }
        rsprintf("</select>\n");
        }
      }
    }
  else
    {
    rsprintf("<small>\n");

    for (i=0 ; i<n ; i++)
      {
      /* display menu item */

      /* strip "logbook" from "move to / copy to" commands */
      strcpy(cmd, menu_item[i]);
      if (strchr(cmd, '\"'))
        *strchr(cmd, '\"') = 0;

      /* strip trailing blanks */
      while (cmd[strlen(cmd)-1] == ' ')
        cmd[strlen(cmd)-1] = 0;

      if (equal_ustring(cmd, loc("Copy to")) ||
          equal_ustring(cmd, loc("Move to")))
        {
        if (equal_ustring(cmd, loc("Copy to")))
          len = strlen(loc("Copy to"));
        else
          len = strlen(loc("Move to"));

        /* if logbook supplied, put a link to that logbook */
        if ((int)strlen(menu_item[i]) > len)
          {
          /* extract logbook */
          if (menu_item[i][len+1] == '"')
            strcpy(ref, menu_item[i]+len+2);
          else
            strcpy(ref, menu_item[i]+len+1);
          if (strchr(ref, '"'))
            *strchr(ref, '"') = 0;

          url_encode(ref);

          if (equal_ustring(cmd, loc("Copy to")))
            rsprintf("&nbsp;<a href=\"/%s/%s?cmd=%s&destc=%s\">%s</a>&nbsp|\n",
                      logbook_enc, path, loc("Copy to"), ref, menu_item[i]);
          else
            rsprintf("&nbsp;<a href=\"/%s/%s?cmd=%s&destm=%s\">%s</a>&nbsp|\n",
                      logbook_enc, path, loc("Move to"), ref, menu_item[i]);
          }
        else
          {
          /* put one link for each logbook except current one */
          for (j=0 ;  ; j++)
            {
            if (!enumgrp(j, str))
              break;

            if (equal_ustring(str, "global"))
              continue;

            if (equal_ustring(str, logbook))
              continue;

            strcpy(ref, str);
            url_encode(ref);
            if (equal_ustring(menu_item[i], "copy to"))
              rsprintf("&nbsp;<a href=\"/%s/%s?cmd=%s&destc=%s\">Copy to \"%s\"</a>&nbsp|\n",
                        logbook_enc, path, loc("Copy to"), ref, str);
            else
              rsprintf("&nbsp;<a href=\"/%s/%s?cmd=%s&destm=%s\">Move to \"%s\"</a>&nbsp|\n",
                        logbook_enc, path, loc("Move to"), ref, str);
            }
          }
        }
      else
        {
        strcpy(str, menu_item[i]);
        url_encode(str);

        if (i < n-1)
          rsprintf("&nbsp;<a href=\"/%s/%s?cmd=%s\">%s</a>&nbsp;|\n", logbook_enc, path, str, menu_item[i]);
        else
          rsprintf("&nbsp;<a href=\"/%s/%s?cmd=%s\">%s</a>&nbsp;\n", logbook_enc, path, str, menu_item[i]);
        }
      }

    rsprintf("</small>\n");
    }

  rsprintf("</td>\n");

  if (atoi(gt("Merge menus")) != 1)
    {
    rsprintf("</tr>\n");
    rsprintf("</table></td></tr>\n\n");
    }

  /*---- next/previous buttons ----*/

  if (!getcfg(logbook, "Enable browsing", str) ||
       atoi(str) == 1)
    {
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
      rsprintf("<input type=image name=cmd_first border=0 alt=\"%s\" src=\"/%s/first.gif\">\n",
               loc("First entry"), logbook_enc);
      rsprintf("<input type=image name=cmd_previous border=0 alt=\"%s\" src=\"/%s/previous.gif\">\n",
               loc("Previous entry"), logbook_enc);
      rsprintf("<input type=image name=cmd_next border=0 alt=\"%s\" src=\"/%s/next.gif\">\n",
               loc("Next entry"), logbook_enc);
      rsprintf("<input type=image name=cmd_last border=0 alt=\"%s\" src=\"/%s/last.gif\">\n",
               loc("Last entry"), logbook_enc);
      }
    else
      {
      rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("First"));
      rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Previous"));
      rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Next"));
      rsprintf("<input type=submit name=cmd value=\"%s\">\n", loc("Last"));
      }

    rsprintf("</td></tr>\n");
    }

  rsprintf("</table></td></tr>\n\n");

  /*---- message ----*/

  /* overall table for message giving blue frame */
  rsprintf("<tr><td><table width=100%% border=%s cellpadding=%s cellspacing=1 bgcolor=%s>\n",
           gt("Categories border"), gt("Categories cellpadding"), gt("Frame color"));

  if (msg_status == EL_FILE_ERROR ||
      (msg_status != EL_SUCCESS && !last_message && !first_message))
    rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><h1>%s</h1></tr>\n", loc("No message available"));
  else
    {
    /* check for locked attributes */
    for (i=0 ; i<n_attr ; i++)
      {
      sprintf(lattr, "l%s", attr_list[i]);
      if (*getparam(lattr) == '1')
        break;
      }
    if (i < n_attr)
      sprintf(str, " %s <i>\"%s = %s\"</i>", loc("with"), attr_list[i], getparam(attr_list[i]));
    else
      str[0] = 0;

    if (last_message)
      rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>%s %s</b></tr>\n", 
               loc("This is the last entry"), str);

    if (first_message)
      rsprintf("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>%s %s</b></tr>\n", 
               loc("This is the first entry"), str);

    /* check for mail submissions */
    if (*getparam("suppress"))
      {
      if (getcfg(logbook, "Suppress default", str) && atoi(str) == 1)
        rsprintf("<tr><td colspan=2 bgcolor=#FFFFFF>%s</tr>\n", loc("Email notification suppressed"));
      else
        rsprintf("<tr><td colspan=2 bgcolor=#FF0000><b>%s</b></tr>\n", loc("Email notification suppressed"));
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
          rsprintf("%s <b>%s</b><br>\n", loc("Mail sent to"), getparam(str));
          }
        else
          break;
        }
      }

    if (i>0)
      rsprintf("</tr>\n");

    /*---- display date ----*/

    if (getcfg(logbook, "Date format", format))
      {
      struct tm ts;

      memset(&ts, 0, sizeof(ts));

      for (i=0 ; i<12 ; i++)
        if (strncmp(date+4, mname[i], 3) == 0)
          break;
      ts.tm_mon = i;

      ts.tm_mday = atoi(date+8);
      ts.tm_hour = atoi(date+11);
      ts.tm_min  = atoi(date+14);
      ts.tm_sec  = atoi(date+17);
      ts.tm_year = atoi(date+20)-1900;

      mktime(&ts);
      strftime(str, sizeof(str), format, &ts);

      rsprintf("<tr><td nowrap bgcolor=%s width=10%%><b>%s:</b></td><td bgcolor=%s>%s\n\n",
               gt("Categories bgcolor1"), loc("Entry date"), gt("Categories bgcolor2"), str);
      }
    else
      rsprintf("<tr><td nowrap bgcolor=%s width=10%%><b>%s:</b></td><td bgcolor=%s>%s\n\n",
               gt("Categories bgcolor1"), loc("Entry date"), gt("Categories bgcolor2"), date);

    for (i=0 ; i<n_attr ; i++)
      rsprintf("<input type=hidden name=\"%s\" value=\"%s\">\n", attr_list[i], attrib[i]);

    /* browsing flag to distinguish "/../<attr>=<value>" from browsing */
    rsprintf("<input type=hidden name=browsing value=1>\n");

    rsprintf("</td></tr>\n\n");

    /*---- link to original message or reply ----*/

    if (msg_status != EL_FILE_ERROR && (reply_tag[0] || orig_tag[0]))
      {

      if (orig_tag[0])
        {
        rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));
        sprintf(ref, "/%s/%s", logbook_enc, orig_tag);
        rsprintf("<b>%s:</b></td><td bgcolor=%s>", loc("In reply to"), gt("Menu2 bgcolor"));
        rsprintf("<a href=\"%s\">%s</a></td></tr>\n", ref, orig_tag);
        }
      if (reply_tag[0])
        {
        rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));
        sprintf(ref, "/%s/%s", logbook_enc, reply_tag);
        rsprintf("<b>%s:</b></td><td bgcolor=%s>", loc("Reply to this"), gt("Menu2 bgcolor"));
        rsprintf("<a href=\"%s\">%s</a></td></tr>\n", ref, reply_tag);
        }
      }

    /*---- display attributes ----*/

    for (i=0 ; i<n_attr ; i++)
      {
      sprintf(lattr, "l%s", attr_list[i]);
      rsprintf("<tr><td nowrap width=10%% bgcolor=%s>", gt("Categories bgcolor1"));

      if (!getcfg(logbook, "Filtered browsing", str) ||
          atoi(str) == 1)
        {
        if (*getparam(lattr) == '1')
          rsprintf("<input type=\"checkbox\" checked name=\"%s\" value=\"1\">&nbsp;", lattr);
        else
          rsprintf("<input alt=\"text\" type=\"checkbox\" name=\"%s\" value=\"1\">&nbsp;", lattr);
        }

      /* display checkbox for boolean attributes */
      if (equal_ustring(attr_options[i][0], "boolean"))
        {
        if (atoi(attrib[i]) == 1)
          rsprintf("<b>%s:</b></td><td bgcolor=%s><input type=checkbox checked disabled></td></tr>\n",
                   attr_list[i], gt("Categories bgcolor2"));
        else
          rsprintf("<b>%s:</b></td><td bgcolor=%s><input type=checkbox disabled></td></tr>\n",
                   attr_list[i], gt("Categories bgcolor2"));
        }
      else
        {
        rsprintf("<b>%s:</b></td><td bgcolor=%s>\n",
                 attr_list[i], gt("Categories bgcolor2"));
        rsputs2(attrib[i]);
        rsprintf("&nbsp</td></tr>\n");
        }
      }

    rsprintf("</td></tr>\n");
    rsputs("</table></td></tr>\n");

    /*---- message text ----*/

    if (!getcfg(logbook, "Show text", str) || atoi(str) == 1)
      {
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
      }

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

        strcpy(str, attachment[index]);
        str[13] = 0;
        sprintf(ref, "/%s/%s/%s", logbook_enc, str, attachment[index]+14);

        rsprintf("<tr><td><table width=100%% border=0 cellpadding=0 cellspacing=1 bgcolor=%s>\n", gt("Frame color"));

        rsprintf("<tr><td nowrap width=10%% bgcolor=%s><b>&nbsp;%s %d:</b></td>",
                 gt("Categories bgcolor1"), loc("Attachment"), index+1);
        rsprintf("<td bgcolor=%s>&nbsp;<a href=\"%s\">%s</a>\n",
                 gt("Categories bgcolor2"), ref, attachment[index]+14);

        if (length < 1024)
          rsprintf("&nbsp;<small><b>%d Bytes</b></small>", length);
        else if (length < 1024*1024)
          rsprintf("&nbsp;<small><b>%d kB</b></small>", length/1024);
        else
          rsprintf("&nbsp;<small><b>%1.3lf MB</b></small>", length/1024.0/1024.0);

        rsprintf("</td></tr></table></td></tr>\n");

        if (!getcfg(logbook, "Show attachments", str) ||
            atoi(str) == 1)
          {
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
    }

  /* table for message body */
  rsprintf("</td></tr></table></td></tr>\n\n");

  /* overall table */
  rsprintf("</td></tr></table></td></tr>\n");

  if (getcfg(logbook, "bottom text", str))
    {
    FILE *f;
    char file_name[256], *buf;

    strcpy(file_name, cfg_dir);
    strcat(file_name, str);

    f = fopen(file_name, "r");
    if (f != NULL)
      {
      fseek(f, 0, SEEK_END);
      size = TELL(fileno(f));
      fseek(f, 0, SEEK_SET);

      buf = malloc(size+1);
      fread(buf, 1, size, f);
      buf[size] = 0;
      fclose(f);

      rsputs(buf);
      }
    }
  else
    /* add little logo */
    rsprintf("<center><font size=1 color=#A0A0A0><a href=\"http://midas.psi.ch/elog/\">ELOG V%s</a></font></center>", VERSION);

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

BOOL check_password(char *logbook, char *name, char *password, char *redir)
{
char  str[256];

  /* get password from configuration file */
  if (getcfg(logbook, name, str))
    {
    if (strcmp(password, str) == 0)
      return TRUE;

    /* show web password page */
    show_standard_header(loc("ELOG password"), NULL);

    /* define hidden fields for current destination */
    if (redir[0])
      rsprintf("<input type=hidden name=redir value=\"%s\">\n", redir);

    rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>",
              gt("Border width"), gt("Frame color"));
    rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

    if (password[0])
      rsprintf("<tr><th bgcolor=#FF0000>%s!</th></tr>\n", loc("Wrong password"));

    rsprintf("<tr><td align=center bgcolor=%s>\n", gt("Title bgcolor"));

    if (strcmp(name, "Write password") == 0)
      {
      rsprintf("<font color=%s>%s</font></td></tr>\n",
               gt("Title fontcolor"), loc("Please enter password to obtain write access"));
      rsprintf("<tr><td align=center bgcolor=%s><input type=password name=wpassword></td></tr>\n", gt("Cell BGColor"));
      }
    else
      {
      rsprintf("<font color=%s>%s</font></td></tr>\n",
               gt("Title fontcolor"), loc("Please enter password to obtain administration access"));
      rsprintf("<tr><td align=center bgcolor=%s><input type=password name=apassword></td></tr>\n", gt("Cell BGColor"));
      }

    rsprintf("<tr><td align=center bgcolor=%s><input type=submit value=\"%s\"></td></tr>", gt("Cell BGColor"), loc("Submit"));

    rsprintf("</table></td></tr></table>\n");

    rsprintf("</body></html>\r\n");

    return FALSE;
    }
  else
    return TRUE;
}

/*------------------------------------------------------------------*/

BOOL check_user_password(char *logbook, char *user, char *password, char *redir)
{
char  str[256], line[256], file_name[256], *p;
FILE  *f;

  getcfg(logbook, "Password file", str);
  strcpy(file_name, cfg_dir);
  strcat(file_name, str);

  f = fopen(file_name, "r");
  if (f != NULL)
    {
    while (!feof(f))
      {
      line[0] = 0;
      fgets(line, sizeof(line), f);

      if (line[0] == ';' || line[0] == '#' || line[0] == 0)
        continue;

      strcpy(str, line);
      if (strchr(str, ':'))
        *strchr(str, ':') = 0;
      if (strcmp(str, user) == 0)
        break;
      }
    fclose(f);

    /* if user found, check password */
    if (user[0] && (strcmp(str, user) == 0))
      {
      p = line+strlen(str);
      if (*p)
        p++;

      strcpy(str, p);
      if (strchr(str, ':'))
        *strchr(str, ':') = 0;

      if (strcmp(password, str) == 0)
        {
        p += strlen(str);
        if (*p)
          p++;
        strcpy(str, p);
        if (strchr(str, ':'))
          *strchr(str, ':') = 0;
        if (strchr(str, '\r'))
          *strchr(str, '\r') = 0;
        if (strchr(str, '\n'))
          *strchr(str, '\n') = 0;
        setparam("full_name", str);
        return TRUE;
        }
      }

    /* show login password page */
    show_standard_header("ELOG login", NULL);

    /* define hidden fields for current destination */
    if (redir[0])
      rsprintf("<input type=hidden name=redir value=\"%s\">\n", redir);

    rsprintf("<p><p><p><table border=%s width=50%% bgcolor=%s cellpadding=1 cellspacing=0 align=center>",
              gt("Border width"), gt("Frame color"));
    rsprintf("<tr><td><table cellpadding=5 cellspacing=0 border=0 width=100%% bgcolor=%s>\n", gt("Frame color"));

    if (password[0])
      rsprintf("<tr><th bgcolor=#FF0000>%s!</th></tr>\n", loc("Wrong password"));

    rsprintf("<tr><td align=center bgcolor=%s>\n", gt("Title bgcolor"));

    rsprintf("<font color=%s>%s</font></td></tr>\n",
             gt("Title fontcolor"), loc("Please login"));
    rsprintf("<tr><td align=center bgcolor=%s>%s:&nbsp;&nbsp;&nbsp;<input type=text name=uname></td></tr>\n", 
             gt("Cell BGColor"), loc("Username"));
    rsprintf("<tr><td align=center bgcolor=%s>%s:&nbsp;&nbsp;&nbsp;<input type=password name=upassword></td></tr>\n", 
             gt("Cell BGColor"), loc("Password"));

    rsprintf("<tr><td align=center bgcolor=%s><input type=submit value=\"%s\"></td></tr>", gt("Cell BGColor"), loc("Submit"));

    rsprintf("</table></td></tr></table>\n");

    rsprintf("</body></html>\r\n");

    return FALSE;
    }
  else
    {
    sprintf(line, "Error: Password file \"%s\" not found", file_name);
    show_error(line);
    return FALSE;
    }
}

/*------------------------------------------------------------------*/

void show_selection_page()
{
int  i;
char str[10000], logbook[256];

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

 if (getcfg("global", "Page Title", str))
   {
   rsprintf("<title>");
   rsputs(str);
   rsprintf("</title>\n");
   }
 else
   rsprintf("<title>ELOG Logbook Selection</title>\n");

  rsprintf("</head>\n\n");

  rsprintf("<body>\n\n");

  rsprintf("<p><p><p><table border=0 width=50%% bgcolor=#486090 cellpadding=0 cellspacing=0 align=center>");
  rsprintf("<tr><td><table cellpadding=5 cellspacing=1 border=0 width=100%% bgcolor=#486090>\n");

  rsprintf("<tr><td align=center colspan=2 bgcolor=#486090>\n");

  if (getcfg("global", "Welcome title", str))
    {
    rsputs(str);
    }
  else
    {
    rsprintf("<font size=5 color=#FFFFFF>\n");
    rsprintf("%s.<BR>\n", loc("Several logbooks are defined on this host"));
    rsprintf("%s:</font></td></tr>\n", loc("Please select the one to connect to"));
    }

  for (i=0 ;  ; i++)
    {
    if (!enumgrp(i, logbook))
      break;

    if (equal_ustring(logbook, "global"))
      continue;

    strcpy(str, logbook);
    url_encode(str);
    rsprintf("<tr><td bgcolor=#CCCCFF><a href=\"/%s\">%s</a></td>", str, logbook);

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

void interprete(char *path)
/********************************************************************\

  Routine: interprete

  Purpose: Interprete parametersand generate HTML output.

  Input:
    char *path              Message path

  <implicit>
    _param/_value array accessible via getparam()

\********************************************************************/
{
int    i, n, index;
double exp;
char   str[256], enc_pwd[80], file_name[256];
char   enc_path[256], dec_path[256];
char   *experiment, *command, *value, *group;
time_t now;
struct tm *gmt;

  /* encode path for further usage */
  strcpy(dec_path, path);
  url_decode(dec_path);
  url_decode(dec_path); /* necessary for %2520 -> %20 -> ' ' */
  strcpy(enc_path, dec_path);
  url_encode(enc_path);

  experiment = getparam("exp");
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

  /* check for global selection page if no logbook given */
  if (!logbook[0] && getcfg("global", "Selection page", str))
    {
    strcpy(file_name, cfg_dir);
    strcat(file_name, str);
    send_file(file_name);
    return;
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
  getcfg(logbook, "Data dir", str);
  if (str[0] == DIR_SEPARATOR || str[1] == ':')
    strcpy(data_dir, str);
  else
    {
    strcpy(data_dir, cfg_dir);
    strcat(data_dir, str);
    }
  
  if (data_dir[strlen(data_dir)-1] != DIR_SEPARATOR)
    strcat(data_dir, DIR_SEPARATOR_STR);

  if (*getparam("wpassword"))
    {
    /* check if password correct */
    do_crypt(getparam("wpassword"), enc_pwd);

    if (!check_password(logbook, "Write password", enc_pwd, getparam("redir")))
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

    if (exp == 0)
      {
      rsprintf("Set-Cookie: wpwd=%s; path=/%s\r\n", enc_pwd, logbook_enc);
      }
    else
      {
      time(&now);
      now += (int) (3600*exp);
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

      rsprintf("Set-Cookie: wpwd=%s; path=/%s; expires=%s\r\n", enc_pwd, logbook_enc, str);
      }

    sprintf(str, "/%s/%s", logbook_enc, getparam("redir"));
    rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
    return;
    }

  if (*getparam("apassword"))
    {
    /* check if password correct */
    do_crypt(getparam("apassword"), enc_pwd);

    if (!check_password(logbook, "Admin password", enc_pwd, getparam("redir")))
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
    if (getcfg(logbook, "Admin password expiration", str))
      exp = atof(str);

    if (exp == 0)
      {
      rsprintf("Set-Cookie: apwd=%s; path=/%s\r\n", enc_pwd, logbook_enc);
      }
    else
      {
      time(&now);
      now += (int) (3600*exp);
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

      rsprintf("Set-Cookie: apwd=%s; path=/%s; expires=%s\r\n", enc_pwd, logbook_enc, str);
      }

    sprintf(str, "/%s/%s", logbook_enc, getparam("redir"));
    rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
    return;
    }

  if (*getparam("uname") && getparam("upassword"))
    {
    /* check if password correct */
    do_crypt(getparam("upassword"), enc_pwd);

    if (!check_user_password(logbook, getparam("uname"), enc_pwd, getparam("redir")))
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
    if (getcfg(logbook, "Login expiration", str))
      exp = atof(str);

    if (exp == 0)
      {
      rsprintf("Set-Cookie: upwd=%s; path=/%s\r\n", enc_pwd, logbook_enc);
      rsprintf("Set-Cookie: unm=%s; path=/%s\r\n", getparam("uname"), logbook_enc);
      }
    else
      {
      time(&now);
      now += (int) (3600*exp);
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

      rsprintf("Set-Cookie: upwd=%s; path=/%s; expires=%s\r\n", enc_pwd, logbook_enc, str);
      rsprintf("Set-Cookie: unm=%s; path=/%s; expires=%s\r\n", getparam("uname"), logbook_enc, str);
      }

    sprintf(str, "/%s/%s", logbook_enc, getparam("redir"));
    rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
    return;
    }

  /*---- show ELog page --------------------------------------------*/

  if (getcfg(logbook, "Password file", str))
    {
    if (!check_user_password(logbook, getparam("unm"), getparam("upwd"), path))
      return;
    }

  if (equal_ustring(command, loc("New")) ||
      equal_ustring(command, loc("Edit")) ||
      equal_ustring(command, loc("Reply")) ||
      equal_ustring(command, loc("Submit")) ||
      equal_ustring(command, loc("Delete")))
    {
    sprintf(str, "%s?cmd=%s", path, command);
    if (!check_password(logbook, "Write password", getparam("wpwd"), str))
      return;
    }

  if (equal_ustring(command, loc("Delete"))  ||
      equal_ustring(command, loc("Config"))  ||
      equal_ustring(command, loc("Copy to")) ||
      equal_ustring(command, loc("Move to")))
    {
    sprintf(str, "%s?cmd=%s", path, command);
    if (!check_password(logbook, "Admin password", getparam("apwd"), str))
      return;
    }

  show_elog_page(logbook, dec_path);
  return;
}

/*------------------------------------------------------------------*/

void decode_get(char *string)
{
char path[256];
char *p, *pitem;

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

  interprete(path);
}

/*------------------------------------------------------------------*/

void decode_post(char *string, char *boundary, int length)
{
char *pinit, *p, *pitem, *ptmp, file_name[256], str[256];
int  i, n;

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
            _attachment_size[n] = (int)(p - string);
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

    } while ((int)(string - pinit) < length);

  interprete("");
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
char                 cookie[256], boundary[256], list[1000],
                     host_list[MAX_N_LIST][NAME_LENGTH], rem_host_name[256],
                     rem_host_ip[256];
int                  lsock, len, flag, content_length, header_length;
struct hostent       *phe;
struct linger        ling;
fd_set               readfds;
struct timeval       timeout;

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
  serv_addr.sin_family = AF_INET;

  /* if no hostname given with the -h flag, listen on any interface */
  if (tcp_hostname[0] == 0)
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  else
    {
    /* look up the given hostname. gethostbyname() will take a hostname
       or an IP address */
    phe = gethostbyname(tcp_hostname);
    if (!phe)
      {
      printf("Cannot find address for -h %s\n", tcp_hostname);
      return;
      }
    if (phe->h_addrtype != AF_INET)
      {
      printf("Non Internet address for -h %s\n", tcp_hostname);
      return;
      }
    memcpy(&serv_addr.sin_addr.s_addr, phe->h_addr_list[0], phe->h_length);
    }

  serv_addr.sin_port = htons((short) tcp_port);

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
      _sock = accept(lsock, (struct sockaddr *) &acc_addr, &len);

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
      memcpy(&remote_addr[i_conn], &(acc_addr.sin_addr), sizeof(rem_addr));
      memcpy(&rem_addr, &(acc_addr.sin_addr), sizeof(rem_addr));

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
        memcpy(&rem_addr, &remote_addr[i_conn], sizeof(rem_addr));

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

        /* check if net_buffer too small */
        if (len >= sizeof(net_buffer))
          {
          /* drain incoming remaining data */
          do
            {
            FD_ZERO(&readfds);
            FD_SET(_sock, &readfds);

            timeout.tv_sec  = 2;
            timeout.tv_usec = 0;

            status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

            if (FD_ISSET(_sock, &readfds))
              i = recv(_sock, net_buffer, sizeof(net_buffer), 0);
            else
              break;
            } while (i);

          memset(return_buffer, 0, sizeof(return_buffer));
          strlen_retbuf = 0;
          return_length = 0;

          show_error("Submitted attachment too large, please increase WEB_BUFFER_SIZE in elogd.c and recompile");
          send(_sock, return_buffer, strlen_retbuf+1, 0);
          keep_alive = 0;
          if (verbose)
            {
            printf("==== Return ================================\n");
            puts(return_buffer);
            printf("\n\n");
            }
          goto error;
          }

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
            strncpy(str, net_buffer+6, sizeof(str));
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

      /* initialize parametr array */
      initparam();

      /* extract cookies */
      if ((p = strstr(net_buffer, "Cookie:")) != NULL)
        {
        p += 6;
        do
          {
          p++;
          while (*p && *p == ' ')
            p++;
          strncpy(str, p, sizeof(str));

          for (i=0 ; i<(int)strlen(str) ; i++)
            if (str[i] == '=' || str[i] == ';')
              break;

          if (str[i] == '=')
            {
            str[i] = 0;
            p += i+1;
            for (i=0 ; *p && *p != ';' && *p != '\r' && *p != '\n' ; i++)
              cookie[i] = *p++;
            cookie[i] = 0;
            }
          else
            {
            /* empty cookie */
            str[i] = 0;
            cookie[0] = 0;
            p += i;
            }

          /* store cookie as parameter */
          setparam(str, cookie);
          } while (*p && *p == ';');
        }

      memset(return_buffer, 0, sizeof(return_buffer));
      strlen_retbuf = 0;

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

        if (strstr(logbook, ".gif") || strstr(logbook, ".jpg") || strstr(logbook, ".png") ||
            strstr(logbook, ".htm"))
          {
          /* server file directly */
          strcpy(str, cfg_dir);
          strcat(str, logbook);
          send_file(str);
          send(_sock, return_buffer, return_length, 0);

          goto error;
          }
        else
          if (!equal_ustring(logbook, str) && logbook[0])
            {
            if (verbose)
              printf("\n\n\n%s\n", net_buffer);

            sprintf(str, "Error: logbook \"%s\" not defined in elogd.cfg", logbook);
            show_error(str);
            send(_sock, return_buffer, strlen(return_buffer), 0);

            if (verbose)
              {
              printf("==== Return ================================\n");
              puts(return_buffer);
              printf("\n\n");
              }
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
          if (verbose)
            printf("\n\n\n%s\n", net_buffer);

          strcpy(logbook, str);
          strcpy(logbook_enc, logbook);
          url_encode(logbook_enc);

          /* redirect to logbook, necessary to get optionsl cookies for that logbook */
          redirect("");

          send(_sock, return_buffer, strlen(return_buffer), 0);

          if (verbose)
            {
            printf("==== Return ================================\n");
            puts(return_buffer);
            printf("\n\n");
            }
          goto error;
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

      /*---- check "hosts deny" ----*/

      authorized = 1;
      if (getcfg(logbook, "Hosts deny", list))
        {
        phe = gethostbyaddr((char *) &rem_addr, 4, PF_INET);
        if (phe != NULL)
          strcpy(rem_host_name, phe->h_name);
        else
          strcpy(rem_host_name, "");

        strcpy(rem_host_ip, (char *)inet_ntoa(rem_addr));

        n = strbreak(list, host_list, MAX_N_LIST);

        /* check if current connection matches anyone on the list */
        for (i=0 ; i<n ; i++)
          {
          if (equal_ustring(rem_host_name, host_list[i]) ||
              equal_ustring(rem_host_ip, host_list[i]) ||
              equal_ustring(host_list[i], "all"))
            {
            if (verbose)
              printf("Remote host \"%s\" matches \"%s\" in \"Hosts deny\". Access denied.\n",
                      equal_ustring(rem_host_ip, host_list[i]) ? rem_host_ip : rem_host_name,
                      host_list[i]);
            authorized = 0;
            break;
            }
          if (host_list[i][0] == '.')
            {
            if (strlen(rem_host_name) > strlen(host_list[i]) &&
                equal_ustring(host_list[i], rem_host_name+strlen(rem_host_name)-strlen(host_list[i])))
              {
              if (verbose)
                printf("Remote host \"%s\" matches \"%s\" in \"Hosts deny\". Access denied.\n",
                        rem_host_name, host_list[i]);
              authorized = 0;
              break;
              }
            }
          if (host_list[i][strlen(host_list[i])-1] == '.')
            {
            strcpy(str, rem_host_ip);
            if (strlen(str) > strlen(host_list[i]))
              str[strlen(host_list[i])] = 0;

            if (equal_ustring(host_list[i], str))
              {
              if (verbose)
                printf("Remote host \"%s\" matches \"%s\" in \"Hosts deny\". Access denied.\n",
                        rem_host_ip, host_list[i]);
              authorized = 0;
              break;
              }
            }
          }
        }

      /*---- check "hosts allow" ----*/

      if (getcfg(logbook, "Hosts allow", list))
        {
        phe = gethostbyaddr((char *) &rem_addr, 4, PF_INET);
        if (phe != NULL)
          strcpy(rem_host_name, phe->h_name);
        else
          strcpy(rem_host_name, "");

        strcpy(rem_host_ip, (char *)inet_ntoa(acc_addr.sin_addr));

        n = strbreak(list, host_list, MAX_N_LIST);

        /* check if current connection matches anyone on the list */
        for (i=0 ; i<n ; i++)
          {
          if (equal_ustring(rem_host_name, host_list[i]) ||
              equal_ustring(rem_host_ip, host_list[i]) ||
              equal_ustring(host_list[i], "all"))
            {
            if (verbose)
              printf("Remote host \"%s\" matches \"%s\" in \"Hosts allow\". Access granted.\n",
                      equal_ustring(rem_host_ip, host_list[i]) ? rem_host_ip : rem_host_name,
                      host_list[i]);
            authorized = 1;
            break;
            }
          if (host_list[i][0] == '.')
            {
            if (strlen(rem_host_name) > strlen(host_list[i]) &&
                equal_ustring(host_list[i], rem_host_name+strlen(rem_host_name)-strlen(host_list[i])))
              {
              if (verbose)
                printf("Remote host \"%s\" matches \"%s\" in \"Hosts allow\". Access granted.\n",
                        rem_host_name, host_list[i]);
              authorized = 1;
              break;
              }
            }
          if (host_list[i][strlen(host_list[i])-1] == '.')
            {
            strcpy(str, rem_host_ip);
            if (strlen(str) > strlen(host_list[i]))
              str[strlen(host_list[i])] = 0;

            if (equal_ustring(host_list[i], str))
              {
              if (verbose)
                printf("Remote host \"%s\" matches \"%s\" in \"Hosts allow\". Access granted.\n",
                        rem_host_ip, host_list[i]);
              authorized = 1;
              break;
              }
            }
          }
        }

      if (!authorized)
        {
        keep_alive = 0;
        goto error;
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
            do_crypt(p, str);
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
        rsprintf("Server: ELOG HTTP %s\r\n", VERSION);
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
          decode_get(p);
          }
        else if (strncmp(net_buffer, "POST", 4) == 0)
          {
          if (verbose)
            printf("%s\n", net_buffer+header_length);
          decode_post(net_buffer+header_length, boundary, content_length);
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
          return_length = strlen_retbuf+1;

        if (keep_alive && strstr(return_buffer, "Content-Length") == NULL)
          {
          /*---- add content-length ----*/

          p = strstr(return_buffer, "\r\n\r\n");

          if (p != NULL)
            {
            length = strlen(p+4);

            header_length = (int) (p - return_buffer);
            memcpy(header_buffer, return_buffer, header_length);

            sprintf(header_buffer+header_length, "\r\nContent-Length: %d\r\n\r\n", length);

            send(_sock, header_buffer, strlen(header_buffer), 0);
            send(_sock, p+4, length, 0);

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
      printf("Cannot create \"%s\".\n", cfg_file);
      return;
      }
    sprintf(str, "[%s]\n%s=%s\n", logbook, name, pwd);
    write(fh, str, strlen(str));
    close(fh);
    printf("File \"%s\" created with password in logbook \"%s\".\n", cfg_file, logbook);
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
        i = (int) (p - cfgbuffer);
        write(fh, cfgbuffer, i);
        sprintf(str, "%s=%s\n", name, pwd);
        write(fh, str, strlen(str));

        printf("Password replaced in logbook \"%s\".\n", logbook);

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

      i = (int) (p - cfgbuffer);
      write(fh, cfgbuffer, i);
      sprintf(str, "%s=%s\n", name, pwd);
      write(fh, str, strlen(str));

      printf("Password added to logbook \"%s\".\n", logbook);

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

    printf("Password added to new logbook \"%s\".\n", logbook);
    }

  free(cfgbuffer);
  close(fh);
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
int i;
int tcp_port = 80, daemon = FALSE;
char read_pwd[80], write_pwd[80], admin_pwd[80], str[80];
time_t now;
struct tm *tms;

  tzset();

  read_pwd[0] = write_pwd[0] = admin_pwd[0] = logbook[0] = 0;

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
      else if (argv[i][1] == 'a')
        strcpy(admin_pwd, argv[++i]);
      else if (argv[i][1] == 'l')
        strcpy(logbook, argv[++i]);
      else if (argv[i][1] == 'h')
        strncpy(tcp_hostname, argv[++i], sizeof(tcp_hostname));
      else
        {
usage:
        printf("usage: %s [-p port] [-h hostname] [-D] [-c file] [-r pwd] [-w pwd] [-a pwd] [-l loggbook]\n\n", argv[0]);
        printf("       -p <port> TCP/IP port\n");
        printf("       -h <hostname> TCP/IP hostname\n");
        printf("       -D become a daemon\n");
        printf("       -c <file> specify configuration file\n");
        printf("       -v debugging output\n");
        printf("       -r create/overwrite read password in config file\n");
        printf("       -w create/overwrite write password in config file\n");
        printf("       -a create/overwrite admin password in config file\n");
        printf("       -l <logbook> specify logbook for -r and -w commands\n\n");
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
    do_crypt(read_pwd, str);
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
    do_crypt(write_pwd, str);
    create_password(logbook, "Write Password", str);
    return 0;
    }

  if (admin_pwd[0])
    {
    if (!logbook[0])
      {
      printf("Must specify a lookbook via the -l parameter.\n");
      return 0;
      }
    do_crypt(admin_pwd, str);
    create_password(logbook, "Admin Password", str);
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

/********************************************************************\

  Name:         mhttpd.c
  Created by:   Stefan Ritt

  Contents:     Web server program for midas RPC calls

  $Log$
  Revision 1.105  2000/03/21 13:48:06  midas
  Web password expires after one day instead of one hour

  Revision 1.104  2000/03/18 08:02:32  midas
  Increased idle time logout to 1h

  Revision 1.103  2000/03/17 13:10:49  midas
  Make port bind check before becoming a daemon

  Revision 1.102  2000/03/15 08:54:39  midas
  Removed web passwd check from "submit" button (necessary if someone edits
  an elog entry for more than an hour)

  Revision 1.101  2000/03/13 09:41:53  midas
  Added refresh of Web password

  Revision 1.100  2000/03/08 23:43:49  midas
  Fixed bug that wrong size was given to el_retrieve

  Revision 1.99  2000/03/08 22:42:01  midas
  Fixed problem with elog froms and several experiments

  Revision 1.98  2000/03/03 01:45:13  midas
  Added web password for mhttpd, added webpasswd command in odbedit

  Revision 1.97  2000/03/01 23:04:49  midas
  Avoid analyzer ratio > 1

  Revision 1.96  2000/03/01 00:53:14  midas
  Use double events_sent values

  Revision 1.95  2000/02/25 23:41:02  midas
  Fixed secondary problem with conversion flags, adjusted mhttpd display of
  event number (M and G)

  Revision 1.94  2000/02/25 22:19:09  midas
  Improved Ctrl-C handling

  Revision 1.93  2000/02/24 22:29:24  midas
  Added deferred transitions

  Revision 1.92  1999/12/08 12:12:22  midas
  Modified "&str" to "str"

  Revision 1.91  1999/11/26 10:49:58  midas
  Display alarym system on/off flag on main status page

  Revision 1.90  1999/11/23 10:17:55  midas
  SC equipment is now normally displayed if no equipment settings are defined

  Revision 1.89  1999/11/11 12:46:36  midas
  Changed alias bar color

  Revision 1.88  1999/11/11 11:53:10  midas
  Link to auto restart and alarms on/off directly goes to "set" page

  Revision 1.87  1999/11/10 10:39:35  midas
  Changed text field size for ODB set page

  Revision 1.86  1999/11/08 14:31:09  midas
  Added hotlink to auto restart in status page

  Revision 1.85  1999/11/08 13:56:09  midas
  Added different alarm types

  Revision 1.84  1999/10/28 13:26:16  midas
  Added "alarms on/off" button

  Revision 1.83  1999/10/28 09:01:25  midas
  Added show_error to display proper HTTP header in error messages

  Revision 1.82  1999/10/22 10:48:25  midas
  Added auto restart in status page

  Revision 1.81  1999/10/20 09:22:39  midas
  Added -c flag

  Revision 1.80  1999/10/19 11:04:13  midas
  Changed expiration date of midas_ref cookie to one year

  Revision 1.79  1999/10/18 11:47:33  midas
  Corrected link to lazy settings

  Revision 1.78  1999/10/15 12:15:52  midas
  Added daemon function

  Revision 1.77  1999/10/15 10:54:03  midas
  Changed column width in elog page

  Revision 1.76  1999/10/15 10:39:51  midas
  Defined function rsputs, because rsprintf screwed if '%' in arguments

  Revision 1.74  1999/10/11 14:15:29  midas
  Use ss_system to launch programs

  Revision 1.73  1999/10/11 12:13:43  midas
  Increased socket timeout

  Revision 1.72  1999/10/11 11:57:39  midas
  Fixed bug with search in full text

  Revision 1.71  1999/10/11 10:52:43  midas
  Added full text search feature

  Revision 1.70  1999/10/11 10:40:44  midas
  Fixed bug with form submit

  Revision 1.69  1999/10/08 22:00:29  midas
  Finished editing of elog messages

  Revision 1.68  1999/10/08 15:07:04  midas
  Program check creates new internal alarm when triggered

  Revision 1.67  1999/10/08 08:38:09  midas
  Added "last<n> messages" feature

  Revision 1.66  1999/10/07 14:04:49  midas
  Added select() before recv() to avoid hangups

  Revision 1.65  1999/10/07 13:31:18  midas
  Fixed truncated date in el_submit, cut off @host in author search

  Revision 1.64  1999/10/07 13:17:34  midas
  Put a few EXPRT im msystem.h to make NT happy, updated NT makefile

  Revision 1.63  1999/10/06 18:01:12  midas
  Fixed bug with subject in new messages

  Revision 1.62  1999/10/06 07:57:55  midas
  Added "last 10 messages" feature

  Revision 1.61  1999/10/06 07:04:59  midas
  Edit mode half finished

  Revision 1.60  1999/10/05 13:36:15  midas
  Use strencode

  Revision 1.59  1999/10/05 13:25:43  midas
  Evaluate global alarm flag

  Revision 1.58  1999/10/05 12:48:17  midas
  Added expiration date 2040 for refresh cookie

  Revision 1.57  1999/10/04 14:16:10  midas
  Fixed bug in form submit

  Revision 1.56  1999/10/04 14:11:43  midas
  Added full display for query

  Revision 1.55  1999/09/29 21:08:54  pierre
  - Modified the Lazy status line for multiple channels

  Revision 1.54  1999/09/28 10:24:15  midas
  Display text attachments

  Revision 1.52  1999/09/27 12:53:50  midas
  Finished alarm system

  Revision 1.51  1999/09/27 09:18:05  midas
  Fixed bug

  Revision 1.50  1999/09/27 09:14:54  midas
  Use _blank for new windows

  Revision 1.49  1999/09/27 09:11:13  midas
  Check domain name in hostname

  Revision 1.48  1999/09/27 08:57:29  midas
  Use <pre> for plain text

  Revision 1.46  1999/09/22 08:57:08  midas
  Implemented auto start and auto stop in /programs

  Revision 1.45  1999/09/21 14:57:39  midas
  Added "execute on start/stop" under /programs

  Revision 1.44  1999/09/21 14:41:35  midas
  Close server socket before system() call

  Revision 1.43  1999/09/21 14:15:03  midas
  Replaces cm_execute by system()

  Revision 1.42  1999/09/21 13:47:39  midas
  Added "programs" page

  Revision 1.41  1999/09/17 15:59:03  midas
  Added internal alarms

  Revision 1.40  1999/09/17 15:11:18  midas
  Fixed bug (triggered is INT, not BOOL)

  Revision 1.39  1999/09/17 15:06:46  midas
  Moved al_check into cm_yield() and rpc_server_thread

  Revision 1.38  1999/09/17 11:47:52  midas
  Added setgid()

  Revision 1.36  1999/09/16 07:36:10  midas
  Added automatic host name in author field

  Revision 1.35  1999/09/15 13:33:32  midas
  Added remote el_submit functionality

  Revision 1.34  1999/09/15 09:33:05  midas
  Re-establish ctrlc-handler

  Revision 1.33  1999/09/15 09:30:49  midas
  Added Ctrl-C handler

  Revision 1.32  1999/09/15 08:05:09  midas
  - Added "last" button
  - Fixed bug that only partial file attachments were returned
  - Corrected query title, since dates are sorted beginning with oldest

  Revision 1.31  1999/09/14 15:15:44  midas
  Moved el_xxx funtions into midas.c

  Revision 1.30  1999/09/14 13:06:32  midas
  Added raw file display (for runlog.txt for now...)

  Revision 1.29  1999/09/14 11:45:25  midas
  Finished run number query

  Revision 1.28  1999/09/14 09:47:36  midas
  Fixed '/' and '\' handling with attachments

  Revision 1.27  1999/09/14 09:20:18  midas
  Finished reply

  Revision 1.26  1999/09/13 15:45:50  midas
  Added forms

  Revision 1.25  1999/09/13 09:50:16  midas
  Finished filtered browsing

  Revision 1.24  1999/09/10 06:10:46  midas
  Reply half finished

  Revision 1.23  1999/09/02 15:02:13  midas
  Display attachments

  Revision 1.22  1999/09/02 10:17:16  midas
  Implemented POST method

  Revision 1.21  1999/08/31 15:12:02  midas
  Finished query

  Revision 1.20  1999/08/27 08:14:26  midas
  Added query

  Revision 1.19  1999/08/26 15:19:05  midas
  Added Next/Previous button

  Revision 1.18  1999/08/24 13:45:44  midas
  - Made Ctrl-C working
  - Re-enabled reuse of port address
  - Added http:// destinations to /alias
  - Made refresh working under kfm (05 instead 5!)

  Revision 1.17  1999/08/13 09:41:07  midas
  Replaced getdomainname by gethostbyaddr

  Revision 1.14  1999/08/12 15:44:50  midas
  Fixed bug when subkey was in variables list, added domainname for unit

  Revision 1.13  1999/07/01 11:12:32  midas
  Changed lazy display if in FTP mode

  Revision 1.12  1999/06/23 09:39:31  midas
  Added lazy logger display

  Revision 1.11  1999/06/05 13:59:48  midas
  Converted ftp entry into ftp://host/dir/file

  Revision 1.10  1999/04/19 07:48:21  midas
  Message display uses cm_msg_retrieve to display old messages

  Revision 1.9  1999/02/18 11:23:50  midas
  Added level parameter to search_callback

  Revision 1.8  1999/01/20 13:51:56  midas
  Fixed some bugs in displaying/setting slow control values

  Revision 1.7  1998/12/11 11:15:50  midas
  Rearranged URL to make it work under the KDE browser, but auto refresh still
  does not work there.

  Revision 1.6  1998/10/28 13:48:17  midas
  Added message display

  Revision 1.5  1998/10/28 12:39:45  midas
  Added hex display in ODB page

  Revision 1.4  1998/10/28 12:25:13  midas
  Fixed bug causing comments in start page being truncated

  Revision 1.3  1998/10/23 14:21:49  midas
  - Modified version scheme from 1.06 to 1.6.0
  - cm_get_version() now returns versino as string

  Revision 1.2  1998/10/12 12:19:02  midas
  Added Log tag in header


\********************************************************************/

#include "midas.h"
#include "msystem.h"

/* refresh times in seconds */
#define DEFAULT_REFRESH 60

/* time until mhttpd disconnects from MIDAS */
#define CONNECT_TIME  3600

char return_buffer[1000000];
int  return_length;
char mhttpd_url[256];
char exp_name[32];
BOOL connected, no_disconnect;

#define MAX_GROUPS 32
#define MAX_PARAM  100
#define VALUE_SIZE 256
#define TEXT_SIZE  4096

char _param[MAX_PARAM][NAME_LENGTH];
char _value[MAX_PARAM][VALUE_SIZE];
char _text[TEXT_SIZE];
char *_attachment_buffer[3];
INT  _attachment_size[3];
char remote_host_name[256];
INT  _sock;

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

/*------------------------------------------------------------------*/

void rsputs(const char *str, ...)
{
  if (strlen(return_buffer) + strlen(str) > sizeof(return_buffer))
    strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
  else
    strcat(return_buffer, str);
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

char message_buffer[256];

INT print_message(const char *message)
{
  strcpy(message_buffer, message);
  return SUCCESS;
}

void receive_message(HNDLE hBuf, HNDLE id, EVENT_HEADER *pheader, void *message)
{
time_t tm;
char   str[80], line[256];

  /* prepare time */
  time(&tm);
  strcpy(str, ctime(&tm));
  str[19] = 0;

  /* print message text which comes after event header */
  sprintf(line, "%s %s", str+11, message);
  print_message(line);
}

/*------------------------------------------------------------------*/

void redirect(char *path)
{
  /* redirect */
  rsprintf("HTTP/1.0 302 Found\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

  if (exp_name[0])
    {
    if (strchr(path, '?'))
      rsprintf("Location: %s%s&exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, path, exp_name);
    else
      rsprintf("Location: %s%s?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, path, exp_name);
    }
  else
    rsprintf("Location: %s%s\n\n<html>redir</html>\r\n", mhttpd_url, path);
}

void redirect2(char *path)
{
  redirect(path);
  send_tcp(_sock, return_buffer, strlen(return_buffer)+1, 0);
  closesocket(_sock);
  return_length = -1;
}

/*------------------------------------------------------------------*/

INT search_callback(HNDLE hDB, HNDLE hKey, KEY *key, INT level, void *info)
{
INT         i, size, status;
char        *search_name, *p;
static char data_str[MAX_ODB_PATH];
static char str1[MAX_ODB_PATH], str2[MAX_ODB_PATH], ref[MAX_ODB_PATH];
char        path[MAX_ODB_PATH], data[10000];

  search_name = (char *) info;

  /* convert strings to uppercase */
  for (i=0 ; key->name[i] ; i++)
    str1[i] = toupper(key->name[i]);
  str1[i] = 0;
  for (i=0 ; key->name[i] ; i++)
    str2[i] = toupper(search_name[i]);
  str2[i] = 0;

  if (strstr(str1, str2) != NULL)
    {
    db_get_path(hDB, hKey, str1, MAX_ODB_PATH);
    strcpy(path, str1+1); /* strip leading '/' */
    strcpy(str1, path);
    urlEncode(str1);

    if (key->type == TID_KEY || key->type == TID_LINK)
      {
      /* for keys, don't display data value */
      if (exp_name[0])
        rsprintf("<tr><td bgcolor=#FFD000><a href=\"%s?exp=%s&path=%s\">%s</a></tr>\n", 
               mhttpd_url, exp_name, str1, path);
      else
        rsprintf("<tr><td bgcolor=#FFD000><a href=\"%s%s\">%s</a></tr>\n", 
               mhttpd_url, str1, path);
      }
    else
      {
      /* strip variable name from path */
      p = path+strlen(path)-1;
      while (*p && *p != '/')
        *p-- = 0;
      if (*p == '/')
        *p = 0;

      /* display single value */
      if (key->num_values == 1)
        {
        size = sizeof(data);
        status = db_get_data(hDB, hKey, data, &size, key->type);
        if (status == DB_NO_ACCESS)
          strcpy(data_str, "<no read access>");
        else
          db_sprintf(data_str, data, key->item_size, 0, key->type);

        if (exp_name[0])
          sprintf(ref, "%s%s?cmd=Set&exp=%s", 
                  mhttpd_url, str1, exp_name);
        else
          sprintf(ref, "%s%s?cmd=Set", 
                  mhttpd_url, str1);

        rsprintf("<tr><td bgcolor=#FFFF00>");

        if (exp_name[0])
          rsprintf("<a href=\"%s%s?exp=%s\">%s</a>/%s", mhttpd_url, path, exp_name, path, key->name);
        else
          rsprintf("<a href=\"%s%s\">%s</a>/%s", mhttpd_url, path, path, key->name);

        rsprintf("<td><a href=\"%s\">%s</a></tr>\n", ref, data_str);
        }
      else
        {
        /* display first value */
        rsprintf("<tr><td rowspan=%d bgcolor=#FFFF00>%s\n", key->num_values, path);

        for (i=0 ; i<key->num_values ; i++)
          {
          size = sizeof(data);
          db_get_data(hDB, hKey, data, &size, key->type);
          db_sprintf(data_str, data, key->item_size, i, key->type);

          if (exp_name[0])
            sprintf(ref, "%s%s?cmd=Set&index=%d&exp=%s", 
                    mhttpd_url, str1, i, exp_name);
          else
            sprintf(ref, "%s%s?cmd=Set&index=%d", 
                    mhttpd_url, str1, i);

          if (i>0)
            rsprintf("<tr>");

          rsprintf("<td><a href=\"%s\">[%d] %s</a></tr>\n", ref, i, data_str);
          }
        }
      }
    }

  return SUCCESS;
}

/*------------------------------------------------------------------*/

void show_help_page()
{
  rsprintf("<html><head>\n");
  rsprintf("<title>MIDAS WWW Gateway Help</title>\n");
  rsprintf("</head>\n\n");

  rsprintf("<body>\n");
  rsprintf("<h1>Using the MIDAS WWW Gateway</h1>\n");
  rsprintf("With the MIDAS WWW Gateway basic experiment control can be achieved.\n");
  rsprintf("The status page displays the current run status including front-end\n");
  rsprintf("and logger statistics. The Start and Stop buttons can start and stop\n");
  rsprintf("a run. The ODB button switches into the Online Database mode, where\n");
  rsprintf("the contents of the experiment database can be displayed and modified.<P>\n\n");

  rsprintf("For more information, refer to the\n"); 
  rsprintf("<A HREF=\"http://pibeta.psi.ch/midas/manual/Manual.html\">MIDAS manual</A>.<P>\n\n");

  rsprintf("<hr>\n");
  rsprintf("<address>\n");
  rsprintf("<a href=\"http://pibeta.psi.ch/~stefan\">S. Ritt</a>, 12 Feb 1998");
  rsprintf("</address>");

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_header(HNDLE hDB, char *title, char *path, int colspan)
{
time_t now;
char   str[256];
int    size;

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>%s</title></head>\n", title);
  rsprintf("<body><form method=\"GET\" action=\"%s%s\">\n\n",
            mhttpd_url, path);

  /* title row */

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);
  time(&now);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table border=3 cellpadding=1>\n");
  rsprintf("<tr><th colspan=%d bgcolor=#A0A0FF>MIDAS experiment \"%s\"", colspan, str);
  rsprintf("<th colspan=%d bgcolor=#A0A0FF>%s</tr>\n", colspan, ctime(&now));
}

/*------------------------------------------------------------------*/

void show_error(char *error)
{
  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS error</title></head>\n");
  rsprintf("<body><H1>%s</H1></body></html>\n", error);
}

/*------------------------------------------------------------------*/

void show_status_page(int refresh, char *cookie_wpwd)
{
int    i, j, k, status, size;
BOOL   flag;
char   str[256], name[32], ref[256];
char   *yn[] = {"No", "Yes"};
char   *state[] = {"", "Stopped", "Paused", "Running" };
char   *trans_name[] = {"Start", "Stop", "Pause", "Resume"};
time_t now, difftime;
double analyzed, analyze_ratio, d;
float  value;
HNDLE  hDB, hkey, hLKey, hsubkey, hkeytmp;
KEY    key;
BOOL   ftp_mode, previous_mode;
char   client_name[NAME_LENGTH];
struct tm *gmt;

RUNINFO_STR(runinfo_str);
RUNINFO runinfo;
EQUIPMENT_INFO equipment;
EQUIPMENT_STATS equipment_stats;
CHN_SETTINGS chn_settings;
CHN_STATISTICS chn_stats;

  cm_get_experiment_database(&hDB, NULL);

  /* create/correct /runinfo structure */
  db_create_record(hDB, 0, "/Runinfo", strcomb(runinfo_str));
  db_find_key(hDB, 0, "/Runinfo", &hkey);
  size = sizeof(runinfo);
  db_get_record(hDB, hkey, &runinfo, &size, 0);

  /* header */
  rsprintf("HTTP/1.1 200 OK\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n");
  rsprintf("Pragma: no-cache\r\n");
  rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n");
  if (cookie_wpwd[0])
    {
    time(&now);
    now += 3600*24;
    gmt = gmtime(&now);
    strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

    rsprintf("Set-Cookie: midas_wpwd=%s; path=/; expires=%s\r\n", cookie_wpwd, str);
    }
    
  rsprintf("\r\n<html>\n");

  /* auto refresh */
  rsprintf("<head><meta http-equiv=\"Refresh\" content=\"%02d\">\n", refresh);

  rsprintf("<title>MIDAS status</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s\">\n", mhttpd_url);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table border=3 cellpadding=2>\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);
  time(&now);

  rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);
  rsprintf("<th colspan=3 bgcolor=#A0A0FF>%s</tr>\n", ctime(&now));

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");

  if (runinfo.state == STATE_STOPPED)
    rsprintf("<input type=submit name=cmd value=Start>\n");
  else if (runinfo.state == STATE_PAUSED)
    rsprintf("<input type=submit name=cmd value=Resume>\n");
  else
    rsprintf("<input type=submit name=cmd value=Stop>\n<input type=submit name=cmd value=Pause>\n");

  rsprintf("<input type=submit name=cmd value=ODB>\n");
  rsprintf("<input type=submit name=cmd value=CNAF>\n");
  rsprintf("<input type=submit name=cmd value=Messages>\n");
  rsprintf("<input type=submit name=cmd value=ELog>\n");
  rsprintf("<input type=submit name=cmd value=Alarms>\n");
  rsprintf("<input type=submit name=cmd value=Programs>\n");
  rsprintf("<input type=submit name=cmd value=Config>\n");
  rsprintf("<input type=submit name=cmd value=Help>\n");

  rsprintf("</tr>\n\n");

  /*---- alarms ----*/

  /* go through all triggered alarms */
  db_find_key(hDB, 0, "/Alarms/Alarms", &hkey);
  if (hkey)
    {
    /* check global alarm flag */
    flag = TRUE;
    size = sizeof(flag);
    db_get_value(hDB, 0, "/Alarms/Alarm System active", &flag, &size, TID_BOOL);
    if (flag)
      {
      for (i=0 ; ; i++)
        {
        db_enum_link(hDB, hkey, i, &hsubkey);

        if (!hsubkey)
          break;

        flag = 0;
        size = sizeof(flag);
        db_get_value(hDB, hsubkey, "Triggered", &flag, &size, TID_INT);
        if (flag)
          {
          if (exp_name[0])
            sprintf(ref, "%s?exp=%s&cmd=alarms", mhttpd_url, exp_name);
          else
            sprintf(ref, "%s?cmd=alarms", mhttpd_url);

          rsprintf("<tr><td colspan=6 bgcolor=#FF0000 align=center><h1><a href=\"%s\">Alarm!</a></h1></tr>\n", ref);
          break;
          }
        }
      }
    }

  /*---- aliases ----*/

  db_find_key(hDB, 0, "/Alias", &hkey);
  if (hkey)
    {
    rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkey, i, &hsubkey);
      if (!hsubkey)
        break;

      db_get_key(hDB, hsubkey, &key);

      if (key.type == TID_STRING)
        {
        /* html link */
        size = sizeof(ref);
        db_get_data(hDB, hsubkey, ref, &size, TID_STRING);
        rsprintf("<a href=\"%s\" target=\"_blank\">%s</a> ", ref, key.name);
        }
      else if (key.type == TID_LINK)
        {
        /* odb link */
        if (exp_name[0])
          sprintf(ref, "%sAlias/%s?exp=%s", mhttpd_url, key.name, exp_name);
        else
          sprintf(ref, "%sAlias/%s", mhttpd_url, key.name);

        rsprintf("<a href=\"%s\" target=\"_blank\">%s</a> ", ref, key.name);
        }
      }
    }

  /*---- run info ----*/

  rsprintf("<tr align=center><td>Run #%d", runinfo.run_number);

  if (runinfo.state == STATE_STOPPED)
    strcpy(str, "FF0000");
  else if (runinfo.state == STATE_PAUSED)
    strcpy(str, "FFFF00");
  else if (runinfo.state == STATE_RUNNING)
    strcpy(str, "00FF00");
  else strcpy(str, "FFFFFF");

  rsprintf("<td colspan=1 bgcolor=#%s>%s", str, state[runinfo.state]);

  if (runinfo.requested_transition)
    for (i=0 ; i<4 ; i++)
      if (runinfo.requested_transition & (1<<i))
        rsprintf("<br><b>%s requested</b>", trans_name[i]);

  if (exp_name[0])
    sprintf(ref, "%sAlarms/Alarm system active?cmd=set&exp=%s", mhttpd_url, exp_name);
  else
    sprintf(ref, "%sAlarms/Alarm system active?cmd=set", mhttpd_url);

  size = sizeof(flag);
  db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL);
  strcpy(str, flag ? "00FF00" : "FFC0C0");
  rsprintf("<td bgcolor=#%s><a href=\"%s\">Alarms: %s</a>", str, ref, flag ? "On" : "Off");

  if (exp_name[0])
    sprintf(ref, "%sLogger/Auto restart?cmd=set&exp=%s", mhttpd_url, exp_name);
  else
    sprintf(ref, "%sLogger/Auto restart?cmd=set", mhttpd_url);

  size = sizeof(flag);
  db_get_value(hDB, 0, "/Logger/Auto restart", &flag, &size, TID_BOOL);
  strcpy(str, flag ? "00FF00" : "FFFF00");
  rsprintf("<td bgcolor=#%s><a href=\"%s\">Restart: %s</a>", str, ref, flag ? "Yes" : "No");

  if (cm_exist("Logger", FALSE) != CM_SUCCESS &&
      cm_exist("FAL", FALSE) != CM_SUCCESS)
    rsprintf("<td colspan=2 bgcolor=#FF0000>Logger not running</tr>\n");
  else
    {
    /* write data flag */
    size = sizeof(flag);
    db_get_value(hDB, 0, "/Logger/Write data", &flag, &size, TID_BOOL);

    if (!flag)
      rsprintf("<td colspan=2 bgcolor=#FFFF00>Logging disabled</tr>\n");
    else
      {
      size = sizeof(str);
      db_get_value(hDB, 0, "/Logger/Data dir", str, &size, TID_STRING);

      rsprintf("<td colspan=3>Data dir: %s</tr>\n", str);
      }
    }

  /*---- time ----*/

  rsprintf("<tr align=center><td colspan=3>Start: %s", runinfo.start_time);

  difftime = now - runinfo.start_time_binary;
  if (runinfo.state == STATE_STOPPED)
    rsprintf("<td colspan=3>Stop: %s</tr>\n", runinfo.stop_time);
  else
    rsprintf("<td colspan=3>Running time: %dh%02dm%02ds</tr>\n", 
             difftime/3600, difftime%3600/60,difftime%60);

  /*---- Equipment list ----*/

  rsprintf("<tr><th>Equipment<th>FE Node<th>Events");
  rsprintf("<th>Event rate[/s]<th>Data rate[kB/s]<th>Analyzed</tr>\n");

  if (db_find_key(hDB, 0, "/equipment", &hkey) == DB_SUCCESS)
    {
    for (i=0 ; ; i++)
      {
      db_enum_key(hDB, hkey, i, &hsubkey);
      if (!hsubkey)
        break;

      db_get_key(hDB, hsubkey, &key);

      memset(&equipment, 0, sizeof(equipment));
      memset(&equipment_stats, 0, sizeof(equipment_stats));

      db_find_key(hDB, hsubkey, "Common", &hkeytmp);

      if (hkeytmp)
        {
        db_get_record_size(hDB, hkeytmp, 0, &size);
        /* discard wrong equipments (caused by analyzer) */
        if (size == sizeof(equipment))
          db_get_record(hDB, hkeytmp, &equipment, &size, 0);
        }
      
      db_find_key(hDB, hsubkey, "Statistics", &hkeytmp);

      if (hkeytmp)
        {
        db_get_record_size(hDB, hkeytmp, 0, &size);
        if (size == sizeof(equipment_stats))
          db_get_record(hDB, hkeytmp, &equipment_stats, &size, 0);
        }

      if (exp_name[0])
        sprintf(ref, "%sSC/%s?exp=%s", mhttpd_url, key.name, exp_name);
      else
        sprintf(ref, "%sSC/%s", mhttpd_url, key.name);
      
      /* check if client running this equipment is present */
      if (cm_exist(equipment.frontend_name, TRUE) != CM_SUCCESS)
        rsprintf("<tr><td><a href=\"%s\">%s</a><td align=center bgcolor=#FF0000>(inactive)", 
                  ref, key.name);
      else
        rsprintf("<tr><td><a href=\"%s\">%s</a><td align=center bgcolor=#00FF00>%s@%s", 
                  ref, key.name, equipment.frontend_name, equipment.frontend_host);
      
      /* get analyzed ratio */
      analyze_ratio = 0;
      sprintf(ref, "/Analyzer/%s", key.name);
      db_find_key(hDB, 0, ref, &hkeytmp);
      if (hkeytmp)
        {
        size = sizeof(double);
        if (db_get_value(hDB, hkeytmp, "Statistics/Events received", 
                         &analyzed, &size, TID_DOUBLE) == DB_SUCCESS &&
            equipment_stats.events_sent > 0)
          analyze_ratio = analyzed / equipment_stats.events_sent;
        if (analyze_ratio > 1)
          analyze_ratio = 1;
        }
      
      d = equipment_stats.events_sent;
      if (d > 1E9)
        sprintf(str, "%1.3lfG", d/1E9);
      else if (d > 1E6)
        sprintf(str, "%1.3lfM", d/1E6);
      else
        sprintf(str, "%1.0lf", d);

      /* check if analyze is running */
      if (cm_exist("Analyzer", FALSE) == CM_SUCCESS)
        rsprintf("<td align=center>%s<td align=center>%1.1lf<td align=center>%1.1lf<td align=center bgcolor=#00FF00>%3.1lf%%</tr>\n",
                 str,
                 equipment_stats.events_per_sec, 
                 equipment_stats.kbytes_per_sec,
                 analyze_ratio*100.0); 
      else
        rsprintf("<td align=center>%s<td align=center>%1.1lf<td align=center>%1.1lf<td align=center bgcolor=#FF0000>%3.1lf%%</tr>\n",
                 str,
                 equipment_stats.events_per_sec, 
                 equipment_stats.kbytes_per_sec,
                 analyze_ratio*100.0); 
      }
    }

  /*---- Logging channels ----*/

  rsprintf("<tr><th colspan=2>Channel<th>Active<th>Events<th>MB written<th>GB total</tr>\n");

  if (db_find_key(hDB, 0, "/Logger/Channels", &hkey) == DB_SUCCESS)
    {
    for (i=0 ; ; i++)
      {
      db_enum_key(hDB, hkey, i, &hsubkey);
      if (!hsubkey)
        break;

      db_get_key(hDB, hsubkey, &key);

      db_find_key(hDB, hsubkey, "Settings", &hkeytmp);
      size = sizeof(chn_settings);
      db_get_record(hDB, hkeytmp, &chn_settings, &size, 0);
      
      db_find_key(hDB, hsubkey, "Statistics", &hkeytmp);
      size = sizeof(chn_stats);
      db_get_record(hDB, hkeytmp, &chn_stats, &size, 0);

      /* filename */
      
      if (strchr(chn_settings.filename, '%'))
        sprintf(str, chn_settings.filename, runinfo.run_number);
      else
        strcpy(str, chn_settings.filename);

      if (equal_ustring(chn_settings.type, "FTP"))
        {
        char *token, orig[256];

        strcpy(orig, str);

        strcpy(str, "ftp://");
        token = strtok(orig, ", ");
        strcat(str, token);
        token = strtok(NULL, ", ");
        token = strtok(NULL, ", ");
        token = strtok(NULL, ", ");
        token = strtok(NULL, ", ");
        strcat(str, "/");
        strcat(str, token);
        strcat(str, "/");
        token = strtok(NULL, ", ");
        strcat(str, token);
        }

      if (exp_name[0])
        sprintf(ref, "%sLogger/Channels/%s/Settings?exp=%s", mhttpd_url, key.name, exp_name);
      else
        sprintf(ref, "%sLogger/Channels/%s/Settings", mhttpd_url, key.name);

      rsprintf("<tr><td colspan=2><B><a href=\"%s\">%s</a></B> %s", ref, key.name, str);

      /* active */

      if (cm_exist("Logger", FALSE) != CM_SUCCESS &&
          cm_exist("FAL", FALSE) != CM_SUCCESS)
        rsprintf("<td align=center bgcolor=\"FF0000\">No Logger");
      else if (!flag)
        rsprintf("<td align=center bgcolor=\"FFFF00\">Disabled");
      else if (chn_settings.active)
        rsprintf("<td align=center bgcolor=\"00FF00\">Yes");
      else
        rsprintf("<td align=center bgcolor=\"FF0000\">No");

      /* statistics */

      rsprintf("<td align=center>%1.0lf<td align=center>%1.3lf<td align=center>%1.3lf</tr>\n",
               chn_stats.events_written,
               chn_stats.bytes_written/1024/1024,
               chn_stats.bytes_written_total/1024/1024/1024);
      }
    }

  /*---- Lazy Logger ----*/
  
  if (db_find_key(hDB, 0, "/Lazy", &hkey) == DB_SUCCESS)
    {
    status = db_find_key(hDB, 0, "System/Clients", &hkey);
    if (status != DB_SUCCESS)
      return;

    k = 0;
    previous_mode  = -1;
    /* loop over all clients */
    for (j=0 ; ; j++)
      {
      status = db_enum_key(hDB, hkey, j, &hsubkey);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      if (status == DB_SUCCESS)
        {
        /* get client name */
        size = sizeof(client_name);
        db_get_value(hDB, hsubkey, "Name", client_name, &size, TID_STRING);
        client_name[4] = 0; /* search only for the 4 first char */
        if (equal_ustring(client_name, "Lazy"))
          {
          sprintf(str, "/Lazy/%s", &client_name[5]);  
          status = db_find_key(hDB, 0, str, &hLKey);
          if (status == DB_SUCCESS)
            {
            size = sizeof(str);
            db_get_value(hDB, hLKey, "Settings/Backup Type", str, &size, TID_STRING);
            ftp_mode = equal_ustring(str, "FTP");
            
            if (previous_mode != ftp_mode)
              k = 0;
            if (k == 0)
              {
              if (ftp_mode)
                rsprintf("<tr><th colspan=2>Lazy Destination<th>Progress<th>File Name<th>Speed [kb/s]<th>Total</tr>\n");
              else
                rsprintf("<tr><th colspan=2>Lazy Label<th>Progress<th>File Name<th># Files<th>Total</tr>\n");
              }
            previous_mode = ftp_mode;
            if (ftp_mode)
              {
              size = sizeof(str);
              db_get_value(hDB, hLKey, "Settings/Path", str, &size, TID_STRING);
              if (strchr(str, ','))
                *strchr(str, ',') = 0;
              }
            else
              {
              size = sizeof(str);
              db_get_value(hDB, hLKey, "Settings/List Label", str, &size, TID_STRING);
              if (str[0] == 0)
                strcpy(str, "(empty)");
              }

            if (exp_name[0])
              sprintf(ref, "%sLazy/%s/Settings?exp=%s", mhttpd_url, &client_name[5], exp_name);
            else
              sprintf(ref, "%sLazy/%s/Settings", mhttpd_url, &client_name[5]);

            rsprintf("<tr><td colspan=2><B><a href=\"%s\">%s</a></B>", ref, str);

            size = sizeof(value);
            db_get_value(hDB, hLKey, "Statistics/Copy progress [%]", &value, &size, TID_FLOAT);
            rsprintf("<td align=center>%1.0f %%", value);

            size = sizeof(str);
            db_get_value(hDB, hLKey, "Statistics/Backup File", str, &size, TID_STRING);
            rsprintf("<td align=center>%s", str);

            if (ftp_mode)
              {
              size = sizeof(value);
              db_get_value(hDB, hLKey, "Statistics/Copy Rate [bytes per s]", &value, &size, TID_FLOAT);
              rsprintf("<td align=center>%1.1f", value);
              }
            else
              {
              size = sizeof(i);
              db_get_value(hDB, hLKey, "/Statistics/Number of files", &i, &size, TID_INT);
              rsprintf("<td align=center>%d", i);
              }

            size = sizeof(value);
            db_get_value(hDB, hLKey, "Statistics/Backup status [%]", &value, &size, TID_FLOAT);
            rsprintf("<td align=center>%1.1f %%", value);
            k++;
            }
          }
        }
      }

    rsprintf("</tr>\n");
    }

  /*---- Messages ----*/

  rsprintf("<tr><td colspan=6>");

  if (message_buffer[0])
    rsprintf("<b>%s</b>", message_buffer);

  rsprintf("</tr>");

  /*---- Clients ----*/

  if (db_find_key(hDB, 0, "/System/Clients", &hkey) == DB_SUCCESS)
    {
    for (i=0 ; ; i++)
      {
      db_enum_key(hDB, hkey, i, &hsubkey);
      if (!hsubkey)
        break;

      if (i%3 == 0)
        rsprintf("<tr bgcolor=#FFFF00>"); 

      size = sizeof(name);
      db_get_value(hDB, hsubkey, "Name", name, &size, TID_STRING);
      size = sizeof(str);
      db_get_value(hDB, hsubkey, "Host", str, &size, TID_STRING);

      rsprintf("<td colspan=2 align=center>%s [%s]", name, str);

      if (i%3 == 2)
        rsprintf("</tr>\n");
      }

    if (i%3 != 0)
      rsprintf("</tr>\n");
    }

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_messages_page(int refresh, int more)
{
int  size;
char str[256], buffer[10000], *pline;
time_t now;
HNDLE hDB;

  cm_get_experiment_database(&hDB, NULL);

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head>\n");

  /* auto refresh */
  if (refresh > 0)
    rsprintf("<meta http-equiv=\"Refresh\" content=\"%d\">\n\n", refresh);

  rsprintf("<title>MIDAS messages</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s\">\n", mhttpd_url);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table columns=2 border=3 cellpadding=2>\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);
  time(&now);

  rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);
  rsprintf("<th bgcolor=#A0A0FF>%s</tr>\n", ctime(&now));

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=ODB>\n");
  rsprintf("<input type=submit name=cmd value=Status>\n");
  rsprintf("<input type=submit name=cmd value=Config>\n");
  rsprintf("<input type=submit name=cmd value=Help>\n");
  rsprintf("</tr>\n\n");

  /*---- messages ----*/

  rsprintf("<tr><td colspan=2>\n");

  /* more button */
  if (!more)
    rsprintf("<input type=submit name=cmd value=More><p>\n");

  if (more)
    size = sizeof(buffer);
  else
    size = 1000;

  cm_msg_retrieve(buffer, &size);
  pline = buffer;

  do
    {
    /* extract single line */
    if (strchr(pline, '\n'))
      {
      *strchr(pline, '\n') = 0;
      }
    else
      break;

    if (strchr(pline, '\r'))
      *strchr(pline, '\r') = 0;

    rsprintf("%s<br>\n", pline);
    pline += strlen(pline);
    while (! *pline) pline++;

    } while (1);

  rsprintf("</tr></table>\n");
  rsprintf("</body></html>\r\n");
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

void el_format(char *text, char *encoding)
{
  if (equal_ustring(encoding, "HTML"))
    rsprintf(text);
  else
    strencode(text);
}

void show_elog_new(char *path, BOOL bedit)
{
int    i, size, run_number, wrap;
char   str[256], ref[256], *p;
char   date[80], author[80], type[80], system[80], subject[256], text[10000], 
       orig_tag[80], reply_tag[80], att1[256], att2[256], att3[256], encoding[80];
time_t now;
HNDLE  hDB, hkey;

  cm_get_experiment_database(&hDB, NULL);

  /* get message for reply */
  type[0] = system[0] = 0;
  att1[0] = att2[0] = att3[0] = 0;
  subject[0] = 0;

  if (path)
    {
    strcpy(str, path);
    size = sizeof(text);
    el_retrieve(str, date, &run_number, author, type, system, subject, 
                text, &size, orig_tag, reply_tag, att1, att2, att3, encoding);
    }

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS ELog</title></head>\n");
  rsprintf("<body><form method=\"POST\" action=\"%s\" enctype=\"multipart/form-data\">\n", mhttpd_url);
//  rsprintf("<body><form method=\"GET\" action=\"%sEL/\">\n", mhttpd_url);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table border=3 cellpadding=5>\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=Submit>\n");
  rsprintf("</tr>\n\n");

  /*---- entry form ----*/

  if (bedit)
    {
    rsprintf("<tr><td bgcolor=#FFFF00>Entry date: %s<br>", date);
    time(&now);                 
    rsprintf("Revision date: %s", ctime(&now));
    }
  else
    {
    time(&now);
    rsprintf("<tr><td bgcolor=#FFFF00>Entry date: %s", ctime(&now));
    }

  if (!bedit)
    {
    size = sizeof(run_number);
    db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT);
    }
  rsprintf("<td bgcolor=#FFFF00>Run number: ");
  rsprintf("<input type=\"text\" size=10 maxlength=10 name=\"run\" value=\"%d\"</tr>", run_number);

  if (bedit)
    {
    strcpy(str, author);
    if (strchr(str, '@'))
      *strchr(str, '@') = 0;
    }
  else
    str[0] = 0;

  rsprintf("<tr><td bgcolor=#FFA0A0>Author: <input type=\"text\" size=\"15\" maxlength=\"80\" name=\"Author\" value=\"%s\">\n", str);

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

  if (exp_name[0])
    sprintf(ref, "%sELog/?exp=%s", mhttpd_url, exp_name);
  else
    sprintf(ref, "%sELog/", mhttpd_url);

  rsprintf("<td bgcolor=#FFA0A0><a href=\"%s\" target=\"_blank\">Type:</a> <select name=\"type\">\n", ref);
  for (i=0 ; i<20 && type_list[i][0] ; i++)
    if ((path && !bedit && equal_ustring(type_list[i], "reply")) ||
        (bedit && equal_ustring(type_list[i], type)))
      rsprintf("<option selected value=\"%s\">%s\n", type_list[i], type_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
  rsprintf("</select></tr>\n");

  rsprintf("<tr><td bgcolor=#A0FFA0><a href=\"%s\" target=\"_blank\">  System:</a> <select name=\"system\">\n", ref);
  for (i=0 ; i<20 && system_list[i][0] ; i++)
    if (path && equal_ustring(system_list[i], system))
      rsprintf("<option selected value=\"%s\">%s\n", system_list[i], system_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", system_list[i], system_list[i]);
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
      rsprintf(text);
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
  rsprintf("<tr><td colspan=2>Attachment1: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile1\" value=\"%s\" accept=\"filetype/*\"></tr>\n", att1);
  rsprintf("<tr><td colspan=2>Attachment2: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile2\" value=\"%s\" accept=\"filetype/*\"></tr>\n", att2);
  rsprintf("<tr><td colspan=2>Attachment3: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile3\" value=\"%s\" accept=\"filetype/*\"></tr>\n", att3);

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_query()
{
int    i, size;
char   str[256];
time_t now;
struct tm *tms;
HNDLE  hDB, hkey, hkeyroot;
KEY    key;

  cm_get_experiment_database(&hDB, NULL);

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS ELog</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%sEL/\">\n", mhttpd_url);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table border=3 cellpadding=5>\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  rsprintf("<tr><th colspan=2 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th colspan=2 bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

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

  rsprintf("<tr><td bgcolor=#A0FFFF>Start run: ");
  rsprintf("<td bgcolor=#A0FFFF><input type=\"text\" size=\"10\" maxlength=\"10\" name=\"r1\">\n");
  rsprintf("<td bgcolor=#A0FFFF>End run: ");
  rsprintf("<td bgcolor=#A0FFFF><input type=\"text\" size=\"10\" maxlength=\"10\" name=\"r2\">\n");
  rsprintf("</tr>\n");

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

  rsprintf("<tr><td colspan=2 bgcolor=#FFA0A0>Author: ");
  rsprintf("<input type=\"test\" size=\"15\" maxlength=\"80\" name=\"author\">\n");

  rsprintf("<td colspan=2 bgcolor=#FFA0A0>Type: ");
  rsprintf("<select name=\"type\">\n");
  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<20 && type_list[i][0] ; i++)
    rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
  /* add forms as types */
  db_find_key(hDB, 0, "/Elog/Forms", &hkeyroot);
  if (hkeyroot)
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyroot, i, &hkey);
      if (!hkey)
        break;
      db_get_key(hDB, hkey, &key);
      rsprintf("<option value=\"%s\">%s\n", key.name, key.name);
      }
  rsprintf("</select></tr>\n");

  rsprintf("<tr><td colspan=2 bgcolor=#A0FFA0>System: ");
  rsprintf("<select name=\"system\">\n");
  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<20 && system_list[i][0] ; i++)
    rsprintf("<option value=\"%s\">%s\n", system_list[i], system_list[i]);
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

void show_elog_submit_query(INT last_n)
{
int    i, size, run, status, m1, d2, m2, y2, index, fh;
char   date[80], author[80], type[80], system[80], subject[256], text[10000], 
       orig_tag[80], reply_tag[80], attachment[3][256], encoding[80];
char   str[256], str2[10000], tag[256], ref[80], file_name[256];
HNDLE  hDB;
BOOL   full, show_attachments;
DWORD  ltime_end, ltime_current, now;
struct tm tms, *ptms;
FILE   *f;

  cm_get_experiment_database(&hDB, NULL);

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS ELog</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%sEL/\">\n", mhttpd_url);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table border=3 cellpadding=2 width=\"100%%\">\n");

  /* get mode */
  if (last_n)
    {
    full = TRUE;
    show_attachments = FALSE;
    }
  else
    {
    full = !(*getparam("mode"));
    show_attachments = (*getparam("attach") > 0);
    }
  
  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th colspan=%d bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", full?3:4, str);

  /*---- menu buttons ----*/

  if (!full)
    {
    rsprintf("<tr><td colspan=7 bgcolor=#C0C0C0>\n");

    rsprintf("<input type=submit name=cmd value=\"Query\">\n");
    rsprintf("<input type=submit name=cmd value=\"ELog\">\n");
    rsprintf("<input type=submit name=cmd value=\"Status\">\n");
    rsprintf("</tr>\n\n");
    }

  /*---- convert end date to ltime ----*/

  ltime_end = 0;

  if (!last_n)
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

  if (*getparam("r1"))
    {
    if (*getparam("r2"))
      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between runs %s and %s</b></tr>\n",
                full?6:7, getparam("r1"), getparam("r2"));
    else
      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between run %s and today</b></tr>\n",
                full?6:7, getparam("r1"));
    }
  else 
    {
    if (last_n)
      rsprintf("<tr><td colspan=6 bgcolor=#FFFF00><b>Last ten messages</b></tr>\n");

    else if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between %s %s %s and %s %d %d</b></tr>\n",
                full?6:7, getparam("m1"), getparam("d1"), getparam("y1"),
                mname[m2], d2, y2);
    else
      {
      time(&now);
      ptms = localtime(&now);
      ptms->tm_year += 1900;

      rsprintf("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between %s %s %s and %s %d %d</b></tr>\n",
                full?6:7, getparam("m1"), getparam("d1"), getparam("y1"),
                mname[ptms->tm_mon], ptms->tm_mday, ptms->tm_year);
      }
    }

  rsprintf("</tr>\n<tr>");

  rsprintf("<td colspan=%d bgcolor=#FFA0A0>\n", full?6:7);

  if (*getparam("author"))
    rsprintf("Author: <b>%s</b>   ", getparam("author"));

  if (*getparam("type"))
    rsprintf("Type: <b>%s</b>   ", getparam("type"));

  if (*getparam("system"))
    rsprintf("System: <b>%s</b>   ", getparam("system"));

  if (*getparam("subject"))
    rsprintf("Subject: <b>%s</b>   ", getparam("subject"));

  if (*getparam("subtext"))
    rsprintf("Text: <b>%s</b>   ", getparam("subtext"));

  rsprintf("</tr>\n");

  /*---- table titles ----*/

  if (full)
    rsprintf("<tr><th>Date<th>Run<th>Author<th>Type<th>System<th>Subject</tr>\n");
  else
    rsprintf("<tr><th>Date<th>Run<th>Author<th>Type<th>System<th>Subject<th>Text</tr>\n");

  /*---- do query ----*/

  if (last_n)
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
    sprintf(tag, "%02d%02d%02d.0", atoi(getparam("y1")) % 100, m1+1, atoi(getparam("d1")));
    }

  do
    {
    size = sizeof(text);
    status = el_retrieve(tag, date, &run, author, type, system, subject, 
                         text, &size, orig_tag, reply_tag, 
                         attachment[0], attachment[1], attachment[2],
                         encoding);
    strcat(tag, "+1");

    /* check for end run */
    if (*getparam("r2") && atoi(getparam("r2")) < run)
      break;

    /* check for end date */
    if (ltime_end > 0)
      {
      /* extract time structure from tag */
      memset(&tms, 0, sizeof(struct tm));
      tms.tm_year = (tag[0]-'0')*10 + (tag[1]-'0');
      tms.tm_mon  = (tag[2]-'0')*10 + (tag[3]-'0') -1;
      tms.tm_mday = (tag[4]-'0')*10 + (tag[5]-'0');
      tms.tm_hour = 12;

      if (tms.tm_year < 90)
        tms.tm_year += 100;
      ltime_current = mktime(&tms);

      if (ltime_current > ltime_end)
        break;
      }

    if (status == EL_SUCCESS)
      {
      /* do filtering */
      if (*getparam("type") && !equal_ustring(getparam("type"), type))
        continue;
      if (*getparam("system") && !equal_ustring(getparam("system"), system))
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
      if (exp_name[0])
        sprintf(ref, "%sEL/%s?exp=%s", mhttpd_url, str, exp_name);
      else
        sprintf(ref, "%sEL/%s", mhttpd_url, str);

      strncpy(str, text, 80);
      str[80] = 0;

      if (full)
        {
        rsprintf("<tr><td><a href=%s>%s</a><td>%d<td>%s<td>%s<td>%s<td>%s</tr>\n", ref, date, run, author, type, system, subject);
        rsprintf("<tr><td colspan=6>");
      
        if (equal_ustring(encoding, "plain"))
          {
          rsputs("<pre>");
          rsputs(text);
          rsputs("</pre>");
          }
        else
          rsputs(text);

        rsprintf("</tr>\n");

        if (show_attachments)
          {
          for (index = 0 ; index < 3 ; index++)
            {
            if (attachment[index][0])
              {
              for (i=0 ; i<(int)strlen(attachment[index]) ; i++)
                str[i] = toupper(attachment[index][i]);
              str[i] = 0;
      
              if (exp_name[0])
                sprintf(ref, "%sEL/%s?exp=%s", 
                        mhttpd_url, attachment[index], exp_name);
              else
                sprintf(ref, "%sEL/%s", 
                        mhttpd_url, attachment[index]);

              if (strstr(str, ".GIF") ||
                  strstr(str, ".JPG"))
                {
                rsprintf("<tr><td colspan=6>Attachment: <a href=\"%s\"><b>%s</b></a><br>\n", 
                          ref, attachment[index]+14);
                rsprintf("<img src=%s></tr>", ref);
                }
              else
                {
                rsprintf("<tr><td colspan=6 bgcolor=#C0C0FF>Attachment: <a href=\"%s\"><b>%s</b></a>\n", 
                          ref, attachment[index]+14);
                if (!strstr(str, ".PS") &&
                    !strstr(str, ".EPS"))
                  {
                  /* display attachment */
                  rsprintf("<br><pre>");

                  file_name[0] = 0;
                  size = sizeof(file_name);
                  memset(file_name, 0, size);
                  db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size, TID_STRING);
                  if (file_name[0] != 0)
                    if (file_name[strlen(file_name)-1] != DIR_SEPARATOR)
                      strcat(file_name, DIR_SEPARATOR_STR);
                  strcat(file_name, attachment[index]);

                  f = fopen(file_name, "rt");
                  if (f != NULL)
                    {
                    while (!feof(f))
                      {
                      str[0] = 0;
                      fgets(str, sizeof(str), f);
                      rsputs(str);
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
        }
      else
        {
        rsprintf("<tr><td>%s<td>%d<td>%s<td>%s<td>%s<td>%s\n", date, run, author, type, system, subject);

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
int    size;
FILE   *f;
char   file_name[256], str[100], line[1000];
HNDLE  hDB;

  cm_get_experiment_database(&hDB, NULL);

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS File Display</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%sEL/\">\n", mhttpd_url);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table border=3 cellpadding=1 width=\"100%%\">\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS File Display");
  rsprintf("<th bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=\"ELog\">\n");
  rsprintf("<input type=submit name=cmd value=\"Status\">\n");
  rsprintf("</tr></table>\n\n");

  /*---- open file ----*/

  cm_get_experiment_database(&hDB, NULL);
  file_name[0] = 0;
  if (hDB > 0)
    {
    size = sizeof(file_name);
    memset(file_name, 0, size);
    db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size, TID_STRING);
    if (file_name[0] != 0)
      if (file_name[strlen(file_name)-1] != DIR_SEPARATOR)
        strcat(file_name, DIR_SEPARATOR_STR);
    }
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
    rsputs(line);
    }
  rsprintf("</pre>\n");
  fclose(f);

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_form_query()
{
int    i, size, run_number;
char   str[256];
time_t now;
HNDLE  hDB, hkey, hkeyroot;
KEY    key;

  cm_get_experiment_database(&hDB, NULL);

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS ELog</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%sEL/\">\n", mhttpd_url);

  if (*getparam("form") == 0)
    return;

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  /* hidden field for form */
  rsprintf("<input type=hidden name=form value=\"%s\">\n", getparam("form"));

  rsprintf("<table border=3 cellpadding=1>\n");

  /*---- title row ----*/

  rsprintf("<tr><th colspan=2 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th colspan=2 bgcolor=#A0A0FF>Form \"%s\"</tr>\n", getparam("form"));

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=4 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=\"Submit\">\n");
  rsprintf("<input type=reset value=\"Reset Form\">\n");
  rsprintf("</tr>\n\n");

  /*---- entry form ----*/

  time(&now);
  rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: %s", ctime(&now));

  size = sizeof(run_number);
  db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT);
  rsprintf("<td bgcolor=#FFFF00>Run number: ");
  rsprintf("<input type=\"text\" size=10 maxlength=10 name=\"run\" value=\"%d\"</tr>", run_number);

  rsprintf("<tr><td colspan=2 bgcolor=#FFA0A0>Author: <input type=\"text\" size=\"15\" maxlength=\"80\" name=\"Author\">\n");

  rsprintf("<tr><th bgcolor=#A0FFA0>Item<th bgcolor=#FFFF00>Checked<th bgcolor=#A0A0FF colspan=2>Comment</tr>\n");

  sprintf(str, "/Elog/Forms/%s", getparam("form"));
  db_find_key(hDB, 0, str, &hkeyroot);
  if (hkeyroot)
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyroot, i, &hkey);
      if (!hkey)
        break;

      db_get_key(hDB, hkey, &key);

      rsprintf("<tr><td bgcolor=#A0FFA0>%d <b>%s</b>", i+1, key.name);
      rsprintf("<td bgcolor=#FFFF00 align=center><input type=checkbox name=x%d value=1>", i);
      rsprintf("<td bgcolor=#A0A0FF colspan=2><input type=text size=30 maxlength=255 name=c%d></tr>\n", i);
      }


  /*---- menu buttons at bottom ----*/

  if (i>10)
    {
    rsprintf("<tr><td colspan=4 bgcolor=#C0C0C0>\n");

    rsprintf("<input type=submit name=cmd value=\"Submit\">\n");
    rsprintf("</tr>\n\n");
    }
    
  rsprintf("</tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void submit_elog()
{
char str[80], author[256];

  /* check for author */
  if (*getparam("author") == 0)
    {
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
    rsprintf("Content-Type: text/html\r\n\r\n");

    rsprintf("<html><head><title>ELog Error</title></head>\n");
    rsprintf("<i>Error: No author supplied.</i><p>\n");
    rsprintf("Please go back and enter your name in the <i>author</i> field.\n");
    rsprintf("<body></body></html>\n");
    return;
    }

  /* add remote host name to author */
  strcpy(author, getparam("author"));
  strcat(author, "@");
  strcat(author, remote_host_name);

  str[0] = 0;
  if (*getparam("edit"))
    strcpy(str, getparam("orig"));

  el_submit(atoi(getparam("run")), author, getparam("type"),
            getparam("system"), getparam("subject"), getparam("text"), 
            getparam("orig"), *getparam("html") ? "HTML" : "plain", 
            getparam("attachment0"), _attachment_buffer[0], _attachment_size[0], 
            getparam("attachment1"), _attachment_buffer[1], _attachment_size[1], 
            getparam("attachment2"), _attachment_buffer[2], _attachment_size[2], 
            str, sizeof(str));

  rsprintf("HTTP/1.0 302 Found\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

  if (*getparam("edit"))
    {
    if (exp_name[0])
      rsprintf("Location: %sEL/%s?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, str, exp_name);
    else
      rsprintf("Location: %sEL/%s\n\n<html>redir</html>\r\n", mhttpd_url, str);
    }
  else
    {
    if (exp_name[0])
      rsprintf("Location: %sEL/?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, exp_name);
    else
      rsprintf("Location: %sEL/%s\n\n<html>redir</html>\r\n", mhttpd_url, str);
    }

}

/*------------------------------------------------------------------*/

void submit_form()
{
char  str[256];
char  text[10000];
int   i;
HNDLE hDB, hkey, hkeyroot;
KEY   key;

  /* check for author */
  if (*getparam("author") == 0)
    {
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
    rsprintf("Content-Type: text/html\r\n\r\n");

    rsprintf("<html><head><title>ELog Error</title></head>\n");
    rsprintf("<i>Error: No author supplied.</i><p>\n");
    rsprintf("Please go back and enter your name in the <i>author</i> field.\n");
    rsprintf("<body></body></html>\n");
    return;
    }

  /* assemble text from form */
  cm_get_experiment_database(&hDB, NULL);
  sprintf(str, "/Elog/Forms/%s", getparam("form"));
  db_find_key(hDB, 0, str, &hkeyroot);
  text[0] = 0;
  if (hkeyroot)
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyroot, i, &hkey);
      if (!hkey)
        break;

      db_get_key(hDB, hkey, &key);

      sprintf(str, "x%d", i);
      sprintf(text+strlen(text), "%d %s : [%c]  ", i+1, key.name, *getparam(str) == '1' ? 'X':' ');
      sprintf(str, "c%d", i);
      sprintf(text+strlen(text), "%s\n", getparam(str));
      }
  
  str[0] = 0;
  el_submit(atoi(getparam("run")), getparam("author"), getparam("form"),
            "General", "", text, "", "plain", "", NULL, 0, "", NULL, 0, "", NULL, 0, str, sizeof(str));

  rsprintf("HTTP/1.0 302 Found\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

  if (exp_name[0])
    rsprintf("Location: %sEL/?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, exp_name);
  else
    rsprintf("Location: %sEL/\n\n<html>redir</html>\r\n", mhttpd_url);
}

/*------------------------------------------------------------------*/

void show_elog_page(char *path)
{
int   size, i, run, msg_status, status, fh, length, first_message, last_message, index;
char  str[256], orig_path[256], command[80], ref[256], file_name[256];
char  date[80], author[80], type[80], system[80], subject[256], text[10000], 
      orig_tag[80], reply_tag[80], attachment[3][256], encoding[80];
HNDLE hDB, hkey, hkeyroot;
KEY   key;
FILE  *f;

  cm_get_experiment_database(&hDB, NULL);

  /*---- interprete commands ---------------------------------------*/

  strcpy(command, getparam("cmd"));

  if (*getparam("form"))
    {
    if (*getparam("type"))
      {
      sprintf(str, "EL/?form=%s", getparam("form"));
      redirect(str);
      return;
      }
    if (equal_ustring(command, "submit"))
      submit_form();
    else
      show_form_query();
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

  if (equal_ustring(command, "query"))
    {
    show_elog_query();
    return;
    }

  if (equal_ustring(command, "submit query"))
    {
    show_elog_submit_query(0);
    return;
    }

  if (equal_ustring(command, "last 10 entries"))
    {
    redirect("EL/last10");
    return;
    }

  if (strncmp(path, "last", 4) == 0)
    {
    show_elog_submit_query(atoi(path+4));
    return;
    }

  if (equal_ustring(command, "runlog"))
    {
    sprintf(str, "EL/runlog");
    redirect(str);
    return;
    }

  /*---- check if file requested -----------------------------------*/

  if (strlen(path) > 13 && path[6] == '_' && path[13] == '_')
    {
    cm_get_experiment_database(&hDB, NULL);
    file_name[0] = 0;
    if (hDB > 0)
      {
      size = sizeof(file_name);
      memset(file_name, 0, size);
      db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size, TID_STRING);
      if (file_name[0] != 0)
        if (file_name[strlen(file_name)-1] != DIR_SEPARATOR)
          strcat(file_name, DIR_SEPARATOR_STR);
      }
    strcat(file_name, path);

    fh = open(file_name, O_RDONLY | O_BINARY);
    if (fh > 0)
      {
      lseek(fh, 0, SEEK_END);
      length = TELL(fh);
      lseek(fh, 0, SEEK_SET);

      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

      /* return proper header for file type */
      for (i=0 ; i<(int)strlen(path) ; i++)
        str[i] = toupper(path[i]);
      if (strstr(str, ".JPG"))
        rsprintf("Content-Type: image/jpeg\r\n");
      else if (strstr(str, ".GIF"))
        rsprintf("Content-Type: image/gif\r\n");
      else if (strstr(str, ".PS"))
        rsprintf("Content-Type: application/postscript\r\n");
      else if (strstr(str, ".EPS"))
        rsprintf("Content-Type: application/postscript\r\n");
      else
        rsprintf("Content-Type: text/plain\r\n");
      
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
      el_retrieve(path, date, &run, author, type, system, subject, 
                  text, &size, orig_tag, reply_tag, 
                  attachment[0], attachment[1], attachment[2], 
                  encoding);
      
      if (strchr(author, '@'))
        *strchr(author, '@') = 0;
      if (*getparam("lauthor")  == '1' && !equal_ustring(getparam("author"),  author ))
        continue;
      if (*getparam("ltype")    == '1' && !equal_ustring(getparam("type"),    type   ))
        continue;
      if (*getparam("lsystem")  == '1' && !equal_ustring(getparam("system"),  system ))
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
      
      sprintf(str, "EL/%s", path);

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

      if (*getparam("lsystem") == '1')
        if (strchr(str, '?') == NULL)
          strcat(str, "?lsystem=1");
        else
          strcat(str, "&lsystem=1");

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
  msg_status = el_retrieve(str, date, &run, author, type, system, subject, 
                           text, &size, orig_tag, reply_tag, 
                           attachment[0], attachment[1], attachment[2],
                           encoding);

  /*---- header ----*/

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS ELog</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%sEL/%s\">\n", mhttpd_url, str);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table cols=2 border=2 cellpadding=2>\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");
  rsprintf("<input type=submit name=cmd value=New>\n");
  rsprintf("<input type=submit name=cmd value=Edit>\n");
  rsprintf("<input type=submit name=cmd value=Reply>\n");
  rsprintf("<input type=submit name=cmd value=Query>\n");
  rsprintf("<input type=submit name=cmd value=\"Last 10 entries\">\n");

  /* check forms from ODB */
  db_find_key(hDB, 0, "/Elog/Forms", &hkeyroot);
  if (hkeyroot)
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyroot, i, &hkey);
      if (!hkey)
        break;

      db_get_key(hDB, hkey, &key);

      rsprintf("<input type=submit name=form value=\"%s\">\n", key.name);
      }

  rsprintf("<input type=submit name=cmd value=Runlog>\n");
  rsprintf("<input type=submit name=cmd value=Status>\n");
  rsprintf("</tr>\n");

  rsprintf("<tr><td colspan=2 bgcolor=#E0E0E0>");
  rsprintf("<input type=submit name=cmd value=Next>\n");
  rsprintf("<input type=submit name=cmd value=Previous>\n");
  rsprintf("<input type=submit name=cmd value=Last>\n");
  rsprintf("<i>Check a category to browse only entries from that category</i>\n");
  rsprintf("</tr>\n\n");

  if (msg_status != EL_FILE_ERROR && (reply_tag[0] || orig_tag[0]))
    {
    rsprintf("<tr><td colspan=2 bgcolor=#F0F0F0>");
    if (orig_tag[0])
      {
      if (exp_name[0])
        sprintf(ref, "%sEL/%s?exp=%s", 
                mhttpd_url, orig_tag, exp_name);
      else
        sprintf(ref, "%sEL/%s", 
                mhttpd_url, orig_tag);
      rsprintf("  <a href=\"%s\">Original message</a>  ", ref);
      }
    if (reply_tag[0])
      {
      if (exp_name[0])
        sprintf(ref, "%sEL/%s?exp=%s", 
                mhttpd_url, reply_tag, exp_name);
      else
        sprintf(ref, "%sEL/%s", 
                mhttpd_url, reply_tag);
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


    rsprintf("<tr><td bgcolor=#FFFF00>Entry date: <b>%s</b>", date);

    rsprintf("<td bgcolor=#FFFF00>Run number: <b>%d</b></tr>\n\n", run);

    /* define hidded fields */
    rsprintf("<input type=hidden name=author  value=\"%s\">\n", author); 
    rsprintf("<input type=hidden name=type    value=\"%s\">\n", type); 
    rsprintf("<input type=hidden name=system  value=\"%s\">\n", system); 
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

    if (*getparam("lsystem") == '1')
      rsprintf("<tr><td bgcolor=#A0FFA0><input type=\"checkbox\" checked name=\"lsystem\" value=\"1\">");
    else
      rsprintf("<tr><td bgcolor=#A0FFA0><input type=\"checkbox\" name=\"lsystem\" value=\"1\">");

    rsprintf("  System: <b>%s</b>\n", system);

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
      rsputs(text);
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
          str[i] = toupper(attachment[index][i]);
        str[i] = 0;
      
        if (exp_name[0])
          sprintf(ref, "%sEL/%s?exp=%s", 
                  mhttpd_url, attachment[index], exp_name);
        else
          sprintf(ref, "%sEL/%s", 
                  mhttpd_url, attachment[index]);

        if (strstr(str, ".GIF") ||
            strstr(str, ".JPG"))
          {
          rsprintf("<tr><td colspan=2>Attachment: <a href=\"%s\"><b>%s</b></a><br>\n", 
                    ref, attachment[index]+14);
          rsprintf("<img src=%s></tr>", ref);
          }
        else
          {
          rsprintf("<tr><td colspan=2 bgcolor=#C0C0FF>Attachment: <a href=\"%s\"><b>%s</b></a>\n", 
                    ref, attachment[index]+14);
          if (!strstr(str, ".PS") &&
              !strstr(str, ".EPS"))
            {
            /* display attachment */
            rsprintf("<br><pre>");

            file_name[0] = 0;
            size = sizeof(file_name);
            memset(file_name, 0, size);
            db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size, TID_STRING);
            if (file_name[0] != 0)
              if (file_name[strlen(file_name)-1] != DIR_SEPARATOR)
                strcat(file_name, DIR_SEPARATOR_STR);
            strcat(file_name, attachment[index]);

            f = fopen(file_name, "rt");
            if (f != NULL)
              {
              while (!feof(f))
                {
                str[0] = 0;
                fgets(str, sizeof(str), f);
                rsputs(str);
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

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_sc_page(char *path)
{
int    i, j, k, colspan, size;
char   str[256], eq_name[32], group[32], name[32], ref[256];
char   group_name[MAX_GROUPS][32], data[256];
HNDLE  hDB, hkey, hkeyeq, hkeyset, hkeynames, hkeyvar, hkeyroot;
KEY    eqkey, key, varkey;
char   data_str[256], hex_str[256];

  cm_get_experiment_database(&hDB, NULL);

  /* split path into equipment and group */
  strcpy(eq_name, path);
  strcpy(group, "All");
  if (strchr(eq_name, '/'))
    {
    strcpy(group, strchr(eq_name, '/')+1);
    *strchr(eq_name, '/') = 0;
    }

  /* check for "names" in settings */
  if (eq_name[0])
    {
    sprintf(str, "/Equipment/%s/Settings", eq_name);
    db_find_key(hDB, 0, str, &hkeyset);
    if (hkeyset)
      {
      for (i=0 ; ; i++)
        {
        db_enum_link(hDB, hkeyset, i, &hkeynames);

        if (!hkeynames)
          break;

        db_get_key(hDB, hkeynames, &key);

        if (strncmp(key.name, "Names", 5) == 0)
          break;
        }

      }

    /* redirect if no names found */
    if (!hkeyset || !hkeynames)
      {
      /* redirect */
      sprintf(str, "Equipment/%s/Variables", eq_name);
      redirect(str);
      }
    }

  show_header(hDB, "MIDAS slow control", "", 3);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=ODB>\n");
  rsprintf("<input type=submit name=cmd value=Status>\n");
  rsprintf("<input type=submit name=cmd value=Help>\n");
  rsprintf("</tr>\n\n");

  /*---- enumerate SC equipment ----*/

  rsprintf("<tr><td colspan=6 bgcolor=#FFFF00><i>Equipment:</i> \n");

  db_find_key(hDB, 0, "/Equipment", &hkey);
  if (hkey)
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkey, i, &hkeyeq);

      if (!hkeyeq)
        break;

      db_get_key(hDB, hkeyeq, &eqkey);

      db_find_key(hDB, hkeyeq, "Settings", &hkeyset);
      if (hkeyset)
        {
        for (j=0 ; ; j++)
          {
          db_enum_link(hDB, hkeyset, j, &hkeynames);

          if (!hkeynames)
            break;

          db_get_key(hDB, hkeynames, &key);

          if (strncmp(key.name, "Names", 5) == 0)
            {
            if (equal_ustring(eq_name, eqkey.name))
              rsprintf("<b>%s</b> ", eqkey.name);
            else
              {
              if (exp_name[0])
                rsprintf("<a href=\"%sSC/%s?exp=%s\">%s</a> ", 
                          mhttpd_url, eqkey.name, exp_name, eqkey.name);
              else
                rsprintf("<a href=\"%sSC/%s?\">%s</a> ", 
                          mhttpd_url, eqkey.name, eqkey.name);
              }
            break;
            }
          }
        }
      }
  rsprintf("</tr>\n");

  if (!eq_name[0])
    {
    rsprintf("</table>");
    return;
    }

  /*---- display SC ----*/

  sprintf(str, "/Equipment/%s/Settings/Names", eq_name);
  db_find_key(hDB, 0, str, &hkey);

  if (hkey)
    {
    /*---- single name array ----*/
    rsprintf("<tr><td colspan=6 bgcolor=#FFFFA0><i>Groups:</i> ");

    /* "all" group */
    if (equal_ustring(group, "All"))
      rsprintf("<b>All</b> ");
    else
      {
      if (exp_name[0])
        rsprintf("<a href=\"%sSC/%s/All?exp=%s\">All</a> ", 
                  mhttpd_url, eq_name, exp_name);
      else
        rsprintf("<a href=\"%sSC/%s/All?\">All</a> ", 
                  mhttpd_url, eq_name);
      }

    /* collect groups */

    memset(group_name, 0, sizeof(group_name));
    db_get_key(hDB, hkey, &key);

    for (i=0 ; i<key.num_values ; i++)
      {
      size = sizeof(str);
      db_get_data_index(hDB, hkey, str, &size, i, TID_STRING);

      if (strchr(str, '%'))
        {
        *strchr(str, '%') = 0;
        for (j=0 ; j<MAX_GROUPS ; j++)
          {
          if (equal_ustring(group_name[j], str) ||
              group_name[j][0] == 0)
            break;
          }
        if (group_name[j][0] == 0)
          strcpy(group_name[j], str);
        }
      }

    for (i=0 ; i<MAX_GROUPS && group_name[i][0]; i++)
      {
      if (equal_ustring(group_name[i], group))
        rsprintf("<b>%s</b> ", group_name[i]);
      else
        {
        if (exp_name[0])
          rsprintf("<a href=\"%sSC/%s/%s?exp=%s\">%s</a> ", 
                    mhttpd_url, eq_name, group_name[i], exp_name, group_name[i]);
        else
          rsprintf("<a href=\"%sSC/%s/%s?\">%s</a> ", 
                    mhttpd_url, eq_name, group_name[i], group_name[i]);
        }
      }
    rsprintf("</tr>\n");

    /* count variables */
    sprintf(str, "/Equipment/%s/Variables", eq_name);
    db_find_key(hDB, 0, str, &hkeyvar);
    if (!hkeyvar)
      {
      rsprintf("</table>");
      return;
      }
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyvar, i, &hkey);
      if (!hkey)
        break;
      }

    if (i == 0 || i > 3)
      {
      rsprintf("</table>");
      return;
      }

    /* title row */
    colspan = 6-i;
    rsprintf("<tr><th colspan=%d>Names", colspan);

    /* display entries for this group */
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyvar, i, &hkey);

      if (!hkey)
        break;

      db_get_key(hDB, hkey, &key);
      rsprintf("<th>%s", key.name);
      }

    rsprintf("</tr>\n");

    /* data for current group */
    sprintf(str, "/Equipment/%s/Settings/Names", eq_name);
    db_find_key(hDB, 0, str, &hkeyset);
    db_get_key(hDB, hkeyset, &key);
    for (i=0 ; i<key.num_values ; i++)
      {
      size = sizeof(str);
      db_get_data_index(hDB, hkeyset, str, &size, i, TID_STRING);

      name[0] = 0;
      if (strchr(str, '%'))
        {
        *strchr(str, '%') = 0;
        strcpy(name, str+strlen(str)+1);
        }
      else
        strcpy(name, str);

      if (!equal_ustring(group, "All") && !equal_ustring(str, group))
        continue;

      rsprintf("<tr><td colspan=%d>%s", colspan, name);

      for (j=0 ; ; j++)
        {
        db_enum_link(hDB, hkeyvar, j, &hkey);
        if (!hkey)
          break;
        db_get_key(hDB, hkey, &varkey);

        size = sizeof(data);
        db_get_data_index(hDB, hkey, data, &size, i, varkey.type);
        db_sprintf(str, data, varkey.item_size, 0, varkey.type);

        if (equal_ustring(varkey.name, "Demand") ||
            equal_ustring(varkey.name, "Output"))
          {
          if (exp_name[0])
            sprintf(ref, "%sEquipment/%s/Variables/%s?cmd=Set&index=%d&group=%s&exp=%s", 
                    mhttpd_url, eq_name, varkey.name, i, group, exp_name);
          else
            sprintf(ref, "%sEquipment/%s/Variables/%s?cmd=Set&index=%d&group=%s", 
                    mhttpd_url, eq_name, varkey.name, i, group);

          rsprintf("<td align=center><a href=\"%s\">%s</a>", 
                    ref, str);
          }
        else
          rsprintf("<td align=center>%s", str);
        }

      rsprintf("</tr>\n");
      }
    }
  else
    {
    /*---- multiple name arrays ----*/

    rsprintf("<tr><td colspan=6 bgcolor=#FFFFA0><i>Groups:</i> ");

    /* "all" group */
    if (equal_ustring(group, "All"))
      rsprintf("<b>All</b> ");
    else
      {
      if (exp_name[0])
        rsprintf("<a href=\"%sSC/%s?exp=%s\">All</a> ", 
                  mhttpd_url, eq_name, exp_name);
      else
        rsprintf("<a href=\"%sSC/%s?\">All</a> ", 
                  mhttpd_url, eq_name);
      }

    /* groups from Variables tree */

    sprintf(str, "/Equipment/%s/Variables", eq_name);
    db_find_key(hDB, 0, str, &hkeyvar);

    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyvar, i, &hkey);

      if (!hkey)
        break;

      db_get_key(hDB, hkey, &key);

      if (equal_ustring(key.name, group))
        rsprintf("<b>%s</b> ", key.name);
      else
        {
        if (exp_name[0])
          rsprintf("<a href=\"%sSC/%s/%s?exp=%s\">%s</a> ", 
                    mhttpd_url, eq_name, key.name, exp_name, key.name);
        else
          rsprintf("<a href=\"%sSC/%s/%s?\">%s</a> ", 
                    mhttpd_url, eq_name, key.name, key.name);
        }
      }

    rsprintf("</tr>\n");

    /* enumerate variable arrays */

    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hkeyvar, i, &hkey);

      if (!hkey)
        break;

      db_get_key(hDB, hkey, &varkey);

      if (!equal_ustring(group, "All") && !equal_ustring(varkey.name, group))
        continue;

      /* title row */
      rsprintf("<tr><th colspan=5>Names<th>%s</tr>\n", varkey.name);


      if (varkey.type == TID_KEY)
        {
        hkeyroot = hkey;

        /* enumerate subkeys */
        for (j=0 ; ; j++)
          {
          db_enum_key(hDB, hkeyroot, j, &hkey);
          if (!hkey)
            break;
          db_get_key(hDB, hkey, &key);
    
          if (key.type == TID_KEY)
            {
            /* for keys, don't display data value */
            rsprintf("<tr><td colspan=5>%s<br></tr>\n", key.name);
            }
          else
            {
            /* display single value */
            if (key.num_values == 1)
              {
              size = sizeof(data);
              db_get_data(hDB, hkey, data, &size, key.type);
              db_sprintf(data_str, data, key.item_size, 0, key.type);
              db_sprintfh(hex_str, data, key.item_size, 0, key.type);

              if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>"))
                {
                strcpy(data_str, "(empty)");
                hex_str[0] = 0;
                }

              if (strcmp(data_str, hex_str) != 0 && hex_str[0])
                rsprintf("<tr><td colspan=5>%s<td align=center>%s (%s)<br></tr>\n", 
                          key.name, data_str, hex_str);
              else
                rsprintf("<tr><td colspan=5>%s<td align=center>%s<br></tr>\n", 
                          key.name, data_str);
              }
            else
              {
              /* display first value */
              rsprintf("<tr><td colspan=5 rowspan=%d>%s\n", key.num_values, key.name);

              for (k=0 ; k<key.num_values ; k++)
                {
                size = sizeof(data);
                db_get_data_index(hDB, hkey, data, &size, k, key.type);
                db_sprintf(data_str, data, key.item_size, 0, key.type);
                db_sprintfh(hex_str, data, key.item_size, 0, key.type);

                if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>"))
                  {
                  strcpy(data_str, "(empty)");
                  hex_str[0] = 0;
                  }

                if (k>0)
                  rsprintf("<tr>");

                if (strcmp(data_str, hex_str) != 0 && hex_str[0])
                  rsprintf("<td>[%d] %s (%s)<br></tr>\n", k, data_str, hex_str);
                else
                  rsprintf("<td>[%d] %s<br></tr>\n", k, data_str);
                }
              }
            }
          }
        }
      else
        {
        /* data for current group */
        sprintf(str, "/Equipment/%s/Settings/Names %s", eq_name, varkey.name);
        db_find_key(hDB, 0, str, &hkeyset);

        for (j=0 ; j<varkey.num_values ; j++)
          {
          if (hkeyset)
            {
            size = sizeof(name);
            db_get_data_index(hDB, hkeyset, name, &size, j, TID_STRING);
            }
          else
            sprintf(name, "%s[%d]", varkey.name, j);

          rsprintf("<tr><td colspan=5>%s", name);

          size = sizeof(data);
          db_get_data_index(hDB, hkey, data, &size, j, varkey.type);
          db_sprintf(str, data, varkey.item_size, 0, varkey.type);

          if (equal_ustring(varkey.name, "Demand") ||
              equal_ustring(varkey.name, "Output"))
            {
            if (exp_name[0])
              sprintf(ref, "%sEquipment/%s/Variables/%s?cmd=Set&index=%d&group=%s&exp=%s", 
                      mhttpd_url, eq_name, varkey.name, j, group, exp_name);
            else
              sprintf(ref, "%sEquipment/%s/Variables/%s?cmd=Set&index=%d&group=%s", 
                      mhttpd_url, eq_name, varkey.name, j, group);

            rsprintf("<td align=center><a href=\"%s\">%s</a>", 
                      ref, str);
            }
          else
            rsprintf("<td align=center>%s", str);
          }

        rsprintf("</tr>\n");
        }
      }
    }

  rsprintf("</table>\r\n");
}


/*------------------------------------------------------------------*/

void show_cnaf_page()
{
char  *cmd, str[256], *pd;
int   c, n, a, f, d, q, x, r, ia, id, w;
int   i, size, status;
HNDLE hDB, hrootkey, hsubkey, hkey;

static char client_name[NAME_LENGTH];
static HNDLE hconn = 0;

  cm_get_experiment_database(&hDB, NULL);

  /* find FCNA server if not specified */
  if (hconn == 0)
    {
    /* find client which exports FCNA function */
    status = db_find_key(hDB, 0, "System/Clients", &hrootkey);
    if (status == DB_SUCCESS)
      {
      for (i=0 ; ; i++)
        {
        status = db_enum_key(hDB, hrootkey, i, &hsubkey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        sprintf(str, "RPC/%d", RPC_CNAF16);
        status = db_find_key(hDB, hsubkey, str, &hkey); 
        if (status == DB_SUCCESS)
          {
          size = sizeof(client_name);
          db_get_value(hDB, hsubkey, "Name", client_name, &size, TID_STRING);
          break;
          }
        }
      }

    if (client_name[0])
      {
      status = cm_connect_client(client_name, &hconn);
      if (status != RPC_SUCCESS)
        hconn = 0;
      }
    }

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS CAMAC interface</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s/CNAF\">\n\n", mhttpd_url);

  /* title row */

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  rsprintf("<table border=3 cellpadding=1>\n");
  rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);

  if (client_name[0] == 0)
    rsprintf("<th colspan=3 bgcolor=#FF0000>No CAMAC server running</tr>\n");
  else if (hconn == 0)
    rsprintf("<th colspan=3 bgcolor=#FF0000>Cannot connect to %s</tr>\n", client_name);
  else
    rsprintf("<th colspan=3 bgcolor=#A0A0FF>CAMAC server: %s</tr>\n", client_name);

  /* default values */
  c = n = 1;
  a = f = d = q = x = 0;
  r = 1;
  ia = id = w = 0;

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=3 bgcolor=#C0C0C0>\n");
  rsprintf("<input type=submit name=cmd value=Execute>\n");

  rsprintf("<td colspan=3 bgcolor=#C0C0C0>\n");
  rsprintf("<input type=submit name=cmd value=ODB>\n");
  rsprintf("<input type=submit name=cmd value=Status>\n");
  rsprintf("<input type=submit name=cmd value=Help>\n");
  rsprintf("</tr>\n\n");

  /* header */
  rsprintf("<tr><th bgcolor=#FFFF00>N");
  rsprintf("<th bgcolor=#FF8000>A");
  rsprintf("<th bgcolor=#00FF00>F");
  rsprintf("<th colspan=3 bgcolor=#8080FF>Data");

  /* execute commands */
  size = sizeof(d);
  
  cmd = getparam("cmd");
  if (equal_ustring(cmd, "C cycle"))
    {
    rpc_client_call(hconn, RPC_CNAF16, CNAF_CRATE_CLEAR, 0, 0, 0, 0, 0, &d, &size, &x, &q);

    rsprintf("<tr><td colspan=6 bgcolor=#00FF00>C cycle executed sucessfully</tr>\n");
    }
  else if (equal_ustring(cmd, "Z cycle"))
    {
    rpc_client_call(hconn, RPC_CNAF16, CNAF_CRATE_ZINIT, 0, 0, 0, 0, 0, &d, &size, &x, &q);

    rsprintf("<tr><td colspan=6 bgcolor=#00FF00>Z cycle executed sucessfully</tr>\n");
    }
  else if (equal_ustring(cmd, "Clear inhibit"))
    {
    rpc_client_call(hconn, RPC_CNAF16, CNAF_INHIBIT_CLEAR, 0, 0, 0, 0, 0, &d, &size, &x, &q);

    rsprintf("<tr><td colspan=6 bgcolor=#00FF00>Clear inhibit executed sucessfully</tr>\n");
    }
  else if (equal_ustring(cmd, "Set inhibit"))
    {
    rpc_client_call(hconn, RPC_CNAF16, CNAF_INHIBIT_SET, 0, 0, 0, 0, 0, &d, &size, &x, &q);

    rsprintf("<tr><td colspan=6 bgcolor=#00FF00>Set inhibit executed sucessfully</tr>\n");
    }
  else if (equal_ustring(cmd, "Execute"))
    {
    c = atoi(getparam("C"));
    n = atoi(getparam("N"));
    a = atoi(getparam("A"));
    f = atoi(getparam("F"));
    r = atoi(getparam("R"));
    w = atoi(getparam("W"));
    id = atoi(getparam("ID"));
    ia = atoi(getparam("IA"));

    pd = getparam("D");
    if (strncmp(pd, "0x", 2) == 0)
      sscanf(pd+2, "%x", &d);
    else
      d = atoi(getparam("D"));

    /* limit repeat range */
    if (r == 0)
      r = 1;
    if (r>100)
      r = 100;
    if (w > 1000)
      w = 1000;

    for (i=0 ; i<r ; i++)
      {
      status = SUCCESS;

      if (hconn)
        {
        size = sizeof(d);
        status = rpc_client_call(hconn, RPC_CNAF24, CNAF, 0, c, n, a, f, &d, &size, &x, &q);
      
        if (status == RPC_NET_ERROR)
          {
          /* try to reconnect */
          cm_disconnect_client(hconn, FALSE);
          status = cm_connect_client(client_name, &hconn);
          if (status != RPC_SUCCESS)
            {
            hconn = 0;
            client_name[0] = 0;
            }

          if (hconn)
            status = rpc_client_call(hconn, RPC_CNAF24, CNAF, 0, c, n, a, f, &d, &size, &x, &q);
          }
        }

      if (status != SUCCESS)
        {
        rsprintf("<tr><td colspan=6 bgcolor=#FF0000>Error executing function, code = %d</tr>", status);
        }
      else
        {
        rsprintf("<tr align=center><td bgcolor=#FFFF00>%d", n);
        rsprintf("<td bgcolor=#FF8000>%d", a);
        rsprintf("<td bgcolor=#00FF00>%d", f);
        rsprintf("<td colspan=3 bgcolor=#8080FF>%d / 0x%04X  Q%d X%d", d, d, q, x);
        }

      d += id;
      a += ia;

      if (w > 0)
        ss_sleep(w);
      }
    }

  /* input fields */
  rsprintf("<tr align=center><td bgcolor=#FFFF00><input type=text size=3 name=N value=%d>\n", n);
  rsprintf("<td bgcolor=#FF8000><input type=text size=3 name=A value=%d>\n", a);
  rsprintf("<td bgcolor=#00FF00><input type=text size=3 name=F value=%d>\n", f);
  rsprintf("<td bgcolor=#8080FF colspan=3><input type=text size=8 name=D value=%d></tr>\n", d);

  /* control fields */
  rsprintf("<tr><td colspan=2 bgcolor=#FF8080>Repeat");
  rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=R value=%d>\n", r);

  rsprintf("<td align=center colspan=3 bgcolor=#FF0000><input type=submit name=cmd value=\"C cycle\">\n");
  rsprintf("<input type=submit name=cmd value=\"Z cycle\">\n");

  rsprintf("<tr><td colspan=2 bgcolor=#FF8080>Repeat delay [ms]");
  rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=W value=%d>\n", w);

  rsprintf("<td align=center colspan=3 bgcolor=#FF0000><input type=submit name=cmd value=\"Set inhibit\">\n");
  rsprintf("<input type=submit name=cmd value=\"Clear inhibit\">\n");

  rsprintf("<tr><td colspan=2 bgcolor=#FF8080>Data increment");
  rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=ID value=%d>\n", id);

  rsprintf("<td colspan=3 align=center bgcolor=#FFFF80>Branch <input type=text size=3 name=B value=0>\n");

  rsprintf("<tr><td colspan=2 bgcolor=#FF8080>A increment");
  rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=IA value=%d>\n", ia);

  rsprintf("<td colspan=3 align=center bgcolor=#FFFF80>Crate <input type=text size=3 name=C value=%d>\n", c);

  rsprintf("</table></body>\r\n");
}

/*------------------------------------------------------------------*/

void show_experiment_page(char exp_list[MAX_EXPERIMENT][NAME_LENGTH])
{
int i;

  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html>\n");
  rsprintf("<head>\n");
  rsprintf("<title>MIDAS Experiment Selection</title>\n");
  rsprintf("</head>\n\n");

  rsprintf("<body>\n\n");
  rsprintf("Several experiments are defined on this host.<BR>\n");
  rsprintf("Please select the one to connect to:<P>");

  for (i=0 ; i<MAX_EXPERIMENT ; i++)
    {
    if (exp_list[i][0])
      rsprintf("<a href=\"%s?exp=%s\">%s</a><p>", mhttpd_url, 
                exp_list[i], exp_list[i]);
    }

  rsprintf("</body>\n");
  rsprintf("</html>\r\n");
  
}

/*------------------------------------------------------------------*/

void show_password_page(char *password, char *experiment)
{
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>Enter password</title></head><body>\n\n");

  rsprintf("<form method=\"GET\" action=\"%s\">\n\n", mhttpd_url);

  /* define hidden fields for current experiment */
  if (experiment[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", experiment);

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

BOOL check_web_password(char *password, char *redir, char *experiment)
{
HNDLE hDB, hkey;
INT   size;
char  str[256];

  cm_get_experiment_database(&hDB, NULL);

  /* check for password */
  db_find_key(hDB, 0, "/Experiment/Security/Web Password", &hkey);
  if (hkey)
    {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (strcmp(password, str) == 0)
      return TRUE;

    /* show web password page */
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
    rsprintf("Content-Type: text/html\r\n\r\n");

    rsprintf("<html><head><title>Enter password</title></head><body>\n\n");

    rsprintf("<form method=\"GET\" action=\"%s\">\n\n", mhttpd_url);

    /* define hidden fields for current experiment and destination */
    if (experiment[0])
      rsprintf("<input type=hidden name=exp value=\"%s\">\n", experiment);
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

void show_start_page(void)
{
int   i, j, n, size, status;
HNDLE hDB, hkey, hsubkey;
KEY   key;
char  data[1000], str[32];
char  data_str[256];

  cm_get_experiment_database(&hDB, NULL);

  show_header(hDB, "Start run", "", 1);

  rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Start new run</tr>\n");
  rsprintf("<tr><td>Run number");

  /* run number */
  size = sizeof(i);
  db_get_value(hDB, 0, "/Runinfo/Run number", &i, &size, TID_INT);

  rsprintf("<td><input type=text size=20 maxlength=80 name=value value=%d></tr>\n", i+1);

  /* run parameters */
  db_find_key(hDB, 0, "/Experiment/Edit on start", &hkey);
  if (hkey)
    {
    for (i=0,n=0 ; ; i++)
      {
      db_enum_link(hDB, hkey, i, &hsubkey);

      if (!hsubkey)
        break;

      db_get_key(hDB, hsubkey, &key);
      strcpy(str, key.name);

      db_enum_key(hDB, hkey, i, &hsubkey);
      db_get_key(hDB, hsubkey, &key);

      size = sizeof(data);
      status = db_get_data(hDB, hsubkey, data, &size, key.type);
      if (status != DB_SUCCESS)
        continue;

      for (j=0 ; j<key.num_values ; j++)
        {
        if (key.num_values > 1)
          rsprintf("<tr><td>%s [%d]", str, j);
        else
          rsprintf("<tr><td>%s", str);

        db_sprintf(data_str, data, key.item_size, j, key.type);
        
        rsprintf("<td><input type=text size=20 maxlenth=80 name=x%d value=\"%s\"></tr>\n",
                 n++, data_str);
        }
      }
    }

  rsprintf("<tr><td align=center colspan=2>");
  rsprintf("<input type=submit name=cmd value=Start>");
  rsprintf("<input type=submit name=cmd value=Cancel>");
  rsprintf("</tr>");
  rsprintf("</table>");

  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_odb_page(char *enc_path, char *dec_path)
{
int    i, j, size, status;
char   str[256], tmp_path[256], url_path[256], 
       data_str[256], hex_str[256], ref[256], keyname[32];
char   *p, *pd;
char   data[10000];
HNDLE  hDB, hkey, hkeyroot;
KEY    key;

  cm_get_experiment_database(&hDB, NULL);

  if (strcmp(enc_path, "root") == 0)
    {
    strcpy(enc_path, "");
    strcpy(dec_path, "");
    }

  show_header(hDB, "MIDAS online database", enc_path, 1);

  /* find key via path */
  status = db_find_key(hDB, 0, dec_path, &hkeyroot);
  if (status != DB_SUCCESS)
    {
    rsprintf("Error: cannot find key %s<P>\n", dec_path);
    rsprintf("</body></html>\r\n");
    return;
    }

  /* if key is not of type TID_KEY, cut off key name */
  db_get_key(hDB, hkeyroot, &key);
  if (key.type != TID_KEY)
    {
    /* strip variable name from path */
    p = dec_path+strlen(dec_path)-1;
    while (*p && *p != '/')
      *p-- = 0;
    if (*p == '/')
      *p = 0;

    strcpy(enc_path, dec_path);
    urlEncode(enc_path);

    status = db_find_key(hDB, 0, dec_path, &hkeyroot);
    if (status != DB_SUCCESS)
      {
      rsprintf("Error: cannot find key %s<P>\n", dec_path);
      rsprintf("</body></html>\r\n");
      return;
      }
    }

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#A0A0A0>\n");
  rsprintf("<input type=submit name=cmd value=Find>\n");
  rsprintf("<input type=submit name=cmd value=Create>\n");
  rsprintf("<input type=submit name=cmd value=Delete>\n");
  rsprintf("<input type=submit name=cmd value=Alarms>\n");
  rsprintf("<input type=submit name=cmd value=Programs>\n");
  rsprintf("<input type=submit name=cmd value=Status>\n");
  rsprintf("<input type=submit name=cmd value=Help>\n");
  rsprintf("</tr>\n\n");

  /*---- ODB display -----------------------------------------------*/

  /* display root key */
  rsprintf("<tr><td colspan=2 align=center><b>");
  if (exp_name[0])
    rsprintf("<a href=\"%sroot?exp=%s\">/</a> \n", mhttpd_url, exp_name);
  else
    rsprintf("<a href=\"%sroot?\">/</a> \n", mhttpd_url);

  strcpy(tmp_path, "");

  p = dec_path;
  if (*p == '/')
    p++;

  /*---- display path ----*/
  while (*p)
    {
    pd = str;
    while (*p && *p != '/')
      *pd++ = *p++;
    *pd = 0;

    strcat(tmp_path, str);
    strcpy(url_path, tmp_path);
    urlEncode(url_path);

    if (exp_name[0])
      rsprintf("<a href=\"%s%s?&exp=%s\">%s</a>\n / ", 
               mhttpd_url, url_path, exp_name, str);
    else
      rsprintf("<a href=\"%s%s\">%s</a>\n / ", 
               mhttpd_url, url_path, str);

    strcat(tmp_path, "/");
    if (*p == '/')
      p++;
    }
  rsprintf("</b></tr>\n");

  rsprintf("<tr><th>Key<th>Value</tr>\n");

  /* enumerate subkeys */
  for (i=0 ; ; i++)
    {
    db_enum_link(hDB, hkeyroot, i, &hkey);
    if (!hkey)
      break;
    db_get_key(hDB, hkey, &key);
    
    strcpy(str, dec_path);
    if (str[0] && str[strlen(str)-1] != '/')
      strcat(str, "/");
    strcat(str, key.name);
    urlEncode(str);
    strcpy(keyname, key.name);

    /* resolve links */
    if (key.type == TID_LINK)
      {
      db_enum_key(hDB, hkeyroot, i, &hkey);
      db_get_key(hDB, hkey, &key);
      }

    if (key.type == TID_KEY)
      {
      /* for keys, don't display data value */
      if (exp_name[0])
        rsprintf("<tr><td colspan=2 bgcolor=#FFD000><a href=\"%s%s?exp=%s\">%s</a><br></tr>\n", 
               mhttpd_url, str, exp_name, keyname);
      else
        rsprintf("<tr><td colspan=2 bgcolor=#FFD000><a href=\"%s%s\">%s</a><br></tr>\n", 
               mhttpd_url, str, keyname);
      }
    else
      {
      /* display single value */
      if (key.num_values == 1)
        {
        size = sizeof(data);
        db_get_data(hDB, hkey, data, &size, key.type);
        db_sprintf(data_str, data, key.item_size, 0, key.type);
        db_sprintfh(hex_str, data, key.item_size, 0, key.type);

        if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>"))
          {
          strcpy(data_str, "(empty)");
          hex_str[0] = 0;
          }

        if (exp_name[0])
          sprintf(ref, "%s%s?cmd=Set&exp=%s", 
                  mhttpd_url, str, exp_name);
        else
          sprintf(ref, "%s%s?cmd=Set", 
                  mhttpd_url, str);

        if (strcmp(data_str, hex_str) != 0 && hex_str[0])
          rsprintf("<tr><td bgcolor=#FFFF00>%s<td><a href=\"%s\">%s (%s)</a><br></tr>\n", 
                    keyname, ref, data_str, hex_str);
        else
          {
          rsprintf("<tr><td bgcolor=#FFFF00>%s<td><a href=\"%s\">", 
                    keyname, ref);
          strencode(data_str);
          rsprintf("</a><br></tr>\n");
          }
        }
      else
        {
        /* display first value */
        rsprintf("<tr><td  bgcolor=#FFFF00 rowspan=%d>%s\n", key.num_values, keyname);

        for (j=0 ; j<key.num_values ; j++)
          {
          size = sizeof(data);
          db_get_data_index(hDB, hkey, data, &size, j, key.type);
          db_sprintf(data_str, data, key.item_size, 0, key.type);
          db_sprintfh(hex_str, data, key.item_size, 0, key.type);

          if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>"))
            {
            strcpy(data_str, "(empty)");
            hex_str[0] = 0;
            }

          if (exp_name[0])
            sprintf(ref, "%s%s?cmd=Set&index=%d&exp=%s", 
                    mhttpd_url, str, j, exp_name);
          else
            sprintf(ref, "%s%s?cmd=Set&index=%d", 
                    mhttpd_url, str, j);

          if (j>0)
            rsprintf("<tr>");

          if (strcmp(data_str, hex_str) != 0 && hex_str[0])
            rsprintf("<td><a href=\"%s\">[%d] %s (%s)</a><br></tr>\n", ref, j, data_str, hex_str);
          else
            rsprintf("<td><a href=\"%s\">[%d] %s</a><br></tr>\n", ref, j, data_str);
          }
        }
      }
    }

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_set_page(char *enc_path, char *dec_path, char *group, int index, char *value)
{
int    status, size;
HNDLE  hDB, hkey;
KEY    key;
char   data_str[256], str[256], *p, eq_name[NAME_LENGTH];
char   data[10000];

  cm_get_experiment_database(&hDB, NULL);

  /* show set page if no value is given */
  if (!isparam("value"))
    {
    status = db_find_key(hDB, 0, dec_path, &hkey);
    if (status != DB_SUCCESS)
      {
      rsprintf("Error: cannot find key %s<P>\n", dec_path);
      return;
      }
    db_get_key(hDB, hkey, &key);

    show_header(hDB, "Set value", enc_path, 1);

    if (index >0)
      rsprintf("<input type=hidden name=index value=\"%d\">\n", index);
    else
      index = 0;

    if (group[0])
      rsprintf("<input type=hidden name=group value=\"%s\">\n", group);

    strcpy(data_str, rpc_tid_name(key.type));
    if (key.num_values > 1)
      {
      sprintf(str, "[%d]", key.num_values);
      strcat(data_str, str);

      sprintf(str, "%s[%d]", dec_path, index);
      }
    else
      strcpy(str, dec_path);

    rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Set new value - type = %s</tr>\n", data_str);
    rsprintf("<tr><td bgcolor=#FFFF00>%s<td>\n", str);

    /* set current value as default */
    size = sizeof(data);
    db_get_data(hDB, hkey, data, &size, key.type);
    db_sprintf(data_str, data, key.item_size, index, key.type);

    if (equal_ustring(data_str, "<NULL>"))
      data_str[0] = 0;

    size = 20;
    if ((int)strlen(data_str) > size)
      size = strlen(data_str)+3;
    if (size > 80)
      size = 80;
    rsprintf("<input type=\"text\" size=%d maxlength=256 name=\"value\" value=\"%s\">\n",
              size, data_str);
    rsprintf("</tr>\n");

    rsprintf("<tr><td align=center colspan=2>");
    rsprintf("<input type=submit name=cmd value=Set>");
    rsprintf("<input type=submit name=cmd value=Cancel>");
    rsprintf("</tr>");
    rsprintf("</table>");

    rsprintf("<input type=hidden name=cmd value=Set>\n");

    rsprintf("</body></html>\r\n");
    return;
    }
  else
    {
    /* set value */

    status = db_find_key(hDB, 0, dec_path, &hkey);
    if (status != DB_SUCCESS)
      {
      rsprintf("Error: cannot find key %s<P>\n", dec_path);
      return;
      }
    db_get_key(hDB, hkey, &key);

    memset(data, 0, sizeof(data));
    db_sscanf(value, data, &size, 0, key.type);
    
    if (index < 0)
      index = 0;

    /* extend data size for single string if necessary */
    if ((key.type == TID_STRING || key.type == TID_LINK) 
          && (int) strlen(data)+1 > key.item_size &&
        key.num_values == 1)
      key.item_size = strlen(data)+1;

    if (key.item_size == 0)
      key.item_size = rpc_tid_size(key.type);

    if (key.num_values > 1)
      status = db_set_data_index(hDB, hkey, data, key.item_size, index, key.type);
    else
      status = db_set_data(hDB, hkey, data, key.item_size, 1, key.type);

    if (status == DB_NO_ACCESS)
      rsprintf("<h2>Write access not allowed</h2>\n");

    /* strip variable name from path */
    p = dec_path+strlen(dec_path)-1;
    while (*p && *p != '/')
      *p-- = 0;
    if (*p == '/')
      *p = 0;

    strcpy(enc_path, dec_path);
    urlEncode(enc_path);

    /* redirect */

    if (group[0])
      {
      /* extract equipment name */
      eq_name[0] = 0;
      if (strncmp(enc_path, "Equipment/", 10) == 0)
        {
        strcpy(eq_name, enc_path+10);
        if (strchr(eq_name, '/'))
          *strchr(eq_name, '/') = 0;
        }

      /* back to SC display */
      sprintf(str, "SC/%s/%s", eq_name, group);
      redirect(str);
      }
    else
      redirect(enc_path);

    return;
    }

}

/*------------------------------------------------------------------*/

void show_find_page(char *enc_path, char *value)
{
HNDLE hDB, hkey;

  cm_get_experiment_database(&hDB, NULL);

  if (value[0] == 0)
    {
    /* without value, show find dialog */

    show_header(hDB, "Find value", enc_path, 1);

    rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Find string in Online Database</tr>\n");
    rsprintf("<tr><td>Enter substring (case insensitive)\n");

    rsprintf("<td><input type=\"text\" size=\"20\" maxlength=\"80\" name=\"value\">\n");
    rsprintf("</tr>");

    rsprintf("<tr><td align=center colspan=2>");
    rsprintf("<input type=submit name=cmd value=Find>");
    rsprintf("<input type=submit name=cmd value=Cancel>");
    rsprintf("</tr>");
    rsprintf("</table>");

    rsprintf("<input type=hidden name=cmd value=Find>");

    rsprintf("</body></html>\r\n");
    }
  else
    {
    show_header(hDB, "Search results", enc_path, 1);

    rsprintf("<tr><td colspan=2 bgcolor=#A0A0A0>\n");
    rsprintf("<input type=submit name=cmd value=Find>\n");
    rsprintf("<input type=submit name=cmd value=ODB>\n");
    rsprintf("<input type=submit name=cmd value=Help>\n");
    rsprintf("</tr>\n\n");

    rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>");
    rsprintf("Results of search for substring \"%s\"</tr>\n", value);
    rsprintf("<tr><th>Key<th>Value</tr>\n");

    /* start from root */
    db_find_key(hDB, 0, "", &hkey);

    /* scan tree, call "search_callback" for each key */
    db_scan_tree(hDB, hkey, 0, search_callback, (void *) value);

    rsprintf("</table>");
    rsprintf("</body></html>\r\n");
    }
}

/*------------------------------------------------------------------*/

void show_create_page(char *enc_path, char *dec_path, char *value, 
                      int index, int type)
{
char  str[256], link[256], *p;
char  data[10000];
int   status;
HNDLE hDB, hkey;
KEY   key;

  cm_get_experiment_database(&hDB, NULL);

  if (value[0] == 0)
    {
    /* without value, show create dialog */

    show_header(hDB, "Create ODB entry", enc_path, 1);

    rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Create ODB entry</tr>\n");

    rsprintf("<tr><td>Type");
    rsprintf("<td><select type=text size=1 name=type>\n");

    rsprintf("<option value=7> Integer (32-bit)\n");
    rsprintf("<option value=9> Float (4 Bytes)\n");
    rsprintf("<option value=12> String\n");
    rsprintf("<option value=15> Subdirectory\n");

    rsprintf("<option value=1> Byte\n");
    rsprintf("<option value=2> Signed byte\n");
    rsprintf("<option value=3> Character (8-bit)\n");
    rsprintf("<option value=4> Word (16-bit)\n");
    rsprintf("<option value=5> Short integer(16-bit)\n");
    rsprintf("<option value=6> Double Word (32-bit)\n");
    rsprintf("<option value=8> Boolean\n");
    rsprintf("<option value=10> Double float(8 Bytes)\n");
    rsprintf("<option value=16> Symbolic link\n");

    rsprintf("</select></tr>\n");

    rsprintf("<tr><td>Name");
    rsprintf("<td><input type=text size=20 maxlength=80 name=value>\n");
    rsprintf("</tr>");

    rsprintf("<tr><td>Array size");
    rsprintf("<td><input type=text size=20 maxlength=80 name=index value=1>\n");
    rsprintf("</tr>");

    rsprintf("<tr><td align=center colspan=2>");
    rsprintf("<input type=submit name=cmd value=Create>");
    rsprintf("<input type=submit name=cmd value=Cancel>");
    rsprintf("</tr>");
    rsprintf("</table>");

    rsprintf("</body></html>\r\n");
    }
  else
    {
    if (type == TID_LINK)
      {
      /* check if destination exists */
      status = db_find_key(hDB, 0, value, &hkey);
      if (status != DB_SUCCESS)
        {
        rsprintf("<h1>Error: Link destination \"%s\" does not exist!</h1>", value);
        return;
        }

      /* extract key name from destination */
      strcpy(str, value);
      p = str+strlen(str)-1;
      while (*p && *p != '/')
        *p--;
      p++;

      /* use it as link name */
      strcpy(link, p);

      strcpy(str, dec_path);
      if (str[strlen(str)-1] != '/')
        strcat(str, "/");
      strcat(str, link);


      status = db_create_link(hDB, 0, str, value); 
      if (status != DB_SUCCESS)
        {
        rsprintf("<h1>Cannot create key %s</h1>\n", str);
        return;
        }

      }
    else
      {
      strcpy(str, dec_path);
      if (str[strlen(str)-1] != '/')
        strcat(str, "/");
      strcat(str, value);

      status = db_create_key(hDB, 0, str, type);
      if (status != DB_SUCCESS)
        {
        rsprintf("<h1>Cannot create key %s</h1>\n", str);
        return;
        }

      if (index > 1)
        {
        db_find_key(hDB, 0, str, &hkey);
        db_get_key(hDB, hkey, &key);
        memset(data, 0, sizeof(data));
        if (key.type == TID_STRING || key.type == TID_LINK)
          key.item_size = NAME_LENGTH;
        db_set_data_index(hDB, hkey, data, key.item_size, index-1, key.type);
        }
      }

    /* redirect */
    redirect(enc_path);
    return;
    }
}

/*------------------------------------------------------------------*/

void show_delete_page(char *enc_path, char *dec_path, char *value, int index)
{
char  str[256];
int   i, status;
HNDLE hDB, hkeyroot, hkey;
KEY   key;

  cm_get_experiment_database(&hDB, NULL);

  if (value[0] == 0)
    {
    /* without value, show delete dialog */

    show_header(hDB, "Delete ODB entry", enc_path, 1);

    rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Delete ODB entry</tr>\n");

    /* find key via from */
    status = db_find_key(hDB, 0, dec_path, &hkeyroot);
    if (status != DB_SUCCESS)
      {
      rsprintf("Error: cannot find key %s<P>\n", dec_path);
      rsprintf("</body></html>\r\n");
      return;
      }
    
    /* count keys */
    for (i=0 ; ; i++)
      {
      db_enum_key(hDB, hkeyroot, i, &hkey);
      if (!hkey)
        break;
      }

    rsprintf("<tr><td align=center colspan=2><select type=text size=%d name=value>\n", i);

    /* enumerate subkeys */
    for (i=0 ; ; i++)
      {
      db_enum_key(hDB, hkeyroot, i, &hkey);
      if (!hkey)
        break;
      db_get_key(hDB, hkey, &key);
      rsprintf("<option> %s\n", key.name);
      }

    rsprintf("</select></tr>\n");

    rsprintf("<tr><td align=center colspan=2>");
    rsprintf("<input type=submit name=cmd value=Delete>");
    rsprintf("<input type=submit name=cmd value=Cancel>");
    rsprintf("</tr>");
    rsprintf("</table>");

    rsprintf("</body></html>\r\n");
    }
  else
    {
    strcpy(str, dec_path);
    if (str[strlen(str)-1] != '/')
      strcat(str, "/");
    strcat(str, value);

    status = db_find_link(hDB, 0, str, &hkey);
    if (status != DB_SUCCESS)
      {
      rsprintf("<h1>Cannot find key %s</h1>\n", str);
      return;
      }

    status = db_delete_key(hDB, hkey, FALSE);
    if (status != DB_SUCCESS)
      {
      rsprintf("<h1>Cannot delete key %s</h1>\n", str);
      return;
      }

    /* redirect */
    redirect(enc_path);
    return;
    }
}

/*------------------------------------------------------------------*/

void show_alarm_page()
{
INT   i, size, triggered, type, index;
BOOL  active;
HNDLE hDB, hkeyroot, hkey;
KEY   key;
char  str[256], ref[256], condition[256], value[256];

  cm_get_experiment_database(&hDB, NULL);

  show_header(hDB, "Alarms", "", 3);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=\"Reset all alarms\">\n");
  rsprintf("<input type=submit name=cmd value=\"Alarms on/off\">\n");
  rsprintf("<input type=submit name=cmd value=Status>\n");

  rsprintf("</tr>\n\n");

  /*---- global flag ----*/

  active = TRUE;
  size = sizeof(active);
  db_get_value(hDB, 0, "/Alarms/Alarm System active", &active, &size, TID_BOOL);
  if (!active)
    {
    if (exp_name[0])
      sprintf(ref, "%sAlarms/Alarm System active?cmd=set&exp=%s", 
              mhttpd_url, exp_name);
    else
      sprintf(ref, "%sAlarms/Alarm System active?cmd=set", 
              mhttpd_url);
    rsprintf("<tr><td align=center colspan=6 bgcolor=#FFC0C0><a href=\"%s\"><h1>Alarm system disabled</h1></a></tr>", ref);
    }

  /*---- alarms ----*/

  for (index = AT_EVALUATED ; index>=AT_INTERNAL ; index--)
    {
    if (index == AT_EVALUATED)
      {
      rsprintf("<tr><th align=center colspan=6 bgcolor=#A0FFFF>Evaluated alarms</tr>\n");
      rsprintf("<tr><th>Alarm<th>State<th>First triggered<th>Class<th>Condition<th>Current value</tr>\n");
      }
    else if (index == AT_PROGRAM)
      {
      rsprintf("<tr><th align=center colspan=6 bgcolor=#A0FFFF>Program alarms</tr>\n");
      rsprintf("<tr><th>Alarm<th>State<th>First triggered<th>Class<th colspan=2>Condition</tr>\n");
      }
    else if (index == AT_INTERNAL)
      {
      rsprintf("<tr><th align=center colspan=6 bgcolor=#A0FFFF>Internal alarms</tr>\n");
      rsprintf("<tr><th>Alarm<th>State<th>First triggered<th>Class<th colspan=2>Condition</tr>\n");
      }

    /* go through all alarms */
    db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
    if (hkeyroot)
      {
      for (i=0 ; ; i++)
        {
        db_enum_link(hDB, hkeyroot, i, &hkey);

        if (!hkey)
          break;

        db_get_key(hDB, hkey, &key);

        /* type */
        size = sizeof(INT);
        db_get_value(hDB, hkey, "Type", &type, &size, TID_INT);
        if (type != index)
          continue;

        /* alarm */
        if (exp_name[0])
          sprintf(ref, "%sAlarms/Alarms/%s?exp=%s", 
                  mhttpd_url, key.name, exp_name);
        else
          sprintf(ref, "%sAlarms/Alarms/%s", 
                  mhttpd_url, key.name);
        rsprintf("<tr><td bgcolor=#C0C0FF><a href=\"%s\"><b>%s</b></a>", ref, key.name);

        /* state */
        size = sizeof(BOOL);
        db_get_value(hDB, hkey, "Active", &active, &size, TID_BOOL);
        size = sizeof(INT);
        db_get_value(hDB, hkey, "Triggered", &triggered, &size, TID_INT);
        if (!active)
          rsprintf("<td bgcolor=#FFFF00 align=center>Disabled");
        else
          {
          if (!triggered)
            rsprintf("<td bgcolor=#00FF00 align=center>OK");
          else
            rsprintf("<td bgcolor=#FF0000 align=center>Triggered");
          }

        /* time */
        size = sizeof(str);
        db_get_value(hDB, hkey, "Time triggered first", str, &size, TID_STRING);
        if (!triggered)
          strcpy(str, "-");
        rsprintf("<td align=center>%s", str);

        /* class */
        size = sizeof(str);
        db_get_value(hDB, hkey, "Alarm Class", str, &size, TID_STRING);

        if (exp_name[0])
          sprintf(ref, "%sAlarms/Classes/%s?exp=%s", 
                  mhttpd_url, str, exp_name);
        else
          sprintf(ref, "%sAlarms/Classes/%s", 
                  mhttpd_url, str);
        rsprintf("<td align=center><a href=\"%s\">%s</a>", ref, str);

        /* condition */
        size = sizeof(condition);
        db_get_value(hDB, hkey, "Condition", &condition, &size, TID_STRING);

        if (index == AT_EVALUATED)
          {
          /* print condition */
          rsprintf("<td>");
          strencode(condition);

          /* retrieve value */
          al_evaluate_condition(condition, value);
          rsprintf("<td align=center bgcolor=#C0C0FF>%s", value);
          }
        else if (index == AT_PROGRAM)
          {
          /* print condition */
          rsprintf("<td colspan=2>");
          strencode(condition);
          }
        else if (index == AT_INTERNAL && triggered)
          {
          size = sizeof(str);
          db_get_value(hDB, hkey, "Alarm message", str, &size, TID_STRING);
          rsprintf("<td colspan=2>%s", str);
          }

        rsprintf("</tr>\n");
        }
      }
    }


  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_programs_page()
{
INT   i, j, size, count;
BOOL  restart, first, required;
HNDLE hDB, hkeyroot, hkey, hkey_rc, hkeycl;
KEY   key, keycl;
char  str[256], ref[256], command[256], name[80];

  cm_get_experiment_database(&hDB, NULL);

  /* stop command */
  if (*getparam("Stop"))
    {
    cm_shutdown(getparam("Stop")+5, FALSE);
    redirect("?cmd=programs");
    return;
    }

  /* start command */
  if (*getparam("Start"))
    {
    /* for NT: close reply socket before starting subprocess */
    redirect2("?cmd=programs");

    strcpy(name, getparam("Start")+6);
    sprintf(str, "/Programs/%s/Start command", name);
    command[0] = 0;
    size = sizeof(command);
    db_get_value(hDB, 0, str, command, &size, TID_STRING);
    if (command[0])
      {
      ss_system(command);
      for (i=0 ; i<50 ; i++)
        {
        if (cm_exist(name, FALSE) == CM_SUCCESS)
          break;
        ss_sleep(100);
        }
      }

    return;
    }

  show_header(hDB, "Programs", "", 3);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=Alarms>\n");
  rsprintf("<input type=submit name=cmd value=Status>\n");
  rsprintf("</tr>\n\n");

  rsprintf("<input type=hidden name=cmd value=Programs>\n");

  /*---- programs ----*/

  rsprintf("<tr><th>Program<th>Running on host<th>Alarm class<th>Autorestart</tr>\n");

  /* go through all programs */
  db_find_key(hDB, 0, "/Programs", &hkeyroot);
  if (hkeyroot)
    {
    for (i=0 ; ; i++)
      {
      db_enum_key(hDB, hkeyroot, i, &hkey);

      if (!hkey)
        break;

      db_get_key(hDB, hkey, &key);

      /* skip "execute on xxxx" */
      if (key.type != TID_KEY)
        continue;

      if (strncmp(key.name, "mhttpd", 6) == 0)
        continue;

      if (exp_name[0])
        sprintf(ref, "%sPrograms/%s?exp=%s", 
                mhttpd_url, key.name, exp_name);
      else
        sprintf(ref, "%sPrograms/%s", 
                mhttpd_url, key.name);

      /* required? */
      size = sizeof(required);
      db_get_value(hDB, hkey, "Required", &required, &size, TID_BOOL);

      /* running */
      count = 0;
      if (db_find_key(hDB, 0, "/System/Clients", &hkey_rc) == DB_SUCCESS)
        {
        first = TRUE;
        for (j=0 ; ; j++)
          {
          db_enum_key(hDB, hkey_rc, j, &hkeycl);
          if (!hkeycl)
            break;

          size = sizeof(name);
          db_get_value(hDB, hkeycl, "Name", name, &size, TID_STRING);

          db_get_key(hDB, hkeycl, &keycl);
          name[strlen(key.name)] = 0;

          if (equal_ustring(name, key.name))
            {
            size = sizeof(str);
            db_get_value(hDB, hkeycl, "Host", str, &size, TID_STRING);

            if (first)
              {
              rsprintf("<tr><td bgcolor=#C0C0FF><a href=\"%s\"><b>%s</b></a>", ref, key.name);
              rsprintf("<td align=center bgcolor=#00FF00>");
              }
            if (!first)
              rsprintf("<br>");
            rsprintf(str);

            first = FALSE;
            count++;
            }
          }
        }

      if (count == 0 && required)
        {
        rsprintf("<tr><td bgcolor=#C0C0FF><a href=\"%s\"><b>%s</b></a>", ref, key.name);
        rsprintf("<td align=center bgcolor=#FF0000>Not running");
        }

      /* dont display non-running programs which are not required */
      if (count == 0 && !required)
        continue;

      /* Alarm */
      size = sizeof(str);
      db_get_value(hDB, hkey, "Alarm Class", str, &size, TID_STRING);
      if (str[0])
        {
        if (exp_name[0])
          sprintf(ref, "%sAlarms/Classes/%s?exp=%s", 
                  mhttpd_url, str, exp_name);
        else
          sprintf(ref, "%sAlarms/Classes/%s", 
                  mhttpd_url, str);
        rsprintf("<td bgcolor=#FFFF00 align=center><a href=\"%s\">%s</a>", ref, str);
        }
      else
        rsprintf("<td align=center>-");

      /* auto restart */
      size = sizeof(restart);
      db_get_value(hDB, hkey, "Auto restart", &restart, &size, TID_BOOL);

      if (restart)
        rsprintf("<td align=center>Yes\n");
      else
        rsprintf("<td align=center>No\n");

      /* start/stop button */
      size = sizeof(str);
      db_get_value(hDB, hkey, "Start Command", str, &size, TID_STRING);
      if (str[0] && count == 0)
        {
        sprintf(str, "Start %s", key.name);
        rsprintf("<td align=center><input type=submit name=\"Start\" value=\"%s\">\n", str);
        }

      if (count > 0)
        {
        sprintf(str, "Stop %s", key.name);
        rsprintf("<td align=center><input type=submit name=\"Stop\" value=\"%s\">\n", str);
        }

      rsprintf("</tr>\n");
      }
    }


  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_config_page(int refresh)
{
char str[80];
HNDLE hDB;

  cm_get_experiment_database(&hDB, NULL);

  show_header(hDB, "Configure", "", 1);

  rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Configure</tr>\n");

  rsprintf("<tr><td bgcolor=#FFFF00>Update period\n");

  sprintf(str, "5");
  rsprintf("<td><input type=text size=5 maxlength=5 name=refr value=%d>\n", refresh);
  rsprintf("</tr>\n");

  rsprintf("<tr><td align=center colspan=2>");
  rsprintf("<input type=submit name=cmd value=Accept>");
  rsprintf("<input type=submit name=cmd value=Cancel>");
  rsprintf("</tr>");
  rsprintf("</table>");

  rsprintf("</body></html>\r\n");
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

void interprete(char *cookie_pwd, char *cookie_wpwd, char *path, int refresh)
/********************************************************************\

  Routine: interprete

  Purpose: Interprete parametersand generate HTML output from odb.

  Input:
    char *cookie_pwd        Cookie containing encrypted password
    char *path              ODB path "/dir/subdir/key"

  <implicit>
    _param/_value array accessible via getparam()

\********************************************************************/
{
int    i, j, n, status, size, run_state, index;
HNDLE  hkey, hsubkey, hDB;
KEY    key;
char   str[256], *p, *ps, *pe;
char   enc_path[256], dec_path[256], eq_name[NAME_LENGTH];
char   data[10000];
char   *experiment, *password, *wpassword, *command, *value, *group;
char   exp_list[MAX_EXPERIMENT][NAME_LENGTH];
time_t now;
struct tm *gmt;

  /* encode path for further usage */
  strcpy(dec_path, path);
  urlDecode(dec_path);
  urlDecode(dec_path); /* necessary for %2520 -> %20 -> ' ' */
  strcpy(enc_path, dec_path);
  urlEncode(enc_path);

  experiment = getparam("exp");
  password = getparam("pwd");
  wpassword = getparam("wpwd");
  command = getparam("cmd");
  value = getparam("value");
  group = getparam("group");
  index = atoi(getparam("index"));

  /*---- experiment connect ----------------------------------------*/

  /* disconnect from previous experiment if current is different */
  if (connected && strcmp(exp_name, experiment) != 0)
    {
    cm_disconnect_experiment();
    connected = FALSE;
    }

  if (!connected)
    {
    /* connect to experiment */
    if (experiment[0] == 0)
      {
      cm_list_experiments("", exp_list);
      if (exp_list[1][0])
        {
        /* present list of experiments to choose from */
        show_experiment_page(exp_list);
        return;
        }
      }

    if (password)
      sprintf(str, "set=%s", password);
    else
      sprintf(str, "set=");

    get_password(str);
    status = cm_connect_experiment("", experiment, "mhttpd", get_password);
    if (status == CM_WRONG_PASSWORD)
      {
      show_password_page(password, experiment);
      return;
      }

    if (status != CM_SUCCESS)
      {
      rsprintf("<H1>Experiment \"%s\" not defined on this host</H1>", experiment);
      return;
      }

    /* remember experiment */
    strcpy(exp_name, experiment);
    connected = TRUE;

    /* place a request for system messages */
    cm_msg_register(receive_message);

    /* redirect message display, turn on message logging */
    cm_set_msg_print(MT_ALL, MT_ALL, print_message);

    /* retrieve old messages */
    size = 1000;
    cm_msg_retrieve(data, &size);
    ps = data;
    do
      {
      if ((pe = strchr(ps, '\n')) != NULL)
        {
        *pe = 0;
        print_message(ps);
        ps = pe+1;
        }
      } while (pe != NULL);
    if (*ps)
      print_message(ps);
    }

  cm_get_experiment_database(&hDB, NULL);

  /* check for password */
  db_find_key(hDB, 0, "/Experiment/Security/Password", &hkey);
  if (!password[0] && hkey)
    {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);

    /* check for excemption */
    db_find_key(hDB, 0, "/Experiment/Security/Allowed programs/mhttpd", &hkey);
    if (hkey == 0 && strcmp(cookie_pwd, str) != 0)
      {
      show_password_page("", experiment);
      return;
      }
    }

  /* get run state */
  size = sizeof(run_state);
  db_get_value(hDB, 0, "/Runinfo/State", &run_state, &size, TID_INT);
  
  /*---- redirect with cookie if password given --------------------*/

  if (password[0])
    {
    rsprintf("HTTP/1.0 302 Found\r\n");
    rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

    time(&now);
    now += 3600*24;
    gmt = gmtime(&now);
    strftime(str, sizeof(str), "%A, %d-%b-%y %H:00:00 GMT", gmt);

    rsprintf("Set-Cookie: midas_pwd=%s; path=/; expires=%s\r\n", ss_crypt(password, "mi"), str);

    if (exp_name[0])
      rsprintf("Location: %s?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, exp_name);
    else
      rsprintf("Location: %s\n\n<html>redir</html>\r\n", mhttpd_url);
    return;
    }

  if (wpassword[0])
    {
    /* check if password correct */
    if (!check_web_password(ss_crypt(wpassword, "mi"), getparam("redir"), experiment))
      return;
    
    rsprintf("HTTP/1.0 302 Found\r\n");
    rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

    time(&now);
    now += 3600;
    gmt = gmtime(&now);
    strftime(str, sizeof(str), "%A, %d-%b-%y %H:%M:%S GMT", gmt);

    rsprintf("Set-Cookie: midas_wpwd=%s; path=/; expires=%s\r\n", ss_crypt(wpassword, "mi"), str);

    sprintf(str, "%s%s", mhttpd_url, getparam("redir"));
    if (exp_name[0])
      {
      if (strchr(str, '?'))
        rsprintf("Location: %s&exp=%s\n\n<html>redir</html>\r\n", str, exp_name);
      else
        rsprintf("Location: %s?exp=%s\n\n<html>redir</html>\r\n", str, exp_name);
      }
    else
      rsprintf("Location: %s\n\n<html>redir</html>\r\n", str);
    return;
    }

  /*---- redirect if ODB command -----------------------------------*/

  if (equal_ustring(command, "ODB"))
    {
    redirect("root");
    return;
    }

  /*---- redirect if SC command ------------------------------------*/

  if (equal_ustring(command, "SC"))
    {
    redirect("SC/");
    return;
    }

  /*---- redirect if status command --------------------------------*/

  if (equal_ustring(command, "status"))
    {
    redirect("");
    return;
    }

  /*---- help command ----------------------------------------------*/

  if (equal_ustring(command, "help"))
    {
    show_help_page();
    return;
    }
  
  /*---- pause run -------------------------------------------*/

  if (equal_ustring(command, "pause"))
    {
    if (run_state != STATE_RUNNING)
      {
      show_error("Run is not running");
      return;
      }

    if (!check_web_password(cookie_wpwd, "?cmd=pause", experiment))
      return;

    status = cm_transition(TR_PAUSE, 0, str, sizeof(str), SYNC);
    if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION)
      show_error(str);
    else
      redirect("");

    return;
    }

  /*---- resume run ------------------------------------------*/

  if (equal_ustring(command, "resume"))
    {
    if (run_state != STATE_PAUSED)
      {
      show_error("Run is not paused");
      return;
      }

    if (!check_web_password(cookie_wpwd, "?cmd=resume", experiment))
      return;

    status = cm_transition(TR_RESUME, 0, str, sizeof(str), SYNC);
    if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION)
      show_error(str);
    else
      redirect("");

    return;
    }

  /*---- start dialog --------------------------------------------*/

  if (equal_ustring(command, "start"))
    {
    if (run_state == STATE_RUNNING)
      {
      show_error("Run is already started");
      return;
      }

    if (value[0] == 0)
      {
      if (!check_web_password(cookie_wpwd, "?cmd=start", experiment))
        return;
      show_start_page();
      }
    else
      {
      /* set run parameters */
      db_find_key(hDB, 0, "/Experiment/Edit on start", &hkey);
      if (hkey)
        {
        for (i=0,n=0 ; ; i++)
          {
          db_enum_key(hDB, hkey, i, &hsubkey);

          if (!hsubkey)
            break;

          db_get_key(hDB, hsubkey, &key);

          for (j=0 ; j<key.num_values ; j++)
            {
            size = key.item_size;
            sprintf(str, "x%d", n++);
            db_sscanf(getparam(str), data, &size, j, key.type);
            db_set_data_index(hDB, hsubkey, data, size+1, j, key.type); 
            }
          }
        }

      i = atoi(value);
      status = cm_transition(TR_START, i, str, sizeof(str), SYNC);
      if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION)
        show_error(str);
      else
        redirect("");
      }
    return;
    }

  /*---- stop run --------------------------------------------*/

  if (equal_ustring(command, "stop"))
    {
    if (run_state != STATE_RUNNING)
      {
      show_error("Run is not running");
      return;
      }

    if (!check_web_password(cookie_wpwd, "?cmd=stop", experiment))
      return;

    status = cm_transition(TR_STOP, 0, str, sizeof(str), SYNC);
    if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION)
      show_error(str);
    else
      redirect("");

    return;
    }

  /*---- cancel command --------------------------------------------*/
  
  if (equal_ustring(command, "cancel"))
    {
    /* strip variable name from path */
    db_find_key(hDB, 0, dec_path, &hkey);
    if (hkey)
      {
      db_get_key(hDB, hkey, &key);

      if (key.type != TID_KEY)
        {
        p = enc_path+strlen(enc_path)-1;
        while (*p && *p != '/')
          *p-- = 0;
        if (*p == '/')
          *p = 0;
        }
      }

    if (group[0])
      {
      /* extract equipment name */
      eq_name[0] = 0;
      if (strncmp(enc_path, "Equipment/", 10) == 0)
        {
        strcpy(eq_name, enc_path+10);
        if (strchr(eq_name, '/'))
          *strchr(eq_name, '/') = 0;
        }

      /* back to SC display */
      sprintf(str, "SC/%s/%s", eq_name, group);
      redirect(str);
      }
    else
      redirect(enc_path);

    return;
    }

  /*---- set command -----------------------------------------------*/

  if (equal_ustring(command, "set"))
    {
    sprintf(str, "%s?cmd=set", enc_path);
    if (!check_web_password(cookie_wpwd, str, experiment))
      return;

    show_set_page(enc_path, dec_path, group, index, value);
    return;
    }

  /*---- find command ----------------------------------------------*/
  
  if (equal_ustring(command, "find"))
    {
    show_find_page(enc_path, value);
    return;
    }

  /*---- create command --------------------------------------------*/
  
  if (equal_ustring(command, "create"))
    {
    sprintf(str, "%s?cmd=create", enc_path);
    if (!check_web_password(cookie_wpwd, str, experiment))
      return;

    show_create_page(enc_path, dec_path, value, index, atoi(getparam("type")));
    return;
    }

  /*---- delete command --------------------------------------------*/
  
  if (equal_ustring(command, "delete"))
    {
    sprintf(str, "%s?cmd=delete", enc_path);
    if (!check_web_password(cookie_wpwd, str, experiment))
      return;

    show_delete_page(enc_path, dec_path, value, index);
    return;
    }

  /*---- CAMAC CNAF command ----------------------------------------*/
  
  if (equal_ustring(command, "CNAF") || strncmp(path, "/CNAF", 5) == 0)
    {
    if (!check_web_password(cookie_wpwd, "?cmd=CNAF", experiment))
      return;

    show_cnaf_page();
    return;
    }

  /*---- alarms command --------------------------------------------*/
  
  if (equal_ustring(command, "reset all alarms"))
    {
    if (!check_web_password(cookie_wpwd, "?cmd=reset%20all%20alarms", experiment))
      return;

    al_reset_alarm(NULL);
    redirect("?cmd=alarms");
    return;
    }

  if (equal_ustring(command, "Alarms on/off"))
    {
    redirect("Alarms/Alarm system active?cmd=set");
    return;
    }

  if (equal_ustring(command, "alarms"))
    {
    show_alarm_page();
    return;
    }

  /*---- programs command ------------------------------------------*/

  if (equal_ustring(command, "programs"))
    {
    str[0] = 0;
    if (*getparam("Start"))
      sprintf(str, "?cmd=programs&Start=%s", getparam("Start"));
    if (*getparam("Stop"))
      sprintf(str, "?cmd=programs&Stop=%s", getparam("Stop"));
 
    if (str[0])
      if (!check_web_password(cookie_wpwd, str, experiment))
        return;

    show_programs_page();
    return;
    }

  /*---- config command --------------------------------------------*/
  
  if (equal_ustring(command, "config"))
    {
    show_config_page(refresh);
    return;
    }

  /*---- Messages command ------------------------------------------*/
  
  if (equal_ustring(command, "messages"))
    {
    show_messages_page(refresh, FALSE);
    return;
    }

  if (equal_ustring(command, "more"))
    {
    show_messages_page(0, TRUE);
    return;
    }

  /*---- ELog command ----------------------------------------------*/

  if (equal_ustring(command, "elog"))
    {
    redirect("EL/");
    return;
    }

  if (strncmp(path, "EL/", 3) == 0)
    {
    if (equal_ustring(command, "new") ||
        equal_ustring(command, "edit") ||
        equal_ustring(command, "reply"))
      {
      sprintf(str, "%s?cmd=%s", path, command);
      if (!check_web_password(cookie_wpwd, str, experiment))
        return;
      }

    show_elog_page(dec_path+3);
    return;
    }
  
  /*---- accept command --------------------------------------------*/
  
  if (equal_ustring(command, "accept"))
    {
    refresh = atoi(getparam("refr"));

    /* redirect with cookie */
    rsprintf("HTTP/1.0 302 Found\r\n");
    rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
    rsprintf("Content-Type: text/html\r\n");

    time(&now);
    now += 3600*24*365;
    gmt = gmtime(&now);
    strftime(str, sizeof(str), "%A, %d-%b-%y %H:00:00 GMT", gmt);

    rsprintf("Set-Cookie: midas_refr=%d; path=/; expires=%s\r\n", refresh, str);

    if (exp_name[0])
      rsprintf("Location: %s?exp=%s\r\n\r\n<html>redir</html>\r\n", mhttpd_url, exp_name);
    else
      rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", mhttpd_url);

    return;
    }

  /*---- slow control display --------------------------------------*/
  
  if (strncmp(path, "SC/", 3) == 0)
    {
    show_sc_page(dec_path+3);
    return;
    }

  /*---- show status -----------------------------------------------*/
  
  if (path[0] == 0)
    {
    show_status_page(refresh, cookie_wpwd);
    return;
    }

  /*---- show ODB --------------------------------------------------*/

  if (path[0])
    {
    show_odb_page(enc_path, dec_path);
    return;
    }
}

/*------------------------------------------------------------------*/

void decode_get(char *string, char *cookie_pwd, char *cookie_wpwd, int refresh)
{
char path[256];
char *p, *pitem;

  initparam();

  strncpy(path, string+1, sizeof(path)); /* strip leading '/' */
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
  
  interprete(cookie_pwd, cookie_wpwd, path, refresh);
}

/*------------------------------------------------------------------*/

void decode_post(char *string, char *boundary, int length, 
                 char *cookie_pwd, char *cookie_wpwd, int refresh)
{
char *pinit, *p, *pitem, *ptmp, file_name[256], str[256];
int  n;

  initparam();

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
  
  interprete(cookie_pwd, cookie_wpwd, "EL/", refresh);
}

/*------------------------------------------------------------------*/

BOOL _abort = FALSE;

void ctrlc_handler(int sig)
{
  _abort = TRUE;
}

/*------------------------------------------------------------------*/

char net_buffer[1000000];

void server_loop(int tcp_port, int daemon)
{
int                  status, i, refresh;
struct sockaddr_in   bind_addr, acc_addr;
char                 host_name[256];
char                 cookie_pwd[256], cookie_wpwd[256], boundary[256];
int                  lsock, len, flag, content_length, header_length;
struct hostent       *phe;
struct linger        ling;
fd_set               readfds;
struct timeval       timeout;
INT                  last_time=0;

  /* establish Ctrl-C handler */
  ss_ctrlc_handler(ctrlc_handler);

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
    sprintf(mhttpd_url, "http://%s/", host_name);
  else
    sprintf(mhttpd_url, "http://%s:%d/", host_name, tcp_port);

  printf("Server listening...\n");
  do
    {
    FD_ZERO(&readfds);
    FD_SET(lsock, &readfds);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 100000;

#ifdef OS_UNIX
    do
      {
      status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);
      /* if an alarm signal was cought, restart with reduced timeout */
      } while (status == -1 && errno == EINTR);
#else
    status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);
#endif

    if (FD_ISSET(lsock, &readfds))
      {
      len = sizeof(acc_addr);
      _sock = accept( lsock,(struct sockaddr *) &acc_addr, &len);

      last_time = (INT) ss_time();

      /* turn on lingering (borrowed from NCSA httpd code) */
      ling.l_onoff = 1;
      ling.l_linger = 600;
      setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));

      /* ret remote host name */
      phe = gethostbyaddr((char *) &acc_addr.sin_addr, 4, PF_INET);
      if (phe == NULL)
        {
        /* use IP number instead */
        strcpy(remote_host_name, (char *)inet_ntoa(acc_addr.sin_addr));
        }
      else
        strcpy(remote_host_name, phe->h_name);

      memset(net_buffer, 0, sizeof(net_buffer));
      len = 0;
      header_length = 0;
      do
        {
        FD_ZERO(&readfds);
        FD_SET(_sock, &readfds);

        timeout.tv_sec  = 6;
        timeout.tv_usec = 0;

#ifdef OS_UNIX
        do
          {
          status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);
          /* if an alarm signal was cought, restart with reduced timeout */
          } while (status == -1 && errno == EINTR);
#else
        status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);
#endif
        if (FD_ISSET(_sock, &readfds))
          i = recv(_sock, net_buffer+len, sizeof(net_buffer)-len, 0);
        else
          goto error;
      
        /* abort if connection got broken */
        if (i<0)
          goto error;

        if (i>0)
          len += i;

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
          if (strstr(net_buffer, "Content-Length:"))
            content_length = atoi(strstr(net_buffer, "Content-Length:") + 15);
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

          net_buffer[header_length-1] = 0;

          if (header_length > 0 && len >= header_length+content_length)
            break;
          }
        else
          printf(net_buffer);

        } while (1);

      if (!strchr(net_buffer, '\r'))
        goto error;

      /* extract cookies */
      cookie_pwd[0] = 0;
      cookie_wpwd[0] = 0;
      if (strstr(net_buffer, "midas_pwd=") != NULL)
        {
        strcpy(cookie_pwd, strstr(net_buffer, "midas_pwd=")+10);
        cookie_pwd[strcspn(cookie_pwd, " ;\r\n")] = 0;
        }
      if (strstr(net_buffer, "midas_wpwd=") != NULL)
        {
        strcpy(cookie_wpwd, strstr(net_buffer, "midas_wpwd=")+11);
        cookie_wpwd[strcspn(cookie_wpwd, " ;\r\n")] = 0;
        }

      refresh = 0;
      if (strstr(net_buffer, "midas_refr=") != NULL)
        refresh = atoi(strstr(net_buffer, "midas_refr=")+11);
      if (refresh == 0)
        refresh = DEFAULT_REFRESH;

      memset(return_buffer, 0, sizeof(return_buffer));

      if (strncmp(net_buffer, "GET", 3) != 0 &&
          strncmp(net_buffer, "POST", 4) != 0)
        goto error;
      
      return_length = 0;

      if (strncmp(net_buffer, "GET", 3) == 0)
        {
        /* extract path and commands */
        *strchr(net_buffer, '\r') = 0;
  
        if (!strstr(net_buffer, "HTTP"))
          goto error;
        *(strstr(net_buffer, "HTTP")-1)=0;

        /* decode command and return answer */
        decode_get(net_buffer+4, cookie_pwd, cookie_wpwd, refresh);
        }
      else
        {
        decode_post(net_buffer+header_length, boundary, content_length, 
                    cookie_pwd, cookie_wpwd, refresh);
        }

      if (return_length != -1)
        {
        if (return_length == 0)
          return_length = strlen(return_buffer)+1;

        send_tcp(_sock, return_buffer, return_length, 0);

  error:

        closesocket(_sock);
        }
      }

    /* re-establish ctrl-c handler */
    ss_ctrlc_handler(ctrlc_handler);

    /* check if disconnect from experiment */
    if ((INT) ss_time() - last_time > CONNECT_TIME && connected && !no_disconnect)
      {
      cm_disconnect_experiment();
      connected = FALSE;
      }

    /* check for shutdown message */
    if (connected)
      {
      status = cm_yield(0);
      if (status == RPC_SHUTDOWN || _abort)
        {
        cm_disconnect_experiment();
        return;
        }
      }

    } while (!_abort);
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
int i;
int tcp_port = 80, daemon = FALSE;

  /* parse command line parameters */
  no_disconnect = FALSE;
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'c')
      no_disconnect = TRUE;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'p')
        tcp_port = atoi(argv[++i]);
      else
        {
usage:
        printf("usage: %s [-p port] [-D] [-c]\n\n", argv[0]);
        printf("       -D become a daemon\n");
        printf("       -c don't disconnect from experiment\n");
        return 0;
        }
      }
    }
  
  server_loop(tcp_port, daemon);

  return 0;
}

/********************************************************************\

  Name:         mserver.c
  Created by:   Stefan Ritt

  Contents:     Server program for midas RPC calls

  $Log$
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
#define DEFAULT_REFRESH  5

/* time until mhttpd disconnects from MIDAS */
#define CONNECT_TIME   600

char return_buffer[1000000];
int  return_length;
char mhttpd_url[256];
char exp_name[32];
BOOL connected;

#define MAX_GROUPS 32
#define MAX_PARAM  100
#define VALUE_SIZE 256
#define TEXT_SIZE  4096

char _param[MAX_PARAM][NAME_LENGTH];
char _value[MAX_PARAM][VALUE_SIZE];
char _text[TEXT_SIZE];

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

/*------------------------------------------------------------------*/

void rsprintf(const char *format, ...)
{
va_list argptr;
char    str[256];

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
    rsprintf("Location: %s%s?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, path, exp_name);
  else
    rsprintf("Location: %s%s\n\n<html>redir</html>\r\n", mhttpd_url, path);
}

/*------------------------------------------------------------------*/

void search_callback(HNDLE hDB, HNDLE hKey, KEY *key, INT level, void *info)
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

void show_status_page(int refresh)
{
int  i, size;
BOOL flag;
char str[256], name[32], ref[256];
char *yn[] = {"No", "Yes"};
char *state[] = {"", "Stopped", "Paused", "Running" };
time_t now, difftime;
DWORD  analyzed;
double analyze_ratio;
float  value;
HNDLE  hDB, hkey, hsubkey, hkeytmp;
KEY    key;
BOOL   ftp_mode;

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
  rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n\r\n");
  rsprintf("<html>\n");

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
  rsprintf("<input type=submit name=cmd value=Config>\n");
  rsprintf("<input type=submit name=cmd value=Help>\n");

  rsprintf("</tr>\n\n");

  /*---- aliases ----*/

  db_find_key(hDB, 0, "/Alias", &hkey);
  if (hkey)
    {
    rsprintf("<tr><td colspan=6 bgcolor=#80FF80>\n");
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
        rsprintf("<a href=\"%s\">%s</a> ", ref, key.name);
        }
      else if (key.type == TID_LINK)
        {
        /* odb link */
        if (exp_name[0])
          sprintf(ref, "%sAlias/%s?exp=%s", mhttpd_url, key.name, exp_name);
        else
          sprintf(ref, "%sAlias/%s", mhttpd_url, key.name);

        rsprintf("<a href=\"%s\" target=\"%s\">%s</a> ", ref, key.name, key.name);
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

  rsprintf("<td colspan=2 bgcolor=#%s>%s", str, state[runinfo.state]);

  if (cm_exist("Logger", FALSE) != CM_SUCCESS &&
      cm_exist("FAL", FALSE) != CM_SUCCESS)
    rsprintf("<td colspan=3 bgcolor=#FF0000>Logger not running</tr>\n");
  else
    {
    /* write data flag */
    size = sizeof(flag);
    db_get_value(hDB, 0, "/Logger/Write data", &flag, &size, TID_BOOL);

    if (!flag)
      rsprintf("<td colspan=3 bgcolor=#FFFF00>Logging disabled</tr>\n");
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
        size = sizeof(DWORD);
        if (db_get_value(hDB, hkeytmp, "Statistics/Events received", 
                         &analyzed, &size, TID_DWORD) == DB_SUCCESS &&
            equipment_stats.events_sent > 0)
          analyze_ratio = (double) analyzed / equipment_stats.events_sent;
        }
      
      /* check if analyze is running */
      if (cm_exist("Analyzer", FALSE) == CM_SUCCESS)
        rsprintf("<td align=center>%d<td align=center>%d<td align=center>%1.1lf<td align=center bgcolor=#00FF00>%3.1lf%%</tr>\n",
                 equipment_stats.events_sent,
                 equipment_stats.events_per_sec, 
                 equipment_stats.kbytes_per_sec,
                 analyze_ratio*100.0); 
      else
        rsprintf("<td align=center>%d<td align=center>%d<td align=center>%1.1lf<td align=center bgcolor=#FF0000>%3.1lf%%</tr>\n",
                 equipment_stats.events_sent,
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

  if (db_find_key(hDB, 0, "Lazy", &hkey) == DB_SUCCESS)
    {
    size = sizeof(str);
    db_get_value(hDB, 0, "/Lazy/Settings/Backup Type", str, &size, TID_STRING);
    ftp_mode = equal_ustring(str, "FTP");

    if (ftp_mode)
      rsprintf("<tr><th colspan=2>Lazy Destination<th>Progress<th>File Name<th>Speed [kb/s]<th>Total</tr>\n");
    else
      rsprintf("<tr><th colspan=2>Lazy Label<th>Progress<th>File Name<th># Files<th>Total</tr>\n");

    if (ftp_mode)
      {
      size = sizeof(str);
      db_get_value(hDB, 0, "/Lazy/Settings/Path", str, &size, TID_STRING);
      if (strchr(str, ','))
        *strchr(str, ',') = 0;
      }
    else
      {
      size = sizeof(str);
      db_get_value(hDB, 0, "/Lazy/Settings/List Label", str, &size, TID_STRING);
      if (str[0] == 0)
        strcpy(str, "(empty)");
      }

    if (exp_name[0])
      sprintf(ref, "%sLazy/Settings?exp=%s", mhttpd_url, exp_name);
    else
      sprintf(ref, "%sLazy/Settings", mhttpd_url);

    rsprintf("<tr><td colspan=2><B><a href=\"%s\">%s</a></B>", ref, str);

    size = sizeof(value);
    db_get_value(hDB, 0, "/Lazy/Statistics/Copy progress [%]", &value, &size, TID_FLOAT);
    rsprintf("<td align=center>%1.0f %%", value);

    size = sizeof(str);
    db_get_value(hDB, 0, "/Lazy/Statistics/Backup File", str, &size, TID_STRING);
    rsprintf("<td align=center>%s", str);

    if (ftp_mode)
      {
      size = sizeof(value);
      db_get_value(hDB, 0, "/Lazy/Statistics/Copy Rate [KB per sec]", &value, &size, TID_FLOAT);
      rsprintf("<td align=center>%1.1f", value);
      }
    else
      {
      size = sizeof(i);
      db_get_value(hDB, 0, "/Lazy/Statistics/Number of files", &i, &size, TID_INT);
      rsprintf("<td align=center>%d", i);
      }

    size = sizeof(value);
    db_get_value(hDB, 0, "/Lazy/Statistics/Backup status [%]", &value, &size, TID_FLOAT);
    rsprintf("<td align=center>%1.1f %%", value);

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

  rsprintf("<table border=3 cellpadding=2>\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);
  time(&now);

  rsprintf("<tr><th colspan=1 bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);
  rsprintf("<th colspan=1 bgcolor=#A0A0FF>%s</tr>\n", ctime(&now));

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

void el_format(char *text, char *encoding)
{
int i;

  for (i=0 ; i<(int) strlen(text) ; i++)
    {
    if (encoding[0] == 'H')
      switch (text[i])
        {
        case '\n': rsprintf("<br>\n"); break;
        default: rsprintf("%c", text[i]);
        }
    else
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

void show_elog_new(char *path)
{
int    i, size, run_number;
char   str[256], ref[256];
char   date[80], author[80], type[80], system[80], subject[256], text[10000], 
       orig_tag[80], reply_tag[80], attachment[256], encoding[80];
time_t now;
HNDLE  hDB, hkey;

  cm_get_experiment_database(&hDB, NULL);

  /* get message for reply */
  type[0] = system[0] = 0;
  if (path)
    {
    strcpy(str, path);
    size = sizeof(text);
    el_retrieve(str, date, &run_number, author, type, system, subject, 
                text, &size, orig_tag, reply_tag, attachment, encoding);
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

  rsprintf("<tr><th colspan=1 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th colspan=1 bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=Submit>\n");
  rsprintf("</tr>\n\n");

  /*---- entry form ----*/

  time(&now);
  rsprintf("<tr><td bgcolor=#FFFF00>Entry date: %s", ctime(&now));

  size = sizeof(run_number);
  db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT);
  rsprintf("<td bgcolor=#FFFF00>Run number: ");
  rsprintf("<input type=\"text\" size=10 maxlength=10 name=\"run\" value=\"%d\"</tr>", run_number);

  rsprintf("<tr><td bgcolor=#FFA0A0>Author: <input type=\"text\" size=\"15\" maxlength=\"80\" name=\"Author\">\n");

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

  rsprintf("<td bgcolor=#FFA0A0><a href=\"%s\" target=\"ELog Type\">Type:</a> <select name=\"type\">\n", ref);
  for (i=0 ; i<20 && type_list[i][0] ; i++)
    if (path && equal_ustring(type_list[i], "reply"))
      rsprintf("<option selected value=\"%s\">%s\n", type_list[i], type_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
  rsprintf("</select></tr>\n");

  rsprintf("<tr><td bgcolor=#A0FFA0><a href=\"%s\" target=\"ELog System\">  System:</a> <select name=\"system\">\n", ref);
  for (i=0 ; i<20 && system_list[i][0] ; i++)
    if (path && equal_ustring(system_list[i], system))
      rsprintf("<option selected value=\"%s\">%s\n", system_list[i], system_list[i]);
    else
      rsprintf("<option value=\"%s\">%s\n", system_list[i], system_list[i]);
  rsprintf("</select>\n");

  str[0] = 0;
  if (path)
    sprintf(str, "Re: %s", subject);
  rsprintf("<td bgcolor=#A0FFA0>Subject: <input type=text size=20 maxlength=\"80\" name=Subject value=\"%s\"></tr>\n", str);

  /* reply text */
  if (path)
    {
    rsprintf("<tr><td colspan=2 bgcolor=#E0E0FF><i>Original entry:</i><br><br>\n");
    el_format(text, encoding);
    rsprintf("</tr>\n");

    /* hidden text for original message */
    rsprintf("<input type=hidden name=orig value=\"%s\">\n", path);
    }
  
  rsprintf("<tr><td colspan=2>Text:<br>\n");
  rsprintf("<textarea rows=10 cols=80 name=Text></textarea></tr>\n");

  /* HTML check box */
  rsprintf("<tr><td><input type=\"checkbox\" name=\"html\" value=\"1\">Text contains HTML tags\n");
  
  /* attachment */
  rsprintf("<td>Attachment: <input type=\"file\" size=\"30\" maxlength=\"256\" name=\"attfile\" accept=\"filetype/*\" value=\"\">\n");

  rsprintf("</tr></table>\n");
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

  time(&now);
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

  rsprintf(" <input type=\"text\" size=3 maxlength=3 name=\"d1\" value=\"%d\">", tms->tm_mday);
  rsprintf(" <input type=\"text\" size=5 maxlength=5 name=\"y1\" value=\"%d\">", tms->tm_year);
  rsprintf("</tr>\n");

  rsprintf("<tr><td bgcolor=#FFFF00>End date: ");
  rsprintf("<td colspan=3 bgcolor=#FFFF00><select name=\"m2\" value=\"%s\">\n", mname[tms->tm_mon]);

  rsprintf("<option value=\"\">\n");
  for (i=0 ; i<12 ; i++)
    rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
  rsprintf("</select>\n");

  rsprintf(" <input type=\"text\" size=3 maxlength=3 name=\"d2\">");
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
  rsprintf("<input type=\"test\" size=\"15\" maxlength=\"80\" name=\"subject\">\n");

  rsprintf("</tr></table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_submit_query()
{
int    i, size, run, status, m1, d2, m2, y2;
char   date[80], author[80], type[80], system[80], subject[256], text[10000], 
       orig_tag[80], reply_tag[80], attachment[256], encoding[80];
char   str[256], tag[256], ref[80];
HNDLE  hDB;
DWORD  ltime_end, ltime_current;
struct tm tms;

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

  rsprintf("<table border=3 cellpadding=5 width=\"100%%\">\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th colspan=4 bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=7 bgcolor=#C0C0C0>\n");

  rsprintf("<input type=submit name=cmd value=\"Query\">\n");
  rsprintf("<input type=submit name=cmd value=\"ELog\">\n");
  rsprintf("<input type=submit name=cmd value=\"Status\">\n");
  rsprintf("</tr>\n\n");

  /*---- convert end date to ltime ----*/

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
  else
    ltime_end = 0;

  /*---- title row ----*/

  if (*getparam("r1"))
    {
    if (*getparam("r2"))
      rsprintf("<tr><td colspan=7 bgcolor=#FFFF00><b>Query result between runs %s and %s</b></tr>\n",
                getparam("r1"), getparam("r2"));
    else
      rsprintf("<tr><td colspan=7 bgcolor=#FFFF00><b>Query result between run %s and today</b></tr>\n",
                getparam("r1"));
    }
  else 
    {
    if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
      rsprintf("<tr><td colspan=7 bgcolor=#FFFF00><b>Query result between %s %s %s and %s %d %d</b></tr>\n",
                getparam("m1"), getparam("d1"), getparam("y1"),
                mname[m2], d2, y2);
    else
      rsprintf("<tr><td colspan=7 bgcolor=#FFFF00><b>Query result between %s %s %s and today</b></tr>\n",
                getparam("m1"), getparam("d1"), getparam("y1"));
    }

  rsprintf("</tr>\n<tr>");

  rsprintf("<td colspan=7 bgcolor=#FFA0A0>\n");

  if (*getparam("author"))
    rsprintf("Author: <b>%s</b>   ", getparam("author"));

  if (*getparam("type"))
    rsprintf("Type: <b>%s</b>   ", getparam("type"));

  if (*getparam("system"))
    rsprintf("System: <b>%s</b>   ", getparam("system"));

  if (*getparam("subject"))
    rsprintf("Subject: <b>%s</b>   ", getparam("subject"));

  rsprintf("</tr>\n");

  /*---- table titles ----*/

  rsprintf("<tr><th>Date<th>Run<th>Author<th>Type<th>System<th>Subject<th>Text</tr>\n");

  /*---- do query ----*/

  if (*getparam("r1"))
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
                         text, &size, orig_tag, reply_tag, attachment, encoding);
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
      if (*getparam("author") && !equal_ustring(getparam("author"), author))
        continue;
      if (*getparam("type") && !equal_ustring(getparam("type"), type))
        continue;
      if (*getparam("system") && !equal_ustring(getparam("system"), system))
        continue;
      if (*getparam("subject"))
        {
        strcpy(str, getparam("subject"));
        for (i=0 ; i<(int)strlen(str) ; i++)
          str[i] = toupper(str[i]);
        for (i=0 ; i<(int)strlen(subject) ; i++)
          subject[i] = toupper(subject[i]);

        if (strstr(subject, str) == NULL)
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
      str[250] = 0;
      rsprintf("<tr><td>%s<td>%d<td>%s<td>%s<td>%s<td>%s\n", date, run, author, type, system, subject);


      rsprintf("<td><a href=%s>", ref);
      
      el_format(str, encoding);
      
      rsprintf("</a></tr>\n");
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
    rsprintf(line);
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
char str[80];

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

  el_submit(atoi(getparam("run")), getparam("author"), getparam("type"),
            getparam("system"), getparam("subject"), getparam("text"), 
            getparam("orig"), *getparam("html") ? "HTML" : "plain", 
            getparam("attachment"), str);

  rsprintf("HTTP/1.0 302 Found\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

  if (exp_name[0])
    rsprintf("Location: %sEL/?exp=%s&msg=%s\n\n<html>redir</html>\r\n", mhttpd_url, exp_name, str);
  else
    rsprintf("Location: %sEL/?msg=%s\n\n<html>redir</html>\r\n", mhttpd_url, str);

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
  strcpy(text, "<code>");
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
  strcpy(text+strlen(text)-1, "</code>");
  
  el_submit(atoi(getparam("run")), getparam("author"), getparam("form"),
            "General", "", text, "", "HTML", "", str);

  rsprintf("HTTP/1.0 302 Found\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

  if (exp_name[0])
    rsprintf("Location: %sEL/?exp=%s&msg=%s\n\n<html>redir</html>\r\n", mhttpd_url, exp_name, str);
  else
    rsprintf("Location: %sEL/?msg=%s\n\n<html>redir</html>\r\n", mhttpd_url, str);
}

/*------------------------------------------------------------------*/

void show_elog_page(char *path)
{
int   size, i, run, msg_status, status, fh, length, first_message, last_message;
char  str[256], orig_path[256], command[80], ref[256], file_name[256];
char  date[80], author[80], type[80], system[80], subject[256], text[10000], 
      orig_tag[80], reply_tag[80], attachment[256], encoding[80];
HNDLE hDB, hkey, hkeyroot;
KEY   key;

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
    show_elog_new(NULL);
    return;
    }

  if (equal_ustring(command, "reply"))
    {
    show_elog_new(path);
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
    show_elog_submit_query();
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

      el_retrieve(path, date, &run, author, type, system, subject, 
                  text, &size, orig_tag, reply_tag, attachment, encoding);
      
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
                           text, &size, orig_tag, reply_tag, attachment, encoding);

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

  rsprintf("<table border=2 cellpadding=2 width=\"100%%\">\n");

  /*---- title row ----*/

  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);

  rsprintf("<tr><th colspan=1 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
  rsprintf("<th colspan=1 bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

  rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");
  rsprintf("<input type=submit name=cmd value=New>\n");
  rsprintf("<input type=submit name=cmd value=Reply>\n");
  rsprintf("<input type=submit name=cmd value=Query>\n");

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

    if (attachment[0])
      {
      for (i=0 ; i<(int)strlen(attachment) ; i++)
        str[i] = toupper(attachment[i]);
      
      if (exp_name[0])
        sprintf(ref, "%sEL/%s?exp=%s", 
                mhttpd_url, attachment, exp_name);
      else
        sprintf(ref, "%sEL/%s", 
                mhttpd_url, attachment);

      if (strstr(str, ".GIF") ||
          strstr(str, ".JPG"))
        {
        rsprintf("<tr><td colspan=2>Attachment: <a href=\"%s\"><b>%s</b></a><br>\n", 
                  ref, attachment+14);
        rsprintf("<img src=%s></tr>", ref);
        }
      else
        {
        rsprintf("<tr><td colspan=2 bgcolor=#8080FF>Attachment: <a href=\"%s\"><b>%s</b></a></tr>\n", 
                  ref, attachment+14);
        }
      }

    rsprintf("<tr><td colspan=2>\n");

    el_format(text, encoding);

    rsprintf("</tr>\n", text);
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

      /* redirect if no names found */
      if (!hkeynames)
        {
        /* redirect */
        sprintf(str, "Equipment/%s/Variables", eq_name);
        redirect(str);
        }
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
          rsprintf("<tr><td bgcolor=#FFFF00>%s<td><a href=\"%s\">%s</a><br></tr>\n", 
                    keyname, ref, data_str);
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
  if (value[0] == 0)
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

    rsprintf("<input type=\"text\" size=\"20\" maxlength=\"80\" name=\"value\" value=\"%s\">\n",
              data_str);
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

void interprete(char *cookie_pwd, char *path, int refresh)
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
char   *experiment, *password, *command, *value, *group;
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
    if (strcmp(cookie_pwd, str) != 0)
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
      rsprintf("<html><body><h1>Run is not running</h1><p></body><html>\n");
      return;
      }

    status = cm_transition(TR_PAUSE, 0, str, sizeof(str), SYNC);
    if (status != CM_SUCCESS)
      rsprintf("<html><body>Error pausing run: %s</body>", str);
    else
      redirect("");

    return;
    }

  /*---- resume run ------------------------------------------*/

  if (equal_ustring(command, "resume"))
    {
    if (run_state != STATE_PAUSED)
      {
      rsprintf("<html><body><h1>Run is not paused </h1><p></body><html>\n");
      return;
      }

    status = cm_transition(TR_RESUME, 0, str, sizeof(str), SYNC);
    if (status != CM_SUCCESS)
      rsprintf("<html><body>Error resuming run: %s</body>", str);
    else
      redirect("");

    return;
    }

  /*---- start dialog --------------------------------------------*/

  if (equal_ustring(command, "start"))
    {
    if (run_state == STATE_RUNNING)
      {
      rsprintf("<html><body><h1>Run is already started</h1><p></body><html>\n");
      return;
      }

    if (value[0] == 0)
      {
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
      if (status != CM_SUCCESS)
        rsprintf("<html><body>Error starting run: %s</body>", str);
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
      rsprintf("<html><body><h1>Run is not running</h1><p></body><html>\n");
      return;
      }

    status = cm_transition(TR_STOP, 0, str, sizeof(str), SYNC);
    if (status != CM_SUCCESS)
      rsprintf("<html><body>Error stopping run: %s</body>", str);
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
    show_create_page(enc_path, dec_path, value, index, atoi(getparam("type")));
    return;
    }

  /*---- delete command --------------------------------------------*/
  
  if (equal_ustring(command, "delete"))
    {
    show_delete_page(enc_path, dec_path, value, index);
    return;
    }

  /*---- CAMAC CNAF command ----------------------------------------*/
  
  if (equal_ustring(command, "CNAF") || strncmp(path, "/CNAF", 5) == 0)
    {
    show_cnaf_page();
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

  if (*getparam("msg"))
    {
    sprintf(str, "EL/%s", getparam("msg"));
    redirect("EL/");
    return;
    }

  if (strncmp(path, "EL/", 3) == 0)
    {
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

    rsprintf("Set-Cookie: midas_refr=%d; path=/; expires=0\r\n", refresh);

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
    show_status_page(refresh);
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

void decode_get(char *string, char *cookie_pwd, int refresh)
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
  
  interprete(cookie_pwd, path, refresh);
}

/*------------------------------------------------------------------*/

void decode_post(char *string, char *boundary, int length, char *cookie_pwd, int refresh)
{
char   *pinit, *p, *pitem, *ptmp, dir[256], file_name[256];
int    fh, size;
DWORD  now;
struct tm *tms;
HNDLE  hDB;

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

        /* strip path from filename */
        while (strchr(p, '\\'))
          p = strchr(p, '\\')+1; /* NT */
        while (strchr(p, '/'))
          p = strchr(p, '/')+1;  /* Unix */
        while (strchr(p, ']'))
          p = strchr(p, ']')+1;  /* VMS */

        /* assemble ELog filename */
        if (p[0])
          {
          cm_get_experiment_database(&hDB, NULL);
          dir[0] = 0;
          if (hDB > 0)
            {
            size = sizeof(dir);
            memset(dir, 0, size);
            db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
            if (dir[0] != 0)
              if (dir[strlen(dir)-1] != DIR_SEPARATOR)
                strcat(dir, DIR_SEPARATOR_STR);
            }


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
          setparam("attachment", file_name);
          sprintf(file_name, "%s%02d%02d%02d_%02d%02d%02d_%s", dir, 
                  tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
                  tms->tm_hour, tms->tm_min, tms->tm_sec, p);
          }
        else
          file_name[0] = 0;

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

        /* save file */
        if (file_name[0])
          {
          fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
          if (fh < 0)
            {
            /* print error message */
            }
          else
            {
            write(fh, string, (INT)p - (INT) string); 
            close(fh);
            }
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
  
  interprete(cookie_pwd, "EL/", refresh);
}

/*------------------------------------------------------------------*/

char net_buffer[1000000];

void server_loop(int tcp_port)
{
int                  status, i, refresh;
struct sockaddr_in   bind_addr, acc_addr;
char                 host_name[256];
char                 cookie_pwd[256], boundary[256];
int                  lsock, sock, len, flag, content_length, header_length;
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
      printf("Cannot bind to port %d.\nPlease use the \"-p\" flag to specify a different port\n", tcp_port);
      return;
      }
    else
      printf("Warning: port %d already in use\n", tcp_port);
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
      sock = accept( lsock,(struct sockaddr *) &acc_addr, &len);

      last_time = (INT) ss_time();

      /* turn on lingering (borrowed from NCSA httpd code) */
      ling.l_onoff = 1;
      ling.l_linger = 600;
      setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));

      memset(net_buffer, 0, sizeof(net_buffer));
      len = 0;
      header_length = 0;
      do
        {
        i = recv(sock, net_buffer+len, sizeof(net_buffer)-len, 0);
      
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
      if (strstr(net_buffer, "midas_pwd=") != NULL)
        {
        strcpy(cookie_pwd, strstr(net_buffer, "midas_pwd=")+10);
        cookie_pwd[strcspn(cookie_pwd, " ;\r\n")] = 0;
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
        decode_get(net_buffer+4, cookie_pwd, refresh);
        }
      else
        {
        decode_post(net_buffer+header_length, boundary, content_length, cookie_pwd, refresh);
        }

      if (return_length == 0)
        return_length = strlen(return_buffer)+1;

      send_tcp(sock, return_buffer, return_length, 0);

  error:

      closesocket(sock);
      }


    /* check if disconnect from experiment */
    if ((INT) ss_time() - last_time > CONNECT_TIME && connected)
      {
      cm_disconnect_experiment();
      connected = FALSE;
      }

    /* check for shutdown message */
    if (connected)
      {
      status = cm_yield(0);
      if (status == RPC_SHUTDOWN)
        {
        cm_disconnect_experiment();
        return;
        }
      }

    } while (1);
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
int i;
int tcp_port = 80;

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'p')
        tcp_port = atoi(argv[++i]);
      else
        {
usage:
        printf("usage: %s [-p port]\n", argv[0]);
        return 0;
        }
      }
    }
  
  server_loop(tcp_port);

  return 0;
}

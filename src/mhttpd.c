/********************************************************************\

  Name:         mserver.c
  Created by:   Stefan Ritt

  Contents:     Server program for midas RPC calls

  $Log$
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

char return_buffer[500000];
char mhttpd_url[256];
char exp_name[32];
BOOL connected;

#define MAX_GROUPS 32
#define MAX_PARAM  32

char _param[MAX_PARAM][NAME_LENGTH];
char _value[MAX_PARAM][256];

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
}

void setparam(char *param, char *value)
{
int i;

  for (i=0 ; i<MAX_PARAM ; i++)
    if (_param[i][0] == 0)
      break;

  if (i<MAX_PARAM)
    {
    strcpy(_param[i], param);
    strcpy(_value[i], value);
    }
  else
    {
    printf("Error: parameter array too small\n");
    }
}

char *getparam(char *param)
{
int i;

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

void search_callback(HNDLE hDB, HNDLE hKey, KEY *key, void *info)
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
    db_get_path(hDB, hKey, path, MAX_ODB_PATH);
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

void show_status_page(HNDLE hDB, int refresh)
{
int  i, size;
BOOL flag;
char str[256], name[32], ref[256];
char *yn[] = {"No", "Yes"};
char *state[] = {"", "Stopped", "Paused", "Running" };
time_t now, difftime;
DWORD  analyzed;
double analyze_ratio;
HNDLE  hkey, hsubkey, hkeytmp;
KEY    key;

RUNINFO_STR(runinfo_str);
RUNINFO runinfo;
EQUIPMENT_INFO equipment;
EQUIPMENT_STATS equipment_stats;
CHN_SETTINGS chn_settings;
CHN_STATISTICS chn_stats;

  /* create/correct /runinfo structure */
  db_create_record(hDB, 0, "/Runinfo", strcomb(runinfo_str));
  db_find_key(hDB, 0, "/Runinfo", &hkey);
  size = sizeof(runinfo);
  db_get_record(hDB, hkey, &runinfo, &size, 0);

  /* header */
  rsprintf("HTTP/1.0 200 Document follows\r\n");
  rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());
  rsprintf("Content-Type: text/html\r\n\r\n");

  rsprintf("<html><head><title>MIDAS status</title></head>\n");
  rsprintf("<body><form method=\"GET\" action=\"%s\">\n", mhttpd_url);

  /* define hidden field for experiment */
  if (exp_name[0])
    rsprintf("<input type=hidden name=exp value=\"%s\">\n", exp_name);

  /* auto refresh */
  rsprintf("<meta http-equiv=\"Refresh\" content=%d>\n\n", refresh);

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

      if (exp_name[0])
        sprintf(ref, "%s/Alias/%s?exp=%s", mhttpd_url, key.name, exp_name);
      else
        sprintf(ref, "%s/Alias/%s", mhttpd_url, key.name);

      rsprintf("<a href=\"%s\">%s</a> ", ref, key.name);
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

      db_find_key(hDB, hsubkey, "Common", &hkeytmp);
      size = sizeof(equipment);
      db_get_record(hDB, hkeytmp, &equipment, &size, 0);
      
      db_find_key(hDB, hsubkey, "Statistics", &hkeytmp);
      size = sizeof(equipment_stats);
      db_get_record(hDB, hkeytmp, &equipment_stats, &size, 0);

      if (exp_name[0])
        sprintf(ref, "%s/SC/%s?exp=%s", mhttpd_url, key.name, exp_name);
      else
        sprintf(ref, "%s/SC/%s", mhttpd_url, key.name);
      
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

      if (exp_name[0])
        sprintf(ref, "%s/Logger/Channels/%s/Settings?exp=%s", mhttpd_url, key.name, exp_name);
      else
        sprintf(ref, "%s/Logger/Channels/%s/Settings", mhttpd_url, key.name);

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

void show_sc_page(HNDLE hDB, char *path)
{
int    i, j, colspan, size;
char   str[256], eq_name[32], group[32], name[32], ref[256];
char   group_name[MAX_GROUPS][32], data[32];
HNDLE  hkey, hkeyeq, hkeyset, hkeynames, hkeyvar;
KEY    eqkey, key, varkey;

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
        sprintf(str, "/Equipment/%s/Variables", eq_name);
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
                rsprintf("<a href=\"%s/SC/%s?exp=%s\">%s</a> ", 
                          mhttpd_url, eqkey.name, exp_name, eqkey.name);
              else
                rsprintf("<a href=\"%s/SC/%s?\">%s</a> ", 
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
        rsprintf("<a href=\"%s/SC/%s/All?exp=%s\">All</a> ", 
                  mhttpd_url, eq_name, exp_name);
      else
        rsprintf("<a href=\"%s/SC/%s/All?\">All</a> ", 
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
          rsprintf("<a href=\"%s/SC/%s/%s?exp=%s\">%s</a> ", 
                    mhttpd_url, eq_name, group_name[i], exp_name, group_name[i]);
        else
          rsprintf("<a href=\"%s/SC/%s/%s?\">%s</a> ", 
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
            sprintf(ref, "%s/Equipment/%s/Variables/%s?cmd=Set&index=%d&group=%s&exp=%s", 
                    mhttpd_url, eq_name, varkey.name, i, group, exp_name);
          else
            sprintf(ref, "%s/Equipment/%s/Variables/%s?cmd=Set&index=%d&group=%s", 
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
        rsprintf("<a href=\"%s/SC/%s?exp=%s\">All</a> ", 
                  mhttpd_url, eq_name, exp_name);
      else
        rsprintf("<a href=\"%s/SC/%s?\">All</a> ", 
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
          rsprintf("<a href=\"%s/SC/%s/%s?exp=%s\">%s</a> ", 
                    mhttpd_url, eq_name, key.name, exp_name, key.name);
        else
          rsprintf("<a href=\"%s/SC/%s/%s?\">%s</a> ", 
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
            sprintf(ref, "%s/Equipment/%s/Variables/%s?cmd=Set&index=%d&group=%s&exp=%s", 
                    mhttpd_url, eq_name, varkey.name, j, group, exp_name);
          else
            sprintf(ref, "%s/Equipment/%s/Variables/%s?cmd=Set&index=%d&group=%s", 
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

  rsprintf("</table>\r\n");
}


/*------------------------------------------------------------------*/

void show_cnaf_page(HNDLE hDB)
{
char  *cmd, str[256], *pd;
int   c, n, a, f, d, q, x, r, ia, id, w;
int   i, size, status;
HNDLE hrootkey, hsubkey, hkey;

static char client_name[NAME_LENGTH];
static HNDLE hconn = 0;

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

void show_start_page(hDB)
{
int   i, j, n, size, status;
HNDLE hkey, hsubkey;
KEY   key;
char  data[1000], str[32];
char  data_str[256];

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

void show_odb_page(HNDLE hDB, char *enc_path, char *dec_path)
{
int    i, j, size, status;
char   str[256], tmp_path[256], url_path[256], 
       data_str[256], ref[256], keyname[32];
char   *p, *pd;
char   data[10000];
HNDLE  hkey, hkeyroot;
KEY    key;

  if (strcmp(enc_path, "/root") == 0)
    {
    strcpy(enc_path, "/");
    strcpy(dec_path, "/");
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
    rsprintf("<a href=\"%s/root?exp=%s\">/</a> \n", mhttpd_url, exp_name);
  else
    rsprintf("<a href=\"%s/root?\">/</a> \n", mhttpd_url);

  strcpy(tmp_path, "/");

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
    if (str[strlen(str)-1] != '/')
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

        if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>"))
          strcpy(data_str, "(empty)");

        if (exp_name[0])
          sprintf(ref, "%s%s?cmd=Set&exp=%s", 
                  mhttpd_url, str, exp_name);
        else
          sprintf(ref, "%s%s?cmd=Set", 
                  mhttpd_url, str);

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
          db_get_data(hDB, hkey, data, &size, key.type);
          db_sprintf(data_str, data, key.item_size, j, key.type);

          if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>"))
            strcpy(data_str, "(empty)");

          if (exp_name[0])
            sprintf(ref, "%s%s?cmd=Set&index=%d&exp=%s", 
                    mhttpd_url, str, j, exp_name);
          else
            sprintf(ref, "%s%s?cmd=Set&index=%d", 
                    mhttpd_url, str, j);

          if (j>0)
            rsprintf("<tr>");

          rsprintf("<td><a href=\"%s\">[%d] %s</a><br></tr>\n", ref, j, data_str);
          }
        }
      }
    }

  rsprintf("</table>\n");
  rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_set_page(HNDLE hDB, char *enc_path, char *dec_path, char *group, int index, char *value)
{
int    status, size;
HNDLE  hkey;
KEY    key;
char   data_str[256], str[256], *p, eq_name[NAME_LENGTH];
char   data[10000];

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
      if (strncmp(enc_path, "/Equipment/", 11) == 0)
        {
        strcpy(eq_name, enc_path+11);
        if (strchr(eq_name, '/'))
          *strchr(eq_name, '/') = 0;
        }

      /* back to SC display */
      sprintf(str, "/SC/%s/%s", eq_name, group);
      redirect(str);
      }
    else
      redirect(enc_path);

    return;
    }

}

/*------------------------------------------------------------------*/

void show_find_page(HNDLE hDB, char *enc_path, char *value)
{
HNDLE hkey;

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
    db_scan_tree(hDB, hkey, search_callback, (void *) value);

    rsprintf("</table>");
    rsprintf("</body></html>\r\n");
    }
}

/*------------------------------------------------------------------*/

void show_create_page(HNDLE hDB, char *enc_path, char *dec_path, char *value, 
                      int index, int type)
{
char  str[256], link[256], *p;
char  data[10000];
int   status;
HNDLE hkey;
KEY   key;

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

void show_delete_page(HNDLE hDB, char *enc_path, char *dec_path, char *value, int index)
{
char  str[256];
int   i, status;
HNDLE hkeyroot, hkey;
KEY   key;

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

void show_config_page(HNDLE hDB, int refresh)
{
char str[80];

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
char   str[256], *p;
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

    /* turn off message display, turn on message logging */
    cm_set_msg_print(MT_ALL, 0, NULL);
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

    rsprintf("Set-cookie: midas_pwd=%s; path=/; expires=%s\r\n", ss_crypt(password, "mi"), str);

    if (exp_name[0])
      rsprintf("Location: %s/?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, exp_name);
    else
      rsprintf("Location: %s\n\n<html>redir</html>\r\n", mhttpd_url);
    return;
    }

  /*---- redirect if ODB command -----------------------------------*/

  if (equal_ustring(command, "ODB"))
    {
    redirect("/root");
    return;
    }

  /*---- redirect if SC command ------------------------------------*/

  if (equal_ustring(command, "SC"))
    {
    redirect("/SC/");
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
      show_start_page(hDB);
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
      if (strncmp(enc_path, "/Equipment/", 11) == 0)
        {
        strcpy(eq_name, enc_path+11);
        if (strchr(eq_name, '/'))
          *strchr(eq_name, '/') = 0;
        }

      /* back to SC display */
      sprintf(str, "/SC/%s/%s", eq_name, group);
      redirect(str);
      }
    else
      redirect(enc_path);

    return;
    }

  /*---- set command -----------------------------------------------*/

  if (equal_ustring(command, "set"))
    {
    show_set_page(hDB, enc_path, dec_path, group, index, value);
    return;
    }

  /*---- find command ----------------------------------------------*/
  
  if (equal_ustring(command, "find"))
    {
    show_find_page(hDB, enc_path, value);
    return;
    }

  /*---- create command --------------------------------------------*/
  
  if (equal_ustring(command, "create"))
    {
    show_create_page(hDB, enc_path, dec_path, value, index, atoi(getparam("type")));
    return;
    }

  /*---- delete command --------------------------------------------*/
  
  if (equal_ustring(command, "delete"))
    {
    show_delete_page(hDB, enc_path, dec_path, value, index);
    return;
    }

  /*---- CAMAC CNAF command ----------------------------------------*/
  
  if (equal_ustring(command, "CNAF") || strncmp(path, "/CNAF", 5) == 0)
    {
    show_cnaf_page(hDB);
    return;
    }

  /*---- config command --------------------------------------------*/
  
  if (equal_ustring(command, "config"))
    {
    show_config_page(hDB, refresh);
    return;
    }

  /*---- accept command --------------------------------------------*/
  
  if (equal_ustring(command, "accept"))
    {
    refresh = atoi(getparam("refr"));

    /* redirect with cookie */
    rsprintf("HTTP/1.0 302 Found\r\n");
    rsprintf("Server: MIDAS HTTP %s\r\n", cm_get_version());

    rsprintf("Set-cookie: midas_refr=%d; path=/; expires=0\n", refresh);

    if (exp_name[0])
      rsprintf("Location: %s/?exp=%s\n\n<html>redir</html>\r\n", mhttpd_url, exp_name);
    else
      rsprintf("Location: %s\n\n<html>redir</html>\r\n", mhttpd_url);
    return;
    }

  /*---- slow control display --------------------------------------*/
  
  if (strncmp(path, "/SC/", 4) == 0)
    {
    show_sc_page(hDB, path+4);
    return;
    }

  /*---- show status -----------------------------------------------*/
  
  if (path[1] == 0)
    {
    show_status_page(hDB, refresh);
    return;
    }

  /*---- show ODB --------------------------------------------------*/

  if (path[1])
    {
    show_odb_page(hDB, enc_path, dec_path);
    return;
    }
}

/*------------------------------------------------------------------*/

void decode(char *string, char *cookie_pwd, int refresh)
{
char path[256];
char *p, *pitem;

  initparam();

  strcpy(path, string);
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

int server_loop(int tcp_port)
{
int                  status, i, refresh;
struct sockaddr_in   bind_addr, acc_addr;
char                 host_name[256];
char                 str[256], cookie_pwd[256];
int                  lsock, sock, len;
struct hostent       *phe;
char                 net_buffer[1000];
struct linger        ling;
fd_set               readfds;
struct timeval       timeout;
INT                  last_time=0;

#ifdef OS_WINNT
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return RPC_NET_ERROR;
  }
#endif

  /* create a new socket */
  lsock = socket(AF_INET, SOCK_STREAM, 0);

  if (lsock == -1)
    {
    printf("Cannot create socket\n");
    return 0;
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
    return 0;
    }
  strcpy(host_name, phe->h_name);

  /* ...better without
  flag = 1;
  setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR,
             (char *) &flag, sizeof(INT));
  */

  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);

  status = bind(lsock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    printf("Cannot bind to port %d. Please use the \"-p\" flag to specify a different port.\n", tcp_port);
    return 0;
    }

  /* listen for connection */
  status = listen(lsock, SOMAXCONN);
  if (status < 0)
    {
    printf("Cannot listen\n");
    return 0;
    }

  /* set my own URL */
  if (tcp_port == 80)
    sprintf(mhttpd_url, "http://%s", host_name);
  else
    sprintf(mhttpd_url, "http://%s:%d", host_name, tcp_port);

  printf("Server listening...\n");
  do
    {
    FD_ZERO(&readfds);
    FD_SET(lsock, &readfds);

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

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
      do
        {
        i = recv(sock, net_buffer+len, sizeof(net_buffer)-len, 0);
      
        /* abort if connection got broken */
        if (i<=0)
          goto error;

        if (i>0)
          len += i;

        /* finish when empty line received */
        if (len>4 && strcmp(&net_buffer[len-4], "\r\n\r\n") == 0)
          break;
        if (len>6 && strcmp(&net_buffer[len-6], "\r\r\n\r\r\n") == 0)
          break;

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

      /* extract path and commands */
      *strchr(net_buffer, '\r') = 0;
      if (strncmp(net_buffer, "GET", 3) != 0)
        goto error;
      strcpy(str, net_buffer+4);
    
      if (!strstr(str, "HTTP"))
        goto error;
      *(strstr(str, "HTTP")-1)=0;

      memset(return_buffer, 0, sizeof(return_buffer));

      /* decode command and return answer */
      decode(str, cookie_pwd, refresh);

      i = strlen(return_buffer);
      send(sock, return_buffer, i+1, 0);
 
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
        connected = FALSE;
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
/********************************************************************\

  Name:         webpaw.c
  Created by:   Stefan Ritt

  Contents:     Web server for remote PAW display

  $Log$
  Revision 1.3  2000/05/12 12:59:48  midas
  Adjusted WebPAW home directory

  Revision 1.2  2000/05/12 09:00:39  midas
  Added execute button

  Revision 1.1  2000/05/11 14:11:45  midas
  Added file

\********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

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
#endif

#define WEB_BUFFER_SIZE 100000
#define MAX_PARAM           10
#define VALUE_SIZE         256
#define PARAM_LENGTH       256

char return_buffer[WEB_BUFFER_SIZE];
int  return_length;
char webpaw_url[256];

char _param[MAX_PARAM][PARAM_LENGTH];
char _value[MAX_PARAM][VALUE_SIZE];
char host_name[256];
char remote_host_name[256];
int  _sock, _quit;

#ifndef O_BINARY
#define O_BINARY 0
#endif

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

int equal_ustring(char *str1, char *str2)
{
  if (str1 == NULL && str2 != NULL)
    return 0;
  if (str1 != NULL && str2 == NULL)
    return 0;
  if (str1 == NULL && str2 == NULL)
    return 1;

  while (*str1)
    if (toupper(*str1++) != toupper(*str2++))
      return 0;

  if (*str2)
    return 0;

  return 1;
}

void trim(char *str)
{
char *p1, *p2;

  if (!str || !str[0])
    return;

  p1 = p2 = str;
  while (*p1 == ' ')
    p1++;

  while (*p1)
    *p2++ = *p1++;
  *p2 = 0;

  p2 -= 1;
  while (p2 > str && (*p2 == ' ' || *p2 == '\n'))
    *p2-- = 0;
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

  for (i=0 ; i<MAX_PARAM && _param[i][0]; i++)
    if (equal_ustring(param, _param[i]))
      break;

  if (i<MAX_PARAM && _param[i][0])
    return _value[i];

  return NULL;
}

int isparam(char *param)
{
int i;

  for (i=0 ; i<MAX_PARAM && _param[i][0]; i++)
    if (equal_ustring(param, _param[i]))
      break;

  if (i<MAX_PARAM && _param[i][0])
    return 1;

  return 0;
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
    if (strchr(" %&=#", *p))
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

int read_paw(int pipe, char *delim, char *result)
{
fd_set readfds;
char   str[10000];
struct timeval timeout;
int    i;

  memset(str, 0, sizeof(str));

  do
    {
    FD_ZERO(&readfds);
    FD_SET(pipe, &readfds);

    timeout.tv_sec  = 5;
    timeout.tv_usec = 0;
    select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

    if (FD_ISSET(pipe, &readfds))
      {
      i = read(pipe, str+strlen(str), sizeof(str)-1-strlen(str));
      if (i <= 0)
        break;
      }

    if (strstr(str, delim))
      break;
    } while (1);

  strcpy(result, str);

  if (i <= 0)
    return 0;
  return 1;
}


int submit_paw(char *kumac, char *result)
{
#ifndef _MSC_VER
static int pid=0, pipe;

char   str[10000];
int    status;

  if (equal_ustring(kumac, "restart") && pid)
    {
    kill(pid, SIGKILL);
    close(pipe);
    pid = pipe = 0;
    }

  if (pid == 0)
    {
    if ((pid = forkpty(&pipe, str, NULL, NULL)) < 0)
      return 0;

    if (pid > 0)
      {
      /* wait for workstation question */
      memset(str, 0, sizeof(str));
      read_paw(pipe, "<CR>=1", str);
      
      /* select default workstation */
      sprintf(str, "1\n");
      write(pipe, str, 2);

      /* wait for prompt */
      read_paw(pipe, "PAW >", str);
      }
    }

  if (pid > 0)
    {
    /* parent */
    if (equal_ustring(kumac, "quit"))
      {
      write(pipe, str, strlen(str));
      close(pipe);
      return 2;
      }

    if (kumac[0])
      {
      strcpy(str, kumac);
      strcat(str, "; pict/print webpaw.gif\n");
      }
    else
      strcpy(str, "pict/print webpaw.gif\n");

    write(pipe, str, strlen(str));

    status = read_paw(pipe, "PAW >", str);
    if (status == 0)
      {
      /* restart PAW */
      pid = 0;
      close(pipe);
      return submit_paw(kumac, result);
      }

    if (strstr(str, "PAW >"))
      {
      *strstr(str, "PAW >") = 0;
      strcpy(result, str);
      return 1;
      }
    else
      return 0;
    }
  else
    {
    /* close inherited network socket */
    closesocket(_sock);

    /* start PAW */
    execlp("paw", "paw", NULL);
    }

#endif
  return 1;
}

/*------------------------------------------------------------------*/

void interprete(char *path)
/********************************************************************\

  Routine: interprete

  Purpose: Interprete parameters and generate HTML output

  Input:
    char *kumac             Kumac to execute

  <implicit>
    _param/_value array accessible via getparam()

\********************************************************************/
{
char   str[10000], *group_name, *display_name, *kumac_name;
char   cur_group[256], last_group_name[256];
FILE   *f;
int    fh, length, status;

  if (!path[0] && !getparam("submit") && !getparam("cmd"))
    {
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: WebPAW\r\n");
    rsprintf("Content-Type: text/html\r\n\r\n");

    /* return frameset if no path is given */
    rsprintf("<html>\r\n");
    rsprintf("<head><title>WebPAW on %s</title></head>\r\n\r\n", host_name);

    rsprintf("<frameset cols=\"300,*\">\r\n");
    rsprintf("  <frame name=kumacs src=kumacs.html marginwidth=1 marginheight=1 scrollling=auto>\r\n");
    rsprintf("  <frameset rows=\"25,*\" frameborder=no>\r\n");
    rsprintf("    <frame name=banner src=banner.html marginwidth=10 marginheight=1 scrolling=no>\r\n");
    rsprintf("    <frame name=contents src=contents.html marginwidth=10 marginheight=5 scrolling=auto>\r\n");
    rsprintf("  </frameset>\r\n");
    rsprintf("</frameset>\r\n\r\n");

    rsprintf("<noframes>\r\n");
    rsprintf("To view this page, your browser must support frames.\r\n");
    rsprintf("<AHREF=\"http://home.netscape.com/comprod/mirror/index.html\">Download</A>\r\n");
    rsprintf("Netscape Navigator 2.0 or later for frames support.\r\n");
    rsprintf("</noframes>\r\n\r\n");

    rsprintf("</html>\r\n");
    return;
    }

  /* display banner */
  if (equal_ustring(path, "banner.html"))
    {
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: WebPAW\r\n");
    rsprintf("Content-Type: text/html\r\n\r\n");
    rsprintf("<html><body>\r\n");

    /* title row */
    rsprintf("<b><a target=_top href=\"http://midas.psi.ch/webpaw/\">WebPAW</a> on %s</b>\r\n", 
              host_name);
    
    rsprintf("</body></html>\r\n");
    return;
    }

  /* display kumacs */
  if (strstr(path, "kumacs.html"))
    {
    rsprintf("HTTP/1.0 200 Document follows\r\n");
    rsprintf("Server: WebPAW\r\n");
    rsprintf("Content-Type: text/html\r\n\r\n");
    rsprintf("<html><body>\r\n");

    rsprintf("<form method=GET action=\"%s\" target=contents>\r\n", webpaw_url);

    /* display input field */
    rsprintf("<table border=0 cellpadding=1>\r\n");
    rsprintf("<tr><td colspan=2 align=center><input type=text name=cmd size=30 maxlength=256></tr>\r\n");
    rsprintf("<tr><td align=center><input type=submit name=submit value=\" Execute! \">\r\n");
    rsprintf("<td align=center><input type=submit name=restart value=\"Restart PAW!\">\r\n");
    rsprintf("</tr></table><hr>\r\n");

    f = fopen("webpaw.cfg", "rt");
    if (f == NULL)
      {
      rsprintf("</body></html>\r\n");
      return;
      }

    if (strchr(path, '/'))
      {
      strcpy(cur_group, path);
      *strchr(cur_group, '/') = 0;
      urlDecode(cur_group);
      }
    else
      cur_group[0] = 0;

    //rsprintf("<ul>\r\n");
    last_group_name[0] = 0;
    do
      {
      str[0] = 0;
      fgets(str, sizeof(str), f);
      if (str[0] && str[0] != '#' && strchr(str, '&'))
        {
        group_name = strtok(str, "&");
        display_name = strtok(NULL, "&");
        kumac_name = strtok(NULL, "&");
        trim(group_name);
        trim(display_name);
        trim(kumac_name);

        if (kumac_name == NULL)
          {
          kumac_name = display_name;
          display_name = group_name;
          group_name = NULL;
          }

        urlEncode(kumac_name);

        if (group_name == NULL)
          rsprintf("<li><a href=\"%s.gif\" target=contents>%s</a></li>\r\n", 
                    kumac_name, display_name);
        else
          {
          if (!equal_ustring(group_name, last_group_name))
            {
            /* finsish previous group */
            if (equal_ustring(last_group_name, cur_group) && last_group_name[0])
              rsprintf("</ul>\r\n");

            rsprintf("<li><a href=\"/%s/kumacs.html\" target=kumacs>%s</a></li>\r\n", 
                      group_name, group_name);
            strcpy(last_group_name, group_name);

            if (equal_ustring(group_name, cur_group))
              {
              rsprintf("<ul>\r\n");
              }
            }
          if (equal_ustring(group_name, cur_group))
            {
            rsprintf("<li><a href=\"/%s.gif\" target=contents>%s</a></li>\r\n", 
                      kumac_name, display_name);
            }
          }
        }
      } while (str[0]);

    if (equal_ustring(last_group_name, cur_group))
      rsprintf("</ul>\r\n");

    fclose(f);
    //rsprintf("</ul>\r\n");
    rsprintf("</body></html>\r\n");
    return;
    }

  /* forward command to paw and display result */
  if (equal_ustring(path, "contents.html") || 
      strstr(path, ".gif") ||
      getparam("submit") || getparam("cmd"))
    {
    if (getparam("restart"))
      strcpy(str, "restart");
    else if (getparam("cmd"))
      strcpy(str, getparam("cmd"));
    else
      strcpy(str, path);

    urlDecode(str);
    if (strstr(path, ".gif"))
      *strstr(str, ".gif") = 0;

    if (equal_ustring(path, "contents.html"))
      str[0] = 0;

#ifndef _MSC_VER
    status = submit_paw(str, str);
    if (status == 0)
      {
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: WebPAW\r\n");
      rsprintf("Content-Type: text/html\r\n\r\n");
      rsprintf("<html><body>Error talking to PAW, return string : <i>%s</i>\r\n", str);
      rsprintf("</body></html>\r\n");
      }
    else if (status == 2)
      {
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: WebPAW\r\n");
      rsprintf("Content-Type: text/html\r\n\r\n");
      rsprintf("<html><body>WebPAW shut down successfully\r\n");
      rsprintf("</body></html>\r\n");

      _quit = 1;
      return;
      }
    else
#endif
      {
      fh = open("webpaw.gif", O_RDONLY | O_BINARY);
      if (fh > 0)
        {
        length = lseek(fh, 0, SEEK_END);
        lseek(fh, 0, SEEK_SET);

        rsprintf("HTTP/1.0 200 Document follows\r\n");
        rsprintf("Server: WebPAW\r\n");
        rsprintf("Accept-Ranges: bytes\r\n");

        /* return proper header for file type */
        rsprintf("Content-Type: image/gif\r\n");
        rsprintf("Content-Length: %d\r\n", length);
        rsprintf("Pragma: no-cache\r\n");
        rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n\r\n");

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
      }

    return;
    }
}

/*------------------------------------------------------------------*/

void decode(char *string)
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
  
  interprete(path);
}

/*------------------------------------------------------------------*/

char net_buffer[WEB_BUFFER_SIZE];

void server_loop(int tcp_port, int daemon)
{
int                  status, i, n_error;
struct sockaddr_in   bind_addr, acc_addr;
int                  lsock, len, flag, header_length;
struct hostent       *phe;
struct linger        ling;
fd_set               readfds;
struct timeval       timeout;

  _quit = 0;

#ifdef _MSC_VER
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
               (char *) &flag, sizeof(int));
    status = bind(lsock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

    if (status < 0)
      {
      printf("Cannot bind to port %d.\nPlease try later or use the \"-p\" flag to specify a different port\n", tcp_port);
      return;
      }
    else
      printf("Warning: port %d already in use\n", tcp_port);
    }

#ifndef _MSC_VER
  /* give up root privilege */
  setuid(getuid());
  setgid(getgid());

  if (daemon)
    {
    int i, fd, pid;

    printf("Becoming a daemon...\n");

    if ( (pid = fork()) < 0)
      return;
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
        return;
      if (fd != i)
        return;
      }
  
    setsid();               /* become session leader */
    umask(0);               /* clear our file mode createion mask */
    }

#endif

  /* listen for connection */
  status = listen(lsock, SOMAXCONN);
  if (status < 0)
    {
    printf("Cannot listen\n");
    return;
    }

  /* set my own URL */
  if (tcp_port == 80)
    sprintf(webpaw_url, "http://%s/", host_name);
  else
    sprintf(webpaw_url, "http://%s:%d/", host_name, tcp_port);

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
      n_error = 0;
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

        if (i == 0)
          {
          n_error++;
          if (n_error == 100)
            goto error;
          }

        /* finish when empty line received */
        if (strstr(net_buffer, "GET") != NULL)
          {
          if (len>4 && strcmp(&net_buffer[len-4], "\r\n\r\n") == 0)
            break;
          if (len>6 && strcmp(&net_buffer[len-6], "\r\r\n\r\r\n") == 0)
            break;
          }
        else
          {
          printf(net_buffer);
          goto error;
          }

        } while (1);

      if (!strchr(net_buffer, '\r'))
        goto error;

      memset(return_buffer, 0, sizeof(return_buffer));
      return_length = 0;

      /* extract path and commands */
      *strchr(net_buffer, '\r') = 0;

      if (!strstr(net_buffer, "HTTP"))
        goto error;
      *(strstr(net_buffer, "HTTP")-1)=0;

      /* decode command and return answer */
      decode(net_buffer+4);

      if (return_length != -1)
        {
        if (return_length == 0)
          return_length = strlen(return_buffer)+1;

        send(_sock, return_buffer, return_length, 0);

  error:

        closesocket(_sock);
        }
      }

    } while (!_quit);
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
int i;
int tcp_port = 80, daemon = 0;

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = 1;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'p')
        tcp_port = atoi(argv[++i]);
      else
        {
usage:
        printf("usage: %s [-p port] [-D]\n\n", argv[0]);
        printf("       -D become a daemon\n");
        return 0;
        }
      }
    }
  
  server_loop(tcp_port, daemon);

  return 0;
}

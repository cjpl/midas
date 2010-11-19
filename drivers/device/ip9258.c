/********************************************************************\

  Name:         ip9258.c
  Created by:   Stefan Ritt

  Contents:     Device Driver for IP Power 9268 Ethernoet power switch

  $Id:$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "midas.h"
#include "msystem.h"

/*---- globals -----------------------------------------------------*/

typedef struct {
   char address[32];
   char username[32];
   char password[32];
} IP9258_SETTINGS;

#define IP9258_SETTINGS_STR "\
Address = STRING : [32] x.x.x.x\n\
Username = STRING : [32] \n\
Password = STRING : [32] \n\
"
typedef struct {
   IP9258_SETTINGS ip9258_settings;
   INT num_channels;
   float *var_cache;
} IP9258_INFO;

static DWORD last_update;

INT ip9258_get(IP9258_INFO * info, INT channel, float *pvalue);

/*-------------------------------------------------------------------*/

char *map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int cind(char c)
{
   int i;

   if (c == '=')
      return 0;

   for (i = 0; i < 64; i++)
      if (map[i] == c)
         return i;

   return -1;
}

void base64_encode(unsigned char *s, unsigned char *d, int size)
{
   unsigned int t, pad;
   unsigned char *p;

   pad = 3 - strlen((char *) s) % 3;
   if (pad == 3)
      pad = 0;
   p = d;
   while (*s) {
      t = (*s++) << 16;
      if (*s)
         t |= (*s++) << 8;
      if (*s)
         t |= (*s++) << 0;

      *(d + 3) = map[t & 63];
      t >>= 6;
      *(d + 2) = map[t & 63];
      t >>= 6;
      *(d + 1) = map[t & 63];
      t >>= 6;
      *(d + 0) = map[t & 63];

      d += 4;
      if (d - p >= size - 3)
         return;
   }
   *d = 0;
   while (pad--)
      *(--d) = '=';
}

/*---- network TCP connection --------------------------------------*/

static INT tcp_connect(char *host, int port, int *sock)
{
   struct sockaddr_in bind_addr;
   struct hostent *phe;
   int status;
   char str[256];

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

   /* create a new socket for connecting to remote server */
   *sock = socket(AF_INET, SOCK_STREAM, 0);
   if (*sock == -1) {
      mfe_error("Cannot create socket");
      return FE_ERR_HW;
   }

   /* let OS choose any port number */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = 0;

   status = bind(*sock, (void *) &bind_addr, sizeof(bind_addr));
   if (status < 0) {
      mfe_error("Cannot bind");
      closesocket(*sock);
      return RPC_NET_ERROR;
   }

   /* connect to remote node */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = htons((short) port);

#ifdef OS_VXWORKS
   {
      INT host_addr;

      host_addr = hostGetByName(host);
      memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   }
#else
   phe = gethostbyname(host);
   if (phe == NULL) {
      mfe_error("Cannot get host name");
      closesocket(*sock);
      return RPC_NET_ERROR;
   }
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

#ifdef OS_UNIX
   do {
      status = connect(*sock, (void *) &bind_addr, sizeof(bind_addr));

      /* don't return if an alarm signal was cought */
   } while (status == -1 && errno == EINTR);
#else
   status = connect(*sock, (void *) &bind_addr, sizeof(bind_addr));
#endif

   if (status != 0) {
      closesocket(*sock);
      *sock = -1;
      sprintf(str, "Cannot connect to host %s", host);
      mfe_error(str);
      return FE_ERR_HW;
   }

   return FE_SUCCESS;
}

/*---- device driver routines --------------------------------------*/

INT ip9258_init(HNDLE hKey, void **pinfo, INT channels)
{
   int status, i, size;
   HNDLE hDB;
   char str[1024];
   IP9258_INFO *info;
   float value;

   /* allocate info structure */
   info = calloc(1, sizeof(IP9258_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create IP9258 settings record */
   status = db_create_record(hDB, hKey, "", IP9258_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->ip9258_settings);
   db_get_record(hDB, hKey, &info->ip9258_settings, &size, 0);

   /* initialize driver */
   info->num_channels = channels;
   info->var_cache = calloc(channels, sizeof(float));

   /* check ip address */
   if (strcmp(info->ip9258_settings.address, "x.x.x.x") == 0) {
      db_get_path(hDB, hKey, str, sizeof(str));
      cm_msg(MERROR, "ip9258_init", "Please set address for %s", str);
      return FE_ERR_ODB;
   }

   /* check username and password */
   if (info->ip9258_settings.username[0] == 0 || info->ip9258_settings.password[0] == 0) {
      db_get_path(hDB, hKey, str, sizeof(str));
      cm_msg(MERROR, "ip9258_init", "Please set username and password for %s", str);
      return FE_ERR_ODB;
   }

   for (i = 0; i < channels; i++) {
      status = ip9258_get(info, i, &value);
      if (status != FE_SUCCESS) {
         cm_msg(MERROR, "ip9258_init", "Cannot connect to IP9258 device at %s", info->ip9258_settings.address);
         return FE_ERR_HW;
      }
   }

   last_update = ss_time();
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT ip9258_exit(IP9258_INFO * info)
{
   if (info->var_cache)
      free(info->var_cache);

   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT ip9258_set(IP9258_INFO * info, INT channel, float value)
{
   INT status, sock;
   char req[1000], str[256], enc[256];

   status = tcp_connect(info->ip9258_settings.address, 80, &sock);
   if (status != FE_SUCCESS)
      return status;

   sprintf(req, "GET /Set.cmd?CMD=SetPower+P6%d=%d HTTP/1.1\r\n", channel, (int)value);
   strcat(req, "Accept: text/html\r\n");
   sprintf(str, "%s:%s", info->ip9258_settings.username, info->ip9258_settings.password);
   base64_encode((unsigned char*)str, (unsigned char*)enc, sizeof(enc));
   sprintf(req+strlen(req), "Authorization: Basic %s\r\n\r\n", enc);
   send(sock, req, strlen(req), 0);
   recv_string(sock, req, sizeof(req), 10000);
   closesocket(sock);

   if (strstr(req, "200 OK")) {
      info->var_cache[channel] = (float)(int)value;
      return FE_SUCCESS;
   }

   if (strstr(req, "Unauthorized")) 
      sprintf(str, "Authorization failure for IP9258 at %s", info->ip9258_settings.address);
   else
      sprintf(str, "Error writing to IP9258 at %s: %s", info->ip9258_settings.address, req);
   mfe_error(str);

   return FE_ERR_HW;
}

/*----------------------------------------------------------------------------*/

INT ip9258_get(IP9258_INFO * info, INT channel, float *pvalue)
{
   INT status, sock, i, retry;
   char req[1000], str[256], enc[256];

   if (ss_time() - last_update > 60) {
      last_update = ss_time();

      for (retry=0 ; retry<10 ; retry++) {
         status = tcp_connect(info->ip9258_settings.address, 80, &sock);
         if (status != FE_SUCCESS)
            return status;

         strcpy(req, "GET /Set.cmd?CMD=GetPower HTTP/1.1\r\n");
         strcat(req, "Accept: text/html\r\n");
         sprintf(str, "%s:%s", info->ip9258_settings.username, info->ip9258_settings.password);
         base64_encode((unsigned char*)str, (unsigned char*)enc, sizeof(enc));
         sprintf(req+strlen(req), "Authorization: Basic %s\r\n\r\n", enc);
         send(sock, req, strlen(req), 0);
         recv_string(sock, req, sizeof(req), 10000);
         if (strstr(req, "HTTP/1.0 200 OK")) {
            recv_string(sock, req, sizeof(req), 1000);
            recv_string(sock, req, sizeof(req), 1000);
            recv_string(sock, req, sizeof(req), 1000);
            for (i=0 ; i<info->num_channels ; i++)
               info->var_cache[i] = (float)atoi(req+10+i*6);
            closesocket(sock);
            *pvalue = info->var_cache[channel];
            return FE_SUCCESS;
         }  else {
            if (strstr(req, "Unauthorized")) {
               sprintf(str, "Authorization failure for IP9258 at %s", info->ip9258_settings.address);
               mfe_error(str);
               for (i=0 ; i<info->num_channels ; i++)
                  info->var_cache[i] = (float)ss_nan();
               closesocket(sock);
               return FE_ERR_HW;
            }
         }
         closesocket(sock);
      }

      sprintf(str, "Error reading from IP9258 at %s", info->ip9258_settings.address);
      for (i=0 ; i<info->num_channels ; i++)
         info->var_cache[i] = (float)ss_nan();
      *pvalue = (float)ss_nan();
      return FE_ERR_HW;
   } else
      *pvalue = info->var_cache[channel];

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT ip9258_get_default_threshold(IP9258_INFO * info, INT channel, float *pvalue)
{
   *pvalue = 0.5f;
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT ip9258_get_label(IP9258_INFO * info, INT channel, char *name)
{
   sprintf(name, "Power %d", channel);
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT ip9258(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   void *info;
   char *name;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      status = ip9258_init(hKey, info, channel);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = ip9258_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = ip9258_set(info, channel, value);
      break;

   case CMD_GET:
   case CMD_GET_DEMAND:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = ip9258_get(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = ip9258_get_label(info, channel, name);
      break;

   case CMD_GET_THRESHOLD:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = ip9258_get_default_threshold(info, channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

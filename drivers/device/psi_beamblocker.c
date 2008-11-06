/********************************************************************\

  Name:         psi_beamblocker.c
  Created by:   Stefan Ritt

  Contents:     Device Driver for PSI beam blocker through 
                Urs Rohrer's beamline control
                (http://people.web.psi.ch/rohrer_u/secblctl.htm)

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "midas.h"
#include "msystem.h"

/*---- globals -----------------------------------------------------*/

typedef struct {
   char beamline_pc[32];
   int port;
   int ncom;
   int acom;
   int com_open;
   int com_close;
   int nstat;
   int astat;
   int stat_open;
   int stat_closed;
   int stat_psa;
} PSI_BEAMBLOCKER_SETTINGS;

// muE1: pc202
// piM1: pc235
// piE1: pc130
// piE3: pc144
// piM3: pc231
// muE4: pc451
// piE5: pc98

/* 
PiE1:
Device Server NCom  ACom ComOpen ComClose NStat AStat StatOpen StatClosed StatPSA  
KSG51  PC130  15    1    1       2        16    0     1        2          4

PiE5:
Device Server NCom  ACom ComOpen ComClose NStat AStat StatOpen StatClosed StatPSA  
KSF41  PC98   17    1    1       2        18    0     1        2          4
KV41   PC98   17    1    4       8        18    0     8        16         32
*/

#define PSI_BEAMBLOCKER_SETTINGS_STR "\
Beamline PC = STRING : [32] PC98\n\
Port = INT : 10000\n\
NCom = INT : 17\n\
ACom = INT : 1\n\
Com Open = INT : 1\n\
Com Close = INT : 2\n\
NStat = INT : 18\n\
AStat = INT : 0\n\
Stat Open = INT : 1\n\
Stat Closed = INT : 2\n\
Stat PSA = INT : 4\n\
"

typedef struct {
   PSI_BEAMBLOCKER_SETTINGS psi_beamblocker_settings;
   float open;
   float psa;
   INT sock;
} PSI_BEAMBLOCKER_INFO;

INT psi_beamblocker_get(PSI_BEAMBLOCKER_INFO * info, INT channel, float *pvalue);

static DWORD last_reconnect;

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
      sprintf(str, "Cannot find host \"%s\"", host);
      mfe_error(str);
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
      sprintf(str, "Cannot connect to \"%s\"", host);
      mfe_error(str);
      return FE_ERR_HW;
   }

   return FE_SUCCESS;
}

/*---- device driver routines --------------------------------------*/

INT psi_beamblocker_init(HNDLE hKey, void **pinfo, INT channels)
{
   int status, size;
   HNDLE hDB;
   PSI_BEAMBLOCKER_INFO *info;
   float value;

   /* only allow single channel */
   if (channels != 1 && channels != 2) {
      cm_msg(MERROR, "psi_beamblocker_init", "Only one or two channels possible for beam blocker");
      return FE_ERR_HW;
   }

   /* allocate info structure */
   info = calloc(1, sizeof(PSI_BEAMBLOCKER_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create PSI_BEAMBLOCKER settings record */
   status = db_create_record(hDB, hKey, "", PSI_BEAMBLOCKER_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->psi_beamblocker_settings);
   db_get_record(hDB, hKey, &info->psi_beamblocker_settings, &size, 0);

   /* contact beamline pc */
   printf("%s...", info->psi_beamblocker_settings.beamline_pc);
   status = tcp_connect(info->psi_beamblocker_settings.beamline_pc,
                        info->psi_beamblocker_settings.port, &info->sock);
   if (status != FE_SUCCESS) {
      printf("error\n");
      return status;
   }

   /* get initial state */
   psi_beamblocker_get(info, 0, &value);

   printf("ok\n");

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT psi_beamblocker_exit(PSI_BEAMBLOCKER_INFO * info)
{
   closesocket(info->sock);

   free(info);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

static DWORD last_action;
static int last_demand;

INT psi_beamblocker_set(PSI_BEAMBLOCKER_INFO * info, INT channel, float value)
{
   char str[80];
   INT status;

   if (channel == 1) {
      mfe_error("Cannot set PSA status. PSA status can only be read.");
      return FE_SUCCESS;        /* cannot change PSA status */
   }

   sprintf(str, "WCAM %d %d 16 %d", info->psi_beamblocker_settings.ncom,
           info->psi_beamblocker_settings.acom,
           ((int) value) ==
           1 ? info->psi_beamblocker_settings.com_open : info->psi_beamblocker_settings.com_close);

   status = send(info->sock, str, strlen(str) + 1, 0);
   if (status < 0) {
      sprintf(str, "Cannot read data from %s", info->psi_beamblocker_settings.beamline_pc);
      mfe_error(str);
      return FE_ERR_HW;
   }
   recv_string(info->sock, str, sizeof(str), 500);

   last_demand = (int) value;
   last_action = ss_time();

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT psi_beamblocker_get(PSI_BEAMBLOCKER_INFO * info, INT channel, float *pvalue)
{
   INT status, i;
   char str[80];
   static DWORD last_update = 0;

   /* update every 5 seconds */
   if (ss_time() - last_update >= 5) {
      last_update = ss_time();

      sprintf(str, "RCAM %d %d 0", info->psi_beamblocker_settings.nstat,
              info->psi_beamblocker_settings.astat);
      status = send(info->sock, str, strlen(str) + 1, 0);
      if (status <= 0) {
         if (info->sock > 0) {
            closesocket(info->sock);
            info->sock = -1;
         }

         /* try to reconnect every minute */
         if (ss_time() - last_reconnect >= 60) {
            last_reconnect = ss_time();
            status = tcp_connect(info->psi_beamblocker_settings.beamline_pc,
                                 info->psi_beamblocker_settings.port, &info->sock);

            if (status != FE_SUCCESS)
               return FE_ERR_HW;
         } else
            return FE_SUCCESS;
      }

      memset(str, 0, sizeof(str));
      status = recv_string(info->sock, str, sizeof(str), 3000);
      if (status <= 0) {
         if (info->sock > 0) {
            closesocket(info->sock);
            info->sock = -1;
         }

         /* try to reconnect every minute */
         if (ss_time() - last_reconnect >= 60) {
            last_reconnect = ss_time();
            status = tcp_connect(info->psi_beamblocker_settings.beamline_pc,
                                 info->psi_beamblocker_settings.port, &info->sock);

            if (status != FE_SUCCESS)
               return FE_ERR_HW;

            return FE_SUCCESS;
         } else
            return FE_SUCCESS;
      }

      if (strstr(str, "*RCAM*")) {
         /* skip *RCAM* name */
         for (i = 0; i < (int) strlen(str) && str[i] != ' '; i++)
            i++;

         /* skip X&Q */
         for (i++; i < (int) strlen(str) && str[i] != ' '; i++);
         i++;

         /* round measured to four digits */
         status = atoi(str + i);

         if ((status & info->psi_beamblocker_settings.stat_open) > 0)
            info->open = 1.f;
         else if ((status & info->psi_beamblocker_settings.stat_closed) > 0)
            info->open = 0.f;
         else
            info->open = 0.5f;

         info->psa = (float) ((status & info->psi_beamblocker_settings.stat_psa) > 0);

      }
   } else
      ss_sleep(10);             // don't eat all CPU

   if (channel == 0)
      *pvalue = (float) info->open;
   else
      *pvalue = (float) info->psa;

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT psi_beamblocker_get_demand(PSI_BEAMBLOCKER_INFO * info, INT channel, float *pvalue)
{
   if (ss_time() - last_action < 5)
      *pvalue = (float) last_demand;
   else
      *pvalue = info->open;

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT psi_beamblocker(INT cmd, ...)
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
      status = psi_beamblocker_init(hKey, info, channel);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = psi_beamblocker_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = psi_beamblocker_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_beamblocker_get(info, channel, pvalue);
      break;

   case CMD_GET_DEMAND:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_beamblocker_get_demand(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      if (channel == 0)
         strcpy(name, "Beam Blocker (1=open)");
      else
         strcpy(name, "PSA (1=ready)");
      status = FE_SUCCESS;
      break;

   case CMD_GET_THRESHOLD:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 0.1f;
      status = FE_SUCCESS;
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

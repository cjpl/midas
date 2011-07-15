/********************************************************************\

  Name:         psi_beamline.c
  Created by:   Stefan Ritt

  Contents:     Device Driver for Urs Rohrer's beamline control at PSI
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
} PSI_BEAMLINE_SETTINGS;

// muE1: pc202
// piM1: pc235
// piE1: pc130
// piE3: pc144
// piM3: pc231
// muE4: pc451
// piE5: pc98

#define PSI_BEAMLINE_SETTINGS_STR "\
Beamline PC = STRING : [32] PCxxx\n\
Port = INT : 10000\n\
"
typedef struct {
   PSI_BEAMLINE_SETTINGS psi_beamline_settings;
   INT num_channels;
   char *name;
   float *demand;
   float *measured;
   INT sock;
} PSI_BEAMLINE_INFO;

static DWORD last_update;
static DWORD last_reconnect;

#define MAX_ERROR        5      // send error after this amount of failed read attempts

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
      mfe_error("cannot create socket");
      return FE_ERR_HW;
   }

   /* let OS choose any port number */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = 0;

   status = bind(*sock, (void *) &bind_addr, sizeof(bind_addr));
   if (status < 0) {
      mfe_error("cannot bind");
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
      mfe_error("cannot get host name");
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
      sprintf(str, "cannot connec to host %s", host);
      mfe_error(str);
      return FE_ERR_HW;
   }

   return FE_SUCCESS;
}

/*---- device driver routines --------------------------------------*/

INT psi_beamline_init(HNDLE hKey, void **pinfo, INT channels)
{
   int status, i, j, size;
   HNDLE hDB;
   char str[1024];
   PSI_BEAMLINE_INFO *info;

   /* allocate info structure */
   info = calloc(1, sizeof(PSI_BEAMLINE_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create PSI_BEAMLINE settings record */
   status = db_create_record(hDB, hKey, "", PSI_BEAMLINE_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->psi_beamline_settings);
   db_get_record(hDB, hKey, &info->psi_beamline_settings, &size, 0);

   /* initialize driver */
   info->num_channels = channels;
   info->name = calloc(channels, NAME_LENGTH);
   info->demand = calloc(channels, sizeof(float));
   info->measured = calloc(channels, sizeof(float));

   /* check beamline pc name */
   if (strcmp(info->psi_beamline_settings.beamline_pc, "PCxxx") == 0) {
      db_get_path(hDB, hKey, str, sizeof(str));
      cm_msg(MERROR, "psi_beamline_init", "Please set \"%s/Beamline PC\"", str);
      return FE_ERR_ODB;
   }
   /* contact beamline pc */
   printf("%s...", info->psi_beamline_settings.beamline_pc);
   status = tcp_connect(info->psi_beamline_settings.beamline_pc,
                        info->psi_beamline_settings.port, &info->sock);
   if (status != FE_SUCCESS) {
      printf("error\n");
      return FE_ERR_HW;
   }

   /* switch combis on */
   send(info->sock, "SWON", 5, 0);
   status = recv_string(info->sock, str, sizeof(str), 10000);
   if (status <= 0) {
      cm_msg(MERROR, "psi_beamline_init", "cannot retrieve data from %s",
             info->psi_beamline_settings.beamline_pc);
      return FE_ERR_HW;
   }

   /* get channel names and initial values */
   status = send(info->sock, "RALL", 5, 0);
   if (status <= 0) {
      cm_msg(MERROR, "psi_beamline_init", "cannot retrieve data from %s",
             info->psi_beamline_settings.beamline_pc);
      return FE_ERR_HW;
   }

   status = recv_string(info->sock, str, sizeof(str), 10000);
   if (status <= 0) {
      cm_msg(MERROR, "psi_beamline_init", "cannot retrieve data from %s",
             info->psi_beamline_settings.beamline_pc);
      return FE_ERR_HW;
   }

   for (i = 0; i < channels; i++) {
      recv_string(info->sock, str, sizeof(str), 10000);

      for (j = 0; j < (int) strlen(str) && str[j] != ' '; j++)
         info->name[i * NAME_LENGTH + j] = str[j];
      info->name[i * NAME_LENGTH + j] = 0;

      info->demand[i] = (float) atof(str + j + 1);

      for (j++; j < (int) strlen(str) && str[j] != ' '; j++);

      /* round measured to four digits */
      info->measured[i] = (float) ((int) (atof(str + j + 1) * 10000) / 10000.0);
   }

   last_update = ss_time();
   printf("ok\n");

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_beamline_exit(PSI_BEAMLINE_INFO * info)
{
   closesocket(info->sock);

   if (info->name)
      free(info->name);

   if (info->demand)
      free(info->demand);

   if (info->measured)
      free(info->measured);

   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_beamline_set(PSI_BEAMLINE_INFO * info, INT channel, float value)
{
   char str[80];
   INT status;

   sprintf(str, "WDAC %s %d", info->name + channel * NAME_LENGTH, (int) value);
   status = send(info->sock, str, strlen(str) + 1, 0);
   if (status < 0) {
      sprintf(str, "cannot read data from %s", info->psi_beamline_settings.beamline_pc);
      mfe_error(str);
      return FE_ERR_HW;
   }
   recv_string(info->sock, str, sizeof(str), 10000);

   info->demand[channel] = value;

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_beamline_rall(PSI_BEAMLINE_INFO * info)
{
   INT status, i, j;
   char str[1024];
   static int error_count = 0;

   if (ss_time() - last_update > 10) {
      last_update = ss_time();

      status = send(info->sock, "RALL", 5, 0);
      if (status <= 0) {
         if (info->sock > 0) {
            closesocket(info->sock);
            info->sock = -1;
         }

         /* try to reconnect every 10 minutes */
         if (ss_time() - last_reconnect > 600) {
            last_reconnect = ss_time();
            status = tcp_connect(info->psi_beamline_settings.beamline_pc,
                                 info->psi_beamline_settings.port, &info->sock);

            if (status != FE_SUCCESS)
               return FE_ERR_HW;
         } else
            return FE_SUCCESS;
      }

      status = recv_string(info->sock, str, sizeof(str), 10000);
      if (status <= 0) {
         error_count++;
         if (error_count > MAX_ERROR) {
            sprintf(str, "cannot retrieve data from %s", info->psi_beamline_settings.beamline_pc);
            mfe_error(str);
         }
         return FE_ERR_HW;
      }
      error_count = 0;

      for (i = 0; i < info->num_channels; i++) {
         recv_string(info->sock, str, sizeof(str), 10000);

         /* skip name */
         for (j = 0; j < (int) strlen(str) && str[j] != ' '; j++)

            /* extract demand value */
            info->demand[i] = (float) atof(str + j + 1);
         for (j++; j < (int) strlen(str) && str[j] != ' '; j++);

         /* round measured to four digits */
         info->measured[i] = (float) ((int) (atof(str + j + 1) * 10000) / 10000.0);
      }
   } else
      ss_sleep(10);             // don't eat all CPU

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_beamline_get(PSI_BEAMLINE_INFO * info, INT channel, float *pvalue)
{
   INT status;

   status = psi_beamline_rall(info);
   *pvalue = info->measured[channel];

   return status;
}

/*----------------------------------------------------------------------------*/

INT psi_beamline_get_demand(PSI_BEAMLINE_INFO * info, INT channel, float *pvalue)
{
   INT status;

   status = psi_beamline_rall(info);
   *pvalue = info->demand[channel];

   return status;
}

/*----------------------------------------------------------------------------*/

INT psi_beamline_get_default_threshold(PSI_BEAMLINE_INFO * info, INT channel, float *pvalue)
{
   /* threshold is 2 bit in 12 bits */
   *pvalue = 0.0005f;
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_beamline_get_label(PSI_BEAMLINE_INFO * info, INT channel, char *name)
{
   strcpy(name, info->name + channel * NAME_LENGTH);
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT psi_beamline(INT cmd, ...)
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
      status = psi_beamline_init(hKey, info, channel);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = psi_beamline_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = psi_beamline_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_beamline_get(info, channel, pvalue);
      break;

   case CMD_GET_DEMAND:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_beamline_get_demand(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = psi_beamline_get_label(info, channel, name);
      break;

   case CMD_GET_THRESHOLD:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_beamline_get_default_threshold(info, channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

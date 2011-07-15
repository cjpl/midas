/********************************************************************\

  Name:         psi_cobra.c
  Created by:   Stefan Ritt

  Contents:     Device Driver for MEG COBRA magnet at PSI.

  $Id: mscbdev.c 3428 2006-12-06 08:49:38Z ritt $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "midas.h"

#ifndef _MSC_VER
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#endif

/*---- globals -----------------------------------------------------*/

/* multicast address */

typedef struct {
   char multicast_group[32];
} PSI_COBRA_SETTINGS;

#define PSI_COBRA_SETTINGS_STR "\
Multicast Group = STRING : [32] 239.208.0.1\n\
"

typedef struct {
   PSI_COBRA_SETTINGS psi_cobra_settings;
   int sock;
} PSI_COBRA_INFO;

/*---- device driver routines --------------------------------------*/

INT psi_cobra_init(HNDLE hKey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
   int status, size, n;
   HNDLE hDB;
   PSI_COBRA_INFO *info;
   struct sockaddr_in myaddr;
   struct ip_mreq req;

   /* allocate info structure */
   info = calloc(1, sizeof(PSI_COBRA_INFO));
   *pinfo = info;
   
   cm_get_experiment_database(&hDB, NULL);

   /* create PSI_COBRA settings record */
   status = db_create_record(hDB, hKey, "", PSI_COBRA_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->psi_cobra_settings);
   db_get_record(hDB, hKey, &info->psi_cobra_settings, &size, 0);

   info->sock = socket(AF_INET, SOCK_DGRAM, 0);

   n = 1;
   if (setsockopt(info->sock, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(int)) < 0) {
      cm_msg(MERROR, "psi_cobra_init", "setsockopt SO_REUSEADDR faild with errno %d", errno);
      return FE_ERR_HW;
   }

   /* bind to port 1178 to receive cobra current broadcasts */
   memset(&myaddr, 0, sizeof(myaddr));
   myaddr.sin_family = AF_INET;
   myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   myaddr.sin_port = htons(1178);
   if (bind(info->sock, (struct sockaddr *)&myaddr, sizeof(myaddr))) {
      cm_msg(MERROR, "psi_cobra_init", "bind faild with errno %d", errno);
      return FE_ERR_HW;
   }

   /* set multicast group */
   memset(&req, 0, sizeof(req));
   req.imr_multiaddr.s_addr = inet_addr(info->psi_cobra_settings.multicast_group);
   req.imr_interface.s_addr = INADDR_ANY;
   
   if (setsockopt(info->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&req, sizeof(req)) < 0) {
      cm_msg(MERROR, "psi_cobra_init", "setsockopt IP_ADD_MEMBERSHIP faild with errno %d", errno);
      return FE_ERR_HW;
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_cobra_exit(PSI_COBRA_INFO * info)
{
   if (info)
      free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_cobra_get(PSI_COBRA_INFO * info, INT channel, float *pvalue)
{
char str[256];
struct sockaddr_in addr;
unsigned int size, n;
fd_set readfds;
struct timeval timeout;
static float value_ch0, value_ch1;
static int first = 1;

   if (channel == 0) {

      str[0] = 0;
      /* continue until nothing received to drain old messages */
      do {
         FD_ZERO(&readfds);
         FD_SET(info->sock, &readfds);

         timeout.tv_sec = 0;
         timeout.tv_usec = 10000; // don't eat all CPU

         select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

         if (FD_ISSET(info->sock, &readfds)) {
            memset(str, 0, sizeof(str));
            size = sizeof(addr);
            n = recvfrom(info->sock, str, sizeof(str), 0, (struct sockaddr *)&addr, &size);
            if (n > 0) {
               value_ch0 = (float)atof(str+10);
               value_ch1 = (float)atof(str+31);
               first = 0;
            }
         } else {
            if (!first)
               break;
         }
      } while (1);

      *pvalue = value_ch0;
   } else
      *pvalue = value_ch1;

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT psi_cobra(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   DWORD flags;
   float *pvalue;
   void *info, *bd;
   char *name;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      flags = va_arg(argptr, DWORD);
      bd = va_arg(argptr, void *);
      status = psi_cobra_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = psi_cobra_exit(info);
      break;

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_cobra_get(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      if (channel == 0)
         strcpy(name, "SC current (A)");
      else
         strcpy(name, "NC current (A)");
      status = FE_SUCCESS;
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

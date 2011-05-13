/********************************************************************\

  Name:         psi_accel.c
  Created by:   Stefan Ritt

  Contents:     Device Driver for PSI 590 MeV proton accelerator

  $Id: mscbdev.c 3428 2006-12-06 08:49:38Z ritt $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "midas.h"
#include "msystem.h"

#ifndef _MSC_VER
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#endif

/*---- globals -----------------------------------------------------*/

/* multicast address */

typedef struct {
   char multicast_host[32];
} PSI_ACCEL_SETTINGS;

#define PSI_ACCEL_SETTINGS_STR "\
Multicast Host = STRING : [32] hipa129-sta\n\
"

typedef struct {
   PSI_ACCEL_SETTINGS psi_accel_settings;
   int sock;
} PSI_ACCEL_INFO;

/*---- device driver routines --------------------------------------*/

INT psi_accel_init(HNDLE hKey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
   int status, size, n;
   char str[80];
   HNDLE hDB;
   PSI_ACCEL_INFO *info;
   struct sockaddr_in myaddr;
   struct ip_mreq req;
   struct hostent *ph;

   /* allocate info structure */
   info = (PSI_ACCEL_INFO *)calloc(1, sizeof(PSI_ACCEL_INFO));
   *pinfo = info;
   
   cm_get_experiment_database(&hDB, NULL);

   /* create PSI_BEAMBLOCKER settings record */
   status = db_create_record(hDB, hKey, "", PSI_ACCEL_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->psi_accel_settings);
   db_get_record(hDB, hKey, &info->psi_accel_settings, &size, 0);

   info->sock = socket(AF_INET, SOCK_DGRAM, 0);

   n = 1;
   if (setsockopt(info->sock, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(int)) < 0) {
      cm_msg(MERROR, "psi_accel_init", "setsockopt SO_REUSEADDR faild with errno %d", errno);
      return FE_ERR_HW;
   }

   /* bind to port 43979 to receive accelerator current broadcasts */
   memset(&myaddr, 0, sizeof(myaddr));
   myaddr.sin_family = AF_INET;
   myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   myaddr.sin_port = htons(43979);
   if (bind(info->sock, (struct sockaddr *)&myaddr, sizeof(myaddr))) {
      cm_msg(MERROR, "psi_accel_init", "bind faild with errno %d", errno);
      return FE_ERR_HW;
   }

   /* set multicast group */
   memset(&req, 0, sizeof(req));
   ph = gethostbyname(info->psi_accel_settings.multicast_host);
   if (ph == NULL) {
      cm_msg(MERROR, "psi_accel_init", "cannot retrieve IP address of %s", info->psi_accel_settings.multicast_host);
      return FE_ERR_HW;
   }
   sprintf(str, "226.%d.%d.%d", ph->h_addr[1]&0xFF, ph->h_addr[2]&0xFF, ph->h_addr[3]&0xFF);
   req.imr_multiaddr.s_addr = inet_addr(str);
   req.imr_interface.s_addr = INADDR_ANY;
   
   if (setsockopt(info->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&req, sizeof(req)) < 0) {
      cm_msg(MERROR, "psi_accel_init", "setsockopt IP_ADD_MEMBERSHIP faild with errno %d", errno);
      return FE_ERR_HW;
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_accel_exit(PSI_ACCEL_INFO * info)
{
   if (info)
      free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_accel_get(PSI_ACCEL_INFO * info, INT channel, float *pvalue)
{
char str[10000];
struct sockaddr_in addr;
unsigned int size, i, n;
fd_set readfds;
struct timeval timeout;

   str[0] = 0;
   n = 0;
   /* continue until nothing received to drain old messages */
   for (i=0 ; i<120 ; i++) { // maximum 120*10ms = 1.2s
      FD_ZERO(&readfds);
      FD_SET(info->sock, &readfds);

      timeout.tv_sec = 0;
      timeout.tv_usec = 10000; // don't eat all CPU

      select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

      if (!FD_ISSET(info->sock, &readfds) && n > 0)
         break;

      if (FD_ISSET(info->sock, &readfds)) {
         memset(str, 0, sizeof(str));
         size = sizeof(addr);
         n = recvfrom(info->sock, str, sizeof(str), 0, (struct sockaddr *)&addr, &size);
      }
   } 

   if (strstr(str, "MHC3"))
      *pvalue = (float)atof(strstr(str, "MHC3")+15);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT psi_accel(INT cmd, ...)
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
      status = psi_accel_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = psi_accel_exit(info);
      break;

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_accel_get(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      strcpy(name, "Proton cur. (mA)");
      status = FE_SUCCESS;
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

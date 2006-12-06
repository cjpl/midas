/********************************************************************\

  Name:         mscbhvr.c
  Created by:   Stefan Ritt

  Contents:     MSCB HVR-400/500 High Voltage Device Driver

  $Id: mscbhvr.c 2753 2005-10-07 14:55:31Z ritt $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "midas.h"
#include "mscb.h"

/*---- globals -----------------------------------------------------*/

typedef struct {
   char mscb_device[NAME_LENGTH];
   char pwd[NAME_LENGTH];
   int  base_address;
   BOOL debug;
} MSCBHVR_SETTINGS;

#define MSCBHVR_SETTINGS_STR "\
MSCB Device = STRING : [32] usb0\n\
MSCB Pwd = STRING : [32] \n\
Base Address = INT : 0\n\
Debug = BOOL : 0\n\
"

typedef struct {
   MSCBHVR_SETTINGS settings;
   int num_channels;
   int fd;
} MSCBHVR_INFO;

/*---- device driver routines --------------------------------------*/

INT mscbhvr_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
   int  i, status, size, flag;
   HNDLE hDB, hkeydd;
   MSCBHVR_INFO *info;
   MSCB_INFO node_info;

   /* allocate info structure */
   info = (MSCBHVR_INFO *) calloc(1, sizeof(MSCBHVR_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create MSCBHVR settings record */
   status = db_create_record(hDB, hkey, "DD", MSCBHVR_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   db_find_key(hDB, hkey, "DD", &hkeydd);
   size = sizeof(info->settings);
   db_get_record(hDB, hkeydd, &info->settings, &size, 0);

   info->num_channels = channels;

   /* open device on MSCB */
   info->fd = mscb_init(info->settings.mscb_device, NAME_LENGTH, info->settings.pwd, info->settings.debug);
   if (info->fd < 0) {
      cm_msg(MERROR, "mscbhvr_init",
             "Cannot access MSCB submaster at \"%s\". Check power and connection.",
             info->settings.mscb_device);
      return FE_ERR_HW;
   }

   /* check if HVR devices are alive */
   for (i=info->settings.base_address ; i<info->settings.base_address+channels ; i++) {
      status = mscb_ping(info->fd, i);
      if (status != MSCB_SUCCESS) {
         cm_msg(MERROR, "mscbhvr_init",
               "Cannot ping MSCB HVR address \"%d\". Check power and connection.", i);
         return FE_ERR_HW;
      }

      mscb_info(info->fd, i, &node_info);
      if (strcmp(node_info.node_name, "HVR-500") != 0 &&
         strcmp(node_info.node_name, "HVR-400") != 0) {
         cm_msg(MERROR, "mscbhvr_init",
               "Found onexpected node \"%s\" at address \"%d\".", node_info.node_name, i);
         return FE_ERR_HW;
      }

      /* turn on HV main switch */
      flag = 3;
      mscb_write(info->fd, i, 0, &flag, 1);  
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_exit(MSCBHVR_INFO * info)
{
   mscb_exit(info->fd);

   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_set(MSCBHVR_INFO * info, INT channel, float value)
{
   mscb_write(info->fd, info->settings.base_address + channel, 1, &value, 4);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_get(MSCBHVR_INFO * info, INT channel, float *pvalue)
{
   INT size = 4;

   mscb_read(info->fd, info->settings.base_address + channel, 2, pvalue, &size);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_get_current(MSCBHVR_INFO * info, INT channel, float *pvalue)
{
   int size = 4;

   mscb_read(info->fd, info->settings.base_address + channel, 3, pvalue, &size);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_set_current_limit(MSCBHVR_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.base_address + channel, 9, &limit, 4);
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT mscbhvr(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   void *info, *bd;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      bd = va_arg(argptr, void *);
      status = mscbhvr_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbhvr_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbhvr_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbhvr_get(info, channel, pvalue);
      break;

   case CMD_GET_CURRENT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbhvr_get_current(info, channel, pvalue);
      break;

   case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbhvr_set_current_limit(info, channel, value);
      break;

   case CMD_GET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_SET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_GET_THRESHOLD:
      value = 0.1f;
      break;

   case CMD_GET_THRESHOLD_CURRENT:
      value = 1;
      break;

   case CMD_GET_VOLTAGE_LIMIT:
   case CMD_GET_CURRENT_LIMIT:
   case CMD_GET_RAMPUP:
   case CMD_GET_RAMPDOWN:
   case CMD_GET_TRIP_TIME:
      status = FE_SUCCESS;
      break;

   case CMD_SET_VOLTAGE_LIMIT:
   case CMD_SET_RAMPUP:
   case CMD_SET_RAMPDOWN:
   case CMD_SET_TRIP_TIME:
      status = FE_SUCCESS;
      break;

   default:
      cm_msg(MERROR, "mscbhvr device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

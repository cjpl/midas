/********************************************************************\

  Name:         dd_mscbfgd.c
  Created by:   Pierre-Andre Amaudruz
  
  Based on the mscbhvr device driver

  Contents:     MSCB FGD-008 SiPm High Voltage Device Driver
  Used for midas/mscb/embedded/Experiements/fgd/
  No Voltage readback
  Set Voltage
  Get Current
  Get temperatures (1,2,3)
  
  $Id: dd_mscbfgd.c $

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
} MSCBFGD_SETTINGS;

#define MSCBFGD_SETTINGS_STR "\
MSCB Device = STRING : [32] usb0\n\
MSCB Pwd = STRING : [32] \n\
Base Address = INT : 0\n\
"

typedef struct {
   MSCBFGD_SETTINGS settings;
   int num_channels;
   int fd;
} MSCBFGD_INFO;

/*---- device driver routines --------------------------------------*/

INT mscbfgd_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
//   unsigned int  ramp;
   int  i, status, size, flag;
   HNDLE hDB, hkeydd;
   MSCBFGD_INFO *info;
   MSCB_INFO node_info;

   /* allocate info structure */
   info = (MSCBFGD_INFO *) calloc(1, sizeof(MSCBFGD_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create MSCBFGD settings record */
   status = db_create_record(hDB, hkey, "DD", MSCBFGD_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   db_find_key(hDB, hkey, "DD", &hkeydd);
   size = sizeof(info->settings);
   db_get_record(hDB, hkeydd, &info->settings, &size, 0);

   info->num_channels = channels;

   /* open device on MSCB */
   info->fd = mscb_init(info->settings.mscb_device, NAME_LENGTH, info->settings.pwd, FALSE);
   if (info->fd < 0) {
      cm_msg(MERROR, "mscbfgd_init",
             "Cannot access MSCB submaster at \"%s\". Check power and connection.",
             info->settings.mscb_device);
      return FE_ERR_HW;
   }

   /* check if FGD devices are alive */
   for (i=info->settings.base_address ; i<info->settings.base_address+channels ; i++) {
      status = mscb_ping(info->fd, i, 0);
      if (status != MSCB_SUCCESS) {
         cm_msg(MERROR, "mscbfgd_init",
               "Cannot ping MSCB FGD address \"%d\". Check power and connection.", i);
         return FE_ERR_HW;
      }

      mscb_info(info->fd, i, &node_info);
      if (strcmp(node_info.node_name, "FGD-008") != 0) {
         cm_msg(MERROR, "mscbfgd_init",
               "Found one expected node \"%s\" at address \"%d\".", node_info.node_name, i);
         return FE_ERR_HW;
      }

      /* turn on HV main switch */
      flag = 0x31;
      // Set rampup/dwn (don't need ramping)
      mscb_write(info->fd, i, 0, &flag, 1);  
      //ramp = 200;
      //mscb_write(info->fd, i, 4, &ramp, 2);  
      //mscb_write(info->fd, i, 5, &ramp, 2);  
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbfgd_exit(MSCBFGD_INFO * info)
{
   mscb_exit(info->fd);

   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set Voltage
INT mscbfgd_set(MSCBFGD_INFO * info, INT channel, float value)
{
   mscb_write(info->fd, info->settings.base_address + channel, 1, &value, 4);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbfgd_get_current(MSCBFGD_INFO * info, INT channel, float *pvalue)
{
   int size = 4;

   mscb_read(info->fd, info->settings.base_address + channel, 2, pvalue, &size);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbfgd_get_temperature(MSCBFGD_INFO * info, INT channel, float *pvalue, int cmd)
{
  int size = 4;

  switch (cmd) {
   case CMD_GET_TEMPERATURE1:  // Temp1 
     mscb_read(info->fd, info->settings.base_address + channel, 7, pvalue, &size);
     break;
   case CMD_GET_TEMPERATURE2:  // Temp2 
     mscb_read(info->fd, info->settings.base_address + channel, 9, pvalue, &size);
     break;
   case CMD_GET_TEMPERATURE3:  // Temp3 
     mscb_read(info->fd, info->settings.base_address + channel, 10, pvalue, &size);
     break;
  }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbfgd_set_current_limit(MSCBFGD_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.base_address + channel, 7, &limit, 4);
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT mscbfgd(INT cmd, ...)
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
      status = mscbfgd_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbfgd_exit(info);
      break;

   case CMD_SET:  // Voltage
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      printf("Set voltage:[%i]%f\n", channel, value);
      status = mscbfgd_set(info, channel, value);
      ss_sleep(1000);
      break;

   case CMD_GET_CURRENT:  // Current 
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbfgd_get_current(info, channel, pvalue);
      break;

   case CMD_GET_TEMPERATURE1:  // Temp1 
   case CMD_GET_TEMPERATURE2:  // Temp2 
   case CMD_GET_TEMPERATURE3:  // Temp3 
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbfgd_get_temperature(info, channel, pvalue, cmd);
      break;

   case CMD_SET_CURRENT_LIMIT:  
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbfgd_set_current_limit(info, channel, value);
      break;

   case CMD_GET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_GET_THRESHOLD:
      status = FE_SUCCESS;
      break;

   case CMD_GET_DEMAND:
      status = FE_SUCCESS;
      break;

   case CMD_SET_LABEL:
      status = FE_SUCCESS;
      break;

   default:
      cm_msg(MERROR, "mscbfgd device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

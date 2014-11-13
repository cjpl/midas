/********************************************************************\

  Name:         dd_mscbivc32.c
  Created by:   Pierre-Andre Amaudruz

  Based on the mscbhvr device driver

  Contents:     MSCB IVC32 for the LiXe Intermediate HV control
                Refer for documentation to:
                http://daq-plone.triumf.ca/HR/PERIPHERALS/IVC32.pdf

  $Id:$
\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include "midas.h"
#include "mscb.h"

unsigned short add_convert(unsigned short base, int ch);
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

/*----------------------------------------------------------------------------*/
unsigned short add_convert(unsigned short base, int ch) {
  int mch, mbase;
  mch = ch % 32;
  mbase = 256 * (ch / 32);
  return (base + mbase + mch);
}

/*---- device driver routines --------------------------------------*/
INT mscbivc32_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
//   unsigned int  ramp;
   int  i, status, size, addr;
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
      cm_msg(MERROR, "mscbivc32_init",
             "Cannot access MSCB submaster at \"%s\". Check power and connection.",
             info->settings.mscb_device);
      return FE_ERR_HW;
   }

   /* check if FGD devices are alive */
   for (i=0 ; i < channels ; i++) {
     addr = add_convert(info->settings.base_address, i);
      mscb_ping(info->fd, addr, 0);
      if (status != MSCB_SUCCESS) {
         cm_msg(MERROR, "mscbivc32_init",
               "Cannot ping MSCB FGD address \"%d\". Check power and connection.", addr);
         return FE_ERR_HW;
      }

      mscb_info(info->fd, addr, &node_info);
      if (strcmp(node_info.node_name, "LXe_IVC32") != 0) {
         cm_msg(MERROR, "mscbivc32_init",
               "Found one expected node \"%s\" at address \"%d\".", node_info.node_name, addr);
         return FE_ERR_HW;
      }
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbivc32_exit(MSCBFGD_INFO * info)
{
   mscb_exit(info->fd);

   free(info);

   return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
// Set Voltage
INT mscbivc32_set(MSCBFGD_INFO * info, INT channel, float value)
{
    int csr=0, size=1;
  if (value > 5.) { // HV ON
    mscb_read(info->fd,add_convert(info->settings.base_address, channel), 2, &csr, &size);
    if (csr & 0x1) {  // channel enabled, write HV
      mscb_write(info->fd, add_convert(info->settings.base_address, channel), 2, &value, 4);
    } else {  // channel OFF, turn on, write HV
      csr |= 0x1;
      mscb_write(info->fd, add_convert(info->settings.base_address, channel), 0, &csr, 1);
      mscb_write(info->fd, add_convert(info->settings.base_address, channel), 2, &value, 4);
    }
  }
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbivc32_get(MSCBFGD_INFO * info, INT channel, float *pvalue)
{
   int size = 4;

   mscb_read(info->fd, add_convert(info->settings.base_address, channel), 3, pvalue, &size);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbivc32_get_current(MSCBFGD_INFO * info, INT channel, float *pvalue)
{
   int size = 4;

   mscb_read(info->fd, add_convert(info->settings.base_address, channel), 4, pvalue, &size);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbivc32_get_other(MSCBFGD_INFO * info, INT channel, float *pvalue, int cmd)
{
  int size = 4;

  switch (cmd) {
   case CMD_GET_TEMPERATURE:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 10, pvalue, &size);
     break;
   case CMD_GET_DEMAND:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 2, pvalue, &size);
     break;
   case CMD_GET_CURRENT_LIMIT:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 5, pvalue, &size);
     break;
  // unsigned int (16bits)

   case CMD_GET_TRIP_TIME:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 6, pvalue, &size);
     break;
   case CMD_GET_RAMPDOWN:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 8, pvalue, &size);
     break;
   case CMD_GET_RAMPUP:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 7, pvalue, &size);
     break;
  }
    return FE_SUCCESS;
 }

/*----------------------------------------------------------------------------*/
INT mscbivc32_get_other16(MSCBFGD_INFO * info, INT channel, float *pvalue, int cmd)
{
  int size = 2;
  short int val;

  switch (cmd) {
  // unsigned int (16bits)
   case CMD_GET_TRIP_TIME:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 6, &val, &size);
     break;
   case CMD_GET_RAMPDOWN:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 8, &val, &size);
     break;
   case CMD_GET_RAMPUP:
     mscb_read(info->fd, add_convert(info->settings.base_address, channel), 7, &val, &size);
     break;
  }
  *pvalue = (float) val;

  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
INT mscbivc32_set_other(MSCBFGD_INFO * info, int channel, float limit, int cmd)
{
  WORD lim;
  switch (cmd) {
   case CMD_SET_RAMPUP:
    lim = (WORD) limit;
    mscb_write(info->fd, add_convert(info->settings.base_address, channel), 7, &lim, 2);
    break;
   case CMD_SET_RAMPDOWN:
    lim = (WORD) limit;
    mscb_write(info->fd, add_convert(info->settings.base_address, channel), 8, &lim, 2);
    break;
   case CMD_SET_TRIP_TIME:
    lim = (WORD) limit;
    mscb_write(info->fd, add_convert(info->settings.base_address, channel), 6, &lim, 2);
    break;
   case CMD_SET_CURRENT_LIMIT:
    mscb_write(info->fd, add_convert(info->settings.base_address, channel), 5, &limit, 4);
    break;
   case CMD_SET_VOLTAGE_LIMIT:
    // cm_msg(MINFO, "dd_mscbivc32", "Set Voltage Limit not implemented");
    break;
    }

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/
INT mscbivc32(INT cmd, ...)
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
      status = mscbivc32_init(hKey, (void **)info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbivc32_exit(info);
      break;

   case CMD_SET:  // Voltage
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbivc32_set(info, channel, value);
      break;

   case CMD_GET: // Voltage
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbivc32_get(info, channel, pvalue);
      break;

   case CMD_GET_CURRENT:  // Current
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbivc32_get_current(info, channel, pvalue);
      break;

   case CMD_GET_TEMPERATURE:    // Local temperature
   case CMD_GET_DEMAND:          // Desired Voltage
   case CMD_GET_CURRENT_LIMIT:  //
     info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbivc32_get_other(info, channel, pvalue, cmd);
      break;

   case CMD_GET_RAMPUP:
   case CMD_GET_RAMPDOWN:
   case CMD_GET_TRIP_TIME:
     info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbivc32_get_other16(info, channel, pvalue, cmd);
      break;

   case CMD_SET_RAMPUP:
   case CMD_SET_RAMPDOWN:
   case CMD_SET_TRIP_TIME:
   case CMD_SET_CURRENT_LIMIT:
   case CMD_SET_VOLTAGE_LIMIT:
     info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbivc32_set_other(info, channel, value, cmd);
      break;

   case CMD_GET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_SET_LABEL:
      status = FE_SUCCESS;
      break;

   default:
//      cm_msg(MERROR, "mscbfgd device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

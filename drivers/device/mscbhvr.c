/********************************************************************\

  Name:         mscbhvr.c
  Created by:   Stefan Ritt

  Contents:     MSCB HVR-400/500 High Voltage Device Driver

  $Id: mscbhvr.c 2753 2005-10-07 14:55:31Z ritt $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "midas.h"
#include "mscb.h"

/*---- globals -----------------------------------------------------*/

typedef struct {
   char mscb_device[NAME_LENGTH];
   char pwd[NAME_LENGTH];
   BOOL debug;
   int  *address;
   int  channels;
} MSCBHVR_SETTINGS;

typedef struct {
   unsigned char control;
   float u_demand;
   float u_meas;
   float i_meas;
   unsigned char status;
   unsigned char trip_cnt;

   float ramp_up;
   float ramp_down;
   float u_limit;
   float i_limit;
   float ri_limit;
   unsigned char trip_max;
   unsigned char trip_time;

   unsigned char cached;
} MSCB_NODE_VARS;

typedef struct {
   MSCBHVR_SETTINGS settings;
   int fd;
   MSCB_NODE_VARS *node_vars;
} MSCBHVR_INFO;

INT mscbhvr_read_all(MSCBHVR_INFO * info, int i);

/*---- device driver routines --------------------------------------*/

INT mscbhvr_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
   int  i, j, status, size, flag, block_address, block_channels, index;
   HNDLE hDB, hsubkey;
   MSCBHVR_INFO *info;
   MSCB_INFO node_info;
   KEY key, adrkey;

   /* allocate info structure */
   info = (MSCBHVR_INFO *) calloc(1, sizeof(MSCBHVR_INFO));
   info->node_vars = (MSCB_NODE_VARS *) calloc(channels, sizeof(MSCB_NODE_VARS));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* retrieve device name */
   db_get_key(hDB, hkey, &key);

   /* create MSCBHVR settings record */
   size = sizeof(info->settings.mscb_device);
   strcpy(info->settings.mscb_device, "usb0");
   status = db_get_value(hDB, hkey, "MSCB Device", &info->settings.mscb_device, &size, TID_STRING, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->settings.pwd);
   info->settings.pwd[0] = 0;
   status = db_get_value(hDB, hkey, "MSCB Pwd", &info->settings.pwd, &size, TID_STRING, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   /* calculate address from block address */
   info->settings.address = calloc(sizeof(INT), channels);
   assert(info->settings.address);

   block_channels = channels;
   status = db_find_key(hDB, hkey, "Block address", &hsubkey);
   if (status == DB_SUCCESS) {
      db_get_key(hDB, hsubkey, &adrkey);
      index = 0;
      for (i=0 ; i<adrkey.num_values ; i++) {
         size = sizeof(block_address);
         status = db_find_key(hDB, hkey, "Block address", &hsubkey);
         assert(status == DB_SUCCESS);
         status = db_get_data_index(hDB, hsubkey, &block_address, &size, i, TID_INT);
         assert(status == DB_SUCCESS);
         size = sizeof(block_channels);
         db_find_key(hDB, hkey, "Block channels", &hsubkey);
         assert(status == DB_SUCCESS);
         status = db_get_data_index(hDB, hsubkey, &block_channels, &size, i, TID_INT);
         assert(status == DB_SUCCESS);

         for (j = 0 ; j<block_channels && index < channels ; j++)
            info->settings.address[index++] = block_address + j;
      }
   } else {
      block_address = 0;
      size = sizeof(INT);
      status = db_set_value(hDB, hkey, "Block address", &block_address, size, 1, TID_INT);
      if (status != DB_SUCCESS)
         return FE_ERR_ODB;
      block_channels = channels;
      size = sizeof(INT);
      status = db_set_value(hDB, hkey, "Block channels", &block_channels, size, 1, TID_INT);
      if (status != DB_SUCCESS)
         return FE_ERR_ODB;

      for (j = 0 ; j<block_channels ; j++)
         info->settings.address[j] = j;
   }


   /* open device on MSCB */
   info->fd = mscb_init(info->settings.mscb_device, NAME_LENGTH, info->settings.pwd, info->settings.debug);
   if (info->fd < 0) {
      cm_msg(MERROR, "mscbhvr_init",
             "Cannot access MSCB submaster at \"%s\". Check power and connection.",
             info->settings.mscb_device);
      return FE_ERR_HW;
   }

   /* check first node */
   status = mscb_info(info->fd, info->settings.address[0], &node_info);
   if (status != MSCB_SUCCESS) {
      cm_msg(MERROR, "mscbhvr_init",
            "Cannot access HVR node at address \"%d\". Please check cabling and power.", 
            info->settings.address[0]);
      return FE_ERR_HW;
   }

   if (strcmp(node_info.node_name, "HVR-800") != 0 &&
      strcmp(node_info.node_name, "HVR-500") != 0 &&
      strcmp(node_info.node_name, "HVR-400") != 0 &&
      strcmp(node_info.node_name, "HVR-200") != 0) {
      cm_msg(MERROR, "mscbhvr_init",
            "Found unexpected node \"%s\" at address \"%d\".", 
            node_info.node_name, info->settings.address[0]);
      return FE_ERR_HW;
   }

   if (node_info.revision < 3518 || node_info.revision < 10) {
      cm_msg(MERROR, "mscbhvr_init",
            "Found node \"%d\" with old firmware %d (SVN revistion >= 3518 required)", 
            info->settings.address[0], node_info.revision);
      return FE_ERR_HW;
   }

   /* read all values from HVR devices */
   for (i=0 ; i<channels ; i++) {

      if (i % 10 == 0)
         printf("%s: %d\r", key.name, i);

      status = mscbhvr_read_all(info, i);
      if (status != FE_SUCCESS)
         return status;
   }
   printf("%s: %d\n", key.name, i);

   /* turn on HV main switch */
   flag = 3;
   mscb_write_group(info->fd, 400, 0, &flag, 1);
   mscb_write_group(info->fd, 500, 0, &flag, 1);
   mscb_write_group(info->fd, 800, 0, &flag, 1);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_read_all(MSCBHVR_INFO * info, int i)
{
   int size, status;
   unsigned char buffer[256], *pbuf;
   char str[256];

   size = sizeof(buffer);
   status = mscb_read_range(info->fd, info->settings.address[i], 0, 12, buffer, &size);
   if (status != MSCB_SUCCESS) {
      sprintf(str, "Error reading MSCB HVR at \"%s:%d\".", 
              info->settings.mscb_device, info->settings.address[i]);
      mfe_error(str);
      return FE_ERR_HW;
   }

   /* decode variables from buffer */
   pbuf = buffer;
   info->node_vars[i].control = *((unsigned char *)pbuf);
   pbuf += sizeof(char);
   DWORD_SWAP(pbuf);
   info->node_vars[i].u_demand = *((float *)pbuf);
   pbuf += sizeof(float);
   DWORD_SWAP(pbuf);
   info->node_vars[i].u_meas = *((float *)pbuf);
   pbuf += sizeof(float);
   DWORD_SWAP(pbuf);
   info->node_vars[i].i_meas = *((float *)pbuf);
   pbuf += sizeof(float);
   info->node_vars[i].status = *((unsigned char *)pbuf);
   pbuf += sizeof(char);
   info->node_vars[i].trip_cnt = *((unsigned char *)pbuf);
   pbuf += sizeof(char);
   DWORD_SWAP(pbuf);
   info->node_vars[i].ramp_up = *((float *)pbuf);
   pbuf += sizeof(float);
   DWORD_SWAP(pbuf);
   info->node_vars[i].ramp_down = *((float *)pbuf);
   pbuf += sizeof(float);
   DWORD_SWAP(pbuf);
   info->node_vars[i].u_limit = *((float *)pbuf);
   pbuf += sizeof(float);
   DWORD_SWAP(pbuf);
   info->node_vars[i].i_limit = *((float *)pbuf);
   pbuf += sizeof(float);
   DWORD_SWAP(pbuf);
   info->node_vars[i].ri_limit = *((float *)pbuf);
   pbuf += sizeof(float);
   info->node_vars[i].trip_max = *((unsigned char *)pbuf);
   pbuf += sizeof(char);
   info->node_vars[i].trip_time = *((unsigned char *)pbuf);

   /* mark voltage/current as valid in cache */
   info->node_vars[i].cached = 1;

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
   mscb_write(info->fd, info->settings.address[channel], 1, &value, 4);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_get(MSCBHVR_INFO * info, INT channel, float *pvalue)
{
   int size, status;
   unsigned char buffer[256], *pbuf;
   char str[256];

   /* check if value was previously read by mscbhvr_read_all() */
   if (info->node_vars[channel].cached) {
      *pvalue = info->node_vars[channel].u_meas;
      info->node_vars[channel].cached = 0;
      return FE_SUCCESS;
   }

   // printf("get: %d %03d %04d  \r", info->fd, channel, info->settings.address[channel]);

   size = sizeof(buffer);
   status = mscb_read_range(info->fd, info->settings.address[channel], 2, 3, buffer, &size);
   if (status != MSCB_SUCCESS) {
      sprintf(str, "Error reading MSCB HVR at \"%s:%d\".", 
              info->settings.mscb_device, info->settings.address[channel]);
      mfe_error(str);
      return FE_ERR_HW;
   }

   /* decode variables from buffer */
   pbuf = buffer;
   DWORD_SWAP(pbuf);
   info->node_vars[channel].u_meas = *((float *)pbuf);
   pbuf += sizeof(float);
   DWORD_SWAP(pbuf);
   info->node_vars[channel].i_meas = *((float *)pbuf);

   *pvalue = info->node_vars[channel].u_meas;
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_set_current_limit(MSCBHVR_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.address[channel], 9, &limit, 4);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_set_voltage_limit(MSCBHVR_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.address[channel], 8, &limit, 4);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_set_rampup(MSCBHVR_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.address[channel], 6, &limit, 4);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_set_rampdown(MSCBHVR_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.address[channel], 7, &limit, 4);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbhvr_set_triptime(MSCBHVR_INFO * info, int channel, float limit)
{
   unsigned char data;
   data = (unsigned char) limit;
   mscb_write(info->fd, info->settings.address[channel], 12, &data, 1);
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT mscbhvr(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   int *pivalue;
   void *bd;
   MSCBHVR_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      bd = va_arg(argptr, void *);
      status = mscbhvr_init(hKey, (void **)info, channel, bd);
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

   case CMD_GET_DEMAND:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = info->node_vars[channel].u_demand;
      break;

   case CMD_GET_CURRENT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = info->node_vars[channel].i_meas;
      break;

   case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbhvr_set_current_limit(info, channel, value);
      break;

   case CMD_SET_VOLTAGE_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbhvr_set_voltage_limit(info, channel, value);
      break;

   case CMD_GET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_SET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_GET_THRESHOLD:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 0.1f;
      break;

   case CMD_GET_THRESHOLD_CURRENT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 1;
      break;

   case CMD_GET_THRESHOLD_ZERO:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 20;
      break;

   case CMD_GET_VOLTAGE_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = info->node_vars[channel].u_limit;
      break;

   case CMD_GET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = info->node_vars[channel].i_limit;
      break;

   case CMD_GET_RAMPUP:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = info->node_vars[channel].ramp_up;
      break;

   case CMD_GET_RAMPDOWN:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = info->node_vars[channel].ramp_down;
      break;

   case CMD_GET_TRIP_TIME:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = info->node_vars[channel].trip_time;
      break;

   case CMD_SET_RAMPUP:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbhvr_set_rampup(info, channel, value);
      break;

   case CMD_SET_RAMPDOWN:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbhvr_set_rampdown(info, channel, value);
      break;

   case CMD_SET_TRIP_TIME:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbhvr_set_triptime(info, channel, value);
      break;

   case CMD_GET_TRIP:
   case CMD_GET_STATUS:
   case CMD_GET_TEMPERATURE:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pivalue = va_arg(argptr, INT *);
      *pivalue = 0; // not implemented for the moment...
      status = FE_SUCCESS;
      break;

   case CMD_START:
   case CMD_STOP:
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

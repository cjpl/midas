/********************************************************************\

  Name:         mscbdev.c
  Created by:   Stefan Ritt

  Contents:     MSCB Device Driver.

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"
#include "mscb.h"

/*---- globals -----------------------------------------------------*/

/* MSCB node address / channel mapping */

typedef struct {
   char mscb_device[256];
   char pwd[32];
   BOOL debug;
   int retries;
   int *mscb_address;
   unsigned char *mscb_index;
   int *var_size;
   float *var_cache;
} MSCBDEV_SETTINGS;

typedef struct {
   MSCBDEV_SETTINGS mscbdev_settings;
   INT fd;
   INT num_channels;
} MSCBDEV_INFO;

/*---- device driver routines --------------------------------------*/

INT addr_changed(HNDLE hDB, HNDLE hKey, void *arg)
{
   INT i, status;
   MSCB_INFO_VAR var_info;
   MSCBDEV_INFO *info;

   info = (MSCBDEV_INFO *) arg;

   /* get info about MSCB channels */
   for (i = 0; i < info->num_channels; i++) {
      status = mscb_info_variable(info->fd, info->mscbdev_settings.mscb_address[i],
                                  info->mscbdev_settings.mscb_index[i], &var_info);
      if (status == MSCB_SUCCESS) {
         if (var_info.flags & MSCBF_FLOAT)
            info->mscbdev_settings.var_size[i] = -1;
         else
            info->mscbdev_settings.var_size[i] = var_info.width;
      } else {
         info->mscbdev_settings.var_size[i] = 0;
         cm_msg(MERROR, "addr_changed", "Cannot read from address %d at submaster %s", 
            info->mscbdev_settings.mscb_address[i], info->mscbdev_settings.mscb_device);
         return FE_ERR_HW;
      }
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

/* the init function creates a ODB record which contains the
   settings and initialized it variables as well as the bus driver */

INT mscbdev_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
   int i, status, size;
   HNDLE hDB, hsubkey;
   MSCBDEV_INFO *info;

   /* allocate info structure */
   info = calloc(1, sizeof(MSCBDEV_INFO));
   *pinfo = info;
   info->mscbdev_settings.mscb_address = calloc(channels, sizeof(INT));
   info->mscbdev_settings.mscb_index = calloc(channels, sizeof(INT));
   info->mscbdev_settings.var_size = calloc(channels, sizeof(INT));
   info->mscbdev_settings.var_cache = calloc(channels, sizeof(float));
   for (i = 0; i < channels; i++)
      info->mscbdev_settings.var_cache[i] = (float) ss_nan();

   cm_get_experiment_database(&hDB, NULL);

   /* create settings record */
   size = sizeof(info->mscbdev_settings.mscb_device);
   strcpy(info->mscbdev_settings.mscb_device, "usb0");
   status = db_get_value(hDB, hkey, "Device", &info->mscbdev_settings.mscb_device, &size, TID_STRING, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->mscbdev_settings.pwd);
   info->mscbdev_settings.pwd[0] = 0;
   status = db_get_value(hDB, hkey, "Pwd", &info->mscbdev_settings.pwd, &size, TID_STRING, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->mscbdev_settings.debug);
   info->mscbdev_settings.debug = 0;
   status = db_get_value(hDB, hkey, "Debug", &info->mscbdev_settings.debug, &size, TID_BOOL, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->mscbdev_settings.retries);
   info->mscbdev_settings.retries = 10;
   status = db_get_value(hDB, hkey, "Retries", &info->mscbdev_settings.retries, &size, TID_INT, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(INT) * channels;
   db_get_value(hDB, hkey, "MSCB Address", info->mscbdev_settings.mscb_address, &size, TID_INT, TRUE);
   db_find_key(hDB, hkey, "MSCB Address", &hsubkey);
   size = sizeof(INT) * channels;
   db_set_data(hDB, hsubkey, info->mscbdev_settings.mscb_address, size, channels, TID_INT);
   db_open_record(hDB, hsubkey, info->mscbdev_settings.mscb_address, size, MODE_READ, (void (*)(INT,INT,void*))addr_changed, info);

   size = sizeof(BYTE) * channels;
   db_get_value(hDB, hkey, "MSCB Index", info->mscbdev_settings.mscb_index, &size, TID_BYTE, TRUE);
   db_find_key(hDB, hkey, "MSCB Index", &hsubkey);
   size = sizeof(BYTE) * channels;
   db_set_data(hDB, hsubkey, info->mscbdev_settings.mscb_index, size, channels, TID_BYTE);
   db_open_record(hDB, hsubkey, info->mscbdev_settings.mscb_index, size, MODE_READ, (void (*)(INT,INT,void*))addr_changed, info);

   /* initialize info structure */
   info->num_channels = channels;

   info->fd = mscb_init(info->mscbdev_settings.mscb_device, sizeof(info->mscbdev_settings.mscb_device),
                        info->mscbdev_settings.pwd, info->mscbdev_settings.debug);
   if (info->fd < 0) {
      cm_msg(MERROR, "mscbdev_init", "Cannot connect to MSCB device \"%s\"", info->mscbdev_settings.mscb_device);
      return FE_ERR_HW;
   }

   /* set number of retries */
   mscb_set_eth_max_retry(info->fd, info->mscbdev_settings.retries);

   /* write back device */
   status = db_set_value(hDB, hkey, "Device", &info->mscbdev_settings.mscb_device,
                         sizeof(info->mscbdev_settings.mscb_device), 1, TID_STRING);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   /* read initial variable sizes */
   return addr_changed(0, 0, info);
}

/*----------------------------------------------------------------------------*/

INT mscbdev_exit(MSCBDEV_INFO * info)
{
   /* close MSCB bus */
   if (info) {
      mscb_exit(info->fd);
      free(info->mscbdev_settings.mscb_address);
      free(info->mscbdev_settings.mscb_index);
      free(info->mscbdev_settings.var_size);
      free(info);
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_set(MSCBDEV_INFO * info, INT channel, float value)
{
   INT value_int;

   if (info->mscbdev_settings.var_size[channel] == -1) {
      /* channel is "float" */
      mscb_write(info->fd, info->mscbdev_settings.mscb_address[channel],
                 info->mscbdev_settings.mscb_index[channel], &value, sizeof(float));
   } else {
      /* channel is int */
      value_int = (INT) value;
      mscb_write(info->fd, info->mscbdev_settings.mscb_address[channel],
                 info->mscbdev_settings.mscb_index[channel], &value_int,
                 info->mscbdev_settings.var_size[channel]);
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_read_all(MSCBDEV_INFO * info)
{
   int i, j, status, i_start, i_stop, v_start, v_stop, addr, size;
   unsigned char buffer[256], *pbuf;
   static DWORD last_error = 0;
   char str[256];

   /* find consecutive ranges in variables */
   v_start = v_stop = 0;
   i_start = i_stop = info->mscbdev_settings.mscb_index[0];
   addr = info->mscbdev_settings.mscb_address[0];

   for (i = 1; i <= info->num_channels; i++) {
      if (i == info->num_channels ||                            // read last chunk
          i_stop - i_start >= 60 ||                             // not more than 60 vars (->240 bytes)
          info->mscbdev_settings.mscb_address[i] != addr ||     // if at different address
          info->mscbdev_settings.mscb_index[i] != i_stop + 1) { // if non-consecutive index

         /* complete range found, so read it */
         size = sizeof(buffer);
         if (i_start == i_stop) {
            /* read single value (bug in old mscbmain.c) */
            status = mscb_read(info->fd, addr, i_start, buffer, &size);
            if (info->mscbdev_settings.var_size[v_start] == -1)
               DWORD_SWAP(buffer);
         } else
            status = mscb_read_range(info->fd, addr, i_start, i_stop, buffer, &size);

         if (status != MSCB_SUCCESS) {
            /* only produce error once every minute */
            if (ss_time() - last_error >= 60) {
               last_error = ss_time();
               sprintf(str, "Read error submaster %s address %d", info->mscbdev_settings.mscb_device, addr);
               mfe_error(str);
            }
            for (j = v_start; j <= v_stop; j++)
               info->mscbdev_settings.var_cache[j] = (float) ss_nan();
            return FE_ERR_HW;
         }

         /* interprete buffer */
         pbuf = buffer;
         for (j = v_start; j <= v_stop; j++) {
            if (info->mscbdev_settings.var_size[j] == -1) {
               DWORD_SWAP(pbuf);
               info->mscbdev_settings.var_cache[j] = *((float *) pbuf);
               pbuf += sizeof(float);
            } else if (info->mscbdev_settings.var_size[j] == 4) {
               DWORD_SWAP(pbuf);
               info->mscbdev_settings.var_cache[j] = (float) *((unsigned int *) pbuf);
               pbuf += sizeof(unsigned int);
            } else if (info->mscbdev_settings.var_size[j] == 2) {
               WORD_SWAP(pbuf);
               info->mscbdev_settings.var_cache[j] = (float) *((unsigned short *) pbuf);
               pbuf += sizeof(unsigned short);
            } else {
               info->mscbdev_settings.var_cache[j] = (float) *((unsigned char *) pbuf);
               pbuf += sizeof(unsigned char);
            }
         }

         if (i < info->num_channels) {
            v_start = v_stop = i;
            i_start = i_stop = info->mscbdev_settings.mscb_index[i];
            addr = info->mscbdev_settings.mscb_address[i];
         }
      } else {
         i_stop++;
         v_stop++;
      }

   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_get(MSCBDEV_INFO * info, INT channel, float *pvalue)
{
   INT status;

   /* check if value was previously read by mscbhvr_read_all() */
   if (!ss_isnan(info->mscbdev_settings.var_cache[channel])) {
      *pvalue = info->mscbdev_settings.var_cache[channel];
      info->mscbdev_settings.var_cache[channel] = (float) ss_nan();
      return FE_SUCCESS;
   }

   status = mscbdev_read_all(info);
   *pvalue = info->mscbdev_settings.var_cache[channel];
   info->mscbdev_settings.var_cache[channel] = (float) ss_nan();

   return status;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_get_label(MSCBDEV_INFO * info, INT channel, char *name)
{
   int status;
   MSCB_INFO_VAR var_info;

   status = mscb_info_variable(info->fd, info->mscbdev_settings.mscb_address[channel],
                               info->mscbdev_settings.mscb_index[channel], &var_info);
   if (status == MSCB_SUCCESS) {
      strlcpy(name, var_info.name, NAME_LENGTH);
      name[8] = 0;
   }

   return status;
}

/*---- device driver entry point -----------------------------------*/

INT mscbdev(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   DWORD flags;
   float value, *pvalue;
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
      status = mscbdev_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbdev_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);   // floats are passed as double
      status = mscbdev_set(info, channel, value);
      break;

   case CMD_GET:
   case CMD_GET_DEMAND:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbdev_get(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = mscbdev_get_label(info, channel, name);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

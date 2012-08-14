/********************************************************************\

  Name:         iseg_hv_mpod.c
  Created by:   Stefan Ritt

  Contents:     High Voltage Device Driver for ISEG modules via
                Wiener MPOD crate

  $Id$

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "midas.h"
#include "mscb.h"

#include "WIENER_SNMP.h"

#undef calloc

/*---- globals -----------------------------------------------------*/

typedef struct {
   char mpod_device[NAME_LENGTH];
   char pwd[NAME_LENGTH];
   BOOL debug;
   int  channels;
   int  *chn_address;
} ISEG_HV_MPOD_SETTINGS;

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
} CHANNEL_VARS;

typedef struct {
   ISEG_HV_MPOD_SETTINGS settings;
   void *crate;
   CHANNEL_VARS *chn_vars;
} ISEG_HV_MPOD_INFO;

INT iseg_hv_mpod_get(ISEG_HV_MPOD_INFO * info, INT channel, float *pvalue);
INT iseg_hv_mpod_get_demand(ISEG_HV_MPOD_INFO * info, INT channel, float *pvalue);
INT iseg_hv_mpod_get_current(ISEG_HV_MPOD_INFO * info, INT channel, float *pvalue);

/*---- device driver routines --------------------------------------*/

INT iseg_hv_mpod_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
   int  i, status, size;
   HNDLE hDB, hsubkey;
   ISEG_HV_MPOD_INFO *info;
   KEY key;

   /* allocate info structure */
   info = (ISEG_HV_MPOD_INFO *) calloc(1, sizeof(ISEG_HV_MPOD_INFO));
   info->chn_vars = (CHANNEL_VARS *) calloc(channels, sizeof(CHANNEL_VARS));
   info->settings.chn_address = (int *) calloc(channels, sizeof(int));
   for (i=0 ; i<channels ; i++)
      info->settings.chn_address[i] = i+1;
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* retrieve device name */
   db_get_key(hDB, hkey, &key);

   /* create ISEG_HV_MPOD settings record */
   size = sizeof(info->settings.mpod_device);
   strcpy(info->settings.mpod_device, "127.0.0.1");
   status = db_get_value(hDB, hkey, "MPOD Device", &info->settings.mpod_device, &size, TID_STRING, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   size = sizeof(info->settings.pwd);
   info->settings.pwd[0] = 0;
   status = db_get_value(hDB, hkey, "MPOD Pwd", &info->settings.pwd, &size, TID_STRING, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;
   
   size = sizeof(INT) * channels;
   db_get_value(hDB, hkey, "Channel Address", info->settings.chn_address, &size, TID_INT, TRUE);
   db_find_key(hDB, hkey, "Channel Address", &hsubkey);
   size = sizeof(INT) * channels;
   db_set_data(hDB, hsubkey, info->settings.chn_address, size, channels, TID_INT);
   db_open_record(hDB, hsubkey, info->settings.chn_address, size, MODE_READ, NULL, info);

   // open device on SNMP
   if (!SnmpInit()) {
      cm_msg(MERROR, "iseg_hv_mpod_init", "Cannot initialize SNMP");
      return FE_ERR_HW;
   }
   info->crate = SnmpOpen(info->settings.mpod_device);

   // check main switch
   for (i=0 ; i<10 ; i++) {
      status = getMainSwitch(info->crate);
      if (status == -1)
         ss_sleep(10);
      else
         break;
   }
   if (status < 0) {
      cm_msg(MERROR, "iseg_hv_mpod_init",
             "Cannot access MPOD crate at \"%s\". Check power and connection.",
             info->settings.mpod_device);
      return FE_ERR_HW;
   }

   if (status == 0) {
      printf("Turn on MPOD main switch...\n");
      status = setMainSwitch(info->crate, 1);
      ss_sleep(5000);
   }
    
   // reset any trip and clear all events    --> don't do this, the ramping procedure will start the trip ramping for the tripped channels 
   //for (i=0; i<channels ; i++)
   //   status = setChannelSwitch(info->crate, i+1, 10);    
   // switch all channels on
   //for (i=0; i<channels ; i++)
   //   status = setChannelSwitch(info->crate, i+1, 1); 

   /* read all values from HV devices */
   for (i=0; i<channels; i++) {
      iseg_hv_mpod_get(info, i, &info->chn_vars[i].u_meas);
      iseg_hv_mpod_get_demand(info, i, &info->chn_vars[i].u_demand);
      iseg_hv_mpod_get_current(info, i, &info->chn_vars[i].i_meas);
      info->chn_vars[i].status = 0;
      info->chn_vars[i].trip_cnt = 0;
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_exit(ISEG_HV_MPOD_INFO * info)
{
   if (info) {
      SnmpClose(info->crate);
      SnmpCleanup();
      free(info->settings.chn_address);
      free(info);
   }
   
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_set(ISEG_HV_MPOD_INFO * info, INT channel, float value)
{
   setOutputVoltage(info->crate, info->settings.chn_address[channel], value);
   
   // if value == 0, reset any trip
   if (value == 0) {
      setChannelSwitch(info->crate, info->settings.chn_address[channel], 10);
      setChannelSwitch(info->crate, info->settings.chn_address[channel], 1);
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_get(ISEG_HV_MPOD_INFO * info, INT channel, float *pvalue)
{
   double value;
   int i, status;

   for (i=0 ; i<10 ; i++) {
      status = getChannelStatus(info->crate, info->settings.chn_address[channel]);
      if (status == -1)
         ss_sleep(10);
      else
         break;
   }

   /*
   if (status & 1<<10)
      printf("Channel %d tripped\n", channel);
   */

   for (i=0 ; i<10 ; i++) {
      value = getOutputSenseMeasurement(info->crate, info->settings.chn_address[channel]);
      if (value == -1)
         ss_sleep(10);
      else
         break;
   }

   if (value != -1) {
      /* round to 3 digites */
      *pvalue = (float)((int)(value*1e3+0.5)/1E3);
   } else
      *pvalue = (float)ss_nan();

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_get_current(ISEG_HV_MPOD_INFO * info, INT channel, float *pvalue)
{
   double value;
   int i; 

   for (i=0 ; i<10 ; i++) {
      value = getCurrentMeasurement(info->crate, info->settings.chn_address[channel]);
      if (value == -1)
         ss_sleep(10);
      else
         break;
   }

   if (value != -1) {
      value = value * 1E6; // convert to uA
      /* round to 3 digites */
      *pvalue = (float)((int)(value*1e3+0.5)/1E3);
   } else
      *pvalue = (float)ss_nan();

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_get_trip(ISEG_HV_MPOD_INFO * info, INT channel, INT *pvalue)
{
   int i, status;

   for (i=0 ; i<10 ; i++) {
      status = getChannelStatus(info->crate, info->settings.chn_address[channel]);
      if (status == -1)
         ss_sleep(10);
      else
         break;
   }

   if (status & 1<<10)
      *pvalue = 1;
   else
      *pvalue = 0;

   return FE_SUCCESS;
}
    
/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_get_demand(ISEG_HV_MPOD_INFO * info, INT channel, float *pvalue)
{
   *pvalue = (float)getOutputVoltage(info->crate, info->settings.chn_address[channel]);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_set_current_limit(ISEG_HV_MPOD_INFO * info, int channel, float limit)
{
   setOutputCurrent(info->crate, info->settings.chn_address[channel], limit/1E6); // converto to A
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_get_current_limit(ISEG_HV_MPOD_INFO * info, int channel, float *pvalue)
{
   *pvalue = (float)(getOutputCurrent(info->crate, info->settings.chn_address[channel])*1E6); // converto to uA
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_set_voltage_limit(ISEG_HV_MPOD_INFO * info, int channel, float limit)
{
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_set_rampup(ISEG_HV_MPOD_INFO * info, int channel, float limit)
{
   setOutputRiseRate(info->crate, info->settings.chn_address[channel], limit);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_get_rampup(ISEG_HV_MPOD_INFO * info, int channel, float *pvalue)
{
   *pvalue = (float)getOutputRiseRate(info->crate, info->settings.chn_address[channel]);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_set_rampdown(ISEG_HV_MPOD_INFO * info, int channel, float limit)
{
   setOutputFallRate(info->crate, info->settings.chn_address[channel], limit);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_get_rampdown(ISEG_HV_MPOD_INFO * info, int channel, float *pvalue)
{
   *pvalue = (float)getOutputFallRate(info->crate, info->settings.chn_address[channel]);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT iseg_hv_mpod_set_triptime(ISEG_HV_MPOD_INFO * info, int channel, float limit)
{
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT iseg_hv_mpod(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status, *pivalue;
   float value, *pvalue;
   void *bd;
   ISEG_HV_MPOD_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      bd = va_arg(argptr, void *);
      status = iseg_hv_mpod_init(hKey, (void **)info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      status = iseg_hv_mpod_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = iseg_hv_mpod_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = iseg_hv_mpod_get(info, channel, pvalue);
      break;

   case CMD_GET_DEMAND:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = iseg_hv_mpod_get_demand(info, channel, pvalue);
      break;

   case CMD_GET_CURRENT:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = iseg_hv_mpod_get_current(info, channel, pvalue);
      break;

   case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = iseg_hv_mpod_set_current_limit(info, channel, value);
      break;

   case CMD_SET_VOLTAGE_LIMIT:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = iseg_hv_mpod_set_voltage_limit(info, channel, value);
      break;

   case CMD_GET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_SET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_GET_THRESHOLD:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 0.1f;
      break;

   case CMD_GET_THRESHOLD_CURRENT:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 1;
      break;

   case CMD_GET_THRESHOLD_ZERO:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 20;
      break;

   case CMD_GET_CURRENT_LIMIT:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = iseg_hv_mpod_get_current_limit(info, channel, pvalue);
      break;

   case CMD_GET_VOLTAGE_LIMIT:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 3000;
      break;

   case CMD_GET_RAMPUP:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = iseg_hv_mpod_get_rampup(info, channel, pvalue);
      break;

   case CMD_GET_RAMPDOWN:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = iseg_hv_mpod_get_rampdown(info, channel, pvalue);
      break;

   case CMD_GET_TRIP:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pivalue = va_arg(argptr, INT *);
      status = iseg_hv_mpod_get_trip(info, channel, pivalue);
      break;

   case CMD_GET_TRIP_TIME:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      *pvalue = 0;
      break;

   case CMD_SET_RAMPUP:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = iseg_hv_mpod_set_rampup(info, channel, value);
      break;

   case CMD_SET_RAMPDOWN:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = iseg_hv_mpod_set_rampdown(info, channel, value);
      break;

   case CMD_SET_TRIP_TIME:
      info = va_arg(argptr, ISEG_HV_MPOD_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = iseg_hv_mpod_set_triptime(info, channel, value);
      break;

   case CMD_START:
   case CMD_STOP:
      break;

   default:
      cm_msg(MERROR, "iseg_hv_mpod device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

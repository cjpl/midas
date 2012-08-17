/********************************************************************\

  Name:         psi_epics.c
  Created by:   Stefan Ritt

  Contents:     Epics channel access device driver for PSI environment

  $Id: psi_epics.c 5219 2011-11-10 23:51:08Z svn $

\********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <epicsStdlib.h>
#include "cadef.h"
#include "dbDefs.h"
#include "epicsTime.h"

#ifdef _MSC_VER                 /* collision between varargs and stdarg */
#undef va_arg
#undef va_start
#undef va_end
#endif

#include "midas.h"
#include "msystem.h"
#include "psi_epics.h"

/*---- globals -----------------------------------------------------*/

#define CHN_NAME_LENGTH 32      /* length of channel names */

#define CAGET_TIMEOUT 4.0

typedef struct {
   char epics_gateway[256];
   int  gateway_port;
} CA_SETTINGS;

typedef struct {
   char  name[CHN_NAME_LENGTH];
   chid  chan_id;
   evid  evt_id;
   float value;
} CA_NODE;

typedef struct {
   CA_SETTINGS ca_settings;
   CA_NODE *demand;
   CA_NODE *measured;
   CA_NODE *extra;
   INT     num_channels;
   char    *channel_name;
   char    *demand_string;
   char    *measured_string;
   INT     *device_type;
   DWORD   flags;
} CA_INFO;

struct ca_client_context *ca_context;

/*---- device driver routines --------------------------------------*/

INT psi_epics_init(HNDLE hKey, void **pinfo, INT channels)
{
   int status, i, size;
   HNDLE hDB;
   CA_INFO *info;
   char str[256];
   
   /* allocate info structure */
   info = calloc(1, sizeof(CA_INFO));
   *pinfo = info;
   
   cm_get_experiment_database(&hDB, NULL);

   /* create settings record */
   size = sizeof(info->ca_settings.epics_gateway);
   strcpy(info->ca_settings.epics_gateway, "hipa-cagw01");
   status = db_get_value(hDB, hKey, "EPICS Gateway", &info->ca_settings.epics_gateway, &size, TID_STRING, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;
   size = sizeof(info->ca_settings.gateway_port);
   info->ca_settings.gateway_port = 5062;
   status = db_get_value(hDB, hKey, "Gateway port", &info->ca_settings.gateway_port, &size, TID_INT, TRUE);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;
   
   /* PSI specific settings */
   setenv("EPICS_CA_ADDR_LIST", info->ca_settings.epics_gateway, 1);
   setenv("EPICS_CA_AUTO_ADDR_LIST", "NO", 1);
   sprintf(str, "%d", info->ca_settings.gateway_port);
   setenv("EPICS_CA_SERVER_PORT", str, 1);
   
   /* allocate arrays */
   info->demand = calloc(channels, sizeof(CA_NODE));
   info->measured = calloc(channels, sizeof(CA_NODE));
   info->extra = calloc(channels, sizeof(CA_NODE));

   /* get channel names */
   info->channel_name = calloc(channels, CHN_NAME_LENGTH);
   for (i=0; i<channels; i++)
      sprintf(info->channel_name + CHN_NAME_LENGTH * i, "<Empty>%d", i);
   db_merge_data(hDB, hKey, "Channel name",
                 info->channel_name, CHN_NAME_LENGTH * channels, channels, TID_STRING);
   
   /* get demand strings */
   info->demand_string = calloc(channels, CHN_NAME_LENGTH);
   for (i=0; i<channels; i++)
      sprintf(info->demand_string + CHN_NAME_LENGTH * i, "<Empty>%d", i);
   db_merge_data(hDB, hKey, "Demand",
                 info->demand_string, CHN_NAME_LENGTH * channels, channels, TID_STRING);

   /* get measured strings */
   info->measured_string = calloc(channels, CHN_NAME_LENGTH);
   for (i=0; i<channels; i++)
      sprintf(info->measured_string + CHN_NAME_LENGTH * i, "<Empty>%d", i);
   db_merge_data(hDB, hKey, "Measured",
                 info->measured_string, CHN_NAME_LENGTH * channels, channels, TID_STRING);

   /* get device types */
   info->device_type = calloc(channels, sizeof(INT));
   for (i=0; i<channels; i++)
      info->device_type[i] = DT_DEVICE;
   db_merge_data(hDB, hKey, "Device type",
                 info->device_type, sizeof(INT) * channels, channels, TID_INT);

   /* create context for multi-thread handling */
   SEVCHK(ca_context_create(ca_enable_preemptive_callback), "ca_context_create");
   ca_context = ca_current_context();

   /* initialize driver */
   status = ca_task_initialize();
   if (!(status & CA_M_SUCCESS)) {
      cm_msg(MERROR, "psi_epics_init", "Unable to initialize");
      return FE_ERR_HW;
   }
   
   /* search channels */
   info->num_channels = channels;
   
   status = FE_SUCCESS;
   for (i = 0; i < channels; i++) {

      /* connect to demand channel */
      if (*(info->demand_string + i*CHN_NAME_LENGTH)) {
         strlcpy(info->demand[i].name, info->channel_name + i*CHN_NAME_LENGTH, CHN_NAME_LENGTH);
         strlcat(info->demand[i].name, info->demand_string + i*CHN_NAME_LENGTH, CHN_NAME_LENGTH);
         status = ca_create_channel(info->demand[i].name, 0, 0, 0, &(info->demand[i].chan_id));
         SEVCHK(status, "ca_create_channel");
         if (ca_pend_io(5.0) == ECA_TIMEOUT) {
            cm_msg(MERROR, "psi_epics_init", "Cannot connect to EPICS channel %s", info->demand[i].name);
            status = FE_ERR_HW;
            break;
         }
      } else {
         info->demand[i].chan_id = NULL;
      }
      
      /* connect to measured channel */
      if (*(info->measured_string + i*CHN_NAME_LENGTH)) {
         strlcpy(info->measured[i].name, info->channel_name + i*CHN_NAME_LENGTH, CHN_NAME_LENGTH);
         strlcat(info->measured[i].name, info->measured_string + i*CHN_NAME_LENGTH, CHN_NAME_LENGTH);
         status = ca_create_channel(info->measured[i].name, 0, 0, 0, &(info->measured[i].chan_id));
         SEVCHK(status, "ca_create_channel");
         if (ca_pend_io(5.0) == ECA_TIMEOUT) {
            cm_msg(MERROR, "psi_epics_init", "Cannot connect to EPICS channel %s", info->measured[i].name);
            status = FE_ERR_HW;
            break;
         }
      } else {
         info->measured[i].chan_id = NULL;
      }
      
      /* connect command channel for separator */
      if (info->device_type[i] == DT_SEPARATOR) {
         strlcpy(info->extra[i].name, info->channel_name + i*CHN_NAME_LENGTH, CHN_NAME_LENGTH);
         info->extra[i].name[strlen(info->extra[i].name)-1] = 0; // strip 'N' form SEP41VHVN
         strlcat(info->extra[i].name, ":COM:2", CHN_NAME_LENGTH);
         status = ca_create_channel(info->extra[i].name, 0, 0, 0, &(info->extra[i].chan_id));
         SEVCHK(status, "ca_create_channel");
         if (ca_pend_io(5.0) == ECA_TIMEOUT) {
            cm_msg(MERROR, "psi_epics_init", "Cannot connect to EPICS channel %s", info->extra[i].name);
            status = FE_ERR_HW;
            break;
         }
      }
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_epics_exit(CA_INFO * info)
{
   int i;
   
   if (info->demand) {
      for (i = 0; i < info->num_channels; i++)
         if (info->demand[i].chan_id)
            ca_clear_channel(info->demand[i].chan_id);
      free(info->demand);
   }
   if (info->measured) {
      for (i = 0; i < info->num_channels; i++)
         if (info->measured[i].chan_id)
           ca_clear_channel(info->measured[i].chan_id);
      free(info->measured);
   }
   if (info->extra) {
      for (i = 0; i < info->num_channels; i++)
         if (info->extra[i].chan_id)
            ca_clear_channel(info->extra[i].chan_id);
      free(info->extra);
   }
   
   ca_task_exit();
   
   if (info->channel_name)
      free(info->channel_name);
   
   if (info->demand_string)
      free(info->demand_string);

   if (info->measured_string)
      free(info->measured_string);

   if (info->device_type)
      free(info->device_type);

   free(info);
   
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_epics_set(CA_INFO * info, INT channel, float value)
{
   int status;
   short int d;
   
   if (info->demand[channel].chan_id) {
      if ((info->device_type[channel]) == DT_BEAMBLOCKER) 
         status = ca_put(DBR_STRING, info->demand[channel].chan_id, value == 1 ? "OPEN" : "CLOSE");
      
      else if ((info->device_type[channel]) == DT_SEPARATOR) { 
         status = ca_put(DBR_FLOAT, info->demand[channel].chan_id, &value);
         if (status != CA_M_SUCCESS)
            cm_msg(MERROR, "psi_epics_set", "Cannot write to EPICS channel %s", info->demand[channel].name);
         d = 3; // HVSET command
         status = ca_put(DBR_SHORT, info->extra[channel].chan_id, &d);
      
      } else
         status = ca_put(DBR_FLOAT, info->demand[channel].chan_id, &value);
      
      if (status != CA_M_SUCCESS)
         cm_msg(MERROR, "psi_epics_set", "Cannot write to EPICS channel %s", info->demand[channel].name);
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_epics_get(CA_INFO * info, INT channel, float *pvalue)
{
   int status, d;
   
   if ((info->device_type[channel] & 0xFF) == DT_BEAMBLOCKER ||
       (info->device_type[channel] & 0xFF) == DT_PSA) {

      status = ca_get(DBR_LONG, info->measured[channel].chan_id, &d);
      SEVCHK(status, "ca_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT)
         cm_msg(MERROR, "psi_epics_get", "Timeout on EPICS channel %s", info->measured[channel].name);
      
      if ((info->device_type[channel] & 0xFF) == DT_BEAMBLOCKER) {
         /* bit0: open, bit1: closed, bit2: psa ok */
         if ((d & 3) == 0)
            *pvalue = 0.5;
         else if (d & 1)
            *pvalue = 1;
         else if (d & 2)
            *pvalue = 0;
         else
            *pvalue = (float)ss_nan();
      } else {
         if (d & 4)
            *pvalue = 1;
         else
            *pvalue = 0;
      }
      
   } else {
      status = ca_get(DBR_FLOAT, info->measured[channel].chan_id, pvalue);
      SEVCHK(status, "ca_array_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT)
         cm_msg(MERROR, "psi_epics_get", "Timeout on EPICS channel %s", info->measured[channel].name);
   }
   
   //printf("%s %f\n", info->measured[channel].name, *pvalue);
   
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_epics_get_demand(CA_INFO * info, INT channel, float *pvalue)
{
   int status;
   char str[80];
   
   /* return NAN for readonly channels */
   if (info->demand[channel].chan_id == NULL) {
      *pvalue = ss_nan();
      return FE_SUCCESS;
   }
   
   if (info->device_type[channel] == DT_BEAMBLOCKER) {
      status = ca_get(DBR_STRING, info->demand[channel].chan_id, str);
      SEVCHK(status, "ca_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT)
         cm_msg(MERROR, "psi_epics_get_demand", "Timeout on EPICS channel %s", info->measured[channel].name);
      *pvalue = (str[0] == 'O') ? 1 : 0;
   } else {
      status = ca_get(DBR_FLOAT, info->demand[channel].chan_id, pvalue);
      SEVCHK(status, "ca_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT)
         cm_msg(MERROR, "psi_epics_get_demand", "Timeout on EPICS channel %s", info->measured[channel].name);
   }
   
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT psi_epics(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   DWORD flags;
   float value, *pvalue;
   char *name;
   CA_INFO *info;
   
   va_start(argptr, cmd);
   status = FE_SUCCESS;
   
   if (cmd == CMD_INIT) {
      void *pinfo;
      
      hKey = va_arg(argptr, HNDLE);
      pinfo = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      flags = va_arg(argptr, DWORD);
      status = psi_epics_init(hKey, pinfo, channel);
      info = *(CA_INFO **) pinfo;
      info->flags = flags;
   } else {
      info = va_arg(argptr, void *);
      
      /* only execute command if enabled */
      switch (cmd) {
         case CMD_INIT:
            break;
            
         case CMD_EXIT:
            status = psi_epics_exit(info);
            break;
            
         case CMD_START:
            SEVCHK(ca_attach_context(ca_context), "ca_attach_context");
            break;
            
         case CMD_SET:
            channel = va_arg(argptr, INT);
            value = (float) va_arg(argptr, double);
            status = psi_epics_set(info, channel, value);
            break;
            
         case CMD_GET:
            channel = va_arg(argptr, INT);
            pvalue = va_arg(argptr, float *);
            status = psi_epics_get(info, channel, pvalue);
            break;
            
         case CMD_GET_DEMAND:
            channel = va_arg(argptr, INT);
            pvalue = va_arg(argptr, float *);
            status = psi_epics_get_demand(info, channel, pvalue);
            break;

         case CMD_GET_LABEL:
            channel = va_arg(argptr, INT);
            name = va_arg(argptr, char *);
            strlcpy(name, info->channel_name + channel*CHN_NAME_LENGTH, CHN_NAME_LENGTH);
            break;

         case CMD_GET_THRESHOLD:
            channel = va_arg(argptr, INT);
            pvalue = va_arg(argptr, float *);
            *pvalue = 0.01;
            break;
            
         default:
            break;
      }
   }
   
   va_end(argptr);
   
   return status;
}

/*------------------------------------------------------------------*/

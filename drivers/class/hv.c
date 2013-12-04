/********************************************************************\

  Name:         hv.c
  Created by:   Stefan Ritt

  Contents:     High Voltage Class Driver

  $Id: hv.c 5126 2011-07-07 12:32:53Z ritt $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "midas.h"
#include "ybos.h"
#include <stdbool.h>

typedef struct {

   /* ODB keys */
   HNDLE hKeyRoot, hKeyDemand, hKeyMeasured, hKeyCurrent, hKeyChStatus, hKeyTemperature;

   /* globals */
   INT num_channels;
   INT format;
   DWORD last_update;
   INT last_channel;
   INT last_channel_updated;

   /* items in /Variables record */
   char *names;
   float *demand;
   float *measured;
   float *current;
   DWORD *chStatus;
   float *temperature;

   /* items in /Settings */
   float *update_threshold;
   float *update_threshold_current;
   float *zero_threshold;
   float *voltage_limit;
   float *current_limit;
   float *rampup_speed;
   float *rampdown_speed;
   float *trip_time;
   DWORD *chState;
   INT   *crateMap;

   /* mirror arrays */
   float *demand_mirror;
   float *measured_mirror;
   float *current_mirror;
   DWORD *chStatus_mirror;
   float *temperature_mirror;
   DWORD *last_change;

   DEVICE_DRIVER **driver;
   INT *channel_offset;
   void **dd_info;

} HV_INFO;

#ifndef abs
#define abs(a) (((a) < 0)   ? -(a) : (a))
#endif

/*------------------------------------------------------------------*/

static void free_mem(HV_INFO * hv_info)
{
   free(hv_info->names);
   free(hv_info->demand);
   free(hv_info->measured);
   free(hv_info->current);
   free(hv_info->chStatus);
   free(hv_info->temperature);

   free(hv_info->update_threshold);
   free(hv_info->update_threshold_current);
   free(hv_info->zero_threshold);
   free(hv_info->voltage_limit);
   free(hv_info->current_limit);
   free(hv_info->rampup_speed);
   free(hv_info->rampdown_speed);
   free(hv_info->trip_time);
   free(hv_info->chState);
   free(hv_info->crateMap);

   free(hv_info->demand_mirror);
   free(hv_info->measured_mirror);
   free(hv_info->current_mirror);
   free(hv_info->chStatus_mirror);
   free(hv_info->temperature_mirror);
   free(hv_info->last_change);

   free(hv_info->dd_info);
   free(hv_info->channel_offset);
   free(hv_info->driver);

   free(hv_info);
}

/*----------------------------------------------------------------------------*/

INT hv_start(EQUIPMENT * pequipment)
{
   INT i;

   /* call start method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL ; i++)
      if (pequipment->driver[i].flags & DF_MULTITHREAD) {
         pequipment->driver[i].pequipment = &pequipment->info;
         device_driver(&pequipment->driver[i], CMD_START);
      }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT hv_stop(EQUIPMENT * pequipment)
{
   INT i;

   /* call close method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL ; i++)
      if (pequipment->driver[i].flags & DF_MULTITHREAD)
         device_driver(&pequipment->driver[i], CMD_STOP);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hv_read(EQUIPMENT * pequipment, int channel)
{
   int i, status;
   float max_diff;
   DWORD act_time, min_time;
   BOOL changed;
   HV_INFO *hv_info;
   HNDLE hDB;

   hv_info = (HV_INFO *) pequipment->cd_info;
   cm_get_experiment_database(&hDB, NULL);

   /* if driver is multi-threaded, read all channels at once */
   for (i=0 ; i < hv_info->num_channels ; i++) {
      if (hv_info->driver[i]->flags & DF_MULTITHREAD) {
         status = device_driver(hv_info->driver[i], CMD_GET,
                                i - hv_info->channel_offset[i],
                                &hv_info->measured[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_CURRENT,
                                i - hv_info->channel_offset[i],
                                &hv_info->current[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_STATUS,
                                i - hv_info->channel_offset[i],
                                &hv_info->chStatus[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_TEMPERATURE,
                                i - hv_info->channel_offset[i],
                                &hv_info->temperature[i]);
      }
   }

   /* else read only single channel */
   if (!(hv_info->driver[channel]->flags & DF_MULTITHREAD)) {
      status = device_driver(hv_info->driver[channel], CMD_GET,
                                 channel - hv_info->channel_offset[channel],
                                 &hv_info->measured[channel]);
      status = device_driver(hv_info->driver[channel], CMD_GET_CURRENT,
                                 channel - hv_info->channel_offset[channel],
                                 &hv_info->current[channel]);
      status = device_driver(hv_info->driver[channel], CMD_GET_STATUS,
                                 channel - hv_info->channel_offset[channel],
                                 &hv_info->chStatus[channel]);
      //cm_msg(MINFO, "hv_read", "chStatus %d", hv_info->chStatus[channel]);
      status = device_driver(hv_info->driver[channel], CMD_GET_TEMPERATURE,
                                 channel - hv_info->channel_offset[channel],
                                 &hv_info->temperature[channel]);
   }

   /* check how much channels have changed since last ODB update */
   act_time = ss_millitime();

   /* check for update measured */
   max_diff = 0.f;
   min_time = 60000;
   changed = FALSE;
   for (i = 0; i < hv_info->num_channels; i++) {
      if (abs(hv_info->measured[i] - hv_info->measured_mirror[i]) > max_diff)
         max_diff = abs(hv_info->measured[i] - hv_info->measured_mirror[i]);

      /* indicate change if variation more than the threshold */
      if (abs(hv_info->measured[i] - hv_info->measured_mirror[i]) >
          hv_info->update_threshold[i] && hv_info->measured[i] > hv_info->zero_threshold[i])
         changed = TRUE;

      if (act_time - hv_info->last_change[i] < min_time)
         min_time = act_time - hv_info->last_change[i];
   }

   /* update if change is more than update_sensitivity or less than 20 seconds ago 
      or last update is older than a minute */
   if (changed || (min_time < 20000 && max_diff > 0) ||
       act_time - hv_info->last_update > 60000) {
      hv_info->last_update = act_time;

      for (i = 0; i < hv_info->num_channels; i++)
         hv_info->measured_mirror[i] = hv_info->measured[i];

      db_set_data(hDB, hv_info->hKeyMeasured, hv_info->measured,
                  sizeof(float) * hv_info->num_channels, hv_info->num_channels,
                  TID_FLOAT);

      pequipment->odb_out++;
   }

   /* check for update current */
   max_diff = 0.f;
   min_time = 10000;
   changed = FALSE;
   for (i = 0; i < hv_info->num_channels; i++) {
      if (abs(hv_info->current[i] - hv_info->current_mirror[i]) > max_diff)
         max_diff = abs(hv_info->current[i] - hv_info->current_mirror[i]);

      if (abs(hv_info->current[i] - hv_info->current_mirror[i]) >
          hv_info->update_threshold_current[i])
         changed = TRUE;

      if (act_time - hv_info->last_change[i] < min_time)
         min_time = act_time - hv_info->last_change[i];
   }

   /* update if change is more than update_sensitivity or less than 5sec ago */
   if (changed || (min_time < 5000 && max_diff > 0)) {
      for (i = 0; i < hv_info->num_channels; i++)
         hv_info->current_mirror[i] = hv_info->current[i];

      db_set_data(hDB, hv_info->hKeyCurrent, hv_info->current,
                  sizeof(float) * hv_info->num_channels, hv_info->num_channels,
                  TID_FLOAT);

      pequipment->odb_out++;
   }

   //check for updated chStatus:
   max_diff = 0.f;
   min_time = 60000;
   changed = FALSE;
   for (i = 0; i < hv_info->num_channels; i++) {
      if (abs(hv_info->chStatus[i] - hv_info->chStatus_mirror[i]) > max_diff)
         max_diff = abs(hv_info->chStatus[i] - hv_info->chStatus_mirror[i]);

      // indicate change if variation more than the threshold 
      //if (abs(hv_info->chStatus[i] - hv_info->chStatus_mirror[i]) > hv_info->update_threshold[i] && hv_info->chStatus[i] > hv_info->zero_threshold[i])
      if(hv_info->chStatus[i] != hv_info->chStatus_mirror[i])
         changed = TRUE;
                                 
      if (act_time - hv_info->last_change[i] < min_time)
         min_time = act_time - hv_info->last_change[i];
   }
      
   //update if change is more than update_sensitivity or less than 20 seconds ago or last update is older than a minute
   if (changed || (min_time < 20000 && max_diff > 0) ||
       act_time - hv_info->last_update > 60000) {
      hv_info->last_update = act_time;
   
      for (i = 0; i < hv_info->num_channels; i++)
         hv_info->chStatus_mirror[i] = hv_info->chStatus[i];
   
      db_set_data(hDB, hv_info->hKeyChStatus, hv_info->chStatus,
                  sizeof(DWORD) * hv_info->num_channels, hv_info->num_channels,
                  TID_DWORD);
      
      pequipment->odb_out++;
   }

   //check for temeprature update:
   max_diff = 0.f;
   min_time = 60000;
   changed = FALSE;
   for (i = 0; i < hv_info->num_channels; i++) {
      if (abs(hv_info->temperature[i] - hv_info->temperature_mirror[i]) > max_diff)
         max_diff = abs(hv_info->temperature[i] - hv_info->temperature_mirror[i]);
   
      // indicate change if variation more than the threshold
      if(hv_info->temperature[i] != hv_info->temperature_mirror[i])
         changed = TRUE;
   
      if (act_time - hv_info->last_change[i] < min_time)
         min_time = act_time - hv_info->last_change[i];
   }
                                 
   //update if change is more than update_sensitivity or less than 20 seconds ago or last update is older than a minute
   if (changed || (min_time < 20000 && max_diff > 0) ||
       act_time - hv_info->last_update > 60000) {
      hv_info->last_update = act_time;
      
      for (i = 0; i < hv_info->num_channels; i++)
         hv_info->temperature_mirror[i] = hv_info->temperature[i];
   
      db_set_data(hDB, hv_info->hKeyTemperature, hv_info->temperature,
                  sizeof(float) * hv_info->num_channels, hv_info->num_channels,
                  TID_FLOAT);
   
      pequipment->odb_out++;
   }

   return status;
}

/*------------------------------------------------------------------*/

INT hv_ramp(HV_INFO * hv_info)
{
   INT i, status = 0, switch_tag = FALSE;
   float delta, ramp_speed = 0;

   for (i = 0; i < hv_info->num_channels; i++) {

      if (hv_info->demand[i] != hv_info->demand_mirror[i]) {
         /* check if to ramp up or down */
         if ((hv_info->demand[i] >= 0.f) && (hv_info->demand_mirror[i] > 0.f)) {
            switch_tag = FALSE;
            if (hv_info->demand[i] > hv_info->demand_mirror[i])
               ramp_speed = hv_info->rampup_speed[i];
            else
               ramp_speed = hv_info->rampdown_speed[i];
         }
         if ((hv_info->demand[i] >= 0.f) && (hv_info->demand_mirror[i] < 0.f)) {
            switch_tag = TRUE;
            ramp_speed = hv_info->rampdown_speed[i];
         }
         if ((hv_info->demand[i] < 0.f) && (hv_info->demand_mirror[i] > 0.f)) {
            switch_tag = TRUE;
            ramp_speed = hv_info->rampdown_speed[i];
         }
         if ((hv_info->demand[i] < 0.f) && (hv_info->demand_mirror[i] < 0.f)) {
            switch_tag = FALSE;
            if (hv_info->demand[i] > hv_info->demand_mirror[i])
               ramp_speed = hv_info->rampdown_speed[i];
            else
               ramp_speed = hv_info->rampup_speed[i];
         }
         if (hv_info->demand_mirror[i] == 0.f) {
            switch_tag = FALSE;
            ramp_speed = hv_info->rampup_speed[i];
         }

         if (ramp_speed == 0.f)
            if (switch_tag)
               hv_info->demand_mirror[i] = 0.f; /* go to zero */
            else
               hv_info->demand_mirror[i] = hv_info->demand[i];  /* step directly to the new high voltage */
         else {
            delta = (float) ((ss_millitime() -
                              hv_info->last_change[i]) / 1000.0 * ramp_speed);
            if (hv_info->demand[i] > hv_info->demand_mirror[i])
               hv_info->demand_mirror[i] =
                   MIN(hv_info->demand[i], hv_info->demand_mirror[i] + delta);
            else
               hv_info->demand_mirror[i] =
                   MAX(hv_info->demand[i], hv_info->demand_mirror[i] - delta);
         }
         status = device_driver(hv_info->driver[i], CMD_SET,
                                i - hv_info->channel_offset[i], hv_info->demand_mirror[i]);
         hv_info->last_change[i] = ss_millitime();
      }
   }

   return status;
}


/*------------------------------------------------------------------*/

void hv_demand(INT hDB, INT hKey, void *info)
{
   INT i;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;

   /* check for voltage limit */
   for (i = 0; i < hv_info->num_channels; i++)
      if (fabs(hv_info->demand[i]) > fabs(hv_info->voltage_limit[i]))
         hv_info->demand[i] = hv_info->voltage_limit[i];

   /* set individual channels only if demand value differs */
   for (i = 0; i < hv_info->num_channels; i++)
      if (hv_info->demand[i] != hv_info->demand_mirror[i])
         hv_info->last_change[i] = ss_millitime();

   pequipment->odb_in++;

   for (i = 0; i < hv_info->num_channels; i++) {
      /* if device can do hardware ramping, just set value */   
      if (hv_info->driver[i]->flags & DF_HW_RAMP) {
         if (hv_info->demand[i] != hv_info->demand_mirror[i]) {
            device_driver(hv_info->driver[i], CMD_SET,
                          i - hv_info->channel_offset[i], hv_info->demand[i]);
            hv_info->last_change[i] = ss_millitime();
            hv_info->demand_mirror[i] = hv_info->demand[i];
         }
      }
   }

   /* execute one ramping for devices which have not hardware ramping */
   hv_ramp(hv_info);
}

/*------------------------------------------------------------------*/

void hv_set_trip_time(INT hDB, INT hKey, void *info)
{
   INT i;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;

   /* check for voltage limit */
   for (i = 0; i < hv_info->num_channels; i++)
      device_driver(hv_info->driver[i], CMD_SET_TRIP_TIME,
                    i - hv_info->channel_offset[i], hv_info->trip_time[i]);

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void hv_set_current_limit(INT hDB, INT hKey, void *info)
{
   INT i;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;

   /* check for voltage limit */
   for (i = 0; i < hv_info->num_channels; i++)
      device_driver(hv_info->driver[i], CMD_SET_CURRENT_LIMIT,
                    i - hv_info->channel_offset[i], hv_info->current_limit[i]);

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void hv_set_rampup(INT hDB, INT hKey, void *info)
{
   INT i;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;

   /* check for voltage limit */
   for (i = 0; i < hv_info->num_channels; i++)
      device_driver(hv_info->driver[i], CMD_SET_RAMPUP,
                    i - hv_info->channel_offset[i], hv_info->rampup_speed[i]);

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void hv_set_rampdown(INT hDB, INT hKey, void *info)
{
   INT i;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;

   /* check for voltage limit */
   for (i = 0; i < hv_info->num_channels; i++)
      device_driver(hv_info->driver[i], CMD_SET_RAMPDOWN,
                    i - hv_info->channel_offset[i], hv_info->rampdown_speed[i]);

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void hv_set_voltage_limit(INT hDB, INT hKey, void *info)
{
   INT i;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;

   /* check for voltage limit */
   for (i = 0; i < hv_info->num_channels; i++)
      device_driver(hv_info->driver[i], CMD_SET_VOLTAGE_LIMIT,
                    i - hv_info->channel_offset[i], hv_info->voltage_limit[i]);

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void hv_update_label(INT hDB, INT hKey, void *info)
{
   
   INT i, status;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;

   //update channel labels based on the midas channel names
   for (i = 0; i < hv_info->num_channels; i++)
      status = device_driver(hv_info->driver[i], CMD_SET_LABEL,
                             i - hv_info->channel_offset[i],
                             hv_info->names + NAME_LENGTH * i);
   
}

/*------------------------------------------------------------------*/

void hv_set_chState(INT hDB, INT hKey, void *info)
{
   INT i;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;
   
   pequipment = (EQUIPMENT *) info;
   hv_info = (HV_INFO *) pequipment->cd_info;
   
   for (i = 0; i < hv_info->num_channels; i++){

      device_driver(hv_info->driver[i], CMD_SET_CHSTATE, i - hv_info->channel_offset[i], hv_info->chState[i] );
   }
   
   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
   
void validate_odb_array(HNDLE hDB, HV_INFO *hv_info, char *path, double default_value, int cmd, 
                        float *array, void (*callback)(INT,INT,void *) ,EQUIPMENT *pequipment)
{
   int i;
   HNDLE hKey;

   for (i = 0; i < hv_info->num_channels; i++)
      array[i] = (float)default_value;
   if (db_find_key(hDB, hv_info->hKeyRoot, path, &hKey) != DB_SUCCESS)
      for (i = 0; i < hv_info->num_channels; i++)
         device_driver(hv_info->driver[i], cmd,
                       i - hv_info->channel_offset[i], array + i);
   db_merge_data(hDB, hv_info->hKeyRoot, path, array, sizeof(float) * hv_info->num_channels,
                 hv_info->num_channels, TID_FLOAT);
   db_find_key(hDB, hv_info->hKeyRoot, path, &hKey);
   assert(hKey);
   db_open_record(hDB, hKey, array, sizeof(float) * hv_info->num_channels, MODE_READ, 
                  callback, pequipment);
}

void validate_odb_array_bool(HNDLE hDB, HV_INFO *hv_info, char *path, double default_value, int cmd,
                        DWORD *array, void (*callback)(INT,INT,void *) ,EQUIPMENT *pequipment)
{
   int i;
   HNDLE hKey;
   
   for (i = 0; i < hv_info->num_channels; i++)
      array[i] = (DWORD)default_value;
   if (db_find_key(hDB, hv_info->hKeyRoot, path, &hKey) != DB_SUCCESS)
      for (i = 0; i < hv_info->num_channels; i++){
         device_driver(hv_info->driver[i], cmd, i - hv_info->channel_offset[i], array + i);
         array[i] = 0;
      }
   db_merge_data(hDB, hv_info->hKeyRoot, path, array, sizeof(DWORD) * hv_info->num_channels, hv_info->num_channels, TID_DWORD);
   db_find_key(hDB, hv_info->hKeyRoot, path, &hKey);
   assert(hKey);
   db_open_record(hDB, hKey, array, sizeof(DWORD) * hv_info->num_channels, MODE_READ,
                  callback, pequipment);
}

void validate_odb_int(HNDLE hDB, HV_INFO *hv_info, char *path, double default_value, int cmd,
                        int *target, void (*callback)(INT,INT,void *) ,EQUIPMENT *pequipment)
{
   int i;
   HNDLE hKey;

   for (i = 0; i < 1; i++)
      target[i] = (INT)default_value;
   if (db_find_key(hDB, hv_info->hKeyRoot, path, &hKey) != DB_SUCCESS)
      for (i = 0; i < 1; i++){
         device_driver(hv_info->driver[i], cmd, i - hv_info->channel_offset[i], target + i);
      }
   db_merge_data(hDB, hv_info->hKeyRoot, path, target, sizeof(INT), 1, TID_INT);
   db_find_key(hDB, hv_info->hKeyRoot, path, &hKey);
   assert(hKey);
   db_open_record(hDB, hKey, target, sizeof(INT), MODE_READ,callback, pequipment);
}

/*------------------------------------------------------------------*/

INT hv_init(EQUIPMENT * pequipment)
{
   int status, size, i, j, index, offset;
   char str[256];
   HNDLE hDB, hKey;
   HV_INFO *hv_info;

   /* allocate private data */
   pequipment->cd_info = calloc(1, sizeof(HV_INFO));
   hv_info = (HV_INFO *) pequipment->cd_info;

   /* get class driver root key */
   cm_get_experiment_database(&hDB, NULL);
   sprintf(str, "/Equipment/%s", pequipment->name);
   db_create_key(hDB, 0, str, TID_KEY);
   db_find_key(hDB, 0, str, &hv_info->hKeyRoot);

   /* save event format */
   size = sizeof(str);
   db_get_value(hDB, hv_info->hKeyRoot, "Common/Format", str, &size, TID_STRING, TRUE);

   if (equal_ustring(str, "Fixed"))
      hv_info->format = FORMAT_FIXED;
   else if (equal_ustring(str, "MIDAS"))
      hv_info->format = FORMAT_MIDAS;
   else if (equal_ustring(str, "YBOS"))
      hv_info->format = FORMAT_YBOS;

   /* count total number of channels */
   for (i = 0, hv_info->num_channels = 0; pequipment->driver[i].name[0]; i++) {
      if (pequipment->driver[i].channels == 0) {
         cm_msg(MERROR, "hv_init", "Driver with zero channels not allowed");
         return FE_ERR_ODB;
      }

      hv_info->num_channels += pequipment->driver[i].channels;
   }

   if (hv_info->num_channels == 0) {
      cm_msg(MERROR, "hv_init", "No channels found in device driver list");
      return FE_ERR_ODB;
   }

   /* Allocate memory for buffers */
   hv_info->names = (char *) calloc(hv_info->num_channels, NAME_LENGTH);

   hv_info->demand = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->measured = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->current = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->chStatus = (DWORD *) calloc(hv_info->num_channels, sizeof(DWORD));
   hv_info->temperature = (float *) calloc(hv_info->num_channels, sizeof(float));

   hv_info->update_threshold = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->update_threshold_current = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->zero_threshold = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->voltage_limit = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->current_limit = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->rampup_speed = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->rampdown_speed = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->trip_time = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->chState = (DWORD *) calloc(hv_info->num_channels, sizeof(DWORD));
   hv_info->crateMap = (INT *) calloc(hv_info->num_channels, sizeof(INT));

   hv_info->demand_mirror = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->measured_mirror = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->current_mirror = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->chStatus_mirror = (DWORD *) calloc(hv_info->num_channels, sizeof(DWORD));
   hv_info->temperature_mirror = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->last_change = (DWORD *) calloc(hv_info->num_channels, sizeof(DWORD));

   hv_info->dd_info = (void *) calloc(hv_info->num_channels, sizeof(void *));
   hv_info->channel_offset = (INT *) calloc(hv_info->num_channels, sizeof(INT));
   hv_info->driver = (void *) calloc(hv_info->num_channels, sizeof(void *));

   if (!hv_info->driver) {
      cm_msg(MERROR, "hv_init", "Not enough memory");
      return FE_ERR_ODB;
   }

   /*---- Initialize device drivers ----*/

   /* call init method */
   for (i = 0; pequipment->driver[i].name[0]; i++) {
      sprintf(str, "Settings/Devices/%s", pequipment->driver[i].name);
      status = db_find_key(hDB, hv_info->hKeyRoot, str, &hKey);
      if (status != DB_SUCCESS) {
         db_create_key(hDB, hv_info->hKeyRoot, str, TID_KEY);
         status = db_find_key(hDB, hv_info->hKeyRoot, str, &hKey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "hv_init", "Cannot create %s entry in online database", str);
            free_mem(hv_info);
            return FE_ERR_ODB;
         }
      }

      status = device_driver(&pequipment->driver[i], CMD_INIT, hKey);
      if (status != FE_SUCCESS) {
         free_mem(hv_info);
         return status;
      }
   }

   /* compose device driver channel assignment */
   for (i = 0, j = 0, index = 0, offset = 0; i < hv_info->num_channels; i++, j++) {
      while (j >= pequipment->driver[index].channels && pequipment->driver[index].name[0]) {
         offset += j;
         index++;
         j = 0;
      }

      hv_info->driver[i] = &pequipment->driver[index];
      hv_info->channel_offset[i] = offset;
   }

   /*---- Create/Read settings ----*/

   /* Names */
   /*  no good - pushes from the ODB back down onto the crate, want the other way!
   //default names
   for (i = 0; i < hv_info->num_channels; i++)
      sprintf(hv_info->names + NAME_LENGTH * i, "Default%%CH %d", i);

   //do some stuff if the names aren't defined in the ODB
   if (db_find_key(hDB, hv_info->hKeyRoot, "Settings/Names", &hKey) != DB_SUCCESS)
      for (i = 0; i < hv_info->num_channels; i++){
         device_driver(hv_info->driver[i], CMD_GET_LABEL, i - hv_info->channel_offset[i], hv_info->names + NAME_LENGTH * i);
      }

   //populate hv_info->names with whatever's in the ODB
   db_merge_data(hDB, hv_info->hKeyRoot, "Settings/Names",
                 hv_info->names, NAME_LENGTH * hv_info->num_channels,
                 hv_info->num_channels, TID_STRING);

   //push down to the crate?
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Names", &hKey);
   assert(hKey);
   db_open_record(hDB, hKey, hv_info->names, NAME_LENGTH * hv_info->num_channels,
                  MODE_READ, hv_update_label, pequipment);
   */
   //Rewrite to pull only from the crate; note this makes the channel names 
   //read-only from MIDAS!  (good - don't mess my channel names).   
   //get names from the crate
   for (i = 0; i < hv_info->num_channels; i++){
      device_driver(hv_info->driver[i], CMD_GET_LABEL, i - hv_info->channel_offset[i], hv_info->names + NAME_LENGTH * i);
   }
   
   //write to ODB
   if (db_find_key(hDB, hv_info->hKeyRoot, "Settings/Names", &hKey) != DB_SUCCESS)
       db_merge_data(hDB, hv_info->hKeyRoot, "Settings/Names",
                     hv_info->names, NAME_LENGTH * hv_info->num_channels,
                     hv_info->num_channels, TID_STRING);
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Names", &hKey);
   assert(hKey);
   db_open_record(hDB, hKey, hv_info->names, NAME_LENGTH * hv_info->num_channels,
                  MODE_WRITE, NULL, pequipment);

   /* Update threshold */
   validate_odb_array(hDB, hv_info, "Settings/Update Threshold Measured", 0.5, CMD_GET_THRESHOLD, 
                      hv_info->update_threshold, NULL, NULL);

   /* Update threshold current */
   validate_odb_array(hDB, hv_info, "Settings/Update Threshold Current", 1, CMD_GET_THRESHOLD_CURRENT, 
                      hv_info->update_threshold_current, NULL, NULL);

   /* Zero threshold */
   validate_odb_array(hDB, hv_info, "Settings/Zero Threshold", 20, CMD_GET_THRESHOLD_ZERO, 
                      hv_info->zero_threshold, NULL, NULL);

   /* Voltage limit */
   validate_odb_array(hDB, hv_info, "Settings/Voltage Limit", 3000, CMD_GET_VOLTAGE_LIMIT, 
                      hv_info->voltage_limit, hv_set_voltage_limit, pequipment);

   /* Current limit */
   validate_odb_array(hDB, hv_info, "Settings/Current Limit", 3000, CMD_GET_CURRENT_LIMIT, 
                      hv_info->current_limit, hv_set_current_limit, pequipment);

   /* Trip Time */
   validate_odb_array(hDB, hv_info, "Settings/Trip Time", 10, CMD_GET_TRIP_TIME, 
                      hv_info->trip_time, hv_set_trip_time, pequipment);

   /* Ramp up */
   validate_odb_array(hDB, hv_info, "Settings/Ramp Up Speed", 0, CMD_GET_RAMPUP, 
                      hv_info->rampup_speed, hv_set_rampup, pequipment);

   /* Ramp down */
   validate_odb_array(hDB, hv_info, "Settings/Ramp Down Speed", 0, CMD_GET_RAMPDOWN, 
                      hv_info->rampdown_speed, hv_set_rampdown, pequipment);

   /* Channel State */
   validate_odb_array_bool(hDB, hv_info, "Settings/ChState", 'n', CMD_GET_CHSTATE,
                      hv_info->chState, hv_set_chState, pequipment);

   /* Crate Map */
   validate_odb_int(hDB, hv_info, "Settings/Devices/sy2527/DD/crateMap", 'n', CMD_GET_CRATEMAP,
                      hv_info->crateMap, NULL, pequipment);

   /*---- Create/Read variables ----*/

   /* Demand */
   db_merge_data(hDB, hv_info->hKeyRoot, "Variables/Demand",
                 hv_info->demand, sizeof(float) * hv_info->num_channels,
                 hv_info->num_channels, TID_FLOAT);
   db_find_key(hDB, hv_info->hKeyRoot, "Variables/Demand", &hv_info->hKeyDemand);

   /* Measured */
   db_merge_data(hDB, hv_info->hKeyRoot, "Variables/Measured",
                 hv_info->measured, sizeof(float) * hv_info->num_channels,
                 hv_info->num_channels, TID_FLOAT);
   db_find_key(hDB, hv_info->hKeyRoot, "Variables/Measured", &hv_info->hKeyMeasured);
   memcpy(hv_info->measured_mirror, hv_info->measured,
          hv_info->num_channels * sizeof(float));

   /* Current */
   db_merge_data(hDB, hv_info->hKeyRoot, "Variables/Current",
                 hv_info->current, sizeof(float) * hv_info->num_channels,
                 hv_info->num_channels, TID_FLOAT);
   db_find_key(hDB, hv_info->hKeyRoot, "Variables/Current", &hv_info->hKeyCurrent);
   memcpy(hv_info->current_mirror, hv_info->current,
          hv_info->num_channels * sizeof(float));

   /* Status */
   db_merge_data(hDB, hv_info->hKeyRoot, "Variables/ChStatus",
                 hv_info->chStatus, sizeof(DWORD) * hv_info->num_channels,
                 hv_info->num_channels, TID_DWORD);
   db_find_key(hDB, hv_info->hKeyRoot, "Variables/ChStatus", &hv_info->hKeyChStatus);
   memcpy(hv_info->chStatus_mirror, hv_info->chStatus,
          hv_info->num_channels * sizeof(DWORD));

   /* Temperature */
   db_merge_data(hDB, hv_info->hKeyRoot, "Variables/Temperature",
                 hv_info->temperature, sizeof(float) * hv_info->num_channels,
                 hv_info->num_channels, TID_FLOAT);
   db_find_key(hDB, hv_info->hKeyRoot, "Variables/Temperature", &hv_info->hKeyTemperature);
   memcpy(hv_info->temperature_mirror, hv_info->temperature,
          hv_info->num_channels * sizeof(float));

   /*---- set labels form midas SC names ----*/
   //for (i = 0; i < hv_info->num_channels; i++) {
   //   status = device_driver(hv_info->driver[i], CMD_SET_LABEL, 
   //                          i - hv_info->channel_offset[i], hv_info->names + NAME_LENGTH * i);
   //}

   /*---- set/get values ----*/
   for (i = 0; i < hv_info->num_channels; i++) {
      if ((hv_info->driver[i]->flags & DF_PRIO_DEVICE) == 0) {
         hv_info->demand_mirror[i] = MIN(hv_info->demand[i], hv_info->voltage_limit[i]);
         status = device_driver(hv_info->driver[i], CMD_SET, 
                                i - hv_info->channel_offset[i], hv_info->demand_mirror[i]);
         status = device_driver(hv_info->driver[i], CMD_SET_TRIP_TIME,
                                i - hv_info->channel_offset[i], hv_info->trip_time[i]);
         status = device_driver(hv_info->driver[i], CMD_SET_CURRENT_LIMIT,
                                i - hv_info->channel_offset[i], hv_info->current_limit[i]);
         status = device_driver(hv_info->driver[i], CMD_SET_VOLTAGE_LIMIT,
                                i - hv_info->channel_offset[i], hv_info->voltage_limit[i]);
         status = device_driver(hv_info->driver[i], CMD_SET_RAMPUP,
                                i - hv_info->channel_offset[i], hv_info->rampup_speed[i]);
         status = device_driver(hv_info->driver[i], CMD_SET_RAMPDOWN,
                                i - hv_info->channel_offset[i], hv_info->rampdown_speed[i]);
         status = device_driver(hv_info->driver[i], CMD_SET_CHSTATE,
                                i - hv_info->channel_offset[i], hv_info->chState[i]);
      } else {
         status = device_driver(hv_info->driver[i], CMD_GET_DEMAND,
                                i - hv_info->channel_offset[i], hv_info->demand + i);
         hv_info->demand_mirror[i] = hv_info->demand[i];

         status = device_driver(hv_info->driver[i], CMD_GET_TRIP_TIME,
                                i - hv_info->channel_offset[i], &hv_info->trip_time[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_CURRENT_LIMIT,
                                i - hv_info->channel_offset[i], &hv_info->current_limit[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_VOLTAGE_LIMIT,
                                i - hv_info->channel_offset[i], &hv_info->voltage_limit[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_RAMPUP,
                                i - hv_info->channel_offset[i], &hv_info->rampup_speed[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_RAMPDOWN,
                                i - hv_info->channel_offset[i], &hv_info->rampdown_speed[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_CHSTATE,
                                i - hv_info->channel_offset[i], &hv_info->chState[i]);
         status = device_driver(hv_info->driver[i], CMD_GET_CRATEMAP,
                                i - hv_info->channel_offset[i], &hv_info->crateMap[i]);
      }
   }
   db_set_record(hDB, hv_info->hKeyDemand, hv_info->demand,
                 hv_info->num_channels * sizeof(float), 0);

   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Trip Time", &hKey);
   db_set_record(hDB, hKey, hv_info->trip_time, hv_info->num_channels * sizeof(float), 0);
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Current Limit", &hKey);
   db_set_record(hDB, hKey, hv_info->current_limit, hv_info->num_channels * sizeof(float), 0);
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Voltage Limit", &hKey);
   db_set_record(hDB, hKey, hv_info->voltage_limit, hv_info->num_channels * sizeof(float), 0);
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Ramp Up Speed", &hKey);
   db_set_record(hDB, hKey, hv_info->rampup_speed, hv_info->num_channels * sizeof(float), 0);
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Ramp Down Speed", &hKey);
   db_set_record(hDB, hKey, hv_info->rampdown_speed, hv_info->num_channels * sizeof(float), 0);
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/ChState", &hKey);
   db_set_record(hDB, hKey, hv_info->chState, hv_info->num_channels * sizeof(DWORD), 'n');
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Devices/sy2527/DD/crateMap", &hKey);
   db_set_record(hDB, hKey, hv_info->crateMap, sizeof(INT), 'n');

   /*--- open hotlink to HV demand values ----*/
   db_open_record(hDB, hv_info->hKeyDemand, hv_info->demand,
                  hv_info->num_channels * sizeof(float), MODE_READ, hv_demand,
                  pequipment);

   /* initially read all channels */
   for (i=0 ; i<hv_info->num_channels ; i++) {

      hv_info->driver[i]->dd(CMD_GET, hv_info->driver[i]->dd_info, 
                             i - hv_info->channel_offset[i], &hv_info->measured[i]);
      hv_info->driver[i]->dd(CMD_GET_CURRENT, hv_info->driver[i]->dd_info, 
                             i - hv_info->channel_offset[i], &hv_info->current[i]);
      hv_info->driver[i]->dd(CMD_GET_STATUS, hv_info->driver[i]->dd_info,
                             i - hv_info->channel_offset[i], &hv_info->chStatus[i]);
      //cm_msg(MINFO, "hv_init", "chStatus %d", hv_info->chStatus[i]);
      hv_info->driver[i]->dd(CMD_GET_TEMPERATURE, hv_info->driver[i]->dd_info,
                             i - hv_info->channel_offset[i], &hv_info->temperature[i]);

      hv_info->measured_mirror[i] = hv_info->measured[i];
      hv_info->current_mirror[i]  = hv_info->current[i];
      hv_info->chStatus_mirror[i]  = hv_info->chStatus[i];
      hv_info->temperature_mirror[i]  = hv_info->temperature[i];
   }

   db_set_data(hDB, hv_info->hKeyCurrent, hv_info->current,
               sizeof(float) * hv_info->num_channels, hv_info->num_channels,
               TID_FLOAT);

   db_set_data(hDB, hv_info->hKeyChStatus, hv_info->chStatus,
               sizeof(DWORD) * hv_info->num_channels, hv_info->num_channels,
               TID_DWORD);

   db_set_data(hDB, hv_info->hKeyTemperature, hv_info->temperature,
               sizeof(float) * hv_info->num_channels, hv_info->num_channels,
               TID_FLOAT);

   pequipment->odb_out++;
   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hv_exit(EQUIPMENT * pequipment)
{
   INT i;

   free_mem((HV_INFO *) pequipment->cd_info);

   /* call exit method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL; i++)
      device_driver(&pequipment->driver[i], CMD_EXIT);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hv_idle(EQUIPMENT * pequipment)
{
   INT act, status;
   DWORD act_time;
   HV_INFO *hv_info;

   hv_info = (HV_INFO *) pequipment->cd_info;

   /* do ramping */
   hv_ramp(hv_info);

   /* select next measurement channel */
   hv_info->last_channel = (hv_info->last_channel + 1) % hv_info->num_channels;

   /* measure channel */
   status = hv_read(pequipment, hv_info->last_channel);

   /* additionally read channel recently updated if not multithreaded */
   if (!(hv_info->driver[hv_info->last_channel]->flags & DF_MULTITHREAD)) {

      act_time = ss_millitime();

      act = (hv_info->last_channel_updated + 1) % hv_info->num_channels;
      while (!(act_time - hv_info->last_change[act] < 10000)) {
         act = (act + 1) % hv_info->num_channels;
         if (act == hv_info->last_channel_updated) {
            /* non found, so return */
            return status;
         }
      }

      /* updated channel found, so read it additionally */
      status = hv_read(pequipment, act);
      hv_info->last_channel_updated = act;
   }

   return status;
}

/*------------------------------------------------------------------*/

INT cd_hv_read(char *pevent, int offset)
{
   float *pdata;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;
#ifdef HAVE_YBOS
   DWORD *pdw;
#endif

   pequipment = *((EQUIPMENT **) pevent);
   hv_info = (HV_INFO *) pequipment->cd_info;

   if (hv_info->format == FORMAT_FIXED) {
      memcpy(pevent, hv_info->demand, sizeof(float) * hv_info->num_channels);
      pevent += sizeof(float) * hv_info->num_channels;

      memcpy(pevent, hv_info->measured, sizeof(float) * hv_info->num_channels);
      pevent += sizeof(float) * hv_info->num_channels;

      memcpy(pevent, hv_info->current, sizeof(float) * hv_info->num_channels);
      pevent += sizeof(float) * hv_info->num_channels;

      memcpy(pevent, hv_info->chStatus, sizeof(DWORD) * hv_info->num_channels);
      pevent += sizeof(DWORD) * hv_info->num_channels;

      memcpy(pevent, hv_info->temperature, sizeof(float) * hv_info->num_channels);
      pevent += sizeof(float) * hv_info->num_channels;

      return ( 4 * sizeof(float) + sizeof(DWORD) )* hv_info->num_channels;
   } else if (hv_info->format == FORMAT_MIDAS) {
      bk_init(pevent);

      /* create DMND bank */
      bk_create(pevent, "DMND", TID_FLOAT, &pdata);
      memcpy(pdata, hv_info->demand, sizeof(float) * hv_info->num_channels);
      pdata += hv_info->num_channels;
      bk_close(pevent, pdata);

      /* create MSRD bank */
      bk_create(pevent, "MSRD", TID_FLOAT, &pdata);
      memcpy(pdata, hv_info->measured, sizeof(float) * hv_info->num_channels);
      pdata += hv_info->num_channels;
      bk_close(pevent, pdata);

      /* create CRNT bank */
      bk_create(pevent, "CRNT", TID_FLOAT, &pdata);
      memcpy(pdata, hv_info->current, sizeof(float) * hv_info->num_channels);
      pdata += hv_info->num_channels;
      bk_close(pevent, pdata);

      /* create STAT bank */
      bk_create(pevent, "STAT", TID_DWORD, &pdata);
      memcpy(pdata, hv_info->chStatus, sizeof(DWORD) * hv_info->num_channels); 
      pdata += hv_info->num_channels;
      bk_close(pevent, pdata);

      /* create TPTR bank */
      bk_create(pevent, "TPTR", TID_FLOAT, &pdata);
      memcpy(pdata, hv_info->temperature, sizeof(float) * hv_info->num_channels); 
      pdata += hv_info->num_channels;
      bk_close(pevent, pdata);

      return bk_size(pevent);
   } else if (hv_info->format == FORMAT_YBOS) {
#ifdef HAVE_YBOS
      ybk_init((DWORD *) pevent);

      /* create EVID bank */
      ybk_create((DWORD *) pevent, "EVID", I4_BKTYPE, (DWORD *) (&pdw));
      *(pdw)++ = EVENT_ID(pevent);      /* Event_ID + Mask */
      *(pdw)++ = SERIAL_NUMBER(pevent); /* Serial number */
      *(pdw)++ = TIME_STAMP(pevent);    /* Time Stamp */
      ybk_close((DWORD *) pevent, pdw);

      /* create DMND bank */
      ybk_create((DWORD *) pevent, "DMND", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, hv_info->demand, sizeof(float) * hv_info->num_channels);
      pdata += hv_info->num_channels;
      ybk_close((DWORD *) pevent, pdata);

      /* create MSRD bank */
      ybk_create((DWORD *) pevent, "MSRD", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, hv_info->measured, sizeof(float) * hv_info->num_channels);
      pdata += hv_info->num_channels;
      ybk_close((DWORD *) pevent, pdata);

      /* create CRNT bank */
      ybk_create((DWORD *) pevent, "CRNT", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, hv_info->current, sizeof(float) * hv_info->num_channels);
      pdata += hv_info->num_channels;
      ybk_close((DWORD *) pevent, pdata);

      /* create STAT bank */ 
      ybk_create((DWORD *) pevent, "STAT", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, hv_info->chStatus, sizeof(DWORD) * hv_info->num_channels);
      pdata += hv_info->num_channels;
      ybk_close((DWORD *) pevent, pdata);

      /* create TPTR bank */
      ybk_create((DWORD *) pevent, "TPTR", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, hv_info->temperature, sizeof(float) * hv_info->num_channels); 
      pdata += hv_info->num_channels;
      ybk_close((DWORD *) pevent, pdata);

      return ybk_size((DWORD *) pevent);
#else
      assert(!"YBOS support not compiled in");
#endif
   }

   return 0;
}

/*------------------------------------------------------------------*/

INT cd_hv(INT cmd, PEQUIPMENT pequipment)
{
   INT status;

   switch (cmd) {
   case CMD_INIT:
      status = hv_init(pequipment);
      break;

   case CMD_EXIT:
      status = hv_exit(pequipment);
      break;

   case CMD_START:
      status = hv_start(pequipment);
      break;

   case CMD_STOP:
      status = hv_stop(pequipment);
      break;

   case CMD_IDLE:
      status = hv_idle(pequipment);
      break;

   default:
      cm_msg(MERROR, "HV class driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   return status;
}

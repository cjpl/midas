/********************************************************************\

  Name:         hv.c
  Created by:   Stefan Ritt

  Contents:     High Voltage Class Driver

  $Id$

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "midas.h"
#include "ybos.h"

typedef struct {

   /* ODB keys */
   HNDLE hKeyRoot, hKeyDemand, hKeyMeasured, hKeyCurrent;

   /* globals */
   INT num_channels;
   INT format;
   DWORD last_update;
   INT last_channel;

   /* items in /Variables record */
   char *names;
   float *demand;
   float *measured;
   float *current;

   /* items in /Settings */
   float *update_threshold;
   float *update_threshold_current;
   float *voltage_limit;
   float *current_limit;
   float *rampup_speed;
   float *rampdown_speed;
   float *trip_time;

   /* mirror arrays */
   float *demand_mirror;
   float *measured_mirror;
   float *current_mirror;
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

   free(hv_info->update_threshold);
   free(hv_info->update_threshold_current);
   free(hv_info->voltage_limit);
   free(hv_info->current_limit);
   free(hv_info->rampup_speed);
   free(hv_info->rampdown_speed);
   free(hv_info->trip_time);

   free(hv_info->demand_mirror);
   free(hv_info->measured_mirror);
   free(hv_info->current_mirror);
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

   /* call close method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
      device_driver(&pequipment->driver[i], CMD_START);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT hv_stop(EQUIPMENT * pequipment)
{
   INT i;

   /* call close method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
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

   status = device_driver(hv_info->driver[channel], CMD_GET,
                              channel - hv_info->channel_offset[channel],
                              &hv_info->measured[channel]);
   status = device_driver(hv_info->driver[channel], CMD_GET_CURRENT,
                              channel - hv_info->channel_offset[channel],
                              &hv_info->current[channel]);

   /* check how much channels have changed since last ODB update */
   act_time = ss_millitime();

   /* check for update measured */
   max_diff = 0.f;
   min_time = 10000;
   changed = FALSE;
   for (i = 0; i < hv_info->num_channels; i++) {
      if (abs(hv_info->measured[i] - hv_info->measured_mirror[i]) > max_diff)
         max_diff = abs(hv_info->measured[i] - hv_info->measured_mirror[i]);

      if (abs(hv_info->measured[i] - hv_info->measured_mirror[i]) >
          hv_info->update_threshold[i])
         changed = TRUE;

      if (act_time - hv_info->last_change[i] < min_time)
         min_time = act_time - hv_info->last_change[i];
   }

   /* update if change is more than update_sensitivity or less than 2sec ago 
      or last update is older than a minute */
   if (changed || (min_time < 2000 && max_diff > 0) ||
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

   /* update if change is more than update_sensitivity or less than 2sec ago */
   if (changed || (min_time < 2000 && max_diff > 0)) {
      for (i = 0; i < hv_info->num_channels; i++)
         hv_info->current_mirror[i] = hv_info->current[i];

      db_set_data(hDB, hv_info->hKeyCurrent, hv_info->current,
                  sizeof(float) * hv_info->num_channels, hv_info->num_channels,
                  TID_FLOAT);

      pequipment->odb_out++;
   }

   return status;
}

/*------------------------------------------------------------------*/

INT hv_ramp(HV_INFO * hv_info)
{
   INT i, status, switch_tag;
   float delta, ramp_speed;

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
      if (hv_info->demand[i] > hv_info->voltage_limit[i])
         hv_info->demand[i] = hv_info->voltage_limit[i];

   /* set individual channels only if demand value differs */
   for (i = 0; i < hv_info->num_channels; i++)
      if (hv_info->demand[i] != hv_info->demand_mirror[i] ||
            fabs(hv_info->measured[i] - hv_info->demand[i]) > 100.f) {
         hv_info->last_change[i] = ss_millitime();
      }

   pequipment->odb_in++;

   /* execute one ramping */
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

   /* update channel labels based on the midas channel names */
   for (i = 0; i < hv_info->num_channels; i++)
      status = device_driver(hv_info->driver[i], CMD_SET_LABEL,
                             i - hv_info->channel_offset[i],
                             hv_info->names + NAME_LENGTH * i);
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

   hv_info->update_threshold = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->update_threshold_current =
       (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->voltage_limit = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->current_limit = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->rampup_speed = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->rampdown_speed = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->trip_time = (float *) calloc(hv_info->num_channels, sizeof(float));

   hv_info->demand_mirror = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->measured_mirror = (float *) calloc(hv_info->num_channels, sizeof(float));
   hv_info->current_mirror = (float *) calloc(hv_info->num_channels, sizeof(float));
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
   for (i = 0; i < hv_info->num_channels; i++)
      sprintf(hv_info->names + NAME_LENGTH * i, "Default%%CH %d", i);
   if (db_find_key(hDB, hv_info->hKeyRoot, "Settings/Names", &hKey) != DB_SUCCESS)
      for (i = 0; i < hv_info->num_channels; i++)
         device_driver(hv_info->driver[i], CMD_GET_LABEL,
                       i - hv_info->channel_offset[i], hv_info->names + NAME_LENGTH * i);
   db_merge_data(hDB, hv_info->hKeyRoot, "Settings/Names",
                 hv_info->names, NAME_LENGTH * hv_info->num_channels,
                 hv_info->num_channels, TID_STRING);
   db_find_key(hDB, hv_info->hKeyRoot, "Settings/Names", &hKey);
   assert(hKey);
   db_open_record(hDB, hKey, hv_info->names, NAME_LENGTH * hv_info->num_channels,
                  MODE_READ, hv_update_label, pequipment);

   /* Update threshold */
   validate_odb_array(hDB, hv_info, "Settings/Update Threshold Measured", 0.5, CMD_GET_THRESHOLD, 
                      hv_info->update_threshold, NULL, NULL);

   /* Update threshold current */
   validate_odb_array(hDB, hv_info, "Settings/Update Threshold Current", 1, CMD_GET_THRESHOLD_CURRENT, 
                      hv_info->update_threshold_current, NULL, NULL);

   /* Voltage limit */
   validate_odb_array(hDB, hv_info, "Settings/Voltage Limit", 3000, CMD_GET_VOLTAGE_LIMIT, 
                      hv_info->voltage_limit, hv_set_voltage_limit, pequipment);

   /* Current limit */
   validate_odb_array(hDB, hv_info, "Settings/Current Limit", 3000, CMD_GET_CURRENT_LIMIT, 
                      hv_info->current_limit, hv_set_current_limit, pequipment);

   /* Trip Time */
   validate_odb_array(hDB, hv_info, "Settings/Trip Time", 10, CMD_GET_TRIP_TIME, 
                      hv_info->current_limit, hv_set_trip_time, pequipment);

   /* Ramp up */
   validate_odb_array(hDB, hv_info, "Settings/Ramp Up Speed", 0, CMD_GET_RAMPUP, 
                      hv_info->rampup_speed, hv_set_rampup, pequipment);

   /* Ramp down */
   validate_odb_array(hDB, hv_info, "Settings/Ramp Down Speed", 0, CMD_GET_RAMPDOWN, 
                      hv_info->rampdown_speed, hv_set_rampdown, pequipment);

   /*---- Create/Read variables ----*/

   /* Demand */
   db_merge_data(hDB, hv_info->hKeyRoot, "Variables/Demand",
                 hv_info->demand, sizeof(float) * hv_info->num_channels,
                 hv_info->num_channels, TID_FLOAT);
   db_find_key(hDB, hv_info->hKeyRoot, "Variables/Demand", &hv_info->hKeyDemand);
   db_open_record(hDB, hv_info->hKeyDemand, hv_info->demand,
                  hv_info->num_channels * sizeof(float), MODE_READ, hv_demand,
                  pequipment);

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

   /*---- set labels form midas SC names ----*/
   for (i = 0; i < hv_info->num_channels; i++) {
      status = device_driver(hv_info->driver[i], CMD_SET_LABEL, 
                             i - hv_info->channel_offset[i], hv_info->names + NAME_LENGTH * i);
   }

   /*---- set/get demand value ----*/
   for (i = 0; i < hv_info->num_channels; i++) {
      if ((hv_info->driver[i]->flags & DF_PRIO_DEVICE) == 0) {
         hv_info->demand_mirror[i] = MIN(hv_info->demand[i], hv_info->voltage_limit[i]);
         status = device_driver(hv_info->driver[i], CMD_SET, 
                                i - hv_info->channel_offset[i], hv_info->demand_mirror[i]);
      } else {
         status = device_driver(hv_info->driver[i], CMD_GET_DEMAND,
                                i - hv_info->channel_offset[i], hv_info->demand + i);
         hv_info->demand_mirror[i] = hv_info->demand[i];
      }
   }
   db_set_record(hDB, hv_info->hKeyDemand, hv_info->demand,
                 hv_info->num_channels * sizeof(float), 0);

   /*---- set limits and ramping speeds ----*/
   for (i = 0; i < hv_info->num_channels; i++) {
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
   }

   /* initially read all channels */
   for (i=0 ; i<hv_info->num_channels ; i++)
      hv_read(pequipment, i);

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
   act_time = ss_millitime();
   act = (hv_info->last_channel + 1) % hv_info->num_channels;

   /* look for the next channel recently updated */
   while (!(act_time - hv_info->last_change[act] < 5000 ||
            (act_time - hv_info->last_change[act] < 20000
             && abs(hv_info->measured[act] - hv_info->demand[act]) >
             2 * hv_info->update_threshold[act]))) {
      act = (act + 1) % hv_info->num_channels;
      if (act == hv_info->last_channel) {
         act = (hv_info->last_channel + 1) % hv_info->num_channels;
         break;
      }
   }

   /* measure channel */
   status = hv_read(pequipment, act);
   hv_info->last_channel = act;

   return status;
}

/*------------------------------------------------------------------*/

INT cd_hv_read(char *pevent, int offset)
{
   float *pdata;
   DWORD *pdw;
   HV_INFO *hv_info;
   EQUIPMENT *pequipment;

   pequipment = *((EQUIPMENT **) pevent);
   hv_info = (HV_INFO *) pequipment->cd_info;

   if (hv_info->format == FORMAT_FIXED) {
      memcpy(pevent, hv_info->demand, sizeof(float) * hv_info->num_channels);
      pevent += sizeof(float) * hv_info->num_channels;

      memcpy(pevent, hv_info->measured, sizeof(float) * hv_info->num_channels);
      pevent += sizeof(float) * hv_info->num_channels;

      memcpy(pevent, hv_info->current, sizeof(float) * hv_info->num_channels);
      pevent += sizeof(float) * hv_info->num_channels;

      return 3 * sizeof(float) * hv_info->num_channels;
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

      return bk_size(pevent);
   } else if (hv_info->format == FORMAT_YBOS) {
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

      return ybk_size((DWORD *) pevent);
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

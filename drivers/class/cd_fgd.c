/********************************************************************\

  Name:         cd_fgd.c
  Created by:   Pierre-Andre Amaudruz

  Based on the Generic class
  Contents:     FGD T2K Class Driver
  Handles the Fine Grid Detector frontend card
  Used for midas/mscb/embedded/Experiements/fgd/
  
  Set HV to the SiPm through a Q pump
  Get Overall current for the SiPm
  Get temperature of the uP (T1)
  Get temperature of the I2C local sensor (T2)
  Get temperature of the I2C remote sensor (T3)

  Requires CMD_GET_TEMPERATURE1/2/3 in the midas.h
  uses the mscbfgd.c and standard mscb suite.
  (mscb, mscbrpc, musbstd)

  $Id: cd_fgd.c$

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "ybos.h"

typedef struct {

   /* ODB keys */
   HNDLE hKeyRoot, hKeyDemand, hKeyMeasured, hKeyTemp1, hKeyTemp2, hKeyTemp3;

   /* globals */
   INT num_channels;
   INT format;
   INT last_channel;

   /* items in /Variables record */
   float *demand;
   float *measured;
   float *temp1;
   float *temp2;
   float *temp3;

   /* items in /Settings */
   char *names;
   float *update_threshold;

   /* mirror arrays */
   float *demand_mirror;
   float *measured_mirror;

   DEVICE_DRIVER **driver;
   INT *channel_offset;

} FGD_INFO;

#ifndef abs
#define abs(a) (((a) < 0)   ? -(a) : (a))
#endif

/*------------------------------------------------------------------*/

static void free_mem(FGD_INFO * fgd_info)
{
   free(fgd_info->names);
   free(fgd_info->demand);
   free(fgd_info->measured);
   free(fgd_info->temp1);
   free(fgd_info->temp2);
   free(fgd_info->temp3);

   free(fgd_info->update_threshold);

   free(fgd_info->demand_mirror);
   free(fgd_info->measured_mirror);

   free(fgd_info->channel_offset);
   free(fgd_info->driver);

   free(fgd_info);
}

/*------------------------------------------------------------------*/
// Read current 
INT fgd_read(EQUIPMENT * pequipment, int channel)
{
   static DWORD last_time = 0;
   int i, status;
   FGD_INFO *fgd_info;
   HNDLE hDB;

   /*---- read measured value ----*/

   fgd_info = (FGD_INFO *) pequipment->cd_info;
   cm_get_experiment_database(&hDB, NULL);

   //status = device_driver(fgd_info->driver[channel], CMD_GET_CURRENT,
   //                       channel - fgd_info->channel_offset[channel],
   //                       &fgd_info->measured[channel]);

   ///* check for update measured */
   //for (i = 0; i < fgd_info->num_channels; i++) {
   //   /* update if change is more than update_threshold */
   //   if (abs(fgd_info->measured[i] - fgd_info->measured_mirror[i]) >
   //       fgd_info->update_threshold[i]) {
   //      for (i = 0; i < fgd_info->num_channels; i++)
   //         fgd_info->measured_mirror[i] = fgd_info->measured[i];

   //      db_set_data(hDB, fgd_info->hKeyMeasured, fgd_info->measured,
   //                  sizeof(float) * fgd_info->num_channels, fgd_info->num_channels,
   //                  TID_FLOAT);

   //      pequipment->odb_out++;

   //      break;
   //   }
   //}

   // Get the temperatures
   if ((ss_time() - last_time) > 1) {
     channel = 0;
     status = device_driver(fgd_info->driver[channel], CMD_GET_TEMPERATURE1,
       channel - fgd_info->channel_offset[channel],
       &fgd_info->temp1[channel]);
     db_set_data(hDB, fgd_info->hKeyTemp1, fgd_info->temp1,
       sizeof(float) * fgd_info->num_channels, fgd_info->num_channels,
       TID_FLOAT);
     status = device_driver(fgd_info->driver[channel], CMD_GET_TEMPERATURE2,
       channel - fgd_info->channel_offset[channel],
       &fgd_info->temp2[channel]);
     db_set_data(hDB, fgd_info->hKeyTemp2, fgd_info->temp2,
       sizeof(float) * fgd_info->num_channels, fgd_info->num_channels,
       TID_FLOAT);
     status = device_driver(fgd_info->driver[channel], CMD_GET_TEMPERATURE3,
       channel - fgd_info->channel_offset[channel],
       &fgd_info->temp3[channel]);
     db_set_data(hDB, fgd_info->hKeyTemp3, fgd_info->temp3,
       sizeof(float) * fgd_info->num_channels, fgd_info->num_channels,
       TID_FLOAT);
     last_time = ss_time();
   }
   return status;
}

/*------------------------------------------------------------------*/

void fgd_demand(INT hDB, INT hKey, void *info)
{
   INT i, status;
   FGD_INFO *fgd_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   fgd_info = (FGD_INFO *) pequipment->cd_info;

   /* set individual channels only if demand value differs */
   for (i = 0; i < fgd_info->num_channels; i++)
      if (fgd_info->demand[i] != fgd_info->demand_mirror[i]) {
         if ((fgd_info->driver[i]->flags & DF_READ_ONLY) == 0) {
            status = device_driver(fgd_info->driver[i], CMD_SET,  // Voltage
                                   i - fgd_info->channel_offset[i], fgd_info->demand[i]);
         }
         fgd_info->demand_mirror[i] = fgd_info->demand[i];
      }

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void fgd_update_label(INT hDB, INT hKey, void *info)
{
   INT i, status;
   FGD_INFO *fgd_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   fgd_info = (FGD_INFO *) pequipment->cd_info;

   /* update channel labels based on the midas channel names */
   for (i = 0; i < fgd_info->num_channels; i++)
      status = device_driver(fgd_info->driver[i], CMD_SET_LABEL,
                             i - fgd_info->channel_offset[i],
                             fgd_info->names + NAME_LENGTH * i);
}

/*------------------------------------------------------------------*/

INT fgd_init(EQUIPMENT * pequipment)
{
   int status, size, i, j, index, offset;
   char str[256];
   HNDLE hDB, hKey, hNames, hThreshold;
   FGD_INFO *fgd_info;

   /* allocate private data */
   pequipment->cd_info = calloc(1, sizeof(FGD_INFO));
   fgd_info = (FGD_INFO *) pequipment->cd_info;

   /* get class driver root key */
   cm_get_experiment_database(&hDB, NULL);
   sprintf(str, "/Equipment/%s", pequipment->name);
   db_create_key(hDB, 0, str, TID_KEY);
   db_find_key(hDB, 0, str, &fgd_info->hKeyRoot);

   /* save event format */
   size = sizeof(str);
   db_get_value(hDB, fgd_info->hKeyRoot, "Common/Format", str, &size, TID_STRING, TRUE);

   if (equal_ustring(str, "Fixed"))
      fgd_info->format = FORMAT_FIXED;
   else if (equal_ustring(str, "MIDAS"))
      fgd_info->format = FORMAT_MIDAS;
   else if (equal_ustring(str, "YBOS"))
      fgd_info->format = FORMAT_YBOS;

   /* count total number of channels */
   for (i = 0, fgd_info->num_channels = 0; pequipment->driver[i].name[0]; i++) {
      if (pequipment->driver[i].channels == 0) {
         cm_msg(MERROR, "fgd_init", "Driver with zero channels not allowed");
         return FE_ERR_ODB;
      }

      fgd_info->num_channels += pequipment->driver[i].channels;
   }

   if (fgd_info->num_channels == 0) {
      cm_msg(MERROR, "fgd_init", "No channels found in device driver list");
      return FE_ERR_ODB;
   }

   /* Allocate memory for buffers */
   fgd_info->names = (char *) calloc(fgd_info->num_channels, NAME_LENGTH);

   fgd_info->demand = (float *) calloc(fgd_info->num_channels, sizeof(float));
   fgd_info->measured = (float *) calloc(fgd_info->num_channels, sizeof(float));

   fgd_info->temp1 = (float *) calloc(fgd_info->num_channels, sizeof(float));
   fgd_info->temp2 = (float *) calloc(fgd_info->num_channels, sizeof(float));
   fgd_info->temp3 = (float *) calloc(fgd_info->num_channels, sizeof(float));

   fgd_info->update_threshold = (float *) calloc(fgd_info->num_channels, sizeof(float));

   fgd_info->demand_mirror = (float *) calloc(fgd_info->num_channels, sizeof(float));
   fgd_info->measured_mirror = (float *) calloc(fgd_info->num_channels, sizeof(float));

   fgd_info->channel_offset = (INT *) calloc(fgd_info->num_channels, sizeof(INT));
   fgd_info->driver = (void *) calloc(fgd_info->num_channels, sizeof(void *));

   if (!fgd_info->driver) {
      cm_msg(MERROR, "hv_init", "Not enough memory");
      return FE_ERR_ODB;
   }

   /*---- Initialize device drivers ----*/

   /* call init method */
   for (i = 0; pequipment->driver[i].name[0]; i++) {
      sprintf(str, "Settings/Devices/%s", pequipment->driver[i].name);
      status = db_find_key(hDB, fgd_info->hKeyRoot, str, &hKey);
      if (status != DB_SUCCESS) {
         db_create_key(hDB, fgd_info->hKeyRoot, str, TID_KEY);
         status = db_find_key(hDB, fgd_info->hKeyRoot, str, &hKey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "hv_init", "Cannot create %s entry in online database", str);
            free_mem(fgd_info);
            return FE_ERR_ODB;
         }
      }

      status = device_driver(&pequipment->driver[i], CMD_INIT, hKey);
      if (status != FE_SUCCESS) {
         free_mem(fgd_info);
         return status;
      }
   }

   /* compose device driver channel assignment */
   for (i = 0, j = 0, index = 0, offset = 0; i < fgd_info->num_channels; i++, j++) {
      while (j >= pequipment->driver[index].channels && pequipment->driver[index].name[0]) {
         offset += j;
         index++;
         j = 0;
      }

      fgd_info->driver[i] = &pequipment->driver[index];
      fgd_info->channel_offset[i] = offset;
   }

   /*---- create demand variables ----*/
   /* get demand from ODB */
   status =
       db_find_key(hDB, fgd_info->hKeyRoot, "Variables/Demand", &fgd_info->hKeyDemand);
   if (status == DB_SUCCESS) {
      size = sizeof(float) * fgd_info->num_channels;
      db_get_data(hDB, fgd_info->hKeyDemand, fgd_info->demand, &size, TID_FLOAT);
   }
   /* let device driver overwrite demand values, if it supports it */
   for (i = 0; i < fgd_info->num_channels; i++) {
      if (fgd_info->driver[i]->flags & DF_PRIO_DEVICE) {
         device_driver(fgd_info->driver[i], CMD_GET_DEMAND,
                       i - fgd_info->channel_offset[i], &fgd_info->demand[i]);
         fgd_info->demand_mirror[i] = fgd_info->demand[i];
      } else
         fgd_info->demand_mirror[i] = -12345.f; /* use -12345 as invalid value */
   }
   /* write back demand values */
   status =
       db_find_key(hDB, fgd_info->hKeyRoot, "Variables/Demand", &fgd_info->hKeyDemand);
   if (status != DB_SUCCESS) {
      db_create_key(hDB, fgd_info->hKeyRoot, "Variables/Demand", TID_FLOAT);
      db_find_key(hDB, fgd_info->hKeyRoot, "Variables/Demand", &fgd_info->hKeyDemand);
   }
   size = sizeof(float) * fgd_info->num_channels;
   db_set_data(hDB, fgd_info->hKeyDemand, fgd_info->demand, size,
               fgd_info->num_channels, TID_FLOAT);
   db_open_record(hDB, fgd_info->hKeyDemand, fgd_info->demand,
                  fgd_info->num_channels * sizeof(float), MODE_READ, fgd_demand,
                  pequipment);

   /*---- create measured variables ----*/
   db_merge_data(hDB, fgd_info->hKeyRoot, "Variables/Current Measured",
                 fgd_info->measured, sizeof(float) * fgd_info->num_channels,
                 fgd_info->num_channels, TID_FLOAT);
   db_find_key(hDB, fgd_info->hKeyRoot, "Variables/Current Measured", &fgd_info->hKeyMeasured);
   memcpy(fgd_info->measured_mirror, fgd_info->measured,
          fgd_info->num_channels * sizeof(float));

   /*---- create Temp1 measured variables ----*/
   db_merge_data(hDB, fgd_info->hKeyRoot, "Variables/Temp1",
                 fgd_info->temp1, sizeof(float) * fgd_info->num_channels,
                 fgd_info->num_channels, TID_FLOAT);
   db_find_key(hDB, fgd_info->hKeyRoot, "Variables/Temp1", &fgd_info->hKeyTemp1);

   /*---- create Temp2 measured variables ----*/
   db_merge_data(hDB, fgd_info->hKeyRoot, "Variables/Temp2",
                 fgd_info->temp2, sizeof(float) * fgd_info->num_channels,
                 fgd_info->num_channels, TID_FLOAT);
   db_find_key(hDB, fgd_info->hKeyRoot, "Variables/Temp2", &fgd_info->hKeyTemp2);

   /*---- create Temp3 measured variables ----*/
   db_merge_data(hDB, fgd_info->hKeyRoot, "Variables/Temp3",
                 fgd_info->temp3, sizeof(float) * fgd_info->num_channels,
                 fgd_info->num_channels, TID_FLOAT);
   db_find_key(hDB, fgd_info->hKeyRoot, "Variables/Temp3", &fgd_info->hKeyTemp3);

   /*---- get default names from device driver ----*/
   for (i = 0; i < fgd_info->num_channels; i++) {
      sprintf(fgd_info->names + NAME_LENGTH * i, "Default%%CH %d", i);
      device_driver(fgd_info->driver[i], CMD_GET_LABEL,
                    i - fgd_info->channel_offset[i], fgd_info->names + NAME_LENGTH * i);
   }
   db_merge_data(hDB, fgd_info->hKeyRoot, "Settings/Names",
                 fgd_info->names, NAME_LENGTH * fgd_info->num_channels,
                 fgd_info->num_channels, TID_STRING);

   /*---- set labels form midas SC names ----*/
   for (i = 0; i < fgd_info->num_channels; i++) {
      fgd_info = (FGD_INFO *) pequipment->cd_info;
      device_driver(fgd_info->driver[i], CMD_SET_LABEL,
                    i - fgd_info->channel_offset[i], fgd_info->names + NAME_LENGTH * i);
   }

   /* open hotlink on channel names */
   if (db_find_key(hDB, fgd_info->hKeyRoot, "Settings/Names", &hNames) == DB_SUCCESS)
      db_open_record(hDB, hNames, fgd_info->names, NAME_LENGTH*fgd_info->num_channels,
                     MODE_READ, fgd_update_label, pequipment);

   /*---- get default update threshold from device driver ----*/
   for (i = 0; i < fgd_info->num_channels; i++) {
      fgd_info->update_threshold[i] = 1.f;      /* default 1 unit */
      device_driver(fgd_info->driver[i], CMD_GET_THRESHOLD,
                    i - fgd_info->channel_offset[i], &fgd_info->update_threshold[i]);
   }
   db_merge_data(hDB, fgd_info->hKeyRoot, "Settings/Update Threshold Measured",
                 fgd_info->update_threshold, sizeof(float)*fgd_info->num_channels,
                 fgd_info->num_channels, TID_FLOAT);

   /* open hotlink on update threshold */
   if (db_find_key(hDB, fgd_info->hKeyRoot, "Settings/Update Threshold Measured", &hThreshold) == DB_SUCCESS)
     db_open_record(hDB, hThreshold, fgd_info->update_threshold, sizeof(float)*fgd_info->num_channels,
		    MODE_READ, NULL, NULL);
   
   /*---- set initial demand values ----*/
   // fgd_demand(hDB, fgd_info->hKeyDemand, pequipment);

   /* initially read all channels */
   for (i = 0; i < fgd_info->num_channels; i++)
      fgd_read(pequipment, i);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT fgd_start(EQUIPMENT * pequipment)
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

INT fgd_stop(EQUIPMENT * pequipment)
{
   INT i;

   /* call stop method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
      device_driver(&pequipment->driver[i], CMD_STOP);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT fgd_exit(EQUIPMENT * pequipment)
{
   INT i;

   free_mem((FGD_INFO *) pequipment->cd_info);

   /* call exit method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL; i++)
      device_driver(&pequipment->driver[i], CMD_EXIT);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT fgd_idle(EQUIPMENT * pequipment)
{
   INT act, status;
   FGD_INFO *fgd_info;

   fgd_info = (FGD_INFO *) pequipment->cd_info;

   /* select next measurement channel */
   act = (fgd_info->last_channel + 1) % fgd_info->num_channels;

   /* measure channel */
   status = fgd_read(pequipment, act);
   fgd_info->last_channel = act;

   return status;
}

/*------------------------------------------------------------------*/

INT cd_fgd_read(char *pevent, int offset)
{
   float *pdata;
   FGD_INFO *fgd_info;
   EQUIPMENT *pequipment;

   pequipment = *((EQUIPMENT **) pevent);
   fgd_info = (FGD_INFO *) pequipment->cd_info;

   if (fgd_info->format == FORMAT_FIXED) {
      memcpy(pevent, fgd_info->demand, sizeof(float) * fgd_info->num_channels);
      pevent += sizeof(float) * fgd_info->num_channels;

      memcpy(pevent, fgd_info->measured, sizeof(float) * fgd_info->num_channels);
      pevent += sizeof(float) * fgd_info->num_channels;

      return 2 * sizeof(float) * fgd_info->num_channels;
   } else if (fgd_info->format == FORMAT_MIDAS) {
      bk_init(pevent);

      /* create VDMD bank */
      bk_create(pevent, "VDMD", TID_FLOAT, &pdata);
      memcpy(pdata, fgd_info->demand, sizeof(float) * fgd_info->num_channels);
      pdata += fgd_info->num_channels;
      bk_close(pevent, pdata);

      /* create IMES bank */
      bk_create(pevent, "IMES", TID_FLOAT, &pdata);
      memcpy(pdata, fgd_info->measured, sizeof(float) * fgd_info->num_channels);
      pdata += fgd_info->num_channels;
      bk_close(pevent, pdata);

      /* create TEMP1 bank */
      bk_create(pevent, "TEM1", TID_FLOAT, &pdata);
      memcpy(pdata, fgd_info->temp1, sizeof(float) * fgd_info->num_channels);
      pdata += fgd_info->num_channels;
      bk_close(pevent, pdata);

      /* create TEMP2 bank */
      bk_create(pevent, "TEM2", TID_FLOAT, &pdata);
      memcpy(pdata, fgd_info->temp2, sizeof(float) * fgd_info->num_channels);
      pdata += fgd_info->num_channels;
      bk_close(pevent, pdata);

      /* create TEMP1 bank */
      bk_create(pevent, "TEM3", TID_FLOAT, &pdata);
      memcpy(pdata, fgd_info->temp3, sizeof(float) * fgd_info->num_channels);
      pdata += fgd_info->num_channels;
      bk_close(pevent, pdata);

      return bk_size(pevent);
   } else if (fgd_info->format == FORMAT_YBOS) {
     printf("Not implemented\n");
     return 0;
   } else
     return 0;
}

/*------------------------------------------------------------------*/

INT cd_fgd(INT cmd, EQUIPMENT * pequipment)
{
   INT status;

   switch (cmd) {
   case CMD_INIT:
      status = fgd_init(pequipment);
      break;

   case CMD_START:
      status = fgd_start(pequipment);
      break;

   case CMD_STOP:
      status = fgd_stop(pequipment);
      break;

   case CMD_EXIT:
      status = fgd_exit(pequipment);
      break;

   case CMD_IDLE:
      status = fgd_idle(pequipment);
      break;

   default:
      cm_msg(MERROR, "Generic class driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   return status;
}

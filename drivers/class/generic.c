/********************************************************************\

  Name:         generic.c
  Created by:   Stefan Ritt

  Contents:     Generic Class Driver

  $Id$

\********************************************************************/

#include <stdio.h>
#include <assert.h>
#include "midas.h"
#include "ybos.h"

typedef struct {

   /* ODB keys */
   HNDLE hKeyRoot, hKeyDemand, hKeyMeasured;

   /* globals */
   INT num_channels;
   INT format;
   INT last_channel;

   /* items in /Variables record */
   float *demand;
   float *measured;

   /* items in /Settings */
   char *names;
   float *update_threshold;

   /* mirror arrays */
   float *demand_mirror;
   float *measured_mirror;

   DEVICE_DRIVER **driver;
   INT *channel_offset;

} GEN_INFO;

#ifndef abs
#define abs(a) (((a) < 0)   ? -(a) : (a))
#endif

/*------------------------------------------------------------------*/

static void free_mem(GEN_INFO * gen_info)
{
   free(gen_info->names);
   free(gen_info->demand);
   free(gen_info->measured);

   free(gen_info->update_threshold);

   free(gen_info->demand_mirror);
   free(gen_info->measured_mirror);

   free(gen_info->channel_offset);
   free(gen_info->driver);

   free(gen_info);
}

/*------------------------------------------------------------------*/

INT gen_read(EQUIPMENT * pequipment, int channel)
{
   int i, status;
   GEN_INFO *gen_info;
   HNDLE hDB;
   gen_info = (GEN_INFO *) pequipment->cd_info;
   cm_get_experiment_database(&hDB, NULL);

   /* if driver is multi-threaded, read all channels at once */
   for (i=0 ; i < gen_info->num_channels ; i++) {
      if (gen_info->driver[i]->flags & DF_MULTITHREAD) {
         status = device_driver(gen_info->driver[i], CMD_GET,
                                i - gen_info->channel_offset[i],
                                &gen_info->measured[i]);
      }
   }

   /* else read only single channel */
   if (!(gen_info->driver[channel]->flags & DF_MULTITHREAD))
      status = device_driver(gen_info->driver[channel], CMD_GET,
                             channel - gen_info->channel_offset[channel],
                             &gen_info->measured[channel]);

   /* check for update measured */
   for (i = 0; i < gen_info->num_channels; i++) {
      /* update if change is more than update_threshold */
      if (abs(gen_info->measured[i] - gen_info->measured_mirror[i]) >
          gen_info->update_threshold[i]) {
         for (i = 0; i < gen_info->num_channels; i++)
            gen_info->measured_mirror[i] = gen_info->measured[i];

         db_set_data(hDB, gen_info->hKeyMeasured, gen_info->measured,
                     sizeof(float) * gen_info->num_channels, gen_info->num_channels,
                     TID_FLOAT);

         pequipment->odb_out++;

         break;
      }
   }

   /*---- read demand value ----*/

   status = device_driver(gen_info->driver[channel], CMD_GET_DEMAND,
                          channel - gen_info->channel_offset[channel],
                          &gen_info->demand[channel]);

   if (gen_info->demand[channel] != gen_info->demand_mirror[channel]) {
      gen_info->demand_mirror[channel] = gen_info->demand[channel];
      db_set_data(hDB, gen_info->hKeyDemand, gen_info->demand,
                  sizeof(float) * gen_info->num_channels, gen_info->num_channels,
                  TID_FLOAT);
   }

   return status;
}

/*------------------------------------------------------------------*/

void gen_demand(INT hDB, INT hKey, void *info)
{
   INT i, status;
   GEN_INFO *gen_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   gen_info = (GEN_INFO *) pequipment->cd_info;

   /* set individual channels only if demand value differs */
   for (i = 0; i < gen_info->num_channels; i++)
      if (gen_info->demand[i] != gen_info->demand_mirror[i]) {
         if ((gen_info->driver[i]->flags & DF_READ_ONLY) == 0) {
            status = device_driver(gen_info->driver[i], CMD_SET,
                                   i - gen_info->channel_offset[i], gen_info->demand[i]);
         }
         gen_info->demand_mirror[i] = gen_info->demand[i];
      }

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void gen_update_label(INT hDB, INT hKey, void *info)
{
   INT i, status;
   GEN_INFO *gen_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   gen_info = (GEN_INFO *) pequipment->cd_info;

   /* update channel labels based on the midas channel names */
   for (i = 0; i < gen_info->num_channels; i++)
      status = device_driver(gen_info->driver[i], CMD_SET_LABEL,
                             i - gen_info->channel_offset[i],
                             gen_info->names + NAME_LENGTH * i);
}

/*------------------------------------------------------------------*/

INT gen_init(EQUIPMENT * pequipment)
{
   int status, size, i, j, index, offset;
   char str[256];
   HNDLE hDB, hKey, hNames, hThreshold;
   GEN_INFO *gen_info;

   /* allocate private data */
   pequipment->cd_info = calloc(1, sizeof(GEN_INFO));
   gen_info = (GEN_INFO *) pequipment->cd_info;

   /* get class driver root key */
   cm_get_experiment_database(&hDB, NULL);
   sprintf(str, "/Equipment/%s", pequipment->name);
   db_create_key(hDB, 0, str, TID_KEY);
   db_find_key(hDB, 0, str, &gen_info->hKeyRoot);

   /* save event format */
   size = sizeof(str);
   db_get_value(hDB, gen_info->hKeyRoot, "Common/Format", str, &size, TID_STRING, TRUE);

   if (equal_ustring(str, "Fixed"))
      gen_info->format = FORMAT_FIXED;
   else if (equal_ustring(str, "MIDAS"))
      gen_info->format = FORMAT_MIDAS;
   else if (equal_ustring(str, "YBOS"))
      gen_info->format = FORMAT_YBOS;

   /* count total number of channels */
   for (i = 0, gen_info->num_channels = 0; pequipment->driver[i].name[0]; i++) {
      if (pequipment->driver[i].channels == 0) {
         cm_msg(MERROR, "gen_init", "Driver with zero channels not allowed");
         return FE_ERR_ODB;
      }

      gen_info->num_channels += pequipment->driver[i].channels;
   }

   if (gen_info->num_channels == 0) {
      cm_msg(MERROR, "gen_init", "No channels found in device driver list");
      return FE_ERR_ODB;
   }

   /* Allocate memory for buffers */
   gen_info->names = (char *) calloc(gen_info->num_channels, NAME_LENGTH);

   gen_info->demand = (float *) calloc(gen_info->num_channels, sizeof(float));
   gen_info->measured = (float *) calloc(gen_info->num_channels, sizeof(float));

   gen_info->update_threshold = (float *) calloc(gen_info->num_channels, sizeof(float));

   gen_info->demand_mirror = (float *) calloc(gen_info->num_channels, sizeof(float));
   gen_info->measured_mirror = (float *) calloc(gen_info->num_channels, sizeof(float));

   gen_info->channel_offset = (INT *) calloc(gen_info->num_channels, sizeof(INT));
   gen_info->driver = (void *) calloc(gen_info->num_channels, sizeof(void *));

   if (!gen_info->driver) {
      cm_msg(MERROR, "hv_init", "Not enough memory");
      return FE_ERR_ODB;
   }

   /*---- Initialize device drivers ----*/

   /* call init method */
   for (i = 0; pequipment->driver[i].name[0]; i++) {
      sprintf(str, "Settings/Devices/%s", pequipment->driver[i].name);
      status = db_find_key(hDB, gen_info->hKeyRoot, str, &hKey);
      if (status != DB_SUCCESS) {
         db_create_key(hDB, gen_info->hKeyRoot, str, TID_KEY);
         status = db_find_key(hDB, gen_info->hKeyRoot, str, &hKey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "hv_init", "Cannot create %s entry in online database", str);
            free_mem(gen_info);
            return FE_ERR_ODB;
         }
      }

      status = device_driver(&pequipment->driver[i], CMD_INIT, hKey);
      if (status != FE_SUCCESS) {
         free_mem(gen_info);
         return status;
      }
   }

   /* compose device driver channel assignment */
   for (i = 0, j = 0, index = 0, offset = 0; i < gen_info->num_channels; i++, j++) {
      while (j >= pequipment->driver[index].channels && pequipment->driver[index].name[0]) {
         offset += j;
         index++;
         j = 0;
      }

      gen_info->driver[i] = &pequipment->driver[index];
      gen_info->channel_offset[i] = offset;
   }

   /*---- create demand variables ----*/

   /* get demand from ODB */
   status =
       db_find_key(hDB, gen_info->hKeyRoot, "Variables/Demand", &gen_info->hKeyDemand);
   if (status == DB_SUCCESS) {
      size = sizeof(float) * gen_info->num_channels;
      db_get_data(hDB, gen_info->hKeyDemand, gen_info->demand, &size, TID_FLOAT);
   }
   /* let device driver overwrite demand values, if it supports it */
   for (i = 0; i < gen_info->num_channels; i++) {
      if (gen_info->driver[i]->flags & DF_PRIO_DEVICE) {
         device_driver(gen_info->driver[i], CMD_GET_DEMAND,
                       i - gen_info->channel_offset[i], &gen_info->demand[i]);
         gen_info->demand_mirror[i] = gen_info->demand[i];
      } else
         gen_info->demand_mirror[i] = -12345.f; /* use -12345 as invalid value */
   }
   /* write back demand values */
   status =
       db_find_key(hDB, gen_info->hKeyRoot, "Variables/Demand", &gen_info->hKeyDemand);
   if (status != DB_SUCCESS) {
      db_create_key(hDB, gen_info->hKeyRoot, "Variables/Demand", TID_FLOAT);
      db_find_key(hDB, gen_info->hKeyRoot, "Variables/Demand", &gen_info->hKeyDemand);
   }
   size = sizeof(float) * gen_info->num_channels;
   db_set_data(hDB, gen_info->hKeyDemand, gen_info->demand, size,
               gen_info->num_channels, TID_FLOAT);
   db_open_record(hDB, gen_info->hKeyDemand, gen_info->demand,
                  gen_info->num_channels * sizeof(float), MODE_READ, gen_demand,
                  pequipment);

   /*---- create measured variables ----*/
   db_merge_data(hDB, gen_info->hKeyRoot, "Variables/Measured",
                 gen_info->measured, sizeof(float) * gen_info->num_channels,
                 gen_info->num_channels, TID_FLOAT);
   db_find_key(hDB, gen_info->hKeyRoot, "Variables/Measured", &gen_info->hKeyMeasured);
   memcpy(gen_info->measured_mirror, gen_info->measured,
          gen_info->num_channels * sizeof(float));

   /*---- get default names from device driver ----*/
   for (i = 0; i < gen_info->num_channels; i++) {
      sprintf(gen_info->names + NAME_LENGTH * i, "Default%%CH %d", i);
      device_driver(gen_info->driver[i], CMD_GET_LABEL,
                    i - gen_info->channel_offset[i], gen_info->names + NAME_LENGTH * i);
   }
   db_merge_data(hDB, gen_info->hKeyRoot, "Settings/Names",
                 gen_info->names, NAME_LENGTH * gen_info->num_channels,
                 gen_info->num_channels, TID_STRING);

   /*---- set labels form midas SC names ----*/
   for (i = 0; i < gen_info->num_channels; i++) {
      gen_info = (GEN_INFO *) pequipment->cd_info;
      device_driver(gen_info->driver[i], CMD_SET_LABEL,
                    i - gen_info->channel_offset[i], gen_info->names + NAME_LENGTH * i);
   }

   /* open hotlink on channel names */
   if (db_find_key(hDB, gen_info->hKeyRoot, "Settings/Names", &hNames) == DB_SUCCESS)
      db_open_record(hDB, hNames, gen_info->names, NAME_LENGTH*gen_info->num_channels,
                     MODE_READ, gen_update_label, pequipment);

   /*---- get default update threshold from device driver ----*/
   for (i = 0; i < gen_info->num_channels; i++) {
      gen_info->update_threshold[i] = 1.f;      /* default 1 unit */
      device_driver(gen_info->driver[i], CMD_GET_THRESHOLD,
                    i - gen_info->channel_offset[i], &gen_info->update_threshold[i]);
   }
   db_merge_data(hDB, gen_info->hKeyRoot, "Settings/Update Threshold Measured",
                 gen_info->update_threshold, sizeof(float)*gen_info->num_channels,
                 gen_info->num_channels, TID_FLOAT);

   /* open hotlink on update threshold */
   if (db_find_key(hDB, gen_info->hKeyRoot, "Settings/Update Threshold Measured", &hThreshold) == DB_SUCCESS)
      db_open_record(hDB, hThreshold, gen_info->update_threshold, sizeof(float)*gen_info->num_channels,
                     MODE_READ, NULL, NULL);

   /*---- set initial demand values ----*/
   gen_demand(hDB, gen_info->hKeyDemand, pequipment);

   /* initially read all channels */
   for (i = 0; i < gen_info->num_channels; i++)
      gen_read(pequipment, i);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT gen_start(EQUIPMENT * pequipment)
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

INT gen_stop(EQUIPMENT * pequipment)
{
   INT i;

   /* call stop method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
      device_driver(&pequipment->driver[i], CMD_STOP);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT gen_exit(EQUIPMENT * pequipment)
{
   INT i;

   free_mem((GEN_INFO *) pequipment->cd_info);

   /* call exit method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL; i++)
      device_driver(&pequipment->driver[i], CMD_EXIT);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT gen_idle(EQUIPMENT * pequipment)
{
   INT act, status;
   GEN_INFO *gen_info;

   gen_info = (GEN_INFO *) pequipment->cd_info;

   /* select next measurement channel */
   act = (gen_info->last_channel + 1) % gen_info->num_channels;

   /* measure channel */
   status = gen_read(pequipment, act);
   gen_info->last_channel = act;

   return status;
}

/*------------------------------------------------------------------*/

INT cd_gen_read(char *pevent, int offset)
{
   float *pdata;
   GEN_INFO *gen_info;
   EQUIPMENT *pequipment;
#ifdef HAVE_YBOS
   DWORD *pdw;
#endif

   pequipment = *((EQUIPMENT **) pevent);
   gen_info = (GEN_INFO *) pequipment->cd_info;

   if (gen_info->format == FORMAT_FIXED) {
      memcpy(pevent, gen_info->demand, sizeof(float) * gen_info->num_channels);
      pevent += sizeof(float) * gen_info->num_channels;

      memcpy(pevent, gen_info->measured, sizeof(float) * gen_info->num_channels);
      pevent += sizeof(float) * gen_info->num_channels;

      return 2 * sizeof(float) * gen_info->num_channels;
   } else if (gen_info->format == FORMAT_MIDAS) {
      bk_init(pevent);

      /* create DMND bank */
      bk_create(pevent, "DMND", TID_FLOAT, &pdata);
      memcpy(pdata, gen_info->demand, sizeof(float) * gen_info->num_channels);
      pdata += gen_info->num_channels;
      bk_close(pevent, pdata);

      /* create MSRD bank */
      bk_create(pevent, "MSRD", TID_FLOAT, &pdata);
      memcpy(pdata, gen_info->measured, sizeof(float) * gen_info->num_channels);
      pdata += gen_info->num_channels;
      bk_close(pevent, pdata);

      return bk_size(pevent);
   } else if (gen_info->format == FORMAT_YBOS) {
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
      memcpy(pdata, gen_info->demand, sizeof(float) * gen_info->num_channels);
      pdata += gen_info->num_channels;
      ybk_close((DWORD *) pevent, pdata);

      /* create MSRD bank */
      ybk_create((DWORD *) pevent, "MSRD", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, gen_info->measured, sizeof(float) * gen_info->num_channels);
      pdata += gen_info->num_channels;
      ybk_close((DWORD *) pevent, pdata);

      return ybk_size((DWORD *) pevent);
#else
      assert(!"YBOS support not compiled in");
#endif
   } else
      return 0;
}

/*------------------------------------------------------------------*/

INT cd_gen(INT cmd, EQUIPMENT * pequipment)
{
   INT status;

   switch (cmd) {
   case CMD_INIT:
      status = gen_init(pequipment);
      break;

   case CMD_START:
     status = gen_start(pequipment);
     break;
     
   case CMD_STOP:
      status = gen_stop(pequipment);
      break;

   case CMD_EXIT:
      status = gen_exit(pequipment);
      break;

   case CMD_IDLE:
      status = gen_idle(pequipment);
      break;

   default:
      cm_msg(MERROR, "Generic class driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   return status;
}

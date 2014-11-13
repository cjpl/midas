/********************************************************************\

  Name:         multi.c
  Created by:   Stefan Ritt

  Contents:     Multimeter Class Driver

  $Id$

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "ybos.h"

typedef struct {
   /* ODB keys */
   HNDLE hDB, hKeyRoot;
   HNDLE hKeyOutput, hKeyInput;

   /* globals */
   INT num_channels_input, num_channels_output;
   INT format;
   INT last_channel;
   DWORD last_update;

   /* items in /Variables record */
   char *names_input, *names_output;
   float *var_input;
   float *var_output;

   /* items in /Settings */
   float *update_threshold;
   float *factor_input, *factor_output;
   float *offset_input, *offset_output;

   /* mirror arrays */
   float *input_mirror;
   float *output_mirror;

   DEVICE_DRIVER **driver_input, **driver_output;
   INT *channel_offset_input, *channel_offset_output;

} MULTI_INFO;

#ifndef abs
#define abs(a) (((a) < 0)   ? -(a) : (a))
#endif

/*----------------------------------------------------------------------------*/

static void free_mem(MULTI_INFO * m_info)
{
   free(m_info->names_input);
   free(m_info->names_output);

   free(m_info->var_input);
   free(m_info->var_output);

   free(m_info->update_threshold);
   free(m_info->offset_input);
   free(m_info->factor_input);
   free(m_info->offset_output);
   free(m_info->factor_output);

   free(m_info->input_mirror);
   free(m_info->output_mirror);

   free(m_info->channel_offset_input);
   free(m_info->channel_offset_output);

   free(m_info->driver_input);
   free(m_info->driver_output);

   free(m_info);
}

/*----------------------------------------------------------------------------*/

void multi_read(EQUIPMENT * pequipment, int channel)
{
   int i, status;
   DWORD actual_time;
   MULTI_INFO *m_info;
   HNDLE hDB;

   m_info = (MULTI_INFO *) pequipment->cd_info;
   cm_get_experiment_database(&hDB, NULL);

   if (channel == -1 || m_info->driver_input[channel]->flags & DF_MULTITHREAD)
      for (i = 0; i < m_info->num_channels_input; i++) {
         status = device_driver(m_info->driver_input[i], CMD_GET,
                                i - m_info->channel_offset_input[i],
                                &m_info->var_input[i]);
         if (status != FE_SUCCESS)
            m_info->var_input[i] = (float)ss_nan();
         else
            m_info->var_input[i] = m_info->var_input[i] * m_info->factor_input[i] -
               m_info->offset_input[i];
   } else {
      status = device_driver(m_info->driver_input[channel], CMD_GET,
                             channel - m_info->channel_offset_input[channel],
                             &m_info->var_input[channel]);
      if (status != FE_SUCCESS)
         m_info->var_input[channel] = (float)ss_nan();
      else
         m_info->var_input[channel] =
            m_info->var_input[channel] * m_info->factor_input[channel] -
            m_info->offset_input[channel];
   }

   /* check if significant change since last ODB update */
   for (i = 0; i < m_info->num_channels_input; i++)
      if ((abs(m_info->var_input[i] - m_info->input_mirror[i]) >
           m_info->update_threshold[i]) || 
          (ss_isnan(m_info->var_input[i]) && !ss_isnan(m_info->input_mirror[i])) ||
          (!ss_isnan(m_info->var_input[i]) && ss_isnan(m_info->input_mirror[i])))
         break;

#ifdef DEBUG_THRESHOLDS
   if (i < m_info->num_channels_input) {
         printf("%d: %lf -> %lf, threshold %lf\n", i, 
            m_info->var_input[i], m_info->input_mirror[i], m_info->update_threshold[i]);
   }
#endif

   /* update if change is more than update_sensitivity or last update more
      than a minute ago */
   actual_time = ss_millitime();
   if (i < m_info->num_channels_input || actual_time - m_info->last_update > 60000) {
      m_info->last_update = actual_time;

      for (i = 0; i < m_info->num_channels_input; i++)
         m_info->input_mirror[i] = m_info->var_input[i];

      db_set_data(hDB, m_info->hKeyInput, m_info->var_input,
                  m_info->num_channels_input * sizeof(float), m_info->num_channels_input,
                  TID_FLOAT);

      pequipment->odb_out++;
   }
}

/*----------------------------------------------------------------------------*/

void multi_read_output(EQUIPMENT * pequipment, int channel)
{
   float value;
   MULTI_INFO *m_info;
   HNDLE hDB;

   m_info = (MULTI_INFO *) pequipment->cd_info;
   cm_get_experiment_database(&hDB, NULL);

   device_driver(m_info->driver_output[channel], CMD_GET,
                 channel - m_info->channel_offset_output[channel], &value);

   value = (value + m_info->offset_output[channel]) / m_info->factor_output[channel];

   if (value != m_info->output_mirror[channel]) {
      m_info->output_mirror[channel] = value;
      m_info->var_input[channel] = value;

      db_set_record(hDB, m_info->hKeyOutput, m_info->output_mirror,
                    m_info->num_channels_output * sizeof(float), 0);

      pequipment->odb_out++;
   }

}

/*----------------------------------------------------------------------------*/

void multi_output(INT hDB, INT hKey, void *info)
{
   INT i;
   DWORD act_time;
   MULTI_INFO *m_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   m_info = (MULTI_INFO *) pequipment->cd_info;

   act_time = ss_millitime();

   for (i = 0; i < m_info->num_channels_output; i++) {
      /* only set channel if demand value differs */
      if (m_info->var_output[i] != m_info->output_mirror[i]) {
         m_info->output_mirror[i] =
             m_info->var_output[i] * m_info->factor_output[i] - m_info->offset_output[i];

         device_driver(m_info->driver_output[i], CMD_SET,
                       i - m_info->channel_offset_output[i],
                       m_info->output_mirror[i]);
      }
   }

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void multi_update_label(INT hDB, INT hKey, void *info)
{
   INT i, status;
   MULTI_INFO *m_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   m_info = (MULTI_INFO *) pequipment->cd_info;

   /* update channel labels based on the midas channel names */
   for (i = 0; i < m_info->num_channels_input; i++)
      status = device_driver(m_info->driver_input[i], CMD_SET_LABEL,
                             i - m_info->channel_offset_input[i],
                             m_info->names_input + NAME_LENGTH * i);

   for (i = 0; i < m_info->num_channels_output; i++)
      status = device_driver(m_info->driver_output[i], CMD_SET_LABEL,
                             i - m_info->channel_offset_output[i],
                             m_info->names_output + NAME_LENGTH * i);
}

/*----------------------------------------------------------------------------*/

INT multi_init(EQUIPMENT * pequipment)
{
   int status, size, i, j, index, ch_offset;
   char str[256];
   HNDLE hDB, hKey, hNamesIn, hNamesOut;
   MULTI_INFO *m_info;

   /* allocate private data */
   pequipment->cd_info = calloc(1, sizeof(MULTI_INFO));
   m_info = (MULTI_INFO *) pequipment->cd_info;

   /* get class driver root key */
   cm_get_experiment_database(&hDB, NULL);
   sprintf(str, "/Equipment/%s", pequipment->name);
   db_create_key(hDB, 0, str, TID_KEY);
   db_find_key(hDB, 0, str, &m_info->hKeyRoot);

   /* save event format */
   size = sizeof(str);
   db_get_value(hDB, m_info->hKeyRoot, "Common/Format", str, &size, TID_STRING, TRUE);

   if (equal_ustring(str, "Fixed"))
      m_info->format = FORMAT_FIXED;
   else if (equal_ustring(str, "MIDAS"))
      m_info->format = FORMAT_MIDAS;
   else if (equal_ustring(str, "YBOS"))
      m_info->format = FORMAT_YBOS;

   /* count total number of channels */
   for (i = m_info->num_channels_input = m_info->num_channels_output = 0;
        pequipment->driver[i].name[0]; i++) {
      if (pequipment->driver[i].flags & DF_INPUT)
         m_info->num_channels_input += pequipment->driver[i].channels;
      if (pequipment->driver[i].flags & DF_OUTPUT)
         m_info->num_channels_output += pequipment->driver[i].channels;
   }

   if (m_info->num_channels_input == 0 && m_info->num_channels_output == 0) {
      cm_msg(MERROR, "multi_init", "No channels found in device driver list");
      return FE_ERR_ODB;
   }

   /* Allocate memory for buffers */
   if (m_info->num_channels_input) {
      m_info->names_input = (char *) calloc(m_info->num_channels_input, NAME_LENGTH);
      m_info->var_input = (float *) calloc(m_info->num_channels_input, sizeof(float));
      m_info->update_threshold = (float *) calloc(m_info->num_channels_input, sizeof(float));
      m_info->offset_input = (float *) calloc(m_info->num_channels_input, sizeof(float));
      m_info->factor_input = (float *) calloc(m_info->num_channels_input, sizeof(float));
      m_info->input_mirror = (float *) calloc(m_info->num_channels_input, sizeof(float));
      m_info->channel_offset_input = (INT *) calloc(m_info->num_channels_input, sizeof(INT));
      m_info->driver_input = (void *) calloc(m_info->num_channels_input, sizeof(void *));
   }
   
   if (m_info->num_channels_output) {
      m_info->names_output = (char *) calloc(m_info->num_channels_output, NAME_LENGTH);
      m_info->var_output = (float *) calloc(m_info->num_channels_output, sizeof(float));
      m_info->offset_output = (float *) calloc(m_info->num_channels_output, sizeof(float));
      m_info->factor_output = (float *) calloc(m_info->num_channels_output, sizeof(float));
      m_info->output_mirror = (float *) calloc(m_info->num_channels_output, sizeof(float));
      m_info->channel_offset_output = (INT *) calloc(m_info->num_channels_output, sizeof(DWORD));
      m_info->driver_output = (void *) calloc(m_info->num_channels_output, sizeof(void *));
   }
 
   /*---- Create/Read settings ----*/

   if (m_info->num_channels_input) {
      /* Update threshold */
      for (i = 0; i < m_info->num_channels_input; i++)
         m_info->update_threshold[i] = 0.1f;       /* default 0.1 */
      db_merge_data(hDB, m_info->hKeyRoot, "Settings/Update Threshold",
                    m_info->update_threshold, m_info->num_channels_input * sizeof(float),
                    m_info->num_channels_input, TID_FLOAT);
      db_find_key(hDB, m_info->hKeyRoot, "Settings/Update Threshold", &hKey);
      db_open_record(hDB, hKey, m_info->update_threshold,
                     m_info->num_channels_input * sizeof(float), MODE_READ, NULL, NULL);

      /* Offset */
      for (i = 0; i < m_info->num_channels_input; i++)
         m_info->offset_input[i] = 0.f;    /* default 0 */
      db_merge_data(hDB, m_info->hKeyRoot, "Settings/Input Offset",
                    m_info->offset_input, m_info->num_channels_input * sizeof(float),
                    m_info->num_channels_input, TID_FLOAT);
      db_find_key(hDB, m_info->hKeyRoot, "Settings/Input Offset", &hKey);
      db_open_record(hDB, hKey, m_info->offset_input,
                     m_info->num_channels_input * sizeof(float), MODE_READ, NULL, NULL);
   }

   for (i = 0; i < m_info->num_channels_output; i++)
      m_info->offset_output[i] = 0.f;

   if (m_info->num_channels_output) {
      db_merge_data(hDB, m_info->hKeyRoot, "Settings/Output Offset",
                    m_info->offset_output, m_info->num_channels_output * sizeof(float),
                    m_info->num_channels_output, TID_FLOAT);
      db_find_key(hDB, m_info->hKeyRoot, "Settings/Output Offset", &hKey);
      db_open_record(hDB, hKey, m_info->offset_output,
                     m_info->num_channels_output * sizeof(float), MODE_READ, NULL, NULL);
   }

   /* Factor */
   for (i = 0; i < m_info->num_channels_input; i++)
      m_info->factor_input[i] = 1.f;    /* default 1 */

   if (m_info->num_channels_input) {
      db_merge_data(hDB, m_info->hKeyRoot, "Settings/Input Factor",
                    m_info->factor_input, m_info->num_channels_input * sizeof(float),
                    m_info->num_channels_input, TID_FLOAT);
      db_find_key(hDB, m_info->hKeyRoot, "Settings/Input factor", &hKey);
      db_open_record(hDB, hKey, m_info->factor_input,
                     m_info->num_channels_input * sizeof(float), MODE_READ, NULL, NULL);
   }

   if (m_info->num_channels_output) {
      for (i = 0; i < m_info->num_channels_output; i++)
         m_info->factor_output[i] = 1.f;
      db_merge_data(hDB, m_info->hKeyRoot, "Settings/Output Factor",
                    m_info->factor_output, m_info->num_channels_output * sizeof(float),
                    m_info->num_channels_output, TID_FLOAT);
      db_find_key(hDB, m_info->hKeyRoot, "Settings/Output factor", &hKey);
      db_open_record(hDB, hKey, m_info->factor_output,
                     m_info->num_channels_output * sizeof(float), MODE_READ, NULL, NULL);
   }

   /*---- Create/Read variables ----*/

   /* Input */
   if (m_info->num_channels_input) {
      db_merge_data(hDB, m_info->hKeyRoot, "Variables/Input",
                    m_info->var_input, m_info->num_channels_input * sizeof(float),
                    m_info->num_channels_input, TID_FLOAT);
      db_find_key(hDB, m_info->hKeyRoot, "Variables/Input", &m_info->hKeyInput);
      memcpy(m_info->input_mirror, m_info->var_input,
             m_info->num_channels_input * sizeof(float));
   }

   /* Output */
   if (m_info->num_channels_output) {
      db_merge_data(hDB, m_info->hKeyRoot, "Variables/Output",
                    m_info->var_output, m_info->num_channels_output * sizeof(float),
                    m_info->num_channels_output, TID_FLOAT);
      db_find_key(hDB, m_info->hKeyRoot, "Variables/Output", &m_info->hKeyOutput);
   }

   /*---- Initialize device drivers ----*/

   /* call init method */
   for (i = 0; pequipment->driver[i].name[0]; i++) {
      sprintf(str, "Settings/Devices/%s", pequipment->driver[i].name);
      status = db_find_key(hDB, m_info->hKeyRoot, str, &hKey);
      if (status != DB_SUCCESS) {
         db_create_key(hDB, m_info->hKeyRoot, str, TID_KEY);
         status = db_find_key(hDB, m_info->hKeyRoot, str, &hKey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "multi_init", "Cannot create %s entry in online database",
                   str);
            free_mem(m_info);
            return FE_ERR_ODB;
         }
      }

      status = device_driver(&pequipment->driver[i], CMD_INIT, hKey);
      if (status != FE_SUCCESS) {
         free_mem(m_info);
         return status;
      }
   }

   /* compose device driver channel assignment */
   for (i = 0, j = 0, index = 0, ch_offset = 0; i < m_info->num_channels_input; i++, j++) {
      while (pequipment->driver[index].name[0] &&
             (j >= pequipment->driver[index].channels ||
              (pequipment->driver[index].flags & DF_INPUT) == 0)) {
         ch_offset += j;
         index++;
         j = 0;
      }

      m_info->driver_input[i] = &pequipment->driver[index];
      m_info->channel_offset_input[i] = ch_offset;
   }

   for (i = 0, j = 0, index = 0, ch_offset = 0; i < m_info->num_channels_output; i++, j++) {
      while (pequipment->driver[index].name[0] &&
             (j >= pequipment->driver[index].channels ||
              (pequipment->driver[index].flags & DF_OUTPUT) == 0)) {
         ch_offset += j;
         index++;
         j = 0;
      }

      m_info->driver_output[i] = &pequipment->driver[index];
      m_info->channel_offset_output[i] = ch_offset;
   }

   /*---- get default names from device driver ----*/
   if (m_info->num_channels_input) {
      for (i = 0; i < m_info->num_channels_input; i++) {
         sprintf(m_info->names_input + NAME_LENGTH * i, "Input Channel %d", i);
         device_driver(m_info->driver_input[i], CMD_GET_LABEL,
                       i - m_info->channel_offset_input[i], 
                       m_info->names_input + NAME_LENGTH * i);
      }
      db_merge_data(hDB, m_info->hKeyRoot, "Settings/Names Input",
                    m_info->names_input, NAME_LENGTH * m_info->num_channels_input,
                    m_info->num_channels_input, TID_STRING);
   }

   if (m_info->num_channels_output) {
      for (i = 0; i < m_info->num_channels_output; i++) {
         sprintf(m_info->names_output + NAME_LENGTH * i, "Output Channel %d", i);
         device_driver(m_info->driver_output[i], CMD_GET_LABEL, 
                       i - m_info->channel_offset_output[i], 
                       m_info->names_output + NAME_LENGTH * i);
      }
      db_merge_data(hDB, m_info->hKeyRoot, "Settings/Names Output",
                    m_info->names_output, NAME_LENGTH * m_info->num_channels_output,
                    m_info->num_channels_output, TID_STRING);
   }

   /*---- set labels from midas SC names ----*/
   if (m_info->num_channels_input) {
      for (i = 0; i < m_info->num_channels_input; i++) {
         device_driver(m_info->driver_input[i], CMD_SET_LABEL,
                       i - m_info->channel_offset_input[i],
                       m_info->names_input + NAME_LENGTH * i);
      }

      /* open hotlink on input channel names */
      if (db_find_key(hDB, m_info->hKeyRoot, "Settings/Names Input", &hNamesIn) ==
          DB_SUCCESS)
         db_open_record(hDB, hNamesIn, m_info->names_input,
                        NAME_LENGTH * m_info->num_channels_input, MODE_READ,
                        multi_update_label, pequipment);
   }

   for (i = 0; i < m_info->num_channels_output; i++) {
      device_driver(m_info->driver_output[i], CMD_SET_LABEL,
                    i - m_info->channel_offset_output[i],
                    m_info->names_output + NAME_LENGTH * i);
   }

   /* open hotlink on output channel names */
   if (m_info->num_channels_output) {
      if (db_find_key(hDB, m_info->hKeyRoot, "Settings/Names Output", &hNamesOut) ==
          DB_SUCCESS)
         db_open_record(hDB, hNamesOut, m_info->names_output,
                        NAME_LENGTH * m_info->num_channels_output, MODE_READ,
                        multi_update_label, pequipment);

      /* open hot link to output record */
      db_open_record(hDB, m_info->hKeyOutput, m_info->var_output,
                     m_info->num_channels_output * sizeof(float),
                     MODE_READ, multi_output, pequipment);
   }

   /* set initial demand values */
   for (i = 0; i < m_info->num_channels_output; i++) {
      if (pequipment->driver[index].flags & DF_PRIO_DEVICE) {
         /* read default value directly from device bypassing multi-thread buffer */
         device_driver(m_info->driver_output[i], CMD_GET_DEMAND,
                       i - m_info->channel_offset_output[i],
                       &m_info->output_mirror[i]);
      } else {
         /* use default value from ODB */
         m_info->output_mirror[i] = m_info->var_output[i] * m_info->factor_output[i] -
             m_info->offset_output[i];

         device_driver(m_info->driver_output[i], CMD_SET,
                       i - m_info->channel_offset_output[i],
                       m_info->output_mirror[i]);
      }
   }

   if (m_info->num_channels_output)
      db_set_record(hDB, m_info->hKeyOutput, m_info->output_mirror,
                    m_info->num_channels_output * sizeof(float), 0);

   /* initially read all input channels */
   if (m_info->num_channels_input)
      multi_read(pequipment, -1);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT multi_exit(EQUIPMENT * pequipment)
{
   INT i;

   free_mem((MULTI_INFO *) pequipment->cd_info);

   /* call exit method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL; i++)
      device_driver(&pequipment->driver[i], CMD_EXIT);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT multi_start(EQUIPMENT * pequipment)
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

INT multi_stop(EQUIPMENT * pequipment)
{
   INT i;

   /* call close method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
      device_driver(&pequipment->driver[i], CMD_STOP);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT multi_idle(EQUIPMENT * pequipment)
{
   int i;
   MULTI_INFO *m_info;

   m_info = (MULTI_INFO *) pequipment->cd_info;

   if (m_info->last_channel < m_info->num_channels_input) {
      /* read input channel */
      multi_read(pequipment, m_info->last_channel);
      m_info->last_channel++;
   } else {
      if (!m_info->num_channels_output) {
         m_info->last_channel = 0;
      } else {
         /* search output channel with DF_PRIO_DEV */
         for (i = m_info->last_channel - m_info->num_channels_input;
              i < m_info->num_channels_output; i++)
            if (m_info->driver_output[i]->flags & DF_PRIO_DEVICE)
               break;

         if (i < m_info->num_channels_output) {
            /* read output channel */
            multi_read_output(pequipment, i);

            m_info->last_channel = i + m_info->num_channels_input;
            if (m_info->last_channel <
                m_info->num_channels_input + m_info->num_channels_output - 1)
               m_info->last_channel++;
            else
               m_info->last_channel = 0;
         } else
            m_info->last_channel = 0;
      }
   }


   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT cd_multi_read(char *pevent, int offset)
{
   float *pdata;
   MULTI_INFO *m_info;
   EQUIPMENT *pequipment;
#ifdef HAVE_YBOS
   DWORD *pdw;
#endif

   pequipment = *((EQUIPMENT **) pevent);
   m_info = (MULTI_INFO *) pequipment->cd_info;

   if (m_info->format == FORMAT_FIXED) {
      memcpy(pevent, m_info->var_input, sizeof(float) * m_info->num_channels_input);
      pevent += sizeof(float) * m_info->num_channels_input;

      memcpy(pevent, m_info->var_output, sizeof(float) * m_info->num_channels_output);
      pevent += sizeof(float) * m_info->num_channels_output;

      return sizeof(float) * (m_info->num_channels_input + m_info->num_channels_output);
   } else if (m_info->format == FORMAT_MIDAS) {
      bk_init(pevent);

      /* create INPT bank */
      if (m_info->num_channels_input) {
         bk_create(pevent, "INPT", TID_FLOAT, &pdata);
         memcpy(pdata, m_info->var_input, sizeof(float) * m_info->num_channels_input);
         pdata += m_info->num_channels_input;
         bk_close(pevent, pdata);
      }

      /* create OUTP bank */
      if (m_info->num_channels_output) {
         bk_create(pevent, "OUTP", TID_FLOAT, &pdata);
         memcpy(pdata, m_info->var_output, sizeof(float) * m_info->num_channels_output);
         pdata += m_info->num_channels_output;
         bk_close(pevent, pdata);
      }

      return bk_size(pevent);
   } else if (m_info->format == FORMAT_YBOS) {
#ifdef HAVE_YBOS
      ybk_init((DWORD *) pevent);

      /* create EVID bank */
      ybk_create((DWORD *) pevent, "EVID", I4_BKTYPE, (DWORD *) (&pdw));
      *(pdw)++ = EVENT_ID(pevent);      /* Event_ID + Mask */
      *(pdw)++ = SERIAL_NUMBER(pevent); /* Serial number */
      *(pdw)++ = TIME_STAMP(pevent);    /* Time Stamp */
      ybk_close((DWORD *) pevent, pdw);

      /* create INPT bank */
      ybk_create((DWORD *) pevent, "INPT", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, m_info->var_input, sizeof(float) * m_info->num_channels_input);
      pdata += m_info->num_channels_input;
      ybk_close((DWORD *) pevent, pdata);

      /* create OUTP bank */
      ybk_create((DWORD *) pevent, "OUTP", F4_BKTYPE, (DWORD *) & pdata);
      memcpy(pdata, m_info->var_output, sizeof(float) * m_info->num_channels_output);
      pdata += m_info->num_channels_output;
      ybk_close((DWORD *) pevent, pdata);

      return ybk_size((DWORD *) pevent);
#lese
      assert(!"YBOS support not compiled in");
#endif
   }

   return 0;
}

/*----------------------------------------------------------------------------*/

INT cd_multi(INT cmd, EQUIPMENT * pequipment)
{
   INT status;

   switch (cmd) {
   case CMD_INIT:
      status = multi_init(pequipment);
      break;

   case CMD_EXIT:
      status = multi_exit(pequipment);
      break;

   case CMD_START:
      status = multi_start(pequipment);
      break;

   case CMD_STOP:
      status = multi_stop(pequipment);
      break;

   case CMD_IDLE:
      status = multi_idle(pequipment);
      break;

   default:
      cm_msg(MERROR, "Multimeter class driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   return status;
}

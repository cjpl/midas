/********************************************************************\

  Name:         cd_asum.c
  Created by:   Brian Lee

  Based on the Generic class
  Contents:     T2K-ASUM Class Driver
  Handles the Fine Grid Detector frontend card
  Used for midas/mscb/embedded/Experiements/asum/

  Set HV to the SiPm through a Q pump
  Get Overall current for the SiPm
  Get temperature of the uP (T1)
  Get temperature of the I2C local sensor (T2)
  Get temperature of the I2C remote sensor (T3)

  Requires CMD_GET_EXTERNALTEMP/CMD_GET_INTERNALTEMP in the midas.h
  uses the mscbasum.c and standard mscb suite.
  (mscb, mscbrpc, musbstd)

  $Id$

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "midas.h"
#include "ybos.h"

#undef CMD_GET_LAST
#define CMD_GET_EXTERNALTEMP         CMD_GET_FIRST+6
#define CMD_GET_INTERNALTEMP         CMD_GET_FIRST+7
#define CMD_GET_ADCMEAS              CMD_GET_FIRST+8
#define CMD_GET_CONTROL              CMD_GET_FIRST+9
#define CMD_GET_BIASEN               CMD_GET_FIRST+10
#define CMD_GET_CHPUMPDAC        CMD_GET_FIRST+11
#define CMD_GET_ASUMDACTH        CMD_GET_FIRST+12
#define CMD_GET_VBIAS				CMD_GET_FIRST+13
#define CMD_GET_LAST             CMD_GET_FIRST+13


#undef CMD_SET_LAST
#define CMD_SET_VBIAS				CMD_SET_FIRST+5
#define CMD_SET_CONTROL              CMD_SET_FIRST+6
#define CMD_SET_ASUMDACTH            CMD_SET_FIRST+7
#define CMD_SET_CHPUMPDAC            CMD_SET_FIRST+8
#define CMD_SET_BIAS_EN          CMD_SET_FIRST+9
#define CMD_SET_LAST             CMD_SET_FIRST+10

int indexNum = 0;
int indexNumADC = 0;
int controlFlag = 0;
int biasEnFlag = 0;

typedef struct {

   /* ODB keys */
   HNDLE hKeyRoot, hKeyvBias[8], hKeyMeasured[9], hKeyIntTemp, hKeyExtTemp, hKeyControl[8], hKeyasumDacTh, hKeybiasEn[8], hKeyChPumpDac;

   /* globals */
   INT num_channels;
   INT format;
   INT last_channel;

   /* items in /Variables record */
   float *vBias[8];
   float *measured[9];
   float *internalTemp;
   float *externalTemp;
   float *control[8];
  float *biasEn[8];
  float *asumDacTh;
  float *ChPumpDac;

   /* items in /Settings */
   char *names;
   float *update_threshold;

   /* mirror arrays */
   float *vBias_mirror[8];
   float *measured_mirror[9];
   float *control_mirror[8];
  float *biasEn_mirror[8];
  float *asumDacTh_mirror;
  float *ChPumpDac_mirror;

   DEVICE_DRIVER **driver;
   INT *channel_offset;

} FGD_INFO;

#ifndef abs
#define abs(a) (((a) < 0)   ? -(a) : (a))
#endif

/*------------------------------------------------------------------*/

static void free_mem(FGD_INFO * asum_info)
{
   int i = 0;

   free(asum_info->names);
   for(i = 0; i < 8; i++) free(asum_info->vBias[i]);
   for(i = 0; i < 9; i++) free(asum_info->measured[i]);
   free(asum_info->internalTemp);
   free(asum_info->externalTemp);
   for(i = 0; i < 8; i++) free(asum_info->control[i]);
  for(i = 0; i < 8; i++) free(asum_info->biasEn[i]);
  free(asum_info->asumDacTh);
  free(asum_info->ChPumpDac);

   free(asum_info->update_threshold);

   for(i = 0; i < 8; i++) free(asum_info->vBias_mirror[i]);
   for(i = 0; i < 9; i++) free(asum_info->measured_mirror[i]);
   for(i = 0; i < 8; i++) free(asum_info->control_mirror[i]);
  for(i = 0; i < 8; i++) free(asum_info->biasEn_mirror[i]);
  free(asum_info->asumDacTh_mirror);
  free(asum_info->ChPumpDac_mirror);

   free(asum_info->channel_offset);
   free(asum_info->driver);

   free(asum_info);
}

/*------------------------------------------------------------------*/
// Read current
INT asum_read(EQUIPMENT * pequipment, int channel)
{
  INT numChannel = 0;
   static DWORD last_time = 0;
   int status, i;
   FGD_INFO *asum_info;
   HNDLE hDB;

   /*---- read measured value ----*/

   asum_info = (FGD_INFO *) pequipment->cd_info;
   cm_get_experiment_database(&hDB, NULL);

  for (numChannel = 0; numChannel < 9; numChannel++)
   {
    indexNumADC = numChannel + 13;
    status = device_driver(asum_info->driver[channel], CMD_GET_ADCMEAS,
                  channel - asum_info->channel_offset[channel],
                  asum_info->measured[numChannel]);

    /* check for update measured */
    for (i = 0; i < asum_info->num_channels; i++)
    {
      /* update if change is more than update_threshold */
      //if (abs(*(asum_info->measured[numChannel]) - *(asum_info->measured_mirror[numChannel])) > asum_info->update_threshold[i])
      //{
        for (i = 0; i < asum_info->num_channels; i++)
        {
          *(asum_info->measured_mirror[numChannel]) = *(asum_info->measured[numChannel]);
        }

        db_set_data(hDB, asum_info->hKeyMeasured[numChannel], asum_info->measured[numChannel],
                sizeof(float) * asum_info->num_channels, asum_info->num_channels,
                TID_FLOAT);

        pequipment->odb_out++;

        break;
      //}
    }
  }

	// Get vBias values
	for (numChannel = 0; numChannel < 8; numChannel++)
	{
		indexNum = 5 + numChannel;
		status = device_driver(asum_info->driver[channel], CMD_GET_VBIAS,
							channel - asum_info->channel_offset[channel],
							asum_info->vBias[numChannel]);
		db_set_data(hDB, asum_info->hKeyvBias[numChannel], asum_info->vBias[numChannel],
                sizeof(float) * asum_info->num_channels, asum_info->num_channels,
                TID_FLOAT);
		*(asum_info->vBias_mirror[numChannel]) = *(asum_info->vBias[numChannel]);
		pequipment->odb_out++;
	}

   // Get the temperatures
   if ((ss_time() - last_time) > 1) {
     channel = 0;
     status = device_driver(asum_info->driver[channel], CMD_GET_EXTERNALTEMP,
       channel - asum_info->channel_offset[channel],
       &asum_info->internalTemp[channel]);
     db_set_data(hDB, asum_info->hKeyIntTemp, asum_info->internalTemp,
       sizeof(float) * asum_info->num_channels, asum_info->num_channels,
       TID_FLOAT);
     status = device_driver(asum_info->driver[channel], CMD_GET_INTERNALTEMP,
       channel - asum_info->channel_offset[channel],
       &asum_info->externalTemp[channel]);
     db_set_data(hDB, asum_info->hKeyExtTemp, asum_info->externalTemp,
       sizeof(float) * asum_info->num_channels, asum_info->num_channels,
       TID_FLOAT);
     last_time = ss_time();
   }
   return status;
}

/*------------------------------------------------------------------*/

void asum_vBias(INT hDB, INT hKey, void *info)
{
   INT numChannel = 0;
   INT i, status;
   FGD_INFO *asum_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   asum_info = (FGD_INFO *) pequipment->cd_info;

   /* set individual channels only if vBias value differs */
   for (numChannel = 0; numChannel < 8; numChannel++)
   {
      for (i = 0; i < asum_info->num_channels; i++)
      {
         if (*(asum_info->vBias[numChannel]) != *(asum_info->vBias_mirror[numChannel]))
         {
            if ((asum_info->driver[i]->flags & DF_READ_ONLY) == 0)
            {
          indexNum = numChannel + 5;
			 if(*(asum_info->vBias[numChannel]) < ((*(asum_info->measured[3])) - 4.962))
          {
            printf("Demanded Bias Voltage should be less or more than the current charge pump voltage minus 4.962\n");
				printf("Setting Demanded Bias Voltage to the charge Pump Value\n");
            *(asum_info->vBias[numChannel]) = (float) (*(asum_info->measured[3]));
          }
               status = device_driver(asum_info->driver[i], CMD_SET_VBIAS,  // Voltage
                                    i - asum_info->channel_offset[i], *(asum_info->vBias[numChannel]));
            }
            *(asum_info->vBias_mirror[numChannel]) = *(asum_info->vBias[numChannel]);
         }
      }
   }
   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void asum_asumDacTh(INT hDB, INT hKey, void *info)
{
  INT i, status;
   FGD_INFO *fgd_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   fgd_info = (FGD_INFO *) pequipment->cd_info;

   /* set individual channels only if demand value differs */
   for (i = 0; i < fgd_info->num_channels; i++)
     if (*fgd_info->asumDacTh != *fgd_info->asumDacTh_mirror) {
         if ((fgd_info->driver[i]->flags & DF_READ_ONLY) == 0) {
            status = device_driver(fgd_info->driver[i], CMD_SET_ASUMDACTH,  // DAC Asum Threshold
                                   i - fgd_info->channel_offset[i], *fgd_info->asumDacTh);
         }
         *fgd_info->asumDacTh_mirror = *fgd_info->asumDacTh;
      }

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void asum_ChPumpDac(INT hDB, INT hKey, void *info)
{
  INT i, status;
   FGD_INFO *fgd_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   fgd_info = (FGD_INFO *) pequipment->cd_info;

   /* set individual channels only if demand value differs */
   for (i = 0; i < fgd_info->num_channels; i++)
     if (*fgd_info->ChPumpDac != *fgd_info->ChPumpDac_mirror) {
         if ((fgd_info->driver[i]->flags & DF_READ_ONLY) == 0) {
            status = device_driver(fgd_info->driver[i], CMD_SET_CHPUMPDAC,  // Charge Pump Dac
                                   i - fgd_info->channel_offset[i], *fgd_info->ChPumpDac);
         }
         *fgd_info->ChPumpDac_mirror = *fgd_info->ChPumpDac;
      }

   pequipment->odb_in++;
}

/*------------------------------------------------------------------*/


void control_update(INT hDB, INT hKey, void *info)
{
   INT numChannel = 0;
   INT i, status;
   FGD_INFO *asum_info;
   EQUIPMENT *pequipment;
  float controlToBeSent = 0.0;

   pequipment = (EQUIPMENT *) info;
   asum_info = (FGD_INFO *) pequipment->cd_info;

   /* set individual channels only if vBias value differs */
   for (numChannel = 0; numChannel < 8; numChannel++)
   {
      for (i = 0; i < asum_info->num_channels; i++)
      {
         if (*(asum_info->control[numChannel]) != *(asum_info->control_mirror[numChannel]))
         {
            if ((asum_info->driver[i]->flags & DF_READ_ONLY) == 0)
            {
          if((*(asum_info->control[numChannel]) != 1) && (*(asum_info->control[numChannel]) != 0))
          {
            printf("Input only 1 or 0, setting to 0 \n");
            *(asum_info->control[numChannel]) = (float) 0;
          }
          //move the received control bit to the correct position
          if(*(asum_info->control[numChannel]) == 1) controlFlag = 1;
          else controlFlag = 0;

          controlToBeSent = (float) (1.0 * pow(2, numChannel));

               status = device_driver(asum_info->driver[i], CMD_SET_CONTROL,  // Control
                                    i - asum_info->channel_offset[i], controlToBeSent);
            }
        *(asum_info->control_mirror[numChannel]) = *(asum_info->control[numChannel]);
         }
      }
   }
   pequipment->odb_in++;
}


/*------------------------------------------------------------------*/


void biasEn_update(INT hDB, INT hKey, void *info)
{
   INT numChannel = 0;
   INT i, status;
   FGD_INFO *asum_info;
   EQUIPMENT *pequipment;
  float biasEnToBeSent = 0.0;

   pequipment = (EQUIPMENT *) info;
   asum_info = (FGD_INFO *) pequipment->cd_info;

   /* set individual channels only if vBias value differs */
   for (numChannel = 0; numChannel < 8; numChannel++)
   {
      for (i = 0; i < asum_info->num_channels; i++)
      {
         if (*(asum_info->biasEn[numChannel]) != *(asum_info->biasEn_mirror[numChannel]))
         {
            if ((asum_info->driver[i]->flags & DF_READ_ONLY) == 0)
            {
          if((*(asum_info->biasEn[numChannel]) != 1) && (*(asum_info->biasEn[numChannel]) != 0))
          {
            printf("Input only 1 or 0, setting to 0 \n");
            *(asum_info->biasEn[numChannel]) = (float) 0;
          }

          if(*(asum_info->biasEn[numChannel]) == 0) biasEnFlag = 1;
          else biasEnFlag = 0;

          biasEnToBeSent = (float) (1.0 * pow(2, numChannel));

               status = device_driver(asum_info->driver[i], CMD_SET_BIAS_EN,  // biasEn
                                    i - asum_info->channel_offset[i], biasEnToBeSent);
            }
        *(asum_info->biasEn_mirror[numChannel]) = *(asum_info->biasEn[numChannel]);
         }
      }
   }
   pequipment->odb_in++;
}


/*------------------------------------------------------------------*/

void asum_update_label(INT hDB, INT hKey, void *info)
{
   INT i, status;
   FGD_INFO *asum_info;
   EQUIPMENT *pequipment;

   pequipment = (EQUIPMENT *) info;
   asum_info = (FGD_INFO *) pequipment->cd_info;

   /* update channel labels based on the midas channel names */
   for (i = 0; i < asum_info->num_channels; i++)
      status = device_driver(asum_info->driver[i], CMD_SET_LABEL,
                             i - asum_info->channel_offset[i],
                             asum_info->names + NAME_LENGTH * i);
}

/*------------------------------------------------------------------*/

INT asum_init(EQUIPMENT * pequipment)
{
   int status, size, i, j, index, offset;
   char str[256];
   HNDLE hDB, hKey, hNames, hThreshold;
   FGD_INFO *asum_info;
  char vBiasString[32];
  char measString[32];
  char ctrlString[32];
  char biasEnString[32];
  unsigned char *controlRead;
  unsigned char *biasEnRead;

   /* allocate private data */

  pequipment->cd_info = calloc(1, sizeof(FGD_INFO));
   asum_info = (FGD_INFO *) pequipment->cd_info;

   /* get class driver root key */
   cm_get_experiment_database(&hDB, NULL);
   sprintf(str, "/Equipment/%s", pequipment->name);
   db_create_key(hDB, 0, str, TID_KEY);
   db_find_key(hDB, 0, str, &asum_info->hKeyRoot);

   /* save event format */
   size = sizeof(str);
   db_get_value(hDB, asum_info->hKeyRoot, "Common/Format", str, &size, TID_STRING, TRUE);

   if (equal_ustring(str, "Fixed"))
      asum_info->format = FORMAT_FIXED;
   else if (equal_ustring(str, "MIDAS"))
      asum_info->format = FORMAT_MIDAS;
   else if (equal_ustring(str, "YBOS"))
      asum_info->format = FORMAT_YBOS;

   /* count total number of channels */
   for (i = 0, asum_info->num_channels = 0; pequipment->driver[i].name[0]; i++) {
      if (pequipment->driver[i].channels == 0) {
         cm_msg(MERROR, "asum_init", "Driver with zero channels not allowed");
         return FE_ERR_ODB;
      }

      asum_info->num_channels += pequipment->driver[i].channels;
   }

   if (asum_info->num_channels == 0) {
      cm_msg(MERROR, "asum_init", "No channels found in device driver list");
      return FE_ERR_ODB;
   }

   /* Allocate memory for buffers */
   asum_info->names = (char *) calloc(asum_info->num_channels, NAME_LENGTH);

   for(i = 0; i < 8; i++) asum_info->vBias[i] = (float *) calloc(asum_info->num_channels, sizeof(float));
   for(i = 0; i < 9; i++) asum_info->measured[i] = (float *) calloc(asum_info->num_channels, sizeof(float));

   asum_info->internalTemp = (float *) calloc(asum_info->num_channels, sizeof(float));
   asum_info->externalTemp = (float *) calloc(asum_info->num_channels, sizeof(float));
   for(i = 0; i < 8; i++) asum_info->control[i] = (float *) calloc(asum_info->num_channels, sizeof(float));
  for(i = 0; i < 8; i++) asum_info->biasEn[i] = (float *) calloc(asum_info->num_channels, sizeof(float));
  asum_info->asumDacTh = (float *) calloc(asum_info->num_channels, sizeof(float));
  asum_info->ChPumpDac = (float *) calloc(asum_info->num_channels, sizeof(float));

   asum_info->update_threshold = (float *) calloc(asum_info->num_channels, sizeof(float));

   for(i = 0; i < 8; i++) asum_info->vBias_mirror[i] = (float *) calloc(asum_info->num_channels, sizeof(float));
   for(i = 0; i < 9; i++) asum_info->measured_mirror[i] = (float *) calloc(asum_info->num_channels, sizeof(float));
   for(i = 0; i < 8; i++) asum_info->control_mirror[i] = (float *) calloc(asum_info->num_channels, sizeof(float));
  for(i = 0; i < 8; i++) asum_info->biasEn_mirror[i] = (float *) calloc(asum_info->num_channels, sizeof(float));
  asum_info->asumDacTh_mirror = (float *) calloc(asum_info->num_channels, sizeof(float));
  asum_info->ChPumpDac_mirror = (float *) calloc(asum_info->num_channels, sizeof(float));

   asum_info->channel_offset = (INT *) calloc(asum_info->num_channels, sizeof(INT));
   asum_info->driver = (void *) calloc(asum_info->num_channels, sizeof(void *));

   if (!asum_info->driver) {
      cm_msg(MERROR, "hv_init", "Not enough memory");
      return FE_ERR_ODB;
   }

   /*---- Initialize device drivers ----*/

   /* call init method */
   for (i = 0; pequipment->driver[i].name[0]; i++) {
      sprintf(str, "Settings/Devices/%s", pequipment->driver[i].name);
      status = db_find_key(hDB, asum_info->hKeyRoot, str, &hKey);
      if (status != DB_SUCCESS) {
         db_create_key(hDB, asum_info->hKeyRoot, str, TID_KEY);
         status = db_find_key(hDB, asum_info->hKeyRoot, str, &hKey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "hv_init", "Cannot create %s entry in online database", str);
            free_mem(asum_info);
            return FE_ERR_ODB;
         }
      }

      status = device_driver(&pequipment->driver[i], CMD_INIT, hKey);
      if (status != FE_SUCCESS) {
         free_mem(asum_info);
         return status;
      }
   }

   /* compose device driver channel assignment */
   for (i = 0, j = 0, index = 0, offset = 0; i < asum_info->num_channels; i++, j++) {
      while (j >= pequipment->driver[index].channels && pequipment->driver[index].name[0]) {
         offset += j;
         index++;
         j = 0;
      }

      asum_info->driver[i] = &pequipment->driver[index];
      asum_info->channel_offset[i] = offset;
   }

   /*---- create vBias variables ----*/
   /* get vBias from ODB */
  for(j = 0; j < 8; j++)
   {
    /* Assign vBias names */
    if(j == 0) strcpy(vBiasString, "Variables/vBias1");
    if(j == 1) strcpy(vBiasString, "Variables/vBias2");
    if(j == 2) strcpy(vBiasString, "Variables/vBias3");
    if(j == 3) strcpy(vBiasString, "Variables/vBias4");
    if(j == 4) strcpy(vBiasString, "Variables/vBias5");
    if(j == 5) strcpy(vBiasString, "Variables/vBias6");
    if(j == 6) strcpy(vBiasString, "Variables/vBias7");
    if(j == 7) strcpy(vBiasString, "Variables/vBias8");

    /* find the key and get data */
    status = db_find_key(hDB, asum_info->hKeyRoot, vBiasString, &asum_info->hKeyvBias[j]);
    if (status == DB_SUCCESS) {
      size = sizeof(float) * asum_info->num_channels;
      db_get_data(hDB, asum_info->hKeyvBias[j], asum_info->vBias[j], &size, TID_FLOAT);
    }

    /* write back vBias values */
    status = db_find_key(hDB, asum_info->hKeyRoot, vBiasString, &asum_info->hKeyvBias[j]);
    if (status != DB_SUCCESS) {
      db_create_key(hDB, asum_info->hKeyRoot, vBiasString, TID_FLOAT);
      db_find_key(hDB, asum_info->hKeyRoot, vBiasString, &asum_info->hKeyvBias[j]);
    }

	 size = sizeof(float) * asum_info->num_channels;

    db_set_data(hDB, asum_info->hKeyvBias[j], asum_info->vBias[j], size,
              asum_info->num_channels, TID_FLOAT);
    db_open_record(hDB, asum_info->hKeyvBias[j], asum_info->vBias[j],
                asum_info->num_channels * sizeof(float), MODE_READ, asum_vBias,
                pequipment);
   }

  /*---- create asumDacTh variables ----*/
  /* find the key and get data */

  status = db_find_key(hDB, asum_info->hKeyRoot, "Variables/asumDacTh", &asum_info->hKeyasumDacTh);
  if (status == DB_SUCCESS) {
    size = sizeof(float) * asum_info->num_channels;
    db_get_data(hDB, asum_info->hKeyasumDacTh, asum_info->asumDacTh, &size, TID_FLOAT);
  }
  else /* write back asumDacTh values */
  {
    db_create_key(hDB, asum_info->hKeyRoot, "Variables/asumDacTh", TID_FLOAT);
    db_find_key(hDB, asum_info->hKeyRoot, "Variables/asumDacTh", &asum_info->hKeyasumDacTh);
  }
  //Update asumDacTh value with the latest mscb value
  device_driver(asum_info->driver[0], CMD_GET_ASUMDACTH, asum_info->channel_offset[0], asum_info->asumDacTh);

  size = sizeof(float) * asum_info->num_channels;

  db_set_data(hDB, asum_info->hKeyasumDacTh, asum_info->asumDacTh, size,
            asum_info->num_channels, TID_FLOAT);
  db_open_record(hDB, asum_info->hKeyasumDacTh, asum_info->asumDacTh,
              asum_info->num_channels * sizeof(float), MODE_READ, asum_asumDacTh,
              pequipment);


  /*---- create ChPumpDac variables ----*/
  /* find the key and get data */
  status = db_find_key(hDB, asum_info->hKeyRoot, "Variables/ChPumpDac", &asum_info->hKeyChPumpDac);
  if (status == DB_SUCCESS) {
    size = sizeof(float) * asum_info->num_channels;
    db_get_data(hDB, asum_info->hKeyChPumpDac, asum_info->ChPumpDac, &size, TID_FLOAT);
  }
  else /* write back ChPumpDac values */
  {
    db_create_key(hDB, asum_info->hKeyRoot, "Variables/ChPumpDac", TID_FLOAT);
    db_find_key(hDB, asum_info->hKeyRoot, "Variables/ChPumpDac", &asum_info->hKeyChPumpDac);
  }
  //Update with the latest mscb value
  device_driver(asum_info->driver[0], CMD_GET_CHPUMPDAC, asum_info->channel_offset[0], asum_info->ChPumpDac);
  size = sizeof(float) * asum_info->num_channels;

  db_set_data(hDB, asum_info->hKeyChPumpDac, asum_info->ChPumpDac, size,
            asum_info->num_channels, TID_FLOAT);
  db_open_record(hDB, asum_info->hKeyChPumpDac, asum_info->ChPumpDac,
              asum_info->num_channels * sizeof(float), MODE_READ, asum_ChPumpDac,
              pequipment);

   /*---- create measured variables ----*/
  for(j = 0; j < 9; j++)
   {
    /* Assign adc measurement channel names */
    if(j == 0) strcpy(measString, "Variables/PanaImon");
    if(j == 1) strcpy(measString, "Variables/PanaVmon");
    if(j == 2) strcpy(measString, "Variables/digImon");
    if(j == 3) strcpy(measString, "Variables/biasRB");
    if(j == 4) strcpy(measString, "Variables/digVmon");
    if(j == 5) strcpy(measString, "Variables/NanaVmon");
    if(j == 6) strcpy(measString, "Variables/NanaImon");
    if(j == 7) strcpy(measString, "Variables/RefCurrent");
    if(j == 8) strcpy(measString, "Variables/BiasCurrent");

    db_merge_data(hDB, asum_info->hKeyRoot, measString,
            asum_info->measured[j], sizeof(float) * asum_info->num_channels,
            asum_info->num_channels, TID_FLOAT);
    status = db_find_key(hDB, asum_info->hKeyRoot, measString, &asum_info->hKeyMeasured[j]);
    /*if (status != DB_SUCCESS) {
      db_create_key(hDB, asum_info->hKeyRoot, ctrlString, TID_FLOAT);
      db_find_key(hDB, asum_info->hKeyRoot, ctrlString, &asum_info->hKeyMeasured[j]);
    }*/
    memcpy(asum_info->measured_mirror[j], asum_info->measured[j],
        asum_info->num_channels * sizeof(float));
  }

   /*---- create ExtTemp measured variables ----*/
   db_merge_data(hDB, asum_info->hKeyRoot, "Variables/ExtTemp",
                 asum_info->internalTemp, sizeof(float) * asum_info->num_channels,
                 asum_info->num_channels, TID_FLOAT);
   db_find_key(hDB, asum_info->hKeyRoot, "Variables/ExtTemp", &asum_info->hKeyIntTemp);

   /*---- create IntTemp measured variables ----*/
   db_merge_data(hDB, asum_info->hKeyRoot, "Variables/IntTemp",
                 asum_info->externalTemp, sizeof(float) * asum_info->num_channels,
                 asum_info->num_channels, TID_FLOAT);
   db_find_key(hDB, asum_info->hKeyRoot, "Variables/IntTemp", &asum_info->hKeyExtTemp);



   /*---- create control variables ----*/
  //initially read in the current control values
   controlRead = calloc(1, sizeof(unsigned char *));
  device_driver(asum_info->driver[0], CMD_GET_CONTROL, asum_info->channel_offset[0], controlRead); // Control

  for(j = 0; j < 8; j++)
   {
    /* Assign control names */
    if(j == 0) strcpy(ctrlString, "Variables/Charge Pump");
    if(j == 1) strcpy(ctrlString, "Variables/Temp Meas");
    if(j == 2) strcpy(ctrlString, "Variables/ADC Meas");
    if(j == 3) strcpy(ctrlString, "Variables/Bias Enable");
    if(j == 4) strcpy(ctrlString, "Variables/Bias DAC");
    if(j == 5) strcpy(ctrlString, "Variables/Asum Threshold");
    if(j == 6) strcpy(ctrlString, "Variables/I2C DAC");
    if(j == 7) strcpy(ctrlString, "Variables/KeepRefAll");

    status = db_find_key(hDB, asum_info->hKeyRoot, ctrlString, &asum_info->hKeyControl[j]);
    if (status != DB_SUCCESS) {
      db_create_key(hDB, asum_info->hKeyRoot, ctrlString, TID_FLOAT);
      db_find_key(hDB, asum_info->hKeyRoot, ctrlString, &asum_info->hKeyControl[j]);
    }

  //distribute the read control values to each element in the control[] array
  if((*controlRead & (unsigned char)pow(2, j))) *(asum_info->control[j]) = 1;
  //update the Variables (Control, 8bits)
  db_set_data(hDB, asum_info->hKeyControl[j], asum_info->control[j],
       sizeof(float) * asum_info->num_channels, asum_info->num_channels,
       TID_FLOAT);

  size = sizeof(float) * asum_info->num_channels;
  db_get_data(hDB, asum_info->hKeyControl[j], asum_info->control[j], &size, TID_FLOAT);
  db_open_record(hDB, asum_info->hKeyControl[j], asum_info->control[j],
            asum_info->num_channels * sizeof(float), MODE_READ, control_update,
            pequipment);

  //copy all control values into the control mirror
  *(asum_info->control_mirror[j]) = *(asum_info->control[j]);
  }

  /*---- create biasEn variables ----*/
  //initially read in the current biasEn values
   biasEnRead = calloc(1, sizeof(unsigned char *));
  device_driver(asum_info->driver[0], CMD_GET_BIASEN, asum_info->channel_offset[0], biasEnRead);

  for(j = 0; j < 8; j++)
   {
    /* Assign biasEn names */
    if(j == 0) strcpy(biasEnString, "Variables/BiasEnable1");
    if(j == 1) strcpy(biasEnString, "Variables/BiasEnable2");
    if(j == 2) strcpy(biasEnString, "Variables/BiasEnable3");
    if(j == 3) strcpy(biasEnString, "Variables/BiasEnable4");
    if(j == 4) strcpy(biasEnString, "Variables/BiasEnable5");
    if(j == 5) strcpy(biasEnString, "Variables/BiasEnable6");
    if(j == 6) strcpy(biasEnString, "Variables/BiasEnable7");
    if(j == 7) strcpy(biasEnString, "Variables/BiasEnable8");

    status = db_find_key(hDB, asum_info->hKeyRoot, biasEnString, &asum_info->hKeybiasEn[j]);
    if (status != DB_SUCCESS) {
      db_create_key(hDB, asum_info->hKeyRoot, biasEnString, TID_FLOAT);
      db_find_key(hDB, asum_info->hKeyRoot, biasEnString, &asum_info->hKeybiasEn[j]);
    }

  //distribute the read biasEn values to each element in the biasEn[] array
  if((*biasEnRead & (unsigned char)pow(2, j))) *(asum_info->biasEn[j]) = 0; //0 means OFF in MIDAS interface (on the circuit, 1 means OFF)
  else *(asum_info->biasEn[j]) = 1; //1 means ON in MIDAS interface (on the circuit, 0 means ON)

  //update the Variables (Control, 8bits)
  db_set_data(hDB, asum_info->hKeybiasEn[j], asum_info->biasEn[j],
       sizeof(float) * asum_info->num_channels, asum_info->num_channels,
       TID_FLOAT);

  size = sizeof(float) * asum_info->num_channels;
  db_get_data(hDB, asum_info->hKeybiasEn[j], asum_info->biasEn[j], &size, TID_FLOAT);
  db_open_record(hDB, asum_info->hKeybiasEn[j], asum_info->biasEn[j],
            asum_info->num_channels * sizeof(float), MODE_READ, biasEn_update,
            pequipment);

  //copy all biasEn values into the biasEn mirror
  *(asum_info->biasEn_mirror[j]) = *(asum_info->biasEn[j]);
  }


   /*---- get default names from device driver ----*/
   for (i = 0; i < asum_info->num_channels; i++) {
      sprintf(asum_info->names + NAME_LENGTH * i, "Default%%CH %d", i);
      device_driver(asum_info->driver[i], CMD_GET_LABEL,
                    i - asum_info->channel_offset[i], asum_info->names + NAME_LENGTH * i);
   }
   db_merge_data(hDB, asum_info->hKeyRoot, "Settings/Names",
                 asum_info->names, NAME_LENGTH * asum_info->num_channels,
                 asum_info->num_channels, TID_STRING);

   /*---- set labels form midas SC names ----*/
   for (i = 0; i < asum_info->num_channels; i++) {
      asum_info = (FGD_INFO *) pequipment->cd_info;
      device_driver(asum_info->driver[i], CMD_SET_LABEL,
                    i - asum_info->channel_offset[i], asum_info->names + NAME_LENGTH * i);
   }

   /* open hotlink on channel names */
   if (db_find_key(hDB, asum_info->hKeyRoot, "Settings/Names", &hNames) == DB_SUCCESS)
      db_open_record(hDB, hNames, asum_info->names, NAME_LENGTH*asum_info->num_channels,
                     MODE_READ, asum_update_label, pequipment);

   /*---- get default update threshold from device driver ----*/
   for (i = 0; i < asum_info->num_channels; i++) {
      asum_info->update_threshold[i] = 1.f;      /* default 1 unit */
      device_driver(asum_info->driver[i], CMD_GET_THRESHOLD,
                    i - asum_info->channel_offset[i], &asum_info->update_threshold[i]);
   }
   db_merge_data(hDB, asum_info->hKeyRoot, "Settings/Update Threshold Measured",
                 asum_info->update_threshold, sizeof(float)*asum_info->num_channels,
                 asum_info->num_channels, TID_FLOAT);

   /* open hotlink on update threshold */
   if (db_find_key(hDB, asum_info->hKeyRoot, "Settings/Update Threshold Measured", &hThreshold) == DB_SUCCESS)
     db_open_record(hDB, hThreshold, asum_info->update_threshold, sizeof(float)*asum_info->num_channels,
        MODE_READ, NULL, NULL);

   /*---- set initial vBias values ----*/
   // asum_vBias(hDB, asum_info->hKeyvBias[j], pequipment);

   /* initially read all channels */
   for (i = 0; i < asum_info->num_channels; i++)
      asum_read(pequipment, i);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT asum_start(EQUIPMENT * pequipment)
{
   INT i;

   /* call start method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
      device_driver(&pequipment->driver[i], CMD_START);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT asum_stop(EQUIPMENT * pequipment)
{
   INT i;

   /* call stop method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
      device_driver(&pequipment->driver[i], CMD_STOP);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT asum_exit(EQUIPMENT * pequipment)
{
   INT i;

   free_mem((FGD_INFO *) pequipment->cd_info);

   /* call exit method of device drivers */
   for (i = 0; pequipment->driver[i].dd != NULL; i++)
      device_driver(&pequipment->driver[i], CMD_EXIT);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT asum_idle(EQUIPMENT * pequipment)
{
   INT act, status;
   FGD_INFO *asum_info;

   asum_info = (FGD_INFO *) pequipment->cd_info;

   /* select next measurement channel */
   act = (asum_info->last_channel + 1) % asum_info->num_channels;

   /* measure channel */
   status = asum_read(pequipment, act);
   asum_info->last_channel = act;

   return status;
}

/*------------------------------------------------------------------*/

INT cd_asum_read(char *pevent, int offset)
{
   float *pdata;
   FGD_INFO *asum_info;
   EQUIPMENT *pequipment;
  int j = 0;

   pequipment = *((EQUIPMENT **) pevent);
   asum_info = (FGD_INFO *) pequipment->cd_info;

   if (asum_info->format == FORMAT_FIXED) {
    for(j = 0; j < 8; j++)
    {
      memcpy(pevent, asum_info->vBias[j], sizeof(float) * asum_info->num_channels);
      pevent += sizeof(float) * asum_info->num_channels;
    }

      for(j = 0; j < 9; j++)
    {
      memcpy(pevent, asum_info->measured[j], sizeof(float) * asum_info->num_channels);
      pevent += sizeof(float) * asum_info->num_channels;
    }

      return 2 * sizeof(float) * asum_info->num_channels;
   } else if (asum_info->format == FORMAT_MIDAS) {
      bk_init(pevent);

      /* create VDMD bank */
    for(j = 0; j < 8; j++)
    {
      bk_create(pevent, "VDMD", TID_FLOAT, &pdata);
      memcpy(pdata, asum_info->vBias[j], sizeof(float) * asum_info->num_channels);
      pdata += asum_info->num_channels;
      bk_close(pevent, pdata);
    }

      /* create IMES bank */
    for(j = 0; j < 9; j++)
    {
      bk_create(pevent, "IMES", TID_FLOAT, &pdata);
      memcpy(pdata, asum_info->measured[j], sizeof(float) * asum_info->num_channels);
      pdata += asum_info->num_channels;
      bk_close(pevent, pdata);
    }

      /* create TEMP1 bank */
      bk_create(pevent, "TEM1", TID_FLOAT, &pdata);
      memcpy(pdata, asum_info->internalTemp, sizeof(float) * asum_info->num_channels);
      pdata += asum_info->num_channels;
      bk_close(pevent, pdata);

      /* create TEMP2 bank */
      bk_create(pevent, "TEM2", TID_FLOAT, &pdata);
      memcpy(pdata, asum_info->externalTemp, sizeof(float) * asum_info->num_channels);
      pdata += asum_info->num_channels;
      bk_close(pevent, pdata);

      return bk_size(pevent);
   } else if (asum_info->format == FORMAT_YBOS) {
     printf("Not implemented\n");
     return 0;
   } else
     return 0;
}

/*------------------------------------------------------------------*/

INT cd_asum(INT cmd, EQUIPMENT * pequipment)
{
   INT status;

   switch (cmd) {
   case CMD_INIT:
      status = asum_init(pequipment);
      break;

   case CMD_START:
      status = asum_start(pequipment);
      break;

   case CMD_STOP:
      status = asum_stop(pequipment);
      break;

   case CMD_EXIT:
      status = asum_exit(pequipment);
      break;

   case CMD_IDLE:
      status = asum_idle(pequipment);
      break;

   default:
      cm_msg(MERROR, "Generic class driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   return status;
}

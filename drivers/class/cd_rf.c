/********************************************************************\

  Name:         cd_rf.c
  Created by:   Brian Lee

  Based on the Generic class
  Contents:     TITAN RF Generator Class Driver
  Handles the Fine Grid Detector frontend card
  Used for midas/mscb/embedded/Experiements/rf/



  Requires CMD_GET_EXTERNALTEMP/CMD_GET_INTERNALTEMP in the midas.h
  uses the mscbrf.c and standard mscb suite.
  (mscb, mscbrpc, musbstd)

  $Id: cd_rf.c 3764 2007-07-20 23:53:16Z amaudruz $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "midas.h"
#include "ybos.h"

//command definitions
#define CMD_SET_VBIAS							CMD_SET_FIRST+5
#define CMD_SET_CONTROL					  CMD_SET_FIRST+6
#define CMD_SET_ASUMDACTH				  CMD_SET_FIRST+7
#define CMD_SET_CHPUMPDAC				  CMD_SET_FIRST+8
#define CMD_SET_BIAS_EN					  CMD_SET_FIRST+9
#define CMD_SET_PARAMS					  CMD_SET_FIRST+10

#define CMD_GET_INTERNALTEMP			CMD_GET_FIRST+2
#define CMD_GET_ADCMEAS					  CMD_GET_FIRST+3
#define CMD_GET_CONTROL					  CMD_GET_FIRST+4
#define CMD_GET_BIASEN					  CMD_GET_FIRST+5
#define CMD_GET_CHPUMPDAC				  CMD_GET_FIRST+6
#define CMD_GET_ASUMDACTH			    CMD_GET_FIRST+7
#define CMD_GET_VBIAS							CMD_GET_FIRST+8
#define CMD_GET_ACTUALVBIAS				CMD_GET_FIRST+9
#define CMD_GET_PARAMS						CMD_GET_FIRST+10
#define CMD_GET_TEMPERATURE1			CMD_GET_FIRST+11
#define CMD_GET_TEMPERATURE2			CMD_GET_FIRST+12
#define CMD_GET_TEMPERATURE3			CMD_GET_FIRST+13
#define CMD_GET_TEMPERATURE4			CMD_GET_FIRST+14
#define CMD_GET_TEMPERATURE5			CMD_GET_FIRST+15
#define CMD_GET_TEMPERATURE6			CMD_GET_FIRST+16
#define CMD_GET_EXTERNALTEMP			CMD_GET_FIRST+17

//define number of RF generators used in the experiment
#define NUMBER_OF_RFGEN 2

#define REFRESH_MODE 0

typedef struct 
{

	/* ODB keys */
	HNDLE hKeyRoot, hKeyParams[NUMBER_OF_RFGEN][7], hKeyLadderParams[NUMBER_OF_RFGEN][5], hKeyLadderBurstParams[NUMBER_OF_RFGEN][4], hKeyFMParams[NUMBER_OF_RFGEN][3], 
	hKeyBurstParams[NUMBER_OF_RFGEN][3], hKeySweepParams[NUMBER_OF_RFGEN][4], hKeyMeasured[4], hKeyExtTemp, hKeyControl[NUMBER_OF_RFGEN][8], hKeyControlMng[NUMBER_OF_RFGEN];

	/* globals */
	INT num_channels;
	INT format;
	INT last_channel;
	char *channel_select[5];

	/* items in /Variables record */
	float *params[NUMBER_OF_RFGEN][7];
	double *ladderParams[NUMBER_OF_RFGEN][5];
	double *burstParams[NUMBER_OF_RFGEN][3];
	double *sweepParams[NUMBER_OF_RFGEN][4];
	double *fmParams[NUMBER_OF_RFGEN][3];
	double *ladderBurstParams[NUMBER_OF_RFGEN][4];
	float *measured[4];
	double *control[NUMBER_OF_RFGEN][8];
	double *controlMng[NUMBER_OF_RFGEN];
	float *externalTemp;

	/* items in /Settings */
	char *names;
	float *update_threshold;

	/* mirror arrays */
	float *params_mirror[NUMBER_OF_RFGEN][7];
	float *measured_mirror[9];
	double *control_mirror[NUMBER_OF_RFGEN][8];
	double *controlMng_mirror[NUMBER_OF_RFGEN];

	DEVICE_DRIVER **driver;
	INT *channel_offset;

} TITANRF_INFO;

#ifndef abs
#define abs(a) (((a) < 0)   ? -(a) : (a))
#endif

/*------------------------------------------------------------------*/

static void free_mem(TITANRF_INFO * rf_info)
{
	int i = 0;
	INT rfNumber = 0;

	for(i = 0; i < 5; i++) free(rf_info->channel_select[i]);	
	for(i = 0; i < 4; i++) free(rf_info->measured[i]);
	free(rf_info->externalTemp);	
	free(rf_info->update_threshold);

	for(rfNumber = 0; rfNumber < NUMBER_OF_RFGEN; rfNumber++)
	{
		free(rf_info->names);		
		for(i = 0; i < 7; i++) free(rf_info->params[rfNumber][i]);	
		for(i = 0; i < 7; i++) free(rf_info->params_mirror[rfNumber][i]);
		for(i = 0; i < 5; i++) free(rf_info->ladderParams[rfNumber][i]);
		for(i = 0; i < 3; i++) free(rf_info->burstParams[rfNumber][i]);
		for(i = 0; i < 4; i++) free(rf_info->sweepParams[rfNumber][i]);
		for(i = 0; i < 3; i++) free(rf_info->fmParams[rfNumber][i]);
		for(i = 0; i < 4; i++) free(rf_info->ladderBurstParams[rfNumber][i]);		
		for(i = 0; i < 8; i++) free(rf_info->control_mirror[rfNumber][i]);
		free(rf_info->controlMng_mirror[rfNumber]);
		for(i = 0; i < 8; i++) free(rf_info->control[rfNumber][i]);
		free(rf_info->controlMng[rfNumber]);
	}

	free(rf_info->channel_offset);
	free(rf_info->driver);

	free(rf_info);
}

/*------------------------------------------------------------------*/
// Read current
INT rf_read(EQUIPMENT * pequipment, int channel)
{
	INT numChannel = 0, varIndex = 0;
	static DWORD last_time = 0;
	int status;
	TITANRF_INFO *rf_info;
	HNDLE hDB;

	// read measured value
	rf_info = (TITANRF_INFO *) pequipment->cd_info;
	cm_get_experiment_database(&hDB, NULL);

	//for (numChannel = 0; numChannel < 4; numChannel++)
	//{
	//	varIndex = numChannel + 13;
	//	status = device_driver(rf_info->driver[channel], CMD_GET_ADCMEAS,
	//						varIndex, rf_info->measured[numChannel]);

	//	db_set_data(hDB, rf_info->hKeyMeasured[numChannel], rf_info->measured[numChannel],
	//			sizeof(float) * rf_info->num_channels, rf_info->num_channels,
	//			TID_FLOAT);

	//	pequipment->odb_out++;
	//}

	//// Get the temperatures
	//if ((ss_time() - last_time) > 1) 
	//{
	//	channel = 0;
	//	status = device_driver(rf_info->driver[channel], CMD_GET_EXTERNALTEMP,
	//	channel - rf_info->channel_offset[channel],	&rf_info->externalTemp[channel]);
	//	db_set_data(hDB, rf_info->hKeyExtTemp, rf_info->externalTemp,
	//							sizeof(float) * rf_info->num_channels, rf_info->num_channels,
	//							TID_FLOAT);
	//	last_time = ss_time();
	//}

	return status;
}

/*------------------------------------------------------------------*/

void rf_setParams(INT hDB, INT hKey, void *info)
{
	INT numChannel = 0;
	INT i, status, varIndex, rfNumber;
	TITANRF_INFO *rf_info;
	EQUIPMENT *pequipment;

	pequipment = (EQUIPMENT *) info;
	rf_info = (TITANRF_INFO *) pequipment->cd_info;

	/* set individual channels only if params value differs */
	for (rfNumber = 0; rfNumber < NUMBER_OF_RFGEN; rfNumber++)
	{
		for (numChannel = 0; numChannel < 7; numChannel++)
		{
			for (i = 0; i < rf_info->num_channels; i++)
			{
				if (*(rf_info->params[rfNumber][numChannel]) != *(rf_info->params_mirror[rfNumber][numChannel]))
				{
					if ((rf_info->driver[i]->flags & DF_READ_ONLY) == 0)
					{
						varIndex = numChannel + 3;

						status = device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Set Parameters
																	(rfNumber * 100) + varIndex, *(rf_info->params[rfNumber][numChannel]));
					}
					*(rf_info->params_mirror[rfNumber][numChannel]) = *(rf_info->params[rfNumber][numChannel]);
				}			 
			}
		}
	}

	pequipment->odb_in++;
}

/*------------------------------------------------------------------*/

void rf_setControl(INT hDB, INT hKey, void *info)
{
	INT numChannel = 0;
	INT status, i, rfNumber, varIndex;
	TITANRF_INFO *rf_info;
	EQUIPMENT *pequipment;
	double controlToBeSent = 0.0;
	unsigned char toBeSent2 = 0;
	unsigned char readValue = 0;
	unsigned char buffer = 0;

	pequipment = (EQUIPMENT *) info;
	rf_info = (TITANRF_INFO *) pequipment->cd_info;
	i = 0;

	for(rfNumber = 0; rfNumber < NUMBER_OF_RFGEN; rfNumber++)
	{
		if((*(rf_info->controlMng[rfNumber]) != *(rf_info->controlMng_mirror[rfNumber])) || (*(rf_info->controlMng[rfNumber]) == REFRESH_MODE))
		{
			if((*(rf_info->controlMng[rfNumber]) == REFRESH_MODE) && (*(rf_info->controlMng[rfNumber]) != *(rf_info->controlMng_mirror[rfNumber])))
			{
					*(rf_info->controlMng[rfNumber]) = *(rf_info->controlMng_mirror[rfNumber]);
					db_set_data(hDB, rf_info->hKeyControlMng[rfNumber], rf_info->controlMng[rfNumber],
											sizeof(double) * rf_info->num_channels, rf_info->num_channels,
											TID_DOUBLE);
			}

			toBeSent2 = (unsigned char) *(rf_info->controlMng[rfNumber]);

			if(toBeSent2 & 0x01)
			{
				varIndex = 3;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  
														(rfNumber * 100) + varIndex, *(rf_info->ladderParams[rfNumber][0]));
				varIndex = 4;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  
														(rfNumber * 100) + varIndex, *(rf_info->ladderParams[rfNumber][1]));
				varIndex = 5;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  
														(rfNumber * 100) + varIndex, *(rf_info->ladderParams[rfNumber][2]));
				varIndex = 6;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  
														(rfNumber * 100) + varIndex, *(rf_info->ladderParams[rfNumber][3]));
				varIndex = 7;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  
														(rfNumber * 100) + varIndex, *(rf_info->ladderParams[rfNumber][4]));
			}
			else if(toBeSent2 & 0x02)
			{
				varIndex = 3;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->sweepParams[rfNumber][0]));
				varIndex = 5;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->sweepParams[rfNumber][1]));
				varIndex = 6;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->sweepParams[rfNumber][2]));
				varIndex = 7;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->sweepParams[rfNumber][3]));								
			}
			else if(toBeSent2 & 0x04)
			{
				varIndex = 7;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->burstParams[rfNumber][0]));
				varIndex = 8;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->burstParams[rfNumber][1]));
				varIndex = 9;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->burstParams[rfNumber][2]));
			}
			else if(toBeSent2 & 0x08)
			{
				varIndex = 4;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->ladderBurstParams[rfNumber][0]));
				varIndex = 7;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->ladderBurstParams[rfNumber][1]));
				varIndex = 8;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->ladderBurstParams[rfNumber][2]));
				varIndex = 9;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->ladderBurstParams[rfNumber][3]));
			}
			else if(toBeSent2 & 0x10)
			{
				varIndex = 7;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->fmParams[rfNumber][0]));
				varIndex = 5;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->fmParams[rfNumber][1]));
				varIndex = 6;
				device_driver(rf_info->driver[i], CMD_SET_PARAMS,  // Voltage
														(rfNumber * 100) + varIndex, *(rf_info->fmParams[rfNumber][2]));
			}			
			rfNumber = rfNumber;
			varIndex = 0;
			status = device_driver(rf_info->driver[i], CMD_SET_CONTROL,  // Control
													rfNumber, *(rf_info->controlMng[rfNumber]), rf_info);
			*(rf_info->controlMng_mirror[rfNumber]) = *(rf_info->controlMng[rfNumber]); 
		
		}
	}

	pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void rf_dummy(INT hDB, INT hKey, void *info)
{
	TITANRF_INFO *rf_info;
	EQUIPMENT *pequipment;

	pequipment = (EQUIPMENT *) info;
	rf_info = (TITANRF_INFO *) pequipment->cd_info;

	pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void rf_update_label(INT hDB, INT hKey, void *info)
{
	INT i, status;
	TITANRF_INFO *rf_info;
	EQUIPMENT *pequipment;

	pequipment = (EQUIPMENT *) info;
	rf_info = (TITANRF_INFO *) pequipment->cd_info;

	/* update channel labels based on the midas channel names */
	for (i = 0; i < rf_info->num_channels; i++)
	status = device_driver(rf_info->driver[i], CMD_SET_LABEL,
												i - rf_info->channel_offset[i],
												rf_info->names + NAME_LENGTH * i);
}

/*------------------------------------------------------------------*/
INT rf_init(EQUIPMENT * pequipment)
{
	int status, size, i, j, k, index, offset, rfNumber, varIndex;
	char str[256];
	HNDLE hDB, hKey, hNames, hThreshold;
	TITANRF_INFO *rf_info;
	char paramsString[32];
	char measString[32];
	char ctrlString[32];
	char ctrlMngString[32];
	unsigned char *controlRead;

	/* allocate private data */

	pequipment->cd_info = calloc(1, sizeof(TITANRF_INFO));
	rf_info = (TITANRF_INFO *) pequipment->cd_info;

	/* get class driver root key */

	cm_get_experiment_database(&hDB, NULL);
	sprintf(str, "/Equipment/%s", pequipment->name);
	db_create_key(hDB, 0, str, TID_KEY);
	db_find_key(hDB, 0, str, &rf_info->hKeyRoot);

	/* save event format */
	size = sizeof(str);
	db_get_value(hDB, rf_info->hKeyRoot, "Common/Format", str, &size, TID_STRING, TRUE);

	if (equal_ustring(str, "Fixed"))
	{
		rf_info->format = FORMAT_FIXED;
	}
	else if (equal_ustring(str, "MIDAS"))
	{
		rf_info->format = FORMAT_MIDAS;
	}
	else if (equal_ustring(str, "YBOS"))
	{
		rf_info->format = FORMAT_YBOS;
	}

	/* count total number of channels */
	for (i = 0, rf_info->num_channels = 0; pequipment->driver[i].name[0]; i++) 
	{
		if (pequipment->driver[i].channels == 0) 
		{
			cm_msg(MERROR, "rf_init", "Driver with zero channels not allowed");
			return FE_ERR_ODB;
		}

		rf_info->num_channels += pequipment->driver[i].channels;
	}

	//rf_info->num_channels = 2;

	if (rf_info->num_channels == 0) 
	{
		cm_msg(MERROR, "rf_init", "No channels found in device driver list");
		return FE_ERR_ODB;
	}

	/* Allocate memory for buffers */

	for(rfNumber = 0; rfNumber < NUMBER_OF_RFGEN; rfNumber++)
	{
		for(i = 0; i < 5; i++) rf_info->ladderParams[rfNumber][i] = (double *) calloc(rf_info->num_channels, sizeof(double));
		for(i = 0; i < 3; i++) rf_info->burstParams[rfNumber][i] = (double *) calloc(rf_info->num_channels, sizeof(double));
		for(i = 0; i < 4; i++) rf_info->sweepParams[rfNumber][i] = (double *) calloc(rf_info->num_channels, sizeof(double));
		for(i = 0; i < 3; i++) rf_info->fmParams[rfNumber][i] = (double *) calloc(rf_info->num_channels, sizeof(double));
		for(i = 0; i < 4; i++) rf_info->ladderBurstParams[rfNumber][i] = (double *) calloc(rf_info->num_channels, sizeof(double));
		for(i = 0; i < 7; i++) rf_info->params[rfNumber][i] = (float *) calloc(rf_info->num_channels, sizeof(float));

		rf_info->controlMng[rfNumber] = (double *) calloc(rf_info->num_channels, sizeof(double));

		for(i = 0; i < 8; i++) rf_info->control[rfNumber][i] = (double *) calloc(rf_info->num_channels, sizeof(double));
		for(i = 0; i < 7; i++) rf_info->params_mirror[rfNumber][i] = (float *) calloc(rf_info->num_channels, sizeof(float));
		for(i = 0; i < 8; i++) rf_info->control_mirror[rfNumber][i] = (double *) calloc(rf_info->num_channels, sizeof(double));
		rf_info->controlMng_mirror[rfNumber] = (double *) calloc(rf_info->num_channels, sizeof(double));		
	}

	rf_info->names = (char *) calloc(rf_info->num_channels, NAME_LENGTH);
	for(i = 0; i < 5; i++) rf_info->channel_select[i] = (char *) calloc(rf_info->num_channels, 32 * sizeof(char));
	
	for(i = 0; i < 9; i++) rf_info->measured[i] = (float *) calloc(rf_info->num_channels, sizeof(float));
	rf_info->externalTemp = (float *) calloc(rf_info->num_channels, sizeof(float));
	rf_info->update_threshold = (float *) calloc(rf_info->num_channels, sizeof(float));	
	for(i = 0; i < 9; i++) rf_info->measured_mirror[i] = (float *) calloc(rf_info->num_channels, sizeof(float));
	rf_info->channel_offset = (INT *) calloc(rf_info->num_channels, sizeof(INT));
	rf_info->driver = (void *) calloc(rf_info->num_channels, sizeof(void *));

	if (!rf_info->driver) 
	{
		cm_msg(MERROR, "rf_init", "Not enough memory");
		return FE_ERR_ODB;
	}


	/*---- Initialize device drivers ----*/

	/* call init method */
	for (i = 0; pequipment->driver[i].name[0]; i++) 
	{
		sprintf(str, "Settings/Devices/%s", pequipment->driver[i].name);
		status = db_find_key(hDB, rf_info->hKeyRoot, str, &hKey);
		if (status != DB_SUCCESS)
		{
			db_create_key(hDB, rf_info->hKeyRoot, str, TID_KEY);
			status = db_find_key(hDB, rf_info->hKeyRoot, str, &hKey);
			if (status != DB_SUCCESS) {
			cm_msg(MERROR, "hv_init", "Cannot create %s entry in online database", str);
			free_mem(rf_info);
			return FE_ERR_ODB;
			}
		}

		status = device_driver(&pequipment->driver[i], CMD_INIT, hKey);
		if (status != FE_SUCCESS)
		{
			free_mem(rf_info);
			return status;
		}
	}

	/* compose device driver channel assignment */
	for (i = 0, j = 0, index = 0, offset = 0; i < rf_info->num_channels; i++, j++) 
	{
		while (j >= pequipment->driver[index].channels && pequipment->driver[index].name[0]) 
		{
			offset += j;
			index++;
			j = 0;
		}

		rf_info->driver[i] = &pequipment->driver[index];
		rf_info->channel_offset[i] = offset;
	}


	// define channel selects
	rf_info->channel_select[0] = "RF1";
	rf_info->channel_select[1] = "RF2";
	rf_info->channel_select[2] = "RF3";
	rf_info->channel_select[3] = "RF4";
	rf_info->channel_select[4] = "Temps";

	for(k = 0, rfNumber = 0; k < NUMBER_OF_RFGEN; rfNumber++, k++)
	{
		/*---- create params variables ----*/
		for(j = 0; j < 7; j++)
		{
			/* Assign params names */
			if(j == 0) sprintf(paramsString, "Variables/%s/sweepDur", rf_info->channel_select[k]);
			if(j == 1) sprintf(paramsString, "Variables/%s/numSteps", rf_info->channel_select[k]);
			if(j == 2) sprintf(paramsString, "Variables/%s/startFreq", rf_info->channel_select[k]);
			if(j == 3) sprintf(paramsString, "Variables/%s/endFreq", rf_info->channel_select[k]);
			if(j == 4) sprintf(paramsString, "Variables/%s/rfAmplitude", rf_info->channel_select[k]);
			if(j == 5) sprintf(paramsString, "Variables/%s/burstFreq", rf_info->channel_select[k]);
			if(j == 6) sprintf(paramsString, "Variables/%s/numCycles", rf_info->channel_select[k]);

			/* find the key and get data */
			db_merge_data(hDB, rf_info->hKeyRoot, paramsString,
						rf_info->params[rfNumber][j], sizeof(float) * rf_info->num_channels,
						rf_info->num_channels, TID_FLOAT);
			status = db_find_key(hDB, rf_info->hKeyRoot, paramsString, &rf_info->hKeyParams[rfNumber][j]);

			size = sizeof(float) * rf_info->num_channels;

			//initially read in the values
			varIndex = 3 + j;
			device_driver(rf_info->driver[0], CMD_GET_PARAMS, (rfNumber * 100) + varIndex, rf_info->params[rfNumber][j]); // Params
			db_set_data(hDB, rf_info->hKeyParams[rfNumber][j], rf_info->params[rfNumber][j], size,
							rf_info->num_channels, TID_FLOAT);

			db_open_record(hDB, rf_info->hKeyParams[rfNumber][j], rf_info->params[rfNumber][j],
								rf_info->num_channels * sizeof(float), MODE_READ, rf_setParams,
								pequipment);
		}

		/*---- create ladder params variables ----*/
		for(j = 0; j < 5; j++)
		{
			/* Assign params names */
			if(j == 0) sprintf(paramsString, "Variables/%s/lsweepDur", rf_info->channel_select[k]);
			if(j == 1) sprintf(paramsString, "Variables/%s/lnumSteps", rf_info->channel_select[k]);
			if(j == 2) sprintf(paramsString, "Variables/%s/lstartFreq", rf_info->channel_select[k]);
			if(j == 3) sprintf(paramsString, "Variables/%s/lendFreq", rf_info->channel_select[k]);
			if(j == 4) sprintf(paramsString, "Variables/%s/lrfAmplitude", rf_info->channel_select[k]);

			/* find the key and get data */
			db_merge_data(hDB, rf_info->hKeyRoot, paramsString,
						rf_info->ladderParams[rfNumber][j], sizeof(double) * rf_info->num_channels,
						rf_info->num_channels, TID_DOUBLE);
			status = db_find_key(hDB, rf_info->hKeyRoot, paramsString, &rf_info->hKeyLadderParams[rfNumber][j]);
			db_open_record(hDB, rf_info->hKeyLadderParams[rfNumber][j], rf_info->ladderParams[rfNumber][j],
								rf_info->num_channels * sizeof(double), MODE_READ, rf_dummy,
								pequipment);
		}

		/*---- create burst params variables ----*/
		for(j = 0; j < 3; j++)
		{
			/* Assign params names */
			if(j == 0) sprintf(paramsString, "Variables/%s/brfAmplitude", rf_info->channel_select[k]);
			if(j == 1) sprintf(paramsString, "Variables/%s/bburstFreq", rf_info->channel_select[k]);
			if(j == 2) sprintf(paramsString, "Variables/%s/bnumCycles", rf_info->channel_select[k]);

			/* find the key and get data */
			db_merge_data(hDB, rf_info->hKeyRoot, paramsString,
						rf_info->burstParams[rfNumber][j], sizeof(double) * rf_info->num_channels,
						rf_info->num_channels, TID_DOUBLE);
			status = db_find_key(hDB, rf_info->hKeyRoot, paramsString, &rf_info->hKeyBurstParams[rfNumber][j]);
			db_open_record(hDB, rf_info->hKeyBurstParams[rfNumber][j], rf_info->burstParams[rfNumber][j],
								rf_info->num_channels * sizeof(double), MODE_READ, rf_dummy,
								pequipment);
		}

		/*---- create sweep params variables ----*/
		for(j = 0; j < 4; j++)
		{
			/* Assign params names */
			if(j == 0) sprintf(paramsString, "Variables/%s/ssweepDur", rf_info->channel_select[k]);
			if(j == 1) sprintf(paramsString, "Variables/%s/sstartFreq", rf_info->channel_select[k]);
			if(j == 2) sprintf(paramsString, "Variables/%s/sendFreq", rf_info->channel_select[k]);
			if(j == 3) sprintf(paramsString, "Variables/%s/srfAmplitude", rf_info->channel_select[k]);

			/* find the key and get data */
			db_merge_data(hDB, rf_info->hKeyRoot, paramsString,
						rf_info->sweepParams[rfNumber][j], sizeof(double) * rf_info->num_channels,
						rf_info->num_channels, TID_DOUBLE);
			status = db_find_key(hDB, rf_info->hKeyRoot, paramsString, &rf_info->hKeySweepParams[rfNumber][j]);
			db_open_record(hDB, rf_info->hKeySweepParams[rfNumber][j], rf_info->sweepParams[rfNumber][j],
								rf_info->num_channels * sizeof(double), MODE_READ, rf_dummy,
								pequipment);
		}

		/*---- create fm params variables ----*/
		for(j = 0; j < 3; j++)
		{
			/* Assign params names */
			if(j == 0) sprintf(paramsString, "Variables/%s/fmrfAmplitude", rf_info->channel_select[k]);
			if(j == 1) sprintf(paramsString, "Variables/%s/fmstartFreq", rf_info->channel_select[k]);
			if(j == 2) sprintf(paramsString, "Variables/%s/fmendFreq", rf_info->channel_select[k]);

			/* find the key and get data */
			db_merge_data(hDB, rf_info->hKeyRoot, paramsString,
						rf_info->fmParams[rfNumber][j], sizeof(double) * rf_info->num_channels,
						rf_info->num_channels, TID_DOUBLE);
			status = db_find_key(hDB, rf_info->hKeyRoot, paramsString, &rf_info->hKeyFMParams[rfNumber][j]);
			db_open_record(hDB, rf_info->hKeyFMParams[rfNumber][j], rf_info->fmParams[rfNumber][j],
								rf_info->num_channels * sizeof(double), MODE_READ, rf_dummy,
								pequipment);
		}

		/*---- create ladder burst params variables ----*/
		for(j = 0; j < 4; j++)
		{
			/* Assign params names */
			if(j == 0) sprintf(paramsString, "Variables/%s/lbnumSteps", rf_info->channel_select[k]);
			if(j == 1) sprintf(paramsString, "Variables/%s/lbrfAmplitude", rf_info->channel_select[k]);
			if(j == 2) sprintf(paramsString, "Variables/%s/lbburstFreq", rf_info->channel_select[k]);
			if(j == 3) sprintf(paramsString, "Variables/%s/lbnumCycles", rf_info->channel_select[k]);

			/* find the key and get data */
			db_merge_data(hDB, rf_info->hKeyRoot, paramsString,
						rf_info->ladderBurstParams[rfNumber][j], sizeof(double) * rf_info->num_channels,
						rf_info->num_channels, TID_DOUBLE);
			status = db_find_key(hDB, rf_info->hKeyRoot, paramsString, &rf_info->hKeyLadderBurstParams[rfNumber][j]);
			db_open_record(hDB, rf_info->hKeyLadderBurstParams[rfNumber][j], rf_info->ladderBurstParams[rfNumber][j],
								rf_info->num_channels * sizeof(double), MODE_READ, rf_dummy,
								pequipment);
		}			

		/*---- create control variables ----*/
		//initially read in the current control values
		controlRead = calloc(1, sizeof(unsigned char *));
		device_driver(rf_info->driver[0], CMD_GET_CONTROL, rf_info->channel_offset[0], controlRead); // Control

		for(j = 0; j < 8; j++)
		{
			/* Assign control names */
			if(j == 0) sprintf(ctrlString, "Variables/%s/LadderStep", rf_info->channel_select[k]);
			if(j == 1) sprintf(ctrlString, "Variables/%s/Sweep", rf_info->channel_select[k]);
			if(j == 2) sprintf(ctrlString, "Variables/%s/Burst", rf_info->channel_select[k]);
			if(j == 3) sprintf(ctrlString, "Variables/%s/LadderBurst", rf_info->channel_select[k]);
			if(j == 4) sprintf(ctrlString, "Variables/%s/fmSweep", rf_info->channel_select[k]);
			if(j == 5) sprintf(ctrlString, "Variables/%s/None1", rf_info->channel_select[k]);
			if(j == 6) sprintf(ctrlString, "Variables/%s/None2", rf_info->channel_select[k]);
			if(j == 7) sprintf(ctrlString, "Variables/%s/KLock", rf_info->channel_select[k]);


			db_merge_data(hDB, rf_info->hKeyRoot, ctrlString,
							rf_info->control[rfNumber][j], sizeof(double) * rf_info->num_channels,
							rf_info->num_channels, TID_DOUBLE);
			status = db_find_key(hDB, rf_info->hKeyRoot, ctrlString, &rf_info->hKeyControl[rfNumber][j]);

			//distribute the read control values to each element in the control[] array
			if((*controlRead & (unsigned char)(1 << j))) *(rf_info->control[rfNumber][j]) = 1;
			//update the Variables (Control, 8bits)
			db_set_data(hDB, rf_info->hKeyControl[rfNumber][j], rf_info->control[rfNumber][j],
			sizeof(double) * rf_info->num_channels, rf_info->num_channels,
			TID_DOUBLE);

			size = sizeof(double) * rf_info->num_channels;
			db_get_data(hDB, rf_info->hKeyControl[rfNumber][j], rf_info->control[rfNumber][j], &size, TID_DOUBLE);
			db_open_record(hDB, rf_info->hKeyControl[rfNumber][j], rf_info->control[rfNumber][j],
						rf_info->num_channels * sizeof(double), MODE_READ, rf_setControl,
						pequipment);

			//copy all control values into the control mirror
			*(rf_info->control_mirror[rfNumber][j]) = *(rf_info->control[rfNumber][j]);
		}

		/*---- create controlMng variable ----*/
		sprintf(ctrlMngString, "Variables/%s/controlMng", rf_info->channel_select[k]);

		db_merge_data(hDB, rf_info->hKeyRoot, ctrlMngString,
						rf_info->controlMng[rfNumber], sizeof(double) * rf_info->num_channels,
						rf_info->num_channels, TID_DOUBLE);
		status = db_find_key(hDB, rf_info->hKeyRoot, ctrlMngString, &rf_info->hKeyControlMng[rfNumber]);
		db_open_record(hDB, rf_info->hKeyControlMng[rfNumber], rf_info->controlMng[rfNumber],
					rf_info->num_channels * sizeof(double), MODE_READ, rf_setControl,
					pequipment);

	//end of the rfNumber++ forloop
	}


	/*---- create measured variables ----*/
	for(j = 0; j < 4; j++)
	{
		/* Assign adc measurement channel names */
		if(j == 0) strcpy(measString, "Variables/RF1Monitor");
		if(j == 1) strcpy(measString, "Variables/RF2Monitor");
		if(j == 2) strcpy(measString, "Variables/RF3Monitor");
		if(j == 3) strcpy(measString, "Variables/RF4Monitor");

		db_merge_data(hDB, rf_info->hKeyRoot, measString,
					rf_info->measured[j], sizeof(float) * rf_info->num_channels,
					rf_info->num_channels, TID_FLOAT);
		status = db_find_key(hDB, rf_info->hKeyRoot, measString, &rf_info->hKeyMeasured[j]);

		memcpy(rf_info->measured_mirror[j], rf_info->measured[j],
		rf_info->num_channels * sizeof(float));
	}

	/*---- create ExtTemp measured variables ----*/
	db_merge_data(hDB, rf_info->hKeyRoot, "Variables/ExtTemp",
						rf_info->externalTemp, sizeof(float) * rf_info->num_channels,
						rf_info->num_channels, TID_FLOAT);
	db_find_key(hDB, rf_info->hKeyRoot, "Variables/ExtTemp", &rf_info->hKeyExtTemp);

	/*---- get default names from device driver ----*/
	for (i = 0; i < rf_info->num_channels; i++) 
	{
		sprintf(rf_info->names + NAME_LENGTH * i, "Default%%CH %d", i);
		device_driver(rf_info->driver[i], CMD_GET_LABEL,
								i - rf_info->channel_offset[i], rf_info->names + NAME_LENGTH * i);
	}
	db_merge_data(hDB, rf_info->hKeyRoot, "Settings/Names",
								rf_info->names, NAME_LENGTH * rf_info->num_channels,
								rf_info->num_channels, TID_STRING);

	/*---- set labels form midas SC names ----*/
	for (i = 0; i < rf_info->num_channels; i++) 
	{
		rf_info = (TITANRF_INFO *) pequipment->cd_info;
		device_driver(rf_info->driver[i], CMD_SET_LABEL,
								i - rf_info->channel_offset[i], rf_info->names + NAME_LENGTH * i);
	}

	/* open hotlink on channel names */
	if (db_find_key(hDB, rf_info->hKeyRoot, "Settings/Names", &hNames) == DB_SUCCESS)
	{
		db_open_record(hDB, hNames, rf_info->names, NAME_LENGTH*rf_info->num_channels,
									MODE_READ, rf_update_label, pequipment);
	}

	/*---- get default update threshold from device driver ----*/
	for (i = 0; i < rf_info->num_channels; i++) 
	{
		rf_info->update_threshold[i] = 1.f;      /* default 1 unit */
		device_driver(rf_info->driver[i], CMD_GET_THRESHOLD,
								i - rf_info->channel_offset[i], &rf_info->update_threshold[i]);
	}

	db_merge_data(hDB, rf_info->hKeyRoot, "Settings/Update Threshold Measured",
						rf_info->update_threshold, sizeof(float)*rf_info->num_channels,
						rf_info->num_channels, TID_FLOAT);

	/* open hotlink on update threshold */
	if (db_find_key(hDB, rf_info->hKeyRoot, "Settings/Update Threshold Measured", &hThreshold) == DB_SUCCESS)
	{
		db_open_record(hDB, hThreshold, rf_info->update_threshold, sizeof(float)*rf_info->num_channels,
		MODE_READ, NULL, NULL);
	}

	/*---- set initial params values ----*/
	// rf_setParamsAndControl(hDB, rf_info->hKeyParams[rfNumber][j], pequipment);

	/* initially read all channels */
	//for (i = 0; i < rf_info->num_channels; i++)
	//rf_read(pequipment, i);

	return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT rf_start(EQUIPMENT * pequipment)
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

INT rf_stop(EQUIPMENT * pequipment)
{
	INT i;

	/* call stop method of device drivers */
	for (i = 0; pequipment->driver[i].dd != NULL && pequipment->driver[i].flags & DF_MULTITHREAD ; i++)
	device_driver(&pequipment->driver[i], CMD_STOP);

	return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rf_exit(EQUIPMENT * pequipment)
{
	INT i;

	free_mem((TITANRF_INFO *) pequipment->cd_info);

	/* call exit method of device drivers */
	for (i = 0; pequipment->driver[i].dd != NULL; i++)
	device_driver(&pequipment->driver[i], CMD_EXIT);

	return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rf_idle(EQUIPMENT * pequipment)
{
	INT act, status;
	TITANRF_INFO *rf_info;

	rf_info = (TITANRF_INFO *) pequipment->cd_info;

	/* select next measurement channel */
	act = (rf_info->last_channel + 1) % rf_info->num_channels;

	/* measure channel */
	status = rf_read(pequipment, act);
	rf_info->last_channel = act;

	return status;
}

/*------------------------------------------------------------------*/

INT cd_rf_read(char *pevent, int offset)
{
	float *pdata;
	INT rfNumber = 0;
	TITANRF_INFO *rf_info;
	EQUIPMENT *pequipment;
	int j = 0;

	pequipment = *((EQUIPMENT **) pevent);
	rf_info = (TITANRF_INFO *) pequipment->cd_info;
	
	if (rf_info->format == FORMAT_FIXED) 
	{
		for(rfNumber = 0; rfNumber < NUMBER_OF_RFGEN; rfNumber++)
		{
			for(j = 0; j < 8; j++)
			{
				memcpy(pevent, rf_info->params[rfNumber][j], sizeof(float) * rf_info->num_channels);
				pevent += sizeof(float) * rf_info->num_channels;
			}
		}

		for(j = 0; j < 9; j++)
		{
			memcpy(pevent, rf_info->measured[j], sizeof(float) * rf_info->num_channels);
			pevent += sizeof(float) * rf_info->num_channels;
		}

		return 2 * sizeof(float) * rf_info->num_channels;
	} 
	else if (rf_info->format == FORMAT_MIDAS) 
	{
		bk_init(pevent);

		/* create VDMD bank */
		for(rfNumber = 0; rfNumber < NUMBER_OF_RFGEN; rfNumber++)
		{
			for(j = 0; j < 8; j++)
			{
				bk_create(pevent, "VDMD", TID_FLOAT, &pdata);
				memcpy(pdata, rf_info->params[rfNumber][j], sizeof(float) * rf_info->num_channels);
				pdata += rf_info->num_channels;
				bk_close(pevent, pdata);
			}
		}

		/* create IMES bank */
		for(j = 0; j < 9; j++)
		{
			bk_create(pevent, "IMES", TID_FLOAT, &pdata);
			memcpy(pdata, rf_info->measured[j], sizeof(float) * rf_info->num_channels);
			pdata += rf_info->num_channels;
			bk_close(pevent, pdata);
		}

		/* create TEMP2 bank */
		bk_create(pevent, "TEM2", TID_FLOAT, &pdata);
		memcpy(pdata, rf_info->externalTemp, sizeof(float) * rf_info->num_channels);
		pdata += rf_info->num_channels;
		bk_close(pevent, pdata);

		return bk_size(pevent);
	} 
	else if (rf_info->format == FORMAT_YBOS)
	{
		printf("Not implemented\n");
		return 0;
	} 
	else
	{
		return 0;
	}
}

/*------------------------------------------------------------------*/

INT cd_rf(INT cmd, EQUIPMENT * pequipment)
{
	INT status;

	switch (cmd) {
		case CMD_INIT:
		status = rf_init(pequipment);
		break;

		case CMD_START:
		status = rf_start(pequipment);
		break;

		case CMD_STOP:
		status = rf_stop(pequipment);
		break;

		case CMD_EXIT:
		status = rf_exit(pequipment);
		break;

		case CMD_IDLE:
		status = rf_idle(pequipment);
		break;

		default:
		cm_msg(MERROR, "Generic class driver", "Received unknown command %d", cmd);
		status = FE_ERR_DRIVER;
		break;
	}

	return status;
}

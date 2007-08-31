/********************************************************************\

  Name:         dd_mscbrf.c
  Created by:   Brian Lee

  Based on the mscbhvr device driver

  Contents:     TITAN RF Generator Device Driver
  Used for midas/mscb/embedded/Experiements/rf/

  $Id: dd_mscbrf.c 3765 2007-07-20 23:53:38Z amaudruz $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "midas.h"
#include "mscb.h"

//command definitions
#define CMD_SET_VBIAS					 CMD_SET_FIRST+5
#define CMD_SET_CONTROL					 CMD_SET_FIRST+6
#define CMD_SET_ASUMDACTH				 CMD_SET_FIRST+7
#define CMD_SET_CHPUMPDAC				 CMD_SET_FIRST+8
#define CMD_SET_BIAS_EN					 CMD_SET_FIRST+9
#define CMD_SET_PARAMS					 CMD_SET_FIRST+10

#define CMD_GET_INTERNALTEMP			 CMD_GET_FIRST+2
#define CMD_GET_ADCMEAS					 CMD_GET_FIRST+3
#define CMD_GET_CONTROL					 CMD_GET_FIRST+4
#define CMD_GET_BIASEN					 CMD_GET_FIRST+5
#define CMD_GET_CHPUMPDAC				 CMD_GET_FIRST+6
#define CMD_GET_ASUMDACTH			    CMD_GET_FIRST+7
#define CMD_GET_VBIAS					 CMD_GET_FIRST+8
#define CMD_GET_ACTUALVBIAS			 CMD_GET_FIRST+9
#define CMD_GET_PARAMS						 CMD_GET_FIRST+10
#define CMD_GET_TEMPERATURE1			 CMD_GET_FIRST+11
#define CMD_GET_TEMPERATURE2			 CMD_GET_FIRST+12
#define CMD_GET_TEMPERATURE3			 CMD_GET_FIRST+13
#define CMD_GET_TEMPERATURE4			 CMD_GET_FIRST+14
#define CMD_GET_TEMPERATURE5			 CMD_GET_FIRST+15
#define CMD_GET_TEMPERATURE6			 CMD_GET_FIRST+16
#define CMD_GET_EXTERNALTEMP			 CMD_GET_FIRST+17

//address of the board that does ADC measurements
#define ADDRESS_OF_ADCBOARD 5
#define EXT_TEMP_INDEX 10

typedef struct {
   char mscb_device[NAME_LENGTH];
   char pwd[NAME_LENGTH];
   int  base_address;
} MSCBFGD_SETTINGS;

#define MSCBFGD_SETTINGS_STR "\
MSCB Device = STRING : [32] usb0\n\
MSCB Pwd = STRING : [32] \n\
Base Address = INT : 0x3\n\
"

typedef struct {
   MSCBFGD_SETTINGS settings;
   int num_channels;
   int fd;
} MSCBTITANRF_INFO;

/*---- device driver routines --------------------------------------*/

INT mscbrf_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
//   unsigned int  ramp;
   int  i, status, size, flag;
   HNDLE hDB, hkeydd;
   MSCBTITANRF_INFO *info;
   MSCB_INFO node_info;

   /* allocate info structure */
   info = (MSCBTITANRF_INFO *) calloc(1, sizeof(MSCBTITANRF_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create MSCBFGD settings record */
   status = db_create_record(hDB, hkey, "DD", MSCBFGD_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   db_find_key(hDB, hkey, "DD", &hkeydd);
   size = sizeof(info->settings);
   db_get_record(hDB, hkeydd, &info->settings, &size, 0);

   info->num_channels = channels;

   /* open device on MSCB */
   info->fd = mscb_init(info->settings.mscb_device, NAME_LENGTH, info->settings.pwd, FALSE);
   if (info->fd < 0) {
      cm_msg(MERROR, "mscbrf_init",
             "Cannot access MSCB submaster at \"%s\". Check power and connection.",
             info->settings.mscb_device);
      return FE_ERR_HW;
   }

   /* check if FGD devices are alive */
   for (i=info->settings.base_address ; i<info->settings.base_address+channels ; i++) {
      status = mscb_ping(info->fd, i, 0);
      if (status != MSCB_SUCCESS) {
         cm_msg(MERROR, "mscbrf_init",
               "Cannot ping MSCB FGD address \"%d\". Check power and connection.", i);
         return FE_ERR_HW;
      }

      mscb_info(info->fd, i, &node_info);
      if (strcmp(node_info.node_name, "TITAN_RF_GEN") != 0) {
         cm_msg(MERROR, "mscbrf_init",
               "Found one expected node \"%s\" at address \"%d\".", node_info.node_name, i);
         return FE_ERR_HW;
      }

      /* turn on HV main switch */
      flag = 0x31;
      // Set rampup/dwn (don't need ramping)
      //mscb_write(info->fd, i, 0, &flag, 1);
      //ramp = 200;
      //mscb_write(info->fd, i, 4, &ramp, 2);
      //mscb_write(info->fd, i, 5, &ramp, 2);
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbrf_exit(MSCBTITANRF_INFO * info)
{
   mscb_exit(info->fd);

   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set Params
INT mscbrf_setParams(MSCBTITANRF_INFO * info, INT channel, INT varIndex, double value)
{
	int size = 4;
	float toBeSent = 0.0;
	short toBeSent2 = 0;
	unsigned int toBeSent3 = 0;
	
	if(varIndex == 4 || varIndex == 9)
	{
		size = 2; //2 bytes for numSteps and numCycles
		toBeSent2 = (short) value;
		mscb_write(info->fd, info->settings.base_address + channel, varIndex, &toBeSent2, size);
	}
	
	else if(varIndex == 5 || varIndex == 6 || varIndex == 8)
	{
		size=4; //4 byte unsigned integer determines the frequencies fStart, fEnd amd fBurts in centiHertz
		        // while the value being passed in the value is odb frequency value in MegaHertz.
		toBeSent3 = (unsigned int) ceil(value*1.e8); // value is the frequency in centiHertz
		mscb_write(info->fd, info->settings.base_address + channel, varIndex, &toBeSent3, size);
	}
	
	else
	{
		size = 4;
		toBeSent = (float) value;
		mscb_write(info->fd, info->settings.base_address + channel, varIndex, &toBeSent, size);
	}
	
	return FE_SUCCESS;
}

// Get Params
INT mscbrf_getParams(MSCBTITANRF_INFO * info, INT channel, INT varIndex, float *pvalue)
{
  int size = 0;
  int readValue=0;
  
	//if the reading is for numSteps and numCycles which are in Int format
	if(varIndex == 4 || varIndex == 9)
	{
		size = 4; //4 bytes
		mscb_read(info->fd, info->settings.base_address + channel, varIndex, &readValue, &size);
		*pvalue = (float) readValue;
	}
	//otherwise it's 4bytes float
	else
	{
		size = 4; //1byte
		mscb_read(info->fd, info->settings.base_address + channel, varIndex, pvalue, &size);
	}

	return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set Control
INT mscbrf_setControl(MSCBTITANRF_INFO * info, INT channel, double value)
{
  int size = 1;
   unsigned char toBeSent2 = 0;
  unsigned char readValue = 0;	

  toBeSent2 = (unsigned char) value;

  printf("Set control:[%i]%o\n", channel, toBeSent2);

	 mscb_write(info->fd, info->settings.base_address + channel, 0, &toBeSent2, 1);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbrf_getControl(MSCBTITANRF_INFO * info, INT channel, unsigned char *rvalue)
{
  int size = 1;

  mscb_read(info->fd, info->settings.base_address + channel, 0, rvalue, &size);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbrf_get_adcMeas(MSCBTITANRF_INFO * info, INT varIndex, float *pvalue)
{
   int size = 4;

   mscb_read(info->fd, ADDRESS_OF_ADCBOARD, varIndex, pvalue, &size);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbrf_get_temperature(MSCBTITANRF_INFO * info, INT channel, float *pvalue, int cmd)
{
  int size = 4;

  switch (cmd) 
	{
		case CMD_GET_EXTERNALTEMP:  // External Temperature
					mscb_read(info->fd, ADDRESS_OF_ADCBOARD, EXT_TEMP_INDEX, pvalue, &size);
					break;
  }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbrf_setDacBias_current_limit(MSCBTITANRF_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.base_address + channel, 7, &limit, 4);
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT mscbrf(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status, rfNumber, varIndex, temp;
   float value, *pvalue;
	double dvalue;
   void *info, *bd;
   unsigned char * rvalue;


   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      bd = va_arg(argptr, void *);
      status = mscbrf_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbrf_exit(info);
      break;

	case CMD_GET_PARAMS:  // Voltage
      info = va_arg(argptr, void *);
			temp = va_arg(argptr, INT);
			rfNumber = (INT) (temp / 100);
			varIndex = temp - (rfNumber * 100);  
      pvalue = (float *) va_arg(argptr, float *);
      status = mscbrf_getParams(info, rfNumber, varIndex, pvalue);
      //ss_sleep(10);
      break;

   case CMD_SET_PARAMS:  // Voltage
      info = va_arg(argptr, void *);
      temp = va_arg(argptr, INT);
			rfNumber = (INT) (temp / 100);
			varIndex = temp - (rfNumber * 100);  
      dvalue = (double) va_arg(argptr, double);
    printf("Set Bias Voltage %d :[%i]%f\n", (varIndex-4), rfNumber, dvalue);
    status = mscbrf_setParams(info, rfNumber, varIndex, dvalue);
      //ss_sleep(10);
      break;

	case CMD_GET_CONTROL:
			info = va_arg(argptr, void *);
      rfNumber = va_arg(argptr, INT);
      rvalue = (unsigned char *) va_arg(argptr, unsigned char *);
			status = mscbrf_getControl(info, rfNumber, rvalue);
      break;

   case CMD_SET_CONTROL:  // Update control variable
      info = va_arg(argptr, void *);
      rfNumber = va_arg(argptr, INT);
      dvalue = (double) va_arg(argptr, double);
      status = mscbrf_setControl(info, rfNumber, dvalue);
      break;

   case CMD_GET_ADCMEAS:  // Current
      info = va_arg(argptr, void *);
      varIndex = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbrf_get_adcMeas(info, varIndex, pvalue);
      break;

   case CMD_GET_EXTERNALTEMP:  // ExtTemp
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbrf_get_temperature(info, channel, pvalue, cmd);
      break;

   case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbrf_setDacBias_current_limit(info, channel, value);
      break;

   case CMD_GET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_GET_THRESHOLD:
      status = FE_SUCCESS;
      break;

   case CMD_GET_DEMAND:
      status = FE_SUCCESS;
      break;

   case CMD_SET_LABEL:
      status = FE_SUCCESS;
      break;

   default:
      cm_msg(MERROR, "mscbrf device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
/********************************************************************\

  Name:         dd_mscbasum.c
  Created by:   Brian Lee

  Based on the mscbhvr device driver

  Contents:     MSCB T2K-ASUM SiPm High Voltage Device Driver
  Used for midas/mscb/embedded/Experiements/asum/

  $Id: dd_mscbasum.c $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "midas.h"
#include "mscb.h"

#undef CMD_GET_LAST
#define CMD_GET_EXTERNALTEMP         CMD_GET_FIRST+6
#define CMD_GET_INTERNALTEMP         CMD_GET_FIRST+7
#define CMD_GET_ADCMEAS              CMD_GET_FIRST+8
#define CMD_GET_CONTROL              CMD_GET_FIRST+9
#define CMD_GET_BIASEN               CMD_GET_FIRST+10
#define CMD_GET_CHPUMPDAC				 CMD_GET_FIRST+11
#define CMD_GET_ASUMDACTH				 CMD_GET_FIRST+12
#define CMD_GET_LAST						 CMD_GET_FIRST+12


#undef CMD_SET_LAST
#define CMD_SET_CONTROL              CMD_SET_FIRST+5 
#define CMD_SET_ASUMDACTH            CMD_SET_FIRST+6
#define CMD_SET_CHPUMPDAC            CMD_SET_FIRST+7
#define CMD_SET_BIAS_EN					 CMD_SET_FIRST+8
#define CMD_SET_LAST						 CMD_SET_FIRST+8


/*---- globals -----------------------------------------------------*/
extern indexNum;
extern indexNumADC;
extern controlFlag;
extern biasEnFlag;

typedef struct {
   char mscb_device[NAME_LENGTH];
   char pwd[NAME_LENGTH];
   int  base_address;
} MSCBFGD_SETTINGS;

#define MSCBFGD_SETTINGS_STR "\
MSCB Device = STRING : [32] usb0\n\
MSCB Pwd = STRING : [32] \n\
Base Address = INT : 0xff00\n\
"

typedef struct {
   MSCBFGD_SETTINGS settings;
   int num_channels;
   int fd;
} MSCBFGD_INFO;

/*---- device driver routines --------------------------------------*/

INT mscbasum_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
//   unsigned int  ramp;
   int  i, status, size, flag;
   HNDLE hDB, hkeydd;
   MSCBFGD_INFO *info;
   MSCB_INFO node_info;

   /* allocate info structure */
   info = (MSCBFGD_INFO *) calloc(1, sizeof(MSCBFGD_INFO));
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
      cm_msg(MERROR, "mscbasum_init",
             "Cannot access MSCB submaster at \"%s\". Check power and connection.",
             info->settings.mscb_device);
      return FE_ERR_HW;
   }

   /* check if FGD devices are alive */
   for (i=info->settings.base_address ; i<info->settings.base_address+channels ; i++) {
      status = mscb_ping(info->fd, i, 0);
      if (status != MSCB_SUCCESS) {
         cm_msg(MERROR, "mscbasum_init",
               "Cannot ping MSCB FGD address \"%d\". Check power and connection.", i);
         return FE_ERR_HW;
      }

      mscb_info(info->fd, i, &node_info);
      if (strcmp(node_info.node_name, "T2K-ASUM") != 0) {
         cm_msg(MERROR, "mscbasum_init",
               "Found one expected node \"%s\" at address \"%d\".", node_info.node_name, i);
         return FE_ERR_HW;
      }

      /* turn on HV main switch */
      flag = 0x31;
      // Set rampup/dwn (don't need ramping)
      mscb_write(info->fd, i, 0, &flag, 1);
      //ramp = 200;
      //mscb_write(info->fd, i, 4, &ramp, 2);
      //mscb_write(info->fd, i, 5, &ramp, 2);
   }

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbasum_exit(MSCBFGD_INFO * info)
{
   mscb_exit(info->fd);

   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set Voltage
INT mscbasum_setDacBias(MSCBFGD_INFO * info, INT channel, float value)
{
   unsigned char toBeSent = 0;

   toBeSent = (unsigned char)(value * 255 / 4.962);

   mscb_write(info->fd, info->settings.base_address + channel, indexNum, &toBeSent, 1);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set Asum Dac Threshold Voltage
INT mscgasum_setasumDacTh (MSCBFGD_INFO * info, INT channel, float value)
{
   unsigned char toBeSent = 0;

   toBeSent = (unsigned char)(value * 65535 / 4.962);

   mscb_write(info->fd, info->settings.base_address + channel, 3, &toBeSent, 1);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set DAC ChargePump Voltage
INT mscgasum_setChPumpDac (MSCBFGD_INFO * info, INT channel, float value)
{
   unsigned char toBeSent = 0;

   toBeSent = (unsigned char)(value * 255 / 4.962);

   mscb_write(info->fd, info->settings.base_address + channel, 4, &toBeSent, 1);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set Bias Voltage
INT mscbasum_controlUpdate(MSCBFGD_INFO * info, INT channel, float value)
{
	int size = 1;
   unsigned char toBeSent2 = 0;
	unsigned char readValue = 0;

	toBeSent2 = (unsigned char) value;
	mscb_read(info->fd, info->settings.base_address + channel, 0, &readValue, &size);

	if(controlFlag) toBeSent2 |= readValue;
	else toBeSent2 = readValue & ~(toBeSent2);

	printf("Set control:[%i]%o\n", channel, toBeSent2);

   mscb_write(info->fd, info->settings.base_address + channel, 0, &toBeSent2, 1);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbasum_getControl(MSCBFGD_INFO * info, INT channel,char *rvalue)
{
	int size = 1;
	char read = 0;

	mscb_read(info->fd, info->settings.base_address + channel, 0, rvalue, &size);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
// Set Bias Voltage
INT mscbasum_biasEnUpdate(MSCBFGD_INFO * info, INT channel, float value)
{
	int size = 1;
   unsigned char toBeSent2 = 0;
	unsigned char readValue = 0;

	toBeSent2 = (unsigned char) value;
	mscb_read(info->fd, info->settings.base_address + channel, 2, &readValue, &size);

	if(biasEnFlag) toBeSent2 |= readValue;
	else toBeSent2 = readValue & ~(toBeSent2);

	printf("Set Bias Enable:[%i]%o\n", channel, toBeSent2);

   mscb_write(info->fd, info->settings.base_address + channel, 2, &toBeSent2, 1);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbasum_getbiasEn(MSCBFGD_INFO * info, INT channel,char *rvalue)
{
	int size = 1;

	mscb_read(info->fd, info->settings.base_address + channel, 2, rvalue, &size);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbasum_getasumDacTh(MSCBFGD_INFO * info, INT channel,float *pvalue)
{
	int size = 2;
	short int readValue=0;

	mscb_read(info->fd, info->settings.base_address + channel, 3, &readValue, &size);

	*pvalue = (float)(readValue * 4.962 / 65535);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT mscbasum_getchPumpDac(MSCBFGD_INFO * info, INT channel,float *pvalue)
{
	int size = 1;
	char readValue=0;

	mscb_read(info->fd, info->settings.base_address + channel, 4, &readValue, &size);

	*pvalue = (float)(readValue * 4.962 / 255);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbasum_get_adcMeas(MSCBFGD_INFO * info, INT channel, float *pvalue)
{
   int size = 4;

   mscb_read(info->fd, info->settings.base_address + channel, indexNumADC, pvalue, &size);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbasum_get_temperature(MSCBFGD_INFO * info, INT channel, float *pvalue, int cmd)
{
  int size = 4;

  switch (cmd) {
   case CMD_GET_EXTERNALTEMP:  // External Temperature
	  mscb_read(info->fd, info->settings.base_address + channel, 22, pvalue, &size);
     break;
   case CMD_GET_INTERNALTEMP:  // Internal Temperature
     mscb_read(info->fd, info->settings.base_address + channel , 24, pvalue, &size);
     break;
  }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbasum_setDacBias_current_limit(MSCBFGD_INFO * info, int channel, float limit)
{
   mscb_write(info->fd, info->settings.base_address + channel, 7, &limit, 4);
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT mscbasum(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   void *info, *bd;
	char * rvalue;


   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      bd = va_arg(argptr, void *);
      status = mscbasum_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbasum_exit(info);
      break;

   case CMD_SET:  // Voltage
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
		printf("Set Bias Voltage %d :[%i]%f\n", (indexNum-4), channel, value);
		status = mscbasum_setDacBias(info, channel, value);
      //ss_sleep(10);
      break;

	case CMD_GET_CONTROL:
		info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      rvalue = (char *) va_arg(argptr, char *);
      status = mscbasum_getControl(info, channel, rvalue);
      break;

   case CMD_SET_CONTROL:  // Update control variable
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbasum_controlUpdate(info, channel, value);
      break;

	case CMD_GET_BIASEN:
		info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      rvalue = (char *) va_arg(argptr, char *);
      status = mscbasum_getbiasEn(info, channel, rvalue);
      break;

	case CMD_SET_BIAS_EN:  // Update Bias Enable variable
		info = va_arg(argptr, void *);
		channel = va_arg(argptr, INT);
		value = (float) va_arg(argptr, double);
		status = mscbasum_biasEnUpdate(info, channel, value);
		break;

	case CMD_SET_ASUMDACTH:
		info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      printf("Set ASUM DAC Threshold voltage:[%i]%f\n", channel, value);
      status = mscgasum_setasumDacTh(info, channel, value);
      break;

	case CMD_GET_ASUMDACTH:
		info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbasum_getasumDacTh(info, channel, pvalue);
      break;

	case CMD_GET_CHPUMPDAC:
		info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbasum_getchPumpDac(info, channel, pvalue);
      break;

	case CMD_SET_CHPUMPDAC:
	info = va_arg(argptr, void *);
   channel = va_arg(argptr, INT);
   value = (float) va_arg(argptr, double);
   printf("Set Charge Pump DAC voltage:[%i]%f\n", channel, value);
   status = mscgasum_setChPumpDac(info, channel, value);
   break;

   case CMD_GET_ADCMEAS:  // Current
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbasum_get_adcMeas(info, channel, pvalue);
      break;

   case CMD_GET_EXTERNALTEMP:  // IntTemp
   case CMD_GET_INTERNALTEMP:  // ExtTemp
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = mscbasum_get_temperature(info, channel, pvalue, cmd);
      break;

   case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = mscbasum_setDacBias_current_limit(info, channel, value);
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
      cm_msg(MERROR, "mscbasum device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

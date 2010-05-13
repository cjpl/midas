/********************************************************************\

  Name:         dd_isegshv.c
  Created by:   Pierre-Andre Amaudruz

  Based on the mscbhvr device driver

  Contents:     ISEG HV 

  $Id$
\********************************************************************/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "midas.h"
#include "dd_isegvhs.h"
#include "isegvhsdrv.h"
#include "isegvhs.h"
#include "mvmestd.h"

/*---- globals -----------------------------------------------------*/

typedef struct {
  DWORD  base;
} ISEGVHS_SETTINGS;

#define ISEGVHS_SETTINGS_STR "\
Base Address = DWORD : 0x4000\n\
"

typedef struct {
   ISEGVHS_SETTINGS settings;
   int num_channels;
   MVME_INTERFACE  *fd;
} ISEGVHS_INFO;

/*---- device driver routines --------------------------------------*/
INT isegvhs_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{

   int  status, size;
   HNDLE hDB, hkeydd;
   ISEGVHS_INFO *info;
   MVME_INTERFACE *myvme;

   /* allocate info structure */
   info = (ISEGVHS_INFO *) calloc(1, sizeof(ISEGVHS_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create ISEGVHS settings record */
   status = db_create_record(hDB, hkey, "DD", ISEGVHS_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   db_find_key(hDB, hkey, "DD", &hkeydd);
   size = sizeof(info->settings);
   db_get_record(hDB, hkeydd, &info->settings, &size, 0);

   info->num_channels = channels;

   /* open device on MSCB */
   status = mvme_open(&myvme, 0);
   if (status != SUCCESS ) {
     cm_msg(MERROR, "isegvhs_init",
	    "Cannot access VME, Check power and connection.");
     return FE_ERR_HW;
   }
   info->fd = myvme;
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT isegvhs_exit(ISEGVHS_INFO * info)
{
  int status;
  status = mvme_close(info->fd);
  
  free(info);
  
  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
// Set Voltage
INT isegvhs_set(ISEGVHS_INFO * info, INT channel, float value)
{
  
  if (value > 0.) { // HV ON
    isegvhs_RegisterWrite(info->fd, info->settings.base
		  , ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CHANNEL_CONTROL
		  , ISEGVHS_SET_ON);
    isegvhs_RegisterWriteFloat(info->fd, info->settings.base
		  , ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_SET
		  , value);
    cm_msg(MINFO, "isegvhs", "Channel %d HV ON", channel);
  } else { // HV OFF
    isegvhs_RegisterWrite(info->fd, info->settings.base
		  , ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CHANNEL_CONTROL
		  , ISEGVHS_SET_OFF);
    cm_msg(MINFO, "isegvhs", "Channel %d HV OFF", channel);
  }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT isegvhs_get(ISEGVHS_INFO * info, INT channel, float *pvalue)
{
  *pvalue = isegvhs_RegisterReadFloat(info->fd, info->settings.base
			  , ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_MEASURE);
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT isegvhs_get_current(ISEGVHS_INFO * info, INT channel, float *pvalue)
{
   *pvalue = isegvhs_RegisterReadFloat(info->fd, info->settings.base
			  , ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CURRENT_MEASURE);
   if (*pvalue < 1E-5) *pvalue = 0.0;
   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT isegvhs_get_other(ISEGVHS_INFO * info, INT channel, float *pvalue, int cmd)
{
  switch (cmd) {
  case CMD_GET_DEMAND:
    *pvalue = isegvhs_RegisterReadFloat(info->fd, info->settings.base
					, ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_SET);
    break;
  case CMD_GET_CURRENT_LIMIT:
    *pvalue = isegvhs_RegisterReadFloat(info->fd, info->settings.base
					, ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CURRENT_SET);
    break;
  case CMD_GET_TRIP_TIME:
    *pvalue = isegvhs_RegisterReadFloat(info->fd, info->settings.base
					, ISEGVHS_CHANNEL_BASE+(channel*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CHANNEL_STATUS);
    break;
  case CMD_GET_RAMPUP:
    *pvalue = isegvhs_RegisterReadFloat(info->fd, info->settings.base, ISEGVHS_VOLTAGE_RAMP_SPEED);
    break;
  case CMD_GET_RAMPDOWN:  // use it for current ramp speed
    *pvalue = isegvhs_RegisterReadFloat(info->fd, info->settings.base, ISEGVHS_CURRENT_RAMP_SPEED);
    break;
  }
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT isegvhs_set_other(ISEGVHS_INFO * info, int channel, float limit, int cmd)
{
  switch (cmd) {
  case CMD_SET_RAMPUP:
    isegvhs_RegisterWriteFloat(info->fd, info->settings.base, ISEGVHS_VOLTAGE_RAMP_SPEED, limit);
    break;
  case CMD_SET_RAMPDOWN:   // Use it for current ramp speed
    isegvhs_RegisterWriteFloat(info->fd, info->settings.base, ISEGVHS_CURRENT_RAMP_SPEED, limit);
    break;
  case CMD_SET_CURRENT_LIMIT:
    isegvhs_RegisterWriteFloat(info->fd, info->settings.base, ISEGVHS_CHANNEL_BASE+(ISEGVHS_CHANNEL_OFFSET*channel)+ISEGVHS_CURRENT_SET, limit);
    break;
    // VOLTAGE_SET is used for setting and trip limit, uses trim pot for all channel limit for voltage and current
    //   case CMD_SET_VOLTAGE_LIMIT:
    //     isegvhs_RegisterWriteFloat(info->fd, info->settings.base, ISEGVHS_CHANNEL_BASE+(ISEGVHS_CHANNEL_OFFSET*channel)+ISEGVHS_VOLTAGE_SET, limit);
    //    break;
  }
    return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
void isegvhs_CsrTest(ISEGVHS_INFO * info)
{
   INT csr;
   char str[128];

   csr = isegvhs_RegisterRead(info->fd, info->settings.base, ISEGVHS_MODULE_STATUS);
   sprintf(str, "CSR:0x%x >> ", csr);
   strcat(str, csr & 0x0001 ? "FineAdj /" : " NoFineAdj /"); 
   strcat(str, csr & 0x0080 ? "CmdCpl /" : " /"); 
   strcat(str, csr & 0x0100 ? "noError /" : " ERROR /"); 
   strcat(str, csr & 0x0200 ? "noRamp /" : " RAMPING /"); 
   strcat(str, csr & 0x0400 ? "Saf closed /" : " /"); 
   strcat(str, csr & 0x0800 ? "Evt Active /" : " /"); 
   strcat(str, csr & 0x1000 ? "Mod Good /" : " MOD NO good /"); 
   strcat(str, csr & 0x2000 ? "Supply Good /" : " Supply NO good /"); 
   strcat(str, csr & 0x4000 ? "Temp Good /" : " Temp NO good /"); 
   printf("CSR:%s\n", str);
   if (!(csr & 0x100)) {
     isegvhs_RegisterWrite(info->fd, info->settings.base, ISEGVHS_MODULE_CONTROL, 0x40);    // doCLEAR
     cm_msg(MERROR, "isegvhs", str);
     cm_msg(MERROR, "isegvhs", "Command doCLEAR issued, check the Voltages!");
   } else
     //     cm_msg(MINFO, "isegvhs", str);
   return;
}

/*---- device driver entry point -----------------------------------*/
INT dd_isegvhs(INT cmd, ...)
{
  static DWORD itstime=0;
  va_list argptr;
  HNDLE hKey;
  INT channel, status;
  float value, *pvalue;
  void *info, *bd;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      bd = va_arg(argptr, void *);
      status = isegvhs_init(hKey, (void **)info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = isegvhs_exit(info);
      break;

   case CMD_SET:  // Voltage
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = isegvhs_set(info, channel, value);
      break;

   case CMD_GET: // Voltage
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      if((ss_time() - itstime) > 10) {
	isegvhs_CsrTest(info);
	itstime = ss_time();
      }
      status = isegvhs_get(info, channel, pvalue);
      break;

   case CMD_GET_CURRENT:  // Current
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = isegvhs_get_current(info, channel, pvalue);
      break;

   case CMD_GET_RAMPUP:
   case CMD_GET_RAMPDOWN:
   case CMD_GET_TRIP_TIME:
   case CMD_GET_DEMAND:          // Desired Voltage
   case CMD_GET_CURRENT_LIMIT:  //
     info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = isegvhs_get_other(info, channel, pvalue, cmd);
      break;

   case CMD_SET_RAMPUP:
   case CMD_SET_RAMPDOWN:
   case CMD_SET_CURRENT_LIMIT:
     info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = isegvhs_set_other(info, channel, value, cmd);
      break;

   case CMD_GET_LABEL:
      status = FE_SUCCESS;
      break;

   case CMD_SET_LABEL:
      status = FE_SUCCESS;
      break;

   default:
//      cm_msg(MERROR, "isegvhs device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

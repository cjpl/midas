/********************************************************************\

  Name:         psi_separator.c
  Created by:   Stefan Ritt      2006/06/22
                based on separator61.c by Thomas Prokscha 

  Contents:     Device Driver for Konrad Deiter's separator control
  		          for the SEP41V PSI separator

                The driver supports three channels: voltage, current
                and vacuum

  $Id: psi_separator.c 1953 2006-05-11 11:15:26Z prokscha@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "midas.h"

/*---- globals -----------------------------------------------------*/

#define STR_SIZE 1024           //!< max. size of string to be sent or read from separator PC

/* --------- to handle error messages ------------------------------*/
#define MAX_ERROR        5      //!< maximum number of error messages
#define DELTA_TIME_ERROR 3600   //!< reset error counter after DELTA_TIME_ERROR seconds

//! Store any parameters the device driver needs in following
//! structure. Edit the PSI_SEPARATOR_SETTINGS_STR accordingly. This
//! contains the initial current limit in muA

typedef struct {
   float max_current;
} PSI_SEPARATOR_SETTINGS;

#define PSI_SEPARATOR_SETTINGS_STR "\
Max. current = FLOAT : 10\n\
"

//! following structure contains private variables to the device
//! driver. It is necessary to store it here in case the device
//! driver is used for more than one device in one frontend. If it
//! would be stored in a global variable, one device could over-
//! write the other device's variables.

typedef struct {
   PSI_SEPARATOR_SETTINGS settings; //!< stores internal settings, not active
   float hvdemand;                    //!< demand values, to be read initially from separator PC
   float hvmeasured;                  //!< measured HV
   float hvcurrent;                   //!< measured current
   float sepvac;                      //!< separator vacuum;
   INT(*bd) (INT cmd, ...);           //!< bus driver entry function
   void *bd_info;                     //!< private info of bus driver
   HNDLE hkey;                        //!< ODB key for bus driver info
   INT errorcount;                    //!< number of error encountered
   DWORD lasterr_resettime;           //!< last error reset time
   DWORD lasterrtime;                 //!< last error time stamp
} PSI_SEPARATOR_INFO;

static DWORD last_update;
static DWORD last_set;

INT psi_separator_rall(PSI_SEPARATOR_INFO * info);

/*---- device driver routines --------------------------------------*/

//! Initializes the device driver, i.e. generates all the necessary
//! structures in the ODB if necessary.
//! \param hkey is the device driver handle given from the class driver
//! \param **pinfo is needed to store the internal info structure
//! \param channels is the number of channels of the device (from the class driver)
//! \param *bd is a pointer to the bus driver
INT psi_separator_init(HNDLE hkey, void **pinfo, INT channels,
                       INT(*bd) (INT cmd, ...))
{
   int status;
   HNDLE hDB, hkeydd;
   PSI_SEPARATOR_INFO *info;

   if (channels != 3) {
      cm_msg(MERROR, "psi_separator_init",
             "Driver requires 3 channels, not %d", channels);
      return FE_ERR_ODB;
   }

   //allocate info structure
   info = calloc(1, sizeof(PSI_SEPARATOR_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   // create settings record
   status = db_create_record(hDB, hkey, "DD", PSI_SEPARATOR_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   db_find_key(hDB, hkey, "DD", &hkeydd);
   status = db_open_record(hDB, hkeydd, &info->settings, sizeof(info->settings),
                      MODE_READ, NULL, NULL);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "psi_separator_init",
             "Error opening hot-link to DD key");
      return FE_ERR_ODB;
   }

   info->bd = bd;
   info->hkey = hkey;
   info->lasterr_resettime = info->lasterrtime = ss_time();

   if (!bd)
      return FE_ERR_ODB;

   if (status != SUCCESS)
      return status;

   last_update = 0;
   last_set = 0;

   // read initial values
   status = psi_separator_rall(info);

   // remember update time
   last_update = ss_time();

   return status;
}

/*----------------------------------------------------------------------------*/

//! frees the memory allocated for the info structure
//! <pre>PSI_SEPARATOR_INFO</pre>.
//! \param info is a pointer to the DD specific info structure
INT psi_separator_exit(PSI_SEPARATOR_INFO * info)
{
   //free local variables
   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
//! reads all values (HV, current and vacuum)
//! back from the separator PC
//! \param info is a pointer to the DD specific info structure
INT psi_separator_rall(PSI_SEPARATOR_INFO * info)
{
   INT status, j;
   char str[STR_SIZE];
   DWORD nowtime, difftime;

   nowtime = ss_time();
   difftime = nowtime - info->lasterr_resettime;
   if (difftime > DELTA_TIME_ERROR) {
      info->errorcount = 0;
      info->lasterr_resettime = nowtime;
   }

   // only update once every 10 seconds
   if (nowtime - last_update < 10)
      return FE_SUCCESS;

   last_update = nowtime;

   /* Open connection. Note tha the bus driver allocates memory for the
      bd_info structure, which must be released manually later */
   status = info->bd(CMD_INIT, info->hkey, &info->bd_info);
   if (status != SUCCESS) {
      if (info->errorcount < MAX_ERROR) {
         cm_msg(MERROR, "psi_separator_rall", "Cannot connect to separator PC");
         info->errorcount++;
      }
      info->bd(CMD_EXIT, info->bd_info);
      free(info->bd_info);
      return FE_ERR_HW;
   }

   strcpy(str, "*RALL");
   status = info->bd(CMD_WRITE, info->bd_info, str, strlen(str) + 1);   // send string zero terminated
   if (status <= 0) {
      if (info->errorcount < MAX_ERROR) {
         cm_msg(MERROR, "psi_separator_rall",
                "Error sending *RALL to SEP pc");
         info->errorcount++;
      }
      info->bd(CMD_EXIT, info->bd_info);
      free(info->bd_info);
      return FE_ERR_HW;
   }

   strcpy(str, "0");
   while (strstr(str, "*RALL*") == NULL) {
      status = info->bd(CMD_GETS, info->bd_info, str, sizeof(str), "\n", 3000);
      
      if (strstr(str, "*RALL*") == NULL)
         ss_sleep(100);         //sleep 100ms before trying again
      
      if (ss_time() - last_update > 30) {
         if (info->errorcount < MAX_ERROR) {
            info->lasterrtime = nowtime;
            info->errorcount++;
         }

         if (info->errorcount < 2 * MAX_ERROR && (nowtime - info->lasterrtime) > 900) { //signal error after 15min
            cm_msg(MERROR, "psi_separator_rall", "Cannot RALL data.");   //this period is given by the
            cm_msg(MINFO, "psi_separator_rall", "str read = %s", str);  //watchdog on separator
            info->errorcount++;
         }

         info->bd(CMD_EXIT, info->bd_info);
         free(info->bd_info);
         return FE_ERR_HW;
      }
   }

   // get first string, should be "HVN <voltage> <current>"
   status = info->bd(CMD_GETS, info->bd_info, str, sizeof(str), "\n", 500);

   // skip name
   for (j = 0; j < (int) strlen(str) && str[j] != ' '; j++);

   // extract measured values
   info->hvmeasured = (float) atof(str + j + 1);
   for (j++; j < (int) strlen(str) && str[j] != ' '; j++);
   info->hvcurrent = (float) atof(str + j + 1);

   // get second string, should be "VAC <vacuum>"
   status = info->bd(CMD_GETS, info->bd_info, str, sizeof(str), "\n", 500);
   for (j = 0; j < (int) strlen(str) && str[j] != ' '; j++);
   info->sepvac = (float) atof(str + j + 1);

   // get status string
   status = info->bd(CMD_GETS, info->bd_info, str, sizeof(str), "\n", 500);

   if ((strstr(str, "OK") == NULL) && (strstr(str, "OVC") == NULL)) {
      if (info->errorcount < MAX_ERROR)
         cm_msg(MERROR, "separator_rall", "Error getting RALL status");
      info->errorcount++;
   }

   if (strstr(str, "OVC") != NULL) {
      if (info->hvcurrent > 0.9 * info->settings.max_current) {
         cm_msg(MTALK, "psi_separator_rall", "Separator in Overcurrent");
         cm_msg(MINFO, "psi_separator_rall",
#ifdef NUNK
                "Read values HVP, HVN, HVPC, HVNC, VAC = %f %f %f %f %10f",
                info->hvmeasuredpos, info->hvmeasuredneg,
                info->hvcurrentpos, info->hvcurrentneg, info->sepvac);
#endif
                "Read values HV, CUR, VAC = %f %f %10f",
                info->hvmeasured, info->hvmeasured, info->sepvac);
      }
   }
   info->bd(CMD_EXIT, info->bd_info);
   free(info->bd_info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
//! sends a new demand value to the corresponding separator PC
//! \param info is a pointer to the DD specific info structure
//! \param channel for which channel is this command
//! \param value to be set.
INT psi_separator_set(PSI_SEPARATOR_INFO * info, INT channel, float value)
{
   INT status;
   char str[STR_SIZE];

   if (channel == 0)            // channel 0: set HV
      info->hvdemand = value;
   else if (channel == 1)       // channel 1: set current limit
      info->settings.max_current = value;

   // force subsequent SET operations to have a interval of at least 5 sec; otherwise
   // the reading will hang
   if (ss_time() - last_set < 5) {
      cm_msg(MINFO, "psi_separator_set",
             "SET disregarded: Need at least 5 sec between subsequent SET operations");
      return FE_SUCCESS;
   }

   last_set = ss_time();

   status = info->bd(CMD_INIT, info->hkey, &info->bd_info);
   if (status != SUCCESS) {
      cm_msg(MERROR, "psi_separator_set",
             "Error initializing connection to separator host");
      info->bd(CMD_EXIT, info->bd_info);
      return FE_ERR_HW;
   }
   sprintf(str, "*HV %1.0lf %1.0lf", info->hvdemand, info->settings.max_current);

   status = info->bd(CMD_WRITE, info->bd_info, str, strlen(str) + 1);   // send string zero terminated
   if (status <= 0) {
      cm_msg(MERROR, "psi_separator_set", "Error sending %s to SEP pc",
             str);
      info->bd(CMD_EXIT, info->bd_info);
      return FE_ERR_HW;
   }

   status = info->bd(CMD_GETS, info->bd_info, str, sizeof(str), "\n", 3000);
   info->bd(CMD_EXIT, info->bd_info);

   if (strstr(str, "*HV *") == NULL) {
      cm_msg(MERROR, "psi_separator_set",
             "Didn't get *HV * acknowledgement after setting new HV");
      cm_msg(MINFO, "psi_separator_set", "returned str = %s", str);
      return FE_ERR_HW;
   }

   last_update = ss_time();     //to force wait before invoking the next read; if not done the
                                //communication is hanging and the slow control frontend has to be
                                //killed manually

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_separator_get(PSI_SEPARATOR_INFO * info, INT channel,
                      float *pvalue)
{
   INT status;

   status = psi_separator_rall(info);
   if (channel == 0)
      *pvalue = info->hvmeasured;
   else if (channel == 1)
      *pvalue = info->hvcurrent;
   else if (channel == 2)
      *pvalue = info->sepvac;

   return status;
}

/*----------------------------------------------------------------------------*/

INT psi_separator_get_demand(PSI_SEPARATOR_INFO * info, INT channel,
                             float *pvalue)
{
   if (channel == 0)
      *pvalue = info->hvdemand;
   else if (channel == 1)
      *pvalue = (float) info->settings.max_current;

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
//! at startup this routine allows to read the device names from the
//! beamline PC and to write the names into the ODB.
//! \param info is a pointer to the DD specific info structure
//! \param channel of the name to be set
//! \param name pointer to the ODB name
INT psi_separator_get_default_name(PSI_SEPARATOR_INFO * info, INT channel,
                                   char *name)
{
   if (channel == 0)
      strcpy(name, "Separator HV (kV)");
   else if (channel == 1)
      strcpy(name, "Separator current (uA)");
   else if (channel == 2)
      strcpy(name, "Separator vacuum (mbar)");

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT psi_separator_get_default_threshold(PSI_SEPARATOR_INFO * info,
                                        INT channel, float *pvalue)
{
   *pvalue = 0.5f;
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT psi_separator(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   DWORD flags;
   void *info, *bd;
   char *name;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      flags = va_arg(argptr, DWORD);
      bd = va_arg(argptr, void *);
      status = psi_separator_init(hKey, info, channel, bd);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      break;

   case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = psi_separator_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_separator_get(info, channel, pvalue);
      break;

   case CMD_GET_DEMAND:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_separator_get_demand(info, channel, pvalue);
      break;

   case CMD_GET_DEFAULT_NAME:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = psi_separator_get_default_name(info, channel, name);
      break;

   case CMD_GET_DEFAULT_THRESHOLD:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = psi_separator_get_default_threshold(info, channel, pvalue);
      break;

   case CMD_SET_LABEL:
      /* ignore ... */
      status = FE_SUCCESS;
      break;

   default:
      cm_msg(MERROR, "psi_separator", "Received unknown command %d", cmd);
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/

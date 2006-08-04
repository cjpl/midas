/********************************************************************\

Name:         dd_sy2527.c
Created by:   Pierre-Andr� Amaudruz

Contents:     SY2527 Device driver. 
Contains the bus call to the HW as it uses the
CAENHV Wrapper library for (XP & Linux).

$Id: dd_sy2527.c 2780 2005-10-19 13:20:29Z ritt $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"
#include "MainWrapp.h"
#include "CAENHVWrapper.h"

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */
#define SY2527_MAX_SLOTS   12

/* Store any parameters the device driver needs in following 
structure.  Edit the DDSY2527_SETTINGS_STR accordingly. This 
contains  usually the address of the device. For a CAMAC device
this  could be crate and station for example. */

typedef struct {
  char      Model[15];  // Model ID for the HV in this slot
  char      Name[32];   // System Name (duplication)
  INT       channels;   // # of channels from this HV
  float     *rup;       // Ramp up values for [channels]
  float     *svmax;     // Max Voltage for [channels]
  BOOL      *onoff;     // Channel ON/OFF for [channel] 
} DDSY2527_SLOT;

typedef struct {
  char   name[32];        // System name (sy2527)
  char   ip[32];          // IP# for network access
  int  linktype;          // Connection type (0:TCP/IP, 1:, 2:)
  int  begslot;           // First slot# belonging to this experiment
  int  endslot;           // Last slot# belonging to this experiment
} DDSY2527_SETTINGS; 

#define DDSY2527_SETTINGS_STR "\
System Name = STRING : [32] sy2527\n\
IP = STRING : [32] 142.90.111.74\n\
LinkType = INT : 0\n\
First Slot = INT : 0\n\
Last Slot = INT : 0\n\
"

/* following structure contains private variables to the device
driver. It  is necessary to store it here in case the device
driver  is used for more than one device in one frontend. If it
would be  stored in a global variable, one device could over-
write the other device's  variables. */

typedef struct {
  DDSY2527_SETTINGS dd_sy2527_settings;
  DDSY2527_SLOT slot[SY2527_MAX_SLOTS];
  float *array;
  INT num_channels;           // Total # of channels
  INT(*bd) (INT cmd, ...);    /* bus driver entry function */
  void  *bd_info;               /* private info of bus driver */
  HNDLE hkey;                 /* ODB key for bus driver info */
} DDSY2527_INFO;

INT dd_sy2527_name_get(DDSY2527_INFO * info, INT channel, char * label);
INT dd_sy2527_lParam_get(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, DWORD *lvalue);
INT dd_sy2527_lParam_set(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, DWORD *lvalue);
INT dd_sy2527_Name_get(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char (*listName)[MAX_CH_NAME]);
INT dd_sy2527_Name_set(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *chName);
INT dd_sy2527_fParam_get(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, float *fvalue);
INT dd_sy2527_fParam_set(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, float *fvalue);

/*---- device driver routines --------------------------------------*/

/* the init function creates a ODB record which contains the
settings  and initialized it variables as well as the bus driver */

INT dd_sy2527_init(HNDLE hkey, void **pinfo, INT channels, INT(*bd) (INT cmd, ...))
{
  int status, size, ret,  islot;
  HNDLE hDB,  hkeydd;
  DDSY2527_INFO *info;
  char  keyloc[128], str[128], username[30], passwd[30];
//  char   listName[32][MAX_CH_NAME];
  unsigned  short NrOfCh, serNumb;
  unsigned  char fmwMax, fmwMin;
  char  model[15], descr[80], *DevName;
  HNDLE shkey;

  /*  allocate info structure */
  info  = calloc(1, sizeof(DDSY2527_INFO));
  *pinfo  = info;

  cm_get_experiment_database(&hDB,  NULL);

  /*  create DDSY2527 settings record */
  status  = db_create_record(hDB, hkey, "DD", DDSY2527_SETTINGS_STR);
  if  (status != DB_SUCCESS)
    return FE_ERR_ODB;

  db_find_key(hDB,  hkey, "DD", &hkeydd);
  size  = sizeof(info->dd_sy2527_settings);
  ret = db_get_record(hDB,  hkeydd, &info->dd_sy2527_settings, &size, 0);

  //  Connect to device
  strcpy(username,  "admin");
  strcpy(passwd,  "admin");
  DevName = info->dd_sy2527_settings.name;
  ret = CAENHVInitSystem(DevName, info->dd_sy2527_settings.linktype
    , info->dd_sy2527_settings.ip,  username, passwd);
  cm_msg(MINFO,"dd_sy2527","CAENHVInitSystem: %s  (num. %d)", CAENHVGetError(DevName), ret);

  //  Retrieve slot table for channels construction
  for (channels = 0,  islot=info->dd_sy2527_settings.begslot; islot<info->dd_sy2527_settings.endslot+1; islot++) {
    ret = CAENHVTestBdPresence(DevName, islot, &NrOfCh, model, descr, &serNumb, &fmwMin, &fmwMax);
    if( ret == CAENHV_OK ) {
      sprintf(str,"Slot %d: Mod. %s %s Nr.Ch: %d  Ser. %d Rel. %d.%d\n"
        ,islot, model,descr,NrOfCh, serNumb,fmwMax,fmwMin);
      sprintf(keyloc, "Slot %i",  islot);

      //  Check for existance of the Slot
      if  (db_find_key(hDB, hkey, keyloc, &shkey) == DB_SUCCESS) {       
        size  = sizeof(info->slot[islot].Model);
        db_get_value(hDB, shkey,  "Model", info->slot[islot].Model, &size, TID_STRING, FALSE);
        //   Check for correct Model in that slot
        if  ((strcmp(info->slot[islot].Model, model)) == 0) {
          // device model found in ODB use ODB settings
          info->slot[islot].channels  = NrOfCh;
          info->slot[islot].rup = calloc(NrOfCh,  sizeof(float));
          size  = NrOfCh * sizeof(float);
          status  = db_get_value(hDB, shkey, "Rup", info->slot[islot].rup, &size, TID_FLOAT,FALSE);          

          info->slot[islot].svmax = calloc(NrOfCh,  sizeof(float));
          size  = NrOfCh * sizeof(float);
          status  = db_get_value(hDB, shkey, "SVMax", info->slot[islot].svmax, &size, TID_FLOAT, FALSE);

          info->slot[islot].onoff  = calloc(NrOfCh, sizeof(BOOL));
          size  = NrOfCh * sizeof(BOOL);
          status  = db_get_value(hDB, shkey, "On", info->slot[islot].onoff, &size, TID_BOOL, FALSE);   
          channels  += NrOfCh;
          continue;
        } else  {
          //  Wrong Model, delete and Create new one
          db_delete_key(hDB,  shkey, FALSE);
        }
      }
      // No Slot entry in ODB, read defaults from device
      cm_msg(MINFO,"dd_sy2527",str);
      sprintf(keyloc,"Slot %i/Description", islot);
      db_set_value(hDB, hkey, keyloc, descr, sizeof(str), 1, TID_STRING);

      sprintf(keyloc,"Slot %i/Name", islot);
      db_set_value(hDB, hkey, keyloc, DevName, sizeof(str), 1, TID_STRING);
      strcpy(info->slot[islot].Name, DevName);

      sprintf(keyloc,"Slot %i/Model", islot);
      db_set_value(hDB, hkey, keyloc, model, sizeof(model), 1, TID_STRING);
      strcpy(info->slot[islot].Model, model);

      sprintf(keyloc,"Slot %i/Channels", islot);
      info->slot[islot].channels = NrOfCh;
      db_set_value(hDB, hkey, keyloc, &NrOfCh, sizeof(WORD), 1, TID_WORD);

      channels += NrOfCh;
    }
  }

  /* initialize driver */
  info->num_channels  = channels;
  info->array = calloc(channels,  sizeof(float));
  info->hkey  = hkey;

  return  FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_exit(DDSY2527_INFO * info)
{
  /*  free local variables */
  if  (info->array)
    free(info->array);

  free(info);

  return  FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
void get_slot(DDSY2527_INFO * info, WORD channel, WORD * chan, WORD * slot)
{
  *slot = *chan = 0;
  do {
    if (channel < info->slot[*slot].channels) {
      *chan = channel;
      return ;
    }
    channel -= info->slot[*slot].channels;
    *slot +=1;
  } while (*slot < SY2527_MAX_SLOTS); 
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_name_get(DDSY2527_INFO * info, INT channel, char * label)
{
  WORD nchannel;
  
  nchannel = 1;
  dd_sy2527_Name_get(info, nchannel, (WORD *)&channel, label);  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_Name_get(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *chName)
{
  WORD    ch, islot;
  CAENHVRESULT  ret;

  // Find out what slot we need to talk to.
  get_slot(info, *chlist, &ch, &islot);
  chlist[0] = ch;
  if (*chlist > 12)
    printf("hello");
  ret = CAENHVGetChName(info->dd_sy2527_settings.name, islot, nchannel, chlist, chName);
  if (ret != CAENHV_OK) { 
//    cm_msg(MINFO,"dd_sy2527","Get Name - Nch:%i Name: %s", nchannel, chName);
    cm_msg(MERROR,  "Name Set", "GetChName returns %d", ret);
  }
  return ret;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_name_set(DDSY2527_INFO * info, INT channel, char * label)
{
  WORD nchannel;

  nchannel = 1;

  dd_sy2527_Name_set(info, nchannel, (WORD*)(&channel), &label);  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_Name_set(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *chName)
{
  WORD      islot, ch;
  CAENHVRESULT  ret;

  // Find out what slot we need to talk to.
  get_slot(info, *chlist, &ch, &islot);
  chlist[0] = ch;
  ret = CAENHVSetChName(info->dd_sy2527_settings.name, islot, nchannel, chlist, chName);
  if (ret != CAENHV_OK) {
    //    cm_msg(MINFO,"dd_sy2527","Set Name - Nch:%i Name: %s", nchannel, chName);
    cm_msg(MERROR,  "Name Set", "SetChName returns %d", ret);
  }
  return ret;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_lParam_get(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, DWORD *lvalue)
{
  WORD      islot, ch;
  DWORD     tipo;
  CAENHVRESULT  ret;

  // Find out what slot we need to talk to.
  get_slot(info, *chlist, &ch, &islot);
  chlist[0] = ch;
  ret = CAENHVGetChParamProp(info->dd_sy2527_settings.name, islot, chlist[0], ParName, "Type", &tipo);
  if (ret != CAENHV_OK) cm_msg(MERROR,"lParam_get", "Type : %d", tipo);
  if ((ret == CAENHV_OK) && (tipo != PARAM_TYPE_NUMERIC)) {
    ret = CAENHVGetChParam(info->dd_sy2527_settings.name, islot,  ParName, nchannel, chlist, lvalue);
    if  (ret != CAENHV_OK)
      cm_msg(MERROR,  "lParam", "GetChlParam returns %d", ret);
  }
  return ret;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_lParam_set(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, DWORD *lvalue)
{
  WORD      islot, ch;
  DWORD     tipo;
  CAENHVRESULT  ret;

  // Find out what slot we need to talk to.
  get_slot(info, *chlist, &ch, &islot);
  chlist[0] = ch;
  ret = CAENHVGetChParamProp(info->dd_sy2527_settings.name, islot, chlist[0], ParName, "Type", &tipo);
  if (ret != CAENHV_OK) cm_msg(MERROR,"lParam_get", "Type : %d", tipo);
  if ((ret == CAENHV_OK) && (tipo != PARAM_TYPE_NUMERIC)) {
    ret = CAENHVSetChParam(info->dd_sy2527_settings.name, islot,  ParName, nchannel, chlist, lvalue);
    if  (ret != CAENHV_OK)
      // cm_msg(MINFO,"dd_sy2527","Set lParam - chNum:%i Value: %ld %ld %ld", nchannel, lvalue[0], lvalue[1], lvalue[2]);
      cm_msg(MERROR,  "lParam", "SetChlParam returns %d", ret);
  }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_fParam_get(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, float *fvalue)
{
  WORD      islot, ch;
  DWORD     tipo;
  CAENHVRESULT  ret;

  // Find out what slot we need to talk to.
  get_slot(info, *chlist, &ch, &islot);
  chlist[0] = ch;
  ret = CAENHVGetChParamProp(info->dd_sy2527_settings.name, islot, chlist[0], ParName, "Type", &tipo);
  if (ret != CAENHV_OK) {
    cm_msg(MERROR,"fParam_get", "Param Not Found Type : %d", tipo);
    return ret;
  }
  if ((ret == CAENHV_OK) && (tipo == PARAM_TYPE_NUMERIC)) {
    ret = CAENHVGetChParam(info->dd_sy2527_settings.name, islot,  ParName, nchannel, chlist, fvalue);
    if  (ret != CAENHV_OK)
      //         cm_msg(MINFO,"dd_sy2527","Get fParam - chNum:%i Value: %f %f %f", nchannel, fvalue[0], fvalue[1], fvalue[2]);
      cm_msg(MERROR,  "fParam", "GetChfParam returns %d", ret);
  }
    return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_fParam_set(DDSY2527_INFO * info, WORD nchannel, WORD *chlist, char *ParName, float *fvalue)
{
  WORD      islot, ch;
  DWORD     tipo;
  CAENHVRESULT  ret;

  // Find out what slot we need to talk to.
  get_slot(info, *chlist, &ch, &islot);
  chlist[0] = ch;
  ret = CAENHVGetChParamProp(info->dd_sy2527_settings.name, islot, chlist[0], ParName, "Type", &tipo);
  if (ret != CAENHV_OK) {
    cm_msg(MERROR,"fParam_get", "Param Not Found Type : %d", tipo);
    return ret;
  }
  if ((ret == CAENHV_OK) && (tipo == PARAM_TYPE_NUMERIC)) {
    ret = CAENHVSetChParam(info->dd_sy2527_settings.name, islot, ParName, nchannel, chlist, fvalue);
    if  (ret != CAENHV_OK) {
      //          cm_msg(MINFO,"dd_sy2527","Set fParam - chNum:%i Value: %f %f %f", nchannel, fvalue[0], fvalue[1], fvalue[2]);
      cm_msg(MERROR,  "fParam", "SetChfParam returns %d", ret);
    }
  }
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
INT dd_sy2527_Label_set(DDSY2527_INFO * info, int channel, char *label) {
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
  if (strlen(label) < MAX_CH_NAME) {
    ret = dd_sy2527_Name_set(info, 1, &ch, label);
    return FE_SUCCESS;
  } else
    return 0;
} 

/*----------------------------------------------------------------------------*/
INT dd_sy2527_get(DDSY2527_INFO * info, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
  ret = dd_sy2527_fParam_get(info, 1, &ch, "VMon",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_get_all(DDSY2527_INFO * info, INT channels, float *pvalue)
{
  return  FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_demand_get(DDSY2527_INFO * info, INT channel, float *value)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
  ret = dd_sy2527_fParam_get(info, 1, &ch, "V0Set", value);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_set(DDSY2527_INFO * info, INT channel, float value)
{
  WORD ch;
  DWORD  temp;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
  if (value < 0.01) {
    // Set value
    ret = dd_sy2527_fParam_set(info, 1, &ch, "V0Set", &value);
    // switch off this channel
    temp = 0;
    ret = dd_sy2527_lParam_set(info, 1, &ch, "Pw", &temp);
  } else {
    // switch on this channel
    temp = 1;
    ret = dd_sy2527_lParam_set(info, 1, &ch, "Pw", &temp);
    // Set Value
    ret = dd_sy2527_fParam_set(info, 1, &ch, "V0Set", &value);
  }
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_set_all(DDSY2527_INFO * info, INT channels, float *value)
{
  return  FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_current_get(DDSY2527_INFO * info, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
  ret = dd_sy2527_fParam_get(info, 1, &ch, "IMon",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_ramp_get(DDSY2527_INFO * info, INT cmd, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
  if (cmd == CMD_GET_RAMPUP)
    ret = dd_sy2527_fParam_get(info, 1, &ch, "RUp",  pvalue);
  if (cmd == CMD_GET_RAMPDOWN)
    ret = dd_sy2527_fParam_get(info, 1, &ch, "RDWn",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_ramp_set(DDSY2527_INFO * info, INT cmd, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
  if (cmd == CMD_SET_RAMPUP)
    ret = dd_sy2527_fParam_set(info, 1, &ch, "RUp",  pvalue);
  if (cmd == CMD_SET_RAMPDOWN)
    ret = dd_sy2527_fParam_set(info, 1, &ch, "RDWn",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_current_limit_get(DDSY2527_INFO * info, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
    ret = dd_sy2527_fParam_get(info, 1, &ch, "I0Set",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_current_limit_set(DDSY2527_INFO * info, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
    ret = dd_sy2527_fParam_set(info, 1, &ch, "I0Set",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_voltage_limit_get(DDSY2527_INFO * info, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
    ret = dd_sy2527_fParam_get(info, 1, &ch, "SVMax",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_voltage_limit_set(DDSY2527_INFO * info, INT channel, float *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
    ret = dd_sy2527_fParam_set(info, 1, &ch, "SVMax",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_onoff_get(DDSY2527_INFO * info, INT channel, BOOL *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
    ret = dd_sy2527_lParam_get(info, 1, &ch, "Pw",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*----------------------------------------------------------------------------*/
INT dd_sy2527_onoff_set(DDSY2527_INFO * info, INT channel, BOOL *pvalue)
{
  WORD ch;
  CAENHVRESULT  ret;

  ch = (WORD) channel;
    ret = dd_sy2527_lParam_set(info, 1, &ch, "Pw",  pvalue);
  return ret == 0 ? FE_SUCCESS : 0;
}

/*---- device driver entry point -----------------------------------*/
INT dd_sy2527(INT cmd, ...)
{
  va_list argptr;
  HNDLE hKey;
  INT channel,  status, icmd;
  DWORD flags;
  char  *label;
  float value,  *pvalue;
  void  *info, *bd;

  va_start(argptr,  cmd);
  status  = FE_SUCCESS;

  switch  (cmd) {
   case CMD_INIT:
     hKey = va_arg(argptr,  HNDLE);
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     flags  = va_arg(argptr, DWORD);
     bd = va_arg(argptr,  void *);
     status = dd_sy2527_init(hKey,  info, channel, bd);
     break;

   case CMD_EXIT:
     info = va_arg(argptr,  void *);
     status = dd_sy2527_exit(info);
     break;

   case CMD_SET:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     value  = (float) va_arg(argptr, double);   // floats are passed as double
     status = dd_sy2527_set(info, channel,  value);
     break;

   case CMD_SET_ALL:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = (float  *) va_arg(argptr, float *);
     status = dd_sy2527_set_all(info, channel,  pvalue);
     break;

   case CMD_SET_LABEL:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     label  = (char *) va_arg(argptr, char *);
     status = dd_sy2527_Label_set(info, channel,  label);
     break;

   case CMD_GET:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_get(info, channel,  pvalue);
     break;

   case CMD_GET_CURRENT:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_current_get(info, channel,  pvalue);
     break;

   case CMD_GET_RAMPUP:
   case CMD_GET_RAMPDOWN:
     info = va_arg(argptr,  void *);
     icmd  = cmd;
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_ramp_get(info, icmd, channel,  pvalue);
     break;

   case CMD_SET_RAMPUP:
   case CMD_SET_RAMPDOWN:
     info = va_arg(argptr,  void *);
     icmd  = cmd;
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_ramp_set(info, icmd, channel,  pvalue);
     break;

   case CMD_SET_CURRENT_LIMIT:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = (float *)va_arg(argptr,  float *);
     status = dd_sy2527_current_limit_set(info, channel, pvalue);
     break;

   case CMD_GET_CURRENT_LIMIT:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = (float *)va_arg(argptr,  float *);
     status = dd_sy2527_current_limit_get(info, channel, pvalue);
     break;

   case CMD_SET_VOLTAGE_LIMIT:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_voltage_limit_set(info, channel, pvalue);
     break;

   case CMD_GET_DEMAND:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_demand_get(info, channel, pvalue);
     break;

   case CMD_GET_VOLTAGE_LIMIT:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_voltage_limit_get(info, channel, pvalue);
     break;

   //case CMD_SET_ONOFF:
   //  info = va_arg(argptr,  void *);
   //  channel  = va_arg(argptr, INT);
   //  pbvalue = va_arg(argptr,  BOOL *);
   //  status = dd_sy2527_onoff_set(info, channel, pbvalue);
   //  break;

   //case CMD_GET_ONOFF:
   //  info = va_arg(argptr,  void *);
   //  channel  = va_arg(argptr, INT);
   //  pbvalue = va_arg(argptr,  BOOL *);
   //  status = dd_sy2527_onoff_get(info, channel, pbvalue);
   //  break;

   case CMD_GET_DEFAULT_NAME:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     label  = (char *) va_arg(argptr, char *);
     status = dd_sy2527_name_get(info, channel, label);
     break;

   case CMD_SET_DEFAULT_NAME:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     label  = (char *) va_arg(argptr, char *);
     status = dd_sy2527_name_set(info, channel, label);
     break;

   case CMD_GET_ALL:
     info = va_arg(argptr,  void *);
     channel  = va_arg(argptr, INT);
     pvalue = va_arg(argptr,  float *);
     status = dd_sy2527_get_all(info, channel, pvalue);
     break;

   default:
     break;
  }

  va_end(argptr);

  return  status;
}

/*------------------------------------------------------------------*/
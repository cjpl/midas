/********************************************************************\

  Name:         null.c
  Created by:   Stefan Ritt

  Contents:     NULL Device Driver

  $Log$
  Revision 1.2  1998/10/12 12:18:57  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "midas.h"

/*---- globals -----------------------------------------------------*/

typedef struct {
  char      port[32];
} NULL_SETTINGS;

#define NULL_SETTINGS_STR "\
Port = STRING : [32] Nowhere\n\
"
typedef struct {
  NULL_SETTINGS null_settings;
  float         *array;
  INT           num_channels;
} NULL_INFO;

/*---- device driver routines --------------------------------------*/

INT null_init(HNDLE hKey, void **pinfo, INT channels)
{
int       status, size;
HNDLE     hDB;
NULL_INFO *info;

  /* allocate info structure */
  info = calloc(1, sizeof(NULL_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  /* create NULL settings record */
  status = db_create_record(hDB, hKey, "", NULL_SETTINGS_STR);
  if (status != DB_SUCCESS)
    return FE_ERR_ODB;

  size = sizeof(info->null_settings);
  db_get_record(hDB, hKey, &info->null_settings, &size, 0);

  /* initialize driver */
  info->num_channels = channels;
  info->array = calloc(channels, sizeof(float));

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT null_exit(NULL_INFO *info)
{
  if (info->array)
    free(info->array);

  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT null_set(NULL_INFO *info, INT channel, float value)
{
  if (channel < info->num_channels)
    info->array[channel] = value;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT null_set_all(NULL_INFO *info, INT channels, float value)
{
INT i;

  for (i=0 ; i<min(info->num_channels, channels) ; i++)
    info->array[i] = value;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT null_get(NULL_INFO *info, INT channel, float *pvalue)
{
  if (channel < info->num_channels)
    *pvalue = info->array[channel];
  else
    *pvalue = 0.f;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT null_get_all(NULL_INFO *info, INT channels, float *pvalue)
{
int   i;

  for (i=0 ; i<min(info->num_channels, channels) ; i++)
    pvalue[i] = info->array[i];

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT null(INT cmd, ...)
{
va_list argptr;
HNDLE   hKey;
INT     channel, status;
float   value, *pvalue;
void    *info;

  va_start(argptr, cmd);
  status = FE_SUCCESS;

  switch (cmd)
    {
    case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      status = null_init(hKey, info, channel);
      break;

    case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = null_exit(info);
      break;

    case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = null_set(info, channel, value);
      break;

    case CMD_SET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = null_set_all(info, channel, value);
      break;

    case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = null_get(info, channel, pvalue);
      break;
    
    case CMD_GET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = null_get_all(info, channel, pvalue);
      break;
    
    default:
      break;
    }

  va_end(argptr);

  return status;
}

/*------------------------------------------------------------------*/

/********************************************************************\

  Name:         mscbdev.c
  Created by:   Stefan Ritt

  Contents:     MSCB Device Driver.

  $Log$
  Revision 1.1  2003/05/12 10:30:16  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "midas.h"
#include "../mscb/mscb.h"

/*---- globals -----------------------------------------------------*/

/* MSCB node address / channel mapping */

typedef struct {
  char          device[256];
  int           *mscb_address;
  unsigned char *mscb_index;
  int           *var_size;
} MSCBDEV_SETTINGS;

typedef struct {
  MSCBDEV_SETTINGS mscbdev_settings;
  INT              fd;
  INT              num_channels;
} MSCBDEV_INFO;

/*---- device driver routines --------------------------------------*/

/* the init function creates a ODB record which contains the
   settings and initialized it variables as well as the bus driver */

INT mscbdev_init(HNDLE hkey, void **pinfo, INT channels, INT (*bd)(INT cmd, ...))
{
int           i, status, size;
HNDLE         hDB, hsubkey;
MSCBDEV_INFO  *info;   
MSCB_INFO_VAR var_info;

  /* allocate info structure */
  info = calloc(1, sizeof(MSCBDEV_INFO));
  *pinfo = info;
  info->mscbdev_settings.mscb_address = calloc(channels, sizeof(INT));
  info->mscbdev_settings.mscb_index   = calloc(channels, sizeof(INT));
  info->mscbdev_settings.var_size     = calloc(channels, sizeof(INT));

  cm_get_experiment_database(&hDB, NULL);

  /* create settings record */
  size = sizeof(info->mscbdev_settings.device);
  strcpy(info->mscbdev_settings.device, DEF_DEVICE);

  status = db_get_value(hDB, hkey, "Device", &info->mscbdev_settings.device, &size, TID_STRING, TRUE);
  if (status != DB_SUCCESS)
    return FE_ERR_ODB;

  size = sizeof(INT) * channels;
  db_get_value(hDB, hkey, "MSCB Address", info->mscbdev_settings.mscb_address, &size, TID_INT, TRUE);
  db_find_key(hDB, hkey, "MSCB Address", &hsubkey);
  size = sizeof(INT) * channels;
  db_set_data(hDB, hsubkey, info->mscbdev_settings.mscb_address, size, channels, TID_INT);
  db_open_record(hDB, hsubkey, info->mscbdev_settings.mscb_address, size, MODE_READ, NULL, NULL);

  size = sizeof(BYTE) * channels;
  db_get_value(hDB, hkey, "MSCB Index", info->mscbdev_settings.mscb_index, &size, TID_BYTE, TRUE);
  db_find_key(hDB, hkey, "MSCB Index", &hsubkey);
  size = sizeof(BYTE) * channels;
  db_set_data(hDB, hsubkey, info->mscbdev_settings.mscb_index, size, channels, TID_BYTE);
  db_open_record(hDB, hsubkey, info->mscbdev_settings.mscb_index, size, MODE_READ, NULL, NULL);

  size = sizeof(INT) * channels;
  db_get_value(hDB, hkey, "Var Size", info->mscbdev_settings.var_size, &size, TID_INT, TRUE);
  db_find_key(hDB, hkey, "Var Size", &hsubkey);
  size = sizeof(INT) * channels;
  db_set_data(hDB, hsubkey, info->mscbdev_settings.var_size, size, channels, TID_INT);
  db_open_record(hDB, hsubkey, info->mscbdev_settings.var_size, size, MODE_READ, NULL, NULL);

  /* initialize info structure */
  info->num_channels = channels;

  info->fd = mscb_init(info->mscbdev_settings.device, FALSE);
  if (info->fd < 0)
    return -1;

  /* get info about MSCB channels */
  for (i=0 ; i<channels ; i++)
    {
    status = mscb_info_variable(info->fd, info->mscbdev_settings.mscb_address[i], 
                                info->mscbdev_settings.mscb_index[i], &var_info);
    if (status == MSCB_SUCCESS)
      {
      if (var_info.flags & MSCBF_FLOAT)
        info->mscbdev_settings.var_size[i] = -1;
      else
        info->mscbdev_settings.var_size[i] = var_info.width;
      }
    else
      info->mscbdev_settings.var_size[i] = 0;
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_exit(MSCBDEV_INFO *info)
{
  mscb_exit(info->fd);

  /* free info structure */
  if (info)
    {
    free(info->mscbdev_settings.mscb_address);
    free(info->mscbdev_settings.mscb_index);
    free(info->mscbdev_settings.var_size);
    free(info);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_set(MSCBDEV_INFO *info, INT channel, float value)
{
INT value_int;

  if (info->mscbdev_settings.var_size[channel] == -1)
    {
    /* channel is "float" */
    mscb_write(info->fd, info->mscbdev_settings.mscb_address[channel],
               info->mscbdev_settings.mscb_index[channel], &value, sizeof(float));
    }
  else
    {
    /* channel is int */
    value_int = (INT) value;
    mscb_write(info->fd, info->mscbdev_settings.mscb_address[channel],
               info->mscbdev_settings.mscb_index[channel], &value_int,
               info->mscbdev_settings.var_size[channel]);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_get(MSCBDEV_INFO *info, INT channel, float *pvalue)
{
INT size;

  size = sizeof(float);
  mscb_read(info->fd, info->mscbdev_settings.mscb_address[channel],
               info->mscbdev_settings.mscb_index[channel], pvalue, &size);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbdev_get_default_name(MSCBDEV_INFO *info, INT channel, char *name)
{
int status;
MSCB_INFO_VAR var_info;

  status = mscb_info_variable(info->fd, info->mscbdev_settings.mscb_address[channel], 
                              info->mscbdev_settings.mscb_index[channel], &var_info);
  if (status == MSCB_SUCCESS)
    strcpy(name, var_info.name);

  return status;
}

/*---- device driver entry point -----------------------------------*/

INT mscbdev(INT cmd, ...)
{
va_list argptr;
HNDLE   hKey;
INT     channel, status;
DWORD   flags;
float   value, *pvalue;
void    *info, *bd;
char    *name;

  va_start(argptr, cmd);
  status = FE_SUCCESS;

  switch (cmd)
    {
    case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      flags = va_arg(argptr, DWORD);
      bd = va_arg(argptr, void *);
      status = mscbdev_init(hKey, info, channel, bd);
      break;

    case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbdev_exit(info);
      break;

    case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double); // floats are passed as double
      status = mscbdev_set(info, channel, value);
      break;

    case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = mscbdev_get(info, channel, pvalue);
      break;
    
    case CMD_GET_DEFAULT_NAME:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = mscbdev_get_default_name(info, channel, name);
      break;

    default:
      break;
    }

  va_end(argptr);

  return status;
}

/*------------------------------------------------------------------*/

/********************************************************************\

  Name:         chn_acc.c
  Created by:   Stefan Ritt

  Contents:     Epics channel access device driver

  $Log$
  Revision 1.2  1999/09/21 13:48:40  midas
  Fixed compiler warning

  Revision 1.1  1999/08/31 15:16:26  midas
  Added file

  Revision 1.2  1998/10/12 12:18:57  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "cadef.h"
#include <stdarg.h>
#include "midas.h"

/*---- globals -----------------------------------------------------*/

#define CHN_NAME_LENGTH 32 /* length of channel names */

typedef struct {
  char          *channel_names;
  chid          *pv_handles;
  float         *array;
  INT           num_channels;
} CA_INFO;

/*---- device driver routines --------------------------------------*/

INT chn_acc_init(HNDLE hKey, void **pinfo, INT channels)
{
int       status, i;
HNDLE     hDB;
CA_INFO   *info;

  /* allocate info structure */
  info = calloc(1, sizeof(CA_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  /* get channel names */
  info->channel_names = malloc(channels*CHN_NAME_LENGTH);
  for (i=0 ; i<channels ; i++)
    sprintf(info->channel_names+CHN_NAME_LENGTH*i, "Channel%d", i);
  db_merge_data(hDB, hKey, "Channel name", 
                info->channel_names, CHN_NAME_LENGTH*channels, 
                channels, TID_STRING);

  /* initialize driver */
	status = ca_task_initialize();
  if (!(status & CA_M_SUCCESS))
    {
    cm_msg(MERROR, "chn_acc_init", "unable to initialize");
    return FE_ERR_HW;
    }

  info->num_channels = channels;
  info->pv_handles = malloc(channels*sizeof(chid));
  for (i=0 ; i<channels ; i++)
    {
  	status = ca_search(info->channel_names+CHN_NAME_LENGTH*i, &info->pv_handles[i]);
    if (!(status & CA_M_SUCCESS))
      cm_msg(MERROR, "chn_acc_init", "cannot find tag %s", info->channel_names+CHN_NAME_LENGTH*i);
    }

  if (ca_pend_io(5.0) == ECA_TIMEOUT)
	  {
		for (i=0; i < channels; i++)
			if (info->pv_handles[i]->state != 2)
        cm_msg(MERROR, "chn_acc_init", "cannot connect to %s", 
               info->channel_names+CHN_NAME_LENGTH*i);
	  }

  info->array = calloc(channels, sizeof(float));

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT chn_acc_exit(CA_INFO *info)
{
  if (info->array)
    free(info->array);
  if (info->channel_names)
    free(info->channel_names);

  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT chn_acc_set(CA_INFO *info, INT channel, float value)
{
  ca_array_put(DBR_FLOAT, 1, info->pv_handles[channel], &value);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT chn_acc_set_all(CA_INFO *info, INT channels, float value)
{
INT i;

  for (i=0 ; i<min(info->num_channels, channels) ; i++)
  	ca_array_put(DBR_FLOAT, 1, info->pv_handles[i], &value);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT chn_acc_get(CA_INFO *info, INT channel, float *pvalue)
{
	ca_array_get(DBR_FLOAT, 1L, info->pv_handles[channel], pvalue);
	if (ca_pend_io(1.0) == ECA_TIMEOUT) 
    cm_msg(MERROR, "chn_acc_get", "got timeout");
  else
    info->array[channel] = *pvalue;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT chn_acc_get_all(CA_INFO *info, INT channels, float *pvalue)
{
int   i;

  for (i=0 ; i<min(info->num_channels, channels) ; i++)
    chn_acc_get(info, i, pvalue+i);

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT chn_acc(INT cmd, ...)
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
      status = chn_acc_init(hKey, info, channel);
      break;

    case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = chn_acc_exit(info);
      break;

    case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = chn_acc_set(info, channel, value);
      break;

    case CMD_SET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = chn_acc_set_all(info, channel, value);
      break;

    case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = chn_acc_get(info, channel, pvalue);
      break;
    
    case CMD_GET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = chn_acc_get_all(info, channel, pvalue);
      break;
    
    default:
      break;
    }

  va_end(argptr);

  return status;
}

/*------------------------------------------------------------------*/

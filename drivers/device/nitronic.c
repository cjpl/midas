/********************************************************************\

  Name:         nitronic.c
  Created by:   Stefan Ritt

  Contents:     Nitronic HVS 132 High Voltage Device Driver

  $Log$
  Revision 1.2  2000/10/19 13:00:42  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "midas.h"
#include "bus\rs232.h"

/*---- globals -----------------------------------------------------*/

int rs232_hdev[2];

typedef struct {
  char rs232_port[NAME_LENGTH];
  int  address;
} NITRONIC_SETTINGS;

#define NITRONIC_SETTINGS_STR "\
RS232 Port = STRING : [32] com1\n\
Address = INT : 0\n\
"

typedef struct {
  NITRONIC_SETTINGS settings;
  int num_channels;
  int hdev;                            /* device handle for RS232 device */
} NITRONIC_INFO;

/*---- device driver routines --------------------------------------*/

void nitronic_switch(NITRONIC_INFO *info)
{
static INT last_address=-1;
char str[80];
INT  status;

  if (info->settings.address != last_address)
    {
    sprintf(str, "M%02d", info->settings.address);
    rs232_puts(info->hdev, str);
    status = rs232_waitfor(info->hdev, "\r+", str, 80, 5);
    if (!status)
      {
      cm_msg(MERROR, "nitronic_init", 
        "NITRONIC adr %d doesn't respond. Check power and RS232 connection.", 
        info->settings.address);
      return;
      }

    last_address = info->settings.address;
    }
}

/*------------------------------------------------------------------*/

INT nitronic_init(HNDLE hKey, void **pinfo, INT channels)
{
int          status, size, no;
char         str[256];
HNDLE        hDB;
NITRONIC_INFO *info;

  /* allocate info structure */
  info = calloc(1, sizeof(NITRONIC_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  /* create NITRONIC settings record */
  status = db_create_record(hDB, hKey, "", NITRONIC_SETTINGS_STR);
  if (status != DB_SUCCESS)
    return FE_ERR_ODB;

  size = sizeof(info->settings);
  db_get_record(hDB, hKey, &info->settings, &size, 0);

  info->num_channels = channels;

  /* initialize RS232 port */
  no = info->settings.rs232_port[3] - '0';
  if (no < 1)
    no = 1;
  if (no > 2)
    no = 2;

  if (rs232_hdev[no-1])
    info->hdev = rs232_hdev[no-1];
  else
    {
    info->hdev = rs232_init(info->settings.rs232_port, 9600, 'N', 8, 1, 0);
    if (info->hdev < 0)
      {
      cm_msg(MERROR, "nitronic_init", "Cannot access port \"%s\"", info->settings.rs232_port);
      return FE_ERR_HW;
      }

    rs232_hdev[no-1] = info->hdev;
    }

  rs232_debug(FALSE);

  /* check if module is living  */
  sprintf(str, "M%02d", info->settings.address);
  rs232_puts(info->hdev, str);
  status = rs232_waitfor(info->hdev, "\r+", str, 80, 2);
  if (!status)
    {
    cm_msg(MERROR, "nitronic_init", "NITRONIC adr %d doesn't respond. Check power and RS232 connection.", info->settings.address);
    return FE_ERR_HW;
    }

  /* turn on HV main switch */
  rs232_puts(info->hdev, "ONA");
  rs232_waitfor(info->hdev, "\r+", str, 80, 5);

  rs232_puts(info->hdev, "M16\r\n");

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_exit(NITRONIC_INFO *info)
{
  rs232_exit(info->hdev);
  
  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_set(NITRONIC_INFO *info, INT channel, float value)
{
char  str[80];

  nitronic_switch(info);

  sprintf(str, "UCH%02d%04.0fL", channel+1, value);

  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "+", str, 80, 1);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_set_all(NITRONIC_INFO *info, INT channels, float value)
{
char  str[80];
INT   i;

  nitronic_switch(info);

  for (i=0 ; i<channels ; i++)
    {
    sprintf(str, "UCH%02d%04.0fL", i+1, value);
    rs232_puts(info->hdev, str);
    rs232_waitfor(info->hdev, "+", str, 80, 3);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_get(NITRONIC_INFO *info, INT channel, float *pvalue)
{
int   value;
char  str[256];

  nitronic_switch(info);

  sprintf(str, "CH%02dL", channel+1);
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r+", str, 80, 1);
  sscanf(str+14, "%d", &value);

  *pvalue = (float) value;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_get_current(NITRONIC_INFO *info, INT channel, float *pvalue)
{
int   value;
char  str[256];

  nitronic_switch(info);

  sprintf(str, "CH%02dL", channel+1);
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r+", str, 80, 1);
  sscanf(str+19, "%d", &value);

  *pvalue = (float) value;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_get_all(NITRONIC_INFO *info, INT channels, float *array)
{
int   value, i;
char  str[256];

  nitronic_switch(info);

  sprintf(str, "CH01A");
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r", str, 80, 5);

  for (i=0 ; i < 32 ; i++)
    {
    rs232_waitfor(info->hdev, "\r", str, 80, 5);
    if (i < channels)
      {
      sscanf(str+12, "%d", &value);
      array[i] = (float) value;
      }
    }
  rs232_waitfor(info->hdev, "+", str, 80, 2);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_get_current_all(NITRONIC_INFO *info, INT channels, float *array)
{
int   value, i;
char  str[256];

  nitronic_switch(info);

  sprintf(str, "CH01A");
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r", str, 80, 5);

  for (i=0 ; i < 32 ; i++)
    {
    rs232_waitfor(info->hdev, "\r", str, 80, 5);
    if (i < channels)
      {
      sscanf(str+19, "%d", &value);
      array[i] = (float) value;
      }
    }
  rs232_waitfor(info->hdev, "+", str, 80, 2);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT nitronic_set_current_limit(NITRONIC_INFO *info, float limit)
{
char str[256];

  nitronic_switch(info);

  sprintf(str, "ICH01%04dA", (int)limit);
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r+", str, 80, 5);

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT nitronic(INT cmd, ...)
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
      status = nitronic_init(hKey, info, channel);
      break;

    case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = nitronic_exit(info);
      break;

    case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = nitronic_set(info, channel, value);
      break;

    case CMD_SET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = nitronic_set_all(info, channel, value);
      break;

    case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = nitronic_get(info, channel, pvalue);
      break;
    
    case CMD_GET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = nitronic_get_all(info, channel, pvalue);
      break;

    case CMD_GET_CURRENT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = nitronic_get_current(info, channel, pvalue);
      break;
    
    case CMD_GET_CURRENT_ALL:
      break;
      
    case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      if (channel == 0) /* one one limit for all channels */
        status = nitronic_set_current_limit(info, value);
      else
        status = FE_SUCCESS;
      break;

    default:
      cm_msg(MERROR, "nitronic device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
    }

  va_end(argptr);

  return status;
}

/*------------------------------------------------------------------*/

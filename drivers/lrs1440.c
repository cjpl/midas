/********************************************************************\

  Name:         lrs1440.c
  Created by:   Stefan Ritt

  Contents:     LeCroy LRS 1440 High Voltage Device Driver

  $Log$
  Revision 1.4  1999/01/14 17:01:44  midas
  Added rs232 port sharing

  Revision 1.3  1999/01/14 15:57:54  midas
  Adapted to new "info" structure

  Revision 1.2  1998/10/12 12:18:57  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "midas.h"
#include "rs232.h"

/*---- globals -----------------------------------------------------*/

int rs232_hdev[2];

typedef struct {
  char rs232_port[NAME_LENGTH];
  int  address;
  int  polarity[16];
} LRS1440_SETTINGS;

#define LRS1440_SETTINGS_STR "\
RS232 Port = STRING : [32] com1\n\
Address = INT : 1\n\
Polarity = INT[16] :\n\
[0] -1\n\
[1] -1\n\
[2] -1\n\
[3] -1\n\
[4] -1\n\
[5] -1\n\
[6] -1\n\
[7] -1\n\
[8] -1\n\
[9] -1\n\
[10] -1\n\
[11] -1\n\
[12] -1\n\
[13] -1\n\
[14] -1\n\
"

typedef struct {
  LRS1440_SETTINGS settings;
  int num_channels;
  int hdev;                            /* device handle for RS232 device */
} LRS1440_INFO;

/*---- device driver routines --------------------------------------*/

void lrs1440_switch(LRS1440_INFO *info)
{
static INT last_address=-1;
char str[80];
INT  status;

  if (info->settings.address != last_address)
    {
    rs232_puts(info->hdev, "M16\r\n");
    rs232_waitfor(info->hdev, "M16\r\n", str, 80, 5);
    sprintf(str, "M%02d\r\n", info->settings.address);
    rs232_puts(info->hdev, str);
    status = rs232_waitfor(info->hdev, "responding\r\n", str, 80, 5);
    if (!status)
      {
      cm_msg(MERROR, "lrs1440_init", 
        "LRS1440 adr %d doesn't respond. Check power and RS232 connection.", 
        info->settings.address);
      return;
      }

    last_address = info->settings.address;
    }
}

/*------------------------------------------------------------------*/

INT lrs1440_init(HNDLE hKey, void **pinfo, INT channels)
{
int          status, size, no;
char         str[256];
HNDLE        hDB;
LRS1440_INFO *info;

  /* allocate info structure */
  info = calloc(1, sizeof(LRS1440_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  /* create LRS1440 settings record */
  status = db_create_record(hDB, hKey, "", LRS1440_SETTINGS_STR);
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
    rs232_hdev[no-1] = info->hdev;
    }

  rs232_debug(FALSE);

  /* check if module is living  */
  rs232_puts(info->hdev, "M16\r\n");
  status = rs232_waitfor(info->hdev, "M16\r\n", str, 80, 2);
  if (!status)
    {
    cm_msg(MERROR, "lrs1440_init", "LRS1440 adr %d doesn't respond. Check power and RS232 connection.", info->settings.address);
    return FE_ERR_HW;
    }

  sprintf(str, "M%02d\r\n", info->settings.address);
  rs232_puts(info->hdev, str);
  status = rs232_waitfor(info->hdev, "responding\r\n", str, 80, 3);
  if (!status)
    {
    cm_msg(MERROR, "lrs1440_init", "LRS1440 adr %d doesn't respond. Check power and RS232 connection.", info->settings.address);
    return FE_ERR_HW;
    }

  /* check if HV enabled */
  rs232_puts(info->hdev, "ST\r");
  rs232_waitfor(info->hdev, "abled", str, 80, 3);

  if (str[strlen(str) - 6] == 's')
    {
    /* read rest of status */
    rs232_gets(info->hdev, str, 80, 0);

    cm_msg(MERROR, "lrs1440_init", "LeCroy1440 adr %d: HV disabled by front panel button", info->settings.address);
//    return FE_ERR_HW;
    }

  /* read rest of status */
  rs232_gets(info->hdev, str, 80, 0);

  /* turn on HV main switch */
  rs232_puts(info->hdev, "ON\r");
  rs232_waitfor(info->hdev, "Turn on\r\n", str, 80, 5);

  rs232_puts(info->hdev, "M16\r\n");

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_exit(LRS1440_INFO *info)
{
  rs232_exit(info->hdev);
  
  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_set(LRS1440_INFO *info, INT channel, float value)
{
char  str[80];

  lrs1440_switch(info);

  sprintf(str, "W%04.0f C%02d\r", info->settings.polarity[channel/16]*value, channel);

  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r\n\r\n", str, 80, 3);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_set_all(LRS1440_INFO *info, INT channels, float value)
{
char  str[80];
INT   i;

  lrs1440_switch(info);

  for (i=0 ; i<(channels-1)/16+1 ; i++)
    {
    sprintf(str, "W%04.0f C%d DO16\r", info->settings.polarity[i/16]*value, i*16);
    rs232_puts(info->hdev, str);
    rs232_waitfor(info->hdev, "\r\n\r\n", str, 80, 3);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_get(LRS1440_INFO *info, INT channel, float *pvalue)
{
int   value;
char  str[256];

  lrs1440_switch(info);

  sprintf(str, "R V C%02d\r", channel);
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "Act ", str, 80, 1);
  rs232_waitfor(info->hdev, "\r\n\r\n", str, 80, 1);
  sscanf(str+1, "%d", &value);

  *pvalue = (float) value;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_get_all(LRS1440_INFO *info, INT channels, float *array)
{
int   value, i, j;
char  str[256];

  lrs1440_switch(info);

  sprintf(str, "R V F C0 DO%d\r", channels);
  rs232_puts(info->hdev, str);

  for (i=0 ; i <= (channels-1)/8 ; i++)
    {
    rs232_waitfor(info->hdev, "Act ", str, 80, 5);
    rs232_waitfor(info->hdev, "\r\n", str, 80, 2);
    for (j=0 ; j<8 && i*8+j < channels; j++)
      {
      sscanf(str + 1 + j*6, "%d", &value);
      array[i*8 + j] = (float) value;
      }
    }
  rs232_waitfor(info->hdev, "\r\n", str, 80, 2);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_set_current_limit(LRS1440_INFO *info, float limit)
{
char str[256];

  lrs1440_switch(info);

  if (limit > 255)
    limit = 255.f;

  sprintf(str, "LI %1.0f\r", limit);
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r\n\r\n", str, 80, 1);

  sprintf(str, "LI %1.0f\r", -limit);
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, "\r\n\r\n", str, 80, 1);

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT lrs1440(INT cmd, ...)
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
      status = lrs1440_init(hKey, info, channel);
      break;

    case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = lrs1440_exit(info);
      break;

    case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = lrs1440_set(info, channel, value);
      break;

    case CMD_SET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = lrs1440_set_all(info, channel, value);
      break;

    case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1440_get(info, channel, pvalue);
      break;
    
    case CMD_GET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1440_get_all(info, channel, pvalue);
      break;

    /* current measurements not supported in LRS1440 */

    case CMD_GET_CURRENT:
      break;
    
    case CMD_GET_CURRENT_ALL:
      break;
      
    case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      if (channel == 0) /* one one limit for all channels */
        status = lrs1440_set_current_limit(info, value);
      else
        status = FE_SUCCESS;
      break;

    default:
      cm_msg(MERROR, "lrs1440 device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
    }

  va_end(argptr);

  return status;
}

/*------------------------------------------------------------------*/

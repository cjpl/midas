/********************************************************************\

  Name:         lrs1454.c
  Created by:   Stefan Ritt

  Contents:     LeCroy LRS1454/1458 Voltage Device Driver

  $Log$
  Revision 1.1  2000/12/05 00:49:55  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "midas.h"
#include "bus\rs232.h"

/*---- globals -----------------------------------------------------*/

int rs232_hdev[10];

typedef struct {
  char rs232_port[NAME_LENGTH];
  int  polarity[16];
} lrs1454_SETTINGS;

#define LRS1454_SETTINGS_STR "\
RS232 Port = STRING : [32] com1\n\
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
  lrs1454_SETTINGS settings;
  int num_channels;
  int hdev;                       /* device handle for RS232 device */
} lrs1454_INFO;

/*---- device driver routines --------------------------------------*/

INT lrs1454_init(HNDLE hKey, void **pinfo, INT channels)
{
int          status, size, i;
char         str[256], *p;
HNDLE        hDB;
lrs1454_INFO *info;

  /* allocate info structure */
  info = calloc(1, sizeof(lrs1454_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  /* create lrs1454 settings record */
  status = db_create_record(hDB, hKey, "", LRS1454_SETTINGS_STR);
  if (status != DB_SUCCESS)
    return FE_ERR_ODB;

  size = sizeof(info->settings);
  db_get_record(hDB, hKey, &info->settings, &size, 0);

  info->num_channels = channels;

  /* initialize RS232 port */
  p = info->settings.rs232_port;
  while (*p && !isdigit(*p))
    p++;

  i = *p - '0';
  
  if (i < 0)
    i = 0;
  if (i > 4)
    i = 4;

  if (rs232_hdev[i])
    info->hdev = rs232_hdev[i];
  else
    {
    info->hdev = rs232_init(info->settings.rs232_port, 9600, 'N', 8, 1, 0);
    if (info->hdev < 0)
      {
      cm_msg(MERROR, "lrs1454_init", "Cannot access port \"%s\"", info->settings.rs232_port);
      return FE_ERR_HW;
      }

    rs232_hdev[i] = info->hdev;
    }

  rs232_debug(FALSE);

  /* check if module is living  */
  rs232_puts(info->hdev, "\r");
  status = rs232_waitfor(info->hdev, ">", str, 80, 2);
  rs232_puts(info->hdev, "1450\r");
  status = rs232_waitfor(info->hdev, ">", str, 80, 2);
  if (!status)
    {
    cm_msg(MERROR, "lrs1454_init", "lrs1454 at port %s doesn't respond. Check power and RS232 connection.", info->settings.rs232_port);
    return FE_ERR_HW;
    }

  /* go into EDIT mode */
  rs232_puts(info->hdev, "EDIT\r");
  rs232_waitfor(info->hdev, ">", str, 80, 5);

  /* turn on HV main switch */
  rs232_puts(info->hdev, "HVON\r");
  rs232_waitfor(info->hdev, ">", str, 80, 5);

  /* enable cheannels */
  for (i=0 ; i<info->num_channels ; i++)
    {
    sprintf(str, "LD L%d.%d CE 1\r", i/12, i%12);
    rs232_puts(info->hdev, str);
    rs232_waitfor(info->hdev, ">", str, 80, 1);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_exit(lrs1454_INFO *info)
{
  rs232_puts(info->hdev, "quit\r");
  rs232_exit(info->hdev);
  
  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_set(lrs1454_INFO *info, INT channel, float value)
{
char  str[80];

  sprintf(str, "LD L%d.%d DV %1.1f\r", channel/12, channel%12, 
          info->settings.polarity[channel/12]*value);

  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, ">", str, 80, 1);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_set_all(lrs1454_INFO *info, INT channels, float value)
{
char  str[80];
INT   i;

  for (i=0 ; i<channels ; i++)
    {
    sprintf(str, "LD L%d.%d DV %1.1f\r", i/12, i%12, value);
    rs232_puts(info->hdev, str);
    rs232_waitfor(info->hdev, ">", str, 80, 3);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_get(lrs1454_INFO *info, INT channel, float *pvalue)
{
int   status;
char  str[256], *p;

  sprintf(str, "RC L%d.%d MV\r", channel/12, channel%12);
  rs232_puts(info->hdev, str);
  status = rs232_waitfor(info->hdev, ">", str, 80, 1);

  if (status == 0)
    {
    rs232_puts(info->hdev, "\r");
    rs232_waitfor(info->hdev, ">", str, 80, 1);
    }
  
  if (strstr(str, "to begin"))
    {
    rs232_puts(info->hdev, "1450\r");
    rs232_waitfor(info->hdev, ">", str, 80, 1);
    return lrs1454_get(info, channel, pvalue);
    }
  p = str+strlen(str)-1;;
  while (*p && *p != ' ')
    p--;

  *pvalue = (float) fabs(atof(p+1));

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_get_current(lrs1454_INFO *info, INT channel, float *pvalue)
{
char  str[256], *p;

  sprintf(str, "RC L%d.%d MC\r", channel/12, channel%12);
  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, ">", str, 80, 1);

  p = str+strlen(str)-1;;
  while (*p && *p != ' ')
    p--;

  *pvalue = (float) fabs(atof(p+1));

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_get_all(lrs1454_INFO *info, INT channels, float *array)
{
int   i, j;
char  str[256], *p;

  for (i=0 ; i<channels/12 ; i++)
    {
    sprintf(str, "RC L%d MV\r", i);
    rs232_puts(info->hdev, str);
    rs232_waitfor(info->hdev, ">", str, 256, 1);

    p = strstr(str, "MV")+3;
    p = strstr(p, "MV")+3;

    for (j=0 ; j<12 && i*12+j < channels ; j++)
      {
      array[i*12+j] = (float) fabs(atof(p));
      p = strchr(p, ' ')+1;
      }
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_get_current_all(lrs1454_INFO *info, INT channels, float *array)
{
int   i, j;
char  str[256], *p;

  for (i=0 ; i<channels/12 ; i++)
    {
    sprintf(str, "RC L%d MC\r", i);
    rs232_puts(info->hdev, str);
    rs232_waitfor(info->hdev, ">", str, 256, 1);

    p = strstr(str, "MC")+3;
    p = strstr(p, "MC")+3;

    for (j=0 ; j<12 && i*12+j < channels ; j++)
      {
      array[i*12+j] = (float) fabs(atof(p));
      p = strchr(p, ' ')+1;
      }
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_set_current_limit(lrs1454_INFO *info, int channel, float limit)
{
char  str[80];

  sprintf(str, "LD L%d.%d TC %1.1f\r", channel/12, channel%12, 
          info->settings.polarity[channel/16]*limit);

  rs232_puts(info->hdev, str);
  rs232_waitfor(info->hdev, ">", str, 80, 1);

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT lrs1454(INT cmd, ...)
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
      status = lrs1454_init(hKey, info, channel);
      break;

    case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = lrs1454_exit(info);
      break;

    case CMD_SET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = lrs1454_set(info, channel, value);
      break;

    case CMD_SET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = lrs1454_set_all(info, channel, value);
      break;

    case CMD_GET:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1454_get(info, channel, pvalue);
      break;
    
    case CMD_GET_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1454_get_all(info, channel, pvalue);
      break;

    case CMD_GET_CURRENT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1454_get_current(info, channel, pvalue);
      break;
    
    case CMD_GET_CURRENT_ALL:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1454_get_current_all(info, channel, pvalue);
      break;
      
    case CMD_SET_CURRENT_LIMIT:
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      status = lrs1454_set_current_limit(info, channel, value);
      break;

    default:
      cm_msg(MERROR, "lrs1454 device driver", "Received unknown command %d", cmd);
      status = FE_ERR_DRIVER;
      break;
    }

  va_end(argptr);

  return status;
}

/*------------------------------------------------------------------*/

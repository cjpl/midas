/********************************************************************\

  Name:         lrs1454.c
  Created by:   Stefan Ritt

  Contents:     LeCroy LRS1454/1458 Voltage Device Driver

  $Log$
  Revision 1.3  2001/01/04 11:17:23  midas
  Implemented Bus Driver scheme

  Revision 1.2  2000/12/18 09:45:39  midas
  changed CR-LF

  Revision 1.1  2000/12/05 00:49:55  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include "midas.h"

/*---- globals -----------------------------------------------------*/

typedef struct {
  int  polarity[16];
} lrs1454_SETTINGS;

#define LRS1454_SETTINGS_STR "\
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
  INT (*bd)(INT cmd, ...);             /* bus driver entry function */
  void *bd_info;                      /* private info of bus driver */
} LRS1454_INFO;

/*---- device driver routines --------------------------------------*/

INT lrs1454_init(HNDLE hkey, void **pinfo, INT channels, INT (*bd)(INT cmd, ...))
{
int          status, size, i;
char         str[256], *p;
HNDLE        hDB, hkeydd;
LRS1454_INFO *info;

  /* allocate info structure */
  info = (LRS1454_INFO *) calloc(1, sizeof(LRS1454_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  /* create lrs1454 settings record */
  status = db_create_record(hDB, hkey, "DD", LRS1454_SETTINGS_STR);
  if (status != DB_SUCCESS)
    return FE_ERR_ODB;

  db_find_key(hDB, hkey, "DD", &hkeydd);
  size = sizeof(info->settings);
  db_get_record(hDB, hkeydd, &info->settings, &size, 0);

  info->num_channels = channels;
  info->bd = bd;

  /* initialize bus driver */
  if (!bd)
    return FE_ERR_ODB;

  status = bd(CMD_INIT, hkey, &info->bd_info);
  
  if (status != SUCCESS)
    return status;

  bd(CMD_DEBUG, FALSE);

  /* check if module is living  */
  BD_PUTS("\r");
  BD_GETS(str, sizeof(str), ">", 2000);
  BD_PUTS("1450\r");
  status = BD_GETS(str, sizeof(str), ">", 2000);
  if (!status)
    {
    cm_msg(MERROR, "lrs1454_init", "lrs1454 doesn't respond. Check power and connection.");
    return FE_ERR_HW;
    }

  /* go into EDIT mode */
  BD_PUTS("EDIT\r");
  BD_GETS(str, sizeof(str), ">", 2000);

  /* turn on HV main switch */
  BD_PUTS("HVON\r");
  BD_GETS(str, sizeof(str), ">", 2000);

  /* enable cheannels */
  for (i=0 ; i<info->num_channels ; i++)
    {
    sprintf(str, "LD L%d.%d CE 1\r", i/12, i%12);
    BD_PUTS(str);
    BD_GETS(str, sizeof(str), ">", 2000);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_exit(LRS1454_INFO *info)
{
  BD_PUTS("quit\r");
  info->bd(CMD_EXIT, info->bd_info);

  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_set(LRS1454_INFO *info, INT channel, float value)
{
char  str[80];

  sprintf(str, "LD L%d.%d DV %1.1f\r", channel/12, channel%12,
          info->settings.polarity[channel/12]*value);

  BD_PUTS(str);
  BD_GETS(str, sizeof(str), ">", 1000);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_set_all(LRS1454_INFO *info, INT channels, float value)
{
char  str[80];
INT   i;

  for (i=0 ; i<channels ; i++)
    {
    sprintf(str, "LD L%d.%d DV %1.1f\r", i/12, i%12, value);
    BD_PUTS(str);
    BD_GETS(str, sizeof(str), ">", 1000);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_get(LRS1454_INFO *info, INT channel, float *pvalue)
{
int   status;
char  str[256], *p;

  sprintf(str, "RC L%d.%d MV\r", channel/12, channel%12);
  BD_PUTS(str);
  status = BD_GETS(str, sizeof(str), ">", 1000);

  /* if connection lost, reconnect */
  if (status == 0)
    {
    BD_PUTS("\r");
    BD_GETS(str, sizeof(str), ">", 1000);
    }

  if (strstr(str, "to begin"))
    {
    BD_PUTS("1450\r");
    BD_GETS(str, sizeof(str), ">", 1000);
    return lrs1454_get(info, channel, pvalue);
    }
  p = str+strlen(str)-1;;
  while (*p && *p != ' ')
    p--;

  *pvalue = (float) fabs(atof(p+1));

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_get_current(LRS1454_INFO *info, INT channel, float *pvalue)
{
char  str[256], *p;

  sprintf(str, "RC L%d.%d MC\r", channel/12, channel%12);
  BD_PUTS(str);
  BD_GETS(str, sizeof(str), ">", 1000);

  p = str+strlen(str)-1;;
  while (*p && *p != ' ')
    p--;

  *pvalue = (float) fabs(atof(p+1));

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1454_get_all(LRS1454_INFO *info, INT channels, float *array)
{
int   i, j;
char  str[256], *p;

  for (i=0 ; i<channels/12 ; i++)
    {
    sprintf(str, "RC L%d MV\r", i);
    BD_PUTS(str);
    BD_GETS(str, sizeof(str), ">", 5000);

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

INT lrs1454_get_current_all(LRS1454_INFO *info, INT channels, float *array)
{
int   i, j;
char  str[256], *p;

  for (i=0 ; i<channels/12 ; i++)
    {
    sprintf(str, "RC L%d MC\r", i);
    BD_PUTS(str);
    BD_GETS(str, sizeof(str), ">", 5000);

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

INT lrs1454_set_current_limit(LRS1454_INFO *info, int channel, float limit)
{
char  str[80];

  sprintf(str, "LD L%d.%d TC %1.1f\r", channel/12, channel%12,
          info->settings.polarity[channel/16]*limit);

  BD_PUTS(str);
  BD_GETS(str, sizeof(str), ">", 1000);

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT lrs1454(INT cmd, ...)
{
va_list argptr;
HNDLE   hKey;
INT     channel, status;
float   value, *pvalue;
void    *info, *bd;

  va_start(argptr, cmd);
  status = FE_SUCCESS;

  switch (cmd)
    {
    case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      bd = va_arg(argptr, void *);
      status = lrs1454_init(hKey, info, channel, bd);
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

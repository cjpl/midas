/********************************************************************\

  Name:         lrs1440.c
  Created by:   Stefan Ritt

  Contents:     LeCroy LRS 1440 High Voltage Device Driver

  $Log$
  Revision 1.2  1998/10/12 12:18:57  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "midas.h"
#include "rs232.h"

/*---- globals -----------------------------------------------------*/

#define MAX_DEVICES 10

static char   rs232_port[MAX_DEVICES][NAME_LENGTH];
static INT    lrs1440_address[MAX_DEVICES];
static INT    *polarity[MAX_DEVICES];

static int    hdev[MAX_DEVICES];  /* device handle for RS232 device */

/*---- device driver routines --------------------------------------*/

void lrs1440_switch(index)
{
static INT last_index=-1;
char str[80];
INT  status;

  if (index != last_index)
    {
    rs232_puts(hdev[index], "M16\r\n");
    rs232_waitfor(hdev[index], "M16\r\n", str, 80, 5);
    sprintf(str, "M%02d\r\n", lrs1440_address[index]);
    rs232_puts(hdev[index], str);
    status = rs232_waitfor(hdev[index], "responding\r\n", str, 80, 5);
    if (!status)
      {
      cm_msg(MERROR, "lrs1440_init", 
        "LRS1440 #%d doesn't respond. Check power and RS232 connection.", 
        lrs1440_address[index]);
      return;
      }

    last_index = index;
    }
}

/*------------------------------------------------------------------*/

INT lrs1440_init(HNDLE hKeyRoot, INT index, INT channels)
{
int   status, size, i, n_cards;
char  str[256];
HNDLE hDB;

  cm_get_experiment_database(&hDB, NULL);

  /* read LRS1440 settings */
  strcpy(rs232_port[index], "COM2");
  size = NAME_LENGTH;
  db_get_value(hDB, hKeyRoot, "RS232 Port", rs232_port[index], &size, TID_STRING);
  
  lrs1440_address[index] = index+1;
  size = sizeof(INT);
  db_get_value(hDB, hKeyRoot, "Address", &lrs1440_address[index], &size, TID_INT);
  
  n_cards = (channels-1)/16+1;
  polarity[index] = malloc(sizeof(INT)*n_cards);
  for (i=0 ; i<n_cards ; i++)
    polarity[index][i] = -1; /* default polarity is negative */
  db_merge_data(hDB, hKeyRoot, "Polarity", polarity[index], sizeof(INT)*n_cards, n_cards, TID_INT);

  /* initialize RS232 port */
  hdev[index] = 0;
  for (i=0 ; i<index ; i++)
    if (strcmp(rs232_port[i], rs232_port[index]) == 0)
      {
      hdev[index] = hdev[i];
      break;
      }

  if (hdev[index] == 0)
    hdev[index] = rs232_init(rs232_port[index], 9600, 'N', 8, 1, 0);

  rs232_debug(FALSE);

  /* check if module is living  */
  rs232_puts(hdev[index], "M16\r\n");
  status = rs232_waitfor(hdev[index], "M16\r\n", str, 80, 2);
  if (!status)
    {
    cm_msg(MERROR, "lrs1440_init", "LRS1440 adr %d doesn't respond. Check power and RS232 connection.", lrs1440_address[index]);
    return FE_ERR_HW;
    }

  sprintf(str, "M%02d\r\n", lrs1440_address[index]);
  rs232_puts(hdev[index], str);
  status = rs232_waitfor(hdev[index], "responding\r\n", str, 80, 3);
  if (!status)
    {
    cm_msg(MERROR, "lrs1440_init", "LRS1440 adr %d doesn't respond. Check power and RS232 connection.", lrs1440_address[index]);
    return FE_ERR_HW;
    }

  /* check if HV enabled */
  rs232_puts(hdev[index], "ST\r");
  rs232_waitfor(hdev[index], "abled", str, 80, 3);

  if (str[strlen(str) - 6] == 's')
    {
    /* read rest of status */
    rs232_gets(hdev[index], str, 80, 0);

    cm_msg(MERROR, "lrs1440_init", "LeCroy1440 #%d: HV disabled by front panel button", index);
//    return FE_ERR_HW;
    }

  /* read rest of status */
  rs232_gets(hdev[index], str, 80, 0);

  /* turn on HV main switch */
  rs232_puts(hdev[index], "ON\r");
  rs232_waitfor(hdev[index], "Turn on\r\n", str, 80, 5);

  rs232_puts(hdev[index], "M16\r\n");

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_exit(INT index)
{
  rs232_exit(hdev[index]);
  
  free(polarity[index]);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_set(INT index, INT channel, float value)
{
char  str[80];

  lrs1440_switch(index);

  sprintf(str, "W%04.0f C%02d\r", polarity[index][channel/16]*value, channel);

  rs232_puts(hdev[index], str);
  rs232_waitfor(hdev[index], "\r\n\r\n", str, 80, 3);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_set_all(INT index, INT channels, float value)
{
char  str[80];
INT   i;

  lrs1440_switch(index);

  for (i=0 ; i<(channels-1)/16+1 ; i++)
    {
    sprintf(str, "W%04.0f C%d DO16\r", polarity[index][i/16]*value, i*16);
    rs232_puts(hdev[index], str);
    rs232_waitfor(hdev[index], "\r\n\r\n", str, 80, 3);
    }

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_get(INT index, INT channel, float *pvalue)
{
int   value;
char  str[256];

  lrs1440_switch(index);

  sprintf(str, "R V C%02d\r", channel);
  rs232_puts(hdev[index], str);
  rs232_waitfor(hdev[index], "Act ", str, 80, 1);
  rs232_waitfor(hdev[index], "\r\n\r\n", str, 80, 1);
  sscanf(str+1, "%d", &value);

  *pvalue = (float) value;

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_get_all(INT index, INT channels, float *array)
{
int   value, i, j;
char  str[256];

  lrs1440_switch(index);

  sprintf(str, "R V F C0 DO%d\r", channels);
  rs232_puts(hdev[index], str);

  for (i=0 ; i <= (channels-1)/8 ; i++)
    {
    rs232_waitfor(hdev[index], "Act ", str, 80, 5);
    rs232_waitfor(hdev[index], "\r\n", str, 80, 2);
    for (j=0 ; j<8 && i*8+j < channels; j++)
      {
      sscanf(str + 1 + j*6, "%d", &value);
      array[i*8 + j] = (float) value;
      }
    }
  rs232_waitfor(hdev[index], "\r\n", str, 80, 2);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT lrs1440_set_current_limit(INT index, float limit)
{
char str[256];

  lrs1440_switch(index);

  if (limit > 255)
    limit = 255.f;

  sprintf(str, "LI %1.0f\r", limit);
  rs232_puts(hdev[index], str);
  rs232_waitfor(hdev[index], "\r\n\r\n", str, 80, 1);

  sprintf(str, "LI %1.0f\r", -limit);
  rs232_puts(hdev[index], str);
  rs232_waitfor(hdev[index], "\r\n\r\n", str, 80, 1);

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT lrs1440(INT cmd, ...)
{
va_list argptr;
HNDLE   hKey;
INT     index, channel, status;
float   value, *pvalue;

  va_start(argptr, cmd);
  status = FE_SUCCESS;

  switch (cmd)
    {
    case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);
      index = va_arg(argptr, INT);
      channel = va_arg(argptr, INT);
      status = lrs1440_init(hKey, index, channel);
      break;

    case CMD_EXIT:
      index = va_arg(argptr, INT);
      status = lrs1440_exit(index);
      break;

    case CMD_SET:
      index = va_arg(argptr, INT);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = lrs1440_set(index, channel, value);
      break;

    case CMD_SET_ALL:
      index = va_arg(argptr, INT);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);
      status = lrs1440_set_all(index, channel, value);
      break;

    case CMD_GET:
      index = va_arg(argptr, INT);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1440_get(index, channel, pvalue);
      break;
    
    case CMD_GET_ALL:
      index = va_arg(argptr, INT);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float*);
      status = lrs1440_get_all(index, channel, pvalue);
      break;

    /* current measurements not supported in LRS1440 */

    case CMD_GET_CURRENT:
      break;
    
    case CMD_GET_CURRENT_ALL:
      break;
      
    case CMD_SET_CURRENT_LIMIT:
      index = va_arg(argptr, INT);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);
      if (channel == 0) /* one one limit for all channels */
        status = lrs1440_set_current_limit(index, value);
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

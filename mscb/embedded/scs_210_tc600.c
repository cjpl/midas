/********************************************************************\

  Name:         scs_210_tc600.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node connected to Pfeiffer
                Turbomolecular Pump with TC600 Electronics

  $Log$
  Revision 1.2  2002/11/28 13:03:41  midas
  Protocol version 1.2

  Revision 1.1  2002/11/20 12:05:14  midas
  Initila revision


\********************************************************************/

#include <stdio.h>
#include <stdlib.h> // for atof()
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "TC600";
bit       terminal_mode;

unsigned char tc600_write(unsigned short param, unsigned char len, unsigned long value);

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  unsigned char  pump_on;
  unsigned char  vent_on;
  unsigned short rot_speed;
  float          tmp_current;
} user_data;

struct {
  unsigned char baud;
  unsigned char address;
} user_conf;
  

MSCB_INFO_CHN code channel[] = {
  SIZE_0BIT,   UNIT_ASCII, 0, 0,           0, "RS232",   0,
  SIZE_8BIT, UNIT_BOOLEAN, 0, 0,           0, "Pump on", &user_data.pump_on,
  SIZE_8BIT, UNIT_BOOLEAN, 0, 0,           0, "Vent on", &user_data.vent_on,
  SIZE_16BIT,  UNIT_HERTZ, 0, 0,           0, "RotSpd",  &user_data.rot_speed,
  SIZE_32BIT, UNIT_AMPERE, 0, 0, MSCBF_FLOAT, "TMPcur",  &user_data.tmp_current,
  0
};

MSCB_INFO_CHN code conf_param[] = {
  SIZE_8BIT,    UNIT_BAUD, 0, 0,           0, "Baud",    &user_conf.baud,
  SIZE_8BIT,    UNIT_BYTE, 0, 0,           0, "Address", &user_conf.address,
  0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char channel) reentrant;
void write_gain(void);

/*---- User init function ------------------------------------------*/

void user_init(void)
{
  /* initialize UART1 */
  if (user_conf.baud == 0xFF)
    {
    user_conf.baud = 1; // 9600 by default
    eeprom_flash();
    }

  uart_init(1, user_conf.baud);

  /* turn on turbo pump motor (not pump station) */
  tc600_write(23, 6, 111111);

  /* vent mode */
  tc600_write(30, 3, 0);
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char idata in_buf[10], out_buf[8];

#pragma NOAREGS

char idata obuf[8];

void user_write(unsigned char channel) reentrant
{
unsigned char i, n;

  if (channel == 0)
    {
    if (in_buf[2] == 27)
      terminal_mode = 0;
    else if (in_buf[2] == 0)
      terminal_mode = 1;
    else
      {
      n = (in_buf[0] & 0x07) - 1;
      for (i=0 ; i< n ; i++)
        putchar(in_buf[i+2]);
      }
    }
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
char c;

  if (channel == 0)
    {
    c = getchar_nowait();
    if (c != -1)
      {
      out_buf[1] = c;
      return 1;
      }
    }

  return 0;
}

/*---- User write config function ----------------------------------*/

void user_write_conf(unsigned char channel) reentrant
{
  if (channel == 0)
    uart_init(1, user_conf.baud);
}

/*---- User read config function -----------------------------------*/

void user_read_conf(unsigned char channel)
{
  if (channel);
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char idata *data_in,
                        unsigned char idata *data_out)
{
  /* echo input data */
  data_out[0] = data_in[0];
  data_out[1] = data_in[1];
  return 2;
}

/*------------------------------------------------------------------*/

unsigned char tc600_read(unsigned short param, char *result)
{
char idata str[32];
unsigned char i, j, cs, len;

  sprintf(str, "%03Bd00%03d02=?", user_conf.address, param);

  for (i=cs=0 ; i<12 ; i++)
    cs += str[i];

  sprintf(str+12, "%03Bd\r", cs);
  printf(str);
  flush();
  i = gets_wait(str, sizeof(str), 200);
  if (i > 2)
    {
    len = (str[8] - '0') * 10 + (str[9] - '0');

    for (j=0 ; j<len ; j++)
      result[j] = str[10+j];

    result[j] = 0;
    return len;
    }

  return 0;
}

/*------------------------------------------------------------------*/

unsigned char tc600_write(unsigned short param, unsigned char len, unsigned long value)
{
char idata str[32];
unsigned char i, cs;

  if (len == 6)
    sprintf(str, "%03Bd10%03d06%06ld", user_conf.address, param, value);
  else if (len == 3)
    sprintf(str, "%03Bd10%03d03%03ld", user_conf.address, param, value);

  for (i=cs=0 ; i<16 ; i++)
    cs += str[i];

  sprintf(str+16, "%03Bd\r", cs);
  printf(str);
  flush();
  i = gets_wait(str, sizeof(str), 200);

  return i;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
char idata str[32];
static unsigned long last=0;
static char pump_on_last = -1;
static char vent_on_last = -1;

  if (!terminal_mode && time() > last+100)
    {
    last = time();

    if (user_data.pump_on != pump_on_last)
      {
      tc600_write(10, 6, user_data.pump_on ? 111111 : 0);
      pump_on_last = user_data.pump_on;
      }

    if (user_data.vent_on != vent_on_last)
      {
      tc600_write(12, 6, user_data.vent_on ? 111111 : 0);
      vent_on_last = user_data.vent_on;
      }

    if (tc600_read(309, str))
      user_data.rot_speed = atoi(str);

    if (tc600_read(310, str))
      user_data.tmp_current = atoi(str) / 100.0;
    }
}


/********************************************************************\

  Name:         scs_210.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node

  $Log$
  Revision 1.1  2002/10/03 15:35:06  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-210";

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  float value;
} user_data;

struct {
  unsigned char baud;
} user_conf;
  

MSCB_INFO_CHN code channel[] = {
  SIZE_32BIT, 0, 0, MSCBF_FLOAT, "Value", &user_data.value,
  SIZE_0BIT,  0, 0,           0, "RS232", 0,
  0
};

MSCB_INFO_CHN code conf_param[] = {
  SIZE_8BIT,  0, 0,           0, "Baud",  &user_conf.baud,
  0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char channel);
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

  // DEBUG_MODE = 1;
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char idata in_buf[10], out_buf[8];

#pragma NOAREGS

char idata obuf[8];

void user_write(unsigned char channel)
{
unsigned char i, n;

  if (channel == 1)
    {
    n = (in_buf[0] & 0x07) - 1;
    for (i=0 ; i< n ; i++)
      putchar(in_buf[i+2]);
    }
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
char c;

  if (channel == 1)
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

void user_write_conf(unsigned char channel)
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

/*---- User loop function ------------------------------------------*/

#include <stdlib.h>

void user_loop(void)
{
  /*
  gets(str, sizeof(str));
  printf("%s\r\n", str);
  user_data.value = atof(str);
  */
}


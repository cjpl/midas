/********************************************************************\

  Name:         scs_210.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node connected to a
                Pfeiffer Dual Gauge TPG262 vacuum sensor

  $Log$
  Revision 1.4  2002/11/06 16:45:42  midas
  Revised LED blinking scheme

  Revision 1.3  2002/10/09 15:48:13  midas
  Fixed bug with download

  Revision 1.2  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.1  2002/10/03 15:34:23  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <stdlib.h> // for atof()
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "TPG262";
bit       terminal_mode;

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  float p1;
  float p2;
} user_data;

struct {
  unsigned char baud;
} user_conf;
  

MSCB_INFO_CHN code channel[] = {
  SIZE_0BIT,   UNIT_ASCII, 0, 0,           0, "RS232", 0,
  SIZE_32BIT, UNIT_PASCAL, 0, 0, MSCBF_FLOAT, "P1", &user_data.p1,
  SIZE_32BIT, UNIT_PASCAL, 0, 0, MSCBF_FLOAT, "P2", &user_data.p2,
  0
};

MSCB_INFO_CHN code conf_param[] = {
  SIZE_8BIT,   UNIT_HERTZ, 0, 0,           0, "Baud",  &user_conf.baud,
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
  if (user_conf.baud == 0xFF ||
      user_conf.baud == 0)
    {
    user_conf.baud = 1; // 9600 by default
    eeprom_flash();
    }

  uart_init(1, user_conf.baud);
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char idata in_buf[10], out_buf[8];

#pragma NOAREGS

char idata obuf[8];

void user_write(unsigned char channel)
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

void user_loop(void)
{
char idata str[32];
unsigned char i;

  if (!terminal_mode)
    {
    i = gets_wait(str, sizeof(str), 200);
  
    if (i == 0)
      {
      // start continuous mode, 1s update
      printf("COM,1\r\n");
      flush();
      }
    else if (i == 27)
      {
      user_data.p1 = atof(str+2);
      user_data.p2 = atof(str+16);
      }
    }
}


/********************************************************************\

  Name:         scd_300.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-300 Parallel Port Interface

  $Log$
  Revision 1.2  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.1  2002/10/04 09:02:05  midas
  Initial revision

  Revision 1.2  2002/10/03 15:31:53  midas
  Various modifications

  Revision 1.1  2002/07/12 15:20:08  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "mscb.h"

char code node_name[] = "SCS-300";

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  float value[4];
} user_data;

MSCB_INFO_CHN code channel[] = {
  SIZE_32BIT, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data0", &user_data.value[0],
  SIZE_32BIT, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data1", &user_data.value[1],
  SIZE_32BIT, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data2", &user_data.value[2],
  SIZE_32BIT, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data3", &user_data.value[3],
  0
};

MSCB_INFO_CHN code conf_param[] = {
  0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

/* 8 data bits to LPT */
#define LPT_DATA         P1      

/* control/status bits to LPT       DB25   */
sbit LPT_STROBE =       P2^3;    // Pin 1
sbit LPT_INIT   =       P2^4;    // Pin 16
sbit LPT_SELECT =       P2^5;    // Pin 17
sbit LPT_ACK    =       P2^6;    // Pin 10
sbit LPT_BUSY   =       P2^7;    // Pin 11

#pragma NOAREGS

void user_write(unsigned char channel);

/*---- User init function ------------------------------------------*/

void user_init(void)
{
  /* set initial state of lines */
  LPT_DATA   = 0xFF;
  LPT_STROBE = 1;
  LPT_INIT   = 1;
  LPT_SELECT = 1;
  LPT_ACK    = 1;
  LPT_BUSY   = 1;
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char channel)
{
  if (channel == 0);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
  if (channel);
  return 0;
}

/*---- User write config function ----------------------------------*/

void user_write_conf(unsigned char channel)
{
  if (channel);
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

/*---- Functions for parallel port ---------------------------------*/

char putchar(char c)
{
  /* wait for acknowledge to go low */
  while (LPT_ACK);

  /* output data */
  LPT_DATA = c;

  /* negative strobe */
  LPT_STROBE = 0;
  
  /* wati for acknowledge to go high */
  while (!LPT_ACK);
  
  /* remove strobe */
  LPT_STROBE = 1;

  /* clear data for next input */
  LPT_DATA = 0xFF;

  return c;
}
 
char getchar_nowait()
{
char c;

  /* if busy signal high -> no data */
  if (LPT_BUSY == 1)
    return -1;

  /* read byte */
  c = LPT_DATA;

  /* singal received data */
  LPT_SELECT = 0;

  /* wait for busy go high */
  while (LPT_BUSY == 0);

  /* remove select */
  LPT_SELECT = 1;
}

unsigned char gets_wait(char *str, unsigned char size, unsigned char timeout)
{
unsigned long start;
unsigned char i;
char          c;

  start = time();
  i = 0;
  do
    {
    c = getchar_nowait();
    if (c != -1 && c != '\n')
      {
      if (c == '\r')
        {
        str[i] = 0;
        return i;
        }
      str[i++] = c;
      if (i == size)
        return i;
      }

    watchdog_refresh();

    } while (time() < start+timeout);

  return 0;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
char idata str[32];
unsigned char i, n;

  return;

  for (i=0 ; i<4 ; i++)
    {
    /* send data request */
    printf("READ %d\r\n", i);

    /* wait for reply for 2 seconds */
    n = gets_wait(str, sizeof(str), 200);
  
    /* if correct response, interprete data */
    if (n == 10)
      user_data.value[i] = atof(str);

    watchdog_refresh();
    }

}


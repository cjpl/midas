/********************************************************************\

  Name:         scs_210.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node

  $Log$
  Revision 1.9  2004/01/07 12:52:23  midas
  Changed indentation

  Revision 1.8  2003/05/02 09:03:01  midas
  Fixed buffer overflows by strlcpy()

  Revision 1.7  2003/04/11 08:59:10  midas
  Merged channels/config into variables

  Revision 1.6  2003/02/19 16:05:36  midas
  Added 'init' parameter to user_init

  Revision 1.5  2002/11/28 14:44:02  midas
  Removed SIZE_XBIT

  Revision 1.4  2002/11/28 13:03:41  midas
  Protocol version 1.2

  Revision 1.3  2002/10/09 15:48:13  midas
  Fixed bug with download

  Revision 1.2  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.1  2002/10/03 15:35:06  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>             // for atof()
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-210";
bit terminal_mode;

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float value;
   unsigned char baud;
} user_data;

MSCB_INFO_VAR code variables[] = {
   1, UNIT_ASCII, 0, 0, 0, "RS232", 0,
   4, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Value", &user_data.value,
   1, UNIT_BAUD, 0, 0, 0, "Baud", &user_data.baud,
   0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char channel) reentrant;
void write_gain(void);

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   /* initialize UART1 */
   if (init)
      user_data.baud = 1;       // 9600 by default

   uart_init(1, user_data.baud);

   // DEBUG_MODE = 1;
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[300], out_buf[300];

#pragma NOAREGS

char idata obuf[8];

void user_write(unsigned char channel) reentrant
{
   unsigned char i, n;

   if (channel == 0) {
      if (in_buf[2] == 27)
         terminal_mode = 0;
      else if (in_buf[2] == 0)
         terminal_mode = 1;
      else {
         n = (in_buf[0] & 0x07) - 1;
         for (i = 0; i < n; i++)
            putchar(in_buf[i + 2]);
      }
   }
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
   char c;

   if (channel == 0) {
      c = getchar_nowait();
      if (c != -1) {
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
      uart_init(1, user_data.baud);
}

/*---- User read config function -----------------------------------*/

void user_read_conf(unsigned char channel)
{
   if (channel);
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char *data_in, unsigned char *data_out)
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

   if (!terminal_mode) {
      i = gets_wait(str, sizeof(str), 200);

      if (i > 2)
         user_data.value = atof(str);
   }
}

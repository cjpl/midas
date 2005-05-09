/********************************************************************\

  Name:         scs_210.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node

  $Log$
  Revision 1.16  2005/05/09 09:09:50  ritt
  Decreased power consumption of scs_210

  Revision 1.15  2005/03/08 14:52:32  ritt
  Adapted SCS_210 to F121 CPU

  Revision 1.14  2004/12/13 11:13:43  midas
  Modified terminal mode

  Revision 1.13  2004/07/30 10:22:03  midas
  Added MSCBF_DATALESS

  Revision 1.12  2004/06/16 11:39:55  midas
  Added external watchdog

  Revision 1.11  2004/04/07 11:06:17  midas
  Version 1.7.1

  Revision 1.10  2004/03/05 12:27:46  midas
  Added input/output strings

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

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

bit output_flag, flush_flag;
unsigned char n_in;

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   char output[32];
   char input[32];
   unsigned char baud;
} xdata user_data;

MSCB_INFO_VAR code variables[] = {
   1,   UNIT_ASCII,  0, 0, MSCBF_DATALESS, "RS232", 0,
   32,  UNIT_STRING, 0, 0,              0, "Output", &user_data.output[0],
   32,  UNIT_STRING, 0, 0,              0, "Input", &user_data.input[0],
   1,   UNIT_BAUD,   0, 0,              0, "Baud", &user_data.baud,
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
      user_data.baud = BD_9600;    // 9600 by default

   uart_init(1, user_data.baud);
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[64], out_buf[64];

#pragma NOAREGS

char idata obuf[8];

void user_write(unsigned char index) reentrant
{
   unsigned char i, n;

   if (index == 0) {
      n = in_buf[1]-1;
      for (i = 0; i < n; i++)
         putchar(in_buf[3 + i]);
      flush_flag = 1;
   }

   if (index == 1) 
      output_flag = 1;

   if (index == 3)
      uart_init(1, user_data.baud);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
   char c, n;

   if (channel == 0) {

      for (n=0 ; n<32 ; n++) {
         c = getchar_nowait();
         if (c == -1)
            break;

         /* put character directly in return buffer */
         out_buf[2 + n] = c;
      }
      return n;
   }

   return 0;
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

/* external watchdog */
sbit EWD = P0 ^ 5;

sbit led_0 = LED_0;

void user_loop(void)
{
   char c;
   unsigned char i;
   unsigned long t_input;

   if (flush_flag) {
      flush_flag = 0;
      flush();
   }

   if (output_flag)
      {
      output_flag = 0;

      /* perform one string exchange: send output, wait for input */
            
      /* delete input string */
      n_in = 0;
      for (i=0 ; i<32 ; i++)
         user_data.input[i] = 0;

      /* send output string */
      for (i=0 ; i<32 ; i++) {
         if (!user_data.output[i])
            break;
         putchar(user_data.output[i]);
         flush();

         /* delay for slow modules */
         delay_ms(10);
      }

      /* remember time */
      t_input = time();
   }

   /* read 3 seconds after write into input string */
   if (time() - t_input < 300) {
      c = getchar_nowait();
      if (c != -1 && n_in < 32) {
         user_data.input[n_in++] = c;
         led_blink(1, 1, 50);
      }
   }

   /* toggle external watchdog */
   EWD = !EWD;

   /* go into idle mode, wakeup via UART or 100Hz interrupt */
   PCON |= 0x01;
}

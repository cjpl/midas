/********************************************************************\

  Name:         scs_220.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-220 RS-485 node

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>             // for atof()
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-220";

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

MSCB_INFO_VAR code vars[] = {
   1,   UNIT_ASCII,  0, 0, MSCBF_DATALESS, "RS485", 0,
   32,  UNIT_STRING, 0, 0,              0, "Output", &user_data.output[0],
   32,  UNIT_STRING, 0, 0,              0, "Input", &user_data.input[0],
   1,   UNIT_BAUD,   0, 0,              0, "Baud", &user_data.baud,
   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char channel) reentrant;
void write_gain(void);

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   P0MDOUT |= 0x40;             // P0.6: RS485_SEC_ENABLE = Push Pull
   P1MDOUT = 0x00;              // P1: LPT
   P2MDOUT = 0x00;              // P2: LPT
   P3MDOUT = 0xE0;              // P3.5,6,7: Optocouplers

   /* initialize UART1 */
   if (init)
      user_data.baud = BD_9600;    // 9600 by default

   uart_init(1, user_data.baud);
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[300], out_buf[300];

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
sbit EWD = P0 ^ 7;

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
}

/********************************************************************\

  Name:         scs_210_ac58.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node connected to Hengstler
                AC58 angular encoder

  $Id: scs_210.c 3178 2006-07-18 09:11:28Z ritt $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>             // for atof()
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "AC58";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float position;
   unsigned long offset;
   float factor;
} xdata user_data;

MSCB_INFO_VAR code vars[] = {
   4,  UNIT_METER,  0, 0,  MSCBF_FLOAT, "Position",&user_data.position,
   4,  UNIT_METER,  0, 0,            0, "Offset",  &user_data.offset,
   4,  UNIT_FACTOR, 0, 0,  MSCBF_FLOAT, "Factor",  &user_data.factor,
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
   if (init) {
      user_data.offset = 0;
      user_data.factor = 1;
   }

   /* initialize UART1 */
   uart_init(1, BD_2400);
   watchdog_enable(2);
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[64], out_buf[64];

#pragma NOAREGS

char idata obuf[8];

void user_write(unsigned char index) reentrant
{
   if (index == 0);
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
static unsigned long xdata last_read = 0;
unsigned char xdata str[80], i, bcc;
unsigned long int xdata d;
float xdata p;

   /* read parameters once each 1000 ms */
   if (time() > last_read + 100) {
      last_read = time();

      putchar(0x02); // STX
      flush();

      i = gets_wait(str, 1, 200);
      if (i == 1 && str[0] == 0x10) {
         putchar(0x80); // read position value
         putchar(0x00);
         putchar(0x00);
         putchar(0x00);

         putchar(0x10); // DLE
         putchar(0x03); // ETX

         // checksum = xor of data bytes
         bcc = 0x80 ^ 0x10 ^ 0x03;
         putchar(bcc);

         // receive DLE
         str[0] = str[1] = 0;
         i = gets_wait(str, 1, 200);
         if (i == 1 && str[0] == 0x10) {

            // receive STX
            i = gets_wait(str, 1, 200);
            if (i == 1 && str[0] == 0x02) {
               putchar(0x10); // DLE
               i = gets_wait(str, 7, 200);
               if (i == 7) {
                  putchar(0x10); // DLE

                  d = str[0];            // MSB
                  d = (d << 8) | str[1]; // MB
                  d = (d << 8) | str[2]; // LSB

                  d = d - user_data.offset;
                  p = d*user_data.factor;
                  DISABLE_INTERRUPTS;
                  user_data.position = p;
                  ENABLE_INTERRUPTS;
               }
            }
         }
      }
   }

   /* go into idle mode, wakeup via UART or 100Hz interrupt */
   //PCON |= 0x01;
}

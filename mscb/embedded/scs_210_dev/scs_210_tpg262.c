/********************************************************************\

  Name:         scs_210.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node connected to a
                Pfeiffer Dual Gauge TPG262 vacuum sensor

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>             // for atof()
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "TPG262";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

bit flush_flag;
static unsigned long last_read = 0;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float p1;
   float p2;
   unsigned char baud;
} idata user_data;


MSCB_INFO_VAR code vars[] = {
   1, UNIT_ASCII,  0, 0, MSCBF_DATALESS, "RS232",               0,
   4, UNIT_PASCAL, 0, 0,    MSCBF_FLOAT, "P1",      &user_data.p1,
   4, UNIT_PASCAL, 0, 0,    MSCBF_FLOAT, "P2",      &user_data.p2,

   1, UNIT_BAUD,   0, 0,              0, "Baud",  &user_data.baud,
   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void write_gain(void);

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   /* initialize UART1 */
   if (init)
      user_data.baud = BD_9600;   // 9600 by default

   uart_init(1, user_data.baud);
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[300], out_buf[300];

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   unsigned char i, n;

   if (index == 0) {
      /* prevent reads for 1s */
      last_read = time();

      /* send characters */
      n = in_buf[1]-1;
      for (i = 0; i < n; i++)
         putchar(in_buf[3 + i]);
      flush_flag = 1;
   }

   if (index == 3)
      uart_init(1, user_data.baud);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
   char c, n;

   if (index == 0) {

      /* prevent pump reads for 1s */
      last_read = time();

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

void user_loop(void)
{
   char idata str[32];
   unsigned char i;

   if (flush_flag) {
      flush_flag = 0;
      flush();
   }

   /* read parameters once each second */
   if (time() > last_read + 100) {
      last_read = time();

      i = gets_wait(str, sizeof(str), 200);

      if (i == 0) {
         // start continuous mode, 1s update
         printf("COM,1\r\n");
         flush();
      } else if (i == 27) {
         user_data.p1 = atof(str + 2);
         user_data.p2 = atof(str + 16);
      }
   }
}

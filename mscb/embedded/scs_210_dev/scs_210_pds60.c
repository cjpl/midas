/********************************************************************\

  Name:         scs_210_pds60.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node connected to Kenwood
                PDS60-6 regulated power supply

  $Id: scs_210.c 3178 2006-07-18 09:11:28Z ritt $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>             // for atof()
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "PDS60";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

sbit CTS = P0 ^ 7;
sbit RTS = P0 ^ 6;

bit output_flag;

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float demand_voltage;
   float demand_current;
   float voltage;
   float current;
   unsigned char on;
} xdata user_data;

MSCB_INFO_VAR code vars[] = {
   4,  UNIT_VOLT,   0, 0,  MSCBF_FLOAT, "DmdVolt", &user_data.demand_voltage,
   4,  UNIT_AMPERE, 0, 0,  MSCBF_FLOAT, "DmdCurr", &user_data.demand_current,
   4,  UNIT_VOLT,   0, 0,  MSCBF_FLOAT, "Volt",    &user_data.voltage,
   4,  UNIT_AMPERE, 0, 0,  MSCBF_FLOAT, "Curr",    &user_data.current,
   1,  UNIT_BOOLEAN,0, 0,            0, "On",      &user_data.on,
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
      user_data.demand_voltage = 0;
      user_data.demand_current = 0;
      user_data.on = 0;
   }

   /* initialize UART1 */
   uart_init(1, BD_9600);
   watchdog_enable(2);


   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0x81;      // P0.0: TX = Push Pull, P0.7 = Push Pull

   /* indicate we can receive data */
   CTS = 0;

   output_flag = 1;
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[64], out_buf[64];

#pragma NOAREGS

char idata obuf[8];

void user_write(unsigned char index) reentrant
{
   if (index == 0 || index == 1 || index == 4) 
      output_flag = 1;
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
char xdata str[80], i;

   if (output_flag) {
      output_flag = 0;

      printf("VA%1.2f\n", user_data.demand_voltage);
      flush();
      printf("AA%1.2f\n", user_data.demand_current);
      flush();
      if (user_data.on)
         puts("SW1\r\n"); // turn output on
      else
         puts("SW0\r\n"); // turn output off
   }

   /* read parameters once each second */
   if (time() > last_read + 100) {
      last_read = time();

      printf("ST0\n");
      flush();

      i = gets_wait(str, sizeof(str), 200);
      if (i == 18) {
         user_data.voltage = atof(str+7);
         user_data.current = atof(str+12);
      } else if (i == 19) {
         user_data.voltage = atof(str+7);
         user_data.current = atof(str+13);
      }
   }

   /* go into idle mode, wakeup via UART or 100Hz interrupt */
   PCON |= 0x01;
}

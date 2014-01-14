/********************************************************************\

  Name:         scs_210_maxigauge.c
  Created by:   Andreas Knecht

  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-210 RS232 node connected to a
                Pfeiffer Maxigauge TPG 256

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h> // for atof()
#include <string.h>

#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "maxigauge";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

bit flush_flag;
static unsigned long xdata last_read = 0;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

typedef struct {
   float p[6];
   char deb_str[32];
   unsigned char baud;
} USER_DATA;

USER_DATA xdata user_data;

MSCB_INFO_VAR code vars[] = {
   1, UNIT_ASCII,  0, 0, MSCBF_DATALESS, "RS232",               0,
   4, UNIT_BAR, PRFX_MILLI, 0,    MSCBF_FLOAT, "P1",      &user_data.p[0],
   4, UNIT_BAR, PRFX_MILLI, 0,    MSCBF_FLOAT, "P2",      &user_data.p[1],
   4, UNIT_BAR, PRFX_MILLI, 0,    MSCBF_FLOAT, "P3",      &user_data.p[2],
   4, UNIT_BAR, PRFX_MILLI, 0,    MSCBF_FLOAT, "P4",      &user_data.p[3],
   4, UNIT_BAR, PRFX_MILLI, 0,    MSCBF_FLOAT, "P5",      &user_data.p[4],
   4, UNIT_BAR, PRFX_MILLI, 0,    MSCBF_FLOAT, "P6",      &user_data.p[5],
   32, UNIT_STRING,  0, 0, 0, "debug",          &user_data.deb_str,
   1, UNIT_BAUD,         0, 0,              0, "Baud",    &user_data.baud,
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
extern unsigned char xdata in_buf[256], out_buf[64];

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
   unsigned char xdata i, r;
   float xdata p;
   char xdata str[32];
   int xdata status;

   const char *SensorStrings[6] = {"PR1\r\n", "PR2\r\n", "PR3\r\n", "PR4\r\n", "PR5\r\n", "PR6\r\n"};


   if (flush_flag) {
      flush_flag = 0;
      flush();
   }

   /* read parameters once every 3 seconds */
   if (time() > last_read + 300) {
      last_read = time();
    
      for (i=0; i<6; i++){

         //DISABLE_INTERRUPTS;
         //user_data.p[i] = 0.0;
         //ENABLE_INTERRUPTS;

         // Query sensor status and value
         //printf("PR%d\r\n", i+1);
         printf(SensorStrings[i]);
         flush();

         // Read reply device
         r = gets_wait(str, sizeof(str), 200);
         
         if (str[0] == 6) { // 6: Acknowledged query
         //if (1) {
 
            // Query sending of data
            printf("%c", 5); // ENQ
            flush();
            
            // Read sensor status and value
            r = gets_wait(str, sizeof(str), 200);

            // For diagnostics
            if (i == 0) {
               DISABLE_INTERRUPTS;
               strcpy(user_data.deb_str, str);
               ENABLE_INTERRUPTS;
            }

            // Convert characters to values
            status = atoi((char *)&str[0]);
            p = atof(str+2);

            if (status == 0 && r == 10) { // Check if the sensor is ok and the correct amount of data read
               DISABLE_INTERRUPTS;
               user_data.p[i] = p;
               ENABLE_INTERRUPTS;
            }
         }
      }
   }
}

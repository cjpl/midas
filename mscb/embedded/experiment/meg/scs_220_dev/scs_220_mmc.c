/********************************************************************\

  Name:         scs_220_mmc.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-220 RS-485 node connected to
                MEG magnet mapping machine

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>             // for atof()
#include <string.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "MMC";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

bit flush_flag;
static unsigned long last_read = 0;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float z;
   float r;
   float phi;
} idata user_data;


MSCB_INFO_VAR code vars[] = {
   4, UNIT_METER, PRFX_MILLI, 0,    MSCBF_FLOAT, "Z",      &user_data.z,
   4, UNIT_METER, PRFX_MILLI, 0,    MSCBF_FLOAT, "R",      &user_data.r,
   4, UNIT_METER, PRFX_MILLI, 0,    MSCBF_FLOAT, "PHI",  &user_data.phi,
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
   if (init);
   /* initialize UART1 */
   uart_init(1, BD_9600);
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[300], out_buf[300];

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   if (index);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
   if (index == 0);
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

char xdata str[32];

void read_axis(unsigned char axis)
{
unsigned char i, j, c;
float value;

   strcpy(str, "\002");            // STX
   sprintf(str+1, "%02Bd", axis);  // node number
   strcat(str, "00");              // subaddress
   strcat(str, "0");               // SID
   
   /* command */
   strcat(str, "01");              // MRC
   strcat(str, "01");              // SRC
   strcat(str, "C00000000001");    // data
   
   strcat(str, "\003");            // ETX
   
   /* calculate BCC */
   j = strlen(str);
   c = str[1];
   for (i=2 ; i<j ; i++)
      c = c ^ str[i];
   str[j] = c;
   str[j+1] = 0;
   
   printf(str);
   flush();
   memset(str, 0, sizeof(str));
   i = gets_wait(str, 24, 50);

   if (i > 16) {

      /* Somtimes, a wrong character slips into str if msc does a
	     read repeat. In that case F appears at str[16], so just
		 ignore the reading */
      if (str[16] == 'F')
	     return;

      value = (float) (atoi(str+16) / 10.0);
      if (str[15] == 'F')
         value = -value;

	  if (axis == 2 && value == 0)
	     i = i+1;
      DISABLE_INTERRUPTS;
      switch (axis) {
         case 0: user_data.z   = value; break;
         case 1: user_data.r   = value; break;
         case 2: user_data.phi = value; break;
      }
      ENABLE_INTERRUPTS;
   }
   
}

/* external watchdog */
sbit EWD = P0 ^ 7;

void user_loop(void)
{
   static unsigned char axis = 0;

   read_axis(axis);
   axis = (axis+1) % 3;

   /* toggle external watchdog */
   EWD = !EWD;
}

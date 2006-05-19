/********************************************************************\

  Name:         splitter.c
  Created by:   Stefan Ritt


  Contents:     Firmware for MEG splitter produced in Lecce

  $Id: scs_1001.c 3104 2006-05-15 15:22:03Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "Splitter";
char code svn_revision[] = "$Id: splitter.c 3104 2006-05-15 15:22:03Z ritt $";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

/*---- Port definitions ----*/

sbit DAC_DIN  = P3 ^ 2;
sbit DAC_SCK  = P3 ^ 1;
sbit DAC_NCS  = P3 ^ 0;
sbit ENABLE   = P0 ^ 7;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char enable;
   float dac;
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Enable",  &user_data.enable },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC",     &user_data.dac },

   { 0 }
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void write_dac(float value) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   unsigned char i;


   /* open drain(*) /push-pull: 
      P0.0 TX1      P3.0 SSYNC
      P0.1*RX1      P3.1 SCLK
      P0.2 TX2      P3.2 SOUT
      P0.3*RX2      P3.3 LED
                                  
      P0.4          P3.4*TRIN0
      P0.5          P3.5*TRIN1
      P0.6          P3.6*TRIN2
      P0.7 ENABLE   P3.7*TRIN3
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xF5;
   P1MDOUT = 0xFF;
   P2MDOUT = 0xFF;
   P3MDOUT = 0x0F;

   /* initial EEPROM value */
   if (init) {
      user_data.enable = 0;
      user_data.dac    = 0;
   }

   /* write digital output and DAC */
   for (i=0 ; i<2 ; i++)
      user_write(i);
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   switch (index) {

   case 0: ENABLE = user_data.enable; break;
   case 1: write_dac(user_data.dac);  break;
   }
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
   if (index);
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

/*---- DAC function ------------------------------------------------*/

void write_dac(float value) reentrant
{
   unsigned short d;
   unsigned char i;

   d = value/2.5*65535;

   DAC_SCK = 1;
   DELAY_US(1);
   DAC_NCS = 0; // chip select

   for (i=0 ; i<8 ; i++) {
      DAC_SCK = 1;
      DELAY_US(1);
      DAC_DIN = 0; // normal operation
      DELAY_US(1);
      DAC_SCK = 0;
      DELAY_US(1);
   }
   
   for (i=0 ; i<16 ; i++) {
      DAC_SCK = 1;
      DELAY_US(1);
      DAC_DIN = (d & 0x8000) > 0;
      DELAY_US(1);
      DAC_SCK = 0;
      d <<= 1;
   }

   DAC_NCS = 1; // remove chip select
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
}

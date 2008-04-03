/********************************************************************\

  Name:         scs_920.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-920 dual channel 4-wire precision 
                voltage source

  $Id:$

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "Voltage Calib";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

/* AD5063 pins */
sbit DAC1_SCLK   = P1 ^ 1;        // Serial Clock
sbit DAC1_SYNC_N = P1 ^ 2;        // !Sync
sbit DAC1_DIN    = P1 ^ 0;        // Data in

sbit DAC2_SCLK   = P1 ^ 5;        // Serial Clock
sbit DAC2_SYNC_N = P1 ^ 6;        // !Sync
sbit DAC2_DIN    = P1 ^ 4;        // Data in

/* OPA569A enable pin */
sbit OPA_EN1     = P1 ^ 3;
sbit OPA_EN2     = P1 ^ 7;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float u1;
   float u2;
   char  disable1;
   char  disable2;
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT, "U1", &user_data.u1  },
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT, "U2", &user_data.u2  },

   { 1, UNIT_BOOLEAN, 0, 0, 0,           "Disable1", &user_data.disable1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "Disable2", &user_data.disable2 },

   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void write_adc(unsigned char a, unsigned char d);

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   DAC0CN = 0x00;               // disable DAC0
   DAC1CN = 0x00;               // disable DAC1
   REF0CN = 0x00;               // disenable internal reference

   /* P1 on push-pull */
   PRT1CF = 0xFF;
   P1 = 0;

   /* initial EEPROM value */
   if (init) {

      user_data.u1 = 0;
      user_data.u2 = 0;
   }

   /* write DACs */
   user_write(0);
   user_write(1);
   user_write(2);
   user_write(3);

}

#pragma NOAREGS

/*---- DAC write ---------------------------------------------------*/

void write_dac1(float voltage)
{
unsigned char i;
unsigned short d;

   d = voltage / 2.5 * 65535;

   DAC1_SYNC_N = 0; // chip select

   /* write 8 zeros */
   DAC1_DIN = 0;
   for (i=0 ; i<8 ; i++) {
      DAC1_SCLK = 1;
      DAC1_SCLK = 0;
   }
   
   /* write 16 data bits */
   for (i=0 ; i<16 ; i++) {
      DAC1_SCLK = 1;
      DAC1_DIN  = (d & 0x8000) > 0;
      DAC1_SCLK = 0;
      d <<= 1;
   }

   DAC1_SYNC_N = 1;
}

void write_dac2(float voltage)
{
unsigned char i;
unsigned short d;

   d = voltage / 2.5 * 65535;

   DAC2_SYNC_N = 0; // chip select

   /* write 8 zeros */
   DAC2_DIN = 0;
   for (i=0 ; i<8 ; i++) {
      DAC2_SCLK = 1;
      DAC2_SCLK = 0;
   }
   
   /* write 16 data bits */
   for (i=0 ; i<16 ; i++) {
      DAC2_SCLK = 1;
      DAC2_DIN  = (d & 0x8000) > 0;
      DAC2_SCLK = 0;
      d <<= 1;
   }

   DAC2_SYNC_N = 1;
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if (index == 0)
     write_dac1(user_data.u1);
   if (index == 1)
     write_dac2(user_data.u2);
   if (index == 2)
     OPA_EN1 = user_data.disable1;
   if (index == 3)
     OPA_EN2 = user_data.disable2;
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

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
}

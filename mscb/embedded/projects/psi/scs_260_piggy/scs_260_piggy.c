/********************************************************************\

  Name:         scs_260_piggy.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-260 Piggy Back device

  $Id: scs_260_piggy.c 4906 2010-12-09 10:38:33Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-260-PIGGY";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern lcd_menu(void);

/*---- Port definitions ----*/

sbit DOUT0    = P3 ^ 0;
sbit DOUT1    = P3 ^ 1;
sbit DOUT2    = P3 ^ 2;
sbit DOUT3    = P3 ^ 3;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float adc[4];
   unsigned char dout[4];
   float aofs[4];
   float again[4];
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC0",    &user_data.adc[0] },  // 0
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC1",    &user_data.adc[1] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC2",    &user_data.adc[2] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC3",    &user_data.adc[3] },

   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout0",   &user_data.dout[0] }, // 4
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout1",   &user_data.dout[1] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout2",   &user_data.dout[2] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout3",   &user_data.dout[3] },

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS0",  &user_data.aofs[0] }, // 8
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS1",  &user_data.aofs[1] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS2",  &user_data.aofs[2] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS3",  &user_data.aofs[3] },

   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN0", &user_data.again[0] }, // 12
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN1", &user_data.again[1] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN2", &user_data.again[2] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN3", &user_data.again[3] },

   { 0 }
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

   led_mode(2, 0);              // buzzer off by default

   SFRPAGE = ADC0_PAGE;
   AMX0CF = 0x00;               // select single ended analog inputs
   ADC0CF = 0xE0;               // 16 system clocks, gain 1
   ADC0CN = 0x80;               // enable ADC 
   REF0CN = 0x00;               // use external voltage reference

   SFRPAGE = LEGACY_PAGE;
   REF0CN = 0x02;               // select external voltage reference

   /* everything push-pull except RX */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xFD;
   P1MDOUT = 0xFF;
   P2MDOUT = 0xFF;
   P3MDOUT = 0xFF;

   /* initial EEPROM value */
   if (init) {
      for (i=0 ; i<4 ; i++)
         user_data.dout[i] = 0;

      for (i=0 ; i<4 ; i++) {
         user_data.aofs[i] = 0;
         user_data.again[i] = 1;
      }
   }

   /* write digital outputs */
   for (i=4 ; i<8 ; i++)
      user_write(i);
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   switch (index) {

   case 4: DOUT0 = user_data.dout[0]; break;
   case 5: DOUT1 = user_data.dout[1]; break;
   case 6: DOUT2 = user_data.dout[2]; break;
   case 7: DOUT3 = user_data.dout[3]; break;

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

/*---- ADC read function -------------------------------------------*/

void adc_read(channel, float *d)
{
   unsigned long value;
   unsigned int i, n;
   float gvalue;

   SFRPAGE = ADC0_PAGE;
   AMX0SL = channel & 0x0F;
   ADC0CF = 0xE0;               // 16 system clocks, gain 1

   n = 1 << 10;                 // average 1024 times

   value = 0;
   for (i = 0; i < n; i++) {
      DISABLE_INTERRUPTS;
  
      SFRPAGE = ADC0_PAGE;
      AD0INT = 0;
      AD0BUSY = 1;
      while (!AD0INT);          // wait until conversion ready, does NOT work with ADBUSY!

      ENABLE_INTERRUPTS;

      value += (ADC0L | (ADC0H << 8));
      yield();
   }

   value >>= 6;

   gvalue = value / 65536.0 * 20 - 10; // +- 10V range

   gvalue -= user_data.aofs[channel];
   gvalue *= user_data.again[channel];

   // round to two significant digits
   gvalue = (long)(gvalue*1E2+0.5)/1E2;

   DISABLE_INTERRUPTS;
   *d = gvalue;
   ENABLE_INTERRUPTS;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   static unsigned char adc_chn = 0;
 
   /* read one ADC channel */
   adc_read(adc_chn, &user_data.adc[adc_chn]);
   adc_chn = (adc_chn + 1) % 4;
}

/********************************************************************\

  Name:         scs_1000.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-1000 stand alone control unit

  $Log$
  Revision 1.1  2004/07/09 07:48:10  midas
  Initial revision


\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-1000";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float adc[8];
   float dac[2];
} xdata user_data;

MSCB_INFO_VAR code variables[] = {

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC0", &user_data.adc[0] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC1", &user_data.adc[1] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC2", &user_data.adc[2] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC3", &user_data.adc[3] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC4", &user_data.adc[4] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC5", &user_data.adc[5] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC6", &user_data.adc[6] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC7", &user_data.adc[7] },

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC0", &user_data.dac[0], 0, 10, 0.1 },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC1", &user_data.dac[1], 0, 10, 0.1 },

   { 0 }
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

void user_init(unsigned char init)
{
   AMX0CF = 0x00;               // select single ended analog inputs
   ADC0CF = 0xE0;               // 16 system clocks, gain 1
   ADC0CN = 0x80;               // enable ADC 

   REF0CN = 0x03;               // enable internal reference
   DAC0CN = 0x80;               // enable DAC0
   DAC1CN = 0x80;               // enable DAC1

   /* push-pull:
      P0.0    TX
      P0.1
      P0.2
      P0.3

      P0.4    SR_CLOCK
      P0.5    SR_STROBE
      P0.6    SR_DATA 
      P0.7
    */
   P0MDOUT = 0x01;              // P0.0: TX = Push Pull
   P1MDOUT = 0x00;              // P1: LPT
   P2MDOUT = 0x00;              // P2: LPT
   P3MDOUT = 0xE0;              // P3.5,6,7: Optocouplers

   /* initial EEPROM value */
   if (init) {
      user_data.dac[0] = 0;
      user_data.dac[1] = 0;
   }

   /* write DACs */
   user_write(8);
   user_write(9);
   user_write(10);
   user_write(12);
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   unsigned short d;

   switch (index) {
   case 8:                     // DAC0
      /* assume -10V..+10V range */
      d = ((user_data.dac[0] + 10) / 20) * 0x1000;
      if (d >= 0x1000)
         d = 0x0FFF;

      DAC0L = d & 0xFF;
      DAC0H = d >> 8;
      break;

   case 9:                     // DAC1
      /* assume -10V..+10V range */
      d = ((user_data.dac[1] + 10) / 20) * 0x1000;
      if (d >= 0x1000)
         d = 0x0FFF;

      DAC1L = d & 0xFF;
      DAC1H = d >> 8;
      break;
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

   AMX0SL = channel & 0x0F;
   ADC0CF = 0xE0;               // 16 system clocks, gain 1

   n = 1 << 12;                 // average 4096 times

   value = 0;
   for (i = 0; i < n; i++) {
      DISABLE_INTERRUPTS;

      AD0INT = 0;
      AD0BUSY = 1;
      while (!AD0INT);          // wait until conversion ready, does NOT work with ADBUSY!

      ENABLE_INTERRUPTS;

      value += (ADC0L | (ADC0H << 8));
      yield();
   }

   value >>= 8;

   if (channel < 2)
      gvalue = value / 65536.0 * 20 - 10; // +- 10V range
   else
      gvalue = value / 65536.0 * 10; // 0...10V range

   DISABLE_INTERRUPTS;
   *d = gvalue;
   ENABLE_INTERRUPTS;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   static unsigned char adc_chn = 0;
 
   adc_read(adc_chn, &user_data.adc[adc_chn]);
   adc_chn = (adc_chn + 1) % 8;
}

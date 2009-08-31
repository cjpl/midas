/********************************************************************\

  Name:         scs_900_psupply.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-900 single channel power supply (2A)

  $Id$

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

/* AD7718 pins */
sbit ADC_P1   = P1 ^ 1;         // P1
sbit ADC_NRES = P1 ^ 2;         // !Reset
sbit ADC_SCLK = P1 ^ 3;         // Serial Clock
sbit ADC_NCS  = P1 ^ 4;         // !Chip select
sbit ADC_NRDY = P1 ^ 5;         // !Ready
sbit ADC_DOUT = P1 ^ 6;         // Data out
sbit ADC_DIN  = P1 ^ 7;         // Data in

/* LTC2600 pins */
sbit DAC_NCS  = P0 ^ 3;         // !Chip select
sbit DAC_SCK  = P0 ^ 4;         // Serial Clock
sbit DAC_CLR  = P0 ^ 5;         // Clear
sbit DAC_DOUT = P0 ^ 6;         // Data out
sbit DAC_DIN  = P0 ^ 7;         // Data in

/* AD7718 registers */
#define REG_STATUS     0
#define REG_MODE       1
#define REG_CONTROL    2
#define REG_FILTER     3
#define REG_ADCDATA    4
#define REG_ADCOFFSET  5
#define REG_ADCGAIN    6
#define REG_IOCONTROL  7

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float adc;
   float demand;
   unsigned char disable;

   float dac;

   float oadc;
   float gadc;
} xdata user_data;

MSCB_INFO_VAR code variables[] = {

   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT, "ADC", &user_data.adc  },
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT, "Demand", &user_data.demand  },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "Disable", &user_data.disable  },

   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DAC",  &user_data.dac  },
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC", &user_data.oadc },
   { 4, UNIT_FACTOR,  0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC", &user_data.gadc },

   0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

unsigned short iadc_read(channel);
void user_write(unsigned char index) reentrant;
void write_adc(unsigned char a, unsigned char d);

unsigned char new_count;

unsigned char adc_chn = 0;

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   DAC0CN = 0x00;               // disable DAC0
   DAC1CN = 0x00;               // disable DAC1
   REF0CN = 0x00;               // disenable internal reference

   /* push-pull:
      P0.0    TX
      P0.1
      P0.2    SW_ADC
      P0.3    DAC_NCS

      P0.4    DAC_SCK
      P0.5    DAC_DIN
      P0.6 
      P0.7    DAC_CLR
    */
   PRT0CF = 0xBD;
   P0 = 0xFF;

   /* push-pull:
      P1.0    SW_DAC
      P1.1
      P1.2    ADC_NRES
      P1.3    ADC_SCLK

      P1.4    ADC_NCS
      P1.5    
      P1.6 
      P1.7    ADC_DIN
    */
   PRT1CF = 0x9D;
   P1 = 0xFF;

   /* initial EEPROM value */
   if (init) {

      user_data.demand = 0;
      user_data.disable = 0;
      user_data.dac = 0;
      user_data.oadc = 0;
      user_data.gadc = 1;
   }

   new_count = 0;

   /* set-up DAC & ADC */
   DAC_CLR = 1;
   ADC_NRES = 1;
   write_adc(REG_FILTER, 13);                   // SF value for 105Hz rejection
   write_adc(REG_MODE, 3);                      // continuous conversion
   write_adc(REG_CONTROL, adc_chn << 4 | 0x0F); // Chn. 1, +2.56V range

   /* write DAC */
   user_write(1);
}

#pragma NOAREGS

/*---- Internal ADC ------------------------------------------------*/

unsigned short iadc_read(channel)
{
   unsigned short value;

   REF0CN = 0x02;               // voltage reference bias enable
   AMX0CF = 0x00;               // select single ended analog inputs
   AMX0SL = channel & 0x0F;
   ADC0CF = 0xE0;               // 16 system clocks, gain 1

   DISABLE_INTERRUPTS;

   ADCINT = 0;
   ADBUSY = 1;
   while (!ADCINT);          // wait until conversion ready, does NOT work with ADBUSY!

   ENABLE_INTERRUPTS;
   yield();

   value = (ADC0L | (ADC0H << 8));
   return value;
}

/*---- DAC functions -----------------------------------------------*/

void write_dac_cmd(unsigned char cmd, unsigned char adr, unsigned short d)
{
unsigned char i, m, b;

   DAC_NCS = 0; // chip select

   for (i=0,m=8 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (cmd & m) > 0;
      DAC_SCK = 1;
      m >>= 1;
   }
   
   for (i=0,m=8 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (adr & m) > 0;
      DAC_SCK = 1;
      m >>= 1;
   }

   b = d >> 8;
   for (i=0,m=0x80 ; i<8 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (b & m) > 0;
      DAC_SCK = 1;
      m >>= 1;
   }
   b = d & 0xFF;
   for (i=0,m=0x80 ; i<8 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (b & m) > 0;
      DAC_SCK = 1;
      m >>= 1;
   }

   DAC_NCS = 1; // remove chip select
}

void write_dac() reentrant
{
   unsigned short d;

   if (user_data.dac < 0)
      user_data.dac = 0;

   if (user_data.dac > 2.5)
      user_data.dac = 2.5;

   d = user_data.dac / 2.5 * 65535 + 0.5;

   /* do mapping */
   write_dac_cmd(0x3, 4, d);
}

/*---- ADC functions -----------------------------------------------*/

void write_adc(unsigned char a, unsigned char d)
{
   unsigned char i, m;

   /* write to communication register */

   ADC_NCS = 0;
   
   /* write zeros to !WEN and R/!W */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = 0;
      ADC_SCLK = 1;
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (a & m) > 0;
      ADC_SCLK = 1;
      m >>= 1;
   }

   ADC_NCS = 1;

   /* write to selected data register */

   ADC_NCS = 0;

   for (i=0,m=0x80 ; i<8 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (d & m) > 0;
      ADC_SCLK = 1;
      m >>= 1;
   }

   ADC_NCS = 1;
}

void read_adc8(unsigned char a, unsigned char *d)
{
   unsigned char i, m;

   /* write to communication register */

   ADC_NCS = 0;
   
   /* write zero to !WEN and one to R/!W */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (i == 1);
      ADC_SCLK = 1;
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (a & m) > 0;
      ADC_SCLK = 1;
      m >>= 1;
   }

   ADC_NCS = 1;

   /* read from selected data register */

   ADC_NCS = 0;

   for (i=0,m=0x80,*d=0 ; i<8 ; i++) {
      ADC_SCLK = 0;
      if (ADC_DOUT)
         *d |= m;
      ADC_SCLK = 1;
      m >>= 1;
   }

   ADC_NCS = 1;
}

void read_adc24(unsigned char a, unsigned long *d)
{
   unsigned char i, m;

   /* write to communication register */

   ADC_NCS = 0;
   
   /* write zero to !WEN and one to R/!W */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (i == 1);
      ADC_SCLK = 1;
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (a & m) > 0;
      ADC_SCLK = 1;
      m >>= 1;
   }

   ADC_NCS = 1;

   /* read from selected data register */

   ADC_NCS = 0;

   for (i=0,*d=0 ; i<24 ; i++) {
      *d <<= 1;
      ADC_SCLK = 0;
      *d |= ADC_DOUT;
      ADC_SCLK = 1;
   }

   ADC_NCS = 1;
}

unsigned char adc_read()
{
   unsigned long value;
   float gvalue;

   if (ADC_NRDY)
      return 0;

   read_adc24(REG_ADCDATA, &value);

   /* convert to volts */
   gvalue = ((float)value / (1l<<24)) * 2.56;

   /* round result to 5 digits */
   gvalue = floor(gvalue*1E5+0.5)/1E5;

   /* apply corrections */
   gvalue = (gvalue - user_data.oadc) * user_data.gadc;

   DISABLE_INTERRUPTS;
   user_data.adc = gvalue;
   ENABLE_INTERRUPTS;

   /* start next conversion */
   write_adc(REG_CONTROL, 6 << 4 | 0x0F); // adc_chn, +2.56V range

   return 1;
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if (index == 1) {
      new_count = 0;
   }

   if (index == 2) {
      if (new_count > 2) {
         if (user_data.disable)
            write_dac_cmd(0x3, 4, 0);
         else
            write_dac();
      }
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

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   if (adc_read()) {
      if (new_count < 6) {
         /* do correction six times */
         user_data.dac += (user_data.demand - user_data.adc);
         write_dac();
         new_count++;
      }
   }
}

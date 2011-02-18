/********************************************************************\

  Name:         scs_900_psupply.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-900 eight channel gauss meter (H. Luetkens)

  $Id: scs_900_psupply.c 2753 2005-10-07 14:55:31Z ritt $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "Gaussmeter";

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

/* analog switch */
sbit UNI_DAC = P1 ^ 0;          // DAC bipolar/unipolar
sbit UNI_ADC = P0 ^ 2;          // ADC bipolar/unipolar

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
   float adc[8];
   float dac[8];
   char  uni_dac;
   char  uni_adc;
   char  adc_25;

   float oadc[8];
   float gadc[8];
   float odac[8];
   float gdac[8];
} xdata user_data;

MSCB_INFO_VAR code variables[] = {

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC0", &user_data.adc[0]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC1", &user_data.adc[1]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC2", &user_data.adc[2]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC3", &user_data.adc[3]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC4", &user_data.adc[4]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC5", &user_data.adc[5]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC6", &user_data.adc[6]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC7", &user_data.adc[7]  },

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC0", &user_data.dac[0]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC1", &user_data.dac[1]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC2", &user_data.dac[2]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC3", &user_data.dac[3]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC4", &user_data.dac[4]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC5", &user_data.dac[5]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC6", &user_data.dac[6]  },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC7", &user_data.dac[7]  },

   { 1, UNIT_BOOLEAN, 0, 0, 0, "Uni DAC",     &user_data.uni_dac },
   { 1, UNIT_BOOLEAN, 0, 0, 0, "Uni ADC",     &user_data.uni_adc },
   { 1, UNIT_BOOLEAN, 0, 0, 0, "2.5V ADC",    &user_data.adc_25  },

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC0", &user_data.oadc[0] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC1", &user_data.oadc[1] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC2", &user_data.oadc[2] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC3", &user_data.oadc[3] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC4", &user_data.oadc[4] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC5", &user_data.oadc[5] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC6", &user_data.oadc[6] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "OADC7", &user_data.oadc[7] },

   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC0", &user_data.gadc[0] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC1", &user_data.gadc[1] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC2", &user_data.gadc[2] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC3", &user_data.gadc[3] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC4", &user_data.gadc[4] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC5", &user_data.gadc[5] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC6", &user_data.gadc[6] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GADC7", &user_data.gadc[7] },

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC0", &user_data.odac[0] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC1", &user_data.odac[1] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC2", &user_data.odac[2] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC3", &user_data.odac[3] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC4", &user_data.odac[4] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC5", &user_data.odac[5] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC6", &user_data.odac[6] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ODAC7", &user_data.odac[7] },

   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC0", &user_data.gdac[0] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC1", &user_data.gdac[1] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC2", &user_data.gdac[2] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC3", &user_data.gdac[3] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC4", &user_data.gdac[4] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC5", &user_data.gdac[5] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC6", &user_data.gdac[6] },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "GDAC7", &user_data.gdac[7] },

   0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

unsigned short iadc_read(channel);
void user_write(unsigned char index) reentrant;
void write_adc(unsigned char a, unsigned char d);

unsigned char adc_chn = 0;

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

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

      for (i = 0; i < 8; i++) {
         user_data.dac[i] = 0;

         user_data.odac[i] = 0;
         user_data.gdac[i] = 1;
         user_data.oadc[i] = 0;
         user_data.gadc[i] = 1;
      }
   }

   /* set-up DAC & ADC */
   DAC_CLR = 1;
   ADC_NRES = 1;
   write_adc(REG_FILTER, 82);                   // SF value for 50Hz rejection
   write_adc(REG_MODE, 3);                      // continuous conversion
   write_adc(REG_CONTROL, adc_chn << 4 | 0x0F); // Chn. 1, +2.56V range

   /* read configuration jumper */

   ADC0CN = 0x80;               // enable ADC 
   delay_ms(100);

   user_data.uni_adc = iadc_read(4) > 0x800;     // JU10
   user_data.adc_25  = iadc_read(2) > 0x800;     // JU11
   user_data.uni_dac = iadc_read(0) > 0x800;     // JU12

   ADC0CN = 0x00;               // disable ADC 

   /* set analog switch */
   user_write(16);
   user_write(17);

   /* write DACs */
   for (i=0 ; i<8 ; i++)
      user_write(i+8);
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

unsigned char code dac_index[8] = {4, 5, 2, 3, 1, 0, 7, 6};

void write_dac(unsigned char index) reentrant
{
   unsigned short d;
   float v;

   /* apply correction */
   v = (user_data.dac[index]  - user_data.odac[index]) * 
        user_data.gdac[index];

   if (user_data.uni_dac) {
      if (v < 0) {
         user_data.dac[index] = 0;
         v = 0;
      }

      if (user_data.adc_25 == 0) {
         if (v > 10) {
            user_data.dac[index] = 10;
            v = 10;
         }
   
         d = v / 10 * 65535 + 0.5;
      } else {
         if (v > 2.5) {
            user_data.dac[index] = 2.5;
            v = 2.5;
         }
   
         d = v / 2.5 * 65535 + 0.5;
      }
   } else {
      
      if (v < -10)
         v = -10;
  
      if (v > 10)
         v = 10;

      d = (v + 10) / 20 * 65535 + 0.5;
   }

   /* do mapping */
   index = dac_index[index % 8];
   write_dac_cmd(0x3, index, d);
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

unsigned char code adc_index[8] = {6, 7, 0, 1, 2, 3, 4, 5};

void adc_read()
{
   unsigned char i;
   unsigned long value;
   float gvalue;

   if (ADC_NRDY)
       return;

   read_adc24(REG_ADCDATA, &value);

   /* convert to volts */
   gvalue = ((float)value / (1l<<24)) * 2.56;

   /* round result to 5 digits */
   gvalue = floor(gvalue*1E5+0.5)/1E5;

   /* apply range */
   if (user_data.adc_25 == 0) {
      if (user_data.uni_adc)
         gvalue *= 10.0/2.56;
      else
         gvalue = gvalue/2.56*20.0 - 10;
   }

   /* apply corrections */
   gvalue = (gvalue - user_data.oadc[adc_chn]) * user_data.gadc[adc_chn];

   DISABLE_INTERRUPTS;
   user_data.adc[adc_chn] = gvalue;
   ENABLE_INTERRUPTS;

   /* start next conversion */
   adc_chn = (adc_chn + 1) % 8;
   i = adc_index[adc_chn];
   write_adc(REG_CONTROL, i << 4 | 0x0F); // adc_chn, +2.56V range
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if (index > 7 && index < 16)
      write_dac(index-8);

   if (index == 16)
      UNI_DAC = user_data.uni_dac;

   if (index == 17)
      UNI_ADC = !user_data.uni_adc;
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

/*---- LCD output --------------------------------------------------*/

void lcd_output()
{
   lcd_goto(0, 0);
   printf("X1%+5.2f  X2%+5.2f", user_data.adc[0], user_data.adc[4]);
   lcd_goto(0, 1);
   printf("Y1%+5.2f  Y2%+5.2f", user_data.adc[1], user_data.adc[5]);
   lcd_goto(0, 2);
   printf("Z1%+5.2f  Z2%+5.2f", user_data.adc[2], user_data.adc[6]);
   lcd_goto(0, 3);
   printf("E1%+5.2f  E2%+5.2f", user_data.adc[3], user_data.adc[7]);
}

/*---- User loop function ------------------------------------------*/

extern bit lcd_present;

void user_loop(void)
{
static xdata unsigned long last = 0;

   adc_read();

   if (time() > last+100) {
      last = time();
      lcd_output();
   }
}

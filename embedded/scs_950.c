/********************************************************************\

  Name:         scs_950.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-950 20-chn analog in

  $Log$
  Revision 1.2  2004/07/09 14:10:04  midas
  Added multi-ADC code

  Revision 1.1  2004/07/09 07:48:10  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-950";

#define UNIPOLAR

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

sbit EXT_WATCHDOG = P0 ^ 3;

/* AD7718 pins */
sbit ADC_SCLK = P1 ^ 6;         // Serial Clock
sbit ADC_DIN  = P1 ^ 7;         // Data in

sbit AD1_NCS  = P1 ^ 0;         // !Chip select
sbit AD2_NCS  = P1 ^ 1;         // !Chip select
sbit AD3_NCS  = P1 ^ 2;         // !Chip select
sbit AD4_NCS  = P1 ^ 3;         // !Chip select

sbit AD1_NRDY = P1 ^ 5;         // !Ready
sbit AD2_NRDY = P1 ^ 5;         // !Ready
sbit AD3_NRDY = P1 ^ 5;         // !Ready
sbit AD4_NRDY = P1 ^ 5;         // !Ready

sbit AD1_DOUT = P0 ^ 4;         // Data out
sbit AD2_DOUT = P0 ^ 5;         // Data out
sbit AD3_DOUT = P0 ^ 6;         // Data out
sbit AD4_DOUT = P0 ^ 7;         // Data out


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
#ifdef UNIPOLAR
   float adc[40];
#else
   float adc[20];
#endif
} xdata user_data;

MSCB_INFO_VAR code variables[] = {

   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC00", &user_data.adc[0],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC01", &user_data.adc[1],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC02", &user_data.adc[2],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC03", &user_data.adc[3],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC04", &user_data.adc[4],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC05", &user_data.adc[5],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC06", &user_data.adc[6],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC07", &user_data.adc[7],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC08", &user_data.adc[8],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC09", &user_data.adc[9],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC10", &user_data.adc[10],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC11", &user_data.adc[11],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC12", &user_data.adc[12],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC13", &user_data.adc[13],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC14", &user_data.adc[14],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC15", &user_data.adc[15],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC16", &user_data.adc[16],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC17", &user_data.adc[17],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC18", &user_data.adc[18],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC19", &user_data.adc[19],

#ifdef UNIPOLAR
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC20", &user_data.adc[20],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC21", &user_data.adc[21],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC22", &user_data.adc[22],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC23", &user_data.adc[23],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC24", &user_data.adc[24],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC25", &user_data.adc[25],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC26", &user_data.adc[26],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC27", &user_data.adc[27],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC28", &user_data.adc[28],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC29", &user_data.adc[29],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC30", &user_data.adc[30],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC31", &user_data.adc[31],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC32", &user_data.adc[32],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC33", &user_data.adc[33],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC34", &user_data.adc[34],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC35", &user_data.adc[35],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC36", &user_data.adc[36],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC37", &user_data.adc[37],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC38", &user_data.adc[38],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC39", &user_data.adc[39],
#endif

   0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void write_adc(unsigned char n, unsigned char a, unsigned char d);

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

   AMX0CF = 0x00;               // select single ended analog inputs
   ADC0CF = 0xE0;               // 16 system clocks, gain 1
   ADC0CN = 0x80;               // enable ADC 
   REF0CN = 0x03;               // enable internal reference

   DAC0CN = 0x00;               // disable DAC0
   DAC1CN = 0x00;               // disable DAC1

   /* push-pull:
      P0.0    TX
      P0.1
      P0.2
      P0.3    DAC_NCS

      P0.4    DAC_SCK
      P0.5    DAC_DIN
      P0.6 
      P0.7    DAC_CLR
    */
   PRT0CF = 0xB9;
   P0 = 0xFF;

   /* push-pull:
      P1.0    
      P1.1
      P1.2    ADC_NRES
      P1.3    ADC_SCLK

      P1.4    ADC_NCS
      P1.5    
      P1.6 
      P1.7    ADC_DIN
    */
   PRT1CF = 0x9C;
   P1 = 0xFF;

   /* initial EEPROM value */
   if (init) {
   }

   /* set-up DAC & ADC */
   for (i=0 ; i<3 ; i++) {
      write_adc(i, REG_FILTER, 82);                   // SF value for 50Hz rejection
      write_adc(i, REG_MODE, 0x13);                   // continuous conversion, 10 channels
      write_adc(i, REG_CONTROL, 0x0F);                // start first conversion
   }
}

#pragma NOAREGS

/*---- ADC functions -----------------------------------------------*/

void set_adc_ncs(unsigned char n, unsigned char d)
{
   switch (n) {
      case 0: AD1_NCS = d; break;
      case 1: AD2_NCS = d; break;
      case 2: AD3_NCS = d; break;
      case 3: AD4_NCS = d; break;
   }
}

unsigned char get_adc_dout(unsigned char n)
{
   switch (n) {
      case 0: return AD1_DOUT;
      case 1: return AD1_DOUT;
      case 2: return AD1_DOUT;
      case 3: return AD1_DOUT;
   }
}

unsigned char get_adc_nrdy(unsigned char n)
{
   unsigned int value;

   AMX0SL = n;
   ADCINT = 0;
   ADBUSY = 1;
   while (!ADCINT);

   value = (ADC0L | (ADC0H << 8));
   return (value > 2048);
}

void write_adc(unsigned char n, unsigned char a, unsigned char d)
{
   unsigned char i, m;

   /* write to communication register */

   set_adc_ncs(n, 0);
   
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

   set_adc_ncs(n, 1);

   /* write to selected data register */

   set_adc_ncs(n, 0);

   for (i=0,m=0x80 ; i<8 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (d & m) > 0;
      ADC_SCLK = 1;
      m >>= 1;
   }

   set_adc_ncs(n, 1);
}

void read_adc24(unsigned char n, unsigned char a, unsigned long *d)
{
   unsigned char i, m;

   /* write to communication register */

   set_adc_ncs(n, 0);
   
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

   set_adc_ncs(n, 1);

   /* read from selected data register */

   set_adc_ncs(n, 0);

   for (i=0,*d=0 ; i<24 ; i++) {
      *d <<= 1;
      ADC_SCLK = 0;
      *d |= get_adc_dout(n);
      ADC_SCLK = 1;
   }

   set_adc_ncs(n, 1);
}

#ifdef UNIPOLAR
unsigned char code adc_index[] = {14, 15, 4, 5, 2, 3, 0, 1, 6, 7};
#else
unsigned char code adc_index[] = {12, 10, 9, 8, 11};
#endif

void adc_read(unsigned char iadc, unsigned char ichn)
{
   unsigned char i, ivalue;
   unsigned long value;
   float gvalue;

   if (get_adc_nrdy(iadc))
       return;

   read_adc24(iadc, REG_ADCDATA, &value);

#ifdef UNIPOLAR
   ivalue = iadc * 10 + ichn;
#else
   ivalue = iadc * 5 + ichn;
#endif

   /* convert to volts */
   gvalue = ((float)value / (1l<<24)) * 2.56;

   /* round result to 5 digits */
   gvalue = floor(gvalue*1E5+0.5)/1E5;

   DISABLE_INTERRUPTS;
   user_data.adc[ivalue] = gvalue;
   ENABLE_INTERRUPTS;

   /* start next conversion */
   i = adc_index[ichn];
   write_adc(iadc, REG_CONTROL, i << 4 | 0x0F); // adc_chn, +2.56V range
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if (index);
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
   static unsigned char ichn;

#ifdef UNIPOLAR   
   adc_read(ichn / 10, ichn % 10);
   ichn = (ichn + 1) % 40;
#else
   adc_read(ichn / 5, ichn % 5);
   ichn = (ichn + 1) % 20;
#endif
}

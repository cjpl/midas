/********************************************************************\

  Name:         scs_950.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-950 20-chn analog in

  $Log$
  Revision 1.1  2004/07/09 07:48:10  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-950";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

sbit UNI_BIP = P0 ^ 2;          // Unipolar/bipolar DAC switch

/* AD7718 pins */
sbit ADC_P2   = P1 ^ 0;         // P2
sbit ADC_P1   = P1 ^ 1;         // P1
sbit ADC_NRES = P1 ^ 2;         // !Reset
sbit ADC_SCLK = P1 ^ 3;         // Serial Clock
sbit ADC_NCS  = P1 ^ 4;         // !Chip select
sbit ADC_NRDY = P1 ^ 5;         // !Ready
sbit ADC_DOUT = P1 ^ 6;         // Data out
sbit ADC_DIN  = P1 ^ 7;         // Data in

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
   float adc[40];
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

   0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void write_adc(unsigned char a, unsigned char d);

unsigned char adc_chn = 0;

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

void user_init(unsigned char init)
{
   ADC0CN = 0x00;               // disable ADC 
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
   ADC_NRES = 1;
   write_adc(REG_FILTER, 82);                   // SF value for 50Hz rejection
   write_adc(REG_MODE, 3);                      // continuous conversion
   write_adc(REG_CONTROL, adc_chn << 4 | 0x0F); // Chn. 1, +2.56V range
}

#pragma NOAREGS

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

   DISABLE_INTERRUPTS;
   user_data.adc[adc_chn] = gvalue;
   ENABLE_INTERRUPTS;

   /* start next conversion */
   adc_chn = (adc_chn + 1) % 8;
   i = adc_index[adc_chn];
   write_adc(REG_CONTROL, i << 4 | 0x0F); // adc_chn, +2.56V range
}

void adc_calib()
{
   unsigned char i;

   for (i=0 ; i<8 ; i++) {

      write_adc(REG_CONTROL, i << 4 | 0x0F);

      /* zero calibration */
      write_adc(REG_MODE, 4);
      while (ADC_NRDY) led_blink(1, 1, 100);

      /* full scale calibration */
      write_adc(REG_MODE, 5);
      while (ADC_NRDY) led_blink(1, 1, 100);
   }

   /* restart continuous conversion */
   write_adc(REG_MODE, 3);
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
   adc_read();
}

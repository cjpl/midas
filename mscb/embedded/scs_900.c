/********************************************************************\

  Name:         scs_900.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-900 analog high precision I/O 

  $Log$
  Revision 1.1  2004/02/05 16:30:54  midas
  DAC works

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-900";

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
   float adc[8];
   float dac[8];
/*
   char  dac_ofs[8];            // channel offset in mV
   char  dac_gain[8];           // channel gain
*/
   char  dac_bip_ofs;           // bipolar offset in 10mV
   char  mode;                  // 0=unipolar, 1=bipolar
} idata user_data;

MSCB_INFO_VAR code variables[] = {

   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC0", &user_data.adc[0],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC1", &user_data.adc[1],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC2", &user_data.adc[2],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC3", &user_data.adc[3],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC4", &user_data.adc[4],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC5", &user_data.adc[5],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC6", &user_data.adc[6],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC7", &user_data.adc[7],

   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC0", &user_data.dac[0],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC1", &user_data.dac[1],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC2", &user_data.dac[2],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC3", &user_data.dac[3],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC4", &user_data.dac[4],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC5", &user_data.dac[5],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC6", &user_data.dac[6],
   4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC7", &user_data.dac[7],

   /*
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs0", &user_data.dac_ofs[0],
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs1", &user_data.dac_ofs[1],
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs2", &user_data.dac_ofs[2],
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs3", &user_data.dac_ofs[3],
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs4", &user_data.dac_ofs[4],
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs5", &user_data.dac_ofs[5],
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs6", &user_data.dac_ofs[6],
   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "DACOfs7", &user_data.dac_ofs[7],

   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain0", &user_data.dac_gain[0],
   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain1", &user_data.dac_gain[1],
   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain2", &user_data.dac_gain[2],
   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain3", &user_data.dac_gain[3],
   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain4", &user_data.dac_gain[4],
   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain5", &user_data.dac_gain[5],
   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain6", &user_data.dac_gain[6],
   1, UNIT_FACTOR, 0, 0, MSCBF_SIGNED, "DACGain7", &user_data.dac_gain[7],
   */

   1, UNIT_VOLT, PRFX_MILLI, 0, MSCBF_SIGNED, "BipOfs", &user_data.dac_bip_ofs,

   1, UNIT_BYTE, 0, 0, 0, "Mode", &user_data.mode,

   0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void write_adc(unsigned char a, unsigned char d);

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

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

   /* push-pull:
      P1.0    
      P1.1
      P1.2    ADC_NRES
      P1.3    ADC_SCLK

      P1.4    ADC_NCS
      P1.5    
      P1.6    ADC_DOUT 
      P1.7
    */
   PRT1CF = 0x5C;

   /* initial EEPROM value */
   if (init) {

      for (i = 0; i < 8; i++) {
         user_data.dac[i] = 0;
//         user_data.dac_ofs[i] = 0;
//         user_data.dac_gain[i] = 0;
      }

      user_data.dac_bip_ofs = 0;
      user_data.mode = 0;
   }

   /* set-up DAC & ADC */
   DAC_CLR = 1;
   ADC_NRES = 1;
//   write_adc(REG_MODE, 2);

   /* write DACs and UNI_BIP */
   for (i=0 ; i<8 ; i++)
      user_write(i+8);

   user_write(33);
}

#pragma NOAREGS

/*---- ADC and DAC functions ---------------------------------------*/

void write_dac_cmd(unsigned char cmd, unsigned char adr, unsigned short d)
{
unsigned char i;

   DAC_NCS = 0; // chip select

   for (i=0 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (cmd & (1 << (3-i))) > 0;
      DAC_SCK = 1;
   }
   
   for (i=0 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (adr & (1 << (3-i))) > 0;
      DAC_SCK = 1;
   }

   for (i=0 ; i<16 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (d & (1 << (15-i))) > 0;
      DAC_SCK = 1;
   }

   DAC_NCS = 1; // remove chip select
}

unsigned char dac_index[8] = {4, 5, 2, 3, 1, 0, 7, 6};

void write_dac(unsigned char index) reentrant
{
   unsigned short d;

   d = user_data.dac[index] / 2.5 * 65535 + 0.5;

   /* do mapping */
   index = dac_index[index % 8];
   write_dac_cmd(0x3, index, d);
}

void write_adc(unsigned char a, unsigned char d)
{
   unsigned char i;

   /* write to communication register */

   ADC_NCS = 0;
   
   /* write zeros to !WEN and R/!W */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = 0;
      ADC_SCLK = 1;
   }

   /* register address */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (a & (1 << (3-i))) > 0;
      ADC_SCLK = 1;
   }

   ADC_NCS = 1;

   /* write to selected data register */

   ADC_NCS = 0;

   for (i=0 ; i<8 ; i++) {
      ADC_SCLK = 0;
      ADC_DIN  = (d & (1 << (7-i))) > 0;
      ADC_SCLK = 1;
   }

   ADC_NCS = 1;
}

void adc_read(channel, float *d)
{
   unsigned long value;
   float gvalue;

   value = 0; //## change to read ADC

   /* convert to volts */
   gvalue = value / 65536.0 * 2.5;

   DISABLE_INTERRUPTS;
   *d = gvalue;
   ENABLE_INTERRUPTS;
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if (index == 0)
      write_adc(REG_MODE, 2);
   if (index == 1)
      write_adc(REG_MODE, 0x08);

   if (index > 7 && index < 16)
      write_dac(index-8);

   if (index == 33)                    // set bipolar/unipoar
      UNI_BIP = (user_data.mode & 0x01) > 0;
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
   static unsigned char i = 0;

   adc_read(i, &user_data.adc[i]);
   i = (i + 1) % 8;
}

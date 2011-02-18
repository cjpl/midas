/*
  Name:         8XPT100.c
  Created by:   Stefan Ritt, Dietmar Krasort


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for PSI PT100-TemparaturMonitor

  $Id: scs_1001.c 4339 2008-09-30 15:20:56Z ritt@PSI.CH $

*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "PT100X8";
char code svn_revision[] = "$Id: scs_1001.c 4339 2008-09-30 15:20:56Z ritt@PSI.CH $";

// declare number of sub-addresses to framework //
unsigned char idata _n_sub_addr = 1;

//---- Port definitions ----//

#define ADDR_SWITCHES P3

sbit SHT71_1_CLK	 = P1 ^ 7;
sbit SHT71_2_CLK	 = P1 ^ 5;
sbit SHT71_3_CLK	 = P1 ^ 3;
sbit SHT71_4_CLK	 = P1 ^ 1;

sbit SHT71_1_DATA	 = P1 ^ 6;
sbit SHT71_2_DATA	 = P1 ^ 4;
sbit SHT71_3_DATA	 = P1 ^ 2;
sbit SHT71_4_DATA	 = P1 ^ 0;

sbit ADC1_DIN      = P5 ^ 5;
sbit ADC1_DOUT     = P5 ^ 4;
sbit ADC1_NRDY     = P5 ^ 3;
sbit ADC1_NCS      = P5 ^ 2;
sbit ADC1_SCLK     = P5 ^ 1;
sbit ADC1_NRES     = P5 ^ 0;

sbit ADC2_DIN      = P2 ^ 7;
sbit ADC2_DOUT     = P2 ^ 6;
sbit ADC2_NRDY     = P2 ^ 5;
sbit ADC2_NCS      = P2 ^ 4;
sbit ADC2_SCLK     = P2 ^ 3;
sbit ADC2_NRES     = P2 ^ 2;

// AD7718 registers //
#define REG_STATUS     0
#define REG_MODE       1
#define REG_CONTROL    2
#define REG_FILTER     3
#define REG_ADCDATA    4
#define REG_ADCOFFSET  5
#define REG_ADCGAIN    6
#define REG_IOCONTROL  7

//---- Define variable parameters returned to CMD_GET_INFO command ----//

// data buffer (mirrored in EEPROM) //

struct {
   float pt100[8];
   unsigned char pttyp[8];

   float sht71_hum[4];
   float sht71_temp[4];
   } xdata user_data;
  
MSCB_INFO_VAR code vars[] = {

   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_0",  		&user_data.pt100[0] }, 
   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_1",  		&user_data.pt100[1] }, 
   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_2",  		&user_data.pt100[2] }, 
   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_3",  		&user_data.pt100[3] }, 
   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_4",  		&user_data.pt100[4] }, 
   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_5",  		&user_data.pt100[5] }, 
   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_6",  		&user_data.pt100[6] }, 
   { 4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT,        "PT100_7",  		&user_data.pt100[7] }, 
   
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_0",  		&user_data.pttyp[0] }, 
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_1",  		&user_data.pttyp[1] }, 
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_2",  		&user_data.pttyp[2] }, 
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_3",  		&user_data.pttyp[3] }, 
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_4",  		&user_data.pttyp[4] }, 
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_5",  		&user_data.pttyp[5] }, 
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_6",  		&user_data.pttyp[6] }, 
   { 1, UNIT_BYTE,    0, 0,           0,        "PTTYP_7",  		&user_data.pttyp[7] }, 

   { 0 }
};

MSCB_INFO_VAR *variables = vars;

void write_adc(unsigned char a, unsigned char d);
void write_adc_2(unsigned char a, unsigned char d);

unsigned char adc_chn = 0;
unsigned char adc_chn2 = 0;





// Channel Reassignement because of Hardware Layout
unsigned char code adc_index[4] = { 2, 1, 0, 3 } ;
unsigned char code adc_index2[4] = {2 ,1 ,0 ,3 } ;





//*******************************************************************

//  Application specific init and inout/output routines

//*******************************************************************

void user_write(unsigned char index) reentrant;

//---- User init function ------------------------------------------//

extern SYS_INFO idata sys_info;



#pragma NOAREGS






//---- User write function -----------------------------------------//

void user_write(unsigned char index) reentrant
{
   if (index);
}




//---- User read function ------------------------------------------//

unsigned char user_read(unsigned char index)
{
   if (index);
   return 0;
}




//---- User function called vid CMD_USER command -------------------//

unsigned char user_func(unsigned char *data_in, unsigned char *data_out)
{
   // echo input data //
   data_out[0] = data_in[0];
   data_out[1] = data_in[1];
   return 2;
}




//---- ADC functions -----------------------------------------------//

void write_adc(unsigned char a, unsigned char d)
{
   unsigned char i, m;

   // write to communication register //

   ADC1_NCS = 0;
   
   // write zeros to !WEN and R/!W //
   for (i=0 ; i<4 ; i++) {
      ADC1_SCLK = 0;
      ADC1_DIN  = 0;
      ADC1_SCLK = 1;
   }

   // register address //
   for (i=0,m=8 ; i<4 ; i++) {
      ADC1_SCLK = 0;
      ADC1_DIN  = (a & m) > 0;
      ADC1_SCLK = 1;
      m >>= 1;
   }

   ADC1_NCS = 1;

   // write to selected data register //

   ADC1_NCS = 0;

   for (i=0,m=0x80 ; i<8 ; i++) {
      ADC1_SCLK = 0;
      ADC1_DIN  = (d & m) > 0;
      ADC1_SCLK = 1;
      m >>= 1;
   }

   ADC1_NCS = 1;
}




void write_adc_2(unsigned char a, unsigned char d)
{
   unsigned char i, m;

   // write to communication register //

   ADC2_NCS = 0;
   
   // write zeros to !WEN and R/!W //
   for (i=0 ; i<4 ; i++) {
      ADC2_SCLK = 0;
      ADC2_DIN  = 0;
      ADC2_SCLK = 1;
   }

   // register address //
   for (i=0,m=8 ; i<4 ; i++) {
      ADC2_SCLK = 0;
      ADC2_DIN  = (a & m) > 0;
      ADC2_SCLK = 1;
      m >>= 1;
   }

   ADC2_NCS = 1;

   // write to selected data register //

   ADC2_NCS = 0;

   for (i=0,m=0x80 ; i<8 ; i++) {
      ADC2_SCLK = 0;
      ADC2_DIN  = (d & m) > 0;
      ADC2_SCLK = 1;
      m >>= 1;
   }

   ADC2_NCS = 1;
}



///////////////////////////////////////////////////////////////////////////////////////////////



void read_adc24(unsigned char a, unsigned long *d)
{
   unsigned char i, m;

   // write to communication register //

   ADC1_NCS = 0;
   
   // write zero to !WEN and one to R/!W //
   for (i=0 ; i<4 ; i++) {
      ADC1_SCLK = 0;
      ADC1_DIN  = (i == 1);
      ADC1_SCLK = 1;
   }

   // register address //
   for (i=0,m=8 ; i<4 ; i++) {
      ADC1_SCLK = 0;
      ADC1_DIN  = (a & m) > 0;
      ADC1_SCLK = 1;
      m >>= 1;
   }

   ADC1_NCS = 1;

   // read from selected data register //

   ADC1_NCS = 0;

   for (i=0,*d=0 ; i<24 ; i++) {
      *d <<= 1;
      ADC1_SCLK = 0;
      *d |= ADC1_DOUT;
      ADC1_SCLK = 1;
   }

   ADC1_NCS = 1;
}





void read_adc24_2(unsigned char a, unsigned long *d)
{
   unsigned char i, m;

   // write to communication register //

   ADC2_NCS = 0;
   
   // write zero to !WEN and one to R/!W //
   for (i=0 ; i<4 ; i++) {
      ADC2_SCLK = 0;
      ADC2_DIN  = (i == 1);
      ADC2_SCLK = 1;
   }

   // register address //
   for (i=0,m=8 ; i<4 ; i++) {
      ADC2_SCLK = 0;
      ADC2_DIN  = (a & m) > 0;
      ADC2_SCLK = 1;
      m >>= 1;
   }

   ADC2_NCS = 1;

   // read from selected data register //

   ADC2_NCS = 0;

   for (i=0,*d=0 ; i<24 ; i++) {
      *d <<= 1;
      ADC2_SCLK = 0;
      *d |= ADC2_DOUT;
      ADC2_SCLK = 1;
   }

   ADC2_NCS = 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////




void adc_read()
{
   unsigned char i;
   unsigned long d;
   float value;

   if (ADC1_NRDY)
       return;


   read_adc24(REG_ADCDATA, &d);

   // convert to volts //
   value = ((float)d / (1l<<24)) * 0.64;


   // convert to Ohms (1mA excitation) //
   if (user_data.pttyp[adc_chn] == 1) // PT100
      value /= 0.001;
   else
      value /= 0.01;                  // PT1000
	
   // convert to Kelvin, coefficients obtained from table fit (www.lakeshore.com) //
   value = 5.232935E-7 * value * value * value + 
           0.0009 * value * value + 
           2.357 * value + 28.288;
	
   // convert to Celsius //
   value -= 273.15;
   
   // round result to 2 digits //
   value = floor(value*1E2+0.5)/1E2;


   if (value < -50) value = -999;

   DISABLE_INTERRUPTS;
   user_data.pt100[adc_chn] = value;
   ENABLE_INTERRUPTS;

   // start next conversion //
   adc_chn = (adc_chn + 1) % 4;
   i = adc_index[adc_chn];
   write_adc(REG_CONTROL, (1 << 7) | (i << 4) | 0x0D); // adc_chn, +-0.64V range
}






void adc_read_2()
{
   unsigned char i;
   unsigned long d;
   float value;

   if (ADC2_NRDY)
       return;


   read_adc24_2(REG_ADCDATA, &d);

   // convert to volts 
   value = ((float)d / (1l<<24)) * 0.64;


   // convert to Ohms (1mA excitation) 
   if (user_data.pttyp[adc_chn2] == 1) // PT100
      value /= 0.001;
   else
      value /= 0.01;                  // PT1000
	
   // convert to Kelvin, coefficients obtained from table fit (www.lakeshore.com) 
   value = 5.232935E-7 * value * value * value + 
           0.0009 * value * value + 
           2.357 * value + 28.288;
	
   // convert to Celsius 
   value -= 273.15;
   
   // round result to 2 digits 
   value = floor(value*1E2+0.5)/1E2;


   if (value < -50) value = -999;

   DISABLE_INTERRUPTS;
   user_data.pt100[adc_chn2 +4] = value;
   ENABLE_INTERRUPTS;

   // start next conversion 
   adc_chn2 = (adc_chn2 + 1) % 4;
   i = adc_index2[adc_chn2];
   write_adc_2(REG_CONTROL, (1 << 7) | (i << 4) | 0x0D); // adc_chn2, +-0.64V range
}





void user_init(unsigned char init)
{
   unsigned char i, d;

   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xF1;
   P1MDOUT = 0xAA;
   P2MDOUT = 0x9C;   // ADC
   P3MDOUT = 0x00;   // Node Adress Schalter
   P4MDOUT = 0x00;   // LCD Modul, Externer Bus
   P5MDOUT = 0x27;   // ADC
   P6MDOUT = 0x00;   // PT
   P7MDOUT = 0x00;

   // initial EEPROM value //
   if (init) {
      for (i=0 ; i<8 ; i++)
         user_data.pttyp[i] = 1; // 1: PT100, 2: PT1000
   }

   // init ADC1 //
   ADC1_NRES = 1;
   write_adc(REG_FILTER, 82);                   // SF value for 50Hz rejection
   write_adc(REG_MODE, 3);                      // continuous conversion
   write_adc(REG_CONTROL, (1 << 7) | (adc_chn << 4) | 0x0F); // Chn. 1-2 diff., +2.56V range



    // init ADC2 //
   ADC2_NRES = 1;
   write_adc_2(REG_FILTER, 82);                   // SF value for 50Hz rejection
   write_adc_2(REG_MODE, 3);                      // continuous conversion
   write_adc_2(REG_CONTROL, (1 << 7) | (adc_chn2 << 4) | 0x0F); // Chn. 1-2 diff., +2.56V range


   // read address from rotary switches //
   d = 255-ADDR_SWITCHES; // invert
   sys_info.node_addr = ((d << 4)/ 16) * 10 + ((d >> 4) & 0x0F); // swap nibbles

   // write digital outputs //
   for (i=0 ; i<8 ; i++)
      user_write(i);
}





//---- User loop function ------------------------------------------//

void user_loop(void)
{
   adc_read();
   adc_read_2();

}

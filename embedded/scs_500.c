/********************************************************************\

  Name:         scs_500.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-500 analog I/O

  $Log$
  Revision 1.6  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.5  2002/10/03 15:31:53  midas
  Various modifications

  Revision 1.4  2002/08/12 12:11:49  midas
  No voltage output in debug mode

  Revision 1.3  2002/08/08 06:47:27  midas
  Fixed typo

  Revision 1.2  2002/07/12 15:19:53  midas
  Added NOAREGS

  Revision 1.1  2002/07/12 08:38:13  midas
  Fixed LCD recognition

  Revision 1.2  2002/07/10 09:52:55  midas
  Finished EEPROM routines

  Revision 1.1  2002/07/09 15:31:32  midas
  Initial Revision

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-500";

sbit SR_CLOCK   = P0 ^ 4;    // Shift register clock
sbit SR_STROBE  = P0 ^ 5;    // Storage register clock
sbit SR_DATA    = P0 ^ 6;    // Serial data

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  unsigned char p1;
  unsigned int  dac0;
  unsigned int  dac1;
  float         adc[8];
} idata user_data;

struct {
  unsigned char adc_average;
  unsigned char gain[8]; // PGA bits
} idata user_conf;
  
float idata gain[8];     // gain resulting from PGA bits

MSCB_INFO_CHN code channel[] = {
  1, UNIT_BYTE,  0, 0, 0, "P1",   &user_data.p1,
  2, UNIT_WORD,  0, 0, 0, "DAC0", &user_data.dac0,
  2, UNIT_WORD,  0, 0, 0, "DAC1", &user_data.dac1,
  2, UNIT_WORD,  0, 0, 0, "ADC0", &user_data.adc[0],
  2, UNIT_WORD,  0, 0, 0, "ADC1", &user_data.adc[1],
  2, UNIT_WORD,  0, 0, 0, "ADC2", &user_data.adc[2],
  2, UNIT_WORD,  0, 0, 0, "ADC3", &user_data.adc[3],
  2, UNIT_WORD,  0, 0, 0, "ADC4", &user_data.adc[4],
  2, UNIT_WORD,  0, 0, 0, "ADC5", &user_data.adc[5],
  2, UNIT_WORD,  0, 0, 0, "ADC6", &user_data.adc[6],
  2, UNIT_WORD,  0, 0, 0, "ADC7", &user_data.adc[7],
  0
};

/* Usage of gain:

 Bipol.     Ext.  PGA          Int. PGA       Gain

  Bit5      Bit4 Bit3       Bit2 Bit1 Bit0
   0         0    0          0    0    0       1
   0         0    0          0    0    1       2
   0         0    0          0    1    0       4
   0         0    0          0    1    1       8
   0         0    0          1    0    0      16
   0         0    0          1    1    0       0.5

   0         0    1          0    0    0      10
   0         1    0          0    0    0     100
   0         1    1          0    0    0    1000

   1  bipolar, represents external jumper setting
*/

MSCB_INFO_CHN code conf_param[] = {
  1, UNIT_COUNT, 0, 0, 0, "ADCAvrg", &user_conf.adc_average,
  1, UNIT_BYTE,  0, 0, 0, "Gain0",   &user_conf.gain[0],
  1, UNIT_BYTE,  0, 0, 0, "Gain1",   &user_conf.gain[1],
  1, UNIT_BYTE,  0, 0, 0, "Gain2",   &user_conf.gain[2],
  1, UNIT_BYTE,  0, 0, 0, "Gain3",   &user_conf.gain[3],
  1, UNIT_BYTE,  0, 0, 0, "Gain4",   &user_conf.gain[4],
  1, UNIT_BYTE,  0, 0, 0, "Gain5",   &user_conf.gain[5],
  1, UNIT_BYTE,  0, 0, 0, "Gain6",   &user_conf.gain[6],
  1, UNIT_BYTE,  0, 0, 0, "Gain7",   &user_conf.gain[7],
  0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char channel);
void write_gain(void) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(void)
{
#ifdef CPU_ADUC812
  ADCCON1 = 0x7C; // power up ADC, 14.5us conv+acq time
  ADCCON2 = 0;    // select channel to convert
  DACCON  = 0x1f; // power on DACs
#endif

#ifdef CPU_C8051F000
  AMX0CF = 0x00;  // select single ended analog inputs
  ADC0CF = 0xE0;  // 16 system clocks, gain 1
  ADC0CN = 0x80;  // enable ADC 

  REF0CN = 0x03;  // enable internal reference
  DAC0CN = 0x80;  // enable DAC0
  DAC1CN = 0x80;  // enable DAC1
#endif

  /* correct initial EEPROM value */
  if (user_conf.adc_average == 0xFF)
    user_conf.adc_average = 0;

  /* write P1 and DACs */
  user_write(0);
  user_write(1);
  user_write(2);

  /* write gains */
  write_gain();

  // DEBUG_MODE = 1;
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char channel)
{
unsigned char data *d;

  switch (channel)
    {
    case 0:  // p1 
      P1 = user_data.p1; 
      break;

    case 1:  // DAC0
      d = (void *)&user_data.dac0;
#ifdef CPU_ADUC812
      DAC0H = d[1];
      DAC0L = d[0];
#endif
#ifdef CPU_C8051F000
      DAC0L = d[0];
      DAC0H = d[1];
#endif
      break;

    case 2:  // DAC1
      d = (void *)&user_data.dac1;
#ifdef CPU_ADUC812
      DAC1H = d[1];
      DAC1L = d[0];
#endif
#ifdef CPU_C8051F000
      DAC1L = d[0];
      DAC1H = d[1];
#endif
      break;
    }
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
  if (channel == 0)
    user_data.p1 = P1; 

  return 0;
}

/*---- User write config function ----------------------------------*/

void write_gain(void) reentrant
{
unsigned char i;
float g;

  SR_STROBE = 0;
  SR_CLOCK  = 0;

  for (i=0 ; i<4 ; i++)
    {
    SR_DATA = ((user_conf.gain[3-i] & 0x10) > 0); // first bit ext. PGA
    SR_CLOCK = 1;
    SR_CLOCK = 0;

    SR_DATA = ((user_conf.gain[3-i] & 0x08) > 0); // second bit ext. PGA
    SR_CLOCK = 1;
    SR_CLOCK = 0;
    }
  
  for (i=0 ; i<4 ; i++)
    {
    SR_DATA = ((user_conf.gain[7-i] & 0x10) > 0); // first bit ext. PGA
    SR_CLOCK = 1;
    SR_CLOCK = 0;

    SR_DATA = ((user_conf.gain[7-i] & 0x08) > 0); // second bit ext. PGA
    SR_CLOCK = 1;
    SR_CLOCK = 0;
    }

  SR_DATA   = 0;
  SR_STROBE = 1;
  SR_STROBE = 0;

  /* calculate gains */
  for (i=0 ; i<8 ; i++)
    {
    /* convert to volts */
    g = 1 / 65536.0 * 2.5;
    
    /* external voltage divider */
    g /= 20;
  
    /* internal PGA */
    switch (user_conf.gain[i] & 0x07)
      {
      case 0: g *= 1; break;
      case 1: g *= 2; break;
      case 2: g *= 4; break;
      case 3: g *= 8; break;
      case 4: g *= 16; break;
      case 6: g *= 0.5; break;
      }
  
    /* external PGA */
    switch (user_conf.gain[i] & 0x18)
      {
      case 0x08: g *= 10; break;
      case 0x10: g *= 100; break;
      case 0x18: g *= 1000; break;
      }

    gain[i] = g;
    }
}

void user_write_conf(unsigned char channel)
{
  if (channel > 0)
    write_gain();
}

/*---- User read config function -----------------------------------*/

void user_read_conf(unsigned char channel)
{
  if (channel);
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char idata *data_in,
                        unsigned char idata *data_out)
{
  /* echo input data */
  data_out[0] = data_in[0];
  data_out[1] = data_in[1];
  return 2;
}

/*---- User loop function ------------------------------------------*/

void adc_read(channel, float *d)
{
unsigned long value;
unsigned int  i, n;
float gvalue;

  AMX0SL = channel & 0x0F;
  ADC0CF = 0xE0 | (user_conf.gain[channel] & 0x07);  // 16 system clocks and gain

  n = 1 << (user_conf.adc_average+4);

  value = 0;
  for (i=0 ; i<n ; i++)
    {
    DISABLE_INTERRUPTS;

    ADCINT = 0;
    ADBUSY = 1;
    while (!ADCINT);  // wait until conversion ready, does NOT work with ADBUSY!

    ENABLE_INTERRUPTS;

    value += (ADC0L | (ADC0H << 8));
    watchdog_refresh();
    }

  if (user_conf.adc_average)
    value >>= (user_conf.adc_average);

  gvalue = value;




  DISABLE_INTERRUPTS;
  *d = gvalue;
  ENABLE_INTERRUPTS;
}

void user_loop(void)
{
unsigned char i;

  /* don't take data in freeze mode */
  if (!FREEZE_MODE);
    {
    for (i=0 ; i<8 ; i++)
      adc_read(i, &user_data.adc[i]);

    if (!DEBUG_MODE)
      {
      lcd_goto(0, 0);
      printf("CH1: %6.4f V", user_data.adc[0]);
  
      lcd_goto(0, 1);
      printf("CH2: %6.4f V", user_data.adc[1]);
      }
    }
}


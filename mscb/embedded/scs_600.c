/********************************************************************\

  Name:         scd_600.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-600 Digital I/O

  $Log$
  Revision 1.1  2002/07/12 15:20:08  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

extern bit FREEZE_MODE;

char code node_name[] = "SCS-600";

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  unsigned char out;
  unsigned char in;
} user_data;

MSCB_INFO_CHN code channel[] = {
  1, 0, 0, 0, "Out",   &user_data.out,
  1, 0, 0, 0, "In",    &user_data.in,
  0
};

MSCB_INFO_CHN code conf_param[] = {
  0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

#pragma NOAREGS

void user_write(unsigned char channel);

/*---- User init function ------------------------------------------*/

void user_init(void)
{
#ifdef CPU_ADUC812
  ADCCON1 = 0x7C; // power up ADC, 14.5us conv+acq time
  ADCCON2 = 0;    // select channel to convert
  DACCON  = 0x1f; // power on DACs
#endif

#ifdef CPU_C8051F000
  PRT1CF = 0xFF;  // push-pull for P1

  AMX0CF = 0x00;  // select single ended analog inputs
  ADC0CF = 0xE0;  // 16 system clocks, gain 1
  ADC0CN = 0x80;  // enable ADC 

  REF0CN = 0x03;  // enable internal reference
  DAC0CN = 0x80;  // enable DAC0
  DAC1CN = 0x80;  // enable DAC1
#endif

  /* write output */
  user_write(0);
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char channel)
{
unsigned char i;

  if (channel == 0)
    {
    for (i=0 ; i<8 ; i++)
      {
      if (user_data.out & (1<<(7-i)))
        P1 |= (1<<i);
      else
        P1 &= ~(1<<i);
      }
    }
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
  if (channel);
  return 0;
}

/*---- User write config function ----------------------------------*/

void user_write_conf(unsigned char channel)
{
  if (channel);
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
  if (data_in);
  if (data_out);
  return 0;
}

/*---- User loop function ------------------------------------------*/

unsigned int adc_read(channel)
{
unsigned long value;

  AMX0SL = channel & 0x0F;

  value = 0;
  DISABLE_INTERRUPTS;

  ADCINT = 0;
  ADBUSY = 1;
  while (!ADCINT);  // wait until conversion ready, does NOT work with ADBUSY!

  ENABLE_INTERRUPTS;

  value += (ADC0L | (ADC0H << 8));
  watchdog_refresh();

  return value;
}

void user_loop(void)
{
unsigned char i;

  /* don't take data in freeze mode */
  if (!FREEZE_MODE);
    {
    for (i=0 ; i<8 ; i++)
      {
      if (adc_read(i) > 0x800)
        user_data.in |= (1<<i);
      else
        user_data.in &= ~(1<<i);
      }

    lcd_goto(0, 0);
    printf("OUT: ");
    for (i=0 ; i<8 ; i++)
      printf("%c", user_data.out & (1<<i) ? '1' : '0');

    lcd_goto(0, 1);
    printf("IN:  ");
    for (i=0 ; i<8 ; i++)
      printf("%c", user_data.in & (1<<i) ? '1' : '0');
    }
}


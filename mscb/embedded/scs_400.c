/********************************************************************\

  Name:         scs_400.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-400 thermo couple I/O

  $Log$
  Revision 1.2  2002/10/03 15:31:53  midas
  Various modifications

  Revision 1.1  2002/08/08 06:46:03  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-400";

#define GAIN      0   // gain for internal PGA
#define AVERAGE  12   // average 2^12 times

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  unsigned char  power[4];
  float          temp[4];
} user_data;


MSCB_INFO_CHN code channel[] = {
  1, 0, 0, 0, "Power0", &user_data.power[0],
  1, 0, 0, 0, "Power1", &user_data.power[1],
  1, 0, 0, 0, "Power2", &user_data.power[2],
  1, 0, 0, 0, "Power3", &user_data.power[3],
  4, 0, 0, MSCBF_FLOAT, "Temp0", &user_data.temp[0],
  4, 0, 0, MSCBF_FLOAT, "Temp1", &user_data.temp[1],
  4, 0, 0, MSCBF_FLOAT, "Temp2", &user_data.temp[2],
  4, 0, 0, MSCBF_FLOAT, "Temp3", &user_data.temp[3],
  0
};

MSCB_INFO_CHN code conf_param[] = {
  0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

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
  AMX0CF = 0x00;  // select single ended analog inputs
  ADC0CF = 0xE0;  // 16 system clocks, gain 1
  ADC0CN = 0x80;  // enable ADC 

  REF0CN = 0x03;  // enable internal reference
  DAC0CN = 0x80;  // enable DAC0
  DAC1CN = 0x80;  // enable DAC1
#endif

}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char channel)
{
  if (channel);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
  if (channel)

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
  /* echo input data */
  data_out[0] = data_in[0];
  data_out[1] = data_in[1];
  return 2;
}

/*---- User loop function ------------------------------------------*/

void adc_read(channel, float *d)
{
unsigned long value;
unsigned int  i;
float t;

  AMX0SL = channel & 0x0F;
  ADC0CF = 0xE0 | (GAIN & 0x07);  // 16 system clocks and gain

  value = 0;
  for (i=0 ; i < (1<<AVERAGE) ; i++)
    {
    DISABLE_INTERRUPTS;

    ADCINT = 0;
    ADBUSY = 1;
    while (!ADCINT);  // wait until conversion ready, does NOT work with ADBUSY!

    ENABLE_INTERRUPTS;

    value += (ADC0L | (ADC0H << 8));
    watchdog_refresh();
    }

  if (AVERAGE > 4)
    value >>= (AVERAGE-4);

  /* convert to volts, times two (divider), times 100deg/Volt */
  t = value  / 65536.0 * 2.5 * 2 * 100;

  DISABLE_INTERRUPTS;
  *d = t;
  ENABLE_INTERRUPTS;
}

void set_power(void)
{
unsigned char i;
static unsigned long on_time;

  /* turn output off after on time expired */
  for (i=0 ; i<4 ; i++)
    if ((time() - on_time) >= user_data.power[i])
      P1 &= ~(1<<i);

  /* turn all outputs on every second */
  if (time() - on_time >= 100)
    {
    on_time = time();
    for (i=0 ; i<4 ; i++)
      if (user_data.power[i] > 0)
        P1 |= (1<<i);
    }
}

void user_loop(void)
{
static unsigned char i;
static unsigned long last_display;

  /* convert one channel at a time */
  i = (i+1) % 4;
  adc_read(i, &user_data.temp[i]);

  /* set output according to power */
  set_power();

  /* output temperature */
  if (!DEBUG_MODE)
    {
    last_display = time();

    lcd_goto(0, 0);
    printf("0:%5.1f�C 1:%5.1f�C", user_data.temp[0], user_data.temp[1]);

    lcd_goto(0, 1);
    printf("2:%5.1f�C 3:%5.1f�C", user_data.temp[2], user_data.temp[3]);
    }
}


/********************************************************************\

  Name:         scs_700.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for SCS-700 PT100/PT1000 unit

  $Log$
  Revision 1.1  2003/03/06 16:08:50  midas
  Protocol version 1.3 (change node name)

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-700";

#define GAIN      0   // gain for internal PGA
#define AVERAGE  12   // average 2^12 times

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  float temp[8];
  float power[8];
} idata user_data;

MSCB_INFO_CHN code channel[] = {
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp0",  &user_data.temp[0],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp1",  &user_data.temp[1],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp2",  &user_data.temp[2],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp3",  &user_data.temp[3],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp4",  &user_data.temp[4],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp5",  &user_data.temp[5],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp6",  &user_data.temp[6],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp7",  &user_data.temp[7],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power0", &user_data.power[0],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power1", &user_data.power[1],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power2", &user_data.power[2],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power3", &user_data.power[3],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power4", &user_data.power[4],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power5", &user_data.power[5],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power6", &user_data.power[6],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power7", &user_data.power[7],
  0
};

MSCB_INFO_CHN code conf_param[] = {
  0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char channel) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
unsigned char i;

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

  if (init)
    for (i=0 ; i<8 ; i++)
      user_data.power[i] = 0;
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char channel) reentrant
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

void user_write_conf(unsigned char channel) reentrant
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
    yield();
    }

  if (AVERAGE > 4)
    value >>= (AVERAGE-4);

  /* convert to volts, op-amp gain of 10 */
  t = value  / 65536.0 * 2.5 / 10;

  /* convert to Ohms (1mA excitation) */
  t = t / 0.001;

  /* convert to degrees, coefficients obtains from table fit (www.lakeshore.com) */
  t = -2.60315E-7*t*t*t + 0.001249273*t*t + 2.310962*t - 243.5;

  DISABLE_INTERRUPTS;
  *d = t;
  ENABLE_INTERRUPTS;
}

void set_power(void)
{
unsigned char i;
static unsigned long on_time;

  /* turn output off after on time expired */
  for (i=0 ; i<8 ; i++)
    if ((time() - on_time) >= user_data.power[i])
      P1 &= ~(1<<i);

  /* turn all outputs on every second */
  if (time() - on_time >= 100)
    {
    on_time = time();
    for (i=0 ; i<8 ; i++)
      if (user_data.power[i] > 0)
        P1 |= (1<<i);
    }
}

void user_loop(void)
{
static unsigned char i;
static unsigned long last_display;

  /* convert one channel at a time */
  i = (i+1) % 8;
  adc_read(i, &user_data.temp[i]);

  /* set output according to power */
  set_power();

  /* output temperature */
  if (!DEBUG_MODE)
    {
    last_display = time();

    lcd_goto(0, 0);
    printf("0:%5.1fßC 1:%5.1fßC", user_data.temp[0], user_data.temp[1]);

    lcd_goto(0, 1);
    printf("2:%5.1fßC 3:%5.1fßC", user_data.temp[2], user_data.temp[3]);
    }
}


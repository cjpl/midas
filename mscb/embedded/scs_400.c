/********************************************************************\

  Name:         scs_400.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for SCS-400 thermo couple I/O

  $Log$
  Revision 1.12  2003/03/21 08:28:15  midas
  Fixed bug with LSB bytes

  Revision 1.11  2003/03/19 16:35:03  midas
  Eliminated configuration parameters

  Revision 1.10  2003/03/14 13:47:22  midas
  Switched P1 to push-pull

  Revision 1.9  2003/03/06 16:08:50  midas
  Protocol version 1.3 (change node name)

  Revision 1.8  2003/02/27 10:44:30  midas
  Added 'init' code

  Revision 1.7  2003/02/19 16:05:36  midas
  Added 'init' parameter to user_init

  Revision 1.6  2003/01/14 08:19:13  midas
  Increased to 8 channels

  Revision 1.4  2002/11/22 15:43:03  midas
  Made user_write reentrant

  Revision 1.3  2002/10/09 11:06:46  midas
  Protocol version 1.1

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

/*---- Define variable parameters returned to the CMD_GET_INFO command ----*/

#define CONTROL_4     // control works only with 4 channels

/* data buffer (mirrored in EEPROM) */

#ifdef CONTROL_4

#define N_CHANNEL 4

struct {
  short demand[4];
  float temp[4];
  float p_int[4];
  char  power[4];
} idata user_data;

MSCB_INFO_VAR code variables[] = {
  2, UNIT_CELSIUS, 0, 0,           0, "Demand0", &user_data.demand[0],
  2, UNIT_CELSIUS, 0, 0,           0, "Demand1", &user_data.demand[1],
  2, UNIT_CELSIUS, 0, 0,           0, "Demand2", &user_data.demand[2],
  2, UNIT_CELSIUS, 0, 0,           0, "Demand3", &user_data.demand[3],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp0",   &user_data.temp[0],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp1",   &user_data.temp[1],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp2",   &user_data.temp[2],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp3",   &user_data.temp[3],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt0",   &user_data.p_int[0],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt1",   &user_data.p_int[1],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt2",   &user_data.p_int[2],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt3",   &user_data.p_int[3],
  1, UNIT_PERCENT, 0, 0,           0, "Power0",  &user_data.power[0],
  1, UNIT_PERCENT, 0, 0,           0, "Power1",  &user_data.power[1],
  1, UNIT_PERCENT, 0, 0,           0, "Power2",  &user_data.power[2],
  1, UNIT_PERCENT, 0, 0,           0, "Power3",  &user_data.power[3],
  0
};

#else

#define N_CHANNEL 8

struct {
  char  power[8];
  float temp[8];
} idata user_data;

MSCB_INFO_VAR code variables[] = {
  1, UNIT_PERCENT, 0, 0,           0, "Power0",  &user_data.power[0],
  1, UNIT_PERCENT, 0, 0,           0, "Power1",  &user_data.power[1],
  1, UNIT_PERCENT, 0, 0,           0, "Power2",  &user_data.power[2],
  1, UNIT_PERCENT, 0, 0,           0, "Power3",  &user_data.power[3],
  1, UNIT_PERCENT, 0, 0,           0, "Power4",  &user_data.power[4],
  1, UNIT_PERCENT, 0, 0,           0, "Power5",  &user_data.power[5],
  1, UNIT_PERCENT, 0, 0,           0, "Power6",  &user_data.power[6],
  1, UNIT_PERCENT, 0, 0,           0, "Power7",  &user_data.power[7],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp0",   &user_data.temp[0],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp1",   &user_data.temp[1],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp2",   &user_data.temp[2],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp3",   &user_data.temp[3],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp4",   &user_data.temp[4],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp5",   &user_data.temp[5],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp6",   &user_data.temp[6],
  4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp7",   &user_data.temp[7],
  0
};

#endif

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
unsigned char i;

  AMX0CF = 0x00;  // select single ended analog inputs
  ADC0CF = 0xE0;  // 16 system clocks, gain 1
  ADC0CN = 0x80;  // enable ADC

  REF0CN = 0x03;  // enable internal reference
  DAC0CN = 0x80;  // enable DAC0
  DAC1CN = 0x80;  // enable DAC1

  PRT1CF = 0xFF;  // P1 on push-pull

  if (init)
    for (i=0 ; i<N_CHANNEL ; i++)
      user_data.power[i] = 0;
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
  if (index);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
  if (index)

  return 0;
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char *data_in,
                        unsigned char *data_out)
{
  /* echo input data */
  data_out[0] = data_in[0];
  data_out[1] = data_in[1];
  return 2;
}

/*------------------------------------------------------------------*/

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

  /* convert to volts, times two (divider), times 100deg/Volt */
  t = value  / 65536.0 * 2.5 * 2 * 100;

  DISABLE_INTERRUPTS;
  *d = t;
  ENABLE_INTERRUPTS;
}

/*------------------------------------------------------------------*/

void set_power(void)
{
unsigned char i;
static unsigned long on_time;

  /* turn output off after on time expired */
  for (i=0 ; i<N_CHANNEL ; i++)
    if ((time() - on_time) >= user_data.power[i])
      P1 &= ~(1<<i);

  /* turn all outputs on every second */
  if (time() - on_time >= 100)
    {
    on_time = time();
    for (i=0 ; i<N_CHANNEL ; i++)
      if (user_data.power[i] > 0)
        P1 |= (1<<i);
    }
}

/*------------------------------------------------------------------*/

#ifdef CONTROL_4

void do_control(void)
{
unsigned char i;
float p;
static unsigned long t;

  /* once every minute */
  if (time() > t + 100*60) //##
    {
    t = time();

    for (i=0 ; i<N_CHANNEL ; i++)
      {
      /* integral part */
      user_data.p_int[i] += (user_data.demand[i] - user_data.temp[i]) * 0.01; 
      if (user_data.p_int[i] < 0)
        user_data.p_int[i] = 0;
      if (user_data.p_int[i] > 100)
        user_data.p_int[i] = 100;
      p = user_data.p_int[i];

      /* proportional part */
      p += (user_data.demand[i] - user_data.temp[i]) * 1;

      if (p < 0)
        p = 0;
      if (p > 100)
        p = 100;

      user_data.power[i] = (char) (p + 0.5);
      }
    }
}

#endif

/*------------------------------------------------------------------*/

void user_loop(void)
{
static unsigned char i;
static unsigned long last_display;

  /* convert one channel at a time */
  i = (i+1) % N_CHANNEL;
  adc_read(i, &user_data.temp[i]);

#ifdef CONTROL_4
  /* do regulation */
  do_control();
#endif

  /* set output according to power */
  set_power();

  /* output temperature */
  if (!DEBUG_MODE)
    {
    last_display = time();

    lcd_goto(0, 0);
    printf("0:%5.1fﬂC 1:%5.1fﬂC", user_data.temp[0], user_data.temp[1]);

    lcd_goto(0, 1);
    printf("2:%5.1fﬂC 3:%5.1fﬂC", user_data.temp[2], user_data.temp[3]);
    }
}


/********************************************************************\

  Name:         scs_400.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for SCS-400 thermo couple I/O

  $Id$

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-400";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

#define GAIN      0             // gain for internal PGA

/*---- Define variable parameters returned to the CMD_GET_INFO command ----*/

#define PID_CONTROL             // activate/deactivate PID control loop

/* data buffer (mirrored in EEPROM) */

#ifdef PID_CONTROL

#define N_CHANNEL 8

struct {
   short demand[N_CHANNEL];
   float temp[N_CHANNEL];
   float c_prop[N_CHANNEL];
   float c_int[N_CHANNEL];
   float p_int[N_CHANNEL];
   float power[N_CHANNEL];
   short ofs[N_CHANNEL];
   unsigned short period;
} xdata user_data;

MSCB_INFO_VAR code vars[] = {
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand0", &user_data.demand[0],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand1", &user_data.demand[1],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand2", &user_data.demand[2],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand3", &user_data.demand[3],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand4", &user_data.demand[4],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand5", &user_data.demand[5],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand6", &user_data.demand[6],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Demand7", &user_data.demand[7],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp0", &user_data.temp[0],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp1", &user_data.temp[1],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp2", &user_data.temp[2],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp3", &user_data.temp[3],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp4", &user_data.temp[4],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp5", &user_data.temp[5],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp6", &user_data.temp[6],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp7", &user_data.temp[7],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp0", &user_data.c_prop[0],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp1", &user_data.c_prop[1],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp2", &user_data.c_prop[2],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp3", &user_data.c_prop[3],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp4", &user_data.c_prop[4],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp5", &user_data.c_prop[5],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp6", &user_data.c_prop[6],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CProp7", &user_data.c_prop[7],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt0", &user_data.c_int[0],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt1", &user_data.c_int[1],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt2", &user_data.c_int[2],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt3", &user_data.c_int[3],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt4", &user_data.c_int[4],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt5", &user_data.c_int[5],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt6", &user_data.c_int[6],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "CInt7", &user_data.c_int[7],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt0", &user_data.p_int[0],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt1", &user_data.p_int[1],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt2", &user_data.p_int[2],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt3", &user_data.p_int[3],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt4", &user_data.p_int[4],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt5", &user_data.p_int[5],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt6", &user_data.p_int[6],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "PInt7", &user_data.p_int[7],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power0", &user_data.power[0],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power1", &user_data.power[1],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power2", &user_data.power[2],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power3", &user_data.power[3],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power4", &user_data.power[4],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power5", &user_data.power[5],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power6", &user_data.power[6],
   4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power7", &user_data.power[7],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs0", &user_data.ofs[0],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs1", &user_data.ofs[1],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs2", &user_data.ofs[2],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs3", &user_data.ofs[3],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs4", &user_data.ofs[4],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs5", &user_data.ofs[5],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs6", &user_data.ofs[6],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs7", &user_data.ofs[7],
   2, UNIT_SECOND, 0, 0, 0, "Period", &user_data.period,
   0
};

#else

#define N_CHANNEL 8

struct {
   char power[N_CHANNEL];
   float temp[N_CHANNEL];
   short ofs[N_CHANNEL];
} idata user_data;

MSCB_INFO_VAR code vars[] = {
   1, UNIT_PERCENT, 0, 0, 0, "Power0", &user_data.power[0],
   1, UNIT_PERCENT, 0, 0, 0, "Power1", &user_data.power[1],
   1, UNIT_PERCENT, 0, 0, 0, "Power2", &user_data.power[2],
   1, UNIT_PERCENT, 0, 0, 0, "Power3", &user_data.power[3],
   1, UNIT_PERCENT, 0, 0, 0, "Power4", &user_data.power[4],
   1, UNIT_PERCENT, 0, 0, 0, "Power5", &user_data.power[5],
   1, UNIT_PERCENT, 0, 0, 0, "Power6", &user_data.power[6],
   1, UNIT_PERCENT, 0, 0, 0, "Power7", &user_data.power[7],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp0", &user_data.temp[0],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp1", &user_data.temp[1],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp2", &user_data.temp[2],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp3", &user_data.temp[3],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp4", &user_data.temp[4],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp5", &user_data.temp[5],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp6", &user_data.temp[6],
   4, UNIT_CELSIUS, 0, 0, MSCBF_FLOAT, "Temp7", &user_data.temp[7],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs0", &user_data.ofs[0],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs1", &user_data.ofs[1],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs2", &user_data.ofs[2],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs3", &user_data.ofs[3],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs4", &user_data.ofs[4],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs5", &user_data.ofs[5],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs6", &user_data.ofs[6],
   2, UNIT_CELSIUS, 0, 0, MSCBF_SIGNED, "Ofs7", &user_data.ofs[7],
   0
};

#endif

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   unsigned char i;

   PRT0CF = 0x90;              // P0.4:TX, P0.7:RS485 enable Push/Pull

   AMX0CF = 0x00;               // select single ended analog inputs
   ADC0CF = 0xE0;               // 16 system clocks, gain 1
   ADC0CN = 0x80;               // enable ADC

   REF0CN = 0x03;               // enable internal reference
   DAC0CN = 0x80;               // enable DAC0
   DAC1CN = 0x80;               // enable DAC1

   PRT1CF = 0xFF;               // P1 on push-pull

   if (init) {
      for (i = 0; i < N_CHANNEL; i++)
         user_data.power[i] = 0;

#ifdef PID_CONTROL
      for (i = 0; i < N_CHANNEL; i++) {
         user_data.demand[i] = 0;
         user_data.c_prop[i] = 1;
         user_data.c_int[i] = 0.1;
         user_data.p_int[i] = 0;
         user_data.period = 60;
      }
#endif
   }
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

/*------------------------------------------------------------------*/

void adc_read()
{
   static unsigned char channel = 0;
   static unsigned long value;
   static unsigned char cnt = 0;
   unsigned char i;
   float t;

   if (cnt == 0) {
      /* switch to new channel */
      channel = (channel + 1) % N_CHANNEL;
      AMX0SL = channel & 0x0F;
      ADC0CF = 0xE0 | (GAIN & 0x07);    // 16 system clocks and gain

      value = 0;
   }

   /* integrate 16 times */
   for (i = 0; i < 16; i++) {
      DISABLE_INTERRUPTS;

      ADCINT = 0;
      ADBUSY = 1;
      while (!ADCINT);          // wait until conversion ready, does NOT work with ADBUSY!

      ENABLE_INTERRUPTS;

      value += (ADC0L | (ADC0H << 8));
      yield();
   }

   cnt++;

   if (cnt == 0) {              /* average 16*256 = 4096 times */
      value /= 256;

      /* convert to volts, times two (divider), times 100deg/Volt */
      t = value / 65536.0 * 2.5 * 2 * 100;

      /* correct offset */
      t += user_data.ofs[channel] / 100.0;

      DISABLE_INTERRUPTS;
      user_data.temp[channel] = t;
      ENABLE_INTERRUPTS;
   }
}

/*------------------------------------------------------------------*/

unsigned char cycle;
unsigned char ca[N_CHANNEL];

void set_power(void)
{
   static unsigned long on_time, last_time;

   unsigned char i;
   float frac;
   unsigned long expired;

   /* only process each 10ms */
   if (time() == last_time)
      return;

   last_time = time();

   /* turn output off after on-time expired */
   for (i = 0; i < N_CHANNEL; i++)
      if (user_data.power[i] < 100) {
         expired = time() - on_time;
         if (expired >= (unsigned long) (user_data.power[i])) {
            frac = user_data.power[i] - (unsigned long) (user_data.power[i]);

            if (frac == 0 || expired >= (unsigned long) (user_data.power[i]) + 1) {
               P1 &= ~(1 << i);
            } else if (cycle > 0) {
               if ((float) ca[i] / cycle > frac) {
                  P1 &= ~(1 << i);
               } else {
                  ca[i]++;
               }
            } else {
               ca[i]++;
            }
         }
      }

   /* turn all outputs on every second */
   if (time() - on_time >= 100) {
      last_time = on_time = time();
      for (i = 0; i < N_CHANNEL; i++)
         if (user_data.power[i] > 0)
            P1 |= (1 << i);

      cycle = (cycle + 1) % 10;

      if (cycle == 0)
         for (i = 0; i < N_CHANNEL; i++)
            ca[i] = 0;
   }
}

/*------------------------------------------------------------------*/

#ifdef PID_CONTROL

void do_control(void)
{
   unsigned char i;
   float p;
   static unsigned long t;

   if (user_data.period == 0)
      return;

   /* once every specified period */
   if (time() > t + 100 * user_data.period) {
      t = time();

      for (i = 0; i < N_CHANNEL; i++) {
         /* integral part */
         user_data.p_int[i] +=
             (user_data.demand[i] - user_data.temp[i]) * user_data.c_int[i];
         if (user_data.p_int[i] < 0)
            user_data.p_int[i] = 0;
         if (user_data.p_int[i] > 100)
            user_data.p_int[i] = 100;
         p = user_data.p_int[i];

         /* proportional part */
         p += (user_data.demand[i] - user_data.temp[i]) * user_data.c_prop[i];

         if (p < 0)
            p = 0;
         if (p > 100)
            p = 100;

         user_data.power[i] = ((int) (p * 10 + 0.5)) / 10.0;
      }
   }
}

#endif

/*------------------------------------------------------------------*/

void user_loop(void)
{
   /* read temperature */
   adc_read();

#ifdef PID_CONTROL
   /* do regulation */
   do_control();
#endif

   /* set output according to power */
   set_power();
}

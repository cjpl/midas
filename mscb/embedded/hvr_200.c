/********************************************************************\

  Name:         hvr_200.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for HVR_200 High Voltage Regulator

  $Id: hvr_500.c 3379 2006-10-18 12:47:07Z ritt@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

/* number of HV channels */
#define N_HV_CHN 2

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_HV_CHN;

char code node_name[] = "HVR-200";

/* maximum current im micro Ampere */
#define MAX_CURRENT 2000

/* maximum voltage in Volt */
#define MAX_VOLTAGE 2500

/* calculate voltage divider */
#define DIVIDER ((41E6 + 33E3) / 33E3)

/* current resistor */
#define RCURR 1E3

/* current multiplier */
#define CUR_MULT 10

/* delay for opto-couplers in us */
#define OPT_DELAY 300

/* Jumper1: HIGH for positive, LOW for negative */
sbit JU1       = P1 ^ 5;

/* main HV switches */
sbit HV1_OFF   = P3 ^ 5;
sbit HV2_OFF   = P3 ^ 4;

/* AD7718 pins */
sbit ADC_SCLK  = P3 ^ 6;       // Serial Clock
sbit ADC_NCS   = P3 ^ 3;       // !Chip select
sbit ADC_NRDY  = P3 ^ 0;       // !Ready
sbit ADC_DOUT  = P3 ^ 1;       // Data out
sbit ADC_DIN   = P3 ^ 7;       // Data in

/* LTC2600 pins */
sbit DAC_SCK   = P3 ^ 6;       // Serial Clock
sbit DAC_NCS   = P3 ^ 2;       // !Chip select
sbit DAC_DIN   = P3 ^ 7;       // Data in

/* LTC1655 pins channel 0 */
sbit C0DAC_SCK = P2 ^ 4;       // Serial Clock
sbit C0DAC_NCS = P1 ^ 2;       // !Chip select
sbit C0DAC_DIN = P1 ^ 1;       // Data in

/* LTC1655 pins channel 1 */
sbit C1DAC_SCK = P2 ^ 4;       // Serial Clock
sbit C1DAC_NCS = P2 ^ 2;       // !Chip select
sbit C1DAC_DIN = P2 ^ 0;       // Data in

/* LTC2400 pins channel 0 */
sbit C0ADC_SCK = P2 ^ 4;       // Serial Clock
sbit C0ADC_NCS = P1 ^ 3;       // !Chip select
sbit C0ADC_SDO = P0 ^ 2;       // Data out

/* LTC2400 pins channel 1 */
sbit C1ADC_SCK = P2 ^ 4;       // Serial Clock
sbit C1ADC_NCS = P2 ^ 3;       // !Chip select
sbit C1ADC_SDO = P0 ^ 3;       // Data out

/* AD7718 registers */
#define REG_STATUS     0
#define REG_MODE       1
#define REG_CONTROL    2
#define REG_FILTER     3
#define REG_ADCDATA    4
#define REG_ADCOFFSET  5
#define REG_ADCGAIN    6
#define REG_IOCONTROL  7

#define ADC_SF_VALUE  82        // SF value for 50Hz rejection

unsigned char idata chn_bits[N_HV_CHN];
#define DEMAND_CHANGED     (1<<0)
#define RAMP_UP            (1<<1)
#define RAMP_DOWN          (1<<2)
#define CUR_LIMIT_CHANGED  (1<<3)

float xdata u_actual[N_HV_CHN];
unsigned long xdata t_ramp[N_HV_CHN];

bit trip_reset;
bit trip_disable;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

/* CSR control bits */
#define CONTROL_HV_ON      (1<<0)
#define CONTROL_REGULATION (1<<1)
#define CONTROL_IDLE       (1<<2)

/* CSR status bits */
#define STATUS_NEGATIVE    (1<<0)
#define STATUS_LOWCUR      (1<<1)
#define STATUS_RAMP_UP     (1<<2)
#define STATUS_RAMP_DOWN   (1<<3)
#define STATUS_VLIMIT      (1<<4)
#define STATUS_ILIMIT      (1<<5)

struct {
   unsigned char control;
   float u_demand;
   float u_meas;
   float i_meas;
   unsigned char status;
   unsigned char trip_cnt;

   unsigned int ramp_up;
   unsigned int ramp_down;
   float u_limit;
   float i_limit;
   unsigned char trip_max;

   float adc_gain;
   float adc_offset;
   float dac_gain;
   float dac_offset;
   float cur_vgain;
   float cur_gain;
   float cur_offset;
   float u_dac;
} xdata user_data[N_HV_CHN];

MSCB_INFO_VAR code vars[] = {

   1, UNIT_BYTE,            0, 0,           0, "Control", &user_data[0].control,     // 0
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "Udemand", &user_data[0].u_demand,    // 1
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "Umeas",   &user_data[0].u_meas,      // 2
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT, "Imeas",   &user_data[0].i_meas,      // 3
   1, UNIT_BYTE,            0, 0,           0, "Status",  &user_data[0].status,      // 4
   1, UNIT_COUNT,           0, 0,           0, "TripCnt", &user_data[0].trip_cnt,    // 5

   2, UNIT_VOLT,            0, 0,           0, "RampUp",  &user_data[0].ramp_up,     // 6
   2, UNIT_VOLT,            0, 0,           0, "RampDown",&user_data[0].ramp_down,   // 7
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "Ulimit",  &user_data[0].u_limit,     // 8
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT, "Ilimit",  &user_data[0].i_limit,     // 9
   1, UNIT_COUNT,           0, 0,           0, "TripMax", &user_data[0].trip_max,    // 10

   /* calibration constants */
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADCgain", &user_data[0].adc_gain,    // 11
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADCofs",  &user_data[0].adc_offset,  // 12
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DACgain", &user_data[0].dac_gain,    // 13
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DACofs",  &user_data[0].dac_offset,  // 14
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURvgain",&user_data[0].cur_vgain,   // 15
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURgain", &user_data[0].cur_gain,    // 16
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURofs",  &user_data[0].cur_offset,  // 17
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "VDAC",    &user_data[0].u_dac,       // 18
   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void write_dac(unsigned char channel, unsigned short value) reentrant;
void write_adc(unsigned char a, unsigned char d);
void user_write(unsigned char index) reentrant;
void ramp_hv(unsigned char channel);
void hv_on(unsigned char channel, unsigned char flag);

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

   /* all output open drain */
   P1MDOUT = 0x00;
   P2MDOUT = 0x00;
   P3MDOUT = 0x00;

   /* initial nonzero EEPROM values */
   if (init) {
      for (i=0 ; i<N_HV_CHN ; i++) {
         user_data[i].u_demand = 0;
         user_data[i].trip_cnt = 0;
         user_data[i].ramp_up = 0;
         user_data[i].ramp_down = 0;
         user_data[i].u_limit = MAX_VOLTAGE;
         user_data[i].i_limit = MAX_CURRENT;
         user_data[i].trip_max = 0;

         user_data[i].adc_gain = 1;
         user_data[i].adc_offset = 0;
         user_data[i].dac_gain = 1;
         user_data[i].dac_offset = 0;
         user_data[i].cur_vgain = 0;
         user_data[i].cur_gain = 1;
         user_data[i].cur_offset = 0;
      }
   }

   /* default control register */
   for (i=0 ; i<N_HV_CHN ; i++) {
      user_data[i].control = CONTROL_REGULATION;
      user_data[i].status = 0;
      user_data[i].u_demand = 0;
      user_data[i].trip_cnt = 0;

      /* check maximum ratings */
      if (user_data[i].u_limit > MAX_VOLTAGE)
         user_data[i].u_limit = MAX_VOLTAGE;

      if (user_data[i].i_limit > MAX_CURRENT)
         user_data[i].i_limit = MAX_CURRENT;

      u_actual[i] = 0;
      t_ramp[i] = time();

      /* read pos/neg jumper */
      JU1 = 1;
      if (JU1 == 0)
         user_data[i].status |= STATUS_NEGATIVE;
      else
         user_data[i].status &= ~STATUS_NEGATIVE;
      user_data[i].status |= STATUS_LOWCUR;
   }

   /* switch off HVs */
   hv_on(0, 0);
   hv_on(1, 0);

   /* set default group address */
   if (sys_info.group_addr == 0xFFFF)
      sys_info.group_addr = 200;

   /* set-up DAC & ADC */
   ADC_NRDY = 1; // input
   ADC_DIN  = 1; // input

   write_adc(REG_FILTER, ADC_SF_VALUE);

   /* force update */
   for (i=0 ; i<N_HV_CHN ; i++)
      chn_bits[i] = DEMAND_CHANGED | CUR_LIMIT_CHANGED;

   /* LED normally off (inverted) */
   for (i=0 ; i<N_HV_CHN ; i++)
      led_mode(i, 1);

   trip_reset = 1;    // reset once
   trip_disable = 0;  // trip initially enabled
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   if (index == 0 || index == 1 || index == 4) {
      /* reset trip */
      user_data[cur_sub_addr()].status &= ~STATUS_ILIMIT;

      /* reset FF's in main loop */
      trip_reset = 1;

      /* indicate new demand voltage */
      chn_bits[cur_sub_addr()] |= DEMAND_CHANGED;
   }

   /* re-check voltage limit */
   if (index == 8) {
      if (user_data[cur_sub_addr()].u_limit > MAX_VOLTAGE)
         user_data[cur_sub_addr()].u_limit = MAX_VOLTAGE;

      chn_bits[cur_sub_addr()] |= DEMAND_CHANGED;
   }

   /* check current limit */
   if (index == 9) {
      if (user_data[cur_sub_addr()].i_limit > MAX_CURRENT &&
          user_data[cur_sub_addr()].i_limit != 9999)
         user_data[cur_sub_addr()].i_limit = MAX_CURRENT;

      chn_bits[cur_sub_addr()] |= CUR_LIMIT_CHANGED;
   }
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

/*---- DAC functions -----------------------------------------------*/

unsigned char code dac_index[] = {0, 1};

void write_dac(unsigned char channel, unsigned short value) reentrant
{
unsigned char i, m, b;

   /* do mapping */
   channel = dac_index[channel % (N_HV_CHN+1)];

   DAC_NCS = 0; // chip select
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   // command 3: write and update
   for (i=0,m=8 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (3 & m) > 0;
      delay_us(OPT_DELAY);
      DAC_SCK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   // channel address
   for (i=0,m=8 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (channel & m) > 0;
      delay_us(OPT_DELAY);
      DAC_SCK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   // MSB
   b = value >> 8;
   for (i=0,m=0x80 ; i<8 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (b & m) > 0;
      delay_us(OPT_DELAY);
      DAC_SCK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   // LSB
   b = value & 0xFF;
   for (i=0,m=0x80 ; i<8 ; i++) {
      DAC_SCK = 0;
      DAC_DIN = (b & m) > 0;
      delay_us(OPT_DELAY);
      DAC_SCK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   DAC_NCS = 1; // remove chip select
   delay_us(OPT_DELAY);
   watchdog_refresh(1);
}

void write_cdac(unsigned char channel, unsigned short value) reentrant
{
unsigned char i, m, b;

   if (channel == 0) {
      C0DAC_SCK = 0;
      delay_us(OPT_DELAY);
      C0DAC_NCS = 0; // chip select
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   
      // MSB
      b = value >> 8;
      for (i=0,m=0x80 ; i<8 ; i++) {
         C0DAC_SCK = 0;
         C0DAC_DIN = (b & m) > 0;
         delay_us(OPT_DELAY);
         C0DAC_SCK = 1;
         delay_us(OPT_DELAY);
         m >>= 1;
         watchdog_refresh(1);
      }
   
      // LSB
      b = value & 0xFF;
      for (i=0,m=0x80 ; i<8 ; i++) {
         C0DAC_SCK = 0;
         C0DAC_DIN = (b & m) > 0;
         delay_us(OPT_DELAY);
         C0DAC_SCK = 1;
         delay_us(OPT_DELAY);
         m >>= 1;
         watchdog_refresh(1);
      }
   
      C0DAC_SCK = 0;
      delay_us(OPT_DELAY);
      C0DAC_NCS = 1; // remove chip select
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   } else {
      C1DAC_SCK = 0;
      delay_us(OPT_DELAY);
      C1DAC_NCS = 0; // chip select
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   
      // MSB
      b = value >> 8;
      for (i=0,m=0x80 ; i<8 ; i++) {
         C1DAC_SCK = 0;
         C1DAC_DIN = (b & m) > 0;
         delay_us(OPT_DELAY);
         C1DAC_SCK = 1;
         delay_us(OPT_DELAY);
         m >>= 1;
         watchdog_refresh(1);
      }
   
      // LSB
      b = value & 0xFF;
      for (i=0,m=0x80 ; i<8 ; i++) {
         C1DAC_SCK = 0;
         C1DAC_DIN = (b & m) > 0;
         delay_us(OPT_DELAY);
         C1DAC_SCK = 1;
         delay_us(OPT_DELAY);
         m >>= 1;
         watchdog_refresh(1);
      }
   
      C1DAC_SCK = 0;
      delay_us(OPT_DELAY);
      C1DAC_NCS = 1; // remove chip select
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   }
}

/*---- ADC functions -----------------------------------------------*/

void write_adc(unsigned char a, unsigned char d)
{
   unsigned char i, m;

   /* write to communication register */

   ADC_NCS = 0;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   /* write zeros to !WEN and R/!W */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      ADC_DIN  = 0;
      delay_us(OPT_DELAY);
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      ADC_DIN  = (a & m) > 0;
      delay_us(OPT_DELAY);
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   ADC_NCS = 1;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   /* write to selected data register */

   ADC_NCS = 0;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   for (i=0,m=0x80 ; i<8 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      ADC_DIN  = (d & m) > 0;
      delay_us(OPT_DELAY);
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   ADC_NCS = 1;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);
}

void read_adc8(unsigned char a, unsigned char *d)
{
   unsigned char i, m;

   /* write to communication register */

   ADC_NCS = 0;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   /* write zero to !WEN and one to R/!W */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      ADC_DIN  = (i == 1);
      delay_us(OPT_DELAY);
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      ADC_DIN  = (a & m) > 0;
      delay_us(OPT_DELAY);
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   ADC_NCS = 1;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   /* read from selected data register */

   ADC_NCS = 0;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   for (i=0,m=0x80,*d=0 ; i<8 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      if (ADC_DOUT)
         *d |= m;
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   ADC_NCS = 1;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);
}

void read_adc24(unsigned char a, unsigned long *d)
{
   unsigned char i, m;

   /* write to communication register */

   ADC_NCS = 0;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   /* write zero to !WEN and one to R/!W */
   for (i=0 ; i<4 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      ADC_DIN  = (i == 1);
      delay_us(OPT_DELAY);
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      ADC_DIN  = (a & m) > 0;
      delay_us(OPT_DELAY);
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   ADC_NCS = 1;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   /* read from selected data register */

   ADC_NCS = 0;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   for (i=0,*d=0 ; i<24 ; i++) {
      *d <<= 1;
      ADC_SCLK = 0;
      delay_us(OPT_DELAY);
      delay_us(OPT_DELAY);
      *d |= ADC_DOUT;
      ADC_SCLK = 1;
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   }

   ADC_NCS = 1;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);
}

unsigned char adc_read(unsigned char channel, float *value)
{
   unsigned long xdata d, start_time;
   unsigned char i;

   /* start conversion */
   write_adc(REG_CONTROL, channel << 4 | 0x0F); // adc_chn, +2.56V range
   write_adc(REG_MODE, (1<<4) | (1<<1));        // single conversion, 10 channel mode

   start_time = time();

   while (ADC_NRDY) {
      yield();
      for (i=0 ; i<N_HV_CHN ; i++)
         ramp_hv(i);      // do ramping while reading ADC

      /* abort if no ADC ready after 300ms */
      if (time() - start_time > 30) {

         write_adc(REG_FILTER, ADC_SF_VALUE);

         return 0;
      }
   }

   read_adc24(REG_ADCDATA, &d);

   /* convert to volts */
   *value = ((float)d / (1l<<24)) * 2.56;

   /* round result to 6 digits */
   *value = floor(*value*1E6+0.5)/1E6;

   return 1;
}

unsigned char cadc_read(unsigned char channel, float *value)
{
   unsigned long xdata d, start_time;
   unsigned char i;

   start_time = time();

   if (channel == 0) {

      C0ADC_NCS = 0;
      delay_us(OPT_DELAY);
      C0ADC_SCK = 0;
      delay_us(OPT_DELAY);

      /* wait for conversion ready */
      while (C0ADC_SDO) {
         yield();
         for (i=0 ; i<N_HV_CHN ; i++)
            ramp_hv(i);      // do ramping while reading ADC
   
         /* abort if no ADC ready after 300ms */
         if (time() - start_time > 30) {
            C0ADC_NCS = 1;
            return 0;
         }
      }
         
      for (i=0,d=0 ; i<28 ; i++) {
         d <<= 1;
         C0ADC_SCK = 0;
         delay_us(OPT_DELAY);
         d |= C0ADC_SDO;
         C0ADC_SCK = 1;
         delay_us(OPT_DELAY);
         watchdog_refresh(1);
      }
   
      C0ADC_SCK = 0;
      C0ADC_NCS = 1;
      delay_us(OPT_DELAY);
      watchdog_refresh(1);

   } else {
      
      C1ADC_NCS = 0;
      delay_us(OPT_DELAY);
      C1ADC_SCK = 0;
      delay_us(OPT_DELAY);

      /* wait for conversion ready */
      while (C1ADC_SDO) {
         yield();
         for (i=0 ; i<N_HV_CHN ; i++)
            ramp_hv(i);      // do ramping while reading ADC
   
         /* abort if no ADC ready after 300ms */
         if (time() - start_time > 30) {
            C1ADC_NCS = 1;
            return 0;
         }
      }
         
      for (i=0,d=0 ; i<28 ; i++) {
         d <<= 1;
         C1ADC_SCK = 0;
         delay_us(OPT_DELAY);
         d |= C1ADC_SDO;
         C1ADC_SCK = 1;
         delay_us(OPT_DELAY);
         watchdog_refresh(1);
      }
   
      C1ADC_SCK = 0;
      C1ADC_NCS = 1;
      delay_us(OPT_DELAY);
      watchdog_refresh(1);
   }

   /* convert to volts */
   if ((d & (1l<<25)) == 0) // negative sign
     *value = (((float)((long)((d & 0xFFFFFF) | 0xFF000000))) / (1l<<24)) * 2.5;
   else   
     *value = ((float)(d & 0xFFFFFF) / (1l<<24)) * 2.5;

   /* round result to 6 digits */
   *value = floor(*value*1E6+0.5)/1E6;

   return 1;
}

/*------------------------------------------------------------------*/

void set_hv(unsigned char channel, float value) reentrant
{
   unsigned short d;

   /* check for limit */
   if (value > user_data[channel].u_limit) {
      value = user_data[channel].u_limit;
      user_data[channel].status |= STATUS_VLIMIT;
   } else
      user_data[channel].status &= ~STATUS_VLIMIT;

   led_mode(channel, !(value > 10));

   /* apply correction */
   value = value * user_data[channel].dac_gain + user_data[channel].dac_offset;
   if (value < 0)
      value = 0;

   /* convert HV to voltage */
   value = value / DIVIDER;

   /* convert to DAC units */
   d = (unsigned short) ((value / 2.5 * 65536) + 0.5);

   /* write dac */
   write_dac(channel, d);
}

/*------------------------------------------------------------------*/

void read_hv(unsigned char channel)
{
   float hv;

   /* read voltage channel */
   if (!adc_read(channel, &hv))
      return;

   /* convert to HV */
   hv *= DIVIDER;

   /* apply calibration */
   hv = hv * user_data[channel].adc_gain + user_data[channel].adc_offset;

   /* 0.01 resolution */
   hv = floor(hv * 100) / 100.0;

   DISABLE_INTERRUPTS;
   user_data[channel].u_meas = hv;
   ENABLE_INTERRUPTS;
}

/*------------------------------------------------------------------*/

void set_current_limit(unsigned char channel, float value)
{
   unsigned short d;

   if (value == 9999) {
      /* disable current trip */
      d = 65535; 
      trip_disable = 1;
   } else {
      
      /* remove reset */
      trip_disable = 0;
   
      /* "reverse" calibration */
      //value = (value - user_data[0].cur_offset) / user_data[0].cur_gain;
   
      /* convert current to voltage */
      //value = value / DIVIDER * CUR_MULT * RCURR / 1E6;
   
      /* convert to DAC units */
      d = (unsigned short) ((value / 2.5 * 65535) + 0.5);
   }

   /* write current dac */
   write_cdac(channel, d);
}

/*------------------------------------------------------------------*/

unsigned char hardware_current_trip(unsigned char channel)
{
unsigned char d;

   /* continuously set FF reset, in case ADC got HV flash */
   if (trip_disable) {
      write_adc(REG_IOCONTROL, (1 << 4)); // P1DIR=1,P1DAT=0
	  return 0;
   }

   read_adc8(REG_IOCONTROL, &d);
   
   /* P2 of AD7718 should be low in case of trip */
   if ((d & 2) > 0)
      return 0;

   /* only check some time after setting/ramping */

   if (time() > t_ramp[channel] + 300) {
      if (user_data[channel].adc_gain == 1) {
         /* uncalibrated */
         if (user_data[channel].u_meas < 100 && 
             user_data[channel].u_demand > 150)
            return 1;

         return 0; 
      } else {
         /* calibrated */
         if (user_data[channel].u_meas < 5 && user_data[channel].u_demand >= 5)
            return 1;

         return 0;
      }
   }

   return 0;
}

/*------------------------------------------------------------------*/

void hv_on(unsigned char channel, unsigned char flag)
{
   if (channel == 0)
      HV1_OFF = !flag;
   else
      HV2_OFF = !flag;
}

/*------------------------------------------------------------------*/

void check_current(unsigned char channel)
{
   if (/*user_data[channel].i_meas > user_data[channel].i_limit ||*/    // "software" check
       hardware_current_trip(channel)) {                            // "hardware" check

      /* zero output voltage */
      set_hv(channel, 0);
      u_actual[channel] = 0;
      user_data[channel].u_dac = 0;

      /* stop possible ramping */
      chn_bits[channel] &= ~DEMAND_CHANGED;
      chn_bits[channel] &= ~RAMP_UP;
      chn_bits[channel] &= ~RAMP_DOWN;
      user_data[channel].status &= ~(STATUS_RAMP_UP | STATUS_RAMP_DOWN);

      /* raise trip flag */
      if ((user_data[channel].status & STATUS_ILIMIT) == 0) {
         user_data[channel].status |= STATUS_ILIMIT;
         user_data[channel].trip_cnt++;
      }

      /* check for trip recovery */
      if (user_data[channel].trip_cnt < user_data[channel].trip_max) {
         /* clear trip */
         user_data[channel].status &= ~STATUS_ILIMIT;

         hv_on(channel, 1);

         /* force ramp up */
         chn_bits[channel] |= DEMAND_CHANGED;
      }
   }

   if (user_data[channel].status & STATUS_ILIMIT)
      led_blink(channel, 3, 100);
}

/*------------------------------------------------------------------*/

void read_current(unsigned char channel)
{
   float current;

   /* read current channel */
   if (!cadc_read(channel, &current))
      return;

   /* I[uA] = U/R*10^6 */
   current = current / CUR_MULT / RCURR * 1E6;

   /* correct for offset */
   current -= user_data[channel].cur_offset;

   /* calibrate gain */
   current = current * user_data[channel].cur_gain;

   /* 0.001 resolution */
   current = floor(current * 1000) / 1000.0;

   DISABLE_INTERRUPTS;
   user_data[channel].i_meas = current;
   ENABLE_INTERRUPTS;
}

/*------------------------------------------------------------------*/

void ramp_hv(unsigned char channel)
{
   unsigned char delta;

   /* only process ramping when HV is on and not tripped */
   if ((user_data[channel].control & CONTROL_HV_ON) &&
       !(user_data[channel].status & STATUS_ILIMIT)) {

      if (chn_bits[channel] & DEMAND_CHANGED) {
         /* start ramping */

         if (user_data[channel].u_demand > u_actual[channel] &&
             user_data[channel].ramp_up > 0) {
            /* ramp up */
            chn_bits[channel] |= RAMP_UP;
            chn_bits[channel] &= ~RAMP_DOWN;
            user_data[channel].status |= STATUS_RAMP_UP;
            user_data[channel].status &= ~STATUS_RAMP_DOWN;
            chn_bits[channel] &= ~DEMAND_CHANGED;
         }

         if (user_data[channel].u_demand < u_actual[channel] &&
             user_data[channel].ramp_down > 0) {
            /* ramp down */
            chn_bits[channel] &= ~RAMP_UP;
            chn_bits[channel] |= RAMP_DOWN;
            user_data[channel].status &= ~STATUS_RAMP_UP;
            user_data[channel].status |= STATUS_RAMP_DOWN;
            chn_bits[channel] &= ~DEMAND_CHANGED;
         }

         /* remember start time */
         t_ramp[channel] = time();
      }

      /* ramp up */
      if (chn_bits[channel] & RAMP_UP) {
         delta = time() - t_ramp[channel];
         if (delta) {
            u_actual[channel] += (float) user_data[channel].ramp_up * delta / 100.0;
            user_data[channel].u_dac = u_actual[channel];

            if (u_actual[channel] >= user_data[channel].u_demand) {
               /* finish ramping */

               u_actual[channel] = user_data[channel].u_demand;
               user_data[channel].u_dac = u_actual[channel];
               chn_bits[channel] &= ~RAMP_UP;
               user_data[channel].status &= ~STATUS_RAMP_UP;

            }

            set_hv(channel, u_actual[channel]);
            t_ramp[channel] = time();
         }
      }

      /* ramp down */
      if (chn_bits[channel] & RAMP_DOWN) {
         delta = time() - t_ramp[channel];
         if (delta) {
            u_actual[channel] -= (float) user_data[channel].ramp_down * delta / 100.0;
            user_data[channel].u_dac = u_actual[channel];

            if (u_actual[channel] <= user_data[channel].u_demand) {
               /* finish ramping */

               u_actual[channel] = user_data[channel].u_demand;
               user_data[channel].u_dac = u_actual[channel];
               chn_bits[channel] &= ~RAMP_DOWN;
               user_data[channel].status &= ~STATUS_RAMP_DOWN;

            }

            set_hv(channel, u_actual[channel]);
            t_ramp[channel] = time();
         }
      }
   }

   watchdog_refresh(1);
}

/*------------------------------------------------------------------*/

void regulation(unsigned char channel)
{
   /* if demand value changed, remember time to avoid current 
      trip sensing for the next few seconds */
   if (chn_bits[channel] & DEMAND_CHANGED)
      t_ramp[channel] = time();

   /* only if HV on and not ramping */
   if ((user_data[channel].control & CONTROL_HV_ON) &&
       (chn_bits[channel] & (RAMP_UP | RAMP_DOWN)) == 0 &&
       !(user_data[channel].status & STATUS_ILIMIT)) {
      if (user_data[channel].control & CONTROL_REGULATION) {

         u_actual[channel] = user_data[channel].u_demand;

         /* correct if difference is at least half a LSB */
         if (fabs(user_data[channel].u_demand - user_data[channel].u_meas) / DIVIDER / 2.5 * 65536 > 0.5) {

            user_data[channel].u_dac += user_data[channel].u_demand - user_data[channel].u_meas;

            /* only allow +-2V fine regulation range */
            if (user_data[channel].u_dac < user_data[channel].u_demand - 2)
               user_data[channel].u_dac = user_data[channel].u_demand - 2;

            if (user_data[channel].u_dac > user_data[channel].u_demand + 2)
               user_data[channel].u_dac = user_data[channel].u_demand + 2;

            chn_bits[channel] &= ~DEMAND_CHANGED;
            set_hv(channel, user_data[channel].u_dac);
         }

      } else {
         /* set voltage directly */
         if (chn_bits[channel] & DEMAND_CHANGED) {
            u_actual[channel] = user_data[channel].u_demand;
            user_data[channel].u_dac = user_data[channel].u_demand;
            set_hv(channel, user_data[channel].u_demand);

            chn_bits[channel] &= ~DEMAND_CHANGED;
         }
      }
   }

   /* if HV switched off, set DAC to zero */
   if (!(user_data[channel].control & CONTROL_HV_ON) &&
       (chn_bits[channel] & DEMAND_CHANGED)) {

      user_data[channel].u_dac = 0;
      set_hv(channel, 0);
      chn_bits[channel] &= ~DEMAND_CHANGED;
   }

   watchdog_refresh(1);
}

/*------------------------------------------------------------------*/

#ifdef NOT_RELIABLE

void read_temperature(void)
{
   float temperature;
   unsigned char i;

   REF0CN = 0x0E; // temperature sensor on
   AD0EN = 1;     // turn on ADC

   /* set MUX */
   AMX0P = 0x1E; // Temp sensor
   AMX0N = 0x1F; // GND

   DISABLE_INTERRUPTS;

   AD0INT = 0;
   AD0BUSY = 1;
   while (AD0INT == 0);

   ENABLE_INTERRUPTS;

   temperature = ADC0L + (ADC0H & 0x03) * 256;

   /* convert to mV */
   temperature = temperature / 1023.0 * 3000;

   /* convert to deg. C */
   temperature = (temperature - 897) / 3.35;

   /* offset correction using prototype */
   temperature -= 27;

   /* strip fractional part */
   temperature = (int) (temperature + 0.5);

   for (i=0 ; i<N_HV_CHN ; i++) {
      DISABLE_INTERRUPTS;
      user_data[i].temperature = temperature;
      ENABLE_INTERRUPTS;

      /* read node configuration */
      if (JU1 == 0)
         user_data[i].status |= STATUS_NEGATIVE;
      else
         user_data[i].status &= ~STATUS_NEGATIVE;

      if (JU2 == 0)
         user_data[i].status |= STATUS_LOWCUR;
      else
         user_data[i].status &= ~STATUS_LOWCUR;
   }
}

#endif

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   unsigned char channel;

   /* loop over all HV channels */
   for (channel=0 ; channel<N_HV_CHN ; channel++) {

      watchdog_refresh(0);

      if ((user_data[0].control & CONTROL_IDLE) == 0) {

         /* set current limit if changed */
         if (chn_bits[channel] & CUR_LIMIT_CHANGED) {
            set_current_limit(channel, user_data[channel].i_limit);
            chn_bits[channel] &= ~CUR_LIMIT_CHANGED;
            trip_reset = 1;
         }

         //check_current(channel);

         read_hv(channel);
         ramp_hv(channel);
         regulation(channel);
         read_current(channel);
      }

      if (trip_reset) {
         hv_on(channel, 1);
         trip_reset = 0;
      }
  }

   // read_temperature();
}

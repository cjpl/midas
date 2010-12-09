/********************************************************************\

  Name:         hvr_500.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for HVR_300 High Voltage Regulator


  Memory usage: 
    Program code: 0x0000 - 0x31FF
    Upgrade code: 0x3200 - 0x3811 (1554 bytes)
    EEPROM page:  0x3A00 - 0x3BFF (one page @ 512 bytes)
                  0x3C00 - 0x3DFF cannot be erased ???

    Check after each compile that CODE size is less than
    0x31FF + 0x0611 = 14352

  $Id$

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mscbemb.h"

/* following flag switches from hardware trip to software trip */
#undef HARDWARE_TRIP

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

/* number of HV channels */
#define N_HV_CHN 5

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_HV_CHN;

char code node_name[] = "HVR-500";

/* maximum current im micro Ampere */
#define MAX_CURRENT 2000

/* maximum voltage in Volt */
#define MAX_VOLTAGE 2500

/* calculate voltage divider */
#define DIVIDER ((41E6 + 33E3) / 33E3)

/* current resistor */
#define RCURR 56E3

/* current multiplier */
#define CUR_MULT 15

/* delay for opto-couplers in us */
#define OPT_DELAY 300

/* configuration jumper */
sbit JU0 = P3 ^ 4;              // negative module if forced to zero
sbit JU1 = P3 ^ 2;              // low current module if forced to zero
sbit JU2 = P3 ^ 1;

/* main switch on crate */
sbit SW1 = P3 ^ 3;
sbit SW2 = P2 ^ 2;

/* geographic address */
sbit GA_A0 = P0 ^ 1;            // 0=top, 1=bottom
sbit GA_A1 = P0 ^ 0;
sbit GA_A2 = P0 ^ 3;
sbit GA_A3 = P0 ^ 2;
sbit GA_A4 = P2 ^ 1;
sbit GA_A5 = P2 ^ 0;

/* AD7718 pins */
sbit ADC_NRES = P1 ^ 7;         // !Reset
sbit ADC_SCLK = P1 ^ 5;         // Serial Clock
sbit ADC_NCS  = P1 ^ 3;         // !Chip select
sbit ADC_NRDY = P1 ^ 0;         // !Ready
sbit ADC_DOUT = P1 ^ 1;         // Data out
sbit ADC_DIN  = P1 ^ 4;         // Data in

/* LTC2600 pins */
sbit DAC_NCS  = P1 ^ 2;         // !Chip select
sbit DAC_SCK  = P1 ^ 5;         // Serial Clock
sbit DAC_CLR  = P1 ^ 6;         // Clear
sbit DAC_DIN  = P1 ^ 4;         // Data in

/* AD7718 registers */
#define REG_STATUS     0
#define REG_MODE       1
#define REG_CONTROL    2
#define REG_FILTER     3
#define REG_ADCDATA    4
#define REG_ADCOFFSET  5
#define REG_ADCGAIN    6
#define REG_IOCONTROL  7

//#define ADC_SF_VALUE  13        // SF value for 50Hz rejection, 4 Hz total update rate
#define ADC_SF_VALUE  27        // SF value for 50Hz rejection, 2 Hz total update rate
//#define ADC_SF_VALUE  82        // SF value for 50Hz rejection, 1 Hz total update rate

unsigned char idata chn_bits[N_HV_CHN];
#define DEMAND_CHANGED     (1<<0)
#define RAMP_UP            (1<<1)
#define RAMP_DOWN          (1<<2)
#define CUR_LIMIT_CHANGED  (1<<3)

float xdata u_actual[N_HV_CHN];
unsigned long xdata t_ramp[N_HV_CHN];

bit trip_reset;
bit trip_disable;
bit old_sw1;

unsigned long xdata trip_time[N_HV_CHN];

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
#define STATUS_RILIMIT     (1<<6)
#define STATUS_DISABLED    (1<<7)

struct {
   unsigned char control;
   float u_demand;
   float u_meas;
   float i_meas;
   unsigned char status;
   unsigned char trip_cnt;

   float ramp_up;
   float ramp_down;
   float u_limit;
   float i_limit;
   float ri_limit;
   unsigned char trip_max;
   unsigned char trip_time;

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

   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "RampUp",  &user_data[0].ramp_up,     // 6
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "RampDown",&user_data[0].ramp_down,   // 7
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "Ulimit",  &user_data[0].u_limit,     // 8
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT, "Ilimit",  &user_data[0].i_limit,     // 9
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT, "RIlimit", &user_data[0].ri_limit,    // 10
   1, UNIT_COUNT,           0, 0,           0, "TripMax", &user_data[0].trip_max,    // 11
   1, UNIT_SECOND,          0, 0,           0, "TripTime",&user_data[0].trip_time,   // 12

   /* calibration constants */
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADCgain", &user_data[0].adc_gain,    // 13
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADCofs",  &user_data[0].adc_offset,  // 14
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DACgain", &user_data[0].dac_gain,    // 15
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DACofs",  &user_data[0].dac_offset,  // 16
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURvgain",&user_data[0].cur_vgain,   // 17
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURgain", &user_data[0].cur_gain,    // 18
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURofs",  &user_data[0].cur_offset,  // 19
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "UDAC",    &user_data[0].u_dac,       // 20
   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void reset_adc();
void write_dac(unsigned char channel, unsigned short value) reentrant;
void write_adc(unsigned char a, unsigned char d);
void user_write(unsigned char index) reentrant;
void ramp_hv(unsigned char channel);
void reset_hardware_trip(void);
unsigned char read_current(unsigned char channel);
unsigned char read_hv(unsigned char channel);

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;
   unsigned short address;

   /* all output open drain */
   P1MDOUT = 0x00;
   P2MDOUT = 0x00;

   /* initial nonzero EEPROM values */
   if (init) {
      memset(user_data, 0, sizeof(user_data));
      for (i=0 ; i<N_HV_CHN ; i++) {
         user_data[i].ramp_up    = 300;
         user_data[i].ramp_down  = 300;
         user_data[i].u_limit    = MAX_VOLTAGE;
         user_data[i].i_limit    = MAX_CURRENT;
         user_data[i].ri_limit   = MAX_CURRENT;
         user_data[i].trip_time  = 10;

         user_data[i].adc_gain   = 1;
         user_data[i].dac_gain   = 1;
         user_data[i].cur_gain   = 1;
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

      if (user_data[i].i_limit > MAX_CURRENT && user_data[i].i_limit != 9999)
         user_data[i].i_limit = MAX_CURRENT;

      u_actual[i] = 0;
      t_ramp[i] = time();
   }

   /* jumper and address bits as input */
   JU1 = 1;
   JU2 = 1;
   SW1 = 1;
   SW2 = 1;
   GA_A0 = 1;
   GA_A1 = 1;
   GA_A2 = 1;
   GA_A3 = 1;
   GA_A4 = 1;
   GA_A5 = 1;

   /* read six address bits */
   address = GA_A5;
   address = (address << 1) | GA_A4;
   address = (address << 1) | GA_A3;
   address = (address << 1) | GA_A2;
   address = (address << 1) | GA_A1;
   address = (address << 1) | GA_A0;

   /* each device has 5 channels */
   address *= 5;                   
   
   /* keep high byte of node address */               
   address |= (sys_info.node_addr & 0xFF00);

   sys_info.node_addr = address;

   /* set default group address */
   if (sys_info.group_addr == 0xFFFF)
      sys_info.group_addr = 500;

   /* sample initial state of switch */
   old_sw1 = SW1;
   if (old_sw1)
      for (i=0 ; i<N_HV_CHN ; i++)
         user_data[i].status |= STATUS_DISABLED;

   /* set-up DAC & ADC */
   DAC_CLR  = 1;
   ADC_NRES = 1;
   ADC_NRDY = 1; // input
   ADC_DIN  = 1; // input

   delay_ms(300); // wait for the ADC to come up after power on
   reset_adc();

#ifdef HARDWARE_TRIP
   /* reset current trip FF's */
   reset_hardware_trip();
#else
   /* disable hardware trip FF's */
   write_adc(REG_IOCONTROL, (1 << 4));
#endif

   /* force update */
   for (i=0 ; i<N_HV_CHN ; i++)
      chn_bits[i] = DEMAND_CHANGED | CUR_LIMIT_CHANGED;

   /* LED normally off (inverted) */
   for (i=0 ; i<N_HV_CHN ; i++)
      led_mode(i, 1);

   /* initially, trips are enabled */
   trip_reset = 1;    // reset once
   trip_disable = 0;  // trip initially enabled
   for (i=0 ; i<N_HV_CHN ; i++)
      trip_time[i] = 0;
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   unsigned char a;

   a = cur_sub_addr();

   /* reset trip on main switch, trip reset and on demand = 0 */
   if (index == 0 || 
      (index == 1 && user_data[a].u_demand == 0) || 
       index == 5) {

      /* reset trip */
      user_data[a].status &= ~STATUS_ILIMIT;

      /* reset trip count if not addressed directly */
      if (index != 5)
         user_data[a].trip_cnt = 0;

      /* reset FF's in main loop */
      trip_reset = 1;

      /* indicate new demand voltage */
      chn_bits[a] |= DEMAND_CHANGED;
   }

   /* indicated changed demand if no current trip */
   if (index == 1 && ((user_data[a].status & STATUS_ILIMIT) == 0))
      chn_bits[a] |= DEMAND_CHANGED;

   /* re-check voltage limit */
   if (index == 8) {
      if (user_data[a].u_limit > MAX_VOLTAGE)
         user_data[a].u_limit = MAX_VOLTAGE;

      chn_bits[a] |= DEMAND_CHANGED;
   }

   /* check current limit */
   if (index == 9) {
      if (user_data[a].i_limit > MAX_CURRENT &&
          user_data[a].i_limit != 9999)
         user_data[a].i_limit = MAX_CURRENT;

      chn_bits[a] |= CUR_LIMIT_CHANGED;
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
   if (data_in || data_out);
   return 0;
}

/*---- DAC functions -----------------------------------------------*/

unsigned char code dac_index[] = {0, 1, 2, 3, 4, 5};

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

/*---- ADC functions -----------------------------------------------*/

void reset_adc()
{
   ADC_NRES = 0;
   delay_us(OPT_DELAY);
   ADC_NRES = 1;
   delay_us(OPT_DELAY);

   write_adc(REG_FILTER, ADC_SF_VALUE);

#ifndef HARDWARE_TRIP
   /* disable hardware trip FF's */
   write_adc(REG_IOCONTROL, (1 << 4));
#endif
}

void write_adc(unsigned char a, unsigned char d)
{
   unsigned char xdata i, m;

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

/* adc mapping: 0U,0I 1U,1I 2U,2I 3U,3I ... */

unsigned char code adc_index[] = {7, 6, 1, 0, 3, 2, 5, 4, 15, 14 };

unsigned char adc_read(unsigned char channel, float *value)
{
   unsigned long xdata d, start_time;
   unsigned char xdata i;

   /* start conversion */
   channel = adc_index[channel % (N_HV_CHN*2)];
   write_adc(REG_CONTROL, channel << 4 | 0x0F); // adc_chn, +2.56V range
   write_adc(REG_MODE, (1<<4) | (1<<1));        // single conversion, 10 channel mode

   start_time = time();

   while (ADC_NRDY) {
      yield();
      for (i=0 ; i<N_HV_CHN ; i++)
         ramp_hv(i);      // do ramping while reading ADC

      /* abort if no ADC ready after 300ms */
      if (time() - start_time > 30) {
         reset_adc();
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

unsigned char read_hv(unsigned char channel)
{
   float xdata hv;

   /* read voltage channel */
   if (!adc_read(channel*2, &hv))
      return 0;

   /* convert to HV */
   hv *= DIVIDER;

   /* apply calibration */
   hv = hv * user_data[channel].adc_gain + user_data[channel].adc_offset;

   /* 0.01 resolution */
   hv = floor(hv * 100) / 100.0;

   led_mode(channel, !(hv > 40));

   DISABLE_INTERRUPTS;
   user_data[channel].u_meas = hv;
   ENABLE_INTERRUPTS;

   return 1;
}

/*------------------------------------------------------------------*/

void set_current_limit(float value)
{
#ifdef HARDWARE_TRIP
   unsigned short d;

   if (value == 9999) {
      /* disable current trip */
      d = 65535; 
      trip_disable = 1;
      write_adc(REG_IOCONTROL, (1 << 4)); // P1DIR=1,P1DAT=0
   } else {
      
      /* remove reset */
      trip_disable = 0;
      write_adc(REG_IOCONTROL, (1 << 4) | (1 << 0)); // P1DIR=1,P1DAT=1
   
      /* "inverse" calibration */
      value /= user_data[0].cur_gain;
      value += user_data[0].cur_offset;

      /* omit cur_vgain calibration, because statistical mean of
         unbalanced dividers is zero */

      if (value < 0)
         value = 0;

      /* convert current to voltage */
      value = value / DIVIDER * CUR_MULT * RCURR / 1E6;
   
      /* convert to DAC units */
      d = (unsigned short) ((value / 2.5 * 65535) + 0.5);
   }

   /* write current dac */
   write_dac(5, d);

#else // HARDWARE_TRIP

   if (value == 9999)
      trip_disable = 1;
   else
      trip_disable = 0;

#endif // !HARDWARE_TRIP
}

/*------------------------------------------------------------------*/

#ifdef HARDWARE_TRIP

unsigned char hardware_current_trip(unsigned char channel)
{
unsigned char d;

   /* continuously set FF reset, in case ADC got HV flash */
   if (trip_disable) {
      write_adc(REG_IOCONTROL, (1 << 4)); // P1DIR=1,P1DAT=0
      return 0;
   }

   /* do not check if reset in progres */
   if (trip_reset)
      return 0;

   read_adc8(REG_IOCONTROL, &d);
   
   /* P2 of AD7718 should be low in case of trip */
   if ((d & 2) > 0)
      return 0;

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

   return 0;
}

/*------------------------------------------------------------------*/

void reset_hardware_trip()
{
   if (!trip_disable) {
      write_adc(REG_IOCONTROL, (1 << 4));            // P1DIR=1,P1DAT=0
      write_adc(REG_IOCONTROL, (1 << 4) | (1 << 0)); // P1DIR=1,P1DAT=1
   }
}

#endif // HARDWARE_TRIP

/*------------------------------------------------------------------*/

void check_current(unsigned char channel)
{
#ifdef HARDWARE_TRIP
   if (user_data[channel].i_meas > user_data[channel].i_limit ||    // "software" check
       hardware_current_trip(channel)) {                            // "hardware" check

      if (trip_time[channel] == 0)
         trip_time[channel] = time();

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
   }
#else
   if (user_data[channel].i_meas > user_data[channel].i_limit &&
       user_data[channel].u_dac > 40) {

      unsigned char i;

      /* ADC could be crashed, so reset it and read current again */
      for (i=0 ; i<10 ; i++) {
         reset_adc();
         delay_ms(50);
         read_hv(channel);
         if (read_current(channel))
            break;
      }

      /* now test again for trip */
      if (user_data[channel].i_meas > user_data[channel].i_limit) {

         if (trip_time[channel] == 0)
            trip_time[channel] = time();
   
         /* ramp down */
         chn_bits[channel] &= ~DEMAND_CHANGED;
         chn_bits[channel] &= ~RAMP_UP;
         user_data[channel].status &= ~STATUS_RAMP_UP;
   
         if (user_data[channel].ramp_down > 0) {
            /* ramp down if requested */
            chn_bits[channel] |= RAMP_DOWN;
            user_data[channel].status |= STATUS_RAMP_DOWN;
         } else {
            /* otherwise go to zero immediately */
            set_hv(channel, 0);
            u_actual[channel] = 0;
            user_data[channel].u_dac = 0;
   
            /* stop possible ramping */
            chn_bits[channel] &= ~RAMP_DOWN;
            user_data[channel].status &= ~STATUS_RAMP_DOWN;
         }
   
         /* raise trip flag */
         if ((user_data[channel].status & STATUS_ILIMIT) == 0) {
            user_data[channel].status |= STATUS_ILIMIT;
            user_data[channel].trip_cnt++;
         }
      }
   }
#endif

   /* check for trip recovery */
   if ((user_data[channel].status & STATUS_ILIMIT) &&
       user_data[channel].trip_cnt < user_data[channel].trip_max &&
       time() >= trip_time[channel] + user_data[channel].trip_time*100) {
      /* clear trip */
      user_data[channel].status &= ~STATUS_ILIMIT;
      trip_time[channel] = 0;

#ifdef HARDWARE_TRIP
      reset_hardware_trip();
#endif

      /* force ramp up */
      chn_bits[channel] |= DEMAND_CHANGED;
   }

   if (user_data[channel].status & STATUS_ILIMIT)
      led_blink(channel, 3, 100);
}

/*------------------------------------------------------------------*/

unsigned char read_current(unsigned char channel)
{
   float xdata current;

   /* read current channel */
   if (!adc_read(channel*2+1, &current))
      return 0;

   /* correct opamp gain, divider & curr. resist, microamp */
   current = current / CUR_MULT * DIVIDER / RCURR * 1E6;

   /* correct for unbalanced voltage dividers */
   current -= user_data[channel].cur_vgain * user_data[channel].u_meas;

   /* correct for offset */
   current -= user_data[channel].cur_offset;

   /* calibrate gain */
   current = current * user_data[channel].cur_gain;

   /* 0.1 uA resolution */
   current = floor(current*10 + 0.5)/10;

   DISABLE_INTERRUPTS;
   user_data[channel].i_meas = current;
   ENABLE_INTERRUPTS;

   return 1;
}

/*------------------------------------------------------------------*/

void ramp_hv(unsigned char channel)
{
   unsigned char delta;

   /* only process ramping when HV is on and not tripped */
   if ((user_data[channel].control & CONTROL_HV_ON) &&
       !(user_data[channel].status & STATUS_DISABLED)) {

      if (chn_bits[channel] & DEMAND_CHANGED) {
         /* start ramping */

         if (user_data[channel].u_demand > u_actual[channel] &&
             user_data[channel].ramp_up > 0) {
            /* ramp up */
            chn_bits[channel] |= RAMP_UP;
            chn_bits[channel] &= ~RAMP_DOWN;
            user_data[channel].status |= STATUS_RAMP_UP;
            user_data[channel].status &= ~STATUS_RAMP_DOWN;
            user_data[channel].status &= ~STATUS_RILIMIT;
            chn_bits[channel] &= ~DEMAND_CHANGED;
         }

         if (user_data[channel].u_demand < u_actual[channel] &&
             user_data[channel].ramp_down > 0) {
            /* ramp down */
            chn_bits[channel] &= ~RAMP_UP;
            chn_bits[channel] |= RAMP_DOWN;
            user_data[channel].status &= ~STATUS_RAMP_UP;
            user_data[channel].status |= STATUS_RAMP_DOWN;
            user_data[channel].status &= ~STATUS_RILIMIT;
            chn_bits[channel] &= ~DEMAND_CHANGED;
         }

         /* remember start time */
         t_ramp[channel] = time();
      }

      /* ramp up */
      if (chn_bits[channel] & RAMP_UP) {
         delta = time() - t_ramp[channel];
         if (user_data[channel].i_meas > user_data[channel].ri_limit)
            user_data[channel].status |= STATUS_RILIMIT;
         else
            user_data[channel].status &= ~STATUS_RILIMIT;

         if (delta && user_data[channel].i_meas < user_data[channel].ri_limit) {
            u_actual[channel] += (float) user_data[channel].ramp_up * delta / 100.0;
            user_data[channel].u_dac = u_actual[channel];

            if (u_actual[channel] >= user_data[channel].u_demand) {
               /* finish ramping */

               u_actual[channel] = user_data[channel].u_demand;
               user_data[channel].u_dac = u_actual[channel];
               chn_bits[channel] &= ~RAMP_UP;
               user_data[channel].status &= ~STATUS_RAMP_UP;
               user_data[channel].status &= ~STATUS_RILIMIT;
            }

            set_hv(channel, user_data[channel].u_dac);
            t_ramp[channel] = time();
         }
      }

      /* ramp down */
      if (chn_bits[channel] & RAMP_DOWN) {
         delta = time() - t_ramp[channel];
         if (delta) {
            u_actual[channel] -= (float) user_data[channel].ramp_down * delta / 100.0;
            user_data[channel].u_dac = u_actual[channel];

            /* handle trip ramping */
            if (user_data[channel].status & STATUS_ILIMIT) {
               if (u_actual[channel] <= 0) {
                  /* finish ramping */
   
                  u_actual[channel] = 0;
                  user_data[channel].u_dac = 0;
                  chn_bits[channel] &= ~RAMP_DOWN;
                  user_data[channel].status &= ~STATUS_RAMP_DOWN;
   
               }
            } else {
               if (u_actual[channel] <= user_data[channel].u_demand) {
                  /* finish ramping */
   
                  u_actual[channel] = user_data[channel].u_demand;
                  user_data[channel].u_dac = u_actual[channel];
                  chn_bits[channel] &= ~RAMP_DOWN;
                  user_data[channel].status &= ~STATUS_RAMP_DOWN;
   
               }
            }

            set_hv(channel, user_data[channel].u_dac);
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

   /* only if HV on and not disabled and not ramping */
   if ((user_data[channel].control & CONTROL_HV_ON) &&
       !(user_data[channel].status & STATUS_DISABLED) &&
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
         }

      } else {
         /* set voltage directly */
         if (chn_bits[channel] & DEMAND_CHANGED) {
            u_actual[channel] = user_data[channel].u_demand;
            user_data[channel].u_dac = user_data[channel].u_demand;

            chn_bits[channel] &= ~DEMAND_CHANGED;
         }
      }
   }

   /* if HV switched off, set DAC to zero */
   if (!(user_data[channel].control & CONTROL_HV_ON) &&
       (chn_bits[channel] & DEMAND_CHANGED)) {

      user_data[channel].u_dac = 0;
      u_actual[channel] = 0;
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
   }
}

#endif

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   unsigned char xdata channel, i;

   /* loop over all HV channels */
   for (channel=0 ; channel<N_HV_CHN ; channel++) {

      watchdog_refresh(0);

      if ((user_data[0].control & CONTROL_IDLE) == 0) {

         /* set current limit if changed */
         if (chn_bits[channel] & CUR_LIMIT_CHANGED) {
            set_current_limit(user_data[channel].i_limit);
            chn_bits[channel] &= ~CUR_LIMIT_CHANGED;
            trip_reset = 1;
         }

         /* read back HV and current */
         read_hv(channel);
         read_current(channel);

         /* check for current trip */
         check_current(channel);

         /* do ramping and regulation */
         ramp_hv(channel);
         regulation(channel);

         /* set voltage regularly, in case DAC got HV spike */
         set_hv(channel, user_data[channel].u_dac);

      }

#ifdef HARDWARE_TRIP
      if (trip_reset) {
         reset_hardware_trip();
         trip_reset = 0;
      }
#endif

      /* if crate HV switched off, set DAC to zero */
      if (SW1) {
         for (i = 0 ; i<N_HV_CHN ; i++ ) {
            user_data[i].u_dac = 0;
            user_data[i].status |= STATUS_DISABLED;
            u_actual[i] = 0;
            set_hv(i, 0);
         }
         old_sw1 = 1;
      }
   
      /* if crate HV switched on, indicated changed demand value*/
      if (!SW1 && old_sw1) {
         for (i = 0 ; i<N_HV_CHN ; i++ ) {
            chn_bits[i] |= DEMAND_CHANGED;
            user_data[i].status &= ~STATUS_DISABLED;
         }
         old_sw1 = 0;
      }
   }

   /* reset ADC once all channels have been read */
   reset_adc();

   // read_temperature();
}

/********************************************************************\

  Name:         hvr_800.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for HVR_800 High Voltage Regulator

  $Id: hvr_800.c 3531 2007-01-30 09:04:40Z ritt $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

/* number of HV channels */
#define N_HV_CHN 8

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_HV_CHN;

char code node_name[] = "HVR-800";

/* maximum current im micro Ampere */
#define MAX_CURRENT 200

/* maximum voltage in Volt */
#define MAX_VOLTAGE 600

/* calculate voltage dividers */
#define DIVIDER ((9.4E6 + 39000) / 39000)

/* current resistor */
#define RCURR 1E3

/* current multiplier */
#define CUR_MULT 11.51          // (g=1+49.4k/4k7) at AD8221

/* delay for opto-couplers in us */
#define OPT_DELAY 1

/* configuration jumper */
sbit JU0 = P3 ^ 4;              // negative module if forced to zero

/* AD7718 pins */
sbit ADC_NRES = P0 ^ 6;         // !Reset
sbit ADC_SCLK = P3 ^ 6;         // Serial Clock
sbit ADC_NCS  = P3 ^ 3;         // !Chip select
sbit ADC_NRDY = P3 ^ 0;         // !Ready
sbit ADC_DOUT = P3 ^ 1;         // Data out
sbit ADC_DIN  = P3 ^ 7;         // Data in

/* LTC2600 pins */
sbit DAC_NCS  = P3 ^ 2;         // !Chip select
sbit DAC_SCK  = P3 ^ 6;         // Serial Clock
sbit DAC_CLR  = P0 ^ 7;         // Clear
sbit DAC_DIN  = P3 ^ 7;         // Data in

/* LTC2400 pins */
sbit CADC_SCLK = P0 ^ 4;        // Serial Clock
sbit CADC_NCS  = P3 ^ 5;        // !Chip select
#define CADC_SDO P2             // 8 SDOs mapped to full port

/* AD7718 registers */
#define REG_STATUS     0
#define REG_MODE       1
#define REG_CONTROL    2
#define REG_FILTER     3
#define REG_ADCDATA    4
#define REG_ADCOFFSET  5
#define REG_ADCGAIN    6
#define REG_IOCONTROL  7

#define ADC_SF_VALUE  27        // SF value for 50Hz rejection, 2 Hz total update rate
//#define ADC_SF_VALUE  82        // SF value for 50Hz rejection, 1 Hz total update rate

unsigned char idata chn_bits[N_HV_CHN];
#define DEMAND_CHANGED     (1<<0)
#define RAMP_UP            (1<<1)
#define RAMP_DOWN          (1<<2)
#define CUR_LIMIT_CHANGED  (1<<3)

float xdata u_actual[N_HV_CHN];
unsigned long xdata t_ramp[N_HV_CHN];

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

void write_dac(unsigned char channel, unsigned short value) reentrant;
void write_adc(unsigned char a, unsigned char d);
void user_write(unsigned char index) reentrant;
void ramp_hv(unsigned char channel);

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

   /* open drain(*) /push-pull: 
      P0.0 TX1      P1.0*LED7         P2.0*DOUT_0       P3.0*RDY
      P0.1*RX1      P1.1*LED6         P2.1*DOUT_1       P3.1*DOUT
      P0.2*TX2      P1.2*LED5         P2.2*DOUT_2       P3.2 CS_DAC
      P0.3*RX2      P1.3*LED4         P2.3*DOUT_3       P3.3 CS_ADC
                                                            
      P0.4 CLK_ADC  P1.4*LED3         P2.4*DOUT_4       P3.4*POLARITY
      P0.5 EN       P1.5*LED2         P2.5*DOUT_5       P3.5 CS_CADC
      P0.6 CLR_ADC  P1.6*LED1         P2.6*DOUT_6       P3.6 CLK
      P0.7 CLR_DAC  P1.7*LED0         P2.7*DOUT_7       P3.7 DIN
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xF1;
   P1MDOUT = 0x00;
   P2MDOUT = 0x00;
   P3MDOUT = 0xEC;

   /* jumper as input */
   JU0 = 1;

   /* initial nonzero EEPROM values */
   if (init) {
      memset(user_data, 0, sizeof(user_data));
      for (i=0 ; i<N_HV_CHN ; i++) {
         user_data[i].u_limit = MAX_VOLTAGE;
         user_data[i].i_limit = MAX_CURRENT;
         user_data[i].ri_limit = MAX_CURRENT;
         user_data[i].ramp_up = 100;
         user_data[i].ramp_down = 100;
         user_data[i].trip_time = 10;

         user_data[i].adc_gain = 1;
         user_data[i].dac_gain = 1;
         user_data[i].cur_gain = 1;
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

   /* set default group address */
   if (sys_info.group_addr == 0xFFFF)
      sys_info.group_addr = 800;

   /* read node configuration */
   for (i=0 ; i<N_HV_CHN ; i++) {
      if (JU0 == 0)
         user_data[i].status |= STATUS_NEGATIVE;
      else
         user_data[i].status &= ~STATUS_NEGATIVE;
   }

   /* set-up DAC & ADC */
   DAC_CLR  = 1;
   ADC_NRES = 1;
   ADC_NRDY = 1; // input
   ADC_DIN  = 1; // input

   // switch to external clock mode
   CADC_NCS  = 1;
   delay_us(OPT_DELAY);
   CADC_SCLK = 0;
   delay_us(OPT_DELAY);
   CADC_NCS  = 0;
   CADC_SDO  = 0xFF; // input

   /* reset and wait for start-up of ADC */
   ADC_NRES = 0;
   delay_us(OPT_DELAY);
   ADC_NRES = 1;
   delay_us(OPT_DELAY);

   write_adc(REG_FILTER, ADC_SF_VALUE);

   /* force update */
   for (i=0 ; i<N_HV_CHN ; i++)
      chn_bits[i] = DEMAND_CHANGED | CUR_LIMIT_CHANGED;

   /* LED normally off (inverted) */
   for (i=0 ; i<N_HV_CHN ; i++)
      led_mode(i, 1);

   for (i=0 ; i<N_HV_CHN ; i++)
      trip_time[i] = 0;
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   if (index == 0 || index == 1 || index == 4) {
      /* reset trip */
      user_data[cur_sub_addr()].status &= ~STATUS_ILIMIT;

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

void write_dac(unsigned char channel, unsigned short value) reentrant
{
unsigned char i, m, b;

   /* do mapping */
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
   unsigned long d, start_time;
   unsigned char i;

   /* start conversion */
   write_adc(REG_CONTROL, channel << 4 | 0x0F); // adc_chn, +2.56V range
   write_adc(REG_MODE, 2);                      // single conversion

   start_time = time();

   while (ADC_NRDY) {
      yield();
      for (i=0 ; i<N_HV_CHN ; i++)
         ramp_hv(i);      // do ramping while reading ADC

      /* abort if no ADC ready after 300ms */
      if (time() - start_time > 30) {

         /* reset ADC */
         ADC_NRES = 0;
         delay_ms(100);
         ADC_NRES = 1;
         delay_ms(300);

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

/*------------------------------------------------------------------*/

void set_hv(unsigned char channel, float value) reentrant
{
   unsigned short d;

   /* check for limit */
   if (value > user_data[channel].u_limit+5) {
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
   if (value > 2.5)
      value = 2.5;

   /* convert to DAC units */
   d = (unsigned short) ((value / 2.5 * 65535) + 0.5);

   /* write dac */
   write_dac(channel, d);
}

/*------------------------------------------------------------------*/

void read_hv(unsigned char channel)
{
   float xdata hv;

   /* read voltage channel */
   if (!adc_read(channel, &hv))
      return;

   /* convert to HV */
   hv *= DIVIDER;

   /* apply calibration */
   hv = hv * user_data[channel].adc_gain + user_data[channel].adc_offset;

   /* 0.001 resolution */
   hv = floor(hv * 1000) / 1000.0;

   led_mode(channel, !(hv > 2));

   DISABLE_INTERRUPTS;
   user_data[channel].u_meas = hv;
   ENABLE_INTERRUPTS;
}

/*------------------------------------------------------------------*/

void check_current(unsigned char channel)
{
   if (user_data[channel].i_meas > user_data[channel].i_limit &&
       user_data[channel].i_limit != 9999) {

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

   /* check for trip recovery */
   if ((user_data[channel].status & STATUS_ILIMIT) &&
       user_data[channel].trip_cnt < user_data[channel].trip_max &&
       time() >= trip_time[channel] + user_data[channel].trip_time*100) {
      /* clear trip */
      user_data[channel].status &= ~STATUS_ILIMIT;
      trip_time[channel] = 0;

      /* force ramp up */
      chn_bits[channel] |= DEMAND_CHANGED;
   }

   if (user_data[channel].status & STATUS_ILIMIT)
      led_blink(channel, 3, 100);
}

/*------------------------------------------------------------------*/

void read_current()
{
   unsigned char i, j, d;
   float xdata current;
   unsigned long xdata sr[8];

   CADC_NCS = 0;
   delay_us(OPT_DELAY);

   /* check if all LTC2400 have converted */
   d = CADC_SDO;
   if (d > 0) {
      CADC_NCS = 1;
      delay_us(OPT_DELAY);
      return;
   }

   /* read 28 bits from 8 ADCs */
   for (i=0 ; i<8 ; i++)
      sr[i] = 0;

   for (i=0 ; i<32 ; i++) {
      d = CADC_SDO;
      for (j=0 ; j<8 ; j++) {
         sr[j] = (sr[j] << 1) | (d & 0x01);
         d >>= 1;
      }
      CADC_SCLK = 1;
      delay_us(OPT_DELAY);
      CADC_SCLK = 0;
      delay_us(OPT_DELAY);
   }

   CADC_NCS  = 1;
   CADC_SCLK = 0;

   for (i=0 ; i<8 ; i++) {

      /* convert to volts */
      if ((sr[i] & (1l<<29)) == 0) // negative sign
        current = (((float)((long)((sr[i] & 0x1FFFFFFF) | 0xE0000000))) / (1l<<28)) * 3.3;
      else   
        current = ((float)(sr[i] & 0x1FFFFFFF) / (1l<<28)) * 3.3;
   
      /* correct opamp gain and curr. resist, microamp */
      current = current / CUR_MULT / RCURR * 1E6;
         
      /* correct for unbalanced voltage dividers */
      current -= user_data[i].cur_vgain * user_data[i].u_meas;
   
      /* correct for offset */
      current -= user_data[i].cur_offset;
   
      /* calibrate gain */
      current = current * user_data[i].cur_gain;
   
      /* 1 nA resolution */
      current = floor(current*1000 + 0.5)/1000.0;
   
      DISABLE_INTERRUPTS;
      user_data[i].i_meas = current;
      ENABLE_INTERRUPTS;
   }
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
   /* only if HV on and not ramping */
   if ((user_data[channel].control & CONTROL_HV_ON) &&
       (chn_bits[channel] & (RAMP_UP | RAMP_DOWN)) == 0 &&
       !(user_data[channel].status & STATUS_ILIMIT)) {
      if (user_data[channel].control & CONTROL_REGULATION) {

         u_actual[channel] = user_data[channel].u_demand;

         /* correct if difference is at least half a LSB */
         if (fabs(user_data[channel].u_demand - user_data[channel].u_meas) / DIVIDER / 2.5 * 65536 > 0.5) {

            user_data[channel].u_dac += user_data[channel].u_demand - user_data[channel].u_meas;

            /* only allow +-5V fine regulation range */
            if (user_data[channel].u_dac < user_data[channel].u_demand - 5)
               user_data[channel].u_dac = user_data[channel].u_demand - 5;

            if (user_data[channel].u_dac > user_data[channel].u_demand + 5)
               user_data[channel].u_dac = user_data[channel].u_demand + 5;

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
   unsigned char channel;

   /* loop over all HV channels */
   for (channel=0 ; channel<N_HV_CHN ; channel++) {

      watchdog_refresh(0);

      if ((user_data[0].control & CONTROL_IDLE) == 0) {
         read_current();
         if (time() > 300) { // current readout unreliable at startup
            check_current(channel);
            read_hv(channel);
            ramp_hv(channel);
            regulation(channel);
         }
      }
   }
}

/********************************************************************\

  Name:         hvr_1600.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for HVR_1600 Low Voltage Regulator

  $Id: hvr_1600.c 3531 2007-01-30 09:04:40Z ritt $

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

/* number of HV channels */
#define N_HV_CHN 16

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_HV_CHN;

char code node_name[] = "HVR-1600";

/* maximum current im micro Ampere */
#define MAX_CURRENT 200

/* maximum voltage in Volt */
#define MAX_VOLTAGE 50

/* calculate voltage dividers */
#define DAC_DIVIDER ((200 + 10) / 10)
#define ADC_DIVIDER ((68 + 1.6) / 1.6)

/* current resistor */
#define RCURR 1E3

/* current multiplier */
#define CUR_MULT 8.97          // (g=1+49.4k/6k2) at AD8221

/* delay for opto-couplers in us */
#define OPT_DELAY 1

/* configuration jumper */
sbit JU0 = P3 ^ 4;              // negative module if forced to zero

/* LTC2600 pins */
sbit DAC_NCS1 = P4 ^ 4;         // !Chip select channel 0-7
sbit DAC_NCS2 = P4 ^ 3;         // !Chip select channel 8-15
sbit DAC_SCK  = P4 ^ 5;         // Serial Clock
sbit DAC_CLR  = P4 ^ 6;         // Clear
sbit DAC_SDI  = P4 ^ 2;         // Serial In

/* LTC2418 pins */
sbit ADC_SCK  = P4 ^ 5;         // Serial Clock
sbit ADC_NCS  = P4 ^ 1;         // !Chip select
sbit ADC_SDI  = P4 ^ 2;         // Serial Out
sbit ADC_SDO  = P4 ^ 0;         // Serial Out

/* LTC2400 pins */
sbit CADC_SCLK = P2 ^ 0;        // Serial Clock
sbit CADC_NCS  = P2 ^ 1;        // !Chip select
#define CADC_SDO0 P5            // 8 SDOs mapped to full port
#define CADC_SDO1 P6            // 8 SDOs mapped to full port

unsigned char xdata chn_bits[N_HV_CHN];
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
void ramp_hvs(void);

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

   /* open drain(*) /push-pull: 
      P0.0 TX1      P1.0          P2.0 CADC_CLK P3.0 LED8
      P0.1*RX1      P1.1          P2.1 CADC_CS  P3.1 LED9
      P0.2          P1.2          P2.2          P3.2 LED10
      P0.3          P1.3          P2.3          P3.3 LED11
                                                       
      P0.4          P1.4          P2.4          P3.4 LED12
      P0.5 EN       P1.5          P2.5          P3.5 LED13
      P0.6          P1.6          P2.6          P3.6 LED14
      P0.7 WD       P1.7          P2.7          P3.7 LED15

      P4.0*SEROUT   P5.0*SER15    P6.0*SER7     P7.0 LED0
      P4.1 ADC_CS   P5.1*SER14    P6.1*SER6     P7.1 LED1
      P4.2 SERIN    P5.2*SER13    P6.2*SER5     P7.2 LED2  
      P4.3 DAC_CS1  P5.3*SER12    P6.3*SER4     P7.3 LED3
                                                            
      P4.4 DAC_CS0  P5.4*SER11    P6.4*SER3     P7.4 LED4
      P4.5 SER_CLK  P5.5*SER10    P6.5*SER2     P7.5 LED5
      P4.6 DAC_CLR  P5.6*SER9     P6.6*SER1     P7.6 LED6
      P4.7          P5.7*SER8     P6.7*SER0     P7.7 LED7
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xFD;
   P1MDOUT = 0xFF;
   P2MDOUT = 0xFF;
   P3MDOUT = 0xFF;
   P4MDOUT = 0xFE;
   P5MDOUT = 0x00;
   P6MDOUT = 0x00;
   P7MDOUT = 0xFF;

   /* jumper as input */
   JU0 = 1;

   /* initial nonzero EEPROM values */
   if (init) {
      memset(user_data, 0, sizeof(user_data));
      for (i=0 ; i<N_HV_CHN ; i++) {
         user_data[i].u_limit = MAX_VOLTAGE;
         user_data[i].i_limit = MAX_CURRENT;
         user_data[i].ri_limit = MAX_CURRENT;
         user_data[i].ramp_up = 10;
         user_data[i].ramp_down = 10;
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

   SFRPAGE = CONFIG_PAGE; // for Port 4!

   /* set-up DAC & ADC */
   DAC_CLR  = 1;

   // switch to external clock mode
   CADC_NCS  = 1;
   delay_us(OPT_DELAY);
   CADC_SCLK = 0;
   delay_us(OPT_DELAY);
   CADC_NCS  = 0;
   CADC_SDO0 = 0xFF; // input
   CADC_SDO1 = 0xFF;

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

   SFRPAGE = CONFIG_PAGE; // for Port 4!

   if (channel < 8)
      DAC_NCS1 = 0; // chip select
   else
      DAC_NCS2 = 0; // chip select

   delay_us(OPT_DELAY);
   watchdog_refresh(1);

   // command 3: write and update
   for (i=0,m=8 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_SDI = (3 & m) > 0;
      delay_us(OPT_DELAY);
      DAC_SCK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   // channel address
   for (i=0,m=8 ; i<4 ; i++) {
      DAC_SCK = 0;
      DAC_SDI = ((channel % 8)& m) > 0;
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
      DAC_SDI = (b & m) > 0;
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
      DAC_SDI = (b & m) > 0;
      delay_us(OPT_DELAY);
      DAC_SCK = 1;
      delay_us(OPT_DELAY);
      m >>= 1;
      watchdog_refresh(1);
   }

   DAC_NCS1 = 1; // remove chip select
   DAC_NCS2 = 1;
   delay_us(OPT_DELAY);
   watchdog_refresh(1);
}

/*---- ADC functions -----------------------------------------------*/

unsigned char adc_read(unsigned char *ch, float *value)
{
   unsigned long d, cmd;
   unsigned char i;
   static unsigned char channel = 0;

   SFRPAGE = CONFIG_PAGE; // for Port 4!

   ADC_SCK = 0; // enforce external serial clock mode
   delay_us(OPT_DELAY);
   ADC_NCS = 0;
   delay_us(OPT_DELAY);

   /* check if LTC2418 has converted */
   d = ADC_SDO;
   if (d > 0) {
      ADC_NCS = 1;
      delay_us(OPT_DELAY);
      return 0;
   }

   /* return channel which just was converted */
   *ch = channel;

   /* prepare for next channel */
   channel = (channel + 1) % 16;

   /* output command: EN=1,SGL=1 */
   cmd = (1l<<31) | (1l<<29) | (1l<<28);
   if (channel & 1)
      cmd |= (1l<<27); // ODD = 1
   cmd |= ((unsigned long)(channel >> 1) << 24); // A2 - A0

   /* read 32 bits from ADC */
   for (i=d=0 ; i<32 ; i++) {
      d <<= 1;
      d |= ADC_SDO;
      ADC_SDI = (cmd & (1l<<31)) > 0;
      cmd <<= 1;
      ADC_SCK = 1;
      delay_us(OPT_DELAY);
      ADC_SCK = 0;
      delay_us(OPT_DELAY);
   }

   ADC_NCS = 1;
   ADC_SCK = 0;

   /* convert to volts */
   if ((d & (1l<<29)) == 0) // negative sign
     *value = (((float)((long)((d & 0x1FFFFFC0) | 0xE0000000))) / (1l<<29)) * 2.5;
   else   
     *value = ((float)(d & 0x1FFFFFC0) / (1l<<29)) * 2.5;

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

   /* turn on LED */
   if (value > 0.5)
      led_mode(channel, 0);

   /* apply correction */
   value = value * user_data[channel].dac_gain + user_data[channel].dac_offset;
   if (value < 0)
      value = 0;

   /* convert HV to voltage */
   value = value / DAC_DIVIDER;
   if (value > 2.5)
      value = 2.5;

   /* convert to DAC units */
   d = (unsigned short) ((value / 2.5 * 65535) + 0.5);

   /* write dac */
   write_dac(channel, d);
}

/*------------------------------------------------------------------*/

unsigned char read_hv(unsigned char *channel)
{
   float xdata hv;

   /* read voltage channel */
   if (!adc_read(channel, &hv))
      return 0;

   /* convert to HV */
   hv *= ADC_DIVIDER;

   /* apply calibration */
   hv = hv * user_data[*channel].adc_gain + user_data[*channel].adc_offset;

   /* 0.001 resolution */
   hv = floor(hv * 1000) / 1000.0;

   led_mode(*channel, !(hv > 0.5));

   DISABLE_INTERRUPTS;
   user_data[*channel].u_meas = hv;
   ENABLE_INTERRUPTS;

   return 1;
}

/*------------------------------------------------------------------*/

void check_currents()
{
   unsigned char channel;

   for (channel = 0 ; channel<N_HV_CHN ; channel++) {
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
}

/*------------------------------------------------------------------*/

void read_currents()
{
   unsigned char i, j;
   unsigned short d;
   float xdata current;
   unsigned long xdata sr[N_HV_CHN];

   SFRPAGE = CONFIG_PAGE; // for Port 5 + 6!

   CADC_NCS = 0;
   delay_us(OPT_DELAY);

   /* check if all LTC2400s have converted */
   d = CADC_SDO0 << 8 | CADC_SDO1;
   if (d > 0) {
      CADC_NCS = 1;
      delay_us(OPT_DELAY);
      return;
   }

   /* read 28 bits from all ADCs */
   for (i=0 ; i<N_HV_CHN ; i++)
      sr[i] = 0;

   for (i=0 ; i<32 ; i++) {
      d = CADC_SDO0;
      for (j=0 ; j<8 ; j++) {
         sr[15-j] = (sr[15-j] << 1) | (d & 0x01);
         d >>= 1;
      }
      d = CADC_SDO1;
      for (j=8 ; j<16 ; j++) {
         sr[15-j] = (sr[15-j] << 1) | (d & 0x01);
         d >>= 1;
      }
      CADC_SCLK = 1;
      delay_us(OPT_DELAY);
      CADC_SCLK = 0;
      delay_us(OPT_DELAY);
   }

   CADC_NCS  = 1;
   CADC_SCLK = 0;

   for (i=0 ; i<N_HV_CHN ; i++) {

      /* convert to volts */
      if ((sr[i] & (1l<<29)) == 0) // negative sign
        current = (((float)((long)((sr[i] & 0x1FFFFFFF) | 0xE0000000))) / (1l<<28)) * 2.5;
      else   
        current = ((float)(sr[i] & 0x1FFFFFFF) / (1l<<28)) * 2.5;
   
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

void ramp_hvs()
{
   unsigned char delta, channel;

   for (channel=0 ; channel<N_HV_CHN ; channel++) {

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

      watchdog_refresh(0);
   }

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
         if (fabs(user_data[channel].u_demand - user_data[channel].u_meas) / DAC_DIVIDER / 2.5 * 65536 > 0.5) {

            user_data[channel].u_dac += user_data[channel].u_demand - user_data[channel].u_meas;

            /* only allow +-0.5V fine regulation range */
            if (user_data[channel].u_dac < user_data[channel].u_demand - 0.5)
               user_data[channel].u_dac = user_data[channel].u_demand - 0.5;

            if (user_data[channel].u_dac > user_data[channel].u_demand + 0.5)
               user_data[channel].u_dac = user_data[channel].u_demand + 0.5;

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

   watchdog_refresh(0);

   read_currents();
   if (time() > 300) { // current readout unreliable at startup
      check_currents();
      ramp_hvs();
      if (read_hv(&channel)) {
         regulation(channel);
      }
   }
}

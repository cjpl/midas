/********************************************************************\

  Name:         fgd_008.c
  Created by:   K. Mizouchi/Pierre-André Amaudruz    Sep/20/2006


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for FGD-008 SiPM HV control

  $Id$

\********************************************************************/
//  need to be define FGD_008

#include <stdio.h>
#include <math.h>
#include "mscbemb.h"
#include "SMBus_handler.h"
#include "fgd_008.h"
#include "fgd_008_tsensor.h"  

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_HV_CHN;

char code node_name[] = "FGD-008";

/* AD7718 pins */
sbit ADC_NRES = P3 ^ 2;         // !Reset
sbit ADC_SCLK = P0 ^ 6;         // Serial Clock
sbit ADC_NCS  = P3 ^ 1;         // !Chip select
sbit ADC_NRDY = P1 ^ 3;         // !Ready
sbit ADC_DOUT = P1 ^ 0;         // Data out
sbit ADC_DIN  = P0 ^ 7;         // Data in

/* LTC2600 pins */
sbit DAC_NCS  = P1 ^ 2;         // !Chip select
sbit DAC_SCK  = P0 ^ 6;         // Serial Clock
sbit DAC_CLR  = P1 ^ 1;         // Clear
sbit DAC_DIN  = P0 ^ 7;         // Data in

/* Charge Pump */
sbit INT_BIAS = P3 ^ 3;         // Internal bias path enable
sbit EXT_BIAS = P3 ^ 4;         // External bias path enable

unsigned char idata chn_bits[N_HV_CHN];
float         xdata u_actual[N_HV_CHN];
unsigned long xdata t_ramp[N_HV_CHN];

bit trip_reset;
bit once = 1;

struct user_data_type xdata user_data[N_HV_CHN];

MSCB_INFO_VAR code vars[] = {

   1, UNIT_BYTE,            0, 0,           0, "Control",   &user_data[0].control,            // 0
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "Udemand",   &user_data[0].u_demand,           // 1
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT, "Imeas",     &user_data[0].i_meas,             // 2
   1, UNIT_BYTE,            0, 0,           0, "Status",    &user_data[0].status,             // 3

   2, UNIT_VOLT,            0, 0,           0, "RampUp",    &user_data[0].ramp_up,            // 4
   2, UNIT_VOLT,            0, 0,           0, "RampDwn",  &user_data[0].ramp_down,          // 5
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "Ulimit",    &user_data[0].u_limit,            // 6
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT, "Ilimit",    &user_data[0].i_limit,            // 7
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "T_CPU",     &user_data[0].temperature_c8051,  // 8
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "TDiode1",  &user_data[0].temperature_diode1, // 9
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "TDiode2",  &user_data[0].temperature_diode2, // 10

   /* calibration constants */
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DACgain", &user_data[0].dac_gain,    // 11
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DACofs",  &user_data[0].dac_offset,  // 12
   4, UNIT_FACTOR,          0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURgain", &user_data[0].cur_gain,    // 13
   4, UNIT_AMPERE, PRFX_MICRO, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "CURofs",  &user_data[0].cur_offset,  // 14
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "VDAC",    &user_data[0].u_dac,       // 15
   0
};


MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

void user_init(unsigned char init)
{
   unsigned char  i;
   unsigned short address;

   /* all output open drain */
   P1MDOUT = 0x00;
   P2MDOUT = 0x00;

   // Turn off HV path. These two lines should be placed 
   // before operating charge-pump. 
   EXT_BIAS = 0x1; // Shutdown external voltage path 
   INT_BIAS = 0x1; // Shutdown internal voltage path

   /* initial nonzero EEPROM values */
   if (init) {

      for (i=0 ; i<N_HV_CHN ; i++) {
         user_data[i].u_demand  = 0;
         user_data[i].ramp_up   = 5.0;
         user_data[i].ramp_down = 5.0;
         user_data[i].u_limit   = MAX_VOLTAGE;
         user_data[i].i_limit   = MAX_CURRENT;

         // correction factors for DAC control (HV adjust) 
         user_data[i].dac_gain   = 1.004;  // slope  correction
         user_data[i].dac_offset = 0.0;    // offset correction

         // correction factors for current measurement
         user_data[i].cur_gain   = 1.;     // slope  correction
         user_data[i].cur_offset = 0.;     // offset correction
      }
   }

   /* default control register */
   for (i=0 ; i<N_HV_CHN ; i++) {
      user_data[i].control  = CONTROL_TSENSOR | CONTROL_ISENSOR;
	                       
      user_data[i].status   = 0;
      user_data[i].u_demand = 0;

      /* check maximum ratings */
      if (user_data[i].u_limit > MAX_VOLTAGE)
         user_data[i].u_limit = MAX_VOLTAGE;

      if (user_data[i].i_limit > MAX_CURRENT)
         user_data[i].i_limit = MAX_CURRENT;

      u_actual[i] = 0;
      t_ramp[i] = time();
   }

   address = 0x0;
   /* each device has 8 channels */
   address *= 8;                   

   
   /* keep high byte of node address */               
   address |= (sys_info.node_addr & 0xFF00);

   sys_info.node_addr = address;

   /* set default group address */
   if (sys_info.group_addr == 0xFFFF)
      sys_info.group_addr = 400;

   /* set-up DAC & ADC */
   DAC_CLR  = 1;
   ADC_NRES = 1;
   ADC_NRDY = 1; // input
   ADC_DIN  = 1; // input

   /* reset and wait for start-up of ADC */
   ADC_NRES = 0;
   delay_ms(100);
   ADC_NRES = 1;
   delay_ms(300);

   write_adc(REG_FILTER, ADC_SF_VALUE);

   // force update 
   for (i=0 ; i<N_HV_CHN ; i++)
      chn_bits[i] = DEMAND_CHANGED;

   // set normal LED mode (non-inverted mode)
   for (i=0 ; i<N_HV_CHN ; i++) {
      led_mode(i+2, 0);
   }

	// Other I/O 
    pca_operation(Q_PUMP_INIT);
    pca_operation(Q_PUMP_OFF);

    // init SMBus (should be called after pca_operation)
    SMBus_Initialization();

    // init temperature sensor 
    init_LM95231(0); 
   
    // Preset high voltage = 0 V. (Actually, ~2.4 V)
    for (i = 0; i < N_HV_CHN; i++) {
	    set_hv(i, 0);    
    }
/*	
	ET1 = 1; // timer 1 interrupt enable
	         // for fgd_008_ihander 
*/
}

/*---- PCA initilalization -----------------------------------------*/

void pca_operation(unsigned char mode)
{
   if (mode == Q_PUMP_INIT) {
	/* PCA setup for Frequency Output Mode on CEX0 */
    P0MDOUT  = 0x94; // Keep 4,7 for Rx,Enable + CEX0 output 
    XBR0     = 0x05; // Enable SMB, UART 
	XBR1     = 0x41; // Enable Xbar , Enable CEX0 on P0.
    PCA0MD   = 0x8;  // Sysclk (24.6MHz)
	PCA0CPL0 = 0x00; 
    PCA0CPM0 = 0x46; // ECM, TOG, PWM
	PCA0CPH0 = 0x06; // 6 (for ~2MHz)
    PCA0CN   = 0x40; // Enable PCA Run Control

   } else if (mode == Q_PUMP_OFF) {
    
    XBR1 = (XBR1 & 0xF8) | 0x00; // turn off Frequency Output Mode

   } else if (mode == Q_PUMP_ON) {    

    XBR1 = (XBR1 & 0xF8) | 0x01; // turn on Frequency Output Mode

   }
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   unsigned char i, command, mask;

   if (index == 0 || index == 1) {
      /* reset trip */
      user_data[cur_sub_addr()].status &= ~STATUS_ILIMIT;

      /* indicate new demand voltage */
      chn_bits[cur_sub_addr()] |= DEMAND_CHANGED;
   }

   /* re-check voltage limit */
   if (index == 6) {
      if (user_data[cur_sub_addr()].u_limit > MAX_VOLTAGE)
         user_data[cur_sub_addr()].u_limit = MAX_VOLTAGE;

      chn_bits[cur_sub_addr()] |= DEMAND_CHANGED;
   }

   /* check current limit */
   if (index == 7) {
      if (user_data[cur_sub_addr()].i_limit > MAX_CURRENT)
         user_data[cur_sub_addr()].i_limit = MAX_CURRENT;
   }

   // In case of you changing common control bit (ex. EXT/IN switch bit)
   // the change should be distributed to other channels.
   if (index == 0) {
       // preserve common command bits for all channels
       command = user_data[cur_sub_addr()].control;
       mask    = CONTROL_ISENSOR | CONTROL_TSENSOR | CONTROL_IB_CTL 
               | CONTROL_EXT_IN  | CONTROL_QPUMP   | CONTROL_IDLE; // common bits

       for (i = 0; i < N_HV_CHN; i++) {
          user_data[i].control &= ~mask;
          user_data[i].control |= (command & mask);
       }
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

unsigned char code dac_index[] = {0, 1, 2, 3, 4, 5, 6, 7};

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

/*------------------------------------------------------------------*/
/* adc mapping: 0U, 1I, 2U, 3U, ... */
unsigned char code adc_index[] = {0, 1, 2, 3, 4, 5, 6, 7};

unsigned char adc_read(unsigned char channel, float *value)
{
   unsigned long d, start_time;

   /* start conversion */
   channel = adc_index[channel % (N_HV_CHN+1)];
   write_adc(REG_CONTROL, channel << 4 | 0x0F); // adc_chn, unipol, +2.56V range
   write_adc(REG_MODE, (0x3<<4) | (1<<1));        // single conversion, 10 channel mode

   start_time = time();

   while (ADC_NRDY) {
      yield();

     /* Eamping at this point should be removed, otherwise MSCB 
	    communication sometimes timeout.
      for (i=0 ; i<N_HV_CHN ; i++)
         ramp_hv(i);      // do ramping while reading ADC
      */
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

   // Turn LED on over 10V
   // Truning on LED with "led_set" is cleared by the timer-1  
   // interruption, thus "led_mode" is used.
   led_mode(channel+2, (value > 10));

   // impose software voltage limit
   if (value >= MAX_VOLTAGE) {  
       value = MAX_VOLTAGE;
   }

   // apply voltage correction
   value = value * user_data[channel].dac_gain + user_data[channel].dac_offset;
 
   // convert to DAC voltage by the following equation
   value = 2 * V_ADJ_LT3010 - REGISTER_RATIO * (value - V_ADJ_LT3010);

   // impose DAC range limit 
   if (value > DAC_VREF) {
       value = DAC_VREF;
   }

   // impose ADJ voltage limit 
   if (value > DAC_VMAX_FOR_ADJ) {
       value = DAC_VMAX_FOR_ADJ;
   }

   // convert to DAC units
   // 16 bit resolution => 2^16 = 65536
   d = (unsigned short) (value / DAC_VREF * 65536.) - 1;

   /* write dac */
   write_dac(channel, d);
}


/*------------------------------------------------------------------*/

void check_current(unsigned char channel)
{
   /* do "software" check */
   if (user_data[channel].i_meas > user_data[channel].i_limit) {  
      // In the case of exceeding the current limit

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
      user_data[channel].status |= STATUS_ILIMIT;
   }

   if (user_data[channel].status & STATUS_ILIMIT)
      led_blink(channel+2, 10, 50);
}

/*------------------------------------------------------------------*/

void read_current(unsigned char mode, unsigned char channel)
{
   float current;
   
   if (mode == 0) return ;

   // read current channel 
   if (!adc_read(channel, &current))
      return;

   // conver to current in microamp 
   current = current / AMP_LT1787 / I_SENSE_REGISTER * 1E6;

   // apply for correction factors 
   current = current * user_data[channel].cur_gain 
                     + user_data[channel].cur_offset;
   // 0.001 resolution 
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
  if ( (user_data[channel].status & STATUS_QPUMP) &&
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
		 watchdog_refresh(0);
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

               led_blink(channel+2, 3, 150);
            }

            set_hv(channel, u_actual[channel]);
			watchdog_refresh(0);
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

               led_blink(channel+2, 3, 150);
            }

            set_hv(channel, u_actual[channel]);
			watchdog_refresh(0);
            t_ramp[channel] = time();
         }
      }
   }

   watchdog_refresh(0);
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   unsigned char channel;

   state_machine();

   /* loop over all HV channels */
    for (channel=0 ; channel<N_HV_CHN ; channel++) {

      watchdog_refresh(0);

      if ((user_data[0].control & CONTROL_IDLE) == 0) {
         /* instead, use software limit */
         check_current(channel);
         ramp_hv(channel);
         read_current(user_data[0].control & CONTROL_ISENSOR, channel);

         if ((user_data[0].control & CONTROL_IDLE) == 0) {
            process_temperature(user_data[0].control & CONTROL_TSENSOR);
         }

      }
    }


	// To be moved in PCA_operation once the 
	// init sequence is understood
	if (once) {
        PCA0MD   = 0x8;  // Sysclk (24.6MHz)
   	    once = 0;
	}
}

void state_machine(void) 
{
    // State machine handling routine. See "STATE TABLE" in fgd_008.h
    // Write down what you want to do here at each state.

    unsigned char i;
    unsigned char monitor;

    for (i = 0; i < N_HV_CHN; i++) {
        watchdog_refresh(0);

        monitor = user_data[i].control;

        if (!(user_data[i].control & CONTROL_EXT_IN) && 
            !(user_data[i].control & CONTROL_QPUMP )) {

            // STATE-1 (default state)
            user_data[i].status &= ~(STATUS_IB_CTL   | STATUS_EXT_BIAS 
                                   | STATUS_INT_BIAS | STATUS_QPUMP);

  		    pca_operation(Q_PUMP_OFF);
            // not change INT_BIAS, and EXT_BIAS
	        INT_BIAS = 1;
	        EXT_BIAS = 1;			
        }

        else if ( !(user_data[i].control & CONTROL_IB_CTL) &&
                  !(user_data[i].control & CONTROL_EXT_IN) &&
                   (user_data[i].control & CONTROL_QPUMP ) ) {
 
            // STATE-2  (internal bias)
            user_data[i].status &= ~(STATUS_IB_CTL | STATUS_EXT_BIAS); 
            user_data[i].status |= STATUS_INT_BIAS | STATUS_QPUMP;

  		    pca_operation(Q_PUMP_ON);
	        INT_BIAS = 0;
	        EXT_BIAS = 1;
        }

        else if ( user_data[i].control & CONTROL_EXT_IN ) {

            // STATE-3  (external bias)
            user_data[i].status &= ~(STATUS_IB_CTL | STATUS_INT_BIAS | STATUS_QPUMP);
            user_data[i].status |= STATUS_EXT_BIAS;

  		    pca_operation(Q_PUMP_OFF);
	        INT_BIAS = 1;
	        EXT_BIAS = 0;
        }

        else if ( (user_data[i].control & CONTROL_IB_CTL) &&
                 !(user_data[i].control & CONTROL_EXT_IN) &&
                  (user_data[i].control & CONTROL_QPUMP ) ) {

            // STATE-4  (internal bias "control" mode)
            user_data[i].status |= (STATUS_IB_CTL   | STATUS_EXT_BIAS 
                                  | STATUS_INT_BIAS | STATUS_QPUMP);

  		    pca_operation(Q_PUMP_ON);
	        INT_BIAS = 0;
	        EXT_BIAS = 0;
        }
    }
}   


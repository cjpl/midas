/********************************************************************\

  Name:         fgd_008.h
  Created by:   K. Mizouchi/Pierre-André Amaudruz    Sep/20/2006


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for FGD-008 SiPM HV control

  $Id$

\********************************************************************/
//  need to be define FGD_008

#ifndef  _FDG_008_H
#define  _FDG_008_H

/* number of HV channels */
#define N_HV_CHN         8

/* maximum current im micro Ampere */
#define MAX_CURRENT      600

/* maximum voltage in Volt */
#define MAX_VOLTAGE      55

// current measurement
#define AMP_LT1787       8.            // amplification factor in LT1787
#define I_SENSE_REGISTER 499.          // current sense register

// HV adjust 
#define V_ADJ_LT3010     1.275         // LT3010 ADJ  voltage 
#define DAC_VMAX_FOR_ADJ 2*1.275       // DAC maximum voltage      
#define DAC_VREF         2.5           // DAC reference vol = 2.5 V
#define REGISTER_RATIO   95.3e3/2.2e6  // 2.2 M Ohm and 95.3 k Ohm registers.
   
/* delay for opto-couplers in us */
#define OPT_DELAY        0    // 300

/* AD7718 registers */
#define REG_STATUS       0
#define REG_MODE         1
#define REG_CONTROL      2
#define REG_FILTER       3
#define REG_ADCDATA      4
#define REG_ADCOFFSET    5
#define REG_ADCGAIN      6
#define REG_IOCONTROL    7

#define ADC_SF_VALUE     82             // SF value for 50Hz rejection

// chn_bits (change request flag) bit map
#define DEMAND_CHANGED     (1<<0)
#define RAMP_UP            (1<<1)
#define RAMP_DOWN          (1<<2)

// charge pump state for PCA control
#define Q_PUMP_INIT          1           
#define Q_PUMP_OFF           2           
#define Q_PUMP_ON            3           

// =======================================================================
//      [State machine transition]
//  
// -------------------- Control Register Map -----------------------------
// Bit: Name        Default Description     
// [MSB]
//  7 : (CONTROL_IDLE) (0) (force CPU idle)     0 : normal,  1 : force idle
//  6 : (not assigned)  X
//  5 : CONTROL_ISENSOR 1   I measurement       0 : disable, 1 : enable
//  4 : CONTROL_TSENSOR 1   T measurement       0 : disable, 1 : enable
//  3 : (not assigned)  X
//  2 : CONTROL_IB_CTL  0   Int-bias cntrl mode 0 : disable, 1 : enable
//  1 : CONTROL_EXT_IN  0   Ext/In bias switch  0 : INTRNL,  1 : EXTRNL   
//  0 : CONTROL_QPUMP   0   Internal Q-Pump     0 : disable, 1 : enable
// [LSB]
// -----------------------------------------------------------------------
//
// -------------------- Status Register Map ------------------------------
// Bit: Name                Description     
// [MSB]
//  7 : STATUS_RAMP_DOWN    HV Rumping down     0 : idle,    1 : processing
//  6 : STATUS_RAMP_UP      HV Rumping up       0 : idle,    1 : processing
//  5 : STATUS_ILIMIT       Current Limit       0 :          1 : observed
//  4 : (not assigned))
//  3 : STATUS_IB_CTL       INT bias control    0 : disable, 1 : enable
//  2 : STATUS_EXT_BIAS     EXT bias            0 : disable, 1 : enable
//  1 : STATUS_INT_BIAS     INT bias            0 : disable, 1 : enable
//  0 : STATUS_QPUMP        Q-Pump status       0 : stopped, 1 : running
// [LSB]
// -----------------------------------------------------------------------
//
// [Abbreviations]
//  QP  := CONTROL_QPUMP or STATUS_QPUMP (depending on the contents)
//  E/I := CONTROL_EXT_IN
//  ICN := CONTROL_IB_CTL or STATUS_IB_CTL 
//  INT := STATUS_INT_BIAS
//  EXT := STATUS_EXT_BIAS
// -----------------------------------------------------------------------
//
//           STATE TABLE
//
//           |----------------------------------------------------|
//           |      CONTROL        |       STATUS                 |
//           |---------------------|------------------------------|
//           |   ICN    E/I    QP  |    ICN    EXT    INT    QP   |
//           |---------------------|------------------------------|
//  STATE-1  |   X      0      0   |    0      0      0      0    |
//  STATE-2  |   0      0      1   |    0      0      1      1    |
//  STATE-3  |   X      1      X   |    0      1      0      0    |     
//  STATE-4  |   1      0      1   |    1      1      1      1    |     
//           |----------------------------------------------------|
//   
// =======================================================================

/* CSR control bits */
#define CONTROL_QPUMP     (1<<0)
#define CONTROL_EXT_IN    (1<<1)
#define CONTROL_IB_CTL    (1<<2)

#define CONTROL_TSENSOR   (1<<4)
#define CONTROL_ISENSOR   (1<<5)

#define CONTROL_IDLE      (1<<7)

/* CSR status bits */
#define STATUS_QPUMP      (1<<0)
#define STATUS_INT_BIAS   (1<<1)
#define STATUS_EXT_BIAS   (1<<2)
#define STATUS_IB_CTL     (1<<3)

#define STATUS_ILIMIT     (1<<5)
#define STATUS_RAMP_UP    (1<<6)
#define STATUS_RAMP_DOWN  (1<<7)

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

struct user_data_type {
   unsigned char control;       // CSR control bits
   float         u_demand;      // Requested HV [V]
   float         i_meas;        // Measured current [mA]
   unsigned char status;        // CSR status bits

   unsigned int  ramp_up;       // Voltage step for ramp-up [V]
   unsigned int  ramp_down;     // Voltage step for ramp-down [V]

   float         u_limit;       // Software Max HV limit [V]
   float         i_limit;       // Current limit

   float         temperature_c8051;     
   float         temperature_diode1;
   float         temperature_diode2;
 
   float         dac_gain;      // linear correction for HV adjust
   float         dac_offset;    // offset correction for HV adjust
// float         cur_vgain;     // 
   float         cur_gain;      // linear correction in I measurement
   float         cur_offset;    // offset correction in I measurement
   float         u_dac;         // Actual HV generatd [V]
}; 

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_init(unsigned char init);
void user_loop(void);
void user_write(unsigned char index) reentrant;
unsigned char user_read(unsigned char index);
unsigned char user_func(unsigned char *data_in, unsigned char *data_out);

void write_dac(unsigned char channel, unsigned short value) reentrant;
void write_adc(unsigned char a, unsigned char d);

unsigned char adc_read(unsigned char channel, float *value);

void read_adc8(unsigned char a, unsigned char *d);
void read_adc24(unsigned char a, unsigned long *d);

void pca_operation(unsigned char mode);
void ramp_hv(unsigned char channel);
void set_hv(unsigned char channel, float value) reentrant;

void check_current(unsigned char channel);
void read_current(unsigned char mode, unsigned char channel);
void set_current_limit(float value);

void state_machine(void);
#endif

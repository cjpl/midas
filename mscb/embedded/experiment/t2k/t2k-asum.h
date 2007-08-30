/********************************************************************\

  Name:         fgd_008.h
  Created by:   Brian Lee  							Jun/07/2007


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for T2K-ASUM test board

  $Id$

\********************************************************************/
//  need to have T2KASUM defined

#ifndef  _T2K_ASUM_H
#define  _T2K_ASUM_H

/* ADT7486A temperature array addresses 
   --> 2 sensors per chip, so each address is repeated twice
   --> Order does not matter unless specified correctly */
#define ADT7486A_ADDR_ARRAY 0x4B
// also define number of temperature sensors you have on board (final design would be 4 ADT7486A's, 8 Remote)
#define ADT7486A_NUM 1 //currently, for the prototype, there is only one

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
#define CONTROL_TEMP_MEAS (1<<1)
#define CONTROL_ADC_MEAS  (1<<2)
#define CONTROL_BIAS_EN   (1<<3)
#define CONTROL_BIAS_DAC  (1<<4)
#define CONTROL_ASUM_TH   (1<<5)
#define CONTROL_D_CHPUMP  (1<<6)
#define CONTROL_KEEP_REF  (1<<7)

/* CSR status bits */
#define STATUS_QPUMP      (1<<0)
#define STATUS_TEMP_MEAS  (1<<1)
#define STATUS_ADC_MEAS   (1<<2)
#define STATUS_BIAS_EN    (1<<3)
#define STATUS_BIAS_DAC   (1<<4)
#define STATUS_ASUM_TH    (1<<5)
#define STATUS_D_CHPUMP   (1<<6)
#define STATUS_IDLE       (1<<7)

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

struct user_data_type {
	unsigned char control;       // CSR control bits
	unsigned char status;        // CSR status bits
	unsigned char biasEn;		// Bias Enable Switch control bits	
	unsigned int dac_asumThreshold;
	unsigned char dac_chPump;
	unsigned char biasDac1;
	unsigned char biasDac2;
	unsigned char biasDac3;
	unsigned char biasDac4;
	unsigned char biasDac5;
	unsigned char biasDac6;
	unsigned char biasDac7;
	unsigned char biasDac8;
	

	float pos_analogImonitor;
	float pos_analogVmonitor;
	float digitalImonitor;
	float biasReadBack;
	float digitalVmonitor;
	float neg_analogVmonitor;
	float neg_analogImonitor;

	float refCurrentSense;	
	float biasCurrentSense;

	
	float		 external_temp; // ADT7486A external temperature [degree celsius]

	float		 external_tempOffset; // ADT7486A external temperature offset[degree celsius]
	float		 internal_temp; // ADT7486A internal temperature [degree celsius]

}; 

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_init(unsigned char init);
void user_loop(void);
void user_write(unsigned char index) reentrant;
unsigned char user_read(unsigned char index);
unsigned char user_func(unsigned char *data_in, unsigned char *data_out);
void pca_operation(unsigned char mode);
void set_current_limit(float value);
void Hardware_Update(void);

#endif

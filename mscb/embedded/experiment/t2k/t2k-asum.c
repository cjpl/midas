/********************************************************************\
  Name:         fgd_008.c
  Created by:   Brian Lee  							Jun/07/2007


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for T2K-ASUM test board

  $Id:$

\********************************************************************/
//  need to have T2KASUM defined.

#include <stdio.h>
#include <math.h>
#include "../../mscbemb.h"
#include "t2k-asum.h"
#include "SMBus_handler.h"
#include "ADT7486A_tsensor.h"
#include "PCA9539_io.h"
#include "AD5301_dac.h"
#include "LTC_dac.h"
#include "AD7718_adc.h"

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_HV_CHN;

char code node_name[] = "T2K-ASUM";

/* Charge Pump */
sbit QPUMP_LED = P3 ^ 3;         // Bias status LED


bit trip_reset;
bit once = 1;

struct user_data_type xdata user_data[N_HV_CHN];

/* User Data structure declaration */
MSCB_INFO_VAR code vars[] = {

   1, UNIT_BYTE,            0, 0,           0, "Control",   &user_data[0].control,    // 0
   1, UNIT_BYTE,            0, 0,           0, "Status",    &user_data[0].status,      // 2
   1, UNIT_BYTE,            0, 0,           0, "D_BiasEN",   &user_data[0].biasEn,    // 1
   2, UNIT_BYTE,            0, 0,           0, "D_AsumTh",    &user_data[0].dac_asumThreshold,      // 3
   1, UNIT_BYTE,            0, 0,           0, "D_ChPump",    &user_data[0].dac_chPump,      // 3
   1, UNIT_BYTE,            0, 0,           0, "D_Bias1",    &user_data[0].biasDac1,      // 4
   1, UNIT_BYTE,            0, 0,           0, "D_Bias2",    &user_data[0].biasDac2,      // 5
   1, UNIT_BYTE,            0, 0,           0, "D_Bias3",    &user_data[0].biasDac3,      // 6
   1, UNIT_BYTE,            0, 0,           0, "D_Bias4",    &user_data[0].biasDac4,      // 7
   1, UNIT_BYTE,            0, 0,           0, "D_Bias5",    &user_data[0].biasDac5,      // 8
   1, UNIT_BYTE,            0, 0,           0, "D_Bias6",    &user_data[0].biasDac6,      // 9
   1, UNIT_BYTE,            0, 0,           0, "D_Bias7",    &user_data[0].biasDac7,      // 10
   1, UNIT_BYTE,            0, 0,           0, "D_Bias8",    &user_data[0].biasDac8,      // 11
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "+AnaIMon",    &user_data[0].pos_analogImonitor, // 12
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "+AnaVMon",    &user_data[0].pos_analogVmonitor, // 13 
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "DigIMon",    &user_data[0].digitalImonitor, // 14 
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "BiasRB",    &user_data[0].biasReadBack, //16 
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "DigVMon",    &user_data[0].digitalVmonitor, // 15 
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "-AnaVMon",    &user_data[0].neg_analogVmonitor, // 13 
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "-AnaIMon",    &user_data[0].neg_analogImonitor, // 12
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "RefCS",    &user_data[0].refCurrentSense, //17 
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "BiasCS",    &user_data[0].biasCurrentSense, //17 
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "ExtTemp",  &user_data[0].external_temp, //21
   
   //Hidden variables (wth "read all" call)
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "TempOS",  &user_data[0].external_tempOffset, //24(hidden)
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "IntTemp",  &user_data[0].internal_temp, //25(hidden)
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
   unsigned char abc = 0;

   /* all output open drain */
   P1MDOUT = 0x00;
   P2MDOUT = 0x00;

   // Turn off HV path. These two lines should be placed
   // before operating charge-pump.
   //EXT_BIAS = 0x1; // Shutdown external voltage path
   //INT_BIAS = 0x1; // Shutdown internal voltage path

   PCA0MD   = 0x8;  // Sysclk (24.6MHz)
    
   /* default settings */
   for (i=0 ; i<N_HV_CHN ; i++) {
      user_data[i].control  = 0x00;
      user_data[i].status   = 0x00;
	  user_data[i].biasEn   = 0x00;
	  user_data[i].dac_asumThreshold   = 0x80;
	  user_data[i].dac_chPump   = 0xFF;
	  user_data[i].biasDac1   = 0xFF;
	  user_data[i].biasDac2   = 0xFF;
	  user_data[i].biasDac3   = 0xFF;
	  user_data[i].biasDac4   = 0xFF;
	  user_data[i].biasDac5   = 0xFF;
	  user_data[i].biasDac6   = 0xFF;
	  user_data[i].biasDac7   = 0xFF;
	  user_data[i].biasDac8   = 0xFF;

      /* check maximum ratings */      

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

   /* set-up / initialize circuit components (order is important)*/
    ADT7486A_Init(); //Temperature measurements related initialization
	LTCdac_Init(); //LTC dacs initialization
	AD7718_Init(); //ADC initialization 		 
	pca_operation(Q_PUMP_INIT); //Charge Pump initialization (crossbar settings)
    pca_operation(Q_PUMP_OFF); //Initially turn it off
	SMBus_Init(); // SMBus initialization (should be called after pca_operation)
	PCA9539_Init(); //PCA General I/O (Bais Enables) initialization	
	AD5301_Init(); //I2C DAC initialization (D_CH_PUMP)

	user_data[0].control = 0x06; //temperature and adc readings ON
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
	PCA0CPH0 = 0x06 ; // 6 (for ~2MHz)
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

   // In case of you changing common control bit (ex. EXT/IN switch bit)
   // the change should be distributed to other channels.
   if (index == 0) {
       // preserve common command bits for all channels
       command = user_data[cur_sub_addr()].control;
       mask    = CONTROL_QPUMP; // common bits

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

/*---- CTL/Status management ------------------------------------------*/
void Hardware_Update(void)
{
    // State machine handling routine. See "STATE TABLE" in fgd_008.h
    // Write down what you want to do here at each state.

    unsigned char i = 0;

    for (i = 0; i < N_HV_CHN; i++) {
        watchdog_refresh(0);
/*
        if (!(user_data[i].control & CONTROL_EXT_IN) &&
            !(user_data[i].control & CONTROL_QPUMP )) {

            // STATE-1 (default state)
            user_data[i].status &= ~(STATUS_IB_CTL   | STATUS_EXT_BIAS
                                   | STATUS_INT_BIAS | STATUS_QPUMP);

  		    //pca_operation(Q_PUMP_OFF);
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

  		    //pca_operation(Q_PUMP_OFF);
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
*/

		if(user_data[i].control & CONTROL_QPUMP)
		{
			pca_operation(Q_PUMP_ON);
			QPUMP_LED = LED_ON;
		}
		else
		{
			pca_operation(Q_PUMP_OFF);
			QPUMP_LED = LED_OFF;
		}

		//Update the Bias_Enable status
		if((user_data[i].control & CONTROL_KEEP_REF) | (user_data[i].control & CONTROL_BIAS_EN))
		{
			PCA9539_Cmd(ADDR_PCA9539, 0x03, user_data[i].biasEn, PCA9539_WRITE);
			user_data[i].control &= ~(CONTROL_BIAS_EN);
		}			


		//Update the Charge Pump Threshold voltage
		if((user_data[i].control & CONTROL_KEEP_REF) | (user_data[i].control & CONTROL_D_CHPUMP))
		{
			AD5301_Cmd(ADDR_AD5301, (user_data[i].dac_chPump >> 4), (user_data[i].dac_chPump << 4), AD5301_WRITE);
			user_data[i].control &= ~(CONTROL_D_CHPUMP);
		}

		// Update Bias Dac voltages as requested
		if((user_data[i].control & CONTROL_KEEP_REF) | (user_data[i].control & CONTROL_BIAS_DAC))
		{
			LTCdac_Cmd(LTC1665_LOAD_A, user_data[cur_sub_addr()].biasDac1);
			LTCdac_Cmd(LTC1665_LOAD_B, user_data[cur_sub_addr()].biasDac2);
			LTCdac_Cmd(LTC1665_LOAD_C, user_data[cur_sub_addr()].biasDac3);
			LTCdac_Cmd(LTC1665_LOAD_D, user_data[cur_sub_addr()].biasDac4);
			LTCdac_Cmd(LTC1665_LOAD_E, user_data[cur_sub_addr()].biasDac5);
			LTCdac_Cmd(LTC1665_LOAD_F, user_data[cur_sub_addr()].biasDac6);
			LTCdac_Cmd(LTC1665_LOAD_G, user_data[cur_sub_addr()].biasDac7);
			LTCdac_Cmd(LTC1665_LOAD_H, user_data[cur_sub_addr()].biasDac8);
			user_data[i].control &=  ~CONTROL_BIAS_DAC;
		}

		// Update Bias Dac voltages as requested
		if((user_data[i].control & CONTROL_KEEP_REF) | (user_data[i].control & CONTROL_ASUM_TH))
		{
			LTCdac_Cmd(LTC2600_LOAD_H, (char) ((user_data[cur_sub_addr()].dac_asumThreshold >> 8) & 0xFF), (char) (user_data[cur_sub_addr()].dac_asumThreshold & 0xFF));
			user_data[i].control &=  ~CONTROL_ASUM_TH;
		}
    }
}

/*---- User loop function ------------------------------------------*/
void user_loop(void)
{
	unsigned char channel;

	/* loop over all HV channels */
	for (channel=0 ; channel<N_HV_CHN ; channel++) 
	{

	 	watchdog_refresh(0);

		if((user_data[channel].control & CONTROL_KEEP_REF) | (user_data[channel].control & CONTROL_TEMP_MEAS))
		{
			User_ADT7486A(); //Temperature Sensor user defined routines
		}

		if((user_data[channel].control & CONTROL_KEEP_REF) | (user_data[channel].control & CONTROL_ADC_MEAS))
		{
			User_AD7718(); //ADC Monitoring user defined routines
		}

		Hardware_Update();
	}	

	yield();
}



/********************************************************************\
  Name:         fgd_008_tsr.c
  Created by:   Brian Lee						    May/11/2007


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for FGD-008-TSR (Temperature Sensor Array)
				testing board
 
 $id:$
\********************************************************************/
//  need to have FGD_008_TSR defined.

#include <stdio.h>
#include "mscbemb.h"
#include "ADT7486A_tsensor.h"
#include "tsr.h"
#include "LTC2497.h"
extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_HV_CHN;

char code node_name[] = "TSR";

/* AD7718 pins */
sbit ADC_NRES = P1 ^ 0;         // !Reset
sbit ADC_SCLK = P0 ^ 6;         // Serial Clock
sbit ADC_NCS  = P1 ^ 5;         // !Chip select
sbit ADC_NRDY = P1 ^ 3;         // !Ready
sbit ADC_DOUT = P1 ^ 0;         // Data out
sbit ADC_DIN  = P0 ^ 7;         // Data in

/* LTC2600 pins */
sbit DAC_NCS  = P1 ^ 2;         // !Chip select
sbit DAC_SCK  = P0 ^ 6;         // Serial Clock
sbit DAC_CLR  = P1 ^ 7;         // Clear
sbit DAC_DIN  = P0 ^ 7;         // Data in

/* Charge Pump */
sbit INT_BIAS = P1 ^ 2;         // Internal bias path enable
sbit EXT_BIAS = P1 ^ 3;         // External bias path enable

/* Cross Bar Related */
//sbit SMBus_XBR_Enable = XBR0 ^ 2;
//sbit CP0_XBR_Enable = XBR0 ^ 4;
//sbit CP0_Fn_Enable = CPT0CN ^ 7;
//sbit CP0A_XBR_Enable = XBR0 ^ 5;

unsigned char idata chn_bits[N_HV_CHN];
float         xdata u_actual[N_HV_CHN];
unsigned long xdata t_ramp[N_HV_CHN];

bit trip_reset;
bit once = 1;

struct user_data_type xdata user_data[N_HV_CHN];

MSCB_INFO_VAR code vars[] = {
   //Main variables (with just "read" call)
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S1+",  &user_data[0].s1, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S2+",  &user_data[0].s2, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S3+",  &user_data[0].s3, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S4+",  &user_data[0].s4, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S5+",  &user_data[0].s5, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S6+",  &user_data[0].s6, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S7+",  &user_data[0].s7, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "S8+",  &user_data[0].s8, //0
	4, UNIT_VOLT,         	 0, 0, MSCBF_FLOAT, "DIFF1",  &user_data[0].diff1, //0
	4, UNIT_VOLT,         	 0, 0, MSCBF_FLOAT, "DIFF2",  &user_data[0].diff2, //0
	4, UNIT_VOLT,         	 0, 0, MSCBF_FLOAT, "DIFF3",  &user_data[0].diff3, //0
   4, UNIT_VOLT,         	 0, 0, MSCBF_FLOAT, "DIFF4",  &user_data[0].diff4, //0
	4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "ExtTemp",  &user_data[0].external_temp, //2(hidden)
   //Hidden variables (wth "read all" call)
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "TempOS",  &user_data[0].external_tempOffset, //1(hidden)
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "IntTemp",  &user_data[0].internal_temp, //2(hidden)
   0  // terminating zero, this is needed for the completion of user_data structure
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

   /* Set outputs' open-drain/push-pull */
   P0MDOUT = P0MDOUT | 0x08;

   RS485_ENABLE = 0; //get C8051 to transmit data out

   PCA0MD   = 0x8;  // Sysclk (24.6MHz)
    
   /* initial nonzero EEPROM values */
   if (init) {

      for (i=0 ; i<N_HV_CHN ; i++) {
         user_data[i].internal_temp = 0.0;
		 user_data[i].external_temp = 0.0;
		 user_data[i].external_tempOffset = 0.0;
      }
   }

   // force update
   for (i=0 ; i<N_HV_CHN ; i++)
      chn_bits[i] = DEMAND_CHANGED;

   // set normal LED mode (non-inverted mode)
   for (i=0 ; i<N_HV_CHN ; i++) {
      led_mode(i+2, 0);
   }

	/* Cross Bar Settings */
	//if the RS485 Communication is not working, it's 90% because of
	//crossbar settings!

	P0SKIP = 0x0F; //skip P0 ^ 2 and P0 ^ 3
				   //P0 ^ 4 and P0 ^ 5 are automatically assigned to be TX0 and RX0
				   //see below where crossbar routes the ports for TX0 and RX0

	XBR0 = 0x31; //route TX0 and RX0 to P0 ^ 4 and P0 ^ 5
	 				//CP0 is enabled and the output of this comparator
					//is routed to port pin P0 ^ 6; by cross bar priority decoder
					//CP0 asynchoronous output pint is routed to port P0 ^ 7
					//this is enabled to just check the difference between
					//synchronous comparator and asynchronous comparator

	/* Comparator 0 Settings */
	CPT0CN = 0x80; //Enable Comparator0 (functional, the one above is only for CrossBar)

	CPT0MX = 0x02; //Comparator0 MUX selection
				   //Negative input is set to P1 ^ 1, and Positive input is set to P2 ^ 0
				   // (P2 ^ 0 is the SST1, so we want to compare SST1 with the threshold voltage
				   //of !~0.8V on P1 ^ 1
	CPT0MD = 0x02; //Comparator0 Mode Selection
				   //Use default, adequate TYP (CP0 Response Time, no edge triggered interrupt)

    //initialize SST protocol related ports/variables/etc for ADT7486A
	ADT7486A_init(); 	
}



/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   index = 0;
   return;
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



/*---- User loop function ------------------------------------------*/
void user_loop(void)
{
	char i = 0;

	//LTC2497_ADC routines
	for(i = 0; i < 1; i++)
	{
		//LTC2497_Cmd(READ_S1, &user_data[i].s1);
		//LTC2497_Cmd(READ_S2, &user_data[i].s2);
		//LTC2497_Cmd(READ_S3, &user_data[i].s3);
		//LTC2497_Cmd(READ_S4, &user_data[i].s4);
		//LTC2497_Cmd(READ_S5, &user_data[i].s5);
		//LTC2497_Cmd(READ_S6, &user_data[i].s6);
		//LTC2497_Cmd(READ_S7, &user_data[i].s7);
		LTC2497_Cmd(READ_S8, &user_data[i].s8);
		//LTC2497_Cmd(READ_DIFF1, &user_data[i].diff1);
		//LTC2497_Cmd(READ_DIFF2, &user_data[i].diff2);
		//LTC2497_Cmd(READ_DIFF3, &user_data[i].diff3);
		LTC2497_Cmd(READ_DIFF4, &user_data[i].diff4);
	}

	User_tsensor(); //run SST user-defined routines	
}



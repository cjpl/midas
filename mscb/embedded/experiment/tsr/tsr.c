/********************************************************************\
  Name:         tsr.c
  Created by:   Brian Lee						    May/11/2007


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for FGD-008-TSR (Temperature Sensor Array)
				testing board
 
 $Id$
\********************************************************************/
//  need to have TSR defined on the compiler option

#include <stdio.h>
#include "mscbemb.h"
#include "ADT7486A_tsensor.h"
#include "tsr.h"
#include "LTC2497.h"
extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = N_CHN;

char code node_name[] = "TSR";

struct user_data_type xdata user_data[N_CHN];
unsigned char ADT7486A_addrArray[] = {ADT7486A_ADDR_ARRAY};

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
   if (init) 
	{
		sys_info.node_addr = 0x5;
		sys_info.group_addr = 400;
   }

	//general initial settings
	for (i=0 ; i<N_CHN ; i++) 
	{
		user_data[i].s1 = 0.0;
		user_data[i].s2 = 0.0;
		user_data[i].s3 = 0.0;
		user_data[i].s4 = 0.0;
		user_data[i].s5 = 0.0;
		user_data[i].s6 = 0.0;
		user_data[i].s7 = 0.0;
		user_data[i].s8 = 0.0;
		user_data[i].diff1 = 0.0;
		user_data[i].diff2 = 0.0;
		user_data[i].diff3 = 0.0;
		user_data[i].diff4 = 0.0;
		user_data[i].internal_temp = 0.0;
		user_data[i].external_temp = 0.0;
		user_data[i].external_tempOffset = 0.0;
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

	P2 &= 0xEF; //Ground for Voltage Divider (set Low)
	P1MDOUT = 0x00; //Set the Threshold pins P1.0 and P1.1 to Open-Drain
	P1MDIN &= 0xFC; //and set them to Analong Input for comparator input

    //initialize SST protocol related ports/variables/etc for ADT7486A
	ADT7486A_Init();

	// Turn All Leds ON and OFF to indicate the board is operating
	led_1 = LED_ON;
	led_2 = LED_ON;
	led_3 = LED_ON;
	led_4 = LED_ON;
	led_5 = LED_ON;
	led_6 = LED_ON;
	led_7 = LED_ON;	

	delay_ms(1000);
	led_1 = LED_OFF;
	led_2 = LED_OFF;
	led_3 = LED_OFF;
	led_4 = LED_OFF;
	led_5 = LED_OFF;
	led_6 = LED_OFF;
	led_7 = LED_OFF;
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
	char chNum = 0;

	//LTC2497_ADC routines
	for(chNum = 0; chNum < N_CHN; chNum++)
	{
		//LTC2497_Cmd(READ_S1, &user_data[chNum].s1);
		//LTC2497_Cmd(READ_S2, &user_data[chNum].s2);
		//LTC2497_Cmd(READ_S3, &user_data[chNum].s3);
		//LTC2497_Cmd(READ_S4, &user_data[chNum].s4);
		//LTC2497_Cmd(READ_S5, &user_data[chNum].s5);
		//LTC2497_Cmd(READ_S6, &user_data[chNum].s6);
		//LTC2497_Cmd(READ_S7, &user_data[chNum].s7);
		LTC2497_Cmd(READ_S8, &user_data[chNum].s8);
		//LTC2497_Cmd(READ_DIFF1, &user_data[chNum].diff1);
		//LTC2497_Cmd(READ_DIFF2, &user_data[chNum].diff2);
		//LTC2497_Cmd(READ_DIFF3, &user_data[chNum].diff3);
		LTC2497_Cmd(READ_DIFF4, &user_data[chNum].diff4);

		//run SST user-defined routines	
		ADT7486A_Cmd(ADT7486A_addrArray[4], GetExt2Temp, &user_data[chNum].external_temp);
		ADT7486A_Cmd(ADT7486A_addrArray[4], GetIntTemp, &user_data[chNum].internal_temp);
	}
}



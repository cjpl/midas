/********************************************************************\

  Name:         titan_rf.c
  Created by:   Brian Lee


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for Agilent RF Generators (Agilent 33220A)

  $Id: scs_310_kei2000.c 3435 2006-12-06 19:05:17Z ritt@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../mscbemb.h"

//define which RF Generator you are going to program
//Currently, 33220A and 33250A are supported
#define AGILENT_33220A 1
//#define AGILENT_33250A 1

//Control definitions
#define CONTROL_LADDERSTEP		(1<<0)
#define CONTROL_SWEEP	 		(1<<1)
#define CONTROL_SINEBURST 		(1<<2)
#define CONTROL_LADDERBURST	(1<<3)
#define CONTROL_FM		  		(1<<4)
#define CONTROL_NOT_USED1 		(1<<5)
#define CONTROL_NOT_USED2 		(1<<6)

//User defined Definitions
#define CHANGE_ON 1
#define CHANGE_OFF 2

// Node Name
char code node_name[] = "TITAN_RF";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

char xdata str[378];
extern SYS_INFO sys_info;

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct 
{
	unsigned char control;
	char gpib_adr;
	unsigned char Switch;
	float sweepDur;
	int numSteps;
	unsigned long int fStart;
	unsigned long int fEnd;
	float rfAmp;
	unsigned long int fBurst;
	int numCyc;   
} user_data;

MSCB_INFO_VAR code vars[] = {
   1, UNIT_BYTE,  			0, 0, 	 			 0, "Control",  &user_data.control,
   1, UNIT_BYTE, 				0, 0,              0, "GPIB Adr", &user_data.gpib_adr,
	1, UNIT_BYTE, 				0, 0,              0, "Switch",   &user_data.Switch,
	4, UNIT_SECOND,			0, 0,		MSCBF_FLOAT, "sweepDur", &user_data.sweepDur,
	2, UNIT_COUNT,			 	0, 0, 				 0, "numSteps", &user_data.numSteps,
	4, UNIT_HERTZ,  PRFX_MEGA, 0,					 0, "fStart"  , &user_data.fStart,
	4, UNIT_HERTZ,  PRFX_MEGA, 0,					 0, "fEnd"    , &user_data.fEnd,
	4, UNIT_VOLT,  PRFX_MILLI, 0,		MSCBF_FLOAT, "rfAmp"   , &user_data.rfAmp,
	4, UNIT_HERTZ,  PRFX_MEGA, 0,					 0, "fBurst"  , &user_data.fBurst,
	2, UNIT_COUNT, 			0, 0, 				 0, "numCyc"  , &user_data.numCyc,
   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

/* 8 data bits to LPT */
#define GPIB_DATA        P1

/* GPIB control/status bits DB40 */
sbit GPIB_EOI  = P2 ^ 1;         // Pin 5
sbit GPIB_DAV  = P2 ^ 2;         // Pin 6
sbit GPIB_NRFD = P2 ^ 3;         // Pin 7
sbit GPIB_NDAC = P2 ^ 4;         // Pin 8
sbit GPIB_IFC  = P2 ^ 5;         // Pin 9
sbit GPIB_SRQ  = P2 ^ 6;         // Pin 10
sbit GPIB_ATN  = P2 ^ 7;         // Pin 11
sbit GPIB_REM  = P2 ^ 0;         // Pin 17

#pragma NOAREGS

void user_write(unsigned char index) reentrant;
unsigned char send(unsigned char adr, char *str);
unsigned char send_byte(unsigned char b);
void fSweep(float sweepDur, unsigned long int fStart, unsigned long int fEnd, float rfAmp);
void sineBurst(unsigned long fBurst, int numCyc, float rfAmp);
void ladderBurst(int numSteps, int numCyc, float rfAmp, unsigned long fBurst);
void fm(unsigned long fStart, unsigned long fEnd, float rfAmp);
void rf_Init(void);
void hardWareUpdate(void);

char flag = CHANGE_OFF;
char kFlag = 0;
bit firstTime = 1;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   /* set initial state of lines */
   GPIB_DATA = 0xFF;
   GPIB_EOI = 1;
   GPIB_DAV = 1;
   GPIB_NRFD = 1;
   GPIB_NDAC = 1;
   GPIB_IFC = 1;
   GPIB_SRQ = 1;
   GPIB_ATN = 1;
   GPIB_REM = 1;

   /* initialize GPIB */
   GPIB_IFC = 0;
   delay_ms(1);
   GPIB_IFC = 1;

   GPIB_ATN = 0;
   send_byte(0x14);             // DCL
   GPIB_ATN = 1;

   if (init) {
      user_data.gpib_adr = 10;
		user_data.sweepDur = 8; // in seconds
		user_data.numSteps = 4;
		user_data.fStart = 100000; //in cHz
		user_data.fEnd = 200000; //in cHz
		user_data.rfAmp = 500; //mV
		user_data.fBurst = 1000000; //in cHz
		user_data.numCyc = 4;
		sys_info.node_addr = 0x01;
   }

	//Initialize RF Generators
	strcpy(str, " ");
	rf_Init();
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if(index == 0)	flag = CHANGE_ON;
	
	return;	
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
 	if(index);

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

/*---- Functions for GPIB port -------------------------------------*/

unsigned char send_byte(unsigned char b)
{
   unsigned int i;

   yield();

   /* wait for NRFD go high */
   for (i = 0; i < 1000; i++)
      if (GPIB_NRFD == 1)
         break;

   if (GPIB_NRFD == 0)
      return 0;

   GPIB_DATA = ~b;              // negate
   delay_us(1);                 // let signals settle
   GPIB_DAV = 0;

   /* wait for NDAC go high */
   for (i = 0; i < 1000; i++)
      if (GPIB_NDAC == 1)
         break;

   if (GPIB_NDAC == 0) {
      GPIB_DAV = 1;
      GPIB_DATA = 0xFF;
      return 0;                 // timeout
   }

   GPIB_DAV = 1;
   GPIB_DATA = 0xFF;            // prepare for input

   /* wait for NRFD go high */
   for (i = 0; i < 1000; i++)
      if (GPIB_NRFD == 1)
         break;

   if (GPIB_NRFD == 0)
      return 0;

   return 1;
}

/*------------------------------------------------------------------*/

unsigned char send(unsigned char adr, char *str)
{
   unsigned char i;

	#ifdef AGILENT_33250A
	delay_ms(100); //for 33250A, delay i needed
	#endif

  /*---- address cycle ----*/

   GPIB_ATN = 0;                // assert attention
   send_byte(0x3F);             // unlisten
   send_byte(0x5F);             // untalk
   send_byte(0x20 | adr);       // listen device
   send_byte(0x40 | 21);        // talk 21
   GPIB_ATN = 1;                // remove attention

  /*---- data cycles ----*/

   for (i = 0; str[i] > 0; i++)
      if (send_byte(str[i]) == 0)
         return 0;

   GPIB_EOI = 0;
   send_byte(0x0A);             // NL
   GPIB_EOI = 1;

   return i;
}

/* User Defined Function Definitions */
void rf_Init(void)
{
	send(user_data.gpib_adr, "OUTPut OFF");
	send(user_data.gpib_adr, "*CLS");
	send(user_data.gpib_adr, "OUTPut:LOAD MAXimum");	
	send(user_data.gpib_adr, "TRIGger:SOURce EXTernal");
	send(user_data.gpib_adr, "TRIGger:SLOPe POSitive");

	//turn all the states off to avoid beeping sounds later
	send(user_data.gpib_adr, "SWEep:STATe OFF");
	send(user_data.gpib_adr, "BURSt:STATe OFF");
	send(user_data.gpib_adr, "FM:STATe OFF");
}

void fSweep(float sweepDur, unsigned long int fStart, unsigned long int fEnd, float rfAmp)
{
	rf_Init();
		
	sprintf(str, "FREQuency:STARt %lu e-2", fStart);
	send(user_data.gpib_adr, str);

	sprintf(str, "FREQuency:STOP %lu e-2", fEnd);
	send(user_data.gpib_adr, str);

	send(user_data.gpib_adr, "SWEep:SPACing LINear");

	sprintf(str, "SWEep:TIME %e", sweepDur);
	send(user_data.gpib_adr, str);

	sprintf(str, "VOLTAGE %e", (rfAmp / 1000));
	send(user_data.gpib_adr, str);

	//finally, turn on the Sweep Mode
	send(user_data.gpib_adr, "SWEep:STATe ON");
	send(user_data.gpib_adr, "OUTPut ON");
}

void sineBurst(unsigned long fBurst, int numCyc, float rfAmp)
{
	rf_Init();

	send(user_data.gpib_adr, "BURSt:MODE TRIGgered");

	sprintf(str, "APPLy:SINusoid %lu e-2, %e", fBurst, (rfAmp / 1000));
	send(user_data.gpib_adr, str);

	sprintf(str, "BURSt:NCYCles %d", numCyc);
	send(user_data.gpib_adr, str);

	//send(user_data.gpib_adr, "BURSt:INTernal:PERiod 10e-3");

	//finally, turn on the Sine Burst Mode
	send(user_data.gpib_adr, "BURSt:STATe ON");
	send(user_data.gpib_adr, "OUTPut ON");
}

void ladderBurst(int numSteps, int numCyc, float rfAmp, unsigned long fBurst)
{
	int i = 0;
	char xdata buffer[30];

	rf_Init();

	strcpy(str, "DATA VOLATILE");
	for(i = 0; i < numSteps; i++)
	{
		sprintf(buffer, ", %2.2f", (double) ((((double)(2 * i)) / ((double)(numSteps -1))) - 1));
		strcat(str, buffer);
	}

	if(!firstTime)	
	{
		send(user_data.gpib_adr, "FUNCtion:USER EXP_RISE");
		send(user_data.gpib_adr, "DATA:DELete TWISTLADDER");		
	}
	//delay_ms(500);
	#ifdef AGILENT_33250A
	//delay_ms(500); //additional delay fo 33250A
	#endif
	send(user_data.gpib_adr, str);
	delay_ms(500);
	yield();
	send(user_data.gpib_adr, "DATA:COPY TWISTLADDER");
	delay_ms(500);
	if(firstTime) firstTime = 0;
	send(user_data.gpib_adr, "FUNCtion:USER TWISTLADDER");
	send(user_data.gpib_adr, "FUNCtion USER");

	sprintf(str, "BURSt:NCYCles %d", numCyc);
	send(user_data.gpib_adr, str);

	sprintf(str, "APPLy:USER %lu e-2, %e", fBurst, (rfAmp / 1000));
	send(user_data.gpib_adr, str);

	//finally, turn on the Ladder Burst Mode
	send(user_data.gpib_adr, "BURSt:STATe ON");
	send(user_data.gpib_adr, "OUTPut ON");
}

void fm(unsigned long fStart, unsigned long fEnd, float rfAmp)
{	
	rf_Init();

	send(user_data.gpib_adr, "FM:SOURce EXTernal");

	sprintf(str, "FM:DEViation %lu e-2", fEnd - fStart);
	send(user_data.gpib_adr, str);

	sprintf(str, "VOLTAGE %e", (rfAmp / 1000));
	send(user_data.gpib_adr, str);

	//finally, turn on the Sweep Mode
	send(user_data.gpib_adr, "FM:STATe ON");
	send(user_data.gpib_adr, "OUTPut ON");
}

void hardWareUpdate(void)
{
	if(flag == CHANGE_ON)
	{
		if(user_data.control & CONTROL_SWEEP)
		{
			fSweep(user_data.sweepDur, user_data.fStart, user_data.fEnd, user_data.rfAmp);
		}

		else if(user_data.control & CONTROL_SINEBURST)
		{
			sineBurst(user_data.fBurst, user_data.numCyc, user_data.rfAmp);
		}

		else if(user_data.control & CONTROL_LADDERBURST)
		{
			ladderBurst(user_data.numSteps, user_data.numCyc, user_data.rfAmp, user_data.fBurst);
		}

		else if(user_data.control & CONTROL_FM)
		{
			fm(user_data.fStart, user_data.fEnd, user_data.rfAmp);
		}
		flag = CHANGE_OFF;
	}
}

/*---- User loop function ------------------------------------------*/
void user_loop(void)
{
	hardWareUpdate();
}

/********************************************************************\

  Name:         tsr.h
  Created by:   K. Mizouchi/Pierre-André Amaudruz    Sep/20/2006


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for FGD-008 SiPM HV control

  $Id$

\********************************************************************/
//  need to be define FGD_008_TSR

#ifndef  _TSR_H
#define  _TSR_H

/* number of HV channels */
#define N_CHN         1

/* ADT7486A temperature array addresses 
   --> 2 sensors per chip, so each address is repeated twice
   --> Order does not matter unless specified correctly */
#define ADT7486A_ADDR_ARRAY 0x50, 0x50, 0x4F, 0x4F, 0x4D, 0x4D, 0x4C, 0x4C

#define TEMP_LIMIT    30 //in degC

/* LED declarations */
sbit led_1 = P3 ^ 1;
sbit led_2 = P3 ^ 2;
sbit led_3 = P3 ^ 3;
sbit led_4 = P3 ^ 4;
sbit led_5 = P2 ^ 7;
sbit led_6 = P2 ^ 6;
sbit led_7 = P2 ^ 5;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

struct user_data_type 
{
	float s1;	
	float s2;
	float s3;
	float s4;
	float s5;
	float s6;
	float s7;
	float s8;
	float diff1;
	float diff2;
	float diff3;
	float diff4;
	
   float	internal_temp; // ADT7486A internal temperature [degree celsius]
   float	external_temp; // ADT7486A external temperature [degree celsius]
   float	external_tempOffset; // ADT7486A external temperature offset[degree celsius] 
}; 

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_init(unsigned char init);
void user_loop(void);
void user_write(unsigned char index) reentrant;
unsigned char user_read(unsigned char index);
unsigned char user_func(unsigned char *data_in, unsigned char *data_out);
void Temp_LedMng(void);

#endif

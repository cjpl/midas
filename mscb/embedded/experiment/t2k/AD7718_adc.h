/********************************************************************\

  Name:         AD7718_adc.c
  Created by:   Brian Lee  								Jun/14/2007


  Contents:     AD7718BRU ADC user interface

  $Id$

\********************************************************************/
//  need to have T2KASUM defined

#ifndef  _AD7718_ADC_H
#define  _AD7718_ADC_H

/* AD7718 registers */
#define REG_STATUS       0
#define REG_MODE         1
#define REG_CONTROL      2
#define REG_FILTER       3
#define REG_ADCDATA      4
#define REG_ADCOFFSET    5
#define REG_ADCGAIN      6
#define REG_IOCONTROL    7

/* AD7718 User Command mapping */
#define AD7718_WRITE	0 //Write
#define READ_AIN2		2, 0x1 //+6V_Analog_IMon
#define READ_AIN3		3, 0x2 //+6V_Analog_VMon
#define READ_AIN4		4, 0x3 //+6V_Dig_IMon
#define READ_AIN5		5, 0x4 //Bias_Readback
#define READ_AIN6		6, 0x5 //+6V_Dig_VMon
#define READ_AIN7		7, 0x6 //-6V_Analog_VMon
#define READ_AIN8		8, 0x7 //-6V_Analog_IMon
#define READ_REFCS		9, 0xF //BiasCurrentSense
#define READ_BIASCS		10, 0xE //BiasCurrentSense


#define ADC_SF_VALUE     82             // SF value for 50Hz rejection

void AD7718_Init(void);
void AD7718_Cmd(unsigned char cmd, unsigned char adcChNum, float *varToBeWritten);
unsigned long AD7718_Read(void);
void AD7718_Clear(void);

#endif


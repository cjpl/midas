/**********************************************************************************\
  Name:         ADT7486A_tsensor.h
  Created by:   Brian Lee						     May/11/2007


  Contents:     Temperature sensor array (ADT7486A) handler

  Version:		Rev 1.2

  $Id$
\**********************************************************************************/

#ifndef  _ADT7486A_TSENSOR_H
#define  _ADT7486A_TSENSOR_H

// --------------------------------------------------------
// ADT7486A related definitions and function prototypes
// --------------------------------------------------------

// Command Map (ADT7486A) - including WL and RL
// Format is as follows:
// #define CommandName writeLength, readLength, command, data(if applicable, defined by user), channel number
                             //dummy bytes are written for those
                             //not applicable to fit functin prototype
#define Ping      0x00, 0x00, 0x00, 0x00, 0x00
#define GetIntTemp    0x01, 0x02, 0x00, 0x00, 0x00
#define GetExt1Temp   0x01, 0x02, 0x01, 0x00, 0x00
#define GetExt2Temp   0x01, 0x02, 0x02, 0x00, 0x00
#define GetAllTemps   0x01, 0x06, 0x00, 0x00, 0x00
#define SetExt1Offset 0x03, 0x00, 0xE0
#define GetExt1Offset 0x01, 0x02, 0xE0, 0x00, 0x00
#define SetExt2Offset 0x03, 0x00, 0xE1
#define GetExt2Offset 0x01, 0x02, 0xE1, 0x00, 0x00
#define ResetDevice   0x01, 0x00, 0xF6, 0x00, 0x00
#define GetDIB_8bytes 0x01, 0x10, 0xF7, 0x00, 0x00
#define DONT_READ_DATA  0xff
#define READ_DATA     0x00
#define TEMP_LIMIT    30
#define AVG_COUNT   1
#define ADT7486A_CONVERSION_TIME 38 //in ms

void ADT7486A_Init(void);
void ADT7486A_Cmd(unsigned char addr, unsigned char writeLength, unsigned char readLength,
        unsigned char command, unsigned char datMSB, unsigned char datLSB, float *dataToBeWritten);
float ADT7486A_Read(unsigned char writeFCS_Originator, unsigned char cmdFlag);

#endif
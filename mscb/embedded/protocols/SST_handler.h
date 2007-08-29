/**********************************************************************************\
  Name:         SST_handler.h
  Created by:   Brian Lee                May/11/2007


  Contents:   SST protocol for temperature sensor array (ADT7486A)

  Version:    Rev 1.2

  $Id: SST_handler.h 3838 2007-08-28 20:35:21Z amaudruz $
\**********************************************************************************/

#ifndef  _SST_HANDLER_H
#define  _SST_HANDLER_H

// --------------------------------------------------------
// SST Protocol related definitions and function prototypes
// --------------------------------------------------------

#define T_BIT 16  //Time Bit for both the Address and Message (AT and MT)
#define T_H1 T_BIT * 0.75  //Logic High time bit
#define T_H0 T_BIT * 0.25  //Logic Low time bit
#define SST_CLEAR_DELAY 2 //in milliseconds, not exactly measured, an assumed one that works

//usable from outside
void SST_Clear(void);
void SST_WriteByte(unsigned char datByte);
unsigned char SST_ReadByte(void);
void SST_Init(void);
unsigned char FCS_Step(unsigned int msg, unsigned char fcs);

//internal functions
void SST_DrvHigh(void);
void SST_DrvLow(void);
unsigned char SST_DrvClientResponse(void);

#endif


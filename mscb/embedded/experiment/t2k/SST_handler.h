// --------------------------------------------------------
//  SST handling routines for C8051F310
//      - Writtern for Temperature Sensor Array board
//      - Brian Lee   (May/11/2007)
//
//  Version:    Rev 1.2
//
//  $Id$
// --------------------------------------------------------
#ifndef  _SST_HANDLER_H
#define  _SST_HANDLER_H

// --------------------------------------------------------
// SST Protocol related definitions and function prototypes
// --------------------------------------------------------

#define T_BIT 16  //Time Bit for both the Address and Message (AT and MT)
#define T_H1 T_BIT * 0.75  //Logic High time bit
#define T_H0 T_BIT * 0.25  //Logic Low time bit
#define SWITCH_TO_SST1 SST = SST1; //CPT0MD = 0x22//Command to switch the line to SST1
//#define SWITCH_TO_SST2 SST = SST2; SST_ClientResponse = SST2_ClientResponse; CPT0MD = 0x20; //Command to switch the line to SST2
#define SST_CLEAR_DELAY 38 //in milliseconds

void SST_Clear(void);
void SST_DrvHigh(void);
void SST_DrvLow(void);
unsigned char SST_DrvClientResponse(void);
void SST_DrvByte(unsigned char datByte);
unsigned char SST_ReadByte(void);
void SST_Init(void);
void SST_SwitchLine(unsigned char lineNum);
unsigned char FCS_Step(unsigned int msg, unsigned char fcs);

#endif


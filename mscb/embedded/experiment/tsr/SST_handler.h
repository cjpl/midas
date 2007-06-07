// --------------------------------------------------------
//  SST handling routines for C8051F310
//      - Writtern for Temperature Sensor Array board
//      - Brian Lee   (May/11/2007)
//
// --------------------------------------------------------
#ifndef  _SST_HANDLER_H   
#define  _SST_HANDLER_H

// --------------------------------------------------------
// SST Protocol related definitions and function prototypes
// --------------------------------------------------------

#define T_BIT 12  //Time Bit for both the Address and Message (AT and MT)
#define T_H1 T_BIT * 0.75  //Logic High time bit
#define T_H0 T_BIT * 0.25  //Logic Low time bit
#define SWITCH_TO_SST1 SST = SST1; SST_ClientResponse = SST1_ClientResponse; CPT0MD = 0x02//Command to switch the line to SST1
#define SWITCH_TO_SST2 SST = SST2; SST_ClientResponse = SST2_ClientResponse; CPT0MD = 0x20; //Command to switch the line to SST2
#define CLKFREQ 24500000

void SST_Clear(void);
void SST_DrvHigh(void);
void SST_DrvLow(void);
unsigned char SST_DrvClientResponse(void);
void SST_DrvByte(unsigned char datByte);
float SST_Read(unsigned char writeFCS_Originator, unsigned char cmdFlag);
void SST_Init(void);
void SST_SwitchLine(unsigned char lineNum);
unsigned char FCS_Step(unsigned int msg, unsigned char fcs);

#endif


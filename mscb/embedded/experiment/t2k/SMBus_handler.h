// --------------------------------------------------------
//  SMBus handling routines for T2K-FGD experiment
//      - Writtern for T2K-ASUM test board 
//      - Brian Lee   (Jun/08/2006)
//
// --------------------------------------------------------
#ifndef  _SMBUS_HANDLER_H   
#define  _SMBUS_HANDLER_H

#define WRITE 0
#define READ 1
#define CMD 2
#define VAL 3
#define SMBUS_SI_WAITTIME 100 //in ms

void SMBus_Init(void);
void SMBus_Start(void);
void SMBus_WriteByte(unsigned char Byte, unsigned char RW_flag);
unsigned char SMBus_ReadByte(void);
void SMBus_Stop(void);
bit SMBus_SerialInterrupt(void);

#endif


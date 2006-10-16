// --------------------------------------------------------
//  SMBus handling routines for C8051F310
//      - Writtern for FGD 8-channel bias board 
//      - K. Mizouchi   (Sep/26/2006)
//
// --------------------------------------------------------
#ifndef  _SMBUS_HANDLER_H   
#define  _SMBUS_HANDLER_H

void          SMBus_Initialization(void);
void          SMBus_Timeout_Interrupt(void);

// Write "command string" + "data string" 
unsigned char SMBus_Write_CD(unsigned char address, 
                             unsigned char command, unsigned char  value);
// Write "command string" and then read "data string" 
unsigned char SMBus_Read_CD (unsigned char address, 
                             unsigned char command, unsigned char * value);
// Single byte comminucation
unsigned char SMBus_Write_Byte(unsigned char address, unsigned char  value);
unsigned char SMBus_Read_Byte (unsigned char address, unsigned char * value);

#endif


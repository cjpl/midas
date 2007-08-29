/**********************************************************************************\
  Name:         SST_handler.c
  Created by:   Brian Lee                Jun/08/2007


  Contents:     SMBus protocol for T2K-FGD experiment
          (using C8051F310's internal SMBus features)

  Version:    Rev 1.0

  Last updated: Jun/08/2007
          - First created

  $Id: SMBus_handler.c 3731 2007-07-03 21:10:34Z amaudruz $
\**********************************************************************************/
// --------------------------------------------------------
//  Include files
// --------------------------------------------------------

#include "../mscbemb.h"
#include "SMBus_handler.h"

// --------------------------------------------------------
//  SMBus_Initialization :
//      Bus Initialization sequence
// --------------------------------------------------------
void  SMBus_Init(void)
{
    // timer 1 is usually initialized by mscbmain.c
  // If you make no change in that file, you do not need to
  // activate the following two lines.
  // TMOD = (TMOD & 0x0F) | 0x20;     // 8 bit auto-reloaded mode
    // TR1 = 0;                         // Activate timer 1

    // SMBus Clock/Configuration
  // SMBus enable, slave inhibit, setup and hold time extension,
  // timeout detection with timer 3
  // free timeout detection, timer1 overflow as clock source
  // Note :"timer1 overflow" is required for clock source
  //  SMB0CF  = 0x5D;     // SMBus configuration
    SMB0CF  = 0x55;     // SMBus configuration
    SMB0CF |= 0x80;     // Enable SMBus after all setting done.
}

// --------------------------------------------------------
//  SMBus_Write_CD :
//      Write a single command and data word to the SMBus as a Master
//
//      Arguments :
//          slave_address   : 7-bit slave address
//          command         : command word
//          value           : data word
//
//      Return    :
//          0               : success
//          non-zero        : error
// --------------------------------------------------------
void SMBus_Start(void)
{
  SI = 0;
  STA = 1;    // make Start bit
  SMBus_SerialInterrupt(); // wait for SMBus interruption..
  STA = 0;    // clear start bit
}

void SMBus_WriteByte(unsigned char Byte, unsigned char RW_flag)
{
  if(RW_flag == WRITE) //write flag on
  {
    SMB0DAT = (Byte << 1) & 0xFE;
  }
  else if(RW_flag == READ) //read flag on
  {
    SMB0DAT = (Byte << 1) | 0x01;
  }
  else //if it's just writing a value or command (CMD or VAL)
  {
    SMB0DAT = Byte; // Set the SMBus Data to be the desired Byte value
  }
  SI = 0;
  SMBus_SerialInterrupt();  // wait for SMBus interruption. sending the byte..
}

unsigned char SMBus_ReadByte(void)
{
  SI      = 0;  // clear interrupt flag
    SMBus_SerialInterrupt();    // wait SMBus interruption. receiving data word ...
  ACK     = 0;        // send back NACK
    return SMB0DAT; // return the Received Byte
}

void SMBus_Stop(void)
{
  STO = 1;    // make Stop bit
    SI = 0;   // clear interrupt flag
}

bit SMBus_SerialInterrupt(void)
{
  unsigned char startTime = 0;

  startTime = time();

  while(SI == 0);
  {
    //if SI doesn't turn on for longer than the defined SIwaitTime
    if((time() - startTime) > SMBUS_SI_WAITTIME)
    {
      return 1; //return 1 as an indication for communication failure
    }
  }

  //if nothing goes wrong and SI does turn on
  return 0; //success
}
/**********************************************************************************\
  Name:         SST_handler.c
  Created by:   Brian Lee                May/11/2007


  Contents:     SST protocol for general MSCB-embedded experiments

  Version:    Rev 1.2

  Last updated: Jun/08/2007
          - Finished Commenting (Rev 1.0 Completed)
        - Got rid of exp function, replaced with different and simpler algorithm
        - Separated some funtions to another file called ADT7486A_tsensor.c and .h
          (Rev 1.1)
        - More modulization and improved compatibility with other SST devices
        - added crossbar settings

  $Id: SST_handler.c 3838 2007-08-28 20:35:21Z amaudruz $
\**********************************************************************************/

// --------------------------------------------------------
//  Include files
// --------------------------------------------------------
#include "../mscbemb.h"
#include "SST_handler.h"

/* SST related variables */
sbit SST = MSCB_SST1; //This variable chooses between SST lines (e.g. SST1, SST2, etc)
				 //Default to SST1
				 //SFR definitions of SST1 and SST2 ports are defined in mscbemb.h

#ifndef SST_ClientResponse
sbit SST_ClientResponse = SST1_ClientResponse; //Response read through
											   			  //comparator output
#endif

/* CRC-8 Table for Polynomial x^8 + x^2 + x^1 + 1 (used for FCS in SST Protocol) */
unsigned char code FCS_data[] = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
  0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
  0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
  0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
  0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
  0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
  0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
  0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
  0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
  0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
  0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
  0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
  0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
  0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
  0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
  0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
  0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
  0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
  0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
  0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
  0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
  0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
  0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
  0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
  0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
  0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
  0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3,
};

unsigned char FCS_Step(unsigned int msg, unsigned char fcs)
/**********************************************************************************\

  Routine: FCS_step

  Purpose: Single calculation for 8-bit cyclic redundancy checksum
         usually used in FORWARD recursive format (to save code-space)

  Input:
    unsigned char msg       new msg
    unsigned char fcs       running fcs

  Function value:
    unsighend char          FCS (CRC-8) code

\**********************************************************************************/
{
   return FCS_data[(msg ^ fcs)];
}

void SST_Init(void)
/**********************************************************************************\

  Routine: init_ADT7486A

  Purpose: ADT7486A temperature sensor array initialization

  Input:
    void

  Function value:
    void

\**********************************************************************************/
{	
	//Clear the SST signals (drive Low for a certain period)
	SST_Clear();
}

void SST_Clear(void)
/**********************************************************************************\

  Routine: SST_Clear

  Purpose: To drive SST line to low for a reasonably long time
         which gives enough time for the SST to go idle

  Input:
    void

  Function value:
    void

\**********************************************************************************/
{
  // Test this, high low high low
  SST = 0;
  delay_ms(SST_CLEAR_DELAY); //clear to 0's
}

void SST_DrvHigh(void)
/**********************************************************************************\

  Routine: SST_DrvHigh

  Purpose: To drive SST line to a logic "high" or "1", which is
         defined as driving SST high 3/4 cycle of t_Bit and the
       rest of it low

  Input:
    void

  Function value:
    void

\**********************************************************************************/
{
  // Drive high for 0.75 * T_BIT
  SST = 1;
  delay_us(T_H1);
  SST = 0; //drive the rest of T_BIT low
  delay_us(T_BIT - T_H1);
}

void SST_DrvLow(void)
/**********************************************************************************\

  Routine: SST_DrvLow

  Purpose: To drive SST line to a logic "low" or "0", which is
         defined as driving SST high for 1/4 cycle of t_Bit and the
       rest of it low

  Input:
    void

  Function value:
    void

\**********************************************************************************/
{
  // Drive high for 0.25 * T_BIT
  SST = 1;
  delay_us(T_H0);
  SST = 0; //drive the rest of T_BIT low
  delay_us(T_BIT - T_H0);
}

unsigned char SST_DrvClientResponse(void)
/**********************************************************************************\

  Routine: SST_DrvClientResponse

  Purpose: To drive SST line to a logic "low" or "0", which is
         defined as driving SST high for 1/4 cycle of t_Bit and the
       rest of it low

  Input:
    void

  Function value:
    unsigned char     Value of Read Bits from Client's Response
              Only 1 or 0 (gone through comparator)

\**********************************************************************************/
{
  // Drive SST to logic "0" (high 1/4, low 3/4) to let Client
  // drive SST to its desired state
  P2MDOUT |= 0x01; //put SST line to push-pull mode
  SST = 1; //Drv high (originator drives SST bus high to toggle client's response

  P2MDOUT &= 0xFE; //put back to open-drain to listen to client's response

  delay_us(T_BIT / 2); //delay for half of T_BIT time

  if(SST_ClientResponse == 1)
  //if the comparator output is high, then return 1
  {
    delay_us(2 * T_H0);
    return 1;
  }
  else
  //if the comparator output is low, then return 0
  {
    delay_us(2 * T_H0);
    return 0;
  }
}

void SST_WriteByte(unsigned char datByte)
/**********************************************************************************\

  Routine: SST_DrvByte

  Purpose: To decode the input Byte and drive SST Low or High according
         to the binary data bits defined in datByte

  Input:
    unsigned char datByte   a Byte (8 bits) that the originator
                wants to drive SST to
                e.g.) 0100 1100 (0x4C, one of addresses)

  Function value:
    void

\**********************************************************************************/
{
  //declare local variables
  unsigned char toBeDrv = 0;
  int i = 0;

  for(i = 7; i >= 0; i--) //8bits in 1 byte
  {
    toBeDrv = (datByte >> i) & 0x1;
    if(toBeDrv == 0) //if the bit to be driven is logic 0
    {
      SST_DrvLow();
    }
    else if(toBeDrv == 1) //if the bit to be driven is logic 1
    {
      SST_DrvHigh();
    }
    else //if the bit to be driven is neither 0 nor 1
    {
      return; //error
    }
  }
}

unsigned char SST_ReadByte(void)
{
  unsigned char din = 0;
  unsigned char dataReceived = 0;
  signed char j = 0;

  for(j = 7; j >= 0; j--) //writeFCS, 1byte
  {
    //Send logic 0's to let client drive SST for Read Bits
    din = SST_DrvClientResponse();
    dataReceived |= (din << j);
  }

  return dataReceived;
}


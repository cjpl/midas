/**********************************************************************************\
  Name:         SPI_handler.c
  Created by:   Brian Lee                Jun/14/2007


  Contents:     SPI protocol for T2K-FGD experiment

  Version:    Rev 1.0

  Last updated:

  $Id: SPI_handler.h 3731 2007-07-03 21:10:34Z amaudruz $
\**********************************************************************************/
//  need to have T2KASUM defined

#ifndef  _SPI_HANDLER_H
#define  _SPI_HANDLER_H

#define SPI_DELAY 1 //in us

void SPI_Init(void);
void SPI_ClockOnce(void);
void SPI_WriteByte(unsigned char dataToSend);
unsigned char SPI_ReadByte(void);

#endif
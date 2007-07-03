/**********************************************************************************\
  Name:         SPI_handler.c
  Created by:   Brian Lee						     Jun/14/2007


  Contents:     SPI protocol for T2K-FGD experiment

  Version:		Rev 1.0

  Last updated: 

  $id$
\**********************************************************************************/
//  need to have T2KASUM defined

#ifndef  _SPI_HANDLER_H
#define  _SPI_HANDLER_H

#define SPI_DELAY 1

void SPI_Init(void);
void SPI_ClockOnce(void);
void SPI_WriteByte(unsigned char dataToSend);
unsigned char SPI_ReadByte(void);

#endif
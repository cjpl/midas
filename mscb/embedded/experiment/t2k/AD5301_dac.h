/********************************************************************\

  Name:         AD5301_dac.h
  Created by:   Brian Lee  								Jun/07/2007


  Contents:     AD5301 DAC user interface

  $Id$

\********************************************************************/
//  need to have T2KASUM defined

#ifndef  _AD5301_DAC_H
#define  _AD5301_DAC_H

#define AD5301_READ 1
#define AD5301_WRITE 0
#define ADDR_AD5301 0x0C

//void User_AD5301Dac(void);
void AD5301_Cmd(unsigned char addr, unsigned char datMSB, unsigned char datLSB, bit flag);
void AD5301_Init(void);

#endif
/**********************************************************************************\
  Name:         I2C_handler.h
  Created by:   Brian Lee						     Jul/11/2007, recovered on Aug/24/2007

  Contents:     I2C Protocol header for any MSCB application

  Version:		Rev 1.1
  
  Requirement:  Need to have I2C_SDA and I2C_SCL defined in mscbemb.h

  Last updated: Aug/24/2007
  				- Recovery

  $Id: $
\**********************************************************************************/

#ifndef _I2C_HANDLER_H
#define _I2C_HANDLER_H

//I2C Parameter Definitions
#define I2C_T_LOW 5 //in us
#define I2C_T_HIGH 5 //in us
#define I2C_SU_STA 5 // in us
#define I2C_ACK 0
#define I2C_NACK 1
#define I2C_WRITE_FLAG 3
#define I2C_READ_FLAG 4
#define I2C_DATA_FLAG 5
#define I2C_CMD_FLAG 6
#define I2C_WRITE_ERROR 99
#define I2C_HD_STA 5 // in us
#define I2C_SU_STO 5 // in us
#define I2C_T_BUF 5 // in us, Bus freeTime between Stop and Start condition

//I2C Function Declarations
void I2C_Clear(void);
void I2C_Start(void);
void I2C_Stop(void);
unsigned char I2C_WriteByte(unsigned char datByte, unsigned char flag);
unsigned char I2C_ReadByte(bit flag);
void I2C_ClockOnce(void);

#endif
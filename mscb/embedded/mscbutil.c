/********************************************************************\

  Name:         mscbmain.asm
  Created by:   Stefan Ritt

  Contents:     Various utility functions for MSCB protocol

  $Log$
  Revision 1.3  2002/07/08 08:51:05  midas
  Wrote eeprom functions

  Revision 1.2  2002/07/05 15:27:39  midas
  Initial revision

  Revision 1.1  2002/07/05 08:12:46  midas
  Moved files

\********************************************************************/

#include "mscb.h"
#include <intrins.h>

/*------------------------------------------------------------------*/

unsigned char code crc8_data[] = {
0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
};

unsigned char crc8(unsigned char *buffer, int len)
/********************************************************************\

  Routine: crc8

  Purpose: Calculate 8-bit cyclic redundancy checksum

  Input:
    unsigned char *data     data buffer
    int len                 data length in bytes


  Function value:
    unsighend char          CRC-8 code

\********************************************************************/
{
int i;
unsigned char crc8_code, index;

  crc8_code = 0;
  for (i=0 ; i<len ; i++)
    {
    index = buffer[i] ^ crc8_code;
    crc8_code = crc8_data[index];
    }

  return crc8_code;
}

/*------------------------------------------------------------------*/

void uart_init(unsigned char baud)
/********************************************************************\

  Routine: uart_init

  Purpose: Initial serial interface

  Input:
    unsigned char baud      1:9600,2:19200,3:28800,4:57600,
                            5:115200,6:172800,7:345600 Baud


\********************************************************************/
{
unsigned char code baud_table[] = 
  { 0x100-36, 0x100-18, 0x100-12, 0x100-6, 0x100-3, 0x100-2, 0x100-1 };


  SCON = 0xD0;   // Mode 3, 9 bit, receive enable
// SCON = 0x50;    // Mode 1, 8 bit, receive enable

  TMOD = 0x00;   // timer 0&1 no function
  T2CON = 0x34;  // timer 2 RX+TX mode
  RCAP2H = 0xFF;
  RCAP2L = baud_table[baud-1];
}

/*------------------------------------------------------------------*/

void watchdog_refresh(void)
/********************************************************************\

  Routine: watchdog_refresh

  Purpose: Refresh watchdog, has to be called regularly, otherwise
           the watchdog issues a reset

\********************************************************************/
{
#ifdef CPU_ADUC812
  WDR1 = 1;
  WDR2 = 1;
#endif;

#ifdef CPU_C8051F000
  WDTCN = 0xA5;
#endif
}

/*------------------------------------------------------------------*\

/********************************************************************\

  Routine: delay_ms, delay_us

  Purpose: Delay functions for 11.0520 MHz quartz

\********************************************************************/

#ifdef CPU_ADUC812
#endif

#ifdef CPU_C8051F000

void delay_ms(unsigned int ms)
{
unsigned int i;

  for (i=0 ; i<ms ; i++)
    delay_us(1000);
}

void delay_us(unsigned int us)
{
unsigned char i;
unsigned int  remaining_us;

  if (us <= 250)
    {
    for (i=(unsigned char)us ; i>0 ; i--)
      {
      _nop_();
      }
    }
  else
    {
    remaining_us = us;
    while (remaining_us > 250)
      {
      delay_us(250);
      remaining_us -= 250;
      }
    if (us > 0)
      delay_us(remaining_us);
    }

}
#endif

/*------------------------------------------------------------------*/

void eeprom_read(void idata *dst, unsigned char len, 
                 unsigned char offset)
/********************************************************************\

  Routine: eeprom_read

  Purpose: Read from internal EEPROM

  Input:
    unsigned char idata *dst    Destination in IDATA memory
    unsigned char len           Number of bytes to copy
    unsigend char offset        Offset in EEPROM in bytes

\********************************************************************/
{
#ifdef CPU_ADUC812

EEPROM_READ:
        MOV     EADRL,A         ; set page
        MOV     ECON,#1         ; read page
        MOV     A,R1
        DEC     A
        MOV     @R0,EDATA1
        JZ      EEPROM_RET
        INC     R0
        DEC     A
        MOV     @R0,EDATA2
        JZ      EEPROM_RET
        INC     R0
        DEC     A
        MOV     @R0,EDATA3
        JZ      EEPROM_RET
        INC     R0
        DEC     A
        MOV     @R0,EDATA4
EEPROM_RET:
        RET

#endif

#ifdef CPU_C8051F000

  unsigned char i;
  unsigned char code *p;
  unsigned char idata *d;

  p = 0x8000; // read from 128-byte EEPROM page
  p += offset;
  d = dst;

  for (i=0 ; i<len ; i++)
    d[i] = p[i];

#endif
}

/*------------------------------------------------------------------*/

void eeprom_write(void idata *src, unsigned char len, 
                  unsigned char offset)
/********************************************************************\

  Routine: eeprom_read

  Purpose: Read from internal EEPROM

  Input:
    unsigned char idata *src    Source in IDATA memory
    unsigned char len           Number of bytes to copy
    unsigend char offset        Offset in EEPROM in bytes

\********************************************************************/
{
#ifdef CPU_ADUC812

EEPROM_WRITE:
        MOV     EADRL,A         ; select page
        MOV     ECON,#5         ; erase page
        MOV     EDATA1,@R0
        INC     R0
        MOV     EDATA2,@R0
        INC     R0
        MOV     EDATA3,@R0
        INC     R0
        MOV     EDATA4,@R0
        INC     R0
        MOV     ECON,#2         ; write page
        RET


#endif

#ifdef CPU_C8051F000
  unsigned char xdata *p;
  unsigned char i;
  unsigned char idata *s;

  FLSCL = (FLSCL & 0xF0) | 0x08; // set timer for 11.052 MHz clock
  PSCTL = 0x03;                  // allow write and erase

  p = 0x8000;                    // erase page
  *p = 0;

  PSCTL = 0x01;                  // allow write

  p = 0x8000 + offset;
  s = src;

  for (i=0 ; i<len ; i++)        // write data
    p[i] = s[i];

  PSCTL = 0x00;                  // don't allow write
#endif
}

/*------------------------------------------------------------------*\

/********************************************************************\

  Routine: lcd_setup, lcd_clear, lcd_goto, lcd_puts, putchar

  Purpose: LCD functions for HD44780/KS0066 compatible LCD displays

           Since putchar is used by printf, this function puts its
           output to the LCD as well

\********************************************************************/

#define LCD P2 // LCD display connected to port2

sbit LCD_RS     = LCD ^ 1;
sbit LCD_R_W    = LCD ^ 2;
sbit LCD_E      = LCD ^ 3;

sbit LCD_DB4    = LCD ^ 4;
sbit LCD_DB5    = LCD ^ 5;
sbit LCD_DB6    = LCD ^ 6;
sbit LCD_DB7    = LCD ^ 7;

sbit LCD_CLK    = LCD ^ 7; // pins for 4021 shift register
sbit LCD_P_W    = LCD ^ 6;
sbit LCD_SD     = LCD ^ 0;

bit lcd_present;

/*------------------------------------------------------------------*/

lcd_out(unsigned char d, bit df)
{
  if (!lcd_present)
    return;

#ifdef CPU_C8051F000
  /* switch LCD_DB7 to input */
  // PRT2CF = 0x7F;
#endif
  LCD = 0xFC;       // data input R/W=High
  delay_us(1);
  while (LCD_DB7);  // loop if busy

#ifdef CPU_C8051F000
  /* switch LCD_DB7 to output */
  // PRT2CF = 0xFF;
#endif

  LCD = 0;
  delay_us(1);

  if (df)
    LCD = (d & 0xF0) | 0x02; 
  else
    LCD = (d & 0xF0); 
  delay_us(1);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_us(1);

  if (df)
    LCD = (d << 4) | 0x02; 
  else
    LCD = (d << 4); 
  delay_us(1);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_us(1);
}

/*------------------------------------------------------------------*/

void lcd_setup()
{
  LCD = 0;
  delay_ms(15);
  LCD = 0x30;
  
  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_ms(5);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_ms(1);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;

  LCD = 0x20; // set 4-bit interface
  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;

  /* test if LCD present */
  LCD = 0xFC; // data input R/W=High
  delay_us(10);
  if (!LCD_DB7)
    {
    lcd_present = 0;
    return;
    }

  lcd_present = 1;

  lcd_out(0x2C, 0); // select 2 lines, big font
  lcd_out(0x0C, 0); // display on
  lcd_out(0x01, 0); // clear display
  lcd_out(0x06, 0); // entry mode
}

/*------------------------------------------------------------------*/

void lcd_clear()
{
  lcd_out(0x01, 0);
}

/*------------------------------------------------------------------*/

void lcd_goto(char x, char y)
{
  /* goto position x(0..19), y(0..1) */

  lcd_out((x & 0x0F) | (0x80) | ((y & 0x01) << 6), 0);
}

/*------------------------------------------------------------------*/

char putchar(char c)
{
  lcd_out(c, 1);
  return c;
}

/*------------------------------------------------------------------*/

void lcd_puts(char *str)
{
  while (*str)
    lcd_out(*str++, 1);
}

/********************************************************************\

  Routine: scs_lcd1_read

  Purpose: Returns switch (button) state of SCS-LCD1 module

\********************************************************************/

char scs_lcd1_read()
{
char i, d;

  LCD_CLK = 0;  // enable input
  LCD_SD = 1;

  LCD_P_W = 1;  // latch input
  delay_us(1);
  LCD_P_W = 0;

  for (i=d=0 ; i<8 ; i++)
    {
    d |= LCD_SD;
    d = (d | LCD_SD) << 1;

    LCD_CLK = 1;
    delay_us(1);
    LCD_CLK = 0;
    delay_us(1);
    }

  return d;
}



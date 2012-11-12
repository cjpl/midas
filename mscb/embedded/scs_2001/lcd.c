/********************************************************************\

  Name:         lcd.c
  Created by:   Stefan Ritt

  Contents:     LCD routines

  $Id: mscbutil.c 5022 2011-05-04 14:22:33Z ritt $

\********************************************************************/

#include <intrins.h>
#include <string.h>
#include <stdio.h>
#include "mscbemb.h"
#include "lcd.h"

bit lcd_present;

/********************************************************************\

  Routine: lcd_setup, lcd_clear, lcd_goto, lcd_puts, putchar

  Purpose: LCD functions for HD44780/KS0066 compatible LCD displays

           Since putchar is used by printf, this function puts its
           output to the LCD as well

\********************************************************************/

#define LCD P2                  // LCD display connected to port2

#if defined (SCS_2000)  || defined(SCS_2001) || defined (SCS_3000)
sbit LCD_RS  = P1 ^ 4;
sbit LCD_R_W = P1 ^ 5;
sbit LCD_E   = P1 ^ 6;
sbit LCD_1D  = P1 ^ 0;
sbit LCD_2D  = P1 ^ 1;
#elif defined (SCS_900) || defined(SCS_1000) || defined (SCS_1001)
sbit LCD_RS  = LCD ^ 3;
sbit LCD_R_W = LCD ^ 2;
sbit LCD_E   = LCD ^ 1;
#else
sbit LCD_RS  = LCD ^ 1;
sbit LCD_R_W = LCD ^ 2;
sbit LCD_E   = LCD ^ 3;
#endif

sbit LCD_DB4 = LCD ^ 4;
sbit LCD_DB5 = LCD ^ 5;
sbit LCD_DB6 = LCD ^ 6;
sbit LCD_DB7 = LCD ^ 7;

sbit LCD_CLK = LCD ^ 7;         // pins for 4021 shift register
sbit LCD_P_W = LCD ^ 6;
sbit LCD_SD  = LCD ^ 0;

/*------------------------------------------------------------------*/

void lcd_out(unsigned char d, bit df)
{
   unsigned char timeout;

   if (!lcd_present)
      return;

   LCD = LCD | 0xF0;            // data input
#if defined(CPU_C8051F120)
   SFRPAGE = CONFIG_PAGE;
#if defined(SCS_2000) || defined(SCS_2001)
   P2MDOUT |= 0x0F;
   LCD_1D = 0;                  // b to a for data
#else
   P2MDOUT = 0x0F;
#endif
#else
   PRT2CF = 0x0F;
#endif
   LCD_RS = 0;                  // select BF
   LCD_R_W = 1;
   delay_us(1);
   LCD_E = 1;

   for (timeout = 0 ; timeout < 200 ; timeout++) {
     delay_us(10);              // let signals settle
     if (!LCD_DB7)              // loop if busy
       break;
   }

   LCD_E = 0;
   delay_us(1);
   LCD_R_W = 0;
#if defined(CPU_C8051F120)
#if defined(SCS_2000) || defined(SCS_2001)
   LCD_1D = 1;                  // a to b for data
#endif
   SFRPAGE = CONFIG_PAGE;
   P2MDOUT = 0xFF;              // data output
#else
   PRT2CF = 0xFF;               // data output
#endif
   delay_us(1);

   /* high nibble, preserve P0.0 */
   LCD = (LCD & 0x01) | (d & 0xF0);
   LCD_RS = df;
   delay_us(1);

   LCD_E = 1;
   delay_us(1);
   LCD_E = 0;
   delay_us(1);

   /* now nibble */
   LCD = (LCD & 0x01) | ((d << 4) & 0xF0);
   LCD_RS = df;
   delay_us(1);

   LCD_E = 1;
   delay_us(1);
   LCD_E = 0;
   delay_us(1);
}

/*------------------------------------------------------------------*/

#if defined(SCS_900) || defined(SCS_1001) || defined(SCS_2000) || defined(SCS_2001) 
// 4-line LCD display with KS0078 controller

void lcd_nibble(unsigned char d)
{
   LCD &= ~(0xF0);
   LCD |= (d & 0xF0);        // high nibble
   LCD_E = 1;
   delay_us(1);
   LCD_E = 0;
   delay_us(1);

   LCD &= ~(0xF0);
   LCD |= ((d << 4) & 0xF0); // low nibble
   LCD_E = 1;
   delay_us(1);
   LCD_E = 0;
   delay_us(1);

   delay_us(100);
}

#endif

void lcd_setup()
{
   unsigned idata i=0;

   LCD_E   = 0;
   LCD_R_W = 0;
   LCD_RS  = 0;

#if defined(CPU_C8051F120)
   SFRPAGE = CONFIG_PAGE;
   P2MDOUT = 0xFF;             // all push-pull
#if defined(SCS_2000) || defined(SCS_2001)
   P1MDOUT |= 0x03;
   LCD_1D = 1;                 // a to b for data
   LCD_2D = 1;                 // a to b for control
#endif
#else
   PRT2CF = 0xFF;              // all push-pull
#endif

#if defined(SCS_900) || defined(SCS_1001) || defined(SCS_2000)  || defined(SCS_2001)
// 4-line LCD display with KS0078 controller

   LCD &= ~(0xFE);
   LCD |= 0x20;  // set 4-bit interface
   LCD_E = 1;
   delay_us(1);
   LCD_E = 0;
   delay_ms(1);
   lcd_nibble(0x20); // function set: 4-bit, RE=0

   // test if LCD present

   LCD = LCD | 0xF0;            // data input
#if defined(CPU_C8051F120)
   SFRPAGE = CONFIG_PAGE;
#if defined(SCS_2000) || defined(SCS_2001)
   P2MDOUT |= 0x0F;
   LCD_1D = 0;                  // b to a for data
#else
   P2MDOUT = 0x0F;
#endif
#else
   PRT2CF = 0x0F;
#endif
   LCD_RS = 0;                  // select BF
   LCD_R_W = 1;
   delay_us(1);
   LCD_E = 1;
   delay_us(100);               // let signal settle
   if (LCD_DB7) {
      lcd_present = 0;
      return;
   }

   lcd_present = 1;

   lcd_out(0x24, 0); // function set: 4-bit, RE=1
   lcd_out(0x0B, 0); // ext function set: 4-line display, inverting cursor
   lcd_out(0x20, 0); // function set: 4-bit, RE=0
   lcd_out(0x0C, 0); // display on
   lcd_out(0x01, 0); // clear display
   lcd_out(0x06, 0); // entry mode: incrementing

#else // 2-line LCD display with KS0066 controller

   LCD &= ~(0xFE);
   delay_ms(15);
   LCD |= 0x30;

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

   LCD = 0x20;                  // set 4-bit interface
   LCD_E = 1;
   delay_us(1);
   LCD_E = 0;

   // test if LCD present

   LCD = LCD | 0xF0;            // data input
#if defined(CPU_C8051F120)
   SFRPAGE = CONFIG_PAGE;
   P2MDOUT = 0x0F;
#else
   PRT2CF = 0x0F;
#endif
   LCD_RS = 0;                  // select BF
   LCD_R_W = 1;
   delay_us(1);
   LCD_E = 1;
   delay_us(100);               // let signal settle
   if (LCD_DB7) {
      lcd_present = 0;
      return;
   }

   lcd_present = 1;

   lcd_out(0x2C, 0);            // select 2 lines, big font
   lcd_out(0x0C, 0);            // display on
   lcd_out(0x01, 0);            // clear display
   lcd_out(0x06, 0);            // entry mode
#endif
}

/*------------------------------------------------------------------*/

void lcd_clear()
{
   lcd_out(0x01, 0);
   lcd_goto(0, 0);
}

void lcd_cursor(unsigned char flag)
{
   if (flag)
      lcd_out(0x0F, 0); // display on, curson on, blink on
   else
      lcd_out(0x0C, 0); // display on, cursor off, blink off
}

/*------------------------------------------------------------------*/

void lcd_goto(char x, char y)
{

#if defined(SCS_900) || defined(SCS_1001) || defined(SCS_2000) || defined(SCS_2001)
   /* goto position x(0..19), y(0..3) */
   lcd_out((x & 0x1F) | (0x80) | ((y & 0x03) << 5), 0);
#else
   /* goto position x(0..19), y(0..1) */
   lcd_out((x & 0x1F) | (0x80) | ((y & 0x01) << 6), 0);
#endif
}

/*------------------------------------------------------------------*/

char putchar(char c)
{
   if (c != '\r' && c != '\n')
      lcd_out(c, 1);
   return c;
}

/*------------------------------------------------------------------*/

void lcd_puts(char *str)
{            
   while (*str)
      lcd_out(*str++, 1);
}

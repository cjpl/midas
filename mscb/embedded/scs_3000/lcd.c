/********************************************************************\

  Routine: lcd_setup, lcd_clear, lcd_goto, lcd_puts, putchar

  Purpose: LCD functions for EA eDIPTFT43-A LCD display

           Since putchar is used by printf, this function puts its
           output to the LCD as well

\********************************************************************/

#include <intrins.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "mscbemb.h"
#include "lcd.h"

sbit LCD_SS   = P2 ^ 4;
sbit LCD_MOSI = P2 ^ 3;
sbit LCD_MISO = P2 ^ 2;
sbit LCD_CLK  = P2 ^ 1;
sbit LCD_SBUF = P2 ^ 0;

bit lcd_present = 1;

#define ACK 0x06
#define NAK 0x15
#define DC1 0x11
#define ESC 0x1B

unsigned char xdata _buf[16];

/*------------------------------------------------------------------*/

unsigned char lcd_io(unsigned char out)
{
   unsigned char idata i, in;

   LCD_SS  = 0;

   in = 0;
   for (i=0 ; i<8 ; i++) {
      LCD_CLK = 0;
      LCD_MOSI = (out & 1);
      out >>= 1;
      delay_us(3);

      in >>= 1;
      if (LCD_MISO)
      in |= 0x80;
      LCD_CLK = 1;
      delay_us(3);
   }

   LCD_SS = 1;
   return in;
}

/*------------------------------------------------------------------*/

void lcd_sendbuf(unsigned char *buf, unsigned char len)
{
   unsigned char idata i, cs;

   lcd_io(DC1);
   cs = DC1;

   lcd_io(len+1); // add 1 for ESC
   cs += len+1;

   lcd_io(ESC);
   cs += ESC;

   for (i=0 ; i<len ; i++) {
      lcd_io(buf[i]);
      cs += buf[i];
   }

   lcd_io(cs);
}

/*------------------------------------------------------------------*/

void lcd_send(unsigned char *buf, unsigned char len)
{
   unsigned char idata status, i;

   for (i=0 ; i<10 ; i++) {
      lcd_sendbuf(buf, len);
      delay_us(6);
      status = lcd_io(0xFF);
      if (status == ACK)
         break;
      watchdog_refresh(0);
      delay_us(10);
   }
}

/*------------------------------------------------------------------*/

void lcd_setup()
{
   /* open drain(*) / push-pull: 
      P2.0*LCD_SBUF
      P2.1 LCD_CLK 
      P2.2*LCD_MISO
      P2.3 LCD_MOSI
                   
      P2.4 LCD_SS  
      P2.5         
      P2.6 LED0    
      P2.7 LED1    
    */
   SFRPAGE = CONFIG_PAGE;
   P2MDOUT = 0xFA;

   // stop all macros
   lcd_send("MS", 3); 
   lcd_clear();

   // change display orientation
   lcd_send("DO\01", 3);

   // change default font and color
   lcd_font(3);
   lcd_fontcolor(WHITE, BLACK);
   lcd_fontzoom(2, 2);
}

/*------------------------------------------------------------------*/

void lcd_clear()
{
   lcd_send("DL", 2);
}

/*------------------------------------------------------------------*/

void lcd_line(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
   _buf[0]  = 'G';
   _buf[1]  = 'D';

   _buf[2]  = x1 & 0xFF;
   _buf[3]  = x1 >> 8;
   _buf[4]  = y1 & 0xFF;
   _buf[5]  = y1 >> 8;

   _buf[6] = x2 & 0xFF;
   _buf[7] = x2 >> 8;
   _buf[8] = y2 & 0xFF;
   _buf[9] = y2 >> 8;

   lcd_send(_buf, 10);
}

/*------------------------------------------------------------------*/

unsigned short xdata _x = 0; 
unsigned short xdata _y = 0; 
unsigned char xdata _sx = 7;
unsigned char xdata _sy = 12;
unsigned char xdata _zx = 1;
unsigned char xdata _zy = 1;

//void lcd_goto(unsigned short x, unsigned short y)
void lcd_goto(unsigned short x, unsigned short y)
{
   _x = x * _sx * _zx;
   _y = y * _sy * _zy;
}

/*------------------------------------------------------------------*/

void lcd_font(unsigned char f)
{
   if (f == 1) {
      _sx = 4;
      _sy = 6;
   } else if (f == 2) {
      _sx = 6;
      _sy = 8;
   } else if (f == 3) {
      _sx = 7;
      _sy = 12;
   } else if (f == 4) {
      _sx = 5;
      _sy = 10;
   } else if (f == 5) {
      _sx = 7;
      _sy = 14;
   } else if (f == 6) {
      _sx = 15;
      _sy = 30;
   }

   _buf[03] = 'Z';
   _buf[1] = 'F';
   _buf[2] = f;
   lcd_send(_buf, 3);
}

/*------------------------------------------------------------------*/

void lcd_fontcolor(unsigned char fg, unsigned char bg)
{
   _buf[0] = 'F';
   _buf[1] = 'Z';
   _buf[2] = fg;
   _buf[3] = bg;
   lcd_send(_buf, 4);
}

/*------------------------------------------------------------------*/

void lcd_fontzoom(unsigned char zx, unsigned char zy)
{
   _zx = zx;
   _zy = zy;

   _buf[0] = 'Z';
   _buf[1] = 'Z';
   _buf[2] = zx;
   _buf[3] = zy;
   lcd_send(_buf, 4);
}

/*------------------------------------------------------------------*/

char putchar(char c)
{
   _buf[0] = 'Z';
   _buf[1] = 'L';

   _buf[2]  = _x & 0xFF;
   _buf[3]  = _x >> 8;
   _buf[4]  = _y & 0xFF;
   _buf[5]  = _y >> 8;

   _buf[6]  = c;
   _buf[7] = 0;

   lcd_send(_buf, 8);
   _x += _sx * _zx;

   return c;
}

/*------------------------------------------------------------------*/

void lcd_puts(char *str)
{            
   while (*str)
      putchar(*str++);
}

/*------------------------------------------------------------------*/

void lcd_text(unsigned short x, unsigned short y, char align, char *str)
{
   unsigned char status, i, j, cs, len;

   len = strlen(str);

   for (j=0 ; j<10 ; j++) {

      lcd_io(DC1);
      cs = DC1;

      lcd_io(len+8);
      cs += len+8;

      lcd_io(ESC);
      cs += ESC;

      lcd_io('Z');
      cs += 'Z';

      lcd_io(align);
      cs += align;

      lcd_io(x & 0xFF);
      cs += (x & 0xFF);

      lcd_io(x >> 8);
      cs += (x >> 8);

      lcd_io(y & 0xFF);
      cs += (y & 0xFF);

      lcd_io(y >> 8);
      cs += (y >> 8);

      for (i=0 ; i<len+1 ; i++) {
         lcd_io(str[i]);
         cs += str[i];
      }

      lcd_io(cs);

      delay_us(6);
      status = lcd_io(0xFF);
      if (status == ACK)
         break;
      watchdog_refresh(0);
      delay_us(10);
   }
}

/*------------------------------------------------------------------*/

void lcd_menu()
{            
}




#if 0

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

#endif // if 0
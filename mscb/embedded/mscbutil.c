/********************************************************************\

  Name:         mscbmain.asm
  Created by:   Stefan Ritt

  Contents:     Various utility functions for MSCB protocol

  $Log$
  Revision 1.21  2003/03/19 16:35:03  midas
  Eliminated configuration parameters

  Revision 1.20  2003/03/14 13:47:54  midas
  Added SCS_310 code

  Revision 1.19  2003/03/06 11:01:13  midas
  Priority inversion for slow ADuC

  Revision 1.18  2003/02/20 13:11:58  midas
  Re-init clock value on sysclock_init()

  Revision 1.17  2003/02/19 16:04:56  midas
  Added code for ADuC812

  Revision 1.16  2003/01/30 08:40:02  midas
  Use USE_WATCHDOG flag

  Revision 1.15  2002/11/20 12:02:39  midas
  Fixed bug with secondary LED

  Revision 1.14  2002/11/06 16:45:42  midas
  Revised LED blinking scheme

  Revision 1.13  2002/10/16 15:28:25  midas
  Fixed typo

  Revision 1.12  2002/10/09 15:48:13  midas
  Fixed bug with download

  Revision 1.11  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.10  2002/10/07 15:16:32  midas
  Added upgrade facility

  Revision 1.9  2002/10/04 09:03:20  midas
  Small mods for scs_300

  Revision 1.8  2002/10/03 15:31:53  midas
  Various modifications

  Revision 1.7  2002/08/08 06:46:15  midas
  Added time functions

  Revision 1.6  2002/07/12 08:38:13  midas
  Fixed LCD recognition

  Revision 1.5  2002/07/10 09:52:55  midas
  Finished EEPROM routines

  Revision 1.4  2002/07/09 15:31:32  midas
  Initial Revision

  Revision 1.3  2002/07/08 08:51:05  midas
  Wrote eeprom functions

  Revision 1.2  2002/07/05 15:27:39  midas
  Initial revision

  Revision 1.1  2002/07/05 08:12:46  midas
  Moved files

\********************************************************************/

#include "mscb.h"
#include <intrins.h>

extern SYS_INFO sys_info;               // for eeprom functions
extern MSCB_INFO_VAR code variables[];
extern bit adr_led_on, adr_led_off;     // used for addressed LED flashing

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

#pragma NOAREGS  // all functions can be called from interrupt routine!

unsigned char crc8(unsigned char *buffer, int len)
/********************************************************************\

  Routine: crc8

  Purpose: Calculate 8-bit cyclic redundancy checksum for a full
           buffer

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

unsigned char crc8_add(unsigned char crc, unsigned int c)
/********************************************************************\

  Routine: crc8_add

  Purpose: Single calculation for 8-bit cyclic redundancy checksum

  Input:
    unsigned char crc       running crc
    unsigned char c         new character


  Function value:
    unsighend char          CRC-8 code

\********************************************************************/
{
unsigned char index;

  index = c ^ crc;
  return crc8_data[index];
}

/*------------------------------------------------------------------*/

#ifdef SCS_210 // SCS_210 uses UAR0 & UART1

bit  ti1_shadow;

char xdata rbuf[1024];
char xdata sbuf[1024];

unsigned char xdata * data rbuf_rp;
unsigned char xdata * data rbuf_wp;
unsigned char xdata * data sbuf_rp;
unsigned char xdata * data sbuf_wp;

void uart_init(unsigned char port, unsigned char baud)
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

  if (port == 0)
    {
    SCON0 = 0xD0;   // Mode 3, 9 bit, receive enable
  // SCON0 = 0x50;    // Mode 1, 8 bit, receive enable
  
    T2CON = 0x34;  // timer 2 RX+TX mode
    RCAP2H = 0xFF;
    RCAP2L = baud_table[baud-1];
  
    IE |= 0x10;    // enable serial interrupt
    IP &= ~0x10;   // serial interrupt low priority
    EA = 1;        // general interrupt enable
    }
  else
    {
    SCON1 = 0x50;  // Mode 1, 8 bit, receive enable
  
    T4CON = 0x34;  // timer 4 RX+TX mode
    RCAP4H = 0xFF;
    RCAP4L = baud_table[baud-1];
  
    EIE2 |= 0x40;  // enable serial interrupt
    EIP2 &= ~0x40; // serial interrupt low priority
    EA = 1;        // general interrupt enable

    rbuf_rp = rbuf_wp = rbuf;
    sbuf_rp = sbuf_wp = sbuf;

    ti1_shadow = 1;
    }
}

/*---- UART1 handling ----------------------------------------------*/

void serial_int1(void) interrupt 20 using 2
{
  if (SCON1 & 0x02) // TI1
    {
    /* character has been transferred */
    
    SCON1 &= ~0x02;       // clear TI flag
    ti1_shadow = 1;
    }

  if (SCON1 & 0x01) // RI1
    {
    /* check for buffer overflow */
    if (rbuf_wp + 1 == rbuf_rp)
      {
      SCON1 &= ~0x01;       // clear RI flag
      return;
      }

    /* character has been received */
    *rbuf_wp++ = SBUF1;
    if (rbuf_wp == rbuf+sizeof(rbuf))
      rbuf_wp = rbuf;

    SCON1 &= ~0x01;       // clear RI flag

    led_blink(2, 1, 100);
    }
}

/*------------------------------------------------------------------*/

void rs232_output(void)
/* check RS232 output buffer to send data */
{
  if (sbuf_wp != sbuf_rp && ti1_shadow == 1)
    {
    ti1_shadow = 0;
    SBUF1 = *sbuf_rp++;
    if (sbuf_rp == sbuf+sizeof(sbuf))
      sbuf_rp = sbuf;
    }
}

/*------------------------------------------------------------------*/

char getchar(void)
{
char c;

  do
    {
    if (rbuf_wp != rbuf_rp)
      {
      c = *rbuf_rp++;
      if (rbuf_rp == rbuf+sizeof(rbuf))
        rbuf_rp = rbuf;

      if (c == '\r') /* make gets() happy */
        return '\n';

      return c;
      }
    yield();
    } while (1);
}

/*------------------------------------------------------------------*/

char getchar_nowait(void)
{
char c;

  if (rbuf_wp != rbuf_rp)
    {
    c = *rbuf_rp++;
    if (rbuf_rp == rbuf+sizeof(rbuf))
      rbuf_rp = rbuf;

    return c;
    }
  return -1;
}

/*------------------------------------------------------------------*/

unsigned char gets_wait(char *str, unsigned char size, unsigned char timeout)
{
unsigned long start;
unsigned char i;
char          c;

  start = time();
  i = 0;
  do
    {
    c = getchar_nowait();
    if (c != -1 && c != '\n')
      {
      if (c == '\r')
        {
        str[i] = 0;
        return i;
        }
      str[i++] = c;
      if (i == size)
        return i;
      }

    yield();
    } while (time() < start+timeout);

  return 0;
}

/*------------------------------------------------------------------*/

char putchar(char c)
{
  /* check if buffer overflow */
  if (sbuf_wp + 1 == sbuf_rp)
    return c;

  *sbuf_wp++ = c;
  if (sbuf_wp == sbuf+sizeof(sbuf))
    sbuf_wp = sbuf;

  return c;
}

/*------------------------------------------------------------------*/

void flush(void)
{
  while (sbuf_wp != sbuf_rp)
    rs232_output();
}

#else // non-SCS_210

/*------------------------------------------------------------------*/

void uart_init(unsigned char port, unsigned char baud)
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

  if (port);

  SCON0 = 0xD0;   // Mode 3, 9 bit, receive enable
// SCON = 0x50;    // Mode 1, 8 bit, receive enable

  T2CON = 0x34;   // timer 2 RX+TX mode
  RCAP2H = 0xFF;
  RCAP2L = baud_table[baud-1];

  ES0 = 1;        // enable serial interrupt

#ifdef CPU_ADUC812
  PS = 1;         // serial interrupt high priority for slow ADuC
#else
  PS = 0;         // serial interrupt low priority
#endif

  EA = 1;         // general interrupt enable
  RB80 = 0;       // clear read bit 9
}

/*------------------------------------------------------------------*/

void rs232_output(void)
{
}

#endif

/*------------------------------------------------------------------*/

static unsigned long _systime;

static unsigned char led_pri_n;
static unsigned char led_pri_int;
static unsigned char led_pri_timer;

#ifdef LED_2
static unsigned char led_sec_n;
static unsigned char led_sec_int;
static unsigned char led_sec_timer;
#endif

/*------------------------------------------------------------------*/

void sysclock_init(void)
/********************************************************************\

  Routine: sysclock_init

  Purpose: Initial sytem clock via timer 1



\********************************************************************/
{

  EA = 1;             // general interrupt enable
  IE |= 0x08;         // Enable Timer1 interrupt
#ifdef CPU_ADUC812
  PT1 = 0;            // Interrupt priority low for slow ADuC
#else         
  PT1 = 1;            // Interrupt priority high
#endif

  TMOD = TMOD | 0x10; // 16-bit counter
#ifdef CPU_CYGNAL
  CKCON = 0x00;       // user SYSCLK/12
#endif
  TH1 = 0xDB;         // load initial values
  TL1 = 0x00; 
  TR1 = 1;            // start timer 1

  _systime = 0;

  led_pri_n     = 0;
  led_pri_int   = 0;
  led_pri_timer = 0;
  
#ifdef LED_2
  led_sec_n     = 0;
  led_sec_int   = 0;
  led_sec_timer = 0;
#endif
}

/*------------------------------------------------------------------*/

void led_blink(int led, int n, int interval) reentrant
/********************************************************************\

  Routine: blink led

  Purpose: Blink primary or secondary LED for a couple of times

  Input:
    int led               1 for primary, 2 for secondary
    int interval          Blink interval in ms
    int n                 Number of blinks

\********************************************************************/
{
  if (led == 1)
    {
    if (led_pri_n == 0 && led_pri_timer == 0)
      {
      led_pri_n = n*2;
      led_pri_int = interval/10;
      led_pri_timer = 0;
      }
    }

#ifdef LED_2
  if (led == 2)
    {
    if (led_sec_n == 0 && led_sec_timer == 0)
      {
      led_sec_n = n*2;
      led_sec_int = interval/10;
      led_sec_timer = 0;
      }
    }
#endif
}

/*------------------------------------------------------------------*/

void timer1_int(void) interrupt 3 using 2
/********************************************************************\

  Routine: timer1_int

  Purpose: Timer 1 interrupt routine for 100Hz system clock

           Reload value = 0x10000 - 0.01 / (11059200/12)

\********************************************************************/
{
  TH1 = 0xDC;         // reload timer values, let LSB freely run
  _systime++;         // increment system time

  /* manage address LED blinking */

  if (led_pri_n)
    {
    if (led_pri_timer == 0)
      {
      LED = !LED;
      led_pri_timer = led_pri_int;
      led_pri_n--;
      if (led_pri_n == 0)
        LED = LED_OFF;
      }
    }

  if (led_pri_timer)
    led_pri_timer--;

#ifdef LED_2
  if (led_sec_n)
    {
    if (led_sec_timer == 0)
      {
      LED_SEC = !LED_SEC;
      led_sec_timer = led_sec_int;
      led_sec_n--;
      if (led_sec_n == 0)
        LED_SEC = LED_OFF;
      }
    }

  if (led_sec_timer)
    led_sec_timer--;
#endif
}

/*------------------------------------------------------------------*/

unsigned long time(void)
/********************************************************************\

  Routine: time

  Purpose: Return system time in units of 10ms 

\********************************************************************/
{
unsigned long t;

  DISABLE_INTERRUPTS;
  t = _systime;
  ENABLE_INTERRUPTS;
  return t;
}

/*------------------------------------------------------------------*/

void watchdog_refresh(void)
/********************************************************************\

  Routine: watchdog_refresh

  Purpose: Refresh watchdog, has to be called regularly, otherwise
           the watchdog issues a reset

\********************************************************************/
{
#ifdef USE_WATCHDOG

#ifdef CPU_ADUC812
  WDR1 = 1;
  WDR2 = 1;
#endif

#ifdef CPU_CYGNAL
  WDTCN = 0xA5;
#endif

#endif
}

/*------------------------------------------------------------------*\

/********************************************************************\

  Routine: delay_ms, delay_us

  Purpose: Delay functions for 11.0520 MHz quartz

\********************************************************************/

void delay_ms(unsigned int ms)
{
unsigned int i;

  for (i=0 ; i<ms ; i++)
    {
    delay_us(1000);
    yield();
    }
}

#ifdef CPU_ADUC812

void delay_us(unsigned int us)
{
unsigned char i;
unsigned int  remaining_us;

  if (us <= 250)
    {
    us /= 12;
    for (i=(unsigned char)us ; i>0 ; i--);
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

#ifdef CPU_CYGNAL

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
                 unsigned char *offset)
/********************************************************************\

  Routine: eeprom_read

  Purpose: Read from internal EEPROM

  Input:
    unsigned char idata *dst    Destination in IDATA memory
    unsigned char len           Number of bytes to copy
    unsigend char *offset       Offset in EEPROM in bytes, gets
                                adjusted after read

\********************************************************************/
{
#ifdef CPU_ADUC812

  unsigned char i, ofs;
  unsigned char idata *d;

  d = dst;

  ofs = *offset;
  EADRL = ofs / 4;
  ECON = 0x01; // read page

  for (i=0 ; i<len ; i++,d++)
    {
    switch (ofs % 4)
      {
      case 0: *d = EDATA1; break;
      case 1: *d = EDATA2; break;
      case 2: *d = EDATA3; break;
      case 3: *d = EDATA4; break;
      }

    ofs++;

    if (ofs % 4 == 0)
      {
      EADRL = ofs / 4;
      ECON = 0x01;  // read next page
      }
    }

  *offset = ofs;

#endif

#ifdef CPU_CYGNAL

  unsigned char i;
  unsigned char code *p;
  unsigned char idata *d;

  p = 0x8000 + *offset; // read from 128-byte EEPROM page
  d = dst;

  for (i=0 ; i<len ; i++)
    d[i] = p[i];

  *offset += len;
#endif
}

/*------------------------------------------------------------------*/

void eeprom_write(void idata *src, unsigned char len, 
                  unsigned char *offset)
/********************************************************************\

  Routine: eeprom_write

  Purpose: Read from internal EEPROM

  Input:
    unsigned char idata *src    Source in IDATA memory
    unsigned char len           Number of bytes to copy
    unsigend char offset        Offset in EEPROM in bytes, gets
                                adjusted after write

\********************************************************************/
{
#ifdef CPU_ADUC812

  unsigned char i, ofs;
  unsigned char idata *d;

  d = src;

  ofs = *offset;
  EADRL = ofs / 4;
  ECON = 0x01; // read page

  for (i=0 ; i<len ; i++,d++)
    {
    switch (ofs % 4)
      {
      case 0: EDATA1 = *d; break;
      case 1: EDATA2 = *d; break;
      case 2: EDATA3 = *d; break;
      case 3: EDATA4 = *d; 
              ECON = 0x02;  // write page
              break;
      }

    ofs++;

    if (ofs % 4 == 0)
      {
      EADRL = ofs / 4;
      ECON = 0x01;  // read next page
      }
    }

  if (ofs % 4 != 0)
    ECON = 0x02; // write last page

  *offset = ofs;

#endif

#ifdef CPU_CYGNAL
  unsigned char xdata *p;
  unsigned char i;
  unsigned char idata *s;

#if defined(CPU_C8051F000)
  FLSCL = (FLSCL & 0xF0) | 0x08; // set timer for 11.052 MHz clock
#elif defined (CPU_C8051F020)
  FLSCL = FLSCL  | 1;            // enable flash writes
#endif
  PSCTL = 0x01;                  // allow write

  p = 0x8000 + *offset;
  s = src;

  for (i=0 ; i<len ; i++)        // write data
    *p++ = *s++;

  PSCTL = 0x00;                  // don't allow write
  FLSCL = FLSCL & 0xF0;

  *offset += len;

  yield();
#endif
}

/*------------------------------------------------------------------*/

void eeprom_erase(void)
/********************************************************************\

  Routine: eeprom_erase

  Purpose: Erase parameter EEPROM page

\********************************************************************/
{
#ifdef CPU_ADUC812
  ECON = 0x06;
#endif

#ifdef CPU_CYGNAL
  unsigned char xdata *p;

#if defined(CPU_C8051F000)
  FLSCL = (FLSCL & 0xF0) | 0x08; // set timer for 11.052 MHz clock
#elif defined (CPU_C8051F020)
  FLSCL = FLSCL  | 1;            // enable flash writes
#endif
  PSCTL = 0x03;                  // allow write and erase

  p = 0x8000;                    // erase page
  *p = 0;

  PSCTL = 0x00;                  // don't allow write
  FLSCL = FLSCL & 0xF0;
#endif
}

/*------------------------------------------------------------------*/

void eeprom_flash(void)
/********************************************************************\

  Routine: eeprom_flash

  Purpose: Write system and user parameters to EEPROM

\********************************************************************/
{
unsigned char offset, i;

  eeprom_erase();
  yield();

  offset = 0;

  // system info (node address etc...)
  eeprom_write(&sys_info, sizeof(SYS_INFO), &offset);

  // user channel variables
  for (i=0 ; variables[i].width ; i++)
    eeprom_write(variables[i].ud, variables[i].width, &offset);
}

/*------------------------------------------------------------------*/

void eeprom_retrieve(void)
/********************************************************************\

  Routine: eeprom_retrieve

  Purpose: Retrieve system and user parameters from EEPROM

\********************************************************************/
{
unsigned char offset, i;

  offset = 0;

  // system info (node address etc...)
  eeprom_read(&sys_info, sizeof(SYS_INFO), &offset);

  // user channel variables
  for (i=0 ; variables[i].width ; i++)
    eeprom_read(variables[i].ud, variables[i].width, &offset);
}

/*------------------------------------------------------------------*/

bit lcd_present;

#if !defined(CPU_ADUC812) && !defined(SCS_210) && !defined(SCS_310) // save code space

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

/*------------------------------------------------------------------*/

lcd_out(unsigned char d, bit df)
{
  if (!lcd_present)
    return;

  LCD = 0xFC;       // data input R/W=High, P2 must be on open drain
  delay_us(1);
  while (LCD_DB7);  // loop if busy

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
  delay_us(100);
  if (LCD_DB7)
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
  lcd_goto(0, 0);
}

/*------------------------------------------------------------------*/

void lcd_goto(char x, char y)
{
  /* goto position x(0..19), y(0..1) */

  lcd_out((x & 0x0F) | (0x80) | ((y & 0x01) << 6), 0);
}

/*------------------------------------------------------------------*/

#if !defined(SCS_210) && !defined(SCS_300) && !defined(SCS_310)

char putchar(char c)
{
  lcd_out(c, 1);
  return c;
}

#endif

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

/*
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
*/

#endif /* CPU_ADUC812 */


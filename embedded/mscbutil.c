/********************************************************************\

  Name:         mscbutil.c
  Created by:   Stefan Ritt

  Contents:     Various utility functions for MSCB protocol

  $Id$

\********************************************************************/

#include "mscb.h"
#include <intrins.h>
#include <string.h>

#ifdef EEPROM_SUPPORT

extern SYS_INFO sys_info;               // for eeprom functions
extern MSCB_INFO_VAR code variables[];

#endif

extern unsigned char idata _n_sub_addr, _var_size, _flkey;

#pragma NOAREGS                 // all functions can be called from interrupt routine!

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

unsigned char crc8(unsigned char *buffer, int len) reentrant
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
   for (i = 0; i < len; i++) {
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

//####################################################################
#if defined(UART1_DEVICE) // user-level device communication via UART1

bit ti1_shadow = 1;

char xdata rbuf[2048];
char xdata sbuf[1024];

unsigned char xdata * xdata rbuf_rp = rbuf;
unsigned char xdata * xdata rbuf_wp = rbuf;
unsigned char xdata * xdata sbuf_rp = sbuf;
unsigned char xdata * xdata sbuf_wp = sbuf;

/*---- UART1 handling ----------------------------------------------*/

void serial_int1(void) interrupt 20
{
   if (SCON1 & 0x02) {          // TI1

#if defined(SCS_220) || defined(SCS_1000) || defined(SCS_1001)
      if (sbuf_wp == sbuf_rp)
         RS485_SEC_ENABLE = 0;
#endif

      /* character has been transferred */
      SCON1 &= ~0x02;           // clear TI flag
      ti1_shadow = 1;
   }

   if (SCON1 & 0x01) {          // RI1
      /* check for buffer overflow */
      if (rbuf_wp + 1 == rbuf_rp) {
         SCON1 &= ~0x01;        // clear RI flag
         return;
      }

      /* character has been received */
      *rbuf_wp++ = SBUF1;
      if (rbuf_wp == rbuf + sizeof(rbuf))
         rbuf_wp = rbuf;

      SCON1 &= ~0x01;           // clear RI flag

      led_blink(1, 1, 100);
   }
}

/*------------------------------------------------------------------*/

void rs232_output(void)
/* check RS232 output buffer to send data */
{
   if (sbuf_wp != sbuf_rp && ti1_shadow == 1) {
      ti1_shadow = 0;
#if defined(SCS_220) || defined(SCS_1000) || defined(SCS_1001)
      RS485_SEC_ENABLE = 1;
#endif

#ifdef CPU_C8051F120
      SFRPAGE = UART1_PAGE;
#endif

      SBUF1 = *sbuf_rp++;
      if (sbuf_rp == sbuf + sizeof(sbuf))
         sbuf_rp = sbuf;
   }
}

/*------------------------------------------------------------------*/

char getchar(void)
{
   char c;

   do {
      if (rbuf_wp != rbuf_rp) {
         c = *rbuf_rp++;
         if (rbuf_rp == rbuf + sizeof(rbuf))
            rbuf_rp = rbuf;

         if (c == '\r')         /* make gets() happy */
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

   if (rbuf_wp != rbuf_rp) {
      c = *rbuf_rp++;
      if (rbuf_rp == rbuf + sizeof(rbuf))
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
   char c;

   start = time();
   i = 0;
   do {
      c = getchar_nowait();
      if (c != -1 && c != '\n') {
         if (c == '\r') {
            str[i] = 0;
            return i;
         }
         str[i++] = c;
         if (i == size)
            return i;
      }

      yield();
   } while (time() < start + timeout);

   return 0;
}

/*------------------------------------------------------------------*/

#ifdef LCD_SUPPORT // putchar is already used for LCD

char putchar1(char c)
{
   /* check if buffer overflow */
   if (sbuf_wp + 1 == sbuf_rp)
      return c;

   *sbuf_wp++ = c;
   if (sbuf_wp == sbuf + sizeof(sbuf))
      sbuf_wp = sbuf;

   return c;
}

#else // LCD_SUPPORT

char putchar(char c)
{
   /* check if buffer overflow */
   if (sbuf_wp + 1 == sbuf_rp)
      return c;

   *sbuf_wp++ = c;
   if (sbuf_wp == sbuf + sizeof(sbuf))
      sbuf_wp = sbuf;

   return c;
}

#endif // LCD_SUPPORT

/*------------------------------------------------------------------*/

void flush(void)
{
   while (sbuf_wp != sbuf_rp)
      rs232_output();
}

/*------------------------------------------------------------------*/

void uart1_init_buffer()
{
   rbuf_rp = rbuf_wp = rbuf;
   sbuf_rp = sbuf_wp = sbuf;

   ti1_shadow = 1;
}

/*------------------------------------------------------------------*/

#endif // UART1_DEVICE ###############################################

#if defined(UART1_MSCB) // UART1 connected as master to MSCB slave bus

bit ti1_shadow = 1;
unsigned char n_recv;

char xdata rbuf[64];

/*---- UART1 handling ----------------------------------------------*/

void serial_int1(void) interrupt 20 
{
   if (SCON1 & 0x02) {          // TI1
      /* character has been transferred */
      SCON1 &= ~0x02;           // clear TI flag
      ti1_shadow = 1;
   }

   if (SCON1 & 0x01) {          // RI1
      /* check for buffer overflow */
      if (n_recv + 1 == sizeof(rbuf)) {
         SCON1 &= ~0x01;        // clear RI flag
         return;
      }

      /* character has been received */
      rbuf[n_recv++] = SBUF1;
      SCON1 &= ~0x01;           // clear RI flag
   }
}

/*------------------------------------------------------------------*/

unsigned char uart1_send(char *buffer, int size, unsigned char bit9)
{
unsigned char i;

   /* empty receive buffer */
   n_recv = 0;

   SFRPAGE = UART1_PAGE;

   RS485_SEC_ENABLE = 1;
   TB81 = bit9;

   for (i=0 ; i<size ; i++) {

      ti1_shadow = 0;

      DELAY_US(INTERCHAR_DELAY);

      if (i == size-1) {
        
         /* interrupt between last send and enable=0 could cause bus collision */
         ES0 = 0;

         SBUF1 = *buffer++;
         while (ti1_shadow == 0);
         
         RS485_SEC_ENABLE = 0;

         ES0 = 1;

      } else {

         SBUF1 = *buffer++;
         while (ti1_shadow == 0);

      }

      watchdog_refresh();
   }
   RS485_SEC_ENABLE = 0;

   return size;
}

/*------------------------------------------------------------------*/

unsigned char uart1_receive(char *buffer, int size)
{
unsigned char len;
long start_time;

   start_time = time();
   len = 0;
   do {

      if (time() - start_time > 1) // timeout after 20ms
         return 0;

      if (n_recv > 0) {

         /* response to ping */
         if (rbuf[0] == CMD_ACK)
            return 1;

         /* response to CMD_READ */
         if ((rbuf[0] & 0xF8) != CMD_ACK)
            return 0;

         len = (rbuf[0] & 0x07) + 2;

         if (n_recv == len) {
            if (n_recv > size)
               memcpy(buffer, rbuf, size);
            else
               memcpy(buffer, rbuf, n_recv);
            return len;
         }
      }

      watchdog_refresh();

   } while (1);

   return len;
}

/*------------------------------------------------------------------*/

void uart1_init_buffer()
{
   n_recv = 0;
   ti1_shadow = 1;
}

/*------------------------------------------------------------------*/

#endif // UART1_MSCB ##################################################

/*------------------------------------------------------------------*/

void uart_init(unsigned char port, unsigned char baud)
/********************************************************************\

  Routine: uart_init

  Purpose: Initialize serial interface, user Timer 1 for UART0 and
           (optionally) Timer 2 (4 for F020) for UART1

  Input:
    unsigned char baud
      1:    2400
      2:    4800
      3:    9600
      4:   19200
      5:   28800
      6:   38400
      7:   57600
      8:  115200
      9:  172800
     10:  345600

\********************************************************************/
{
#if defined (CPU_C8051F310)        // 24.5 MHz
   unsigned char code baud_table[] =
     {0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
	  0x100 - 0,    //  N/A
      0x100 - 213,  //  57600
      0x100 - 106,  // 115200
      0x100 - 71,   // 172800
      0x100 - 35 }; // 345600
#elif defined(CPU_C8051F320)       // 12 MHz
   unsigned char code baud_table[] =
     {0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 208,  //  28800
      0x100 - 156,  //  38400
      0x100 - 104,  //  57600
      0x100 - 52,   // 115200
      0x100 - 35,   // 172800  2% error
      0x100 - 17 }; // 345600  2% error
#elif defined(SCS_210)             // 24.5 MHz
   unsigned char code baud_table[] =  // UART0 via timer 2
     {0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 160,  //   9600  0.3% error
      0x100 - 80,   //  19200  0.3% error
      0x100 - 53,   //  28800  0.3% error
      0x100 - 40,   //  38400  0.3% error
      0x100 - 27,   //  57600  1.5% error
      0x100 - 13,   // 115200  2.2% error
      0x100 - 9,    // 172800  1.5% error
      0x100 - 0 };  // N/A
   unsigned char code baud_table1[] =  // UART1 via timer 1
     {0x100 - 0,    //  N/A
      0x100 - 212,  //   4800  0.3% error
      0x100 - 106,  //   9600  0.3% error
      0x100 - 53,   //  19200  0.3% error
      0x100 - 35,   //  28800  1.3% error
      0x100 - 27,   //  38400  1.6% error
      0x100 - 18,   //  57600  1.6% error
      0x100 - 9,    // 115200  1.6% error
      0x100 - 6,    // 172800  1.6% error
      0x100 - 3 };  // 345600  1.6% error
#elif defined(CPU_C8051F120)       // 98 MHz
   unsigned char code baud_table[] =  // UART0 via timer 2
     {0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 0,    //  N/A
      0x100 - 213,  //  28800  0.2% error
      0x100 - 160,  //  38400  0.3% error
      0x100 - 106,  //  57600  0.3% error
      0x100 - 53,   // 115200  0.3% error
      0x100 - 35,   // 172800  1.3% error
      0x100 - 18 }; // 345600  1.6% error
#if defined(UART1_MSCB) || defined(UART1_DEVICE)
   unsigned char code baud_table1[] =  // UART1 via timer 1
     {0x100 - 0,    //  N/A
      0x100 - 212,  //   4800  0.3% error
      0x100 - 106,  //   9600  0.3% error
      0x100 - 53,   //  19200  0.3% error
      0x100 - 35,   //  28800  1.3% error
      0x100 - 27,   //  38400  1.6% error
      0x100 - 18,   //  57600  1.6% error
      0x100 - 9,    // 115200  1.6% error
      0x100 - 6,    // 172800  1.6% error
      0x100 - 3 };  // 345600  1.6% error
#endif
#else                              // 11.0592 MHz
   unsigned char code baud_table[] =
     {0x100 - 144,  //   2400
      0x100 - 72,   //   4800
      0x100 - 36,   //   9600
      0x100 - 18,   //  19200
      0x100 - 12,   //  28800
      0x100 - 9,    //  38400
      0x100 - 6,    //  57600
      0x100 - 3,    // 115200
      0x100 - 2,    // 172800
      0x100 - 1 };  // 345600
#endif

   if (port == 0) { /*---- UART0 ----*/

#ifdef CPU_C8051F120
      SFRPAGE = UART0_PAGE;
#endif

      SCON0 = 0xD0;                // Mode 3, 9 bit, receive enable

#ifdef CPU_C8051F120

      SFRPAGE = UART0_PAGE;
      SSTA0 = 0x15;                // User Timer 2 for baud rate, div2 disabled

      SFRPAGE = TMR2_PAGE;
      TMR2CF = 0x08;               // use system clock for timer 2
      RCAP2H = 0xFF;
      RCAP2L = baud_table[baud - 1];  // load initial values
      TMR2CN = 0x04;               // start timer 2
      SFRPAGE = UART0_PAGE;

#else // CPU_C8051F120

      TMOD  = (TMOD & 0x0F)| 0x20; // Timer 1 8-bit counter with auto reload

#if defined(CPU_C8051F310) || defined(CPU_C8051F320)
      CKCON |= 0x08;               // use system clock
#else
      T2CON &= ~30;                // User Timer 1 for baud rate
      CKCON |= 0x10;               // use system clock
#endif

      TH1 = baud_table[baud - 1];  // load initial values
      TR1 = 1;                     // start timer 1

#endif // CPU_C8051F120

      ES0 = 1;                     // enable serial interrupt
      PS0 = 0;                     // serial interrupt low priority


#if defined(UART1_MSCB) || defined(UART1_DEVICE)

   } else { /*---- UART1 ----*/

#if defined(CPU_C8051F020)
      SCON1 = 0x50;                // Mode 1, 8 bit, receive enable

      T4CON = 0x34;                // timer 4 RX+TX mode
      RCAP4H = 0xFF;
      RCAP4L = baud_table[baud - 1];

      EIE2 |= 0x40;                // enable serial interrupt
      EIP2 &= ~0x40;               // serial interrupt low priority


#elif defined(SCS_210)             // 24.5 MHz
      SFRPAGE = UART1_PAGE;
      SCON1 = 0x50;                // Mode 1, 8 bit, receive enable

      SFRPAGE = TIMER01_PAGE;
      TMOD  = (TMOD & 0x0F)| 0x20; // Timer 1 8-bit counter with auto reload
      CKCON = 0x00;                // use SYSCLK/12 (needed by timer 0)

      TH1 = baud_table1[baud - 1];
      TR1 = 1;                     // start timer 1

      EIE2 |= 0x40;                // enable serial interrupt
      EIP2 &= ~0x40;               // serial interrupt low priority
#elif defined(CPU_C8051F120)       // 98 MHz
      SFRPAGE = UART1_PAGE;
#ifdef UART1_MSCB
      SCON1 = 0xD0;                // Mode 3, 9 bit, receive enable
#else
      SCON1 = 0x50;                // Mode 1, 8 bit, receive enable
#endif

      SFRPAGE = TIMER01_PAGE;
      TMOD  = (TMOD & 0x0F)| 0x20; // Timer 1 8-bit counter with auto reload
      CKCON = 0x02;                // use SYSCLK/48 (needed by timer 0)

      TH1 = baud_table1[baud - 1];
      TR1 = 1;                     // start timer 1

      EIE2 |= 0x40;                // enable serial interrupt
      EIP2 |= 0x40;                // serial interrupt high priority, needed in order not
                                   // to loose data during UART0 communication
#endif

      uart1_init_buffer();
#endif // defined(UART1_MSCB) || defined(UART1_DEVICE)
   }

   EA = 1;                         // general interrupt enable
}

/*------------------------------------------------------------------*/

static unsigned long _systime;

/* LED structure */

struct {
  unsigned char mode;
  unsigned char timer;
  unsigned char interval;
  unsigned char n;
} idata leds[N_LED];

#ifdef LED_0
sbit led_0 = LED_0;
#endif
#ifdef LED_1
sbit led_1 = LED_1;
#endif
#ifdef LED_2
sbit led_2 = LED_2;
#endif
#ifdef LED_3
sbit led_3 = LED_3;
#endif
#ifdef LED_4
sbit led_4 = LED_4;
#endif
#ifdef LED_5
sbit led_5 = LED_5;
#endif

/*------------------------------------------------------------------*/

void sysclock_init(void)
/********************************************************************\

  Routine: sysclock_init

  Purpose: Initial sytem clock via timer 0

*********************************************************************/
{
   unsigned char i;

   EA = 1;                      // general interrupt enable
   ET0 = 1;                     // Enable Timer 0 interrupt
   PT1 = 1;                     // Interrupt priority high

#ifdef CPU_C8051F120
   SFRPAGE = TIMER01_PAGE;
#endif

   TMOD = (TMOD & 0x0F) | 0x01; // 16-bit counter
#if defined(SCS_210)
   CKCON = 0x00;                // use SYSCLK/12
   TH0 = 0xAF;                  // load initial value (24.5 MHz SYSCLK)
#elif defined(CPU_C8051F120)
   CKCON = 0x02;                // use SYSCLK/48 (98 MHz SYSCLK)
   TH0 = 0xAF;                  // load initial value
#elif defined(CPU_C8051F310)
   CKCON = 0x00;                // use SYSCLK/12
   TH0 = 0xAF;                  // load initial value (24.5 MHz SYSCLK)
#else
   CKCON = 0x00;                // use SYSCLK/12
   TH0 = 0xDB;                  // load initial value
#endif

   TL0 = 0x00;
   TR0 = 1;                     // start timer 0

   _systime = 0;

   for (i=0 ; i<N_LED ; i++) {
     leds[i].mode = 0;
     leds[i].timer = 0;
     leds[i].interval = 0;
     leds[i].n = 0;
   }
}

/*------------------------------------------------------------------*/

void led_int() reentrant
{
unsigned char led_i;

   /* manage blinking LEDs */
   for (led_i=0 ; led_i<N_LED ; led_i++) {
      if (leds[led_i].n > 0 && leds[led_i].timer == 0) {
         if ((leds[led_i].n & 1) && leds[led_i].n > 1)
            led_set(led_i, LED_ON);
         else
            led_set(led_i, LED_OFF);

         leds[led_i].n--;
         if (leds[led_i].n)
            leds[led_i].timer = leds[led_i].interval;
      }

      if (leds[led_i].timer)
         leds[led_i].timer--;

      if (leds[led_i].n == 0)
         led_set(led_i, LED_OFF);
   }
}

/*------------------------------------------------------------------*/

extern void tcp_timer(void);

void timer0_int(void) interrupt 1
/********************************************************************\

  Routine: timer0_int

  Purpose: Timer 0 interrupt routine for 100Hz system clock

           Reload value = 0x10000 - 0.01 / (11059200/12)

\********************************************************************/
{
#ifdef SUBM_260
   tcp_timer();
#else /* SUBM_260 */

#if defined(CPU_C8051F120)
   TH0 = 0xAF;                  // for 98 MHz clock
#elif defined(CPU_C8051F310)
   TH0 = 0xAF;                  // for 24.5 MHz clock
#else
   TH0 = 0xDC;                  // reload timer values, let LSB freely run
#endif

#endif /* !SUBM_260 */
   _systime++;                  // increment system time

   led_int();
}


/*------------------------------------------------------------------*/

void led_mode(unsigned char led, unsigned char flag) reentrant
/********************************************************************\

  Routine: led_mode

  Purpose: Set LED mode

  Input:
    int led               0 for primary, 1 for secondary
    int flag              Noninverted (0) / Inverted (1)

\********************************************************************/
{
   if (led < N_LED)
      leds[led].mode = flag;
}

/*------------------------------------------------------------------*/

void led_blink(unsigned char led, unsigned char n, int interval) reentrant
/********************************************************************\

  Routine: blink led

  Purpose: Blink primary or secondary LED for a couple of times

  Input:
    int led               0 for primary, 1 for secondary, ...
    int interval          Blink interval in ms
    int n                 Number of blinks

\********************************************************************/
{
   if (led < N_LED) {
      if (leds[led].n == 0 && leds[led].timer == 0) {
         leds[led].n = n*2+1;
         leds[led].interval = interval / 10;
         leds[led].timer = 0;
      }
   }
}

/*------------------------------------------------------------------*/

void led_set(unsigned char led, unsigned char flag) reentrant 
{
   /* invert on/off if mode == 1 */
   if (led < N_LED && leds[led].mode)
      flag = !flag;

#ifdef LED_0
   if (led == 0)
      led_0 = flag;
#endif
#ifdef LED_1
   if (led == 1)
      led_1 = flag;
#endif
#ifdef LED_2
   if (led == 2)
      led_2 = flag;
#endif
#ifdef LED_3
   if (led == 3)
      led_3 = flag;
#endif
#ifdef LED_4
   if (led == 4)
      led_4 = flag;
#endif
#ifdef LED_5
   if (led == 5)
      led_5 = flag;
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

#if defined(CPU_C8051F310) || defined(CPU_C8051F320)
   PCA0CPH4 = 0x00;
#else
   WDTCN = 0xA5;
#endif

#endif
#ifdef EXT_WATCHDOG
   EXT_WATCHDOG_PIN = !EXT_WATCHDOG_PIN;
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

   for (i = 0; i < ms; i++) {
      delay_us(1000);
      yield();
   }
}

void delay_us(unsigned int us)
{
#if defined(CPU_C8051F120) || defined(CPU_C8051F310)
   unsigned char j;
#endif

   unsigned char i;
   unsigned int remaining_us;

   if (us <= 250) {
      for (i = (unsigned char) us; i > 0; i--) {
#if defined(SCS_210)
         _nop_();
         for (j=3 ; j>0 ; j--)
            _nop_();
#elif defined(CPU_C8051F120)
         for (j=22 ; j>0 ; j--)
            _nop_();
#elif defined(CPU_C8051F310)
         _nop_();
         for (j=3 ; j>0 ; j--)
            _nop_();
#elif defined(CPU_C8051F320)
         _nop_();
         _nop_();
#else
         _nop_();
#endif
      }
   } else {
      remaining_us = us;
      while (remaining_us > 250) {
         delay_us(250);
         remaining_us -= 250;
      }
      if (us > 0)
         delay_us(remaining_us);
   }
}

/*------------------------------------------------------------------*/

#ifdef EEPROM_SUPPORT

void eeprom_read(void * dst, unsigned char len, unsigned short *offset)
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
   unsigned char i;
   unsigned char code *p;
   unsigned char *d;

   watchdog_refresh();

   p = EEPROM_OFFSET + *offset;        // read from 128-byte EEPROM page
   d = dst;

   for (i = 0; i < len; i++)
      d[i] = p[i];

   *offset += len;
}

/*------------------------------------------------------------------*/

void eeprom_write(void * src, unsigned char len, unsigned short *offset)
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
   unsigned char xdata * idata p;      // xdata pointer causes MOVX command
   unsigned char idata i, b;
   unsigned char * idata s;

   if (_flkey != 0xF1)
      return;

   DISABLE_INTERRUPTS;

#ifdef CPU_C8051F120
   SFRPAGE = LEGACY_PAGE;
#endif

#if defined(CPU_C8051F000)
   FLSCL = (FLSCL & 0xF0) | 0x08;  // set timer for 11.052 MHz clock
#elif defined(CPU_C8051F020) || defined(CPU_C8051F120)
   FLSCL = FLSCL | 1;           // enable flash writes
#endif
   PSCTL = 0x01;                // allow write

   p = EEPROM_OFFSET + *offset;
   s = src;

   for (i = 0; i < len; i++) {  // write data
      b = *s++;

#if defined(CPU_C8051F310) || defined(CPU_C8051F320)
      FLKEY = 0xA5;             // write flash key code
      FLKEY = _flkey;
#endif

      *p++ = b;
   }

   PSCTL = 0x00;                // don't allow write
   FLSCL = FLSCL & 0xF0;

   *offset += len;

   ENABLE_INTERRUPTS;

   watchdog_refresh();
}

/*------------------------------------------------------------------*/

void eeprom_erase(void)
/********************************************************************\

  Routine: eeprom_erase

  Purpose: Erase parameter EEPROM page

\********************************************************************/
{
   int idata i;
   unsigned char xdata * idata p;

   if (_flkey != 0xF1)
      return;

   DISABLE_INTERRUPTS;

#ifdef CPU_C8051F120
   SFRPAGE = LEGACY_PAGE;
#endif

#if defined(CPU_C8051F000)
   FLSCL = (FLSCL & 0xF0) | 0x08;       // set timer for 11.052 MHz clock
#elif defined(CPU_C8051F020) || defined(CPU_C8051F120)
   FLSCL = FLSCL | 1;                   // enable flash writes
#endif
   PSCTL = 0x03;                        // allow write and erase

#if defined(CPU_C8051F310) || defined(CPU_C8051F320)

// Erasing more than one page crashed the uC ???
#undef N_EEPROM_PAGE
#define N_EEPROM_PAGE 1

   p = EEPROM_OFFSET;
   for (i=0 ; i<N_EEPROM_PAGE ; i++) {
      FLKEY = 0xA5;                        // write flash key code
      FLKEY = _flkey;
      *p = 0;                              // erase page
      watchdog_refresh();
      p += 512;
   }
#else
   p = EEPROM_OFFSET;
   for (i=0 ; i<N_EEPROM_PAGE ; i++) {
      *p = 0; // erase page
      watchdog_refresh();
      p += 512;
   }
#endif

   PSCTL = 0x00;                        // don't allow write
   FLSCL = FLSCL & 0xF0;

   ENABLE_INTERRUPTS;
}

/*------------------------------------------------------------------*/

void eeprom_flash(void)
/********************************************************************\

  Routine: eeprom_flash

  Purpose: Write system and user parameters to EEPROM

\********************************************************************/
{
   unsigned char i, adr;
   unsigned short magic, offset;

   eeprom_erase();

   offset = 0;

   // system info (node address etc...)
   eeprom_write(&sys_info, sizeof(SYS_INFO), &offset);

   // user channel variables
   for (adr = 0 ; adr < _n_sub_addr ; adr++)
      for (i = 0; variables[i].width; i++)
         eeprom_write((char *)variables[i].ud + _var_size*adr,
                      variables[i].width, &offset);

   // magic
   magic = 0x1234;
   eeprom_write(&magic, 2, &offset);

   _flkey = 0;
}

/*------------------------------------------------------------------*/

unsigned char eeprom_retrieve(void)
/********************************************************************\

  Routine: eeprom_retrieve

  Purpose: Retrieve system and user parameters from EEPROM

\********************************************************************/
{
   unsigned char i, adr;
   unsigned short magic, offset;

   offset = 0;

   // system info (node address etc...)
   eeprom_read(&sys_info, sizeof(SYS_INFO), &offset);

   // user channel variables
   for (adr = 0 ; adr < _n_sub_addr ; adr++)
      for (i = 0; variables[i].width; i++)
         eeprom_read((char *)variables[i].ud + _var_size*adr,
                     variables[i].width, &offset);

   // check for magic
   eeprom_read(&magic, 2, &offset);

   return (magic == 0x1234);
}

#endif /* EEPROM_SUPPORT */

/*------------------------------------------------------------------*/

#ifdef LCD_SUPPORT

bit lcd_present;

/********************************************************************\

  Routine: lcd_setup, lcd_clear, lcd_goto, lcd_puts, putchar

  Purpose: LCD functions for HD44780/KS0066 compatible LCD displays

           Since putchar is used by printf, this function puts its
           output to the LCD as well

\********************************************************************/

#define LCD P2                  // LCD display connected to port2

#if defined(SCS_1000) || defined (SCS_1001) || defined (SCS_900)
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

lcd_out(unsigned char d, bit df)
{
   if (!lcd_present)
      return;

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

   delay_us(10);                // let signals settle
   while (LCD_DB7);             // loop if busy

   LCD_E = 0;
   delay_us(1);
   LCD_R_W = 0;
#if defined(CPU_C8051F120)
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

void lcd_setup()
{
   unsigned i=0;

#if defined(CPU_C8051F120)
   SFRPAGE = CONFIG_PAGE;
   P2MDOUT = 0xFF;             // all push-pull
#else
   PRT2CF = 0xFF;              // all push-pull
#endif

#if defined(SCS_1001) || defined(SCS_900)  // 4-line LCD display with KS0078 controller

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

   lcd_out(0x24, 0); // function set: 4-bit, RE=1
   lcd_out(0x09, 0); // ext function set: 4-line display
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

/*------------------------------------------------------------------*/

void lcd_goto(char x, char y)
{

#if defined(SCS_1001) || defined(SCS_900)
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
   if (c >= ' ')
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

#endif

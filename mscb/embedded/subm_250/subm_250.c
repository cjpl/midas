/********************************************************************\

  Name:         subm_250.c
  Created by:   Stefan Ritt

  Contents:     MSCB program for Cygnal USB sub-master
                SUBM250 running on Cygnal C8051F320

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include <stdlib.h>
#include "mscbemb.h"
#include "usb.h"

#define IDENT_STR "SUBM_250"

#define SUBM_VERSION 5   // used for PC-Submaster communication

char code svn_revision_subm[] = "$Rev$";

/*------------------------------------------------------------------*/

#define RS485_RX_SIZE 512

unsigned char xdata rs485_rx_buf[RS485_RX_SIZE];

//unsigned char xdata usb_tx_buf[EP1_PACKET_SIZE];
unsigned char xdata usb_rx_buf[EP2_PACKET_SIZE];
unsigned char n_usb_rx, n_rs485_tx, i_rs485_tx, i_rs485_rx;

unsigned char rs485_tx_bit9[4];

sbit RS485_ENABLE = RS485_EN_PIN; // port pin for RS485 enable

/*------------------------------------------------------------------*/

extern void watchdog_refresh(unsigned char from_interrupt) reentrant;

/*---- MSCB commands -----------------------------------------------*/

#define MCMD_ADDR_NODE8  0x09
#define MCMD_ADDR_NODE16 0x0A
#define MCMD_ADDR_BC     0x10
#define MCMD_ADDR_GRP8   0x11
#define MCMD_ADDR_GRP16  0x12
#define MCMD_PING8       0x19
#define MCMD_PING16      0x1A

#define MCMD_INIT        0x20
#define MCMD_GET_INFO    0x28
#define MCMD_SET_ADDR    0x34
#define MCMD_SET_BAUD    0x39

#define MCMD_FREEZE      0x41
#define MCMD_SYNC        0x49
#define MCMD_UPGRADE     0x50
#define MCMD_USER        0x58

#define MCMD_ECHO        0x61
#define MCMD_TOKEN       0x68
#define MCMD_SET_FLAGS   0x69

#define MCMD_ACK         0x78

#define MCMD_WRITE_NA    0x80
#define MCMD_WRITE_ACK   0x88

#define MCMD_FLASH       0x98

#define MCMD_READ        0xA0

#define MCMD_WRITE_BLOCK 0xB5
#define MCMD_READ_BLOCK  0xB9

#define GET_INFO_GENERAL   0
#define GET_INFO_VARIABLE  1

/*------------------------------------------------------------------*/

#define RS485_FLAG_BIT9      (1<<0)
#define RS485_FLAG_NO_ACK    (1<<1)
#define RS485_FLAG_SHORT_TO  (1<<2)
#define RS485_FLAG_LONG_TO   (1<<3)
#define RS485_FLAG_CMD       (1<<4)
#define RS485_FLAG_ADR_CYCLE (1<<5)

/*------------------------------------------------------------------*/

void setup(void)
{
   unsigned char i;

   watchdog_disable();
   
   XBR0 = 0x01;                 // Enable RX/TX
   XBR1 = 0x40;                 // Enable crossbar
   P0MDOUT = 0x90;              // P0.4: TX, P0.7: RS485 enable Push/Pull
   P0SKIP  = 0x0C;              // Skip P0.2&3 for Xtal
   P0MDIN  = 0xF3;              // P0.2&3 as analog input for Xtal
   P2MDOUT = 0x02;              // P2.1 for external watchdog

   OSCXCN = 0;
   OSCICN |= 0x03;              // Configure internal oscillator for
                                // its maximum frequency

   CLKMUL = 0x00;               // Select internal oscillator as
                                // input to clock multiplier

   CLKMUL |= 0x80;              // Enable clock multiplier
   for (i=0 ; i<100 ; i++);     // Delay for >5us
   CLKMUL |= 0xC0;              // Initialize the clock multiplier

   while(!(CLKMUL & 0x20));     // Wait for multiplier to lock

   CLKSEL |= USB_4X_CLOCK;      // Select USB clock
   CLKSEL |= SYS_INT_OSC;       // Select system clock

   VDM0CN = 0x80;               // VDD monitor enable
   while ((VDM0CN & 0x40)==0);  // wait for VDD monitor to stabilize

   /* start system clock */
   sysclock_init();

   /* init memory */
   RS485_ENABLE = 0;

   /* initialize UART0 */
   uart_init(0, BD_115200);
   PS0 = 1;                     // serial interrupt high priority

   /* Blink LEDs */
   led_blink(0, 3, 150);
   led_blink(1, 3, 150);

   /* invert first LED */
   led_mode(0, 1);

   /* dummy to prevent linker error */
   delay_us(1);
}

/*------------------------------------------------------------------*/

void serial_int(void) interrupt 4 using 1
{
   if (TI0) {
      TI0 = 0;

      /* transmit next byte */

      i_rs485_tx++;
      if (i_rs485_tx < n_rs485_tx) {

         /* set bit9 according to array */
         if (i_rs485_tx < 5 && rs485_tx_bit9[i_rs485_tx-1])
            TB80 = 1;
         else
            TB80 = 0;

         DELAY_US(INTERCHAR_DELAY);
         SBUF0 = usb_rx_buf[i_rs485_tx];
      } else {
         /* end of buffer */
         DELAY_US(10);
         RS485_ENABLE = 0;
         i_rs485_tx = n_rs485_tx = 0;
      }
   }

   if (RI0) {
      /* put received byte into buffer */
      if (i_rs485_rx < RS485_RX_SIZE)
         rs485_rx_buf[i_rs485_rx++] = SBUF0;

      RI0 = 0;
   }
}

/*------------------------------------------------------------------*/

sbit led_0 = LED_0;
sbit led_1 = LED_1;

void execute()
{
   unsigned short svn_rev;

   if (usb_rx_buf[1] == MCMD_INIT) {
      /* reboot */
      RSTSRC = 0x10;
   }

   if (usb_rx_buf[1] == MCMD_ECHO) {
      /* return echo */
      svn_rev = atoi(svn_revision_subm+6);
      led_blink(1, 1, 50);
      rs485_rx_buf[0] = MCMD_ACK;
      rs485_rx_buf[1] = SUBM_VERSION;
      rs485_rx_buf[2] = svn_rev >> 8;
      rs485_rx_buf[3] = svn_rev & 0xFF;
      usb_send(rs485_rx_buf, 4);
   }

   if (usb_rx_buf[1] == RS485_FLAG_CMD) {
      /* just blink LED, discard data */
      led_1 = !led_1;
   }

   if (usb_rx_buf[1] == MCMD_FREEZE) {

      /* just send ack */
      rs485_rx_buf[0] = MCMD_ACK;
      usb_send(rs485_rx_buf, 1);
   }

}

/*------------------------------------------------------------------*/

unsigned char rs485_send(unsigned char len, unsigned char flags)
{
   unsigned char i, j;

   /* clear receive buffer */
   i_rs485_rx = 0;

   if (flags & RS485_FLAG_CMD) {

      /* execute direct command */
      execute();
      return 0;

   } else {

      /* send buffer to RS485 */
      memset(rs485_tx_bit9, 0, sizeof(rs485_tx_bit9));

      /* set all bit9 if BIT9 flag */
      if (flags & RS485_FLAG_BIT9)
         for (i=1 ; i<len && i<5 ; i++)
            rs485_tx_bit9[i-1] = 1;

      /* set first two/four bit9 if ADR_CYCLE flag */
      if (flags & RS485_FLAG_ADR_CYCLE) {
         if (usb_rx_buf[1] == MCMD_ADDR_BC)
           j = 2;
         else
           j = 4;
         for (i=0 ; i<j ; i++)
            rs485_tx_bit9[i] = 1;
      }
      

      TB80 = rs485_tx_bit9[0];

      n_rs485_tx = len;
      DELAY_US(INTERCHAR_DELAY);
      RS485_ENABLE = 1;
      SBUF0 = usb_rx_buf[1];
      i_rs485_tx = 1;
   }

   return 1;
}

void rs485_receive(unsigned char rs485_flags)
{
unsigned char  n;
unsigned short i, j, k, to, rx_old;

   if (rs485_flags & RS485_FLAG_NO_ACK)
      return;

   if (rs485_flags & RS485_FLAG_SHORT_TO)
      to = 4;     // 400 us for PING
   else if (rs485_flags & RS485_FLAG_LONG_TO)    
      to = 50000; // 5 s for flash/upgrade
   else 
      to = 100;   // 10 ms for other commands

   n = 0;
   rx_old = 0;

   for (i = 0; i<to ; i++) {

      /* reset timeout if a charactes has been received */
      if (i_rs485_rx > rx_old) {
         i = 0;
         rx_old = i_rs485_rx;
      }

      /* check for PING acknowledge (single byte) */
      if ((usb_rx_buf[1] == MCMD_PING16 ||
           usb_rx_buf[1] == MCMD_PING16) &&
          i_rs485_rx == 1 && rs485_rx_buf[0] == MCMD_ACK) {

         led_blink(1, 1, 50);
         usb_send(rs485_rx_buf, 1);
         break;
      }

      /* check for READ error acknowledge */
      if ((usb_rx_buf[1] == MCMD_READ+1 || 
           (usb_rx_buf[1] == MCMD_ADDR_NODE16 && usb_rx_buf[5] == MCMD_READ+1)) &&
          i_rs485_rx == 1 && rs485_rx_buf[0] == MCMD_ACK) {

         led_blink(1, 1, 50);
         usb_send(rs485_rx_buf, 1);
         break;
      }

      /* check for normal acknowledge */
      if (i_rs485_rx > 0 && (rs485_rx_buf[0] & MCMD_ACK) == MCMD_ACK) {

         n = rs485_rx_buf[0] & 0x07;       // length is three LSB
         if (n == 7 && i_rs485_rx > 1) {
            n = rs485_rx_buf[1]+3;         // variable length acknowledge one byte
            if ((n & 0x80) && i_rs485_rx > 2)
               n = (rs485_rx_buf[1] & ~0x80) << 8 | (rs485_rx_buf[2]) + 4; // two bytes
         } else
            n += 2;                      // add ACK and CRC

         if (i_rs485_rx == n) {
            led_blink(1, 1, 50);
            for (j=0 ; j<n ; j+=60) {
               if (n-j == 60)
                  k = 61;
               else
                  k = n-j > 60 ? 60 : n-j;
               usb_send(rs485_rx_buf+j, k);
            }
            break;
         }
      }

      watchdog_refresh(0);
      delay_us(100);
   }

   /* send timeout */
   if (i == to) {
      rs485_rx_buf[0] = 0xFF;
      usb_send(rs485_rx_buf, 1);
   }
}

/*------------------------------------------------------------------*\

  Main loop

\*------------------------------------------------------------------*/

void main(void)
{
   unsigned char flags;
   unsigned long last = 0;

   setup();
   usb_init(usb_rx_buf, &n_usb_rx);
   n_usb_rx = 0;

   /* turn on watchdog */
   watchdog_enable(2);

   do {
      watchdog_refresh(0);

      /* enable USB reset 3 sec. after start */
      if (time() > 300)
         RSTSRC = 0x86; 
   
      /* reboot if 10 sec no activity */
      //if (last > 0 && time() - last > 1000) {
      //   RSTSRC = 0x10;
      //}

      /* check for incoming USB data */
      if (n_usb_rx) {

         last = time();

         /* signal incoming data */
         led_blink(0, 1, 50);

         /* check for ident command */
         if (n_usb_rx == 1 && usb_rx_buf[0] == 0) {
            usb_send(IDENT_STR, strlen(IDENT_STR)+1);
            n_usb_rx = 0;
            continue;
         }


         flags = usb_rx_buf[0];
         n_rs485_tx = n_usb_rx;

         if (rs485_send(n_rs485_tx, flags)) {

            /* wait until sent */
            while (n_rs485_tx)
               watchdog_refresh(0);

            /* wait for data to be received */
            rs485_receive(flags);

            /* change baud rate if requested */
            if (usb_rx_buf[1] == MCMD_ADDR_BC &&
                usb_rx_buf[3] == MCMD_SET_BAUD) {
               uart_init(0, usb_rx_buf[4]);
            }
         }                        

         /* mark USB receive buffer empty */
         n_usb_rx = 0;
      }

   } while (1);

}

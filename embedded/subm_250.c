/********************************************************************\

  Name:         subm_250.c
  Created by:   Stefan Ritt

  Contents:     MSCB program for Cygnal USB sub-master
                SUBM250 running on Cygnal C8051F320

  $Log$
  Revision 1.2  2004/03/09 14:19:08  midas
  Increased default timeout

  Revision 1.1  2004/03/04 14:36:52  midas
  Initial revision


\********************************************************************/

#include <stdio.h>
#include <string.h>
#include "mscb.h"
#include "usb.h"

/*------------------------------------------------------------------*/

unsigned char xdata usb_tx_buf[EP1_PACKET_SIZE];
unsigned char xdata usb_rx_buf[EP2_PACKET_SIZE];
unsigned char n_usb_rx, i_rs485_tx, i_rs485_rx;

/*------------------------------------------------------------------*/

extern void watchdog_refresh(void);

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

void setup(void)
{
   unsigned char i;

   XBR0 = 0x01;                 // Enable RX/TX
   XBR1 = 0x40;                 // Enable crossbar
   P0MDOUT = 0x90;              // P0.4: TX, P0.7: RS485 enable Push/Pull
   P0SKIP  = 0x0C;              // Skip P0.2&3 for Xtal
   P0MDIN  = 0xF3;              // P0.2&3 as analog input for Xtal


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

   /* enable watchdog */
   PCA0MD = 0x00;               // disable watchdog
//   PCA0CPL4 = 255;              // 71.1 msec
//   PCA0MD = 0x40;               // enable watchdog

   /* enable reset pin and watchdog reset */
   RSTSRC = 0x09;

   /* start system clock */
   sysclock_init();

   /* init memory */
   RS485_ENABLE = 0;

   /* initialize UART0 */
   uart_init(0, BD_115200);

   /* Blink LEDs */
   led_blink(0, 5, 150);
   led_blink(1, 5, 150);

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
      if (i_rs485_tx < n_usb_rx)
         SBUF0 = usb_rx_buf[i_rs485_tx];
      else {
         /* end of buffer */
         RS485_ENABLE = 0;
         i_rs485_tx = n_usb_rx = 0;
      }
   }

   if (RI0) {
      /* put received byte into buffer */
      usb_tx_buf[i_rs485_rx++] = SBUF0;

      RI0 = 0;
   }
}

/*------------------------------------------------------------------*/

void yield()
{
   watchdog_refresh();
}

/*------------------------------------------------------------------*/

void execute()
{
   if (usb_rx_buf[1] == MCMD_INIT) {
      /* reboot */
      RSTSRC = 0x10;
   }

   if (usb_rx_buf[1] == MCMD_ECHO) {
      /* return echo */
      led_blink(1, 1, 50);
      usb_tx_buf[0] = MCMD_ACK;
      usb_tx_buf[1] = 0;  // reserved for future use
      usb_send(usb_tx_buf, 2);
   }
}

/*------------------------------------------------------------------*/

#define RS485_FLAG_BIT9      (1<<0)
#define RS485_FLAG_NO_ACK    (1<<1)
#define RS485_FLAG_SHORT_TO  (1<<2)
#define RS485_FLAG_LONG_TO   (1<<3)
#define RS485_FLAG_CMD       (1<<4)

unsigned char rs485_flags;

unsigned char rs485_send()
{
   rs485_flags = usb_rx_buf[0];

   /* clear receive buffer */
   i_rs485_rx = 0;

   if (rs485_flags & RS485_FLAG_CMD) {

      /* execute direct command */
      execute();
      n_usb_rx = 0;
      return 0;

   } else {

      /* send buffer to RS485 */

      /* set bit9 */
      TB80 = (rs485_flags & RS485_FLAG_BIT9) > 0;

      RS485_ENABLE = 1;
      SBUF0 = usb_rx_buf[1];
      i_rs485_tx = 1;
   }

   return 1;
}

void rs485_receive()
{
unsigned char i, n;

   if (rs485_flags & RS485_FLAG_NO_ACK)
      return;

   n = 0;
   for (i = 0; i<100 ; i++) {

      /* check for PING acknowledge (single byte) */
      if ((usb_rx_buf[1] == MCMD_PING16 ||
           usb_rx_buf[1] == MCMD_PING16) &&
          i_rs485_rx == 1 && usb_tx_buf[0] == MCMD_ACK) {

         led_blink(1, 1, 50);
         usb_send(usb_tx_buf, 1);
         break;
      }

      /* check for normal acknowledge */
      if (i_rs485_rx > 0 && (usb_tx_buf[0] & MCMD_ACK) == MCMD_ACK) {

         n = usb_tx_buf[0] & 0x07;       // length is three LSB
         if (n == 7 && i_rs485_rx > 1)
            n = usb_tx_buf[1]+3;         // variable length acknowledge
         else
            n += 2;                      // add ACK and CRC

         if (i_rs485_rx == n) {
            led_blink(1, 1, 50);
            usb_send(usb_tx_buf, n);
            break;
         }
      }

      if (rs485_flags & RS485_FLAG_SHORT_TO)        // 200us for PING
         delay_us(2);
      else if (rs485_flags & RS485_FLAG_LONG_TO)    // 100ms for flash
         delay_ms(1);
      else
         delay_us(100);                             //  10ms for other commands
   }

   /* send timeout */
   if (i == 100) {
      usb_tx_buf[0] = 0xFF;
      usb_send(usb_tx_buf, 1);
   }
}

/*------------------------------------------------------------------*\

  Main loop

\*------------------------------------------------------------------*/

sbit led0 = P1 ^ 2;

void main(void)
{
   setup();
   usb_init(usb_rx_buf, &n_usb_rx);
   n_usb_rx = 0;

   do {
      yield();

      /* check for incoming USB data */
      if (n_usb_rx) {

         /* signal incoming data */
         led_blink(0, 1, 50);
         if (rs485_send()) {

            /* wait until sent */
            while (n_usb_rx)
               yield();

            /* wait for data to be received */
            rs485_receive();
         }
      }

   } while (1);

}

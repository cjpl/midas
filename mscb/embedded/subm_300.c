/********************************************************************\

  Name:         subm_300.c
  Created by:   Stefan Ritt

  Contents:     MSCB program for Cygnal parallel port sub-master
                SUBM300 running on Cygnal C8051F021

  $Log$
  Revision 1.2  2002/10/04 09:03:20  midas
  Small mods for scs_300

  Revision 1.1  2002/10/03 15:35:15  midas
  Initial revision


\********************************************************************/

#include <stdio.h>
#include <string.h>
#include "mscb.h"

/* bit definitions */

#define LPT_DATA         P1      // 8 data bits to LPT

sbit LPT_NSTROBE =       P2^3;   // control/status bits to LPT
sbit LPT_BUSY =          P2^7;
sbit LPT_NDATAREADY =    P2^6;
sbit LPT_NACK =          P2^5;
sbit LPT_BIT9 =          P2^4;

/*------------------------------------------------------------------*/

/* write and read pointers for receive buffer in xdata */
unsigned char xdata * data rbuf_rp;
unsigned char xdata * data rbuf_wp;

bit ti_shadow;


/* dummies needed by mscbutil.c */
MSCB_INFO_CHN code conf_param[] = {
  0
};
MSCB_INFO_CHN code channel[] = {
  0
};

SYS_INFO sys_info;

/*------------------------------------------------------------------*/

void setup(void)
{
unsigned char i;

  XBR0 = 0x04; // Enable RX/TX
  XBR1 = 0x00;
  XBR2 = 0x40; // Enable crossbar

  /* Port configuration (1 = Push Pull Output) */
  P0MDOUT = 0x01;  // P0.1: TX = Push Pull
  P1MDOUT = 0x00;  // P1: LPT
  P2MDOUT = 0x00;  // P2: LPT
  P3MDOUT = 0xE0;  // P3.5,6,7: Optocouplers

  /* Disable watchdog (for delays) */
  WDTCN  = 0xDE;
  WDTCN  = 0xAD;

  /* Select external quartz oscillator */
  OSCXCN = 0x66;  // Crystal mode, Power Factor 22E6
  OSCICN = 0x08;  // CLKSL=1 (external)

  /* enable reset pin and watchdog reset */
  RSTSRC = 0x09;

  /* initialize UART0 */
  uart_init(0, BD_115200);

  /* clear all dynamic memory */
  rbuf_wp = 0;
  rbuf_rp = 0;
  
  /* set input ports to one */
  LPT_DATA = 0xFF;
  LPT_NSTROBE = 1;
  LPT_BUSY = 1;
  LPT_NACK = 1;
  LPT_NDATAREADY = 1;
  LPT_BIT9 = 1;

  /* blink LEDs */
  for (i=0 ; i<5 ; i++)
    {
    LED = LED_ON;
    LED_SEC = LED_ON;
    delay_ms(200);
    LED = LED_OFF;
    LED_SEC = LED_OFF;
    delay_ms(100);
    }
  LED_SEC = LED_ON;

  /* Enable watchdog 95ms (@11.0592MHz) */
  WDTCN=0x07;

}

/*------------------------------------------------------------------*/

void serial_int(void) interrupt 4 using 1
{
  if (TI0)
    {
    /* signal transmitted character */
    ti_shadow = 1;
    TI0 = 0;
    }
  if (RI0)
    {
    /* check for buffer overflow */
    if (rbuf_wp + 1 == rbuf_rp)
      {
      RI0 = 0;
      return;
      }

    /* store character in read buffer */
    *rbuf_wp = SBUF0;
    rbuf_wp = (unsigned char) (rbuf_wp + 1);

    RI0 = 0;
    }
}

/*------------------------------------------------------------------*/

void check_lpt()
{
unsigned char byte;
bit bit9;

  if (LPT_NSTROBE == 1)
    return;

  /* receive new data */
  byte = LPT_DATA;
  bit9 = LPT_BIT9;

  /* signal busy to PC */
  LPT_BUSY = 1;

  /* send byte to serial port, flash LED */
  RS485_ENABLE = 1;

  LED = LED_ON;
  TB80 = bit9;
  SBUF0 = byte;

  while (!ti_shadow); /* wait until interrupt routine acknowledged send */

  RS485_ENABLE = 0;

  ti_shadow = 0;
  LED = LED_OFF;

  while (LPT_NSTROBE == 0) /* wait for strobe to be removed */
    watchdog_refresh();

  /* remove busy */
  LPT_BUSY = 0;

}

/*------------------------------------------------------------------*/

void check_rs485()
{
unsigned char byte;

  /* return if no data in buffer */
  if (rbuf_wp == rbuf_rp)
    return;

  /* abort if PC strobe is active to avoid data collision */
  if (LPT_NSTROBE == 0)
    return;

  /* get received byte */
  byte = *rbuf_rp;

  /* increment read pointer with wrap around at 256 bytes */
  rbuf_rp = (unsigned char)(rbuf_rp + 1);

  /* signal new data to PC */
  LPT_DATA = byte;
  LPT_NDATAREADY = 0;

  /* wait for PC acknowledge */
  while (LPT_NACK == 1)
    watchdog_refresh();

  /* remove data ready signal */
  LPT_NDATAREADY = 1;

  /* prepare port for input */
  LPT_DATA = 0xFF;

  /* wait for PC acknowlege to be removed */
  while (LPT_NACK == 0)
    watchdog_refresh();
}

/*------------------------------------------------------------------*\

  Main loop

\*------------------------------------------------------------------*/

void main(void)
{
  setup();

  TI0 = 0;
  ti_shadow = 0;
  RS485_ENABLE = 0;

  LPT_BUSY = 0;  /* indicate ready to receive LPT data */

  do
    {
    watchdog_refresh();

    /* check if data from PC */
    check_lpt();

    /* check if data frm RS485 */
    check_rs485();

    } while (1); /* repeat forever */
  
}


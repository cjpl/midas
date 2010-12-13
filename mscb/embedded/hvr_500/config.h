/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-210 RS232 node

  $Id$

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define HVR_500

/* define CPU type */
#define CPU_C8051F310

/* include file for CPU registers etc. */
#include <c8051F310.h>

/* CPU clock speed */
#define CLK_25MHZ

/* ports for LED's */
#define LED_0 P2 ^ 7
#define LED_1 P2 ^ 6 
#define LED_2 P2 ^ 5 
#define LED_3 P2 ^ 4
#define LED_4 P2 ^ 3

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 1

/* port pin for RS485 enable signal */
#define RS485_EN_PIN P0 ^ 7

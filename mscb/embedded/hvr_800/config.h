/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-210 RS232 node

  $Id$

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define HVR_800

/* define CPU type */
#define CPU_C8051F120

/* include file for CPU registers etc. */
#include <c8051F120.h>

/* CPU clock speed */
#define CLK_25MHZ

/* ports for LED's */
#define LED_0 P1 ^ 7
#define LED_1 P1 ^ 6 
#define LED_2 P1 ^ 5 
#define LED_3 P1 ^ 4
#define LED_4 P1 ^ 3
#define LED_5 P1 ^ 2
#define LED_6 P1 ^ 1
#define LED_7 P1 ^ 0

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 1

/* port pin for RS485 enable signal */
#define RS485_EN_PIN P0 ^ 5

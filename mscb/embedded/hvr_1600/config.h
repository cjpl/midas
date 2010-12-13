/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-210 RS232 node

  $Id: config.h 4915 2010-12-13 16:25:03Z ritt $

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define HVR_1600

/* define CPU type */
#define CPU_C8051F120

/* include file for CPU registers etc. */
#include <c8051F120.h>

/* CPU clock speed */
#define CLK_98MHZ

/* ports for LED's */
#define LED_0 P7 ^ 0
#define LED_1 P7 ^ 1 
#define LED_2 P7 ^ 2 
#define LED_3 P7 ^ 3
#define LED_4 P7 ^ 4
#define LED_5 P7 ^ 5
#define LED_6 P7 ^ 6
#define LED_7 P7 ^ 7
#define LED_8 P3 ^ 0
#define LED_9 P3 ^ 1 
#define LED_10 P3 ^ 2 
#define LED_11 P3 ^ 3
#define LED_12 P3 ^ 4
#define LED_13 P3 ^ 5
#define LED_14 P3 ^ 6
#define LED_15 P3 ^ 7

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 1

/* port pin for RS485 enable signal */
#define RS485_EN_PIN P0 ^ 5

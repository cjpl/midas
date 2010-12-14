/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-310 GPIB port interface

  $Id$

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define SCS_310

/* define CPU type */
#define CPU_C8051F020

/* include file for CPU registers etc. */
#include <c8051F020.h>

/* CPU clock speed */
#define CLK_25MHZ

/* ports for LED's */
#define LED_0 P3 ^ 3
#define LED_1 P3 ^ 4 

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 0

/* port pin for RS485 enable signal */
#define RS485_EN_PIN P3 ^ 5

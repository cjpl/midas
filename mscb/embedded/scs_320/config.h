/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-320 VME crate controller

  $Id$

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define SCS_320

/* define CPU type */
#define CPU_C8051F320

/* include file for CPU registers etc. */
#include <c8051F320.h>

/* CPU clock speed */
#define CLK_25MHZ

/* ports for LED's */
#define LED_0 P2 ^ 7

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 0

/* port pin for RS485 enable signal */
#define RS485_EN_PIN P0 ^ 3

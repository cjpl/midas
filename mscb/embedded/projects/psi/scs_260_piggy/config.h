/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-260 piggy back device

  $Id: config.h 4918 2010-12-14 12:49:37Z ritt $

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define SCS_260_PIGGY

/* define CPU type */
#define CPU_C8051F120

/* include file for CPU registers etc. */
#include <c8051F120.h>

/* CPU clock speed */
#define CLK_98MHZ

/* ports for LED's */
#define LED_0 P2 ^ 0

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 1

/* port pin for RS485 enable signals */
#define RS485_EN_PIN P0 ^ 4

/* watchdog flags */
//#define CFG_EXT_WATCHDOG               // external watchdog is usually not soldered
//#define EXT_WATCHDOG_PIN P0 ^ 2        // external watchdog operated through DAC1


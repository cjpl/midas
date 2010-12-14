/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                USB sub-master SUBM250 running on Cygnal C8051F320

  $Id$

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define SUBM_250

/* define CPU type */
#define CPU_C8051F320

/* include file for CPU registers etc. */
#include <c8051F320.h>

/* CPU clock speed */
#define CLK_25MHZ

/* ports for LED's */
#define LED_0 P1 ^ 2
#define LED_1 P1 ^ 3 

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 0

/* port pin for RS485 enable signal */
#define RS485_EN_PIN P1 ^ 0

/* watchdog flags */
#define CFG_USE_WATCHDOG               // enable watchdog functionality
#define CFG_EXT_WATCHDOG               // external watchdog is present
#define EXT_WATCHDOG_PIN P2 ^ 1        // external watchdog pin
#undef CFG_HAVE_EEPROM


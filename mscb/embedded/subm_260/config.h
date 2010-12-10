/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-2001 device

  $Id: scs_210.c 4906 2010-12-09 10:38:33Z ritt $

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define SUBM_260

/* define CPU type */
#define CPU_C8051F120

/* include file for CPU registers etc. */
#include <c8051F120.h>

/* CPU clock speed */
#define CLK_49MHZ

/* ports for LED's */
#define LED_0 P0 ^ 2
#define LED_1 P0 ^ 3

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 0

/* port pin for RS485 enable signals */
#define RS485_EN_PIN P0 ^ 4

/* watchdog flags */
#define CFG_EXT_WATCHDOG               // external watchdog is present
#define EXT_WATCHDOG_PIN_DAC1          // external watchdog operated through DAC1

/* this node does not support variables in EEPROM */
#undef CFG_HAVE_EEPROM
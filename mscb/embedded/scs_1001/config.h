/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-1001 device

  $Id: config.h 4906 2010-12-09 10:38:33Z ritt $

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define SCS_1001

/* define CPU type */
#define CPU_C8051F120

/* include file for CPU registers etc. */
#include <c8051F120.h>

/* CPU clock speed */
#define CLK_98MHZ

/* ports for LED's */
#define LED_0 P2 ^ 0
#define LED_1 P0 ^ 6
#define LED_2 P0 ^ 7 // buzzer

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 1

/* port pin for RS485 enable signals */
#define RS485_EN_PIN P0 ^ 5
#define RS485_SEC_EN_PIN P0 ^ 4

/* watchdog flags */
//#define CFG_EXT_WATCHDOG               // external watchdog is usually not soldered
//#define EXT_WATCHDOG_PIN P1 ^ 4        // external watchdog operated through DAC1

/* switches to activate certain features in the framework */
#define CFG_HAVE_LCD                   // 8-bit LCD is present
#define CFG_EXTENDED_VARIABLES         // remote variables on slave modules
#define CFG_UART1_MSCB

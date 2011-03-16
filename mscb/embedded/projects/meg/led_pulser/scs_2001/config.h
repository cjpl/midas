/********************************************************************\

  Name:         config.h
  Created by:   Stefan Ritt


  Contents:     Configuration file for 
                for SCS-2001 device

  $Id: config.h 4918 2010-12-14 12:49:37Z ritt $

\********************************************************************/

/* Define device name. Used by various parts of the framwork to enable
   or disable device-specific features */
#define SCS_2001

/* define CPU type */
#define CPU_C8051F120

/* include file for CPU registers etc. */
#include <c8051F120.h>

/* CPU clock speed */
#define CLK_98MHZ

/* ports for LED's */
#define LED_0 P0 ^ 6
#define LED_1 P0 ^ 7

/* polarity of LED: "0" if a low level activates the LEDs, 
   "1" if a high level activates the LEDs */
#define LED_ON 0

/* port pin for RS485 enable signals */
#define RS485_EN_PIN P0 ^ 5
#define RS485_SEC_EN_PIN P0 ^ 4

/* watchdog flags */
//#define CFG_EXT_WATCHDOG               // external watchdog is usually not soldered
//#define EXT_WATCHDOG_PIN_DAC1          // external watchdog operated through DAC1

/* switches to activate certain features in the framework */
#define CFG_HAVE_LCD                   // 8-bit LCD is present
#define CFG_LCD_8BIT
//#define CFG_DYN_VARIABLES              // variables are defined at run-time (plug-in boards)
#define CFG_EXTENDED_VARIABLES         // remote variables on slave modules
#define CFG_UART1_MSCB                 // external slave modules are connected via UART1
#define CFG_HAVE_RTC                   // real-time clock is present
#define CFG_HAVE_EMIF                  // external RAM is present

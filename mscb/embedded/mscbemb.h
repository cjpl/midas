/********************************************************************\

  Name:         mscbemb.h
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus protocol commands

  $Id$

\********************************************************************/

/*---- map small defines from makefile to capital defines ----------*/

#ifdef scs_210
#define SCS_210
#endif
#ifdef scs_220
#define SCS_220
#endif
#ifdef subm_250
#define SUBM_250
#endif
#ifdef subm_300
#define SUBM_300
#endif
#ifdef scs_300
#define SCS_300
#endif
#ifdef scs_310
#define SCS_310
#endif
#ifdef scs_320
#define SCS_320
#endif
#ifdef scs_400
#define SCS_400
#endif
#ifdef scs_500
#define SCS_500
#endif
#ifdef scs_520
#define SCS_520
#endif
#ifdef scs_600
#define SCS_600
#endif
#ifdef scs_700
#define SCS_700
#endif
#ifdef scs_800
#define SCS_800
#endif
#ifdef scs_900
#define SCS_900
#endif
#ifdef scs_910
#define SCS_910
#endif
#ifdef scs_920
#define SCS_920
#endif
#ifdef scs_1000
#define SCS_1000
#endif
#ifdef scs_1001
#define SCS_1001
#endif
#ifdef scs_2000
#define SCS_2000
#endif
#ifdef hvr_200
#define HVR_200
#endif
#ifdef hvr_400
#define HVR_400
#endif
#ifdef hvr_500
#define HVR_500
#endif
#ifdef hvr_800
#define HVR_800
#endif
#ifdef hvr_test
#define HVR_TEST
#endif
#ifdef meg_splt_bp   // MEG experiment "Lecce" splitter backplane
#define MEG_SPLT_BP
#endif
#ifdef   fgd_008     // T2K-FGD SiPM 8-channel bias board K. Mizouchi Sep/19/2006
#define  FGD_008
#endif

/*---- CPU specific items ------------------------------------------*/

/* default flags */
#define USE_WATCHDOG
#define HAVE_EEPROM

/*--------------------------------*/
#if defined(SCS_210)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P3 ^ 4
#define LED_1 P3 ^ 3
#define LED_ON 0
sbit RS485_ENABLE = P3 ^ 5;

/*--------------------------------*/
#elif defined(SCS_220)
#include <c8051F020.h>
#define CPU_C8051F020

#define LED_0 P3 ^ 4
#define LED_1 P3 ^ 3
#define LED_ON 0
sbit RS485_ENABLE = P3 ^ 5;
sbit RS485_SEC_ENABLE = P0 ^ 6;

/*--------------------------------*/
#elif defined(SUBM_250)
#include "c8051F320.h" // don't use the one from Keil !!
#define CPU_C8051F320

#define LED_0 P1 ^ 2
#define LED_1 P1 ^ 3
#define LED_ON 0
sbit RS485_ENABLE = P1 ^ 0;

//#define EXT_WATCHDOG
//sbit EXT_WATCHDOG_PIN = P2 ^ 1;

#undef HAVE_EEPROM

/*--------------------------------*/
#elif defined(SUBM_260)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P0 ^ 2
#define LED_1 P0 ^ 3
#define LED_ON 0
sbit RS485_ENABLE = P0 ^ 4;

#undef HAVE_EEPROM

/*--------------------------------*/
#elif defined(SCS_300) || defined(SCS_310)
#include <c8051F020.h>
#define CPU_C8051F020

#define LED_0 P3 ^ 3
#define LED_1 P3 ^ 4
#define LED_ON 0
sbit RS485_ENABLE = P3 ^ 5;

/*--------------------------------*/
#elif defined(SCS_320)
#include <c8051F320.h>
#define CPU_C8051F320

#define LED_0 P2 ^ 7
#define LED_ON 0
sbit RS485_ENABLE = P0 ^ 3;

/*--------------------------------*/
#elif defined(SUBM_300)
#include <c8051F020.h>
#define CPU_C8051F020

#define LED_0 P3 ^ 3
#define LED_1 P3 ^ 4
#define LED_ON 0
sbit RS485_ENABLE = P3 ^ 5;

#undef HAVE_EEPROM

/*--------------------------------*/
#elif defined(SCS_400) || defined(SCS_500)
#include <c8051F000.h>
#define CPU_C8051F000

#define LED_0 P3 ^ 4
#define LED_ON 1
sbit RS485_ENABLE = P3 ^ 5;

/*--------------------------------*/
#elif defined(SCS_520) || defined(SCS_600) || defined(SCS_700) || defined (SCS_800) || defined (SCS_900) || defined (SCS_910) || defined(CFD_950)
#include <c8051F000.h>
#define CPU_C8051F000

#define LED_0 P3 ^ 4
#define LED_ON 0
sbit RS485_ENABLE = P3 ^ 5;

/*--------------------------------*/
#elif defined (SCS_920)
#include <c8051F000.h>
#define CPU_C8051F000

#define LED_0 P3 ^ 4
#define LED_ON 1
sbit RS485_ENABLE = P3 ^ 5;

/*--------------------------------*/
#elif defined(SCS_1000)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P2 ^ 0
#define LED_1 P0 ^ 6
#define LED_2 P0 ^ 7 // buzzer
#define LED_ON 1     // will be zero when 2981 will be used instead of 2982
sbit RS485_ENABLE = P0 ^ 5;
sbit RS485_SEC_ENABLE = P0 ^ 4;

#define HAVE_LCD

/*--------------------------------*/
#elif defined(SCS_1001)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P2 ^ 0
#define LED_1 P0 ^ 6
#define LED_2 P0 ^ 7 // buzzer
#define LED_ON 1
sbit RS485_ENABLE = P0 ^ 5;
sbit RS485_SEC_ENABLE = P0 ^ 4;

//#define EXT_WATCHDOG              // use external watchdog
sbit EXT_WATCHDOG_PIN = P1 ^ 4;

#define HAVE_LCD

/*--------------------------------*/
#elif defined(SCS_2000)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P0 ^ 6
#define LED_1 P0 ^ 7
#define LED_ON 0
sbit RS485_ENABLE = P0 ^ 5;
sbit RS485_SEC_ENABLE = P0 ^ 4;

//#define EXT_WATCHDOG              // use external watchdog
//sbit EXT_WATCHDOG_PIN = DAC0;

#define HAVE_LCD
#define LCD_8BIT
#define DYN_VARIABLES
#define HAVE_RTC

/*--------------------------------*/
#elif defined(HVR_200)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P1 ^ 6
#define LED_1 P1 ^ 7 
#define LED_ON 1
sbit RS485_ENABLE = P0 ^ 5;

/*--------------------------------*/
#elif defined(HVR_400)
#include <c8051F310.h>
#define CPU_C8051F310

#define LED_0 P2 ^ 4
#define LED_1 P2 ^ 5 
#define LED_2 P2 ^ 6 
#define LED_3 P2 ^ 7
#define LED_ON 1
sbit RS485_ENABLE = P0 ^ 7;

/*--------------------------------*/
#elif defined(HVR_500)
#include <c8051F310.h>
#define CPU_C8051F310

#define LED_0 P2 ^ 7
#define LED_1 P2 ^ 6 
#define LED_2 P2 ^ 5 
#define LED_3 P2 ^ 4
#define LED_4 P2 ^ 3
#define LED_ON 1
sbit RS485_ENABLE = P0 ^ 7;

/*--------------------------------*/
#elif defined(HVR_800)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P1 ^ 7
#define LED_1 P1 ^ 6 
#define LED_2 P1 ^ 5 
#define LED_3 P1 ^ 4
#define LED_4 P1 ^ 3
#define LED_5 P1 ^ 2
#define LED_6 P1 ^ 1
#define LED_7 P1 ^ 0
#define LED_ON 1
sbit RS485_ENABLE = P0 ^ 5;

/*--------------------------------*/
#elif defined(HVR_TEST)
#include <c8051F000.h>
#define CPU_C8051F000

#define LED_0 P3 ^ 4
#define LED_ON 0
sbit RS485_ENABLE = P3 ^ 5;

/*--------------------------------*/
#elif defined(MEG_SPLT_BP)
#include <c8051F120.h>
#define CPU_C8051F120

#define LED_0 P3 ^ 3
#define LED_ON 1
sbit RS485_ENABLE = P0 ^ 4;

/*--------------------------------*/
#elif defined(FGD_008)
#include <c8051F310.h>
#define  CPU_C8051F310

// Add a new entry for the T2K-FGD SiPM 8-channel bias board
// K. Mizouchi Sep/19/2006 
//
// LED_0, which is used as a primary led in the system, is 
// connected to a spare pin (NC) and secondary LED, LED_1, 
// is made to be undefined. It is only used in mscbutil 
// libraries and we don't have it physically.
// 
// We define the SELECT_LED_DACX pin here, because we want to 
// use led_xxxx() in mscbutil.c for DAC LED control. 
#define LED_0 P2 ^ 4

#if defined(LED_1) 
#undef LED_1
#endif

#define LED_2 P1 ^ 4
#define LED_3 P1 ^ 5
#define LED_4 P1 ^ 6
#define LED_5 P1 ^ 7
#define LED_6 P2 ^ 0
#define LED_7 P2 ^ 1
#define LED_8 P2 ^ 2
#define LED_9 P2 ^ 3

#define LED_ON 0

sbit RS485_ENABLE = P0 ^ 3;
/*--------------------------------*/
#else
#error Please define SCS_xxx or HVR_xxx in project options
#endif

#define LED_OFF !LED_ON

#if defined(LED_7)
#define N_LED 8
#elif defined(LED_6)
#define N_LED 7
#elif defined(LED_5)
#define N_LED 6
#elif defined(LED_4)
#define N_LED 5
#elif defined(LED_3)
#define N_LED 4
#elif defined(LED_2)
#define N_LED 3
#elif defined(LED_1)
#define N_LED 2
#elif defined(LED_0)
#define N_LED 1
#else
#define N_LED 0
#endif
       
/* map SBUF0 & Co. to SBUF */
#if !defined(CPU_C8051F020) && !defined(CPU_C8051F120) && !defined(CPU_C8051F310) && !defined(CPU_C8051F320)
#define SCON0    SCON
#define SBUF0    SBUF
#define TI0      TI
#define RI0      RI
#define RB80     RB8
#define ES0      ES
#define PS0      PS
#endif

#if defined(CPU_C8051F020) || defined(CPU_C8051F120)
#define PS0      PS
#endif

#if defined(CPU_C8051F310) || defined(CPU_C8051F320)
#define EEPROM_OFFSET 0x3A00 // 0x3A00-0x3BFF = 0.5kB, 0x3C00 cannot be erased!
#define N_EEPROM_PAGE      1 // 1 page  @ 512 bytes
#elif defined(CPU_C8051F120)
#define EEPROM_OFFSET 0xE000 // 0xE000-0xEFFF = 4kB
#define N_EEPROM_PAGE      8 // 8 pages @ 512 bytes
#else
#define EEPROM_OFFSET 0x6000 // 0x6000-0x6FFF = 4kB
#define N_EEPROM_PAGE      8 // 8 pages @ 512 bytes
#endif


/* UART1 specific definitions */
#if defined(SCS_210) | defined(SCS_220)
#define UART1_DEVICE                     // use direct device communication
#endif

#if defined(UART1_DEVICE) && defined(HAVE_LCD)
char putchar1(char c);                   // putchar cannot be used with LCD support
#endif

#ifndef UART1_DEVICE
#if defined(SCS_1000) || defined(SCS_1001) || defined(SCS_2000)
#define UART1_MSCB                       // UART1 connected as master to MSCB slave bus
#endif
#endif

/*---- Delay macro to be used in interrupt routines etc. -----------*/

#if defined(SUBM_260)
#define DELAY_US(_us) { \
   unsigned char _i,_j; \
   for (_i = (unsigned char) _us; _i > 0; _i--) \
      for (_j=2 ; _j>0 ; _j--) \
         _nop_(); \
}
#elif defined(SCS_210)
#define DELAY_US(_us) { \
   unsigned char _i,_j; \
   for (_i = (unsigned char) _us; _i > 0; _i--) { \
      _nop_(); \
      for (_j=3 ; _j>0 ; _j--) \
         _nop_(); \
   } \
}
#elif defined(CPU_C8051F120)
#define DELAY_US(_us) { \
   unsigned char _i,_j; \
   for (_i = (unsigned char) _us; _i > 0; _i--) \
      for (_j=19 ; _j>0 ; _j--) \
         _nop_(); \
}
#define DELAY_US_REENTRANT(_us) { \
   unsigned char _i,_j; \
   for (_i = (unsigned char) _us; _i > 0; _i--) \
      for (_j=3 ; _j>0 ; _j--) \
         _nop_(); \
}
#elif defined(CPU_C8051F310)
#define DELAY_US(_us) { \
   unsigned char _i,_j; \
   for (_i = (unsigned char) _us; _i > 0; _i--) { \
      _nop_(); \
      for (_j=3 ; _j>0 ; _j--) \
         _nop_(); \
   } \
}
#elif defined(CPU_C8051F320)
#define DELAY_US(_us) { \
   unsigned char _i,_j; \
   for (_i = (unsigned char) _us; _i > 0; _i--) { \
      _nop_(); \
      for (_j=1 ; _j>0 ; _j--) \
         _nop_(); \
   } \
}
#else
#define DELAY_US(_us) { \
   unsigned char _i; \
   for (_i = (unsigned char) _us; _i > 0; _i--) { \
      _nop_(); \
      _nop_(); \
   } \
}
#endif

/*---- MSCB commands -----------------------------------------------*/

#define PROTOCOL_VERSION 5
#define INTERCHAR_DELAY 20      // 20us between characters

/* Version history:

1.0 Initial
1.1 Add unit prefix, implement upgrade command
1.2 Add CMD_ECHO, CMD_READ with multiple channels
1.3 Add "set node name"
1.4 Remove CMD_xxxs_CONF
1.5 Return 0x0A for protected pages on upload
1.6 Upload subpages of 60 bytes with ACK
1.7 Upload with simplified CRC code
1.8 Add 20us delay betwee characters for repeater
*/

#define CMD_ADDR_NODE8  0x09
#define CMD_ADDR_NODE16 0x0A
#define CMD_ADDR_BC     0x10
#define CMD_ADDR_GRP8   0x11
#define CMD_ADDR_GRP16  0x12
#define CMD_PING8       0x19
#define CMD_PING16      0x1A

#define CMD_INIT        0x20
#define CMD_GET_INFO    0x28
#define CMD_SET_ADDR    0x33
#define CMD_SET_NAME    0x37
#define CMD_SET_BAUD    0x39

#define CMD_FREEZE      0x41
#define CMD_SYNC        0x49
#define CMD_SET_TIME    0x4E
#define CMD_UPGRADE     0x50
#define CMD_USER        0x58

#define CMD_ECHO        0x61
#define CMD_TOKEN       0x68
#define CMD_GET_UPTIME  0x70

#define CMD_ACK         0x78

#define CMD_WRITE_NA    0x80
#define CMD_WRITE_ACK   0x88

#define CMD_FLASH       0x98

#define CMD_READ        0xA0

#define CMD_WRITE_BLOCK 0xB5
#define CMD_READ_BLOCK  0xB9

#define GET_INFO_GENERAL   0
#define GET_INFO_VARIABLES 1

#define ADDR_SET_NODE      1
#define ADDR_SET_HIGH      2
#define ADDR_SET_GROUP     3

/*---- MSCB upgrade commands ---------------------------------------*/

#define UCMD_ECHO          1
#define UCMD_ERASE         2
#define UCMD_PROGRAM       3
#define UCMD_VERIFY        4
#define UCMD_READ          5
#define UCMD_REBOOT        6
#define UCMD_RETURN        7

/*---- flags from the configuration and status register (CSR) ------*/

#define CSR_DEBUG       (1<<0)
#define CSR_LCD_PRESENT (1<<1)
#define CSR_SYNC_MODE   (1<<2)
#define CSR_FREEZE_MODE (1<<3)
#define CSR_WD_RESET    (1<<2)

/*---- baud rates used ---------------------------------------------*/

#define BD_2400            1
#define BD_4800            2
#define BD_9600            3
#define BD_19200           4
#define BD_28800           5
#define BD_38400           6
#define BD_57600           7
#define BD_115200          8
#define BD_172800          9
#define BD_345600         10

/*---- info structures ---------------------------------------------*/

typedef struct {
   unsigned char protocol_version;
   unsigned char node_status;
   unsigned char n_variables;
   unsigned short node_address;
   unsigned short group_address;
   unsigned short watchdog_resets;
   char node_name[16];
} MSCB_INFO;

typedef struct {
   unsigned char width;         // width in bytes
   unsigned char unit;          // physical units UNIT_xxxx
   unsigned char prefix;        // unit prefix PRFX_xxx
   unsigned char status;        // status (not yet used)
   unsigned char flags;         // flags MSCBF_xxx
   char name[8];                // name
   void *ud;                    // point to user data buffer

#if defined(SCS_1000) || defined(SCS_1001) || defined(SCS_2000)
   float min, max, delta;       // limits for button control
   unsigned short node_address; // address for remote node on subbus
   unsigned char  channel;      // address for remote channel subbus
#endif
} MSCB_INFO_VAR;

#define MSCBF_FLOAT    (1<<0)   // channel in floating point format
#define MSCBF_SIGNED   (1<<1)   // channel is signed integer
#define MSCBF_DATALESS (1<<2)   // channel doesn't contain data
#define MSCBF_HIDDEN   (1<<3)   // used for internal config parameters
#define MSCBF_REMIN    (1<<4)   // get variable from remote node on subbus
#define MSCBF_REMOUT   (1<<5)   // send variable to remote node on subbus
#define MSCBF_INVALID  (1<<6)   // cannot read remote variable

/* physical units */

#define PRFX_PICO       -12
#define PRFX_NANO        -9
#define PRFX_MICRO       -6
#define PRFX_MILLI       -3
#define PRFX_NONE         0
#define PRFX_KILO         3
#define PRFX_MEGA         6
#define PRFX_GIGA         9
#define PRFX_TERA        12

#define UNIT_UNDEFINED    0

// SI base units
#define UNIT_METER        1
#define UNIT_GRAM         2
#define UNIT_SECOND       3
#define UNIT_MINUTE       4
#define UNIT_HOUR         5
#define UNIT_AMPERE       6
#define UNIT_KELVIN       7
#define UNIT_CELSIUS      8
#define UNIT_FARENHEIT    9

// SI derived units

#define UNIT_HERTZ       20
#define UNIT_PASCAL      21
#define UNIT_BAR         22
#define UNIT_WATT        23
#define UNIT_VOLT        24
#define UNIT_OHM         25
#define UNIT_TESLA       26
#define UNIT_LITERPERSEC 27
#define UNIT_RPM         28
#define UNIT_FARAD       29
#define UNIT_JOULE       30

// computer units

#define UNIT_BOOLEAN     50
#define UNIT_BYTE        52
#define UNIT_WORD        53
#define UNIT_DWORD       54
#define UNIT_ASCII       55
#define UNIT_STRING      56
#define UNIT_BAUD        57

// others

#define UNIT_PERCENT     90
#define UNIT_PPM         91
#define UNIT_COUNT       92
#define UNIT_FACTOR      93

/*------------------------------------------------------------------*/

typedef struct {                // system info stored in EEPROM
   unsigned int node_addr;
   unsigned int group_addr;
   unsigned int svn_revision;
   char node_name[16];
} SYS_INFO;

#ifndef ENABLE_INTERRUPTS
#define ENABLE_INTERRUPTS { EA = 1; }
#endif
#ifndef DISABLE_INTERRUPTS
#define DISABLE_INTERRUPTS { EA = 0; }
#endif

/*---- function declarations ---------------------------------------*/

void watchdog_refresh(unsigned char from_interrupt) reentrant;
void watchdog_enable(unsigned char watchdog_timeout);
void watchdog_disable(void);
void yield(void);
void led_set(unsigned char led, unsigned char flag) reentrant;
void led_blink(unsigned char led, unsigned char n, int interval) reentrant;
void led_mode(unsigned char led, unsigned char flag) reentrant;
void delay_us(unsigned int us);
void delay_ms(unsigned int ms);

unsigned char crc8(unsigned char *buffer, int len) reentrant;
unsigned char crc8_add(unsigned char crc, unsigned int c);

void lcd_setup();
void lcd_clear();
void lcd_cursor(unsigned char flag);
void lcd_goto(char x, char y);
void lcd_putc(char c);
void lcd_puts(char *str);
char scs_lcd1_read();

char getchar_nowait(void);
unsigned char gets_wait(char *str, unsigned char size, unsigned char timeout);
void flush(void);

void eeprom_flash(void);
unsigned char eeprom_retrieve(unsigned char flag);
void setup_variables(void);

void uart_init(unsigned char port, unsigned char baud);

unsigned char uart1_send(char *buffer, int size, unsigned char bit9);
unsigned char uart1_receive(char *buffer, int size);
void send_remote_var(unsigned char i);
unsigned char ping(unsigned short addr);

void sysclock_init(void);
void sysclock_reset(void);
unsigned long time(void);
unsigned long uptime(void);

unsigned char user_func(unsigned char *data_in, unsigned char *data_out);

void set_n_sub_addr(unsigned char n);
unsigned char cur_sub_addr(void);

void rtc_init(void);
unsigned char rtc_present(void);
void rtc_write(unsigned char d[6]);
void rtc_read(unsigned char d[6]);
void rtc_write_item(unsigned char item, unsigned char d);
void rtc_conv_date(unsigned char d[6], char *str);
void rtc_conv_time(unsigned char d[6], char *str);
void rtc_print(void);




/********************************************************************\

  Name:         mscb.h
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus protocol commands

  $Log$
  Revision 1.21  2003/03/19 16:35:03  midas
  Eliminated configuration parameters

  Revision 1.20  2003/03/14 13:47:54  midas
  Added SCS_310 code

  Revision 1.19  2003/03/06 16:08:50  midas
  Protocol version 1.3 (change node name)

  Revision 1.18  2003/03/06 11:00:17  midas
  Added section for SCS-600

  Revision 1.17  2003/02/19 16:05:13  midas
  Added code for ADuC812

  Revision 1.16  2003/01/30 08:35:30  midas
  Added watchdog switch

  Revision 1.15  2002/11/28 14:44:02  midas
  Removed SIZE_XBIT

  Revision 1.14  2002/11/28 13:03:41  midas
  Protocol version 1.2

  Revision 1.13  2002/11/20 12:02:09  midas
  Fixed bug with secondary LED

  Revision 1.12  2002/11/06 16:45:42  midas
  Revised LED blinking scheme

  Revision 1.11  2002/10/22 15:05:13  midas
  Added UNIT_FACTOR

  Revision 1.10  2002/10/09 15:48:13  midas
  Fixed bug with download

  Revision 1.9  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.8  2002/10/07 15:16:32  midas
  Added upgrade facility

  Revision 1.7  2002/10/04 09:03:20  midas
  Small mods for scs_300

  Revision 1.6  2002/10/03 15:31:53  midas
  Various modifications

  Revision 1.5  2002/08/08 06:45:38  midas
  Added time functions

  Revision 1.4  2002/07/10 09:53:00  midas
  Finished EEPROM routines

  Revision 1.3  2002/07/08 08:50:42  midas
  Added mscbutil functions

  Revision 1.2  2002/07/05 15:27:23  midas
  *** empty log message ***

  Revision 1.1  2002/07/05 08:12:46  midas
  Moved files

\********************************************************************/

/*---- CPU specific items ------------------------------------------*/

#undef LCD_DEBUG  // debug output on LCD

/*--------------------------------*/
#if defined(SCS_210)
#include <c8051F020.h>
#define CPU_C8051F020
#define CPU_CYGNAL

sbit LED =               P3^4;
sbit LED_SEC =           P3^3;
sbit RS485_ENABLE =      P3^5;
#define LED_2
#define LED_ON 0

/*--------------------------------*/
#elif defined(SCS_300) || defined(SCS_310)
#include <c8051F020.h>
#define CPU_C8051F020
#define CPU_CYGNAL

sbit LED =               P3^3;
sbit LED_SEC =           P3^4;
sbit RS485_ENABLE =      P3^5;
#define LED_2
#define LED_ON 0

/*--------------------------------*/
#elif defined(SCS_400) || defined(SCS_500)
#include <c8051F000.h>
#define CPU_C8051F000
#define CPU_CYGNAL

sbit LED =               P3^4;
sbit RS485_ENABLE =      P3^5;
#define LED_ON 1

/*--------------------------------*/
#elif defined(SCS_600) || defined(SCS_700)
#include <c8051F000.h>
#define CPU_C8051F000
#define CPU_CYGNAL

sbit LED =               P3^4;
sbit RS485_ENABLE =      P3^5;
#define LED_ON 0

/*--------------------------------*/
#elif defined(HVR_300)
#include <aduc812.h>
#define CPU_ADUC812

sbit LED =               P3^4;
sbit RS485_ENABLE =      P3^5;
#define LED_ON 1

/*--------------------------------*/
#else
#error Please define SCS_xxx or HVR_xxx in project options
#endif

#define LED_OFF !LED_ON

/* use hardware watchdog of CPU */
#define USE_WATCHDOG

/* map SBUF0 & Co. to SBUF */
#ifndef CPU_C8051F020
#define SCON0    SCON
#define SBUF0    SBUF
#define TI0      TI
#define RI0      RI
#define RB80     RB8
#define ES0      ES
#endif

/*---- MSCB commands -----------------------------------------------*/

#define VERSION 0x14    // version 1.4

/* Version history:

1.0 Initial
1.1 Add unit prefix, implement upgrade command
1.2 Add CMD_ECHO, CMD_READ with multiple channels
1.3 Add "set node name"
1.4 Remove CMD_xxxs_CONF

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
#define CMD_SET_ADDR    0x34
#define CMD_SET_BAUD    0x39

#define CMD_FREEZE      0x41
#define CMD_SYNC        0x49
#define CMD_UPGRADE     0x50
#define CMD_USER        0x58

#define CMD_ECHO        0x61
#define CMD_TOKEN       0x68
#define CMD_SET_FLAGS   0x69

#define CMD_ACK         0x78

#define CMD_WRITE_NA    0x80
#define CMD_WRITE_ACK   0x88

#define CMD_FLASH       0x98

#define CMD_READ        0xA0

#define CMD_WRITE_BLOCK 0xB5
#define CMD_READ_BLOCK  0xB9

#define GET_INFO_GENERAL   0
#define GET_INFO_VARIABLES 1

/*---- flags from the configuration and status register (CSR) ------*/

#define CSR_DEBUG       (1<<0)
#define CSR_LCD_PRESENT (1<<1)
#define CSR_SYNC_MODE   (1<<2)
#define CSR_FREEZE_MODE (1<<3)
#define CSR_WD_RESET    (1<<2)

/*---- baud rates used ---------------------------------------------*/

#define BD_9600            1
#define BD_19200           2
#define BD_28800           3
#define BD_57600           4 
#define BD_115200          5
#define BD_172800          6
#define BD_345600          7

/*---- info structures ---------------------------------------------*/

typedef struct {
  unsigned char  protocol_version;
  unsigned char  node_status;
  unsigned char  n_variables;
  unsigned short node_address;
  unsigned short group_address;
  unsigned short watchdog_resets;
  char           node_name[16];
} MSCB_INFO;

typedef struct {
  unsigned char width;    // width in bytes
  unsigned char unit;     // physical units UNIT_xxxx
  unsigned char prefix;   // unit prefix PRFX_xxx
  unsigned char status;   // status (not yet used)
  unsigned char flags;    // flags MSCBF_xxx
  char          name[8];  // name
  void data     *ud;      // point to user data buffer
} MSCB_INFO_VAR;

#define MSCBF_FLOAT  (1<<0) // channel in floating point format
#define MSCBF_SIGNED (1<<1) // channel is signed integer

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

typedef struct {          // system info stored in EEPROM
unsigned int  node_addr;
unsigned int  group_addr;
unsigned int  wd_counter;
char          node_name[16];
unsigned char magic;
} SYS_INFO;

#define ENABLE_INTERRUPTS { EA = 1; }
#define DISABLE_INTERRUPTS { EA = 0; }

/*---- function declarations ---------------------------------------*/

void yield(void);
void led_blink(int led, int n, int interval) reentrant;
void delay_us(unsigned int us);
void delay_ms(unsigned int ms);

unsigned char crc8(unsigned char *buffer, int len);
unsigned char crc8_add(unsigned char crc, unsigned int c);

void lcd_setup();
void lcd_clear();
void lcd_goto(char x, char y);
void lcd_putc(char c);
void lcd_puts(char *str);
char scs_lcd1_read();

char getchar_nowait(void);
unsigned char gets_wait(char *str, unsigned char size, unsigned char timeout);
void flush(void);

void eeprom_flash(void); 
void eeprom_retrieve(void);

void uart_init(unsigned char port, unsigned char baud);

void sysclock_init(void);
unsigned long time(void);

unsigned char user_func(unsigned char *data_in,
                        unsigned char *data_out);


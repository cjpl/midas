/********************************************************************\

  Name:         mscb.h
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus protocol commands

  $Log$
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

/*--------------------------------*/
#if defined(SCS_210)
#include <c8051F020.h>
#define CPU_C8051F020
#define CPU_CYGNAL

sbit LED =               P3^4;
sbit LED_SEC =           P3^3;
sbit RS485_ENABLE =      P3^5;
#define LED_ON 0

/*--------------------------------*/
#elif defined(SCS_300)
#include <c8051F020.h>
#define CPU_C8051F020
#define CPU_CYGNAL

sbit LED =               P3^3;
sbit LED_SEC =           P3^4;
sbit RS485_ENABLE =      P3^5;
#define LED_ON 0

/*--------------------------------*/
#elif defined(SCS_400) || defined(SCS_500) || defined(SCS_600)
#include <c8051F000.h>
#define CPU_C8051F000
#define CPU_CYGNAL

sbit LED =               P3^4;
sbit RS485_ENABLE =      P3^5;
#define LED_ON 1

/*--------------------------------*/
#else
#error Please define SCD_xxx in project options
#endif

#define LED_OFF !LED_ON

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

#define VERSION 0x10    // version 1.0

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
#define CMD_TRANSP      0x50
#define CMD_USER        0x58

#define CMD_TOKEN       0x60
#define CMD_SET_FLAGS   0x69

#define CMD_ACK         0x78

#define CMD_WRITE_NA    0x80
#define CMD_WRITE_ACK   0x88

#define CMD_WRITE_CONF  0x90
#define CMD_FLASH       0x98

#define CMD_READ        0xA1
#define CMD_READ_CONF   0xA9

#define CMD_WRITE_BLOCK 0xB5
#define CMD_READ_BLOCK  0xB9

#define GET_INFO_GENERAL   0
#define GET_INFO_CHANNEL   1
#define GET_INFO_CONF      2

#define SIZE_0BIT          0
#define SIZE_8BIT          1
#define SIZE_16BIT         2
#define SIZE_24BIT         3
#define SIZE_32BIT         4

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
  unsigned char  n_channel;
  unsigned char  n_conf;
  unsigned short node_address;
  unsigned short group_address;
  unsigned short watchdog_resets;
  char           node_name[16];
} MSCB_INFO;

typedef struct {
  unsigned char width;    // width in bytes
  unsigned char units;    // physical units (not yet used)
  unsigned char status;   // status (not yet used)
  unsigned char flags;    // flags
  char          name[8];  // name
  void data     *ud;      // point to user data buffer
} MSCB_INFO_CHN;

#define MSCBF_FLOAT  (1<<0) // channel in floating point format

typedef struct {          // system info stored in EEPROM
unsigned int  node_addr;
unsigned int  group_addr;
unsigned int  wd_counter;
} SYS_INFO;

#define ENABLE_INTERRUPTS { EA = 1; }
#define DISABLE_INTERRUPTS { EA = 0; }

/*---- function declarations ---------------------------------------*/

void watchdog_refresh(void);
void delay_us(unsigned int us);
void delay_ms(unsigned int ms);

unsigned char crc8(unsigned char idata *buffer, int len);
unsigned char crc8_add(unsigned char crc, unsigned int c);

void lcd_setup();
void lcd_clear();
void lcd_goto(char x, char y);
void lcd_putc(char c);
void lcd_puts(char *str);
char scs_lcd1_read();

char getchar_nowait(void);
void flush(void);

void eeprom_flash(void); 
void eeprom_retrieve(void);

void uart_init(unsigned char port, unsigned char baud);

void sysclock_init(void);
unsigned long time(void);

unsigned char user_func(unsigned char idata *data_in,
                        unsigned char idata *data_out);


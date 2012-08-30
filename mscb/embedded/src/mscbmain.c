/********************************************************************\

  Name:         mscbmain.c
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus protocol main program

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include <stdlib.h>
#include "mscbemb.h"

#ifdef CFG_HAVE_LCD
#include "lcd.h"
#endif

/* GET_INFO attributes */
#define GET_INFO_GENERAL  0
#define GET_INFO_VARIABLE 1

/* Variable attributes */
#define SIZE_8BIT         1
#define SIZE_16BIT        2
#define SIZE_24BIT        3
#define SIZE_32BIT        4

/* Address modes */
#define ADDR_NONE         0
#define ADDR_NODE         1
#define ADDR_GROUP        2
#define ADDR_ALL          3

/*---- functions and data in user part -----------------------------*/

void user_init(unsigned char init);
void user_write(unsigned char index) reentrant;
unsigned char user_read(unsigned char index);
void user_loop(void);

extern MSCB_INFO_VAR *variables;
extern unsigned char idata _n_sub_addr;

extern char code node_name[];

char code svn_rev_main[] = "$Rev$";

/*------------------------------------------------------------------*/

/* funtions in mscbutil.c */

#ifdef CFG_HAVE_LCD
extern bit lcd_present;
#endif

#ifdef CFG_UART1_DEVICE
extern void rs232_output(void);
#endif

/* forward declarations */
void flash_upgrade(void);
void send_remote_var(unsigned char i);

/*------------------------------------------------------------------*/

/* variables in internal RAM (indirect addressing) */

#if defined(CPU_C8051F020) || defined(CPU_C8051F120)
unsigned char xdata in_buf[256], out_buf[64];
#else
unsigned char idata in_buf[20], out_buf[8];
#endif

unsigned char idata i_in, last_i_in, final_i_in, i_out, cmd_len;
unsigned char idata crc_code, addr_mode, n_variables;

/* use absolute value between main program and upgrader */
unsigned char idata _flkey _at_ 0x80;
unsigned char idata n_out _at_ 0x81;

unsigned char idata _cur_sub_addr, _var_size;

#ifdef CFG_UART1_MSCB
unsigned char idata var_to_send = 0xFF;
#endif

SYS_INFO idata sys_info;

#ifdef CFG_HAVE_RTC
/* buffer for setting RTC */
unsigned char xdata rtc_bread[6];
unsigned char xdata rtc_bwrite[6];
bit rtc_set;
#endif

/*------------------------------------------------------------------*/

/* bit variables in internal RAM */

sbit RS485_ENABLE = RS485_EN_PIN; // port pin for RS485 enable

#ifdef EXT_WATCHDOG_PIN
sbit EXT_WATCHDOG = EXT_WATCHDOG_PIN; // port pin for external watchdog
#endif

unsigned char bdata CSR;        // byte address of CSR consisting of bits below 

sbit DEBUG_MODE = CSR ^ 0;      // debugging mode
sbit SYNC_MODE = CSR ^ 1;       // turned on in SYNC mode
sbit FREEZE_MODE = CSR ^ 2;     // turned on in FREEZE mode
sbit WD_RESET = CSR ^ 3;        // got rebooted by watchdog reset

bit addressed;                  // true if node addressed
bit flash_param;                // used for EEPROM flashing
bit flash_program;              // used for upgrading firmware
bit configured_addr;            // TRUE if node address is configured
bit configured_vars;            // TRUE if variables are configured
bit flash_allowed;              // TRUE 5 sec after booting node
bit wrong_cpu;                  // TRUE if code uses xdata and CPU does't have it

/*------------------------------------------------------------------*/

void setup(void)
{
   unsigned char adr, flags, d;
   unsigned short i;
   unsigned char *p;

   _flkey = 0;
   
   /* first disable watchdog */
   watchdog_disable();

   /* avoid any blocking of RS485 bus */
   RS485_ENABLE = RS485_ENABLE_OFF;

   /* Port and oscillator configuration */

#if defined(CPU_C8051F120)

   SFRPAGE   = CONFIG_PAGE;
  
   XBR0 = 0x04;                 // Enable XBar, UART0 & UART1
   XBR1 = 0x00;
   XBR2 = 0x44;

  #ifdef CLK_25MHZ
   /* Select internal quartz oscillator */
   SFRPAGE   = LEGACY_PAGE;
   FLSCL     = 0x00;            // set flash read time for <25 MHz

   SFRPAGE   = CONFIG_PAGE;
   OSCICN    = 0x83;            // divide by 1
   CLKSEL    = 0x00;            // select internal oscillator
  #else          // 98 MHz
   /* Select internal quartz oscillator */
   SFRPAGE   = LEGACY_PAGE;
   FLSCL     = 0xB0;            // set flash read time for 100 MHz

   SFRPAGE   = CONFIG_PAGE;
   OSCICN    = 0x83;            // divide by 1
   CLKSEL    = 0x00;            // select internal oscillator

   PLL0CN    |= 0x01;
   PLL0DIV   = 0x01;
   PLL0FLT   = 0x01;
   PLL0MUL   = 0x04;
   for (i = 0 ; i < 15; i++);   // Wait 5us for initialization
   PLL0CN    |= 0x02;
   for (i = 0 ; i<50000 && ((PLL0CN & 0x10) == 0) ; i++);

   CLKSEL    = 0x02;            // select PLL as sysclk src
  #endif

#elif defined(CPU_C8051F020)

   XBR0 = 0x04;                 // Enable UART0 & UART1
   XBR1 = 0x00;
   XBR2 = 0x44;

   /* Select external quartz oscillator */
   OSCXCN = 0x67;               // Crystal mode, Power Factor 22E6
   OSCICN = 0x08;               // CLKSL=1 (external)

#elif defined(CPU_C8051F310) || defined(CPU_C8051F320)

   XBR0 = 0x01;                 // Enable RX/TX
   XBR1 = 0x40;                 // Enable crossbar

   /* Select internal quartz oscillator */
   OSCICN = 0x83;               // IOSCEN=1, SYSCLK=24.5 MHz
   CLKSEL = 0x00;               // derive SYSCLK from internal source

#else

   XBR0 = 0x04;                 // Enable RX/TX
   XBR1 = 0x00;
   XBR2 = 0x40;                 // Enable crossbar

   PRT0CF = 0x01;               // P0.0: TX = Push Pull
   PRT1CF = 0x00;               // P1
   PRT2CF = 0x00;               // P2  Open drain for 5V LCD
   PRT3CF = 0x20;               // P3.5: RS485 enable = Push Pull

   /* Select external quartz oscillator */
   OSCXCN = 0x67;               // Crystal mode, Power Factor 22E6
   OSCICN = 0x08;               // CLKSL=1 (external)

#endif
        
#ifdef CFG_HAVE_LCD
   lcd_setup();
#endif

#ifdef CFG_HAVE_EMIF
   /* initialize external memory interface */
   d = emif_init();

   /* do memory test on cold start */
   SFRPAGE = LEGACY_PAGE;
   if (d > 0 && (RSTSRC & 0x02) > 0)
      emif_test(d);
#endif

   /* start system clock */
   sysclock_init();

   /* enable watchdog with default timeout */
   watchdog_enable(0);

   /* enable missing clock detector */
   RSTSRC |= 0x04;

   /* default LED mode */
   for (i=0 ; i<N_LED ; i++)
      led_mode(i, 1);
   
   /* initialize all memory */
   CSR = 0;
   addressed = 0;
   flash_param = 0;
   flash_program = 0;
   flash_allowed = 0;
   wrong_cpu = 0;
   _flkey = 0;

#ifdef CFG_HAVE_RTC
   rtc_set = 0;
#endif

   i_in = i_out = n_out = 0;
   _cur_sub_addr = 0;
   for (i=0 ; i<sizeof(in_buf) ; i++)
      in_buf[i] = 0;
   for (i=0 ; i<sizeof(out_buf) ; i++)
      out_buf[i] = 0;

   /* check if we got reset by watchdog */
#if defined(CPU_C8051F120)
   SFRPAGE   = LEGACY_PAGE;
#endif
   WD_RESET = ((RSTSRC & 0x02) == 0 && (RSTSRC & 0x08) > 0);

   /* initialize UART(s) */
   uart_init(0, BD_115200);

#ifdef CFG_UART1_MSCB
   uart_init(1, BD_115200);
#endif

#ifdef CFG_DYN_VARIABLES
   setup_variables();
#endif

   /* count variables */
   for (n_variables = _var_size = 0;; n_variables++) {
      _var_size += variables[n_variables].width;
      if (variables[n_variables].width == 0)
         break;
   }

   /* check if variables are in xdata and xdata is present */
   if (n_variables > 0) {
      p = variables[0].ud;
      d = *p;
      *p = 0x55;
      if (*p != 0x55)
         wrong_cpu = 1;
      *p = 0xAA;
      if (*p != 0xAA)
         wrong_cpu = 1;
      *p = d;
   }

   /* retrieve EEPROM data */
#ifdef CPU_C8051F120
   SFRPAGE = LEGACY_PAGE;
#endif
   if ((RSTSRC & 0x02) > 0)
      flags = eeprom_retrieve(1); // vars on cold start
   else
      flags = eeprom_retrieve(0);

   if ((flags & (1 << 0)) == 0) {
      configured_addr = 0;
   
      /* set initial values */
      sys_info.node_addr = 0xFFFF;
      sys_info.group_addr = 0xFFFF;
      memset(sys_info.node_name, 0, sizeof(sys_info.node_name));
      strncpy(sys_info.node_name, node_name, sizeof(sys_info.node_name));
   } else
      configured_addr = 1;

   /* store SVN revision */
   sys_info.svn_revision = (svn_rev_main[6]-'0')*1000+
                           (svn_rev_main[7]-'0')*100+
                           (svn_rev_main[8]-'0')*10+
                           (svn_rev_main[9]-'0');

   if ((flags & (1 << 1)) == 0) {

      /* init variables */
      for (i = 0; variables[i].width; i++)
         if (!(variables[i].flags & MSCBF_DATALESS)) {
            /* do it for each sub-address */
            for (adr = 0 ; adr < _n_sub_addr ; adr++) {
               memset((char*)variables[i].ud + _var_size*adr, 0, variables[i].width);
            }
         }

      /* call user initialization routine with initialization */
      user_init(1);

      /* write current variables to flash later in main routine */
      configured_vars = 0;
   } else {
      /* call user initialization routine without initialization */
      user_init(0);
      configured_vars = 1;
   }

   /* Blink LEDs */
   for (i=0 ; i<N_LED ; i++)
      led_blink(i, 3, 150);

}

/*------------------------------------------------------------------*/

unsigned char cur_sub_addr()
{
   return _cur_sub_addr;
}

/*------------------------------------------------------------------*\

  Serial interrupt

\*------------------------------------------------------------------*/

void interprete(void);

void serial_int(void) interrupt 4 
{
   if (TI0) {
      /* character has been transferred */

      TI0 = 0;                   // clear TI flag

      i_out++;                   // increment output counter
      if (i_out == n_out) {
         i_out = n_out = 0;      // send buffer empty, clear pointer
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF; // disable RS485 driver
      } else {
         DELAY_US(INTERCHAR_DELAY);
         SBUF0 = out_buf[i_out]; // send character
      }
   }

   if (RI0) {
      /* character has been received */

      if (!RB80 && !addressed) {
         RI0 = 0;
         i_in = 0;
         return;                // discard data if not bit9 and not addressed
      }

      RB80 = 0;
      in_buf[i_in++] = SBUF0;
      RI0 = 0;

      if (i_in == 1) {
         /* check for padding character */
         if (in_buf[0] == 0) {
            i_in = 0;
            return;
         }

         /* initialize command length if first byte */
         cmd_len = (in_buf[0] & 0x07) + 2;    // + cmd + crc
      }

      if (i_in == 2 && cmd_len == 9) {
         /* variable length command */
         cmd_len = in_buf[1] + 3;             // + cmd + N + crc
      }

      if (i_in == sizeof(in_buf)) {   // check for buffer overflow
         i_in = 0;
         return;                      // don't interprete command
      }

      if (i_in < cmd_len)             // return if command not yet complete
         return;

      if (in_buf[i_in - 1] != crc8(in_buf, i_in - 1)) {
         i_in = 0;
         return;                      // return if CRC code does not match
      }

      DELAY_US(INTERCHAR_DELAY);

      interprete();             // interprete command
      i_in = 0;
   }
}

/*------------------------------------------------------------------*\

  Interprete MSCB command

\*------------------------------------------------------------------*/

#pragma NOAREGS

#include <intrins.h>

static void send_byte(unsigned char d, unsigned char *crc)
{
#ifdef CPU_C8051F120
   SFRPAGE = UART0_PAGE;
#endif

   if (crc)
      *crc = crc8_add(*crc, d);
   DELAY_US(INTERCHAR_DELAY);
   SBUF0 = d;
   watchdog_refresh(1);
   while (!TI0);
   TI0 = 0;
}

static void send_obuf(unsigned char n)
{
#ifdef CPU_C8051F120
   SFRPAGE = UART0_PAGE;
#endif

   n_out = n;
   RS485_ENABLE = RS485_ENABLE_ON;
   DELAY_US(INTERCHAR_DELAY);
   SBUF0 = out_buf[0];
}

void addr_node8(unsigned char mode, unsigned char adr, unsigned char node_addr)
{
   if (mode == ADDR_NODE) {
      if (adr >= node_addr &&
          adr <  node_addr + _n_sub_addr) {

         addressed = 1;
         _cur_sub_addr = adr - node_addr;
         addr_mode = ADDR_NODE;
      } else {
         addressed = 0;
         addr_mode = ADDR_NONE;
      }
   } else if (mode == ADDR_GROUP) {
      if (adr == node_addr) {
         addressed = 1;
         _cur_sub_addr = 0;
         addr_mode = ADDR_GROUP;
      } else {
         addressed = 0;
         addr_mode = ADDR_NONE;
      }
   }
}

void addr_node16(unsigned char mode, unsigned int adr, unsigned int node_addr)
{
   if (node_addr == 0xFFFF) {
      if (adr == node_addr) {
         addressed = 1;
         _cur_sub_addr = 0;
         addr_mode = mode;
      } else {
         addressed = 0;
         addr_mode = ADDR_NONE;
      }
   } else {
      if (mode == ADDR_NODE) {
         if (adr >= node_addr &&
             adr <  node_addr + _n_sub_addr) {
   
            addressed = 1;
            _cur_sub_addr = adr - node_addr;
            addr_mode = ADDR_NODE;
         } else {
            addressed = 0;
            addr_mode = ADDR_NONE;
         }
      } else if (mode == ADDR_GROUP) {
         if (adr == node_addr) {
            addressed = 1;
            _cur_sub_addr = 0;
            addr_mode = ADDR_GROUP;
         } else {
            addressed = 0;
            addr_mode = ADDR_NONE;
         }
      }
   }
}

void interprete(void) 
{
   unsigned char crc, cmd, i, j, n, ch, ch1, ch2, a1, a2;
   unsigned short size;
   MSCB_INFO_VAR *pvar;
   unsigned long idata u;

   cmd = (in_buf[0] & 0xF8);    // strip length field

#ifdef CPU_C8051F120
   SFRPAGE = UART0_PAGE;        // needed for SBUF0
#endif

   switch (in_buf[0]) {
   case CMD_ADDR_NODE8:
      addr_node8(ADDR_NODE, in_buf[1], sys_info.node_addr & 0xFF);
      break;

   case CMD_ADDR_NODE16:
      addr_node16(ADDR_NODE, *(unsigned int *) &in_buf[1], sys_info.node_addr);
      break;

   case CMD_ADDR_BC:
      addressed = 1;
      addr_mode = ADDR_ALL;
      break;

   case CMD_ADDR_GRP8:
      addr_node8(ADDR_GROUP, in_buf[1], sys_info.group_addr & 0xFF);
      break;

   case CMD_ADDR_GRP16:
      addr_node16(ADDR_GROUP, *(unsigned int *) &in_buf[1], sys_info.group_addr);
      break;

   case CMD_PING8:
      addr_node8(ADDR_NODE, in_buf[1], sys_info.node_addr & 0xFF);
      if (addressed) {
         out_buf[0] = CMD_ACK;
         send_obuf(1);
      }
      break;

   case CMD_PING16:
      addr_node16(ADDR_NODE, *(unsigned int *) &in_buf[1], sys_info.node_addr);
      if (addressed) {
         out_buf[0] = CMD_ACK;
         send_obuf(1);
      }
      break;

   case CMD_INIT:
#ifdef CPU_C8051F120
      SFRPAGE = LEGACY_PAGE;
#endif

      RSTSRC = 0x10;         // force software reset
      break;

   case CMD_GET_INFO:
      /* general info */

      ES0 = 0;                  // temporarily disable serial interrupt
      crc = 0;
      RS485_ENABLE = RS485_ENABLE_ON;

      send_byte(CMD_ACK + 7, &crc);      // send acknowledge, variable data length
#ifdef CFG_HAVE_RTC
      send_byte(30, &crc);               // send data length
#else
      send_byte(24, &crc);               // send data length
#endif
      send_byte(PROTOCOL_VERSION, &crc); // send protocol version

      send_byte(n_variables, &crc);      // send number of variables

      send_byte(*(((unsigned char *) &sys_info.node_addr) + 0), &crc);  // send node address
      send_byte(*(((unsigned char *) &sys_info.node_addr) + 1), &crc);

      send_byte(*(((unsigned char *) &sys_info.group_addr) + 0), &crc); // send group address
      send_byte(*(((unsigned char *) &sys_info.group_addr) + 1), &crc);

      send_byte(*(((unsigned char *) &sys_info.svn_revision) + 0), &crc);   // send svn revision
      send_byte(*(((unsigned char *) &sys_info.svn_revision) + 1), &crc);

      for (i = 0; i < 16; i++)  // send node name
         send_byte(sys_info.node_name[i], &crc);

#ifdef CFG_HAVE_RTC
      for (i = 0; i < 6 ; i++)
         send_byte(rtc_bread[i], &crc);
#endif

      send_byte(crc, NULL);     // send CRC code

      DELAY_US(10);
      RS485_ENABLE = RS485_ENABLE_OFF;
      ES0 = 1;                  // re-enable serial interrupts
      break;

   case CMD_GET_INFO + 1:
      /* send variable info */

      if (in_buf[1] < n_variables) {
         pvar = variables + in_buf[1];
                                  
         ES0 = 0;                       // temporarily disable serial interrupt
         crc = 0;
         RS485_ENABLE = RS485_ENABLE_ON;

         send_byte(CMD_ACK + 7, &crc);  // send acknowledge, variable data length
         send_byte(13, &crc);           // send data length
         send_byte(pvar->width, &crc);
         send_byte(pvar->unit, &crc);
         send_byte(pvar->prefix, &crc);
         send_byte(pvar->status, &crc);
         send_byte(pvar->flags, &crc);

         for (i = 0; i < 8; i++)        // send variable name
            send_byte(pvar->name[i], &crc);

         send_byte(crc, NULL);          // send CRC code

         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;
         ES0 = 1;                       // re-enable serial interrupts
      } else {
         /* just send dummy ack */
         out_buf[0] = CMD_ACK;
         out_buf[1] = 0;
         send_obuf(2);
      }

      break;

   case CMD_GET_UPTIME:
      /* send uptime */

      u = uptime();

      out_buf[0] = CMD_ACK + 4;
      out_buf[1] = *(((unsigned char *)&u) + 0);
      out_buf[2] = *(((unsigned char *)&u) + 1);
      out_buf[3] = *(((unsigned char *)&u) + 2);
      out_buf[4] = *(((unsigned char *)&u) + 3);
      out_buf[5] = crc8(out_buf, 5);
      send_obuf(6);
      break;

   case CMD_SET_ADDR:

      if (in_buf[1] == ADDR_SET_NODE) 
         /* complete node address */
         sys_info.node_addr = *((unsigned int *) (in_buf + 2));
      else if (in_buf[1] == ADDR_SET_HIGH)
         /* only high byte node address */
         *((unsigned char *)(&sys_info.node_addr)) = *((unsigned char *) (in_buf + 2));
      else if (in_buf[1] == ADDR_SET_GROUP)
         /* group address */
         sys_info.group_addr = *((unsigned int *) (in_buf + 2));

      /* copy address to EEPROM */
      flash_param = 1;
      _flkey = 0xF1;

      break;

   case CMD_SET_NAME:
      /* set node name in RAM */
      for (i = 0; i < 16 && i < in_buf[1]; i++)
         sys_info.node_name[i] = in_buf[2 + i];
      sys_info.node_name[15] = 0;

      /* copy address to EEPROM */
      flash_param = 1;
      _flkey = 0xF1;

      break;

   case CMD_SET_BAUD:
      led_blink(_cur_sub_addr, 1, 50);
      uart_init(0, in_buf[1]);
      break;

   case CMD_FREEZE:
      FREEZE_MODE = in_buf[1];
      break;

   case CMD_SYNC:
      SYNC_MODE = in_buf[1];
      break;

   case CMD_SET_TIME:
#ifdef CFG_HAVE_RTC
      led_blink(0, 1, 50);
      for (i=0 ; i<6 ; i++)
         rtc_bwrite[i] = in_buf[i+1];
      rtc_set = 1;
#endif
      break;

   case CMD_UPGRADE:
      if (_cur_sub_addr != 0)
         n = 2; // reject upgrade for sub address
      else if (flash_allowed == 0)
         n = 3; // upgrade not yet allowed
      else
         n = 1; // positive acknowledge

      out_buf[0] = CMD_ACK + 1;
      out_buf[1] = n;
      out_buf[2] = crc8(out_buf, 2);
      send_obuf(3);

      if (n == 1) {
         flash_program = 1;
         _flkey = 0xF1;
      }
      break;

   case CMD_FLASH:
      flash_param = 1;
      _flkey = 0xF1;
      break;

   case CMD_ECHO:
      led_blink(0, 1, 50);
      out_buf[0] = CMD_ACK + 1;
      out_buf[1] = in_buf[1];
      out_buf[2] = crc8(out_buf, 2);
      send_obuf(3);
      break;

   }

   if (cmd == CMD_READ) {
      if (in_buf[0] == CMD_READ + 1) {  // single variable
         if (in_buf[1] < n_variables) {
            n = variables[in_buf[1]].width;     // number of bytes to return

            if (variables[in_buf[1]].flags & MSCBF_DATALESS) {
               n = user_read(in_buf[1]);        // for dataless variables, user routine returns bytes
               out_buf[0] = CMD_ACK + 7;        // and places data directly in out_buf
               out_buf[1] = n;

               out_buf[2 + n] = crc8(out_buf, 2 + n);      // generate CRC code
   
               /* send result */
               send_obuf(3 + n);

            } else {

               user_read(in_buf[1]);

               ES0 = 0;            // temporarily disable serial interrupt
               crc = 0;
               RS485_ENABLE = RS485_ENABLE_ON;
   
               if (n > 6) {
                  /* variable length buffer */
                  send_byte(CMD_ACK + 7, &crc);       // send acknowledge, variable data length
                  send_byte(n, &crc);                 // send data length

                  for (i = 0; i < n; i++)             // copy user data
                     send_byte(((char *) variables[in_buf[1]].ud)[i+_var_size*_cur_sub_addr], &crc);
                  n++;
               } else {

                  send_byte(CMD_ACK + n, &crc);       // send acknowledge
               
                  for (i = 0; i < n; i++)             // copy user data
                     send_byte(((char *) variables[in_buf[1]].ud)[i+_var_size*_cur_sub_addr], &crc);
               }

               send_byte(crc, NULL);                  // send CRC code
   
               DELAY_US(10);
               RS485_ENABLE = RS485_ENABLE_OFF;
               ES0 = 1;            // re-enable serial interrupts
            }
         } else {
            /* just send dummy ack to indicate error */
            out_buf[0] = CMD_ACK;
            send_obuf(1);
         }  

      } else if (in_buf[0] == CMD_READ + 2) {   // variable range

        if (in_buf[1] < n_variables && in_buf[2] < n_variables && in_buf[1] <= in_buf[2]) {
            /* calculate number of bytes to return */
            for (i = in_buf[1], size = 0; i <= in_buf[2]; i++) {
               user_read(i);
               size += variables[i].width;
            }

            ES0 = 0;            // temporarily disable serial interrupt
            crc = 0;
            RS485_ENABLE = RS485_ENABLE_ON;

            send_byte(CMD_ACK + 7, &crc);            // send acknowledge, variable data length
            if (size < 0x80)
               send_byte(size, &crc);                // send data length one byte
            else {
               send_byte(0x80 | size / 0x100, &crc); // send data length two bytes
               send_byte(size & 0xFF, &crc);
            }

            /* loop over all variables */
            for (i = in_buf[1]; i <= in_buf[2]; i++) {
               for (j = 0; j < variables[i].width; j++)    // send user data
                  send_byte(((char *) variables[i].ud)[j+_var_size*_cur_sub_addr], &crc); 
            }

            send_byte(crc, NULL);       // send CRC code

            DELAY_US(10);
            RS485_ENABLE = RS485_ENABLE_OFF;
            ES0 = 1;            // re-enable serial interrupts
         } else {
            /* just send dummy ack to indicate error */
            out_buf[0] = CMD_ACK;
            send_obuf(1);
         }
      }
   }

   if (cmd == CMD_USER) {
      led_blink(_cur_sub_addr, 1, 50);
      n = user_func(in_buf + 1, out_buf + 1);
      out_buf[0] = CMD_ACK + n;
      out_buf[n + 1] = crc8(out_buf, n + 1);
      send_obuf(n+2);
   }

   if (cmd == CMD_WRITE_NA || cmd == CMD_WRITE_ACK) {

      /* blink LED once when writing data */
      if (addr_mode == ADDR_NODE)
         led_blink(_cur_sub_addr, 1, 50);
      else 
         for (i=0 ; i<_n_sub_addr ; i++)
            led_blink(i, 1, 50);

      n = in_buf[0] & 0x07;

      if (n == 0x07) {  // variable length
         j = 1;
         n = in_buf[1];
         ch = in_buf[2];
      } else {
         j = 0;
         ch = in_buf[1];
      }

      n--; // data size (minus channel)

      if (ch < n_variables) {
   
         /* don't exceed variable width */
         if (n > variables[ch].width)
            n = variables[ch].width;

         if (addr_mode == ADDR_NODE)
            a1 = a2 = _cur_sub_addr;
         else {
            a1 = 0;
            a2 = _n_sub_addr-1;
         }
            
         for (_cur_sub_addr = a1 ; _cur_sub_addr <= a2 ; _cur_sub_addr++) {
            for (i = 0; i < n; i++)
               if (!(variables[ch].flags & MSCBF_DATALESS)) {
                  if (variables[ch].unit == UNIT_STRING) {
                     if (n > 4)
                        /* copy bytes in normal order */
                        ((char *) variables[ch].ud)[i + _var_size*_cur_sub_addr] = 
                           in_buf[2 + j + i];
                     else
                        /* copy bytes in reverse order (got swapped on host) */
                        ((char *) variables[ch].ud)[i + _var_size*_cur_sub_addr] = 
                           in_buf[i_in - 2 - i];
                  } else
                     /* copy LSB bytes, needed for BYTE if DWORD is sent */
                     ((char *) variables[ch].ud)[i + _var_size*_cur_sub_addr] = 
                           in_buf[i_in - 1 - variables[ch].width + i + j];
               }
   
            user_write(ch);
         }
         _cur_sub_addr = a1; // restore previous value
   
#ifdef CFG_UART1_MSCB
         /* mark variable to be send in main loop */
         if (variables[ch].flags & MSCBF_REMOUT)
            var_to_send = ch;
#endif

         if (cmd == CMD_WRITE_ACK) {
            out_buf[0] = CMD_ACK;
            out_buf[1] = in_buf[i_in - 1];
            send_obuf(2);
         }
      } else if (ch == 0xFF) {
         CSR = in_buf[2];

         if (cmd == CMD_WRITE_ACK) {
            out_buf[0] = CMD_ACK;
            out_buf[1] = in_buf[i_in - 1];
            send_obuf(2);
         }
      }
   }


   if (cmd == CMD_WRITE_RANGE) {

      /* blink LED once when writing data */
      if (addr_mode == ADDR_NODE)
         led_blink(_cur_sub_addr, 1, 50);
      else 
         for (i=0 ; i<_n_sub_addr ; i++)
            led_blink(i, 1, 50);

      if (in_buf[1] & 0x80) {
         size = (in_buf[1] & 0x7F) << 8 | in_buf[2];
         j = 3;
      } else {
         size = in_buf[1];
         j = 2;
      }

      ch1 = in_buf[j];
      ch2 = in_buf[j+1];
      for (ch = ch1 ; ch <= ch2 ; ch++) {
         if (ch < n_variables) {
      
            n = variables[ch].width;

            if (addr_mode == ADDR_NODE)
               a1 = a2 = _cur_sub_addr;
            else {
               a1 = 0;
               a2 = _n_sub_addr-1;
            }
               
            for (_cur_sub_addr = a1 ; _cur_sub_addr <= a2 ; _cur_sub_addr++) {
               for (i = 0; i < n; i++)
                  if (!(variables[ch].flags & MSCBF_DATALESS)) {
                     ((char *) variables[ch].ud)[i + _var_size*_cur_sub_addr] = 
                        in_buf[2 + j + i];
                  }
      
               user_write(ch);
            }
            _cur_sub_addr = a1; // restore previous value
            j += n;
         }
      }
      out_buf[0] = CMD_ACK;
      out_buf[1] = in_buf[i_in - 1];
      send_obuf(2);
   }

}

/*------------------------------------------------------------------*/

#ifdef CFG_UART1_MSCB

static unsigned short xdata last_addr = -1;
static unsigned char xdata uart1_buf[10];

/*------------------------------------------------------------------*/

void address_node(unsigned short addr)
{
   if (addr != last_addr) {
      uart1_buf[0] = CMD_ADDR_NODE16;
      uart1_buf[1] = (unsigned char) (addr >> 8);
      uart1_buf[2] = (unsigned char) (addr & 0xFF);
      uart1_buf[3] = crc8(uart1_buf, 3);
      uart1_send(uart1_buf, 4, 1);
      last_addr = addr;
   }
}

/*------------------------------------------------------------------*/

unsigned char ping(unsigned short addr)
{
   unsigned char n;

   uart1_buf[0] = CMD_PING16;
   uart1_buf[1] = (unsigned char) (addr >> 8);
   uart1_buf[2] = (unsigned char) (addr & 0xFF);
   uart1_buf[3] = crc8(uart1_buf, 3);
   uart1_send(uart1_buf, 4, 1);

   n = uart1_receive(uart1_buf, 10);
   if (n == 0)
      return 0; // no response

   if (uart1_buf[0] != CMD_ACK)
      return 0; // invalid response

   return 1; // node resonded
}

/*------------------------------------------------------------------*/

void poll_error(unsigned char i)
{
   if (i);
   led_blink(1, 1, 50);
   last_addr = -1; // force re-adressing of single node

   /* flush input queue of remote device */
   for (i=0 ; i<10 ; i++)
      uart1_buf[i] = 0;
   uart1_send(uart1_buf, 10, 1);
}

void poll_remote_vars()
{
unsigned char i, n;

   for (i=0 ; i<n_variables ; i++)
      if (variables[i].flags & MSCBF_REMIN) {
         
         address_node(variables[i].node_address);

         /* read variable */
         uart1_buf[0] = CMD_READ + 1;
         uart1_buf[1] = variables[i].channel;
         uart1_buf[2] = crc8(uart1_buf, 2);
         uart1_send(uart1_buf, 3, 0);

         n = uart1_receive(uart1_buf, 100);

         if (n<2) {
            poll_error(i);
            continue; // no bytes receive
         }

         if (uart1_buf[0] != CMD_ACK + n - 2) {
            poll_error(i);
            continue; // invalid command received
         }

         if (variables[i].width != n - 2) {
            poll_error(i);
            continue; // variables has wrong length
         }

         if (uart1_buf[n-1] != crc8(uart1_buf, n-1)) {
            poll_error(i);
            continue; // invalid CRC
         }

         /* all ok, so copy variable */
         DISABLE_INTERRUPTS;
         memcpy(variables[i].ud, uart1_buf+1, variables[i].width);
         ENABLE_INTERRUPTS;
      }

}

/*------------------------------------------------------------------*/

void send_remote_var(unsigned char i)
{
unsigned char size;

   address_node(variables[i].node_address);

   /* send variable */
   size = variables[i].width;
   uart1_buf[0] = CMD_WRITE_NA + size + 1;
   uart1_buf[1] = variables[i].channel;
   memcpy(uart1_buf+2, variables[i].ud, size);
   uart1_buf[2+size] = crc8(uart1_buf, 2+size);
   uart1_send(uart1_buf, 3+size, 0);
}

/*------------------------------------------------------------------*/

unsigned long xdata last_poll = 0;

void manage_remote_vars()
{
   /* read remote variables once every 10 ms */
   if (time() > last_poll+10) {
      poll_remote_vars();
      last_poll = time();
   }

   /* send remote variables if changed */
   if (var_to_send != 0xFF) {
      send_remote_var(var_to_send);
      var_to_send = 0xFF;
   }
}

#endif // UART1_MSCB

/*------------------------------------------------------------------*/

#ifdef LED_0
sbit led_0 = LED_0;
#endif

#define SEND_BYTE(_b) \
   TI0 = 0; \
   DELAY_US(INTERCHAR_DELAY); \
   SBUF0 = _b; \
   while (TI0 == 0);

#pragma OT(8, SIZE) // 9 would call subroutines in program body -> crash on upgrade

void upgrade()
{
   unsigned char idata cmd, page, crc, j, k;
   unsigned short idata i;
   unsigned char xdata * idata pw;
   unsigned char code * idata pr;

   if (_flkey != 0xF1)
      return;

   /* wait for acknowledge to be sent */
   for (i=0 ; i<10000 ; i++) {
      if (n_out == 0)
         break;
      DELAY_US(10);
   }

   /* disable all interrupts */
   EA = 0;
#if defined(CPU_C8051F120)
   SFRPAGE = UART1_PAGE;
   SCON1 &= ~0x03; // clear pending UART1 interrupts
#endif

   /* disable watchdog */
#if defined(CPU_C8051F310) || defined(CPU_C8051F320)
   PCA0MD = 0x00;
#else
   WDTCN = 0xDE;
   WDTCN = 0xAD;
#endif

   cmd = page = 0;

   do {

receive_cmd:

#ifdef CPU_C8051F120
      SFRPAGE = UART0_PAGE;
#endif

      /* receive command */
      while (!RI0) {
         for (i=0 ; !RI0 && i<5000 ; i++)
            DELAY_US(10);
         led_0 = !led_0;
#if defined(CFG_EXT_WATCHDOG) && defined(EXT_WATCHDOG_PIN)
         EXT_WATCHDOG = !EXT_WATCHDOG;
#endif
      }

      cmd = SBUF0;
      RI0 = 0;

#if defined(CFG_EXT_WATCHDOG) && defined(EXT_WATCHDOG_PIN)
      EXT_WATCHDOG = !EXT_WATCHDOG;
#endif

      /* cannot use case since it calls the C library */

      if (cmd == CMD_PING16) {

         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         page = SBUF0; // LSB
         RI0 = 0;
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         page = SBUF0; // MSB
         RI0 = 0;
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         page = SBUF0; // CRC
         RI0 = 0;

         /* acknowledge ping, independent of own address */
         RS485_ENABLE = RS485_ENABLE_ON;
         SEND_BYTE(CMD_ACK);
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;

      } else if (cmd == CMD_UPGRADE) {

         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         page = SBUF0; // CRC
         RI0 = 0;

         /* acknowledge upgrade */
         RS485_ENABLE = RS485_ENABLE_ON;
         SEND_BYTE(CMD_ACK+1);
         SEND_BYTE(1);
         SEND_BYTE(0); // dummy CRC
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;

      } else if (cmd == UCMD_ECHO) {

         RS485_ENABLE = RS485_ENABLE_ON;
         SEND_BYTE(CMD_ACK);
         SEND_BYTE(0); // dummy CRC, needed by subm_250
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;

      } else if (cmd == UCMD_ERASE) {

         /* receive page */
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;

         page = SBUF0;
         RI0 = 0;
         crc = 0;

         led_0 = !(page & 1);

         /* erase page if not page of upgrade() function */
         if (page*512 < (unsigned int)upgrade && page*512 < EEPROM_OFFSET) {

#ifdef CPU_C8051F120
            /* for F120, only erase even pages (1024kB page size!) */
            if (page & 1)
               goto erase_ok;

            SFRPAGE = LEGACY_PAGE;
#endif

#if defined(CPU_C8051F000)
            FLSCL = (FLSCL & 0xF0) | 0x08; // set timer for 11.052 MHz clock
#elif defined (CPU_C8051F020) || defined(CPU_C8051F120)
            FLSCL = FLSCL | 1;     // enable flash writes
#endif
            PSCTL = 0x03;          // allow write and erase
   
            pw = (char xdata *) (512 * page);
   
#if defined(CPU_C8051F310) || defined (CPU_C8051F320)
            FLKEY = 0xA5;          // write flash key code
            FLKEY = _flkey;
#endif
            
            *pw = 0;
   
#if !defined(CPU_C8051F310) && !defined(CPU_C8051F320)
            FLSCL = (FLSCL & 0xF0);
#endif
            PSCTL = 0x00;

         } else {
            crc = 0xFF;            // return 'protected' flag
         }

#ifdef CPU_C8051F120
         SFRPAGE = UART0_PAGE;
#endif

#ifdef CPU_C8051F120
erase_ok:
#endif
         /* return acknowledge */
         RS485_ENABLE = RS485_ENABLE_ON;
         SEND_BYTE(CMD_ACK);
         SEND_BYTE(crc);
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;

      } else if (cmd == UCMD_PROGRAM) {

         /* receive page */
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         page = SBUF0;
         RI0 = 0;

         /* receive subpage */
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         j = SBUF0;
         RI0 = 0;

         led_0 = page & 1;

         /* program page if not page of upgrade() function */
         if (page*512 >= (unsigned int)upgrade || page*512 >= EEPROM_OFFSET)
            goto receive_cmd;

#ifdef CPU_C8051F120
         SFRPAGE = LEGACY_PAGE;
#endif

         /* allow write */
#if defined(CPU_C8051F000)
         FLSCL = (FLSCL & 0xF0) | 0x08; // set timer for 11.052 MHz clock
#elif defined (CPU_C8051F020) || defined(CPU_C8051F120)
         FLSCL = FLSCL | 1;        // enable flash writes
#endif
         PSCTL = 0x01;             // allow write access

         pw = (char xdata *) (page*512 + j*32);

#ifdef CPU_C8051F120
         SFRPAGE = UART0_PAGE;
#endif

         /* receive 32 bytes */
         for (k = 0; k < 32; k++) {
            for (i=0 ; !RI0 && i < 5000 ; i++)
               DELAY_US(10);
            if (!RI0) 
               goto receive_cmd;

#if defined(CPU_C8051F310) || defined (CPU_C8051F320)
            FLKEY = 0xA5;          // write flash key code
            FLKEY = _flkey;
#endif
            /* flash byte */
            *pw++ = SBUF0;
            RI0 = 0;
         }

#ifdef CPU_C8051F120
         SFRPAGE = LEGACY_PAGE;
#endif

         /* disable write */
#if !defined(CPU_C8051F310) && !defined(CPU_C8051F320)
         FLSCL = (FLSCL & 0xF0);
#endif
         PSCTL = 0x00;

#ifdef CPU_C8051F120
         SFRPAGE = UART0_PAGE;
#endif

         RS485_ENABLE = RS485_ENABLE_ON;
         SEND_BYTE(CMD_ACK);
         SEND_BYTE(0);
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;

      } else if (cmd == UCMD_VERIFY) {

         /* receive page */
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;

         page = SBUF0;
         RI0 = 0;

         pr = 512 * page;

         /* return simplified CRC */
         for (i = crc = 0; i < 512; i++)
            crc += *pr++;

         /* return acknowledge */
         RS485_ENABLE = RS485_ENABLE_ON;
         SEND_BYTE(CMD_ACK);
         SEND_BYTE(crc);
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;

      } else if (cmd == UCMD_READ) {

         /* receive page */
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         page = SBUF0;
         RI0 = 0;

         /* receive subpage */
         for (i=0 ; !RI0 && i < 5000 ; i++)
            DELAY_US(10);
         if (!RI0) 
            goto receive_cmd;
         j = SBUF0;
         RI0 = 0;

         RS485_ENABLE = RS485_ENABLE_ON;

         SEND_BYTE(CMD_ACK+7);     // send acknowledge, variable data length
         SEND_BYTE(32);            // send data length

         pr = (512 * page + 32 * j);

         /* send 32 bytes */
         for (k = crc = 0 ; k<32 ; k++) {
            SEND_BYTE(*pr);
            crc += *pr++;
         }

         SEND_BYTE(crc);
         DELAY_US(10);
         RS485_ENABLE = RS485_ENABLE_OFF;

      } else if (cmd == UCMD_REBOOT) {

#ifdef CPU_C8051F120
         SFRPAGE = LEGACY_PAGE;
#endif
         RSTSRC = 0x10;
      }

   } while (cmd != UCMD_RETURN);


   _flkey = 0;
   EA = 1;                      // re-enable interrupts
}

/* block remainder of segment for linker */
// unsigned char code BLOCK_F000_REMAINDER[0x9F2] _at_ 0xF60D; // for small model
unsigned char code BLOCK_F000_REMAINDER[0x7AC] _at_ 0xF853; // for large model

/*------------------------------------------------------------------*\

  Yield should be called periodically by applications with long loops
  to insure proper watchdog refresh and other functions

\*------------------------------------------------------------------*/

#ifdef CFG_HAVE_RTC
unsigned long xdata rtc_last;   
#endif

void yield(void)
{
   watchdog_refresh(0);

   /* output RS232 data if present */
#ifdef CFG_UART1_DEVICE
   rs232_output();
#endif

   /* blink LED if not configured */
   if (!configured_addr)
      led_blink(0, 1, 50);

   /* blink LED if wrong CPU */
   if (wrong_cpu)
      led_blink(0, 1, 30);

   /* flash EEPROM if asked by interrupt routine, wait 3 sec
      after reboot (power might not be stable) */
   if (flash_param && flash_allowed) {
      led_blink(_cur_sub_addr, 1, 50);

      flash_param = 0;

      eeprom_flash(); 
      configured_addr = 1;
   }

   /* flash EEPROM if variables just got initialized */
   if (!configured_vars && flash_allowed) {
      _flkey = 0xF1;
      eeprom_flash();
      configured_vars = 1;
   }

   if (flash_program && flash_allowed) {
      flash_program = 0;

#ifdef CFG_HAVE_LCD
      lcd_clear();
      lcd_goto(0, 0);
      puts("    Upgrading"); 
      lcd_goto(0, 1);
      puts("    Firmware...");
#endif

      /* go to "bootloader" program */
      upgrade();
   }

#ifdef CFG_HAVE_RTC
   if (rtc_set) {
      rtc_write(rtc_bwrite);
      rtc_set = 0;
   }

   if (time() > rtc_last+90 || time() < rtc_last) {
      rtc_last = time();
      rtc_read(rtc_bread);
   }
#endif

   /* allow flash 3 sec after reboot */
   if (!flash_allowed && time() > 300)
      flash_allowed = 1;

}

/*------------------------------------------------------------------*\

  Main loop

\*------------------------------------------------------------------*/

void main(void)
{
   setup();

   do {
      yield();
      
#ifdef CFG_UART1_MSCB
      manage_remote_vars();
#endif
      
      user_loop();

   } while (1);

}

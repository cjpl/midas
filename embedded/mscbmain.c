/********************************************************************\

  Name:         mscbmain.c
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus protocol main program

  $Log$
  Revision 1.23  2003/02/19 16:04:50  midas
  Added code for ADuC812

  Revision 1.22  2003/01/16 16:34:07  midas
  Do not use printf() for scs_210/300

  Revision 1.21  2002/11/29 08:02:52  midas
  Fixed linux warnings

  Revision 1.20  2002/11/28 14:44:02  midas
  Removed SIZE_XBIT

  Revision 1.19  2002/11/28 13:03:41  midas
  Protocol version 1.2

  Revision 1.18  2002/11/22 15:43:03  midas
  Made user_write reentrant

  Revision 1.17  2002/11/20 12:02:25  midas
  Fixed bug with secondary LED

  Revision 1.16  2002/11/06 16:45:42  midas
  Revised LED blinking scheme

  Revision 1.15  2002/10/16 15:24:12  midas
  Fixed bug for reboot

  Revision 1.14  2002/10/09 15:48:13  midas
  Fixed bug with download

  Revision 1.13  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.12  2002/10/07 15:16:32  midas
  Added upgrade facility

  Revision 1.11  2002/10/04 09:03:20  midas
  Small mods for scs_300

  Revision 1.10  2002/10/03 15:31:53  midas
  Various modifications

  Revision 1.9  2002/08/12 12:12:28  midas
  Added zero padding

  Revision 1.8  2002/08/08 06:46:15  midas
  Added time functions

  Revision 1.7  2002/07/12 15:19:01  midas
  Call user_init before blinking

  Revision 1.6  2002/07/12 08:38:13  midas
  Fixed LCD recognition

  Revision 1.5  2002/07/10 09:52:55  midas
  Finished EEPROM routines

  Revision 1.4  2002/07/09 15:31:32  midas
  Initial Revision

  Revision 1.3  2002/07/08 08:51:39  midas
  Test eeprom functions

  Revision 1.2  2002/07/05 15:27:28  midas
  *** empty log message ***

  Revision 1.1  2002/07/05 08:12:46  midas
  Moved files

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include "mscb.h"

/* GET_INFO attributes */
#define GET_INFO_GENERAL 0
#define GET_INFO_CHANNEL 1
#define GET_INFO_CONF    2

/* Channel attributes */
#define SIZE_8BIT        1
#define SIZE_16BIT       2
#define SIZE_24BIT       3
#define SIZE_32BIT       4

/* Address modes */
#define ADDR_NONE        0
#define ADDR_NODE        1
#define ADDR_GROUP       2
#define ADDR_ALL         3

/*---- functions and data in user part -----------------------------*/

void user_init(unsigned char init);
void user_write(unsigned char channel) reentrant;
unsigned char user_read(unsigned char channel);
void user_write_conf(unsigned char channel) reentrant;
void user_read_conf(unsigned char channel);
void user_loop(void);

extern MSCB_INFO_CHN code channel[];
extern MSCB_INFO_CHN code conf_param[];

extern char code node_name[];

/*------------------------------------------------------------------*/

/* funtions in mscbutil.c */
extern void rs232_output(void);
extern bit lcd_present;
extern void watchdog_refresh(void);

/* forward declarations */
void flash_upgrade(void);

/*------------------------------------------------------------------*/


/* variables in internal RAM (indirect addressing) */

unsigned char idata in_buf[10], out_buf[8];

unsigned char idata i_in, last_i_in, final_i_in, n_out, i_out, cmd_len;
unsigned char idata crc_code, addr_mode, n_channel, n_param;

SYS_INFO sys_info;

/*------------------------------------------------------------------*/

/* bit variables in internal RAM */

unsigned char bdata CSR;         // byte address of CSR consisting of bits below 

sbit DEBUG_MODE     = CSR ^ 0;   // debugging mode
sbit SYNC_MODE      = CSR ^ 1;   // turned on in SYNC mode
sbit FREEZE_MODE    = CSR ^ 2;   // turned on in FREEZE mode
sbit WD_RESET       = CSR ^ 3;   // got rebooted by watchdog reset

bit addressed;                   // true if node addressed
bit new_address, debug_new_i, debug_new_o;    // used for LCD debug output
bit flash_param;                 // used for EEPROM flashing
bit flash_program;               // used for upgrading firmware
bit reboot;                      // used for rebooting

/*------------------------------------------------------------------*/

void setup(void)
{
unsigned char i;

#ifdef CPU_CYGNAL
  
  /* Port configuration (1 = Push Pull Output) */

#ifdef CPU_C8051F020
  XBR0 = 0x04; // Enable UART0 & UART1
  XBR1 = 0x00;
  XBR2 = 0x44;

  P0MDOUT = 0x05;  // P0.0,2: TX,RS485 = Push Pull
  P1MDOUT = 0x00;  // P1: LPT
  P2MDOUT = 0x00;  // P2: LPT
  P3MDOUT = 0xE0;  // P3.5,6,7: Optocouplers
#else
  XBR0 = 0x04; // Enable RX/TX
  XBR1 = 0x00;
  XBR2 = 0x40; // Enable crossbar

  PRT0CF = 0x05;   // P0.0,2: TX,RS485 = Push Pull
  PRT1CF = 0x00;   // P1
  PRT2CF = 0x00;   // P2  Open drain for 5V LCD
  PRT3CF = 0x00;   // P3
#endif

  /* Select external quartz oscillator */
  OSCXCN = 0x66;  // Crystal mode, Power Factor 22E6
  OSCICN = 0x08;  // CLKSL=1 (external)

#endif

  /* enable watchdog */
#ifdef CPU_ADUC812
  WDCON = 0xE0;      // 2048 msec
  WDE = 1;
#endif
#ifdef CPU_CYGNAL
  WDTCN = 0x07;      // 95 msec
  WDTCN = 0xA5;      // start watchdog
#endif

  /* start system clock */
  sysclock_init();

  /* init memory */
  CSR = 0;
  LED = LED_OFF;
#ifdef LED_2
  LED_SEC = LED_OFF;
#endif
  RS485_ENABLE = 0;
  i_in = i_out = n_out = 0;

  uart_init(0, BD_115200);

  /* count channels and parameters */
  for (n_channel=0 ; ; n_channel++)
    if (channel[n_channel].width == 0)
      break;
  for (n_param=0 ; ; n_param++)
    if (conf_param[n_param].width == 0)
      break;

  /* retrieve EEPROM data */
  eeprom_retrieve();

  /* correct initial value */
  if (sys_info.magic != 0xC0DE)
    {
    sys_info.node_addr  = 0xFFFF;
    sys_info.group_addr = 0xFFFF;
    sys_info.wd_counter = 0;
    sys_info.magic      = 0xC0DE;

    // init channel variables
    for (i=0 ; channel[i].width ; i++)
      if (channel[i].ud)
        memset(channel[i].ud, 0, channel[i].width);
  
    // init configuration parameters 
    for (i=0 ; conf_param[i].width ; i++)
      if (conf_param[i].ud)
        memset(conf_param[i].ud, 0, conf_param[i].width);

    /* call user initialization routine with initialization */
    user_init(1);

    eeprom_flash();
    }
  else
    /* call user initialization routine without initialization */
    user_init(0);

  /* check if reset by watchdog */
#ifdef CPU_ADUC812
  if (WDS)
#endif
#ifdef CPU_CYGNAL
  if (RSTSRC & 0x08)
#endif
    {
    WD_RESET = 1;
    sys_info.wd_counter++;
    eeprom_flash();
    }

#if !defined(CPU_ADUC812) && !defined(SCS_300) && !defined(SCS_210) // SCS210/300 redefine printf()
  lcd_setup();

  if (lcd_present)
    {
    printf("AD:%04X GA:%04X WD:%d", sys_info.node_addr, 
            sys_info.group_addr, sys_info.wd_counter);
    }
#else
  lcd_present = 0;
#endif

  /* Blink LEDs */
  led_blink(1, 5, 150);
  led_blink(2, 5, 150);
}

/*------------------------------------------------------------------*/

void debug_output()
{
  if (!DEBUG_MODE)
    return;
                             
#if !defined(CPU_ADUC812) && !defined(SCS_300) && !defined(SCS_210) // SCS210/300 redefine printf()
  {
  unsigned char i, n;

  if (debug_new_i)
    {
    debug_new_i = 0;
    lcd_clear();

    n = i_in ? i_in : cmd_len;
    for (i=0 ; i<n ; i++)
      printf("%02bX ", in_buf[i]);
    }

  if (debug_new_o)
    {
    debug_new_o = 0;
    lcd_goto(0, 1);
    for (i=0 ; i<n_out ; i++)
      printf("%02bX ", out_buf[i]);
    }
  }
#endif

}
        
/*------------------------------------------------------------------*\

  Serial interrupt

\*------------------------------------------------------------------*/

void interprete(void);

void serial_int(void) interrupt 4 using 1
{
  if (TI0)
    {
    /* character has been transferred */
    
    TI0 = 0;               // clear TI flag

    i_out++;              // increment output counter
    if (i_out == n_out)
      {
      i_out = 0;          // send buffer empty, clear pointer
      RS485_ENABLE = 0;   // disable RS485 driver
      debug_new_o = 1;    // indicate new debug output
      }
    else
      {
      SBUF0 = out_buf[i_out];  // send character
      }
    }

  if (RI0)
    {
    /* character has been received */

    if (!RB80 && !addressed)
      {
      RI0 = 0;
      i_in = 0;
      return;           // discard data if not bit9 and not addressed
      }

    RB80 = 0;
    in_buf[i_in++] = SBUF0;
    RI0 = 0;

    if (i_in == 1)
      {
      /* check for padding character */
      if (in_buf[0] == 0)
        {
        i_in = 0;
        return;
        }

      /* initialize command length if first byte */
      cmd_len = (in_buf[0] & 0x07) + 2; // + cmd + crc
      }

    debug_new_i = 1; // indicate new data in input buffer

    if (i_in == sizeof(in_buf))  // check for buffer overflow
      {
      i_in = 0;
      return;                    // don't interprete command
      }

    if (i_in < cmd_len)          // return if command not yet complete
      return;

    if (in_buf[i_in-1] != crc8(in_buf, i_in-1))
      {
      i_in = 0;
      return;                    // return if CRC code does not match
      }

    interprete();                // interprete command
    i_in = 0;
    }
}

/*------------------------------------------------------------------*\

  Interprete MSCB command

\*------------------------------------------------------------------*/

#pragma NOAREGS

void set_addressed(unsigned char mode, bit flag)
{
  if (flag)
    {
    addressed = 1;
    led_blink(1, 1, 50);
    addr_mode = mode;
    }
  else
    {
    addressed = 0;
    addr_mode = ADDR_NONE;
    }
}

void send_byte(unsigned char d, unsigned char data *crc)
{
  if (crc)
    *crc = crc8_add(*crc, d);
  SBUF0 = d;          
  while (!TI0);
  TI0 = 0;
}

void interprete(void)
{
unsigned char crc, cmd, i, j, n;
MSCB_INFO_CHN code *pchn;

  cmd = (in_buf[0] & 0xF8); // strip lenth field

  switch (in_buf[0])
    {
    case CMD_ADDR_NODE8:
      set_addressed(ADDR_NODE, in_buf[1] == *(unsigned char *)&sys_info.node_addr);
      break;

    case CMD_ADDR_NODE16:
      set_addressed(ADDR_NODE, *(unsigned int *)&in_buf[1] == sys_info.node_addr);
      break;

    case CMD_ADDR_BC:
      set_addressed(ADDR_ALL, 1);
      break;

    case CMD_ADDR_GRP8:
      set_addressed(ADDR_GROUP, in_buf[1] == *(unsigned char *)&sys_info.group_addr);
      break;

    case CMD_ADDR_GRP16:
      set_addressed(ADDR_GROUP, *(unsigned int *)&in_buf[1] == sys_info.group_addr);
      break;

    case CMD_PING8:
      set_addressed(ADDR_NODE, in_buf[1] == *(unsigned char *)&sys_info.node_addr);
      if (addressed)
        {
        out_buf[0] = CMD_ACK;
        n_out = 1;
        RS485_ENABLE = 1;
        SBUF0 = CMD_ACK;
        }
      break;

    case CMD_PING16:
      set_addressed(ADDR_NODE, *(unsigned int *)&in_buf[1] == sys_info.node_addr);
      if (addressed)
        {
        out_buf[0] = CMD_ACK;
        n_out = 1;
        RS485_ENABLE = 1;
        SBUF0 = CMD_ACK;
        }
      break;

    case CMD_INIT:
      reboot = 1; // reboot in next main loop
      break;

    case CMD_GET_INFO:
      /* general info */

      ES0 = 0;                     // temporarily disable serial interrupt
      crc = 0;
      RS485_ENABLE = 1;

      send_byte(CMD_ACK+7, &crc); // send acknowledge, variable data length
      send_byte(26, &crc);        // send data length
      send_byte(VERSION, &crc);   // send protocol version
      send_byte(CSR, &crc);       // send node status

      send_byte(n_channel, &crc); // send number of channels
      send_byte(n_param, &crc);   // send number of configuration parameters

      send_byte(*(((unsigned char *)&sys_info.node_addr)+0), &crc);    // send node address
      send_byte(*(((unsigned char *)&sys_info.node_addr)+1), &crc); 

      send_byte(*(((unsigned char *)&sys_info.group_addr)+0), &crc);   // send group address
      send_byte(*(((unsigned char *)&sys_info.group_addr)+1), &crc); 

      send_byte(*(((unsigned char *)&sys_info.wd_counter)+0), &crc);   // send watchdog resets
      send_byte(*(((unsigned char *)&sys_info.wd_counter)+1), &crc);

      for (i=0 ; i<16 ; i++)                       // send node name
        send_byte(node_name[i], &crc);

      send_byte(crc, NULL);                        // send CRC code

      RS485_ENABLE = 0;
      ES0 = 1;                                      // re-enable serial interrupts
      break;

    case CMD_GET_INFO+2:
      /* send channel/config param info */

      if (in_buf[1] == GET_INFO_CHANNEL)
        {
        i = n_channel;
        pchn = channel+in_buf[2];
        }
      else
        {
        i = n_param;
        pchn = conf_param+in_buf[2];
        }

      if (in_buf[2] < i)
        {
        ES0 = 0;                     // temporarily disable serial interrupt
        crc = 0;
        RS485_ENABLE = 1;
  
        send_byte(CMD_ACK+7, &crc); // send acknowledge, variable data length
        send_byte(13, &crc);        // send data length
        send_byte(pchn->width, &crc);
        send_byte(pchn->unit, &crc);
        send_byte(pchn->prefix, &crc);
        send_byte(pchn->status, &crc);
        send_byte(pchn->flags, &crc);
  
        for (i=0 ; i<8 ; i++)                        // send channel name
          send_byte(pchn->name[i], &crc);
  
        send_byte(crc, NULL);                        // send CRC code
  
        RS485_ENABLE = 0;
        ES0 = 1;                                      // re-enable serial interrupts
        }
      break;

    case CMD_SET_ADDR:
      /* set address in RAM */
      sys_info.node_addr  = *((unsigned int*)(in_buf+1));
      sys_info.group_addr = *((unsigned int*)(in_buf+3));

      /* copy address to EEPROM */
      flash_param = 1;

      /* output new address */
      new_address = 1;
      break;

    case CMD_SET_BAUD:
      // uart_init(0, in_buf[1]);
      break;

    case CMD_FREEZE:
      FREEZE_MODE = in_buf[1];
      break;

    case CMD_SYNC:
      SYNC_MODE = in_buf[1];
      break;

    case CMD_UPGRADE:

      flash_program = 1;

      /* send acknowledge */
      out_buf[0] = CMD_ACK;
      out_buf[1] = in_buf[i_in-1];
      n_out = 2;
      RS485_ENABLE = 1;
      SBUF0 = out_buf[0];
      break;

    case CMD_FLASH:

      flash_param = 1;

      /* send acknowledge */
      out_buf[0] = CMD_ACK;
      out_buf[1] = in_buf[i_in-1];
      n_out = 2;
      RS485_ENABLE = 1;
      SBUF0 = out_buf[0];
      break;

    case CMD_ECHO:
      out_buf[0] = CMD_ACK+1;
      out_buf[1] = in_buf[1];
      out_buf[2] = crc8(out_buf, 2);
      n_out = 3;
      RS485_ENABLE = 1;
      SBUF0 = out_buf[0];
      break;

    }

  if (cmd == CMD_READ)
    {
    if (in_buf[0] == CMD_READ + 1)  // single channel
      {
      if (in_buf[1] < n_channel)
        {
        n = channel[in_buf[1]].width; // number of bytes to return
    
        if (channel[in_buf[1]].ud == 0)
          {
          n = user_read(in_buf[1]);   // for dataless channels, user routine returns bytes
          out_buf[0] = CMD_ACK + n;   // and places data directly in out_buf
          }
        else
          {
          user_read(in_buf[1]);
    
          out_buf[0] = CMD_ACK + n;
          for (i=0 ; i<n ; i++)
            out_buf[1+i] = ((char idata *)channel[in_buf[1]].ud)[i];  // copy user data
          }
    
        out_buf[1+n] = crc8(out_buf, 1 + n); // generate CRC code
    
        /* send result */
        n_out = 2 + n;
        RS485_ENABLE = 1;
        SBUF0 = out_buf[0];
        }
      }
    else if (in_buf[0] == CMD_READ + 2) // channel range
      {
      if (in_buf[1] < n_channel && in_buf[2] < n_channel && in_buf[1] < in_buf[2])
        {
        /* calculate number of bytes to return */
        for (i=in_buf[1],n=0 ; i<=in_buf[2] ; i++)
          {
          user_read(i);
          n += channel[i].width;
          }
  
        ES0 = 0;                      // temporarily disable serial interrupt
        crc = 0;
        RS485_ENABLE = 1;
  
        send_byte(CMD_ACK+7, &crc);   // send acknowledge, variable data length
        send_byte(n, &crc);           // send data length
  
        /* loop over all channels */
        for (i=in_buf[1] ; i<=in_buf[2] ; i++)
          {
          for (j=0 ; j<channel[i].width ; j++)
            send_byte(((char idata *)channel[i].ud)[j], &crc); // send user data
          }
  
        send_byte(crc, NULL);         // send CRC code
  
        RS485_ENABLE = 0;
        ES0 = 1;                      // re-enable serial interrupts
        }
      }
    }

  if (cmd == CMD_READ_CONF)
    {
    if (in_buf[1] < n_param)
      {
      user_read_conf(in_buf[1]);      // let user readout routine update data buffer
  
      n = conf_param[in_buf[1]].width; // number of bytes to return
  
      out_buf[0] = CMD_ACK + n;
      for (i=0 ; i<n ; i++)
        if (conf_param[in_buf[1]].ud)
          out_buf[1+i] = ((char idata *)conf_param[in_buf[1]].ud)[i];  // copy user data
  
      out_buf[1+i] = crc8(out_buf, 1 + n); // generate CRC code
  
      /* send result */
      n_out = 2 + n;
      RS485_ENABLE = 1;
      SBUF0 = out_buf[0];
      }
    }

  if (cmd == CMD_USER)
    {
    n = user_func(in_buf+1, out_buf+1);
    out_buf[0] = CMD_ACK + n;
    out_buf[n+1] = crc8(out_buf, n+1);
    n_out = n+2;
    RS485_ENABLE = 1;
    SBUF0 = out_buf[0];
    }

  if (cmd == CMD_WRITE_NA ||
      cmd == CMD_WRITE_ACK)
    {
    if (in_buf[1] < n_channel)
      {
      n = channel[in_buf[1]].width;
  
      /* copy LSB bytes */
      for (i=0 ; i<n ; i++)
        if (conf_param[in_buf[1]].ud)
          ((char idata *)channel[in_buf[1]].ud)[i] = in_buf[i_in-1-n+i];
      
      user_write(in_buf[1]);
  
      if (cmd == CMD_WRITE_ACK)
        {
        out_buf[0] = CMD_ACK;
        out_buf[1] = in_buf[i_in-1];
        n_out = 2;
        RS485_ENABLE = 1;
        SBUF0 = out_buf[0];
        }
      }
    }

  if (cmd == CMD_WRITE_CONF)
    {
    if (in_buf[1] < n_param && in_buf[1] != 0xFF)
      {
      if (in_buf[1] == 0xFF)
        {
        CSR = in_buf[2];
        }
      else
        {
        n = conf_param[in_buf[1]].width;
    
        /* copy LSB bytes */
        for (i=0 ; i<n ; i++)
          if (channel[in_buf[1]].ud)
            ((char idata *)conf_param[in_buf[1]].ud)[i] = in_buf[i_in-1-n+i];
        
        user_write_conf(in_buf[1]);
        }
    
      out_buf[0] = CMD_ACK;
      out_buf[1] = in_buf[i_in-1];
      n_out = 2;
      RS485_ENABLE = 1;
      SBUF0 = out_buf[0];
      }
    }

}

/*------------------------------------------------------------------*/

void upgrade()
{
#ifdef CPU_CYGNAL
unsigned char cmd, page;
unsigned short i;
unsigned char xdata *pw;
unsigned char code  *pr;

  /* wait until acknowledge has been sent */
  while (RS485_ENABLE);

  /* disable all interrupts */
  EA = 0;

  /* disable watchdog */
  WDTCN  = 0xDE;
  WDTCN  = 0xAD;

  cmd = page = 0;


  /* send ready */
  RS485_ENABLE = 1;
  TI0 = 0;
  SBUF0 = 0xBE;
  while (TI0 == 0);
  RS485_ENABLE = 0;

  do
    {
    /* receive command */
    while (!RI0);
    cmd = SBUF0;
    RI0 = 0;

    switch (cmd)
      {
      case 1: // return acknowledge
        break;

      case 2: // erase page
        /* receive page */
        while (!RI0);
        page = SBUF0;
        RI0 = 0;

        /* erase page */
#if defined(CPU_C8051F000)
        FLSCL = (FLSCL & 0xF0) | 0x08; // set timer for 11.052 MHz clock
#elif defined (CPU_C8051F020)
        FLSCL = FLSCL  | 1;            // enable flash writes
#endif
        PSCTL = 0x03;                  // allow write and erase

        pw = (char xdata *)(512 * page);
        *pw = 0;
        
        FLSCL = (FLSCL & 0xF0);
        PSCTL = 0x00;

        break;

      case 3: // program page
        LED = LED_OFF;

        /* receive page */
        while (!RI0);
        page = SBUF0;
        RI0 = 0;

        /* allow write */
#if defined(CPU_C8051F000)
        FLSCL = (FLSCL & 0xF0) | 0x08; // set timer for 11.052 MHz clock
#elif defined (CPU_C8051F020)
        FLSCL = FLSCL  | 1;            // enable flash writes
#endif
        PSCTL = 0x01;                  // allow write access

        pw = (char xdata *)(512 * page);

        for (i=0 ; i<512 ; i++)
          {
          /* receive byte */
          while (!RI0);

          /* flash byte */
          *pw++ = SBUF0;
          RI0 = 0;
          }

        /* disable write */
        FLSCL = (FLSCL & 0xF0);
        PSCTL = 0x00;
        break;

      case 4: // verify page
        LED = LED_ON;

        /* receive page */
        while (!RI0);
        page = SBUF0;
        RI0 = 0;

        pr = 512 * page;

        RS485_ENABLE = 1;
        for (i=0 ; i<512 ; i++)
          {
          TI0 = 0;
          SBUF0 = *pr++;
          while (TI0 == 0);
          }
        RS485_ENABLE = 0;
        break;

      case 5: // reboot
        RSTSRC = 0x02;
        break;

      case 6: // return
        break;
      }

    /* return acknowledge */
    RS485_ENABLE = 1;
    TI0 = 0;
    SBUF0 = cmd;
    while (TI0 == 0);
    RS485_ENABLE = 0;

    } while (cmd != 6);


  EA = 1; // re-enable interrupts

#endif // CPU_CYGNAL
}

/*------------------------------------------------------------------*\

  Yield should be called periodically by applications with long loops
  to insure proper watchdog refresh

\*------------------------------------------------------------------*/

void yield(void)
{
  watchdog_refresh();
}

/*------------------------------------------------------------------*\

  Main loop

\*------------------------------------------------------------------*/

void main(void)
{
  setup();

  do
    {
    yield();

    user_loop();

    /* output debug info to LCD if asked by interrupt routine */
    debug_output();

    /* output RS232 data if present */
    rs232_output();
    
    /* output new address to LCD if available */
#if !defined(CPU_ADUC812) && !defined(SCS_300) && !defined(SCS_210) // SCS210/300 redefine printf()
    if (new_address)
      {
      new_address = 0;
      lcd_clear();
      printf("AD:%04X GA:%04X WD:%d", sys_info.node_addr, 
              sys_info.group_addr, sys_info.wd_counter);
      }
#endif

    /* flash EEPROM if asked by interrupt routine */
    if (flash_param)
      {
      flash_param = 0;
      eeprom_flash();
      }

    if (flash_program)
      {
      flash_program = 1;
      upgrade();
      }

    if (reboot)
      {
#ifdef CPU_CYGNAL
      RSTSRC = 0x02;   // force power-on reset
#else
      WDCON = 0x00;      // 16 msec
      WDE = 1;
      while (1);       // should be hardware reset later...
#endif
      }

    } while (1);
  
}

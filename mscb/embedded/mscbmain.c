/********************************************************************\

  Name:         mscbmain.c
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus protocol main program

  $Log$
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

/* bit definitions */
sbit LED =               P3^4;   // red LED on SCS100/SCS101
sbit RS485_ENABLE =      P3^5;   // enable bit for MAX1483

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

void user_init(void);
void user_write(unsigned char channel);
unsigned char user_read(unsigned char channel);
void user_write_conf(unsigned char channel);
void user_read_conf(unsigned char channel);
void user_loop(void);

extern MSCB_INFO_CHN code channel[];
extern MSCB_INFO_CHN code conf_param[];

extern char code node_name[];

/*------------------------------------------------------------------*/

/* variables in internal RAM */

unsigned char idata in_buf[10], out_buf[8];  // move buffers to IDATA 

unsigned char i_in, last_i_in, final_i_in, n_out, i_out, cmd_len;
unsigned char crc_code, addr_mode, tmp[4];

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
bit flash;                       // used for EEPROM flashing

/*------------------------------------------------------------------*/

void setup(void)
{
unsigned char i;

#ifdef CPU_C8051F000
  
  XBR0 = 0x04; // Enable RX/TX
  XBR1 = 0x00;
  XBR2 = 0x40; // Enable crossbar

  /* Port configuration (1 = Push Pull Output) */
  PRT0CF = 0xFF;  // P0: TX = Push Pull
  PRT1CF = 0xFF;  // P1
  PRT2CF = 0x00;  // P2  Open drain for 5V LCD
  PRT3CF = 0x00;  // P3

  /* Disable watchdog (for LCD setup with delays) */
  WDTCN  = 0xDE;
  WDTCN  = 0xAD;

  /* Select external quartz oscillator */
  OSCXCN = 0x66;  // Crystal mode, Power Factor 22E6
  OSCICN = 0x08;  // CLKSL=1 (external)

#endif

  /* init memory */
  CSR = 0;
  LED = 0;
  RS485_ENABLE = 0;

  uart_init(BD_115200);

  /* retrieve EEPROM data */
  eeprom_retrieve();

  /* call user initialization routine */
  user_init();

  /* correct initial value */
  if (sys_info.wd_counter == 0xFFFF)
    {
    sys_info.wd_counter = 0;

    // init channel variables
    for (i=0 ; channel[i].width ; i++)
      memset(channel[i].ud, 0, channel[i].width);
  
    // init configuration parameters 
    for (i=0 ; conf_param[i].width ; i++)
      memset(conf_param[i].ud, 0, conf_param[i].width);

    eeprom_flash();
    }

  /* check if reset by watchdog */
#ifdef CPU_ADUC812
  if (WDS)
#endif
#ifdef CPU_C8051F000
  if (RSTSRC & 0x08)
#endif
    {
    WD_RESET = 1;
    sys_info.wd_counter++;
    eeprom_flash();
    }

  lcd_setup();
  lcd_clear();
  printf("AD:%04X GA:%04X WD:%d", sys_info.node_addr, 
          sys_info.group_addr, sys_info.wd_counter);

  /* Blink LED */
  for (i=0 ; i<5 ; i++)
    {
    LED = 1;
    delay_ms(100);
    LED = 0;
    delay_ms(200);
    }

  lcd_clear();

  /* enable watchdog */
#ifdef CPU_ADUC812
  WDCON = 0xE0;      // 2048 msec
  WDE = 1;
#endif
#ifdef CPU_C8051F000
  WDTCN=0x07;        // 95 msec
#endif

  /* start system clock */
  sysclock_init();
}

/*------------------------------------------------------------------*/

void debug_output()
{
unsigned char i, n;

  if (!DEBUG_MODE)
    return;
                             
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
        
/*------------------------------------------------------------------*\

  Serial interrupt

\*------------------------------------------------------------------*/

void interprete(void);

void serial_int(void) interrupt 4 using 1
{
  if (TI)
    {
    /* character has been transferred */
    
    TI = 0;               // clear TI flag

    i_out++;              // increment output counter
    if (i_out == n_out)
      {
      i_out = 0;          // send buffer empty, clear pointer
      RS485_ENABLE = 0;   // disable RS485 driver
      debug_new_o = 1;    // indicate new debug output
      }
    else
      {
      SBUF = out_buf[i_out];  // send character
      }
    }
  else
    {
    /* character has been received */

    if (!RB8 && !addressed)
      {
      RI = 0;
      i_in = 0;
      return;           // discard data if not bit9 and not addressed
      }

    RB8 = 0;
    in_buf[i_in++] = SBUF;
    RI = 0;

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
    LED = 1;
    addr_mode = mode;
    }
  else
    {
    addressed = 0;
    LED = 0;
    addr_mode = ADDR_NONE;
    }
}

void send_byte(unsigned char d, unsigned char data *crc)
{
  if (crc)
    *crc = crc8_add(*crc, d);
  SBUF = d;
  while (!TI);
  TI = 0;
}

void interprete(void)
{
unsigned char crc, cmd, i, n;
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
        SBUF = CMD_ACK;
        }
      break;

    case CMD_PING16:
      set_addressed(ADDR_NODE, *(unsigned int *)&in_buf[1] == sys_info.node_addr);
      if (addressed)
        {
        out_buf[0] = CMD_ACK;
        n_out = 1;
        RS485_ENABLE = 1;
        SBUF = CMD_ACK;
        }
      break;

    case CMD_INIT:
#ifdef CPU_C8051F000
      RSTSRC = 0x02;   // force power-on reset
#endif
      break;

    case CMD_GET_INFO:
      /* general info */

      ES = 0;                     // temporarily disable serial interrupt
      crc = 0;
      RS485_ENABLE = 1;

      send_byte(CMD_ACK+7, &crc); // send acknowledge, variable data length
      send_byte(26, &crc);        // send data length
      send_byte(VERSION, &crc);   // send protocol version
      send_byte(CSR, &crc);       // send node status

      for (i=0 ; ; i++)           // send number of channels
        if (channel[i].width == 0)
          break;
      send_byte(i, &crc);

      for (i=0 ; ; i++)           // send number of configuration parameters
        if (conf_param[i].width == 0)
          break;
      send_byte(i, &crc);

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
      ES = 1;                                      // re-enable serial interrupts
      break;

    case CMD_GET_INFO+2:
      /* send channel/config param info */

      if (in_buf[1] == GET_INFO_CHANNEL)
        pchn = channel+in_buf[2];
      else
        pchn = conf_param+in_buf[2];
      
      ES = 0;                     // temporarily disable serial interrupt
      crc = 0;
      RS485_ENABLE = 1;

      send_byte(CMD_ACK+7, &crc); // send acknowledge, variable data length
      send_byte(12, &crc);        // send data length
      send_byte(pchn->width, &crc);
      send_byte(pchn->units, &crc);
      send_byte(pchn->status, &crc);
      send_byte(pchn->flags, &crc);

      for (i=0 ; i<8 ; i++)                        // send channel name
        send_byte(pchn->name[i], &crc);

      send_byte(crc, NULL);                        // send CRC code

      RS485_ENABLE = 0;
      ES = 1;                                      // re-enable serial interrupts
      break;

    case CMD_SET_ADDR:
      /* set address in RAM */
      sys_info.node_addr  = *((unsigned int*)(in_buf+1));
      sys_info.group_addr = *((unsigned int*)(in_buf+3));

      /* copy address to EEPROM */
      flash = 1;

      /* output new address */
      new_address = 1;
      break;

    case CMD_SET_BAUD:
      uart_init(in_buf[1]);
      break;

    case CMD_FREEZE:
      FREEZE_MODE = in_buf[1];
      break;

    case CMD_SYNC:
      SYNC_MODE = in_buf[1];
      break;

    case CMD_TRANSP: // TBD...
      break;

    case CMD_FLASH:

      flash = 1;

      /* send acknowledge */
      out_buf[0] = CMD_ACK;
      out_buf[1] = in_buf[i_in-1];
      n_out = 2;
      RS485_ENABLE = 1;
      SBUF = out_buf[0];
      break;

    case CMD_READ:
      user_read(in_buf[1]);  // let user readout routine update data buffer

      n = channel[in_buf[1]].width; // number of bytes to return

      out_buf[0] = CMD_ACK + n;
      for (i=0 ; i<n ; i++)
        out_buf[1+i] = ((char idata *)channel[in_buf[1]].ud)[i];  // copy user data

      out_buf[1+i] = crc8(out_buf, 1 + n); // generate CRC code

      /* send result */
      n_out = 2 + n;
      RS485_ENABLE = 1;
      SBUF = out_buf[0];

      break;

    case CMD_READ_CONF:
      user_read_conf(in_buf[1]);  // let user readout routine update data buffer

      n = conf_param[in_buf[1]].width; // number of bytes to return

      out_buf[0] = CMD_ACK + n;
      for (i=0 ; i<n ; i++)
        out_buf[1+i] = ((char idata *)conf_param[in_buf[1]].ud)[i];  // copy user data

      out_buf[1+i] = crc8(out_buf, 1 + n); // generate CRC code

      /* send result */
      n_out = 2 + n;
      RS485_ENABLE = 1;
      SBUF = out_buf[0];

      break;
    }

  if (cmd == CMD_USER)
    {
    n = user_func(in_buf+1, out_buf+1);
    out_buf[0] = CMD_ACK + n;
    out_buf[n+1] = crc8(out_buf, n+1);
    n_out = n+2;
    RS485_ENABLE = 1;
    SBUF = out_buf[0];
    }

  if (cmd == CMD_WRITE_NA ||
      cmd == CMD_WRITE_ACK)
    {
    n = channel[in_buf[1]].width;

    /* copy LSB bytes */
    for (i=0 ; i<n ; i++)
      ((char idata *)channel[in_buf[1]].ud)[i] = in_buf[i_in-1-n+i];
    
    user_write(in_buf[1]);

    if (cmd == CMD_WRITE_ACK)
      {
      out_buf[0] = CMD_ACK;
      out_buf[1] = in_buf[i_in-1];
      n_out = 2;
      RS485_ENABLE = 1;
      SBUF = out_buf[0];
      }
    }

  if (cmd == CMD_WRITE_CONF)
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
        ((char idata *)conf_param[in_buf[1]].ud)[i] = in_buf[i_in-1-n+i];
      
      user_write_conf(in_buf[1]);
      }
  
    out_buf[0] = CMD_ACK;
    out_buf[1] = in_buf[i_in-1];
    n_out = 2;
    RS485_ENABLE = 1;
    SBUF = out_buf[0];
    }

}

/*------------------------------------------------------------------*\

  Main loop

\*------------------------------------------------------------------*/

void main(void)
{
  setup();

  do
    {
    watchdog_refresh();
    user_loop();

    /* output debug info to LCD if asked by interrupt routine */
    debug_output();

    /* output new address to LCD if available */
    if (new_address)
      {
      new_address = 0;
      lcd_clear();
      printf("AD:%04X GA:%04X WD:%d", sys_info.node_addr, 
              sys_info.group_addr, sys_info.wd_counter);
      }

    /* flash EEPROM if asked by interrupt routine */
    if (flash)
      {
      flash = 0;
      eeprom_flash();
      }

    } while (1);
  
}

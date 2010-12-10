/********************************************************************\

  Name:         scd_310.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-310 GPIB Adapter

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mscbemb.h"

char code node_name[] = "SCS-310";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

bit output_flag, control_flag;

xdata char term_obuf[250], term_ibuf[250];
unsigned long last_read;
unsigned char towp;
unsigned char tiwp, tirp;

/*---- Define variable parameters returned to the CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   char output[32];
   char input[32];
   char control;
   char srq;
   char gpib_adr;
} idata user_data;

MSCB_INFO_VAR code vars[] = {
   1,  UNIT_ASCII,  0, 0, MSCBF_DATALESS, "GPIB",                       0, // 0
   32, UNIT_STRING, 0, 0,              0, "Output",  &user_data.output[0], // 1
   32, UNIT_STRING, 0, 0,              0, "Input",    &user_data.input[0], // 2
   1,  UNIT_BYTE,   0, 0,              0, "Control",   &user_data.control, // 3
   1,  UNIT_BYTE,   0, 0,              0, "SRQ",           &user_data.srq, // 4
   1,  UNIT_BYTE,   0, 0,              0, "GPIB Adr", &user_data.gpib_adr, // 5
   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

/* 8 data bits to LPT */
#define GPIB_DATA        P1

/* GPIB control/status bits DB40 */
sbit GPIB_EOI  = P2 ^ 1;         // Pin 5
sbit GPIB_DAV  = P2 ^ 2;         // Pin 6
sbit GPIB_NRFD = P2 ^ 3;         // Pin 7
sbit GPIB_NDAC = P2 ^ 4;         // Pin 8
sbit GPIB_IFC  = P2 ^ 5;         // Pin 9
sbit GPIB_SRQ  = P2 ^ 6;         // Pin 10
sbit GPIB_ATN  = P2 ^ 7;         // Pin 11
sbit GPIB_REM  = P2 ^ 0;         // Pin 17

#pragma NOAREGS

void user_write(unsigned char index) reentrant;
char send(unsigned char adr, char *str);
char send_byte(unsigned char b);

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   P0MDOUT = 0x01;              // P0.0: TX = Push Pull
   P1MDOUT = 0x00;              // P1: LPT
   P2MDOUT = 0x00;              // P2: LPT
   P3MDOUT = 0xE0;              // P3.5,6,7: Optocouplers

   /* set initial state of lines */
   GPIB_DATA = 0xFF;
   GPIB_EOI = 1;
   GPIB_DAV = 1;
   GPIB_NRFD = 1;
   GPIB_NDAC = 1;
   GPIB_IFC = 1;
   GPIB_SRQ = 1;
   GPIB_ATN = 1;
   GPIB_REM = 1;

   /* initialize GPIB */
   GPIB_IFC = 0;
   delay_ms(1);
   GPIB_IFC = 1;

   GPIB_ATN = 0;
   send_byte(0x14);             // DCL
   GPIB_ATN = 1;

   output_flag = 0;
   control_flag = 0;

   if (init) {
      user_data.gpib_adr = 16;
      user_data.control = 0;
      user_data.srq = 0;
   }
}

/*---- User write function -----------------------------------------*/

/* buffers in mscbmain.c */
extern unsigned char xdata in_buf[64], out_buf[64];

void user_write(unsigned char index) reentrant
{
   unsigned char i, n;

   if (index == 0) {
      /* put bytes in terminal buffer for main loop */
      n = in_buf[1]-1;
      for (i=0 ; i<n ; i++) {
         term_obuf[towp++] = in_buf[3 + i];
         if (towp == sizeof(term_obuf))
            towp--;
      }

   }

   if (index == 1)
      output_flag = 1;

   if (index == 3)
      control_flag = 1;
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
   unsigned char n;

   if (index == 0) {

      /* prevent reads for 1s */
      last_read = time();

      for (n=0 ; n<32 ; n++) {
         if (tirp != tiwp) {
            out_buf[2 + n] = term_ibuf[tirp++];
            if (tirp == sizeof(term_ibuf))
               tirp = 0;
         } else
            break;
      }

      return n;
   }

   return 0;
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char *data_in, unsigned char *data_out)
{
   /* echo input data */
   data_out[0] = data_in[0];
   data_out[1] = data_in[1];
   return 2;
}

/*---- Functions for GPIB port -------------------------------------*/

char send_byte(unsigned char b)
{
   unsigned int i;

   yield();

   /* wait for NRFD go high */
   for (i = 0; i < 1000; i++)
      if (GPIB_NRFD == 1)
         break;

   if (GPIB_NRFD == 0)
      return 0;

   GPIB_DATA = ~b;              // negate
   delay_us(1);                 // let signals settle
   GPIB_DAV = 0;

   /* wait for NDAC go high */
   for (i = 0; i < 1000; i++)
      if (GPIB_NDAC == 1)
         break;

   if (GPIB_NDAC == 0) {
      GPIB_DAV = 1;
      GPIB_DATA = 0xFF;
      return 0;                 // timeout
   }

   GPIB_DAV = 1;
   GPIB_DATA = 0xFF;            // prepare for input

   /* wait for NRFD go high */
   for (i = 0; i < 1000; i++)
      if (GPIB_NRFD == 1)
         break;

   if (GPIB_NRFD == 0)
      return 0;

   return 1;
}

/*------------------------------------------------------------------*/

char send(unsigned char adr, char *str)
{
   unsigned char i;

  /*---- address cycle ----*/

   GPIB_ATN = 0;                // assert attention
   send_byte(0x3F);             // unlisten
   send_byte(0x5F);             // untalk
   send_byte(0x20 | adr);       // listen device
   send_byte(0x40 | 21);        // talk 21
   GPIB_ATN = 1;                // remove attention

  /*---- data cycles ----*/

   for (i = 0; str[i] > 0; i++)
      if (send_byte(str[i]) == 0)
         return 0;

   GPIB_EOI = 0;
   send_byte(0x0A);             // NL
   GPIB_EOI = 1;

   return i;
}

/*------------------------------------------------------------------*/

unsigned char enter(unsigned char adr, char *str, unsigned char maxlen)
{
   unsigned long t;
   unsigned char i, flag;
   unsigned int j;

  /*---- address cycle ----*/

   GPIB_ATN = 0;                // assert attention
   send_byte(0x3F);             // unlisten
   send_byte(0x5F);             // untalk
   send_byte(0x20 | 21);        // listen 21
   send_byte(0x40 | adr);       // talk device
   GPIB_ATN = 1;                // remove attention

  /*---- data cycles ----*/

   GPIB_NDAC = 0;               // init NDAC line

   memset(str, 0, maxlen);

   for (i = 0; i < maxlen; i++) {
      yield();

      GPIB_NRFD = 1;            // singal ready for data

      /* wait 1s for DAV go low */
      t = time();
      do {
         if (GPIB_DAV == 0)
            break;

         yield();

      } while (time() - t < 100);

      if (GPIB_DAV == 1) {
         GPIB_NDAC = 1;
         GPIB_NRFD = 1;
         return 0;              // timeout
      }

      led_blink(1, 1, 100);     // singal data received

      GPIB_NRFD = 0;            // signal busy

      str[i] = ~GPIB_DATA;      // read negated data

      flag = GPIB_EOI;          // read EOI flag

      GPIB_NDAC = 1;            // signal acknowledge

      /* wait for DAV go high */
      for (j = 0; j < 1000; j++)
         if (GPIB_DAV == 1)
            break;

      GPIB_NDAC = 0;            // remove acknowledge

      if (flag == 0)            // stop if end of data
         break;
   }

   GPIB_NDAC = 1;               // release handshake lines
   GPIB_NRFD = 1;

   /* stop talker */
   GPIB_ATN = 0;                // assert attention
   send_byte(0x3F);             // unlisten
   send_byte(0x5F);             // untalk
   GPIB_ATN = 1;                // remove attention

   return i;
}

/*---- User loop function ------------------------------------------*/

idata char str[32];

void user_loop(void)
{
   /* check terminal buffer */
   if (towp > 0 && (term_obuf[towp - 1] == 13 || term_obuf[towp - 1] == 0)) {
      
      DISABLE_INTERRUPTS;
   
      /* add trailing "0" */
      if (towp == sizeof(term_obuf))
         towp--;
      term_obuf[towp] = 0;

      /* send buffer */
      send(user_data.gpib_adr, term_obuf);
      towp = 0;

      /* receive buffer */
      tiwp = enter(user_data.gpib_adr, term_ibuf, sizeof(term_ibuf));
      tirp = 0;

      /* add newline */
      if (tiwp > 0 && tiwp < sizeof(term_ibuf)-1)
         term_ibuf[tiwp++] = 10;

      ENABLE_INTERRUPTS;
   }

   if (output_flag) {
      output_flag = 0;

      /* send buffer */
      send(user_data.gpib_adr, user_data.output);

      /* receive buffer */
      if (enter(user_data.gpib_adr, user_data.input, sizeof(user_data.input)))
         led_blink(1, 1, 100);

      /* stip NL */
      if (strlen(user_data.input) > 0 &&
          user_data.input[strlen(user_data.input) - 1] == 10)
         user_data.input[strlen(user_data.input) - 1] = 0;
   }

   if (control_flag) {
      control_flag = 0;

      GPIB_ATN = 0;             // assert attention
      send_byte(0x3F);          // unlisten
      send_byte(0x5F);          // untalk
      send_byte(0x20 | user_data.gpib_adr);     // listen device
      send_byte(0x40 | 21);     // talk 21
      send_byte(user_data.control);     // send control
      send_byte(0x3F);          // unlisten
      send_byte(0x5F);          // untalk
      GPIB_ATN = 1;             // remove attention
   }
}

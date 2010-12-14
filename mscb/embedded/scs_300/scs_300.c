/********************************************************************\

  Name:         scd_300.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-300 Parallel Port Interface

  $Id$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "mscbemb.h"

char code node_name[] = "SCS-300";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float value[4];
} idata user_data;

MSCB_INFO_VAR code vars[] = {
   4, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data0", &user_data.value[0],
   4, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data1", &user_data.value[1],
   4, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data2", &user_data.value[2],
   4, UNIT_UNDEFINED, 0, 0, MSCBF_FLOAT, "Data3", &user_data.value[3],
   0
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

/* 8 data bits to LPT */
#define LPT_DATA         P1

/* control/status bits to LPT       DB25   */
sbit LPT_STROBE = P2 ^ 3;       // Pin 1
sbit LPT_INIT = P2 ^ 4;         // Pin 16
sbit LPT_SELECT = P2 ^ 5;       // Pin 17
sbit LPT_ACK = P2 ^ 6;          // Pin 10
sbit LPT_BUSY = P2 ^ 7;         // Pin 11

#pragma NOAREGS

void user_write(unsigned char channel) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   P0MDOUT = 0x01;              // P0.0: TX = Push Pull
   P1MDOUT = 0x00;              // P1: LPT
   P2MDOUT = 0x00;              // P2: LPT
   P3MDOUT = 0xE0;              // P3.5,6,7: Optocouplers

   /* set initial state of lines */
   LPT_DATA = 0xFF;
   LPT_STROBE = 1;
   LPT_INIT = 1;
   LPT_SELECT = 1;
   LPT_ACK = 1;
   LPT_BUSY = 1;

   if (init);
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if (index);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
   if (index);
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

/*---- Functions for parallel port ---------------------------------*/

char putchar(char c)
{
   /* wait for acknowledge to go low */
   while (LPT_ACK);

   /* output data */
   LPT_DATA = c;

   /* negative strobe */
   LPT_STROBE = 0;

   /* wati for acknowledge to go high */
   while (!LPT_ACK);

   /* remove strobe */
   LPT_STROBE = 1;

   /* clear data for next input */
   LPT_DATA = 0xFF;

   return c;
}

char getchar_nowait1()
{
   char c;

   /* if busy signal high -> no data */
   if (LPT_BUSY == 1)
      return -1;

   /* read byte */
   c = LPT_DATA;

   /* singal received data */
   LPT_SELECT = 0;

   /* wait for busy go high */
   while (LPT_BUSY == 0);

   /* remove select */
   LPT_SELECT = 1;

   return c;
}

unsigned char gets_wait(char *str, unsigned char size, unsigned char timeout)
{
   unsigned long idata start;
   unsigned char i;
   char c;

   start = time();
   i = 0;
   do {
      c = getchar_nowait1();
      if (c != -1 && c != '\n') {
         if (c == '\r') {
            str[i] = 0;
            return i;
         }
         str[i++] = c;
         if (i == size)
            return i;
      }

      yield();

   } while (time() < start + timeout);

   return 0;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   char idata str[32];
   unsigned char i, n;

   return;                      // do not execute example code

   /* this could be some example code to demonstrate a 4 chn readout */

   for (i = 0; i < 4; i++) {
      /* send data request */
      printf("READ %d\r\n", i); // goes through putchar()

      /* wait for reply for 2 seconds */
      n = gets_wait(str, sizeof(str), 200);

      /* if correct response, interprete data */
 //     if (n == 10)
 //        user_data.value[i] = atof(str);

      yield();
   }

}

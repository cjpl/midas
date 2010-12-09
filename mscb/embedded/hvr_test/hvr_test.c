/********************************************************************\

  Name:         hvr_test.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for HVR test stand

  $Id: scs_400.c 3104 2006-05-15 15:22:03Z ritt $

\********************************************************************/

#include <stdio.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "HVR-TEST";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

/*---- Define variable parameters returned to the CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char channel;
} xdata user_data;

MSCB_INFO_VAR code vars[] = {
   1, UNIT_BYTE, 0, 0, 0, "Channel", &user_data.channel,
   0
};


MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

sbit P02 = P0 ^ 2;
sbit P03 = P0 ^ 3;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   if (init);
   PRT0CF |= 0x0C; // push-pull for P0.2 & P0.3
   PRT1CF = 0xFF;  // push-pull for P1
   P1 = 0xFF;
   P02 = 1;
   P03 = 1;
   user_data.channel = 0;
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS


void user_write(unsigned char index) reentrant
{             
   if (index == 0) {
      P1 = ~(1 << user_data.channel);
      P02 = !(user_data.channel == 8);
      P03 = !(user_data.channel == 9);
   }
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

/*------------------------------------------------------------------*/

void user_loop(void)
{
}

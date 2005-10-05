/********************************************************************\

  Name:         scd_320.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-320 VME Crate interface

  $Log$
  Revision 1.1  2005/10/05 15:21:25  ritt
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mscb.h"

char code node_name[] = "SCS-320";

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
   unsigned char fan_ok;
   unsigned char sys_fail;
   unsigned char ac_power_fail;
   unsigned char p5v_ok;
   unsigned char sys_reset;
   unsigned char inhibit;
} idata user_data;

MSCB_INFO_VAR code variables[] = {
   1, UNIT_BOOLEAN, 0, 0, 0, "Fan OK",   &user_data.fan_ok,        // 0
   1, UNIT_BOOLEAN, 0, 0, 0, "SysFail",  &user_data.sys_fail,      // 1
   1, UNIT_BOOLEAN, 0, 0, 0, "ACFail",   &user_data.ac_power_fail, // 2
   1, UNIT_BOOLEAN, 0, 0, 0, "+5V OK",   &user_data.p5v_ok,        // 3
   1, UNIT_BOOLEAN, 0, 0, 0, "SysReset", &user_data.sys_reset,     // 4
   1, UNIT_BOOLEAN, 0, 0, 0, "Inhibit",  &user_data.inhibit,       // 5
   0
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

/* VME control/status bits DB9 connector */
sbit VME_FAN_OK        = P2 ^ 6; // Pin 2
sbit VME_SYS_FAIL      = P2 ^ 5; // Pin 3
sbit VME_AC_POWER_FAIL = P2 ^ 4; // Pin 5
sbit VME_P5V_OK        = P2 ^ 2; // Pin 9
sbit VME_SYS_RESET     = P2 ^ 1; // Pin 7
sbit VME_INHIBIT       = P2 ^ 0; // Pin 8


#pragma NOAREGS

void user_write(unsigned char index) reentrant;
char send(unsigned char adr, char *str);
char send_byte(unsigned char b);

bit do_sys_reset;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   /* set initial state of lines */
   VME_SYS_RESET = 1; // negative polarity
   VME_INHIBIT   = 1; // negative polarity

   do_sys_reset = 0;

   if (init) {
   }
}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   if (index == 4) 
      do_sys_reset = 1;

   if (index == 5)
      VME_INHIBIT = !user_data.inhibit;
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

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   user_data.fan_ok        = VME_FAN_OK;
   user_data.sys_fail      = VME_SYS_FAIL;
   user_data.ac_power_fail = VME_AC_POWER_FAIL;
   user_data.p5v_ok        = VME_P5V_OK;

   if (do_sys_reset) {
      do_sys_reset = 0;
      user_data.sys_reset = 0;

      VME_SYS_RESET = 0;
      delay_ms(100);
      VME_SYS_RESET = 1;
   }
}

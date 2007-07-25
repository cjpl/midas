/********************************************************************\

  Name:         splitter_bp.c
  Created by:   Stefan Ritt


  Contents:     Firmware for MEG splitter produced in Lecce

  $Id: scs_1001.c 3104 2006-05-15 15:22:03Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"

#define DUAL_THRESHOLD /* define for combined backplane Lecce/Pavia */

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "Splitter";
char code svn_revision[] = "$Id: splitter.c 3104 2006-05-15 15:22:03Z ritt $";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

bit update;

void write_tdac(void);

/*---- Port definitions ----*/

sbit DAC_DIN  = P3 ^ 2;
sbit DAC_SCK  = P3 ^ 1;
sbit DAC_NCS  = P3 ^ 0;
sbit ENABLE   = P0 ^ 7;

sbit TDAC_NCS = P1 ^ 0;
sbit TDAC_SDI = P0 ^ 5;
sbit TDAC_CLR = P0 ^ 6;
sbit TDAC_CLK = P0 ^ 3;

sbit FAN1     = P2 ^ 7;
sbit FAN2     = P2 ^ 6;
sbit FAN3     = P2 ^ 5;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char enable;
   float dac;
#ifdef DUAL_THRESHOLD
   float hlt, llt;
#endif
   unsigned int fan1, fan2, fan3;
   float offset;
   float gain;
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 1, UNIT_BOOLEAN, 0, 0, 0,                          "Enable",  &user_data.enable },
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT,                "Utest",   &user_data.dac    },
#ifdef DUAL_THRESHOLD
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT,                "HLT",     &user_data.hlt    },
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT,                "LLT",     &user_data.llt    },
#endif
   { 2, UNIT_RPM,     0, 0, 0,                          "Fan1",    &user_data.fan1   },
   { 2, UNIT_RPM,     0, 0, 0,                          "Fan2",    &user_data.fan2   },
   { 2, UNIT_RPM,     0, 0, 0,                          "Fan3",    &user_data.fan3   },
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "Offset",  &user_data.offset },
   { 4, UNIT_FACTOR,  0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "Gain",    &user_data.gain   },

   { 0 }
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void write_dac(float value) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(unsigned char init)
{
   unsigned char i;


   /* open drain(*) /push-pull: 
      P0.0 TX1       P1.0 TDAC_NCS   P2.0          P3.0 SSYNC
      P0.1*RX1       P1.0            P2.1          P3.1 SCLK 
      P0.2           P1.0            P2.2          P3.2 SOUT 
      P0.3 TDAC_CLK  P1.0            P2.3          P3.3 LED  
                                                           
      P0.4           P1.0            P2.4 *PIC     P3.4*TRIN0
      P0.5 TDAC_SDI  P1.0            P2.5 *FAN1    P3.5*TRIN1
      P0.6 TDAC_CLR  P1.0            P2.6 *FAN1    P3.6*TRIN2
      P0.7 ENABLE    P1.0            P2.7 *FAN1    P3.7*TRIN3
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xFD;
   P1MDOUT = 0xFF;
   P2MDOUT = 0x0F;
   P3MDOUT = 0x0F;

   XBR2 = 0x40; // disable UART1

   P1 = 0xFF;
   P2 = 0xFF;
   P3 = 0xFF;

   /* initial EEPROM value */
   if (init) {
      user_data.enable = 0;
      user_data.dac    = 0;
      user_data.gain   = 1;
   }

   write_tdac();

   /* write digital output and DAC */
   for (i=0 ; i<2 ; i++)
      user_write(i);
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   switch (index) {

   case 0: ENABLE = user_data.enable; break;
   case 1: write_dac(user_data.dac);  break;
   case 2: update = 1;  break;
   case 3: update = 1;  break;
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

/*---- DAC function ------------------------------------------------*/

void write_dac(float value) reentrant
{
   unsigned short d;
   unsigned char i;

   if (value > 2.5)
      value = 2.5;

   value -= user_data.offset;
   value *= user_data.gain;
   d = value/2.5*65535;

   DAC_SCK = 1;
   DELAY_US(1);
   DAC_NCS = 0; // chip select

   for (i=0 ; i<8 ; i++) {
      DAC_SCK = 1;
      DELAY_US(1);
      DAC_DIN = 0; // normal operation
      DELAY_US(1);
      DAC_SCK = 0;
      DELAY_US(1);
   }
   
   for (i=0 ; i<16 ; i++) {
      DAC_SCK = 1;
      DELAY_US(1);
      DAC_DIN = (d & 0x8000) > 0;
      DELAY_US(1);
      DAC_SCK = 0;
      d <<= 1;
   }

   DAC_NCS = 1; // remove chip select
}

/*---- TDAC function ------------------------------------------------*/

void write_tdac(void)
{
unsigned short dllt, dhlt;
unsigned char i, m, b, slot;

   /* convert to bits */
   if (user_data.llt > 2.048)
      user_data.llt = 2.048;
   if (user_data.hlt > 4.096)
      user_data.hlt = 4.096;
   dllt = user_data.llt/2.048*65535;
   dhlt = user_data.hlt/4.096*65535;

   TDAC_CLK = 0;
   TDAC_NCS = 0; // chip select
   DELAY_US(1);

   for (slot = 0 ; slot < 8 ; slot++) {
      
      //==== high level threshold
 
      // 8 bits don't care
      for (i=0 ; i<8 ; i++) {
         TDAC_SDI = 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }

      // command 3: write and update
      for (i=0,m=8 ; i<4 ; i++) {
         TDAC_SDI = (3 & m) > 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }
   
      // address all channels
      for (i=0,m=8 ; i<4 ; i++) {
         TDAC_SDI = 1;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }
   
      // MSB
      b = dhlt >> 8;
      for (i=0,m=0x80 ; i<8 ; i++) {
         TDAC_SDI = (b & m) > 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }
   
      // LSB
      b = dhlt & 0xFF;
      for (i=0,m=0x80 ; i<8 ; i++) {
         TDAC_SDI = (b & m) > 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }

      watchdog_refresh(1);
      DELAY_US(10);

      //==== low level threshold

      // 8 bits don't care
      for (i=0 ; i<8 ; i++) {
         TDAC_SDI = 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }

      // command 3: write and update
      for (i=0,m=8 ; i<4 ; i++) {
         TDAC_SDI = (3 & m) > 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }
   
      // address all channels
      for (i=0,m=8 ; i<4 ; i++) {
         TDAC_SDI = 1;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }
   
      // MSB
      b = dllt >> 8;
      for (i=0,m=0x80 ; i<8 ; i++) {
         TDAC_SDI = (b & m) > 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }
   
      // LSB
      b = dllt & 0xFF;
      for (i=0,m=0x80 ; i<8 ; i++) {
         TDAC_SDI = (b & m) > 0;
         DELAY_US(1);
         TDAC_CLK = 1;
         DELAY_US(1);
         TDAC_CLK = 0;
         m >>= 1;
      }

      watchdog_refresh(1);
      DELAY_US(10);
   }

   TDAC_NCS = 1; // remove chip select
   watchdog_refresh(1);
}

/*---- User loop function ------------------------------------------*/

unsigned short fan1, fan2, fan3;
unsigned long fan_last = 0;
bit old_fan1, old_fan2, old_fan3;

void user_loop(void)
{
   if (FAN1 && !old_fan1)
      fan1++;

   if (FAN2 && !old_fan2)
      fan2++;

   if (FAN3 && !old_fan3)
      fan3++;

   old_fan1 = FAN1;
   old_fan2 = FAN2;
   old_fan3 = FAN3;

   if (update) {
      update = 0;
      write_tdac();
   }

   if (time() - fan_last >= 100) {
      update = 1;
      DISABLE_INTERRUPTS;
      user_data.fan1 = fan1*60;
      user_data.fan2 = fan2*60;
      user_data.fan3 = fan3*60;
      ENABLE_INTERRUPTS;

      if (fan1 > 0 && fan2 > 0 && fan3 > 0)
         led_blink(0, 1, 50);

      fan1 = fan2 = fan3 = 0;
      fan_last = time();
   }

}

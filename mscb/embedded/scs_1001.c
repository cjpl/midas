/********************************************************************\

  Name:         scs_1001.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-1001 stand alone control unit

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-1001";
char code svn_revision[] = "$Id$";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern lcd_menu(void);

/*---- Port definitions ----*/

sbit RELAIS0  = P3 ^ 0;
sbit RELAIS1  = P3 ^ 1;
sbit RELAIS2  = P3 ^ 2;
sbit RELAIS3  = P3 ^ 3;

sbit DOUT0    = P3 ^ 7;
sbit DOUT1    = P3 ^ 6;
sbit DOUT2    = P3 ^ 5;
sbit DOUT3    = P3 ^ 4;

sbit SRCLK    = P1 ^ 7;
sbit SRIN     = P1 ^ 6;
sbit SRSTROBE = P1 ^ 5;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char relais[4];
   unsigned char dout[4];
   float dac[2];
   unsigned char din[4];
   float adc[8];
   float dofs[2];
   float dgain[2];
   float aofs[8];
   float again[8];
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Relais0", &user_data.relais[0], 1, 0, 1, 1 }, // 0
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Relais1", &user_data.relais[1], 1, 0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Relais2", &user_data.relais[2], 1, 0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Relais3", &user_data.relais[3], 1, 0, 1, 1 },

   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout0",   &user_data.dout[0],   1, 0, 1, 1 }, // 4
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout1",   &user_data.dout[1],   1, 0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout2",   &user_data.dout[2],   1, 0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Dout3",   &user_data.dout[3],   1, 0, 1, 1 },

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC0",    &user_data.dac[0],  2, -10, 10, 0.01 }, // 8
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "DAC1",    &user_data.dac[1],  2, -10, 10, 0.01 },

   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Din0",    &user_data.din[0] },  // 10
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Din1",    &user_data.din[1] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Din2",    &user_data.din[2] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,        "Din3",    &user_data.din[3] },

   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC0",    &user_data.adc[0] },  // 14
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC1",    &user_data.adc[1] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC2",    &user_data.adc[2] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC3",    &user_data.adc[3] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC4",    &user_data.adc[4] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC5",    &user_data.adc[5] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC6",    &user_data.adc[6] },
   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT, "ADC7",    &user_data.adc[7] },

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DOFS0",  &user_data.dofs[0],  3,-0.1, 0.1, 0.001 }, // 22
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DOFS1",  &user_data.dofs[1],  3,-0.1, 0.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DGAIN0", &user_data.dgain[0], 3, 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "DGAIN1", &user_data.dgain[1], 3, 0.9, 1.1, 0.001 },

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS0",  &user_data.aofs[0], 3,-0.1, 0.1, 0.001 }, // 26
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS1",  &user_data.aofs[1], 3,-0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS2",  &user_data.aofs[2], 3,-0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS3",  &user_data.aofs[3], 3,-0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS4",  &user_data.aofs[4], 3,-0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS5",  &user_data.aofs[5], 3,-0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS6",  &user_data.aofs[6], 3,-0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS7",  &user_data.aofs[7], 3,-0.1, 0.1, 0.001 },

   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN0", &user_data.again[0], 3, 0.9, 1.1, 0.001 }, // 34
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN1", &user_data.again[1], 3, 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN2", &user_data.again[2], 3, 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN3", &user_data.again[3], 3, 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN4", &user_data.again[4], 3, 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN5", &user_data.again[5], 3, 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN6", &user_data.again[6], 3, 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN7", &user_data.again[7], 3, 0.9, 1.1, 0.001 },


   //## test
//   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_REMIN,  "RemAdc",  &user_data.rem_adc, 0, 0, 0, 2, 0 },
//   { 4, UNIT_VOLT, 0, 0, MSCBF_FLOAT | MSCBF_REMOUT, "RemDac",  &user_data.rem_dac, 0, 0, 0, 1, 2 },
//   { 1, UNIT_BOOLEAN, 0, 0, MSCBF_REMIN | MSCBF_REMOUT, "RemRel0",  &user_data.remrel[0], 0, 1, 1, 1, 0 },

   { 0 }
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;
   char xdata str[64];

   SFRPAGE = ADC0_PAGE;
   AMX0CF = 0x00;               // select single ended analog inputs
   ADC0CF = 0xE0;               // 16 system clocks, gain 1
   ADC0CN = 0x80;               // enable ADC 
   REF0CN = 0x00;               // use external voltage reference

   SFRPAGE = LEGACY_PAGE;
   REF0CN = 0x02;               // select external voltage reference

   SFRPAGE = DAC0_PAGE;
   DAC0CN = 0x80;               // enable DAC0
   SFRPAGE = DAC1_PAGE;
   DAC1CN = 0x80;               // enable DAC1

   /* open drain(*) /push-pull: 
      P0.0 TX1      P1.0 TP6          P2.0 LED1         P3.0 RELAIS0
      P0.1*RX1      P1.1 BACKLIGHT    P2.1 LCD_E        P3.1 RELAIS1
      P0.2 TX2      P1.2 TP5          P2.2 LCD_RW       P3.2 RELAIS2
      P0.3*RX2      P1.3 TP7          P2.3 LCD_RS       P3.3 RELAIS3
                                                                      
      P0.4 EN1      P1.4 WATCHDOG     P2.4 LCD_DB4      P3.4 DOUT0
      P0.5 EN2      P1.5 SRSTROBE     P2.5 LCD_DB5      P3.5 DOUT1
      P0.6 LED2     P1.6*SRIN         P2.6 LCD_DB6      P3.6 DOUT2
      P0.7 BUZZER   P1.7 SRCLK        P2.7 LCD_DB7      P3.7 DOUT3
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xF5;
   P1MDOUT = 0xBF;
   P2MDOUT = 0xFF;
   P3MDOUT = 0xFF;

   /* initial EEPROM value */
   if (init) {
      for (i=0 ; i<4 ; i++)
         user_data.relais[i] = 0;

      for (i=0 ; i<4 ; i++)
         user_data.dout[i] = 0;

      for (i=0 ; i<2 ; i++) {
         user_data.dac[i] = 0;
         user_data.dofs[i] = 0;
         user_data.dgain[i] = 1;
      }

      for (i=0 ; i<8 ; i++) {
         user_data.aofs[i] = 0;
         user_data.again[i] = 1;
      }
   }

   /* write digital outputs */
   for (i=0 ; i<8 ; i++)
      user_write(i);

   /* write DACs */
   user_write(8);
   user_write(9);

   /* write remote variables */
   for (i = 0; variables[i].width; i++)
      if (variables[i].flags & MSCBF_REMOUT)
         send_remote_var(i);

   /* display startup screen */
   lcd_goto(0, 0);
   for (i=0 ; i<7-strlen(sys_info.node_name)/2 ; i++)
      puts(" ");
   puts("** ");
   puts(sys_info.node_name);
   puts(" **");
   lcd_goto(0, 1);
   printf("   Address:  %04X", sys_info.node_addr);
   lcd_goto(0, 2);
   strcpy(str, svn_revision + 16);
   *strchr(str, ' ') = 0;
   printf("  Revision:  %s", str);
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   unsigned short d;

   switch (index) {

   /* RELAIS go through inverter */
   case 0: RELAIS0 = !user_data.relais[0]; break;
   case 1: RELAIS1 = !user_data.relais[1]; break;
   case 2: RELAIS2 = !user_data.relais[2]; break;
   case 3: RELAIS3 = !user_data.relais[3]; break;

   /* DOUT go through inverter */
   case 4: DOUT0 = !user_data.dout[0]; break;
   case 5: DOUT1 = !user_data.dout[1]; break;
   case 6: DOUT2 = !user_data.dout[2]; break;
   case 7: DOUT3 = !user_data.dout[3]; break;

   case 8:                     // DAC0
   case 22:                    // DOFS0
   case 24:                    // DGAIN0
      /* assume -10V..+10V range */
      d = (((user_data.dac[0] + user_data.dofs[0]) * user_data.dgain[0] + 10) / 20) * 0x1000;
      if (d >= 0x1000)
         d = 0x0FFF;
      if (d < 0)
         d = 0;

      SFRPAGE = DAC0_PAGE;
      DAC0L = d & 0xFF;
      DAC0H = d >> 8;
      break;

   case 9:                     // DAC1
   case 23:                    // DOFS1
   case 25:                    // DGAIN1
      /* assume -10V..+10V range */
      d = (((user_data.dac[1] + user_data.dofs[1]) * user_data.dgain[1] + 10) / 20) * 0x1000;
      if (d >= 0x1000)
         d = 0x0FFF;
      if (d < 0)
         d = 0;

      SFRPAGE = DAC1_PAGE;
      DAC1L = d & 0xFF;
      DAC1H = d >> 8;
      break;
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

/*---- ADC read function -------------------------------------------*/

void adc_read(channel, float *d)
{
   unsigned long value;
   unsigned int i, n;
   float gvalue;

   SFRPAGE = ADC0_PAGE;
   AMX0SL = channel & 0x0F;
   ADC0CF = 0xE0;               // 16 system clocks, gain 1

   n = 1 << 10;                 // average 1024 times

   value = 0;
   for (i = 0; i < n; i++) {
      DISABLE_INTERRUPTS;
  
      SFRPAGE = ADC0_PAGE;
      AD0INT = 0;
      AD0BUSY = 1;
      while (!AD0INT);          // wait until conversion ready, does NOT work with ADBUSY!

      ENABLE_INTERRUPTS;

      value += (ADC0L | (ADC0H << 8));
      yield();
   }

   value >>= 6;

   if (channel < 2)
      gvalue = value / 65536.0 * 20 - 10; // +- 10V range
   else
      gvalue = value / 65536.0 * 10; // 0...10V range

   gvalue -= user_data.aofs[channel];
   gvalue *= user_data.again[channel];

   DISABLE_INTERRUPTS;
   *d = gvalue;
   ENABLE_INTERRUPTS;
}

/*---- Shift register read function --------------------------------*/

bit b0, b1, b2, b3;

#define SHIFT SRCLK = 1; delay_us(1); SRCLK = 0; delay_us(1)

void sr_read()
{
   SRCLK = 0;
   SRIN = 1;
   SRSTROBE = 1;
   delay_us(1);
   SRSTROBE = 0;

   user_data.din[2] = SRIN;
   SHIFT;
   user_data.din[3] = SRIN;
   SHIFT;
   user_data.din[0] = SRIN;
   SHIFT;
   user_data.din[1] = SRIN;

   SHIFT;
   b3 = SRIN;
   SHIFT;
   b2 = SRIN;
   SHIFT;
   b1 = SRIN;
   SHIFT;
   b0 = SRIN;
}

/*---- Application display -----------------------------------------*/

unsigned char application_display(bit init)
{
static bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0;
unsigned char i;

   if (init)
      lcd_clear();

   /* display first two ADCs */
   for (i=0 ; i<3 ; i++) {
      lcd_goto(0, i);
      printf("%bd:%6.3fV", 0+i*2, user_data.adc[0+i*2]);
      lcd_goto(11, i);
      printf("%bd:%6.3fV", 1+i*2, user_data.adc[1+i*2]);
   }

   /* show state of relais 0-2 */
   for (i=0 ; i<3 ; i++) {
      lcd_goto(i*5+i/2, 3);
      printf("R%bd:%bd", i, user_data.relais[i]);
   }

   lcd_goto(16, 3);
   printf("MENU");

   /* toggle relais 0 with button 0 */
   if (b0 && !b0_old) {
      user_data.relais[0] = !user_data.relais[0];
      user_write(0);
   }

   /* toggle relais 1 with button 1 */
   if (b1 && !b1_old) {
      user_data.relais[1] = !user_data.relais[1];
      user_write(1);
   }

   /* toggle relais 2 with button 2 */
   if (b2 && !b2_old) {
      user_data.relais[2] = !user_data.relais[2];
      user_write(2);
   }

   /* enter menu on release of button 3 */
   if (!init)
      if (!b3 && b3_old)
         return 1;

   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   static unsigned char adc_chn = 0;

   /* read one ADC channel */
   adc_read(adc_chn, &user_data.adc[adc_chn]);
   adc_chn = (adc_chn + 1) % 8;

   /* read buttons and digital input */
   sr_read();

   /* mangae menu on LCD display */
   lcd_menu();
}

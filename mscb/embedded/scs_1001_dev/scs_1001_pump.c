/********************************************************************\

  Name:         scs_1001.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-1001 stand alone control unit

  $Log$
  Revision 1.1  2005/06/24 18:50:48  ritt
  Initial revision

  Revision 1.3  2005/03/08 12:41:32  ritt
  Version 1.9.0

  Revision 1.2  2005/02/16 14:17:58  ritt
  Two-speed menu increment

  Revision 1.1  2005/02/16 13:17:25  ritt
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "PUMP-STATION";

#define PS_VERSION 1

#define TC600_ADDRESS 1   // Address of turbo pump via RS486 bus

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern lcd_menu(void);

unsigned char tc600_write(unsigned short param, unsigned char len, unsigned long value);

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
   unsigned char turbo_on;
   unsigned char vent_on;
   unsigned short rot_speed;
   float tmp_current;
   unsigned char din[4];
   unsigned char mv_close;
   unsigned char bv_open;
   unsigned char bv_close;
   float hv_mbar;
   float fv_mbar;
   float adc[8];
   float aofs[8];
   float again[8];
} xdata user_data;

MSCB_INFO_VAR code variables[] = {

   { 1, UNIT_BOOLEAN, 0, 0, 0,           "Forepump", &user_data.relais[0], 0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "Forevlv",  &user_data.relais[1], 0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "Mainvlv",  &user_data.relais[2], 0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "Bypvlv",   &user_data.relais[3], 0, 1, 1 },

   { 1, UNIT_BOOLEAN, 0, 0,           0, "Turbo on", &user_data.turbo_on,  0, 1, 1 },
   { 1, UNIT_BOOLEAN, 0, 0,           0, "Vent on",  &user_data.vent_on,   0, 1, 1 },
   { 2, UNIT_HERTZ,   0, 0,           0, "RotSpd",   &user_data.rot_speed, },
   { 4, UNIT_AMPERE,  0, 0, MSCBF_FLOAT, "TMPcur",   &user_data.tmp_current, },

   { 1, UNIT_BOOLEAN, 0, 0, 0,           "FP on",    &user_data.din[0] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "FV open",  &user_data.din[1] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "FV close", &user_data.din[2] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "MV open",  &user_data.din[3] },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "MV close", &user_data.mv_close },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "BV open",  &user_data.bv_open  },
   { 1, UNIT_BOOLEAN, 0, 0, 0,           "BV close", &user_data.bv_close },

   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,          "HV",      &user_data.hv_mbar },
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,          "FV",      &user_data.fv_mbar },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC0",    &user_data.adc[0] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC1",    &user_data.adc[1] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC2",    &user_data.adc[2] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC3",    &user_data.adc[3] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC4",    &user_data.adc[4] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC5",    &user_data.adc[5] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC6",    &user_data.adc[6] },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC7",    &user_data.adc[7] },

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS0",  &user_data.aofs[0], -0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS1",  &user_data.aofs[1], -0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS2",  &user_data.aofs[2], -0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS3",  &user_data.aofs[3], -0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS4",  &user_data.aofs[4], -0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS5",  &user_data.aofs[5], -0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS6",  &user_data.aofs[6], -0.1, 0.1, 0.001 },
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS7",  &user_data.aofs[7], -0.1, 0.1, 0.001 },

   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN0", &user_data.again[0], 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN1", &user_data.again[1], 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN2", &user_data.again[2], 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN3", &user_data.again[3], 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN4", &user_data.again[4], 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN5", &user_data.again[5], 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN6", &user_data.again[6], 0.9, 1.1, 0.001 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN7", &user_data.again[7], 0.9, 1.1, 0.001 },


   { 0 }
};

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

void user_init(unsigned char init)
{
   unsigned char i;

   led_mode(2, 0);              // buzzer off by default

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

      for (i=0 ; i<8 ; i++) {
         user_data.aofs[i] = 0;
         user_data.again[i] = 1;
      }
   }

   /* write digital outputs */
   for (i=0 ; i<4 ; i++)
      user_write(i);

   /* initialize UART1 for TC600 */
   uart_init(1, BD_9600);

   /* turn on turbo pump motor (not pump station) */
   tc600_write(23, 6, 111111);

   /* vent mode */
   tc600_write(30, 3, 0);

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
   printf("   Version:  %bd", PS_VERSION);
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   switch (index) {

   /* RELAIS go through inverter */
   case 0: RELAIS0 = !user_data.relais[0]; break;
   case 1: RELAIS1 = !user_data.relais[1]; break;
   case 2: RELAIS2 = !user_data.relais[2]; break;
   case 3: RELAIS3 = !user_data.relais[3]; break;

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

/*------------------------------------------------------------------*/

unsigned char tc600_read(unsigned short param, char *result)
{
   char idata str[32];
   unsigned char i, j, cs, len;

   sprintf(str, "%03Bd00%03d02=?", TC600_ADDRESS, param);

   for (i = cs = 0; i < 12; i++)
      cs += str[i];

   sprintf(str + 12, "%03Bd\r", cs);
   for (i=0 ; i<strlen(str); i++)
      putchar1(str[i]);
   flush();

   i = gets_wait(str, sizeof(str), 200);
   if (i > 2) {
      len = (str[8] - '0') * 10 + (str[9] - '0');

      for (j = 0; j < len; j++)
         result[j] = str[10 + j];

      result[j] = 0;
      return len;
   }

   return 0;
}

/*------------------------------------------------------------------*/

unsigned char tc600_write(unsigned short param, unsigned char len, unsigned long value)
{
   char idata str[32];
   unsigned char i, cs;

   if (len == 6)
      sprintf(str, "%03Bd10%03d06%06ld", TC600_ADDRESS, param, value);
   else if (len == 3)
      sprintf(str, "%03Bd10%03d03%03ld", TC600_ADDRESS, param, value);

   for (i = cs = 0; i < 16; i++)
      cs += str[i];

   sprintf(str + 16, "%03Bd\r", cs);
   for (i=0 ; i<strlen(str); i++)
      putchar1(str[i]);
   flush();
   i = gets_wait(str, sizeof(str), 200);

   return i;
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
   char idata str[32];
   static unsigned char xdata adc_chn = 0;
   static char xdata turbo_on_last = -1;
   static char xdata vent_on_last = -1;
   static unsigned long xdata last_read = 0;
 
   /* read one ADC channel */
   adc_read(adc_chn, &user_data.adc[adc_chn]);

   /* convert voltage to pressure, see data sheets */
   if (adc_chn == 0)
      user_data.hv_mbar  = pow(10, 1.667 * user_data.adc[0] - 11.33); // PKR 251
   if (adc_chn == 1)
      user_data.fv_mbar  = pow(10, user_data.adc[1] - 5.5);           // TPR 280
   if (adc_chn == 5)
      user_data.mv_close = user_data.adc[5] > 5;                     // ADC as digital input
   if (adc_chn == 6)
      user_data.bv_open  = user_data.adc[6] > 5;                     // ADC as digital input
   if (adc_chn == 7)
      user_data.bv_close = user_data.adc[7] > 5;                     // ADC as digital input

   adc_chn = (adc_chn + 1) % 8;

   /* read buttons and digital input */
   sr_read();

   /* read turbo pump parameters once each second */
   if (time() > last_read + 100) {
      last_read = time();

      if (user_data.turbo_on != turbo_on_last) {
         tc600_write(10, 6, user_data.turbo_on ? 111111 : 0);
         turbo_on_last = user_data.turbo_on;
      }

      if (user_data.vent_on != vent_on_last) {
         tc600_write(12, 6, user_data.vent_on ? 111111 : 0);
         vent_on_last = user_data.vent_on;
      }

      if (tc600_read(309, str))
         user_data.rot_speed = atoi(str);

      if (tc600_read(310, str))
         user_data.tmp_current = atoi(str) / 100.0;
   }
   
   /* mangae menu on LCD display */
   lcd_menu();
}

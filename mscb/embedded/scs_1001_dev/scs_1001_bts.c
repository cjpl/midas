/********************************************************************\

  Name:         scs_1001_bts.c
  Created by:   Stefan Ritt


  Contents:     BTS control program for SCS-1001 connected to one
                SCS-910

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mscb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "BTS";
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

/*---- Error codes ----*/

#define ERR_HE         (1<<0)

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char error;
   unsigned char bts_state;

   unsigned short ln2_on;
   unsigned short ln2_off;

   unsigned char ka_out;
   unsigned char ka_in;
   float         ka_level;

   unsigned char ln2_valve;
   unsigned char ln2_heater;
   float         ln2_valve_temp;
   float         ln2_heater_temp;

   float         jt_valve_forerun;
   float         jt_valve_return;
   float         jt_temp;

   float         lhe_level1; // main
   float         lhe_level2; // reserve

   float         ln2_temp1; // screen left
   float         ln2_temp2; // screen right
   float         ln2_temp3; // screen tower
   float         ln2_temp4; // screen top

   float         lhe_temp1a; // middle D9K
   float         lhe_temp2a; // left D9K
   float         lhe_temp3a; // right D9K
   float         lhe_temp1b; // middle TVO
   float         lhe_temp2b; // left TVO
   float         lhe_temp3b; // right TVO

   float         ln2_mbar;
   float         lhe_mbar;

   float         quench1;
   float         quench2;

   float adc[8];
   float aofs[8];
   float again[8];

   float scs_910[20];

} xdata user_data;

MSCB_INFO_VAR code variables[] = {

   { 1, UNIT_BYTE,    0, 0, 0,                               "Error",    &user_data.error },                    
   { 1, UNIT_BYTE,    0, 0, 0,                               "State",    &user_data.bts_state, 0, 9, 1 },
                                                                                                               
   { 2, UNIT_SECOND,  0, 0, 0,                               "LN2 on",   &user_data.ln2_on, 0, 120, 10 },
   { 2, UNIT_SECOND,  0, 0, 0,                               "LN2 off",  &user_data.ln2_off, 0, 3600, 10 },

   { 1, UNIT_BYTE,    0, 0, 0,                               "KA Out",   &user_data.ka_out, 0, 7, 1 },
   { 1, UNIT_BYTE,    0, 0, 0,                               "KA In",    &user_data.ka_in },
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,                     "KA Level", &user_data.ka_level },                    

   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "LN2 vlve", &user_data.ln2_valve, 0, 1, 1 },      
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "LN2 htr",  &user_data.ln2_heater, 0, 1, 1 },    
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LN2vlv T", &user_data.ln2_valve_temp },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LN2htr T", &user_data.ln2_heater_temp },
                                                                                                               
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,                     "JT vlveF", &user_data.jt_valve_forerun },
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,                     "JT vlveR", &user_data.jt_valve_return },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "JT T",     &user_data.jt_temp },

   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,                     "LHe lvl1", &user_data.lhe_level1 },
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,                     "LHe lvl2", &user_data.lhe_level2 },

   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LN2 T1",   &user_data.ln2_temp1 },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LN2 T2",   &user_data.ln2_temp2 },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LN2 T3",   &user_data.ln2_temp3 },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LN2 T4",   &user_data.ln2_temp4 },

   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LHE T1a",  &user_data.lhe_temp1a },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LHE T2a",  &user_data.lhe_temp2a },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LHE T3a",  &user_data.lhe_temp3a },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LHE T1b",  &user_data.lhe_temp1b },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LHE T2b",  &user_data.lhe_temp2b },
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,                     "LHE T3b",  &user_data.lhe_temp3b },

   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "LN2 P",    &user_data.ln2_mbar }, 
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "LHE P",    &user_data.lhe_mbar }, 

   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT,                     "Quench1",  &user_data.quench1 }, 
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT,                     "Quench2",  &user_data.quench2 }, 

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC0",     &user_data.adc[0] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC1",     &user_data.adc[1] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC2",     &user_data.adc[2] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC3",     &user_data.adc[3] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC4",     &user_data.adc[4] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC5",     &user_data.adc[5] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC6",     &user_data.adc[6] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC7",     &user_data.adc[7] },                    
                                                                                                                
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS0",    &user_data.aofs[0], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS1",    &user_data.aofs[1], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS2",    &user_data.aofs[2], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS3",    &user_data.aofs[3], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS4",    &user_data.aofs[4], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS5",    &user_data.aofs[5], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS6",    &user_data.aofs[6], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS7",    &user_data.aofs[7], -0.1, 0.1, 0.001 }, 
                                                                                                                
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN0",   &user_data.again[0], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN1",   &user_data.again[1], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN2",   &user_data.again[2], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN3",   &user_data.again[3], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN4",   &user_data.again[4], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN5",   &user_data.again[5], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN6",   &user_data.again[6], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN7",   &user_data.again[7], 0.9, 1.1, 0.001 }, 

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_00", &user_data.scs_910[0], 0, 0, 0, 1, 0 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_01", &user_data.scs_910[1], 0, 0, 0, 1, 1 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_02", &user_data.scs_910[2], 0, 0, 0, 1, 2 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_03", &user_data.scs_910[3], 0, 0, 0, 1, 3 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_04", &user_data.scs_910[4], 0, 0, 0, 1, 4 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_05", &user_data.scs_910[5], 0, 0, 0, 1, 5 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_06", &user_data.scs_910[6], 0, 0, 0, 1, 6 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_07", &user_data.scs_910[7], 0, 0, 0, 1, 7 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_08", &user_data.scs_910[8], 0, 0, 0, 1, 8 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_09", &user_data.scs_910[9], 0, 0, 0, 1, 9 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_10", &user_data.scs_910[10], 0, 0, 0, 1, 10 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_11", &user_data.scs_910[11], 0, 0, 0, 1, 11 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_12", &user_data.scs_910[12], 0, 0, 0, 1, 12 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_13", &user_data.scs_910[13], 0, 0, 0, 1, 13 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_14", &user_data.scs_910[14], 0, 0, 0, 1, 14 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_15", &user_data.scs_910[15], 0, 0, 0, 1, 15 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_16", &user_data.scs_910[16], 0, 0, 0, 1, 16 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_17", &user_data.scs_910[17], 0, 0, 0, 1, 17 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_18", &user_data.scs_910[18], 0, 0, 0, 1, 18 },    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN | MSCBF_REMIN, "910_19", &user_data.scs_910[19], 0, 0, 0, 1, 19 },

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

      for (i=0 ; i<8 ; i++) {
         user_data.aofs[i] = 0;
         user_data.again[i] = 1;
      }

	  user_data.ln2_on = 30;
	  user_data.ln2_off = 180;
   }

   /* initialize UART1 for SCS_910 */
   uart_init(1, BD_115200);

   /* display startup screen */
   lcd_goto(0, 0);
   for (i=0 ; i<7-strlen(sys_info.node_name)/2 ; i++)
      puts(" ");
   puts("** ");
   puts(sys_info.node_name);
   puts(" **");
   lcd_goto(0, 1);
   printf("  Address:   %04X", sys_info.node_addr);
   lcd_goto(0, 2);
   strcpy(str, svn_revision + 20);
   *strchr(str, ' ') = 0;
   printf("  Revision:  %s", str);

   user_data.error = 0;
   user_data.bts_state = 0;
   user_data.ka_out = 0; // 1
   user_data.ln2_valve = 0;
   user_data.ln2_heater = 0;

   /* write digital outputs */
   for (i=0 ; i<10 ; i++)
      user_write(i);

}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
float curr;
unsigned short d;

   switch (index) {

   /* digital outputs go through inverter */

   case 2: DOUT1 = (user_data.ka_out & (1<<0)) == 0;
           DOUT2 = (user_data.ka_out & (1<<1)) == 0;
           DOUT3 = (user_data.ka_out & (1<<2)) == 0; break;
   
   /* LN2 valve has inverse logic (closes on power) */
   case 5: RELAIS0 = user_data.ln2_valve; break; 

   case 6: DOUT0   = !user_data.ln2_heater; break;
   
   /* "Vorlauf" JT-valve */
   case 11:
      /* convert % to 4-20mA current */
	   curr = 4+16*user_data.jt_valve_forerun/100.0;

      /* 0...24mA range */
      d = (curr / 24.0) * 0x1000;
      if (d >= 0x1000)
         d = 0x0FFF;
      if (d < 0)
         d = 0;

      SFRPAGE = DAC0_PAGE;
      DAC0L = d & 0xFF;
      DAC0H = d >> 8;
      break;

   /* "Ruecklauf" JT-valve */
   case 12:
      /* convert % to 4-20mA current */
	   curr = 4+16*user_data.jt_valve_return/100.0;

      /* 0...24mA range */
      d = (curr / 24.0) * 0x1000;
      if (d >= 0x1000)
         d = 0x0FFF;
      if (d < 0)
         d = 0;

      SFRPAGE = DAC0_PAGE;
      DAC0L = d & 0xFF;
      DAC0H = d >> 8;
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

void dround(float *v, unsigned char digits)
{
unsigned char i;
float b;

   for (i=0,b=1 ; i<digits ; i++)
      b *= 10;
   *v = floor((*v * b) + 0.5) / b;
}

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
   dround(&gvalue, 3);

   DISABLE_INTERRUPTS;
   *d = gvalue;
   ENABLE_INTERRUPTS;
}

/*---- Shift register read function --------------------------------*/

bit b0, b1, b2, b3;

#define SHIFT SRCLK = 1; delay_us(1); SRCLK = 0; delay_us(1)

void sr_read()
{
unsigned char d;

   SRCLK = 0;
   SRIN = 1;
   SRSTROBE = 1;
   delay_us(1);
   SRSTROBE = 0;

   d = 0;
   if (SRIN)
     d |= (1<<2);
   SHIFT;
   if (SRIN)
     d |= (1<<3);
   SHIFT;
   if (SRIN)
     d |= (1<<0);
   SHIFT;
   if (SRIN)
     d |= (1<<1);
   SHIFT;

   user_data.ka_in = d & 0x07;

   b3 = SRIN;
   SHIFT;
   b2 = SRIN;
   SHIFT;
   b1 = SRIN;
   SHIFT;
   b0 = SRIN;
}

/*------------------------------------------------------------------*/

unsigned char application_display(bit init)
{
static bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0;
static long xdata last_ln2time = 0, last_b;
//unsigned short xdata delta;

   if (init)
      lcd_clear();

   /* display pressures */
   lcd_goto(0, 0);
   printf("TT:%5.1fK", user_data.ln2_temp3);
   lcd_goto(10, 0);
   printf("TV:%5.1fK", user_data.ln2_valve_temp);
   
   lcd_goto(0, 1);
   printf("He:%5.1f%%", user_data.lhe_level1);
   lcd_goto(10, 1);
   printf("TH:%5.1f%K", user_data.lhe_temp1b);

   lcd_goto(0, 2);
   printf("JT:%5.1f%%", user_data.jt_valve_forerun);
   lcd_goto(10, 2);
   printf("TJ:%5.1f%K", user_data.jt_temp);

   lcd_goto(0, 3);
   if (user_data.bts_state == 0)
      printf("LN2OFF");
   else if (user_data.bts_state == 1)
      printf("LN2ON ");
   else
      printf("LN2AUT");

   lcd_goto(6, 3);
   printf(user_data.ln2_heater ? "HON " : "HOFF");

   lcd_goto(13, 3);
   printf("+ HE -");

   /* toggle state with button 0 */
   if (b0 && !b0_old) {
      last_ln2time = time();
      user_data.bts_state = (user_data.bts_state + 1) % 3;

      if (user_data.bts_state == 0)
         user_data.ln2_valve = 0;
      else if (user_data.bts_state == 1)
         user_data.ln2_valve = 1;
      user_write(5);
   }

   /* toggle heater with button 1 */
   if (b1 && !b1_old) {
      user_data.ln2_heater = !user_data.ln2_heater;
      user_write(6);
   }

   /* increase HE flow with button 2 */
   if (b2) {
      if (!b2_old) {
         user_data.jt_valve_forerun += 1;
         last_b = time();
      }
      if (time() > last_b + 70)
         user_data.jt_valve_forerun += 1;
      if (time() > last_b + 300)
         user_data.jt_valve_forerun += 9;
      if (user_data.jt_valve_forerun > 100)
         user_data.jt_valve_forerun = 100;
      user_write(11);
   }

   /* decrease HE flow with button 3 */
   if (b3) {
      if (!b3_old) {
         user_data.jt_valve_forerun -= 1;
         last_b = time();
      }
      if (time() > last_b + 70)
         user_data.jt_valve_forerun -= 1;
      if (time() > last_b + 300)
         user_data.jt_valve_forerun -= 9;
      if (user_data.jt_valve_forerun < 0)
         user_data.jt_valve_forerun = 0;
      user_write(11);
   }

   /* enter menu on release of button 2 & 3 */
   if (b2 && b3) {
      while (b2 || b3)
         sr_read();
      return 1;
   }

   /* swith ln2 valve on by specified time */
   if (user_data.bts_state == 2 && 
       user_data.ln2_valve == 0 && 
       time() - last_ln2time > user_data.ln2_off * 100) {
      last_ln2time = time();
	  user_data.ln2_valve = 1;
	  user_write(5);
   }

   /* swith ln2 valve off by specified time */
   if (user_data.bts_state == 2 && 
       user_data.ln2_valve == 1 && 
       time() - last_ln2time > user_data.ln2_on * 100) {
      last_ln2time = time();
	  user_data.ln2_valve = 0;
	  user_write(5);
   }

   /*
   lcd_goto(14, 3);
   delta = (unsigned short)((time() - last_time) / 100);
   if (user_data.bts_state == 2) {
	   if (user_data.ln2_valve == 0)
	      printf("%3d s", user_data.ln2_off - delta);
	   if (user_data.ln2_valve == 1) 
	      printf("%3d s", user_data.ln2_on - delta);
   } else
      printf("     ");
   */

   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   static unsigned char xdata adc_chn = 0;
   float xdata x, temp;
 
   /* read one ADC channel */
   adc_read(adc_chn, &user_data.adc[adc_chn]);

   /* convert voltage to temperatures */
   if (adc_chn == 0)
      user_data.ln2_mbar = pow(10, 1.667 * user_data.adc[0] - 11.33); // PKR 251

   if (adc_chn == 1)
      user_data.lhe_mbar = pow(10, 1.667 * user_data.adc[1] - 11.33); // PKR 251

   if (adc_chn == 2) {
      // convert CLTS voltage to Ohm (1mA current)
      x = user_data.adc[2] / 0.001;

      // convert Ohm to Temperature, 24deg: 290 Ohm, -270deg: 220 Ohm
      x = (x-220.0)*(24.0+270.0)/(290.0-220.0) - 270.0;
      user_data.jt_temp = x; 
      user_data.jt_temp = 0;
   }

   adc_chn = (adc_chn + 1) % 8;

   /* convert SCS_910 voltges to degree */
   temp = (user_data.scs_910[0]*1000.0 - 224) / -2.064 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.ln2_valve_temp = temp;
   temp = (user_data.scs_910[1]*1000.0 - 224) / -2.064 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.ln2_heater_temp = temp;

   temp = (user_data.scs_910[2]*1000.0 - 224) / -2.064 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.ln2_temp1 = temp;
   temp = (user_data.scs_910[3]*1000.0 - 213) / -2.087 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.ln2_temp2 = temp;
   temp = (user_data.scs_910[4]*1000.0 - 229) / -1.984 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.ln2_temp3 = temp;
   temp = (user_data.scs_910[5]*1000.0 - 243) / -1.946 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.ln2_temp4 = temp;

   temp = (user_data.scs_910[6]*1000.0 - 228) / -2.018 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.lhe_temp1a = temp;
   temp = (user_data.scs_910[7]*1000.0 - 230) / -1.996 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.lhe_temp2a = temp;
   temp = (user_data.scs_910[8]*1000.0 - 230) / -1.981 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   user_data.lhe_temp3a = temp;

   if (user_data.scs_910[9] != 0) {
      x = 1/user_data.scs_910[9];
	   temp = 0.1189*x*x*x*x-1.8979*x*x*x+12.401*x*x-35.427*x+40.5; // R5
      if (temp < 0 || temp > 999) temp = 0;
   } else 
      temp = 0;
   user_data.lhe_temp1b = temp;

   if (user_data.scs_910[10] != 0) {
      x = 1/user_data.scs_910[10];
	   temp = 0.1403*x*x*x*x-2.3273*x*x*x+15.733*x*x-46.722*x+54.5; // R6
      if (temp < 0 || temp > 999) temp = 0;
   } else
      temp = 0;
   user_data.lhe_temp2b = temp;

   if (user_data.scs_910[11] != 0) {
      x = 1/user_data.scs_910[11];
	   temp = 0.2491*x*x*x*x-4.3396*x*x*x+29.209*x*x-85.018*x+93.4; // R1
      if (temp < 0 || temp > 999) temp = 0;
   } else
      temp = 0;
   user_data.lhe_temp3b = temp;
   
   x = user_data.scs_910[12]*6;
   x = (1-x/7.32)*100;
   if (x < 0)
      x = 0;
   user_data.lhe_level1 = x;

   /* read buttons and digital input */
   sr_read();
   
   /* mangae menu on LCD display */
   lcd_menu();

   /* watchdog test */
   if (user_data.error == 123)
      while(1);
}

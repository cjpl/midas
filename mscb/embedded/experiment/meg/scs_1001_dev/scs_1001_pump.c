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
#include <stdlib.h>
#include <math.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "PUMP-STATION";
char code svn_revision[] = "$Id$";

#define TC600_ADDRESS 1   // Address of turbo pump via RS486 bus

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern lcd_menu(void);

unsigned char tc600_read(unsigned short param, char *result);
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

/*---- Error codes ----*/

#define ERR_TURBO_COMM (1<<0)
#define ERR_FOREVAC    (1<<1)
#define ERR_MAINVAC    (1<<2)
#define ERR_TURBO      (1<<3)

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

char xdata turbo_err[10];

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char error;
   unsigned char station_on;
   unsigned char valve_locked;
   unsigned char vacuum_ok;
   unsigned char man_mode;
   unsigned char relais[4];
   unsigned char turbo_on;
   unsigned short rot_speed;
   float tmp_current;
   unsigned char din[4];
   unsigned char hv_close;
   unsigned char bv_open;
   unsigned char bv_close;
   float hv_mbar;
   float vv_mbar;
   unsigned short final_speed;
   unsigned short evac_timeout;
   unsigned short fp_cycle;
   float vv_max;
   float vv_min;
   float hv_thresh;
   float adc[8];
   float aofs[8];
   float again[8];
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 1, UNIT_BYTE,    0, 0, 0,                               "Error",    &user_data.error },                        // 0
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Station",  &user_data.station_on, 0, 0, 1, 1 },       // 1
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Locked",   &user_data.valve_locked, 0, 0, 1, 1 },     // 2
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Vac OK",   &user_data.vacuum_ok,  0, 0, 1, 1 },       // 3
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "ManMode",  &user_data.man_mode,  0, 0, 1, 1 },        // 4
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Forepump", &user_data.relais[0], 0, 0, 1, 1 },        // 5
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Forevlv",  &user_data.relais[1], 0, 0, 1, 1 },        // 6
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Mainvlv",  &user_data.relais[2], 0, 0, 1, 1 },        // 7
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Bypvlv",   &user_data.relais[3], 0, 0, 1, 1 },        // 8
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Turbo on", &user_data.turbo_on,  0, 0, 1, 1 },        // 9
   { 2, UNIT_HERTZ,   0, 0, 0,                               "RotSpd",   &user_data.rot_speed, },                   // 10
   { 4, UNIT_AMPERE,  0, 0, MSCBF_FLOAT,                     "TMPcur",   &user_data.tmp_current, },                 // 11
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "VP on",    &user_data.din[0] },                       // 12
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "VV open",  &user_data.din[1] },                       // 13
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "VV close", &user_data.din[2] },                       // 14
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "HV open",  &user_data.din[3] },                       // 15
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "HV close", &user_data.hv_close },                     // 16
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "BV open",  &user_data.bv_open  },                     // 17
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "BV close", &user_data.bv_close },                     // 18
                                                                                                               
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "HV",       &user_data.hv_mbar },                      // 19
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "VV",       &user_data.vv_mbar },                      // 20
                                                                                                               
   { 2, UNIT_HERTZ,   0, 0, MSCBF_HIDDEN,                    "FinSpd",   &user_data.final_speed },                  // 21
   { 2, UNIT_MINUTE,  0, 0, MSCBF_HIDDEN,                    "Timeout",  &user_data.evac_timeout, 0, 0, 300, 10 },  // 22
   { 2, UNIT_SECOND,  0, 0, MSCBF_HIDDEN,                    "FP cycle", &user_data.fp_cycle, 0, 10, 600, 10 },     // 23
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "VV max",   &user_data.vv_max, 1, 1, 10, 1 },          // 24
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "VV min",   &user_data.vv_min, 1, 0.1, 1, 0.1 },       // 25
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "HV thrsh", &user_data.hv_thresh, 3, 0.001, 1, 0.001 },// 26

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC0",     &user_data.adc[0] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC1",     &user_data.adc[1] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC2",     &user_data.adc[2] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC3",     &user_data.adc[3] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC4",     &user_data.adc[4] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC5",     &user_data.adc[5] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC6",     &user_data.adc[6] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "ADC7",     &user_data.adc[7] },                    
                                                                                                                
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS0",    &user_data.aofs[0], 3, -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS1",    &user_data.aofs[1], 3, -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS2",    &user_data.aofs[2], 3, -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS3",    &user_data.aofs[3], 3, -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS4",    &user_data.aofs[4], 3, -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS5",    &user_data.aofs[5], 3, -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS6",    &user_data.aofs[6], 3, -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AOFS7",    &user_data.aofs[7], 3, -0.1, 0.1, 0.001 }, 
                                                                                                                
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN0",   &user_data.again[0], 3, 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN1",   &user_data.again[1], 3, 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN2",   &user_data.again[2], 3, 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN3",   &user_data.again[3], 3, 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN4",   &user_data.again[4], 3, 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN5",   &user_data.again[5], 3, 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN6",   &user_data.again[6], 3, 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN,       "AGAIN7",   &user_data.again[7], 3, 0.9, 1.1, 0.001 }, 

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
   xdata char str[64];

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

      user_data.station_on = 0;
      user_data.valve_locked = 0;
      user_data.vacuum_ok = 0;
      user_data.man_mode = 0;

      for (i=0 ; i<4 ; i++)
         user_data.relais[i] = 0;

      for (i=0 ; i<8 ; i++) {
         user_data.aofs[i] = 0;
         user_data.again[i] = 1;
      }

      user_data.evac_timeout = 60; // 1h to pump recipient
      user_data.fp_cycle = 20;     // run fore pump for min. 20 sec.
      user_data.vv_max = 4;        // start fore pump at 4 mbar     
      user_data.vv_min = 0.4;      // stop fore pump at 0.4 mbar
      user_data.hv_thresh = 1E-3;  // vacuum ok if < 10^-3 mbar
   }

   /* write digital outputs */
   for (i=0 ; i<20 ; i++)
      user_write(i);

   /* initialize UART1 for TC600 */
   uart_init(1, BD_9600);

   /* turn on turbo pump motor (not pump station) */
   tc600_write(23, 6, 111111);

   /* get parameter 315 (TMP finspd) */
   tc600_read(315, str);
   user_data.final_speed = atoi(str);

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
   strcpy(str, svn_revision + 21);
   *strchr(str, ' ') = 0;
   printf("  Revision:  %s", str);

   user_data.error = 0;
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   switch (index) {

   /* DOUT go through inverter */
   case 3: DOUT0 = DOUT1 = !user_data.vacuum_ok;

   /* RELAIS go through inverter */
   case 4: RELAIS0 = !user_data.relais[0]; break;
   case 5: RELAIS1 = !user_data.relais[1]; break;
   case 6: RELAIS2 = !user_data.relais[2]; break;
   case 7: RELAIS3 = !user_data.relais[3]; break;

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
   char xdata str[32];
   unsigned char xdata i, j, cs, len;

   sprintf(str, "%03Bd00%03d02=?", TC600_ADDRESS, param);

   for (i = cs = 0; i < 12; i++)
      cs += str[i];

   sprintf(str + 12, "%03Bd\r", cs);
   for (i=0 ; i<strlen(str); i++)
      putchar1(str[i]);
   flush();

   i = gets_wait(str, sizeof(str), 20);
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
   char xdata str[32];
   unsigned char xdata i, cs;

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
   i = gets_wait(str, sizeof(str), 50);

   return i;
}

/*---- Pumpstation specific code -----------------------------------*/

#define ST_OFF           1  // station if off
#define ST_START_FORE    2  // fore pump is starting up
#define ST_EVAC_FORE     3  // evacuating buffer tank
#define ST_EVAC_MAIN     4  // evacuating main recipient
#define ST_RAMP_TURBO    5  // ramp up turbo pump
#define ST_RUN_FPON      6  // running, fore pump on
#define ST_RUN_FPOFF     7  // running, fore pump off
#define ST_RUN_FPUP      8  // running, fore pump ramping up
#define ST_RUN_FPDOWN    9  // running, fore pump ramping down
#define ST_ERROR        10  // error condition

unsigned long xdata start_time;
unsigned char xdata pump_state = ST_OFF;

void set_forepump(unsigned char flag)
{
   user_data.relais[0] = flag;
   user_write(4);
}

void set_forevalve(unsigned char flag)
{
   user_data.relais[1] = flag;
   user_write(5);
}

void set_mainvalve(unsigned char flag)
{
   user_data.relais[2] = flag;
   user_write(6);
}

void set_bypassvalve(unsigned char flag)
{
   user_data.relais[3] = flag;
   user_write(7);
}

unsigned char application_display(bit init)
{
static bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0, 
   station_on_old = 0, valve_locked_old = 0;

   if (init)
      lcd_clear();

   /* display pressures */
   lcd_goto(0, 0);
   if (user_data.relais[0] && user_data.relais[1])
      printf("P1*: %9.2E mbar", user_data.vv_mbar);
   else
      printf("P1: %10.2E mbar", user_data.vv_mbar);
   
   lcd_goto(0, 1);
   if (user_data.turbo_on && (user_data.relais[2] || user_data.relais[3]))
      printf("P2*: %9.2E mbar", user_data.hv_mbar);
   else
      printf("P2: %10.2E mbar", user_data.hv_mbar);
 
   lcd_goto(0, 2);
   if (user_data.error & ERR_TURBO_COMM)
      printf("ERROR: turbo comm.  ");
   else if (user_data.error & ERR_FOREVAC)
      printf("ERROR: fore vaccuum ");
   else if (user_data.error & ERR_MAINVAC)
      printf("ERROR: main vaccuum ");
   else if (user_data.error & ERR_TURBO)
      printf("TMP ERROR: %s    ", turbo_err);
   else {
      if (user_data.man_mode) {
         printf("TMP: %4d Hz, %4.2f A", user_data.rot_speed, user_data.tmp_current);
      } else {
         if (pump_state == ST_OFF) {
            if (user_data.rot_speed > 10)
               printf("Rmp down TMP:%4d Hz", user_data.rot_speed);
            else
               printf("Pump station off    ");
         } else if (pump_state == ST_START_FORE)
            printf("Starting fore pump  ");
         else if (pump_state == ST_EVAC_FORE)
            printf("Pumping buffer tank ");
         else if (pump_state == ST_EVAC_MAIN)
            printf("Pumping recipient   ");
         else
            printf("TMP: %4d Hz, %4.2f A", user_data.rot_speed, user_data.tmp_current);
      }
   }

   if (user_data.man_mode) {

      lcd_goto(0, 3);
      printf(user_data.relais[0] ? "FP0" : "FP1");

      lcd_goto(5, 3);
      printf(user_data.relais[1] ? "VV0" : "VV1");

      lcd_goto(11, 3);
      printf(user_data.relais[2] ? "HV0" : "HV1");

      lcd_goto(16, 3);
      printf(user_data.relais[3] ? "BV0" : "BV1");

      if (b0 && !b0_old)
         user_data.relais[0] = !user_data.relais[0];
      if (b1 && !b1_old)
         user_data.relais[1] = !user_data.relais[1];
      if (b2 && !b2_old)
         user_data.relais[2] = !user_data.relais[2];
      if (b3 && !b3_old)
         user_data.relais[3] = !user_data.relais[3];
         
      user_write(4);
      user_write(5);
      user_write(6);
      user_write(7);

      /* enter menu on buttons 2 & 3 */                                   
      if (b2 && b3) {

         /* wait for buttons to be released */
         do {
           sr_read();
           } while (b2 || b3);

         return 1;
      }

   } else {

      lcd_goto(0, 3);
      printf(user_data.station_on ? "OFF" : "ON ");
   
      lcd_goto(4, 3);
      printf(user_data.valve_locked ? "UNLOCK" : " LOCK ");
   
      /* toggle main switch with button 0 */
      if (b0 && !b0_old)
         user_data.station_on = !user_data.station_on;
   
      /* toggle locking with button 1 */
      if (b1 && !b1_old)
         user_data.valve_locked = !user_data.valve_locked;
   
      /* enter menu on release of button 3 */
      if (!init)
         if (!b3 && b3_old)
            return 1;
   }

   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   /*---- Pump station turn on logic ----*/

   if (user_data.station_on && !station_on_old) {

      /* start station */
      
      /* vent enable off */
      tc600_write(12, 6, 0);

      set_mainvalve(0);
      set_bypassvalve(0);

      set_forepump(1);
      pump_state = ST_START_FORE;

      start_time = time();     // remember start time
   }

   /* wait 5s for forepump to start (turbo could be still rotating!) */
   if (pump_state == ST_START_FORE && time() > start_time + 5*100) {

      set_forevalve(1);

      start_time = time();     // remember start time
      pump_state = ST_EVAC_FORE;
   }

   /* check if buffer tank gets evacuated in less than 15 min */
   if (pump_state == ST_EVAC_FORE && time() > start_time + 15*60*100) {
      pump_state = ST_ERROR;
      user_data.error = ERR_FOREVAC;
      set_forevalve(0);
      set_forepump(0);
   }

   /* check if evacuation of forepump and buffer tank is ready */
   if (pump_state == ST_EVAC_FORE && user_data.vv_mbar < 1) {

      if (user_data.hv_mbar < 1) {
         
         /* recipient is already evacuated, go to running mode */
         start_time = time();     // remember start time
         pump_state = ST_EVAC_MAIN ;
      
      } else {

         if (user_data.valve_locked) {
            /* go immediately to running mode */
            user_data.turbo_on = 1;
            set_forevalve(1);
      
            start_time = time();     // remember start time
            pump_state = ST_RAMP_TURBO;

         } else {
         
            /* evacuate recipient through bypass valve */
            set_forevalve(0);
            delay_ms(2000);          // wait 2s
            set_bypassvalve(1);
      
            start_time = time();     // remember start time
            pump_state = ST_EVAC_MAIN;
         }
      }
   }

   /* check if recipient gets evacuated in less than evac_timeout minutes */
   if (pump_state == ST_EVAC_MAIN && 
       user_data.evac_timeout > 0 && 
       time() > start_time + (unsigned long)user_data.evac_timeout*60*100) {
      pump_state = ST_ERROR;
      user_data.error = ERR_MAINVAC;
      set_forevalve(0);
      set_forepump(0);
   }

   /* check if evacuation of main recipient is ready, may take very long time */
   if (pump_state == ST_EVAC_MAIN && user_data.hv_mbar < 1) {
      
      set_forevalve(1);        // now evacuate both recipient and buffer tank
   }

   /* check if vacuum on both sides of main valve is ok */
   if (pump_state == ST_EVAC_MAIN && user_data.hv_mbar < 1 && user_data.vv_mbar < 1) {

      /* turn on turbo pump */
      user_data.turbo_on = 1;

      start_time = time();     // remember start time
      pump_state = ST_RAMP_TURBO;
   }

   /* check if vacuum leak after ramping */
   if (pump_state == ST_RAMP_TURBO || pump_state == ST_RUN_FPON || pump_state == ST_RUN_FPOFF ||
       pump_state == ST_RUN_FPUP || pump_state == ST_RUN_FPDOWN) {

       if (user_data.hv_mbar > 4 && user_data.valve_locked == 0) {

          /* protect turbo pump */
          set_mainvalve(0);
          set_forevalve(0);
          user_data.turbo_on = 0;

          /* force restart of pump station */
          if (user_data.station_on) {
             station_on_old = 0;
             pump_state = ST_OFF;
             return 0;
          }
       }
   }
   

   /* check if turbo pump started successfully in unlocked mode */
   if (user_data.valve_locked == 0 && pump_state == ST_RAMP_TURBO && 
       user_data.rot_speed > user_data.final_speed*0.8 &&
       user_data.hv_mbar < 1 && user_data.vv_mbar < 1) {
    
      set_bypassvalve(0);

      /* now open main (gate) valve */
      set_mainvalve(1);

      start_time = time();     // remember start time
      pump_state = ST_RUN_FPON;
   }

   /* check if turbo pump started successfully in locked mode */
   if (user_data.valve_locked == 1 && pump_state == ST_RAMP_TURBO && 
       user_data.rot_speed > user_data.final_speed*0.8 &&
       user_data.vv_mbar < 1) {
    
      start_time = time();     // remember start time
      pump_state = ST_RUN_FPON;
   }


   /* check for fore pump off */
   if (pump_state == ST_RUN_FPON) {

      if (time() > start_time + user_data.fp_cycle*100 && 
          user_data.vv_mbar < user_data.vv_min) {
         set_forevalve(0);
         pump_state = ST_RUN_FPDOWN;
         start_time = time();
      }
   }

   /* turn fore pump off */
   if (pump_state == ST_RUN_FPDOWN) {

      if (time() > start_time + 3*100) {  // wait 3s
         set_forepump(0);                 // turn fore pump off
         pump_state = ST_RUN_FPOFF;
      }
   }

   /* check for fore pump on */
   if (pump_state == ST_RUN_FPOFF) {

      if (user_data.vv_mbar > user_data.vv_max) {
         set_forepump(1);
         pump_state = ST_RUN_FPUP;
         start_time = time();
      }
   }
   
   /* turn fore pump on */
   if (pump_state == ST_RUN_FPUP) {

      if (time() > start_time + 10*100) { // wait 10s
         set_forevalve(1); 
         pump_state = ST_RUN_FPON;
      }
   }
   
   /* set vacuum status */
   user_data.vacuum_ok = (user_data.hv_mbar <= user_data.hv_thresh);
   user_write(3);

   /*---- Pump station turn off logic ----*/

   if (!user_data.station_on && station_on_old) {

      /* close all valves */
      
      set_forevalve(0);
      set_mainvalve(0);
      set_bypassvalve(0);
      set_forepump(0);

      /* stop turbo pump */
      user_data.turbo_on = 0;

      /* vent enable on */
      tc600_write(12, 6, 111111);
   
      /* vent mode auto */
      tc600_write(30, 3, 0);

      pump_state = ST_OFF;
      user_data.error = 0;
   }
   
   /*---- Lock logic ----*/
   
   if (user_data.valve_locked && !valve_locked_old) {
      set_mainvalve(0);
      set_bypassvalve(0);
   }

   if (!user_data.valve_locked && valve_locked_old) {
      if (user_data.station_on) {
         start_time = time();       // remember start time
         pump_state = ST_EVAC_FORE; // start with buffer tank evacuation
         set_forepump(1);
         delay_ms(1000);
         set_forevalve(1);
      }
   }

   station_on_old = user_data.station_on;
   valve_locked_old = user_data.valve_locked;

   return 0;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
   char xdata str[32];
   static unsigned char xdata adc_chn = 0;
   static char xdata turbo_on_last = -1;
   static unsigned long xdata last_read = 0;
 
   /* read one ADC channel */
   adc_read(adc_chn, &user_data.adc[adc_chn]);

   /* convert voltage to pressure, see data sheets */
   if (adc_chn == 0)
      user_data.hv_mbar  = pow(10, 1.667 * user_data.adc[0] - 11.33); // PKR 251
   if (adc_chn == 1)
      user_data.vv_mbar  = pow(10, user_data.adc[1] - 5.5);           // TPR 280
   if (adc_chn == 5)
      user_data.hv_close = user_data.adc[5] > 5;                     // ADC as digital input
   if (adc_chn == 6)
      user_data.bv_open  = user_data.adc[6] > 5;                     // ADC as digital input
   if (adc_chn == 7)
      user_data.bv_close = user_data.adc[7] > 5;                     // ADC as digital input

   adc_chn = (adc_chn + 1) % 8;

   /* read buttons and digital input */
   sr_read();

   /* read turbo pump parameters once each second */
   if (time() > last_read + 100) {

      if (user_data.turbo_on != turbo_on_last) {
         tc600_write(10, 6, user_data.turbo_on ? 111111 : 0);
         turbo_on_last = user_data.turbo_on;
      }

      if (tc600_read(309, str)) {
         user_data.error &= ~ERR_TURBO_COMM;
         user_data.rot_speed = atoi(str);

         if (tc600_read(310, str))
            user_data.tmp_current = atoi(str) / 100.0;

         if (tc600_read(303, str)) {
            if (atoi(str) != 0) {
               user_data.error |= ERR_TURBO;
               strcpy(turbo_err, str);
            } else {
               user_data.error &= ~ERR_TURBO;
               turbo_err[0] = 0;
            }
         }
      } else
         user_data.error |= ERR_TURBO_COMM;

       last_read = time();
   }
   
   /* mangae menu on LCD display */
   lcd_menu();
}

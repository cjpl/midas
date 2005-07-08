/********************************************************************\

  Name:         scs_1001.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-1001 stand alone control unit

  $Log$
  Revision 1.6  2005/07/08 06:32:43  ritt
  Added parameters

  Revision 1.5  2005/07/08 06:23:24  ritt
  Added MV check

  Revision 1.4  2005/07/07 10:45:15  ritt
  Added better fore pump cycling

  Revision 1.3  2005/07/05 06:17:54  ritt
  Added timeout parameter

  Revision 1.2  2005/07/04 18:35:20  ritt
  Implemented pump station logic

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
char code cvs_revision[] = "$Id$";

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

/*---- Error codes ----*/

#define ERR_TURBO_COMM (1<<0)
#define ERR_FOREVAC    (1<<1)
#define ERR_MAINVAC    (1<<2)
#define ERR_TURBO      (1<<3)

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char error;
   unsigned char station_on;
   unsigned char valve_locked;
   unsigned char relais[4];
   unsigned char turbo_on;
   unsigned short rot_speed;
   float tmp_current;
   unsigned char din[4];
   unsigned char mv_close;
   unsigned char bv_open;
   unsigned char bv_close;
   float mv_mbar;
   float fv_mbar;
   unsigned short evac_timeout;
   unsigned short fp_cycle;
   unsigned char fv_max;
   float fv_min;
   float adc[8];
   float aofs[8];
   float again[8];
} xdata user_data;

MSCB_INFO_VAR code variables[] = {

   { 1, UNIT_BYTE,    0, 0, 0,                               "Error",    &user_data.error },                    
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Station",  &user_data.station_on, 0, 1, 1 },      
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Locked",   &user_data.valve_locked, 0, 1, 1 },    
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Forepump", &user_data.relais[0], 0, 1, 1 },       
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Forevlv",  &user_data.relais[1], 0, 1, 1 },       
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Mainvlv",  &user_data.relais[2], 0, 1, 1 },       
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Bypvlv",   &user_data.relais[3], 0, 1, 1 },       
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Turbo on", &user_data.turbo_on,  0, 1, 1 },       
   { 2, UNIT_HERTZ,   0, 0, 0,                               "RotSpd",   &user_data.rot_speed, },               
   { 4, UNIT_AMPERE,  0, 0, MSCBF_FLOAT,                     "TMPcur",   &user_data.tmp_current, },             
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "FP on",    &user_data.din[0] },                   
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "FV open",  &user_data.din[1] },                   
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "FV close", &user_data.din[2] },                   
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "MV open",  &user_data.din[3] },                   
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "MV close", &user_data.mv_close },                 
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "BV open",  &user_data.bv_open  },                 
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "BV close", &user_data.bv_close },                 
                                                                                                               
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "MV",       &user_data.mv_mbar },                  
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "FV",       &user_data.fv_mbar },                  
                                                                                                               
   { 2, UNIT_MINUTE,  0, 0, MSCBF_HIDDEN,                    "Timeout",  &user_data.evac_timeout, 10, 120, 10 },
   { 2, UNIT_SECOND,  0, 0, MSCBF_HIDDEN,                    "FP cycle", &user_data.fp_cycle, 10, 600, 10 },
   { 1, UNIT_BAR, PRFX_MILLI, 0, MSCBF_HIDDEN,               "FV max",   &user_data.fv_max, 1, 10, 1 },         
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "FV min",   &user_data.fv_min, 0.1, 1, 0.1 },

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

      user_data.evac_timeout = 60; // 1h to pump recipient
      user_data.fp_cycle = 20;     // run fore pump for min. 20 sec.
      user_data.fv_max = 4;        // start fore pump at 4 mbar     
      user_data.fv_min = 1E-5;     // keep fore pump running until main vacuum below 1E-5 mbar
   }

   /* write digital outputs */
   for (i=0 ; i<4 ; i++)
      user_write(i);

   /* initialize UART1 for TC600 */
   uart_init(1, BD_9600);

   /* turn on turbo pump motor (not pump station) */
   tc600_write(23, 6, 111111);

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
   printf("   Version:  %bd", 1);

   user_data.error = 0;
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

   i = gets_wait(str, sizeof(str), 50);
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
   user_write(0);
}

void set_forevalve(unsigned char flag)
{
   user_data.relais[1] = flag;
   user_write(1);
}

void set_mainvalve(unsigned char flag)
{
   user_data.relais[2] = flag;
   user_write(2);
}

void set_bypassvalve(unsigned char flag)
{
   user_data.relais[3] = flag;
   user_write(3);
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
      printf("P1*: %9.2E mbar", user_data.fv_mbar);
   else
      printf("P1: %10.2E mbar", user_data.fv_mbar);
   
   lcd_goto(0, 1);
   if (user_data.turbo_on && (user_data.relais[2] || user_data.relais[3]))
      printf("P2*: %9.2E mbar", user_data.mv_mbar);
   else
      printf("P2: %10.2E mbar", user_data.mv_mbar);
 
   lcd_goto(0, 2);
   if (user_data.error & ERR_TURBO_COMM)
      printf("ERROR: turbo comm.  ");
   else if (user_data.error & ERR_FOREVAC)
      printf("ERROR: fore vaccuum ");
   else if (user_data.error & ERR_MAINVAC)
      printf("ERROR: main vaccuum ");
   else if (user_data.error & ERR_TURBO)
      printf("ERROR: TMP < 80%%    ");
   else {
      if (pump_state == ST_OFF) {
         if (user_data.rot_speed > 10)
            printf("Rmp down TMP: %3d Hz", user_data.rot_speed);
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
   if (pump_state == ST_EVAC_FORE && user_data.fv_mbar < 1) {

      if (user_data.mv_mbar < 1) {
         
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

   /* check if recipient gets evacuated in less than 1h */
   if (pump_state == ST_EVAC_MAIN && time() > start_time + user_data.evac_timeout*60*100) {
      pump_state = ST_ERROR;
      user_data.error = ERR_MAINVAC;
      set_forevalve(0);
      set_forepump(0);
   }

   /* check if evacuation of main recipient is ready, may take very long time */
   if (pump_state == ST_EVAC_MAIN && user_data.mv_mbar < 1) {
      
      set_forevalve(1);        // now evacuate both recipient and buffer tank
   }

   /* check if vacuum on both sides of main valve is ok */
   if (pump_state == ST_EVAC_MAIN && user_data.mv_mbar < 1 && user_data.fv_mbar < 1) {

      /* turn on turbo pump */
      user_data.turbo_on = 1;

      start_time = time();     // remember start time
      pump_state = ST_RAMP_TURBO;
   }

   /* check for turbo pump error */
   if (pump_state == ST_RAMP_TURBO && user_data.rot_speed < 800 && time() > start_time + 15*6*100) {
      pump_state = ST_ERROR;
      user_data.error = ERR_TURBO;
   }

   /* check if turbo pump started successfully */
   if (pump_state == ST_RAMP_TURBO && user_data.rot_speed > 800 &&
       user_data.mv_mbar < 1 && user_data.fv_mbar < 1) {
    
      set_bypassvalve(0);

      /* now open main (gate) valve if not locked */
      if (!user_data.valve_locked)
         set_mainvalve(1);

      start_time = time();     // remember start time
      pump_state = ST_RUN_FPON;
   }


   /* check for fore pump off */
   if (pump_state == ST_RUN_FPON) {

      if (time() > start_time + user_data.fp_cycle*100 && 
          user_data.fv_mbar < user_data.fv_min) {
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

      if (user_data.fv_mbar > user_data.fv_max) {
         set_forepump(1);
         pump_state = ST_RUN_FPUP;
         start_time = time();
      }
   }
   
   /* turn fore pump on */
   if (pump_state == ST_RUN_FPUP) {

      if (time() > start_time + 3*100) { // wait 3s
         set_forevalve(1); 
         pump_state = ST_RUN_FPON;
      }
   }
   

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
   char idata str[32];
   static unsigned char xdata adc_chn = 0;
   static char xdata turbo_on_last = -1;
   static unsigned long xdata last_read = 0;
 
   /* read one ADC channel */
   adc_read(adc_chn, &user_data.adc[adc_chn]);

   /* convert voltage to pressure, see data sheets */
   if (adc_chn == 0)
      user_data.mv_mbar  = pow(10, 1.667 * user_data.adc[0] - 11.33); // PKR 251
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

      if (user_data.turbo_on != turbo_on_last) {
         tc600_write(10, 6, user_data.turbo_on ? 111111 : 0);
         turbo_on_last = user_data.turbo_on;
      }

      if (tc600_read(309, str)) {
         user_data.error &= ~ERR_TURBO_COMM;
         user_data.rot_speed = atoi(str);

         if (tc600_read(310, str))
            user_data.tmp_current = atoi(str) / 100.0;
      } else
         user_data.error |= ERR_TURBO_COMM;

      last_read = time();
   }
   
   /* mangae menu on LCD display */
   lcd_menu();
}

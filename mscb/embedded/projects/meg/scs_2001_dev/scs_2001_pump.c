/********************************************************************\

  Name:         scs_2001_pump.c
  Created by:   Stefan Ritt


  Contents:     Control program for PSI pump station

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"
#include "scs_2001.h"
#include "lcd.h"

char code node_name[] = "PUMP-STATION";
char code svn_revision[] = "$Id$";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern lcd_menu(void);
extern char putchar1(char c);

unsigned char button(unsigned char i);

#define TC600_ADDRESS 1   // Address of turbo pump via RS486 bus

unsigned char tc600_read(unsigned short param, char *result);
unsigned char tc600_write(unsigned short param, unsigned char len, unsigned long value);

void set_forepump(unsigned char flag);
void set_forevalve(unsigned char flag);
void set_mainvalve(unsigned char flag);
void set_bypassvalve(unsigned char flag);

/*---- pump station states ----*/

#define ST_OFF           1  // station if off
#define ST_START_FORE    2  // fore pump is starting up
#define ST_EVAC_FORE     3  // evacuating buffer tank
#define ST_PREEVAC_MAIN  4  // wait for forevalve to be closed
#define ST_EVAC_MAIN     5  // evacuating main recipient
#define ST_POSTEVAC_MAIN 6  // checking buffer vacuum again
#define ST_RAMP_TURBO    7  // ramp up turbo pump
#define ST_RUN_FPON      8  // running, fore pump on
#define ST_RUN_FPOFF     9  // running, fore pump off
#define ST_RUN_FPUP     10  // running, fore pump ramping up
#define ST_RUN_FPDOWN   11  // running, fore pump ramping down
#define ST_ERROR        12  // error condition

/*---- Error codes ----*/

#define ERR_TURBO_COMM (1<<0)
#define ERR_FOREVAC    (1<<1)
#define ERR_MAINVAC    (1<<2)
#define ERR_TURBO      (1<<3)

/*---- Buttons ----*/

bit b0, b1, b2, b3;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   unsigned char error;
   unsigned char station_on;
   unsigned char valve_locked;
   unsigned char vacuum_ok;
   unsigned char man_mode;
   unsigned char fore_pump;
   unsigned char fore_valve;
   unsigned char main_valve;
   unsigned char bypass_valve;
   unsigned char turbo_on;
   unsigned short rot_speed;
   float tmp_current;
   unsigned char fp_on;
   unsigned char fv_open;
   unsigned char fv_close;
   unsigned char hv_open;
   unsigned char hv_close;
   unsigned char bv_open;
   unsigned char bv_close;
   float hv_mbar;
   float vv_mbar;
   unsigned char  pbr_type;
   unsigned short final_speed;
   unsigned short evac_timeout;
   unsigned short fp_cycle;
   float vv_max;
   float vv_min;
   float hv_thresh;
   unsigned char pump_state;
   char turbo_err[10];
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 1, UNIT_BYTE,    0, 0, 0,                               "Error",    &user_data.error },                        // 0
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Station",  &user_data.station_on, 0, 0, 1, 1 },       // 1
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Locked",   &user_data.valve_locked, 0, 0, 1, 1 },     // 2
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Vac OK",   &user_data.vacuum_ok,  0, 0, 1, 1 },       // 3
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "ManMode",  &user_data.man_mode,  0, 0, 1, 1 },        // 4
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Forepump", &user_data.fore_pump, 0, 0, 1, 1 },        // 5
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Forevlv",  &user_data.fore_valve, 0, 0, 1, 1 },       // 6
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Mainvlv",  &user_data.main_valve, 0, 0, 1, 1 },       // 7
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Bypvlv",   &user_data.bypass_valve, 0, 0, 1, 1 },     // 8
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "Turbo on", &user_data.turbo_on,  0, 0, 1, 1 },        // 9
   { 2, UNIT_HERTZ,   0, 0, 0,                               "RotSpd",   &user_data.rot_speed, },                   // 10
   { 4, UNIT_AMPERE,  0, 0, MSCBF_FLOAT,                     "TMPcur",   &user_data.tmp_current, },                 // 11
                                                                                                               
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "VP on",    &user_data.fp_on },                        // 12
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "VV open",  &user_data.fv_open },                      // 13
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "VV close", &user_data.fv_close },                     // 14
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "HV open",  &user_data.hv_open  },                     // 15
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "HV close", &user_data.hv_close },                     // 16
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "BV open",  &user_data.bv_open  },                     // 17
   { 1, UNIT_BOOLEAN, 0, 0, 0,                               "BV close", &user_data.bv_close },                     // 18
                                                                                                               
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "HV",       &user_data.hv_mbar },                      // 19
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,                "VV",       &user_data.vv_mbar },                      // 20
                                                                                                               
   { 2, UNIT_HERTZ,   0, 0, MSCBF_HIDDEN,                    "FinSpd",   &user_data.final_speed },                  // 21
   { 1, UNIT_BYTE,    0, 0, MSCBF_HIDDEN,                    "PBR",      &user_data.pbr_type },                     // 22
   { 2, UNIT_MINUTE,  0, 0, MSCBF_HIDDEN,                    "Timeout",  &user_data.evac_timeout, 0, 0, 300, 10 },  // 23
   { 2, UNIT_SECOND,  0, 0, MSCBF_HIDDEN,                    "FP cycle", &user_data.fp_cycle, 0, 10, 600, 10 },     // 24
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "VV max",   &user_data.vv_max, 1, 1, 10, 1 },          // 25
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "VV min",   &user_data.vv_min, 1, 0.1, 1, 0.1 },       // 26
   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "HV thrsh", &user_data.hv_thresh, 3, 0.001, 1, 0.001 },// 27

   { 1, UNIT_BYTE,    0, 0, MSCBF_HIDDEN,                    "State",    &user_data.pump_state },                   // 28
   {10, UNIT_STRING,  0, 0, MSCBF_HIDDEN,                    "TMPErr",   &user_data.turbo_err[0] },                 // 29

   { 0 }
};

MSCB_INFO_VAR *variables = vars;
unsigned char xdata update_data[64];

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- Check for correct modules -----------------------------------*/

void setup_variables(void)
{
   /* open drain(*) /push-pull: 
      P0.0 TX1      P1.0 LCD_D1       P2.0 WATCHDOG     P3.0 OPT_CLK
      P0.1*RX1      P1.1 LCD_D2       P2.1 LCD_E        P3.1 OPT_ALE
      P0.2 TX2      P1.2 RTC_IO       P2.2 LCD_RW       P3.2 OPT_STR
      P0.3*RX2      P1.3 RTC_CLK      P2.3 LCD_RS       P3.3 OPT_DATAO
                                                                      
      P0.4 EN1      P1.4              P2.4 LCD_DB4      P3.4*OPT_DATAI
      P0.5 EN2      P1.5              P2.5 LCD_DB5      P3.5*OPT_STAT
      P0.6 LED1     P1.6              P2.6 LCD_DB6      P3.6*OPT_SPARE1
      P0.7 LED2     P1.7 BUZZER       P2.7 LCD_DB7      P3.7*OPT_SPARE2
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xF5;
   P1MDOUT = 0xFF;
   P2MDOUT = 0xFF;
   P3MDOUT = 0x0F;

   /* enable ADC & DAC */
   SFRPAGE = ADC0_PAGE;
   AMX0CF = 0x00;               // select single ended analog inputs
   ADC0CF = 0x98;               // ADC Clk 2.5 MHz @ 98 MHz, gain 1
   ADC0CN = 0x80;               // enable ADC 
   REF0CN = 0x00;               // use external voltage reference

   SFRPAGE = LEGACY_PAGE;
   REF0CN = 0x03;               // select internal voltage reference

   SFRPAGE = DAC0_PAGE;
   DAC0CN = 0x80;               // enable DAC0
   SFRPAGE = DAC1_PAGE;
   DAC1CN = 0x80;               // enable DAC1

   lcd_goto(0, 0);

   /* check if correct modules are inserted */
   if (!verify_module(0, 0, 0x61)) {
      printf("Please insert module");
      printf(" 'Uin +-10V' (0x61) ");
      printf("     into port 0    ");
      while (!button(0)) watchdog_refresh(0);
   }
   if (!verify_module(0, 4, 0x40)) {
      printf("Please insert module");
      printf(" 'Dout 24V' (0x40)  ");
      printf("     into port 4    ");
      while (!button(0)) watchdog_refresh(0);
   }
   if (!verify_module(0, 5, 0x20)) {
      printf("Please insert module");
      printf("  'Din 5V' (0x40)   ");
      printf("     into port 5    ");
      while (!button(0)) watchdog_refresh(0);
   }

   sysclock_reset();

   /* initialize drivers */
   dr_ad7718(0x61, MC_INIT, 0, 0, 0, NULL);
   dr_dout_bits(0x40, MC_INIT, 0, 4, 0, NULL);
   dr_din_bits(0x20, MC_INIT, 0, 5, 0, NULL);
}

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char xdata i;
   char xdata str[6];

   /* green (lower) LED on by default */
   led_mode(0, 1);
   /* red (upper) LED off by default */
   led_mode(1, 0);

   /* initialize power monitor */
   for (i=0 ; i<3 ; i++)
      monitor_init(i);

   /* initizlize real time clock */
   rtc_init();

   /* initial EEPROM value */
   if (init) {
      user_data.station_on = 0;
      user_data.valve_locked = 0;
      user_data.vacuum_ok = 0;
      user_data.man_mode = 0;
      user_data.fore_pump = 0;
      user_data.fore_valve = 0;
	   user_data.main_valve = 0;
	   user_data.bypass_valve = 0;

      user_data.pbr_type = 0;      // PKR sensor type by default
      user_data.evac_timeout = 60; // 1h to pump recipient
      user_data.fp_cycle = 20;     // run fore pump for min. 20 sec.
      user_data.vv_max = 4;        // start fore pump at 4 mbar     
      user_data.vv_min = 0.4;      // stop fore pump at 0.4 mbar
      user_data.hv_thresh = 1E-3;  // vacuum ok if < 10^-3 mbar
   }

   /* initially the pump state is off */
   user_data.pump_state = ST_OFF;

   set_forevalve(0);
   set_mainvalve(0);
   set_bypassvalve(0);
   set_forepump(0);

   /* write digital outputs */
   for (i=0 ; i<20 ; i++)
      user_write(i);

   /* initialize UART1 for TC600 */
   uart_init(1, BD_9600);

   /* get parameter 315 (TMP finspd) */
   tc600_read(315, str);
   user_data.final_speed = atoi(str);

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
   printf("  Address:   %04X", sys_info.node_addr);
   lcd_goto(0, 2);
   strcpy(str, svn_revision + 21);
   *strchr(str, ' ') = 0;
   printf("  Revision:  %s", str);
   lcd_goto(0, 3);
   printf("                    ");

   user_data.error = 0;
}

#pragma NOAREGS

/*---- Front panel button read -------------------------------------*/

bit adc_init = 0;

unsigned char button(unsigned char i)
{
unsigned short xdata value;

   SFRPAGE = ADC0_PAGE;
   if (!adc_init) {
      adc_init = 1;
      SFRPAGE = LEGACY_PAGE;
      REF0CN  = 0x03;           // use internal voltage reference
      AMX0CF  = 0x00;           // select single ended analog inputs
      ADC0CF  = 0x98;           // ADC Clk 2.5 MHz @ 98 MHz, gain 1
      ADC0CN  = 0x80;           // enable ADC 
   }

   AMX0SL  = (7-i) & 0x07;      // set multiplexer
   DELAY_US(2);                 // wait for settling time

   DISABLE_INTERRUPTS;
  
   AD0INT = 0;
   AD0BUSY = 1;
   while (!AD0INT);   // wait until conversion ready

   ENABLE_INTERRUPTS;

   value = (ADC0L | (ADC0H << 8));

   return value < 1000;
}

/*---- Power management --------------------------------------------*/

bit trip_5V = 0, trip_24V = 0, wrong_firmware = 0;
unsigned char xdata trip_5V_box;
#define N_BOX 1 // increment this if you are using slave boxes
#define CPLD_FIRMWARE_REQUIRED 2

unsigned char power_management(void)
{
static unsigned long xdata last_pwr;
unsigned char xdata status, return_status, i, a[3];
unsigned short xdata d;

   return_status = 0;

   /* only 10 Hz */
   if (time() > last_pwr+10 || time() < last_pwr) {
      last_pwr = time();
    
      for (i=0 ; i<N_BOX ; i++) {

         read_csr(i, &status);

         if ((status >> 4) != CPLD_FIRMWARE_REQUIRED) {
            led_blink(1, 1, 100);
            if (!wrong_firmware) {
               lcd_clear();
               lcd_goto(0, 0);
               puts("Wrong CPLD firmware");
               lcd_goto(0, 1);
               if (i > 0) 
                 printf("Slave addr: %bd", i);
               lcd_goto(0, 2);
               printf("Req: %02bd != Act: %02bd", CPLD_FIRMWARE_REQUIRED, status >> 4);
            }
            wrong_firmware = 1;
            return_status = 1;
         }

         monitor_read(i, 0x01, 0, a, 3); // Read alarm register
         if (a[0] & 0x08) {
            led_blink(1, 1, 100);
            if (!trip_24V) {
               lcd_clear();
               lcd_goto(0, 0);
               printf("Overcurrent >1.5A on");
               lcd_goto(0, 1);
               printf("   24V output !!!   ");
               lcd_goto(0, 2);
               if (i > 0) 
                  printf("   Slave addr: %bd", i);
               lcd_goto(15, 3);
               printf("RESET");
            }
            trip_24V = 1;
         
            if (button(3)) {
               monitor_clear(i);
               trip_24V = 0;
               while (button(3));  // wait for button to be released
               lcd_clear();
            }

            return_status = 1;
         }

         if (time() > 100) { // wait for first conversion
            monitor_read(i, 0x02, 3, (char *)&d, 2); // Read +5V ext
            if (2.5*(d >> 4)*2.5/4096.0 < 4.5) {
               if (!trip_5V) {
                  lcd_clear();
                  led_blink(1, 1, 100);
                  lcd_goto(0, 0);
                  printf("Overcurrent >0.5A on");
                  lcd_goto(0, 1);
                  printf("    5V output !!!");
                  lcd_goto(0, 2);
                  if (i > 0) 
                     printf("    Slave addr: %bd", i);
               }
               lcd_goto(0, 3);
               printf("    U = %1.2f V", 2.5*(d >> 4)*2.5/4096.0);
               trip_5V = 1;
               trip_5V_box = 1;
               return_status = 1;
            } else if (trip_5V && trip_5V_box == i) {
               trip_5V = 0;
               lcd_clear();
            }
         }

      }
   } else if (trip_24V || trip_5V || wrong_firmware)
      return_status = 1; // do not go into application_display
  
   return return_status;
}

/*------------------------------------------------------------------*/

unsigned char tc600_read(unsigned short param, char *result)
{
   char xdata str[32];
   unsigned char xdata i, j, cs, len;

   uart1_init_buffer();

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

   uart1_init_buffer();

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

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   update_data[index] = 1;
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

/*---- Pumpstation specific code -----------------------------------*/

unsigned long xdata start_time;

void set_forepump(unsigned char flag)
{
   user_data.fore_pump = flag;
   user_write(5);
}

void set_forevalve(unsigned char flag)
{
   user_data.fore_valve = flag;
   user_write(6);
}

void set_mainvalve(unsigned char flag)
{
   user_data.main_valve = flag;
   user_write(7);
}

void set_bypassvalve(unsigned char flag)
{
   user_data.bypass_valve = flag;
   user_write(8);
}

unsigned char application_display(bit init)
{
static bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0, 
   station_on_old = 0, valve_locked_old = 0;

   if (init)
      lcd_clear();

   /* display pressures */
   lcd_goto(0, 0);
   if (user_data.fore_pump && user_data.fore_valve)
      printf("P1*: %9.2E mbar", user_data.vv_mbar);
   else
      printf("P1: %10.2E mbar", user_data.vv_mbar);
   
   lcd_goto(0, 1);
   if ((user_data.turbo_on && (user_data.main_valve || user_data.bypass_valve)) ||
       (user_data.fore_pump && user_data.bypass_valve))
      printf("P2*: %9.2E mbar", user_data.hv_mbar);
   else
      printf("P2: %10.2E mbar", user_data.hv_mbar);

   /* blink LED in error status */
   if (user_data.error) {
      led_blink(1, 1, 100);
   }

   lcd_goto(0, 2);
   if (user_data.error & ERR_TURBO_COMM)
      printf("ERROR: turbo comm.  ");
   else if (user_data.error & ERR_FOREVAC)
      printf("ERROR: fore vaccuum ");
   else if (user_data.error & ERR_MAINVAC)
      printf("ERROR: main vaccuum ");
   else if (user_data.error & ERR_TURBO)
      printf("TMP ERROR: %s    ", user_data.turbo_err);
   else {
      if (user_data.man_mode) {
         printf("TMP: %4d Hz, %4.2f A", user_data.rot_speed, user_data.tmp_current);
      } else {
         if (user_data.pump_state == ST_OFF) {
            if (user_data.rot_speed > 10)
               printf("Rmp down TMP:%4d Hz", user_data.rot_speed);
            else
               printf("Pump station off    ");
         } else if (user_data.pump_state == ST_START_FORE)
            printf("Starting fore pump  ");
         else if (user_data.pump_state == ST_EVAC_FORE)
            printf("Pumping buffer tank ");
         else if (user_data.pump_state == ST_EVAC_MAIN)
            printf("Pumping recipient   ");
         else
            printf("TMP: %4d Hz, %4.2f A", user_data.rot_speed, user_data.tmp_current);
      }
   }

   if (user_data.man_mode) {

      lcd_goto(0, 3);
      printf(user_data.fore_pump ? "VP0" : "VP1");

      lcd_goto(5, 3);
      printf(user_data.fore_valve ? "VV0 " : "VV1 ");

      lcd_goto(11, 3);
      printf(user_data.main_valve ? "HV0" : "HV1");

      lcd_goto(16, 3);
      printf(user_data.bypass_valve ? "BV0" : "BV1");

      if (b0 && !b0_old)
         user_data.fore_pump = !user_data.fore_pump;
      if (b1 && !b1_old)
         user_data.fore_valve = !user_data.fore_valve;
      if (b2 && !b2_old)
         user_data.main_valve = !user_data.main_valve;
      if (b3 && !b3_old)
         user_data.bypass_valve = !user_data.bypass_valve;
         
      user_write(5);
      user_write(6);
      user_write(7);
      user_write(8);

      /* enter menu on buttons 2 & 3 */                                   
      if (b2 && b3) {

         /* wait for buttons to be released */
         do {
           b2 = button(2);
           b3 = button(3);
           } while (b2 || b3);

         return 1;
      }

   } else {

      lcd_goto(0, 3);
      printf(user_data.station_on ? "OFF" : "ON ");
   
      lcd_goto(4, 3);
      printf(user_data.valve_locked ? "UNLOCK" : " LOCK ");
   
      printf("        ");
      printf("%2bd", user_data.pump_state);

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
      user_data.pump_state = ST_START_FORE;

      start_time = time();     // remember start time
   }

   /* wait 15s for forepump to start (turbo could be still rotating!) */
   if (user_data.pump_state == ST_START_FORE && time() > start_time + 15*100) {

      set_forevalve(1);

      start_time = time();     // remember start time
      user_data.pump_state = ST_EVAC_FORE;
   }

   /* check if buffer tank gets evacuated in less than 15 min */
   if (user_data.pump_state == ST_EVAC_FORE && time() > start_time + 15*60*100) {
      user_data.pump_state = ST_ERROR;
      user_data.error = ERR_FOREVAC;
      set_forevalve(0);
      set_forepump(0);
      set_bypassvalve(0);
   }

   /* check if evacuation of forepump and buffer tank is ready */
   if (user_data.pump_state == ST_EVAC_FORE && user_data.vv_mbar < user_data.vv_min) {

      if (user_data.hv_mbar < 1) {
         
         /* recipient is already evacuated, go to running mode */
         start_time = time();     // remember start time
         user_data.pump_state = ST_EVAC_MAIN ;
      
      } else {

         if (user_data.valve_locked) {
            /* go immediately to running mode */
            user_data.turbo_on = 1;
            set_forevalve(1);
      
            start_time = time();     // remember start time
            user_data.pump_state = ST_RAMP_TURBO;

         } else {
            /* evacuate recipient through bypass valve */
            set_forevalve(0);
            user_data.pump_state = ST_PREEVAC_MAIN;
            start_time = time();
         }
      }
   }

   /* after 5 sec. evacuate through bypass valve */
   if (user_data.pump_state == ST_PREEVAC_MAIN && time() > start_time + 5*100) {
      set_bypassvalve(1);
      start_time = time();
      user_data.pump_state = ST_EVAC_MAIN;
   }

   /* check if recipient gets evacuated in less than evac_timeout minutes */
   if (user_data.pump_state == ST_EVAC_MAIN && 
       user_data.evac_timeout > 0 && 
       time() > start_time + (unsigned long)user_data.evac_timeout*60*100) {
      user_data.pump_state = ST_ERROR;
      user_data.error = ERR_MAINVAC;
      set_bypassvalve(0);
      set_forevalve(0);
      set_forepump(0);
   }

   /* check if evacuation of main recipient is ready, may take very long time */
   if (user_data.pump_state == ST_EVAC_MAIN && user_data.hv_mbar < 1) {
      set_bypassvalve(0);
      start_time = time();
      user_data.pump_state = ST_POSTEVAC_MAIN;
   }

   /* wait for 2s to open forvalve */
   if (user_data.pump_state == ST_POSTEVAC_MAIN && time() > start_time + 2*100) {
      set_forevalve(1);
   }

   /* check if vacuum on both sides of main valve is ok */
   if (user_data.pump_state == ST_POSTEVAC_MAIN && time() > start_time + 4*100 &&
       user_data.vv_mbar < user_data.vv_min) {

      /* turn on turbo pump */
      user_data.turbo_on = 1;

      start_time = time();     // remember start time
      user_data.pump_state = ST_RAMP_TURBO;
   }

   /* check if vacuum leak after ramping */
   if (user_data.pump_state == ST_RAMP_TURBO || user_data.pump_state == ST_RUN_FPON || user_data.pump_state == ST_RUN_FPOFF ||
       user_data.pump_state == ST_RUN_FPUP || user_data.pump_state == ST_RUN_FPDOWN) {

       if (user_data.hv_mbar > 4 && user_data.valve_locked == 0) {

          /* protect turbo pump */
          set_mainvalve(0);
          set_forevalve(0);
          user_data.turbo_on = 0;

          /* force restart of pump station */
          if (user_data.station_on) {
             station_on_old = 0;
             user_data.pump_state = ST_OFF;
             return 0;
          }
       }
   }

   /* if main vaccum got back, go back to evacuation */
   if (user_data.valve_locked == 0 && user_data.pump_state == ST_RAMP_TURBO && 
       user_data.hv_mbar > 1) {
    
      set_forevalve(0);
      start_time = time();     // remember start time
      user_data.pump_state = ST_PREEVAC_MAIN;
   }

   /* check if turbo pump started successfully in unlocked mode */
   if (user_data.valve_locked == 0 && user_data.pump_state == ST_RAMP_TURBO && 
       user_data.rot_speed > user_data.final_speed*0.8 &&
       user_data.hv_mbar < 1 && user_data.vv_mbar < user_data.vv_min) {
    
      /* now open main (gate) valve */
      set_mainvalve(1);

      start_time = time();     // remember start time
      user_data.pump_state = ST_RUN_FPON;
   }

   /* check if turbo pump started successfully in locked mode */
   if (user_data.valve_locked == 1 && user_data.pump_state == ST_RAMP_TURBO && 
       user_data.rot_speed > user_data.final_speed*0.8 &&
       user_data.vv_mbar < user_data.vv_min) {
    
      start_time = time();     // remember start time
      user_data.pump_state = ST_RUN_FPON;
   }


   /* check for fore pump off */
   if (user_data.pump_state == ST_RUN_FPON) {

      /* close main valve if turbe speed dropped */
      if (user_data.rot_speed < user_data.final_speed * 0.8) {
         set_mainvalve(0);
         start_time = time();     // remember start time
         user_data.pump_state = ST_RAMP_TURBO;
      }

      if (time() > start_time + user_data.fp_cycle*100 && 
          user_data.vv_mbar < user_data.vv_min) {
         set_forevalve(0);
         user_data.pump_state = ST_RUN_FPDOWN;
         start_time = time();
      }
   }

   /* turn fore pump off */
   if (user_data.pump_state == ST_RUN_FPDOWN) {

      if (time() > start_time + 3*100) {  // wait 3s
         set_forepump(0);                 // turn fore pump off
         user_data.pump_state = ST_RUN_FPOFF;
      }
   }

   /* check for fore pump on */
   if (user_data.pump_state == ST_RUN_FPOFF) {

      if (user_data.vv_mbar > user_data.vv_max) {
         set_forepump(1);
         user_data.pump_state = ST_RUN_FPUP;
         start_time = time();
      }
   }
   
   /* turn fore pump on */
   if (user_data.pump_state == ST_RUN_FPUP) {

      if (time() > start_time + 10*100) { // wait 10s
         set_forevalve(1); 
         user_data.pump_state = ST_RUN_FPON;
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

      user_data.pump_state = ST_OFF;
      user_data.error = 0;
   }
   
   /*---- Lock logic ----*/
   
   if (user_data.valve_locked && !valve_locked_old) {
      set_mainvalve(0);
      set_bypassvalve(0);
   }

   if (!user_data.valve_locked && valve_locked_old) {
      if (user_data.station_on) {
         start_time = time();        // remember start time
         if (user_data.rot_speed > user_data.final_speed*0.8 &&
             user_data.hv_mbar < 1) {
            user_data.pump_state = ST_RUN_FPON;
            set_mainvalve(1);
         } else {
            user_data.pump_state = ST_START_FORE; // start with starting forepump
            set_forepump(1);
         }
      }
   }

   station_on_old = user_data.station_on;
   valve_locked_old = user_data.valve_locked;

   /*---- Turbo error ----*/

   if (user_data.error == ERR_TURBO_COMM) {
      user_data.pump_state = ST_ERROR;
      set_forevalve(0);
      set_mainvalve(0);
      set_bypassvalve(0);
      set_forepump(0);
   }

   return 0;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
char xdata str[32];
static char xdata turbo_on_last = -1;
static unsigned long xdata last_read = 0;
unsigned char xdata n;
float xdata value;

   /* check if anything to write */
   if (update_data[5]) {
      update_data[5] = 0;
      dr_dout_bits(0x40, MC_WRITE, 0, 4, 2, &user_data.fore_pump);
   }
   if (update_data[6]) {
      update_data[6] = 0;
      dr_dout_bits(0x40, MC_WRITE, 0, 4, 3, &user_data.fore_valve);
   }
   if (update_data[7]) {
      update_data[7] = 0;
      dr_dout_bits(0x40, MC_WRITE, 0, 4, 4, &user_data.main_valve);
   }
   if (update_data[8]) {
      update_data[8] = 0;
      dr_dout_bits(0x40, MC_WRITE, 0, 4, 5, &user_data.bypass_valve);
   }

   /* output Vacuum OK signal directly */
   dr_dout_bits(0x40, MC_WRITE, 0, 4, 6, &user_data.vacuum_ok);
   dr_dout_bits(0x40, MC_WRITE, 0, 4, 7, &user_data.vacuum_ok);

   /* read ADC channels */
   dr_ad7718(0x61, MC_READ, 0, 0, 0, &value);
   dr_ad7718(0x61, MC_READ, 0, 0, 1, &value);
   dr_ad7718(0x61, MC_READ, 0, 0, 2, &value);
   n = dr_ad7718(0x61, MC_READ, 0, 0, 3, &value);
   if (n > 0) {
      if (user_data.pbr_type == 0)
         value  = pow(10, 1.667 * value - 11.33);   // PKR 251
      else
         value = pow(10, (value-7.75)/0.75);        // PBR 260
      DISABLE_INTERRUPTS;
      user_data.hv_mbar = value;
      ENABLE_INTERRUPTS;
   }
   dr_ad7718(0x61, MC_READ, 0, 0, 4, &value);
   n = dr_ad7718(0x61, MC_READ, 0, 0, 5, &value);
   if (n > 0) {
      value  = pow(10, value - 5.5);  // TPR 280
      DISABLE_INTERRUPTS;
      user_data.vv_mbar = value;
      ENABLE_INTERRUPTS;
   }
   dr_ad7718(0x61, MC_READ, 0, 0, 6, &value);
   dr_ad7718(0x61, MC_READ, 0, 0, 7, &value);

   /* read DIN */
   dr_din_bits(0x20, MC_READ, 0, 5, 2, &user_data.fp_on);
   dr_din_bits(0x20, MC_READ, 0, 5, 3, &user_data.fv_open);
   dr_din_bits(0x20, MC_READ, 0, 5, 4, &user_data.fv_close);
   dr_din_bits(0x20, MC_READ, 0, 5, 5, &user_data.hv_open);
   dr_din_bits(0x20, MC_READ, 0, 5, 6, &user_data.hv_close);
   dr_din_bits(0x20, MC_READ, 0, 5, 0, &user_data.bv_open);
   dr_din_bits(0x20, MC_READ, 0, 5, 1, &user_data.bv_close);

   /* read buttons and digital input */
   b0 = button(0);
   b1 = button(1);
   b2 = button(2);
   b3 = button(3);

   /* read turbo pump parameters once each two seconds */
   if (time() > last_read + 200) {

      if (user_data.turbo_on != turbo_on_last) {
         tc600_write(10, 6, user_data.turbo_on ? 111111 : 0);
         turbo_on_last = user_data.turbo_on;
      }

      if (tc600_read(309, str)) {
         user_data.error &= ~ERR_TURBO_COMM;
         user_data.rot_speed = atoi(str);

         if (tc600_read(310, str))
            user_data.tmp_current = atoi(str) / 100.0;
         else 
            user_data.tmp_current = nan();

         if (tc600_read(303, str)) {
            if (atoi(str) != 0) {
               user_data.error |= ERR_TURBO;
               strcpy(user_data.turbo_err, str);
            } else {
               user_data.error &= ~ERR_TURBO;
               user_data.turbo_err[0] = 0;
            }
         }
      } else {
         user_data.rot_speed = 9999;
         user_data.error |= ERR_TURBO_COMM;
      }

      if (user_data.final_speed < 100) {
         tc600_read(315, str);
         user_data.final_speed = atoi(str);
      }

      last_read = time();
   }
   
   /* manage menu on LCD display */
   lcd_menu();
}

/********************************************************************\

  Name:         scs_2000.c
  Created by:   Stefan Ritt


  Contents:     General-purpose framework for SCS-2000 control unit.

  $Id: scs_2000.c 2874 2005-11-15 08:47:14Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"
#include "scs_2001_led.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "LED PULSER";
char code svn_revision[] = "$Id: scs_2000.c 2874 2005-11-15 08:47:14Z ritt $";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern lcd_menu(void);

/*---- Port definitions ----*/

sbit B0         = P1 ^ 3;
sbit B1         = P1 ^ 2;
sbit B2         = P1 ^ 1;
sbit B3         = P1 ^ 0;

bit b0, b1, b2, b3;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

typedef struct {
   unsigned char freq;
   unsigned char pwidth;
   float ampl[40];
} USER_DATA;

USER_DATA xdata user_data;
unsigned char xdata update_data[42];

MSCB_INFO_VAR code vars[] = {

   { 1, UNIT_BYTE,   0, 0, 0,           "Freq",   &user_data.freq,     0,  18, 1 },
   { 1, UNIT_BYTE,   0, 0, 0,           "PWidth", &user_data.pwidth,   1,  20, 1 },

   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl00", &user_data.ampl[0],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl01", &user_data.ampl[1],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl02", &user_data.ampl[2],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl03", &user_data.ampl[3],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl04", &user_data.ampl[4],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl05", &user_data.ampl[5],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl06", &user_data.ampl[6],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl07", &user_data.ampl[7],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl08", &user_data.ampl[8],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl09", &user_data.ampl[9],  0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl10", &user_data.ampl[10], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl11", &user_data.ampl[11], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl12", &user_data.ampl[12], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl13", &user_data.ampl[13], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl14", &user_data.ampl[14], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl15", &user_data.ampl[15], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl16", &user_data.ampl[16], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl17", &user_data.ampl[17], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl18", &user_data.ampl[18], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl19", &user_data.ampl[19], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl20", &user_data.ampl[20], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl21", &user_data.ampl[21], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl22", &user_data.ampl[22], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl23", &user_data.ampl[23], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl24", &user_data.ampl[24], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl25", &user_data.ampl[25], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl26", &user_data.ampl[26], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl27", &user_data.ampl[27], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl28", &user_data.ampl[28], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl29", &user_data.ampl[29], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl30", &user_data.ampl[30], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl31", &user_data.ampl[31], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl32", &user_data.ampl[32], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl33", &user_data.ampl[33], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl34", &user_data.ampl[34], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl35", &user_data.ampl[35], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl36", &user_data.ampl[36], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl37", &user_data.ampl[37], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl38", &user_data.ampl[38], 0, 100, 1 },
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT, "Ampl39", &user_data.ampl[39], 0, 100, 1 },


   { 0 }
};

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

extern unsigned char idata n_variables;

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;
   char xdata str[6];

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
   AMX0CF  = 0x00;               // select single ended analog inputs
   ADC0CF  = 0x98;               // ADC Clk 2.5 MHz @ 98 MHz, gain 1
   ADC0CN  = 0x80;               // enable ADC 
   REF0CN  = 0x00;               // use external voltage reference

   SFRPAGE = LEGACY_PAGE;
   REF0CN  = 0x03;               // select internal voltage reference

   SFRPAGE = DAC0_PAGE;
   DAC0CN  = 0x80;               // enable DAC0
   SFRPAGE = DAC1_PAGE;
   DAC1CN  = 0x80;               // enable DAC1

   /* initizlize real time clock */
   rtc_init();

   /* green (lower) LED on by default */
   led_mode(0, 1);
   /* red (upper) LED off by default */
   led_mode(1, 0);

   /* initialize power monitor */
   monitor_init(0);

   lcd_goto(0, 0);
   printf("ampl %f", user_data.ampl[0]);

   /* initial values */
   if (init) {
      memset(&user_data, 0, sizeof(user_data));
      user_data.freq = 7;
      user_data.pwidth = 6;
   }

   /* write digital outputs */
   for (i=0 ; i<n_variables ; i++)
      user_write(i);

   /* check if correct modules are inserted */
   for (i=0 ; i<5 ; i++) {
      if (!verify_module(0, i, 0x02)) {
         lcd_goto(0, 0);
         printf("Please insert module");
         printf("'LED Pulser' (0x02) ");
         printf("     into port %bd    ", i);
         printf("                    ");
//         while (1) watchdog_refresh(0);
      }
   }   if (!verify_module(0, 5, 0x20)) {
      lcd_goto(0, 0);
      printf("Please insert module");
      printf("  'DIN 5V' (0x20)   ");
      printf("    into port 5     ");
      printf("                    ");
      while (1) watchdog_refresh(0);
   }

   /* initialize drivers */
   for (i=0 ; i<5 ; i++)
      dr_pulser(MC_INIT, 0, i, 0, NULL);

   /* display startup screen */
   lcd_goto(0, 0);
   for (i=0 ; i<7-strlen(sys_info.node_name)/2 ; i++)
      puts(" ");
   puts("** ");
   puts(sys_info.node_name);
   puts(" **");
   lcd_goto(0, 1);
   printf("   Address:  %04X    ", sys_info.node_addr);
   lcd_goto(0, 2);
   strncpy(str, svn_revision + 16, 6);
   *strchr(str, ' ') = 0;
   printf("  Revision:  %s", str);
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

#define CPLD_FIRMWARE_REQUIRED 2

unsigned char power_management(void)
{
static unsigned long xdata last_pwr;
unsigned char xdata status, return_status, a[3];
unsigned short xdata d;

   return_status = 0;

   /* only 10 Hz */
   if (time() > last_pwr+10 || time() < last_pwr) {
      last_pwr = time();
    
      read_csr(0, CSR_PWR_STATUS, &status);

      if ((status >> 4) != CPLD_FIRMWARE_REQUIRED) {
         led_blink(1, 1, 100);
         if (!wrong_firmware) {
            lcd_clear();
            lcd_goto(0, 0);
            puts("Wrong CPLD firmware");
            lcd_goto(0, 2);
            printf("Req: %02bd != Act: %02bd", CPLD_FIRMWARE_REQUIRED, status >> 4);
         }
         wrong_firmware = 1;
         return_status = 1;
      }

      /*
      monitor_read(0, 0x01, 0, a, 3); // Read alarm register
      if (a[0] & 0x08) {
         led_blink(1, 1, 100);
         if (!trip_24V) {
            lcd_clear();
            lcd_goto(0, 0);
            printf("Overcurrent >1.5A on");
            lcd_goto(0, 1);
            printf("   24V output !!!   ");
            lcd_goto(15, 3);
            printf("RESET");
         }
         trip_24V = 1;
      
         if (button(3)) {
            monitor_clear(0);
            trip_24V = 0;
            while (button(3));  // wait for button to be released
            lcd_clear();
         }

         return_status = 1;
      }

      if (time() > 100) { // wait for first conversion
         monitor_read(0, 0x02, 3, (char *)&d, 2); // Read +5V ext
         if (2.5*(d >> 4)*2.5/4096.0 < 4.5) {
            if (!trip_5V) {
               lcd_clear();
               led_blink(1, 1, 100);
               lcd_goto(0, 0);
               printf("Overcurrent >0.5A on");
               lcd_goto(0, 1);
               printf("    5V output !!!");
            }
            lcd_goto(0, 3);
            printf("    U = %1.2f V", 2.5*(d >> 4)*2.5/4096.0);
            trip_5V = 1;
            trip_5V_box = 1;
            return_status = 1;
         } else if (trip_5V) {
            trip_5V = 0;
            lcd_clear();
         }
      }
      */
   } else if (trip_24V || trip_5V || wrong_firmware)
      return_status = 1; // do not go into application_display
  
   return return_status;
}

/*---- Special data for LED pulser ---------------------------------*/

unsigned char xdata pulser_ampl = 0;

typedef struct {
  unsigned long  count;
  unsigned short freq;
  unsigned char  exp;
} FREQ_TABLE;

/* frequency table for 100MHz quartz clodk */

FREQ_TABLE code freq_table[] = {
   { 0,          0, 0 },   //    EXT
   { 100000000,  1, 0 },   //    1 Hz
   { 50000000,   2, 0 },   //    2 Hz
   { 20000000,   5, 0 },   //    5 Hz
   { 10000000,  10, 0 },   //   10 Hz
   { 5000000,   20, 0 },   //   20 Hz
   { 2000000,   50, 0 },   //   50 Hz
   { 1000000,  100, 0 },   //  100 Hz
   { 500000,   200, 0 },   //  200 Hz
   { 200000,   500, 0 },   //  500 Hz
   { 100000,     1, 1 },   //    1 kHz
   { 50000,      2, 1 },   //    2 kHz
   { 20000,      5, 1 },   //    5 kHz
   { 10000,     10, 1 },   //   10 kHz
   { 5000,      20, 1 },   //   20 kHz
   { 2000,      50, 1 },   //   50 kHz
   { 1000,     100, 1 },   //  100 kHz
   { 500,      200, 1 },   //  200 kHz
   { 200,      500, 1 },   //  500 kHz
   { 100,        1, 2 },   //    1 MHz
   0,
};

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   /* will be updated in main loop */
   if (index < sizeof(update_data) / sizeof(update_data[0]) &&
       index >= 0) {
      update_data[index] = 1;
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
   data_out[0] = data_in[0];
   return 1;
}

/*---- Application display -----------------------------------------*/

bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0;
unsigned long xdata last_b2 = 0, last_b3 = 0;

unsigned char application_display(bit init)
{
static unsigned char xdata pulser_ampl_old=255, pulser_freq_old=255;
unsigned char i;

   if (init)
      lcd_clear();

   if (power_management())
      return 0;

   lcd_goto(0, 0);
   if (user_data.freq == 0)
      printf("External Trigger");
   else {
      printf("Frequency: %d ", freq_table[user_data.freq].freq);
      if (freq_table[user_data.freq].exp == 0)
         printf("Hz   ");
      else if (freq_table[user_data.freq].exp == 1)
         printf("kHz   ");
      else
         printf("MHz   ");
      }

   lcd_goto(0, 1);
   printf("Amplitude: %bd %%   ", pulser_ampl);

   lcd_goto(0, 3);
   printf(" - Hz +     - Amp +");

   if (b0 && !b0_old && user_data.freq > 0)
      user_data.freq--;

   if (b1 && !b1_old && user_data.freq < 19)
      user_data.freq++;

   if (b2 && !b2_old && pulser_ampl > 0) {
      pulser_ampl--;
      last_b2 = time();
   }

   if (b2 && time() > last_b2 + 70 && pulser_ampl > 0)
      pulser_ampl--;

   if (b3 && !b3_old && pulser_ampl < 100) {
      pulser_ampl++;
      last_b3 = time();
   }

   if (b3 && time() > last_b3 + 70 && pulser_ampl < 100)
      pulser_ampl++;

   if (user_data.freq != pulser_freq_old)
      user_write(0);

   if (pulser_ampl != pulser_ampl_old) {
      for (i=2 ; i<n_variables ; i++) {
         *((float *)variables[i].ud) = pulser_ampl / 100.0;
         user_write(i);
      }
   }

   pulser_ampl_old = pulser_ampl;
   pulser_freq_old = user_data.freq;
   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

/*---- User loop function ------------------------------------------*/

static unsigned char idata port_index = 0, first_var_index = 0;

void read_port_reg(unsigned char addr, unsigned char port_no, unsigned char *pd); //##

void user_loop(void)
{
unsigned char xdata i;
unsigned char d;

   /* internal variables */
   if (update_data[0]) {
      update_data[0] = 0;
      dr_pulser(MC_FREQ, 0, 0, 0, &freq_table[user_data.freq].count);
   }

   if (update_data[1]) {
      update_data[1] = 0;
      dr_pulser(MC_PWIDTH, 0, 0, 0, &user_data.pwidth);
   }

   for (i=2 ; i<n_variables ; i++) {
      if (update_data[i]) {
         update_data[i] = 0;
         dr_pulser(MC_WRITE, 0, (i-2)/8, (i-2)%8, &user_data.ampl[i-2]);
      }
   }

   read_port_reg(0, 0, &d);

   /* read buttons */
   b0 = button(0);
   b1 = button(1);
   b2 = button(2);
   b3 = button(3);

   /* manage menu on LCD display */
   lcd_menu();
}

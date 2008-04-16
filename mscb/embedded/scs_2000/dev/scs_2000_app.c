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
#include "../scs_2000.h"

char code node_name[] = "SCS-2000-APP"; // not more than 15 characters !
char code svn_revision[] = "$Id: scs_2000_app.c 2874 2005-11-15 08:47:14Z ritt $";

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern lcd_menu(void);

/*---- Port definitions ----*/

bit b0, b1, b2, b3;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

struct {
   float adc[24];
   unsigned char valve[8];
} xdata user_data;

MSCB_INFO_VAR code vars[] = {

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC00",    &user_data.adc[0] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC01",    &user_data.adc[1] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC02",    &user_data.adc[2] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC03",    &user_data.adc[3] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC04",    &user_data.adc[4] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC05",    &user_data.adc[5] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC06",    &user_data.adc[6] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC07",    &user_data.adc[7] },                    
                                                                                                
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC08",    &user_data.adc[8] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC09",    &user_data.adc[9] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC10",    &user_data.adc[10] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC11",    &user_data.adc[11] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC12",    &user_data.adc[12] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC13",    &user_data.adc[13] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC14",    &user_data.adc[14] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC15",    &user_data.adc[15] },                    

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC16",    &user_data.adc[16] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC17",    &user_data.adc[17] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC18",    &user_data.adc[18] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC19",    &user_data.adc[19] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC20",    &user_data.adc[20] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC21",    &user_data.adc[21] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC22",    &user_data.adc[22] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT,       "ADC23",    &user_data.adc[23] },                    

   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve0",   &user_data.valve[0], 0, 0, 1, 1 },  // 24
   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve1",   &user_data.valve[1], 0, 0, 1, 1 },  // 25
   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve2",   &user_data.valve[2], 0, 0, 1, 1 },  // 26
   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve3",   &user_data.valve[3], 0, 0, 1, 1 },  // 27
   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve4",   &user_data.valve[4], 0, 0, 1, 1 },  // 28
   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve5",   &user_data.valve[5], 0, 0, 1, 1 },  // 29
   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve6",   &user_data.valve[6], 0, 0, 1, 1 },  // 30
   { 1, UNIT_BOOLEAN, 0, 0, 0,                "Valve7",   &user_data.valve[7], 0, 0, 1, 1 },  // 31

   { 0 }
};

MSCB_INFO_VAR *variables = vars;
float xdata backup_data[64];
unsigned char xdata update_data[64];

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- Check for correct modules -----------------------------------*/

void setup_variables(void)
{
   lcd_goto(0, 0);
   /* check if correct modules are inserted */
   if (!verify_module(0, 0, 0x61)) {
      printf("Please insert module");
      printf(" 'Uin +-10V' (0x61) ");
      printf("     into port 0    ");
      while (1) watchdog_refresh(0);
   }
   if (!verify_module(0, 1, 0x61)) {
      printf("Please insert module");
      printf(" 'Uin +-10V' (0x61) ");
      printf("     into port 1    ");
      while (1) watchdog_refresh(0);
   }
   if (!verify_module(0, 2, 0x61)) {
      printf("Please insert module");
      printf(" 'Uin +-10V' (0x61) ");
      printf("     into port 2    ");
      while (1) watchdog_refresh(0);
   }
   if (!verify_module(0, 4, 0x40)) {
      printf("Please insert module");
      printf("   'Dout' (0x40)    ");
      printf("    into port 4     ");
      while (1) watchdog_refresh(0);
   }

   /* initialize drivers */
   dr_ad7718(0x61, MC_INIT, 0, 0, 0, NULL);
   dr_ad7718(0x61, MC_INIT, 0, 1, 0, NULL);
   dr_ad7718(0x61, MC_INIT, 0, 2, 0, NULL);
   dr_dout_bits(0x40, MC_INIT, 0, 4, 0, NULL);
}

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char i;
   char xdata str[6];

   /* open drain(*) /push-pull: 
      P0.0 TX1      P1.0*SW1          P2.0 WATCHDOG     P3.0 OPT_CLK
      P0.1*RX1      P1.1*SW2          P2.1 LCD_E        P3.1 OPT_ALE
      P0.2 TX2      P1.2*SW3          P2.2 LCD_RW       P3.2 OPT_STR
      P0.3*RX2      P1.3*SW4          P2.3 LCD_RS       P3.3 OPT_DATAO
                                                                      
      P0.4 EN1      P1.4              P2.4 LCD_DB4      P3.4*OPT_DATAI
      P0.5 EN2      P1.5              P2.5 LCD_DB5      P3.5*OPT_STAT
      P0.6 LED1     P1.6              P2.6 LCD_DB6      P3.6*OPT_SPARE1
      P0.7 LED2     P1.7 BUZZER       P2.7 LCD_DB7      P3.7*OPT_SPARE2
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xF5;
   P1MDOUT = 0xF0;
   P2MDOUT = 0xFF;
   P3MDOUT = 0x00;

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

   /* red (upper) LED off by default */
   led_mode(1, 0);

   /* initial EEPROM value */
   if (init) {
   }

   /* retrieve backup data from RAM if not reset by power on */
   SFRPAGE = LEGACY_PAGE;
   if ((RSTSRC & 0x02) == 0)
      memcpy(&user_data, &backup_data, sizeof(user_data));

   /* write digital outputs */
   for (i=0 ; i<8 ; i++)
      user_write(8+i);

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
   for (i=10 ; i<strlen(svn_revision) ; i++)
      if (svn_revision[i] == ' ')
         break;
   strncpy(str, svn_revision + i + 1, 6);
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

/*---- Power management --------------------------------------------*/

bit trip_5V = 0, trip_24V = 0, trip_24V_old = 0, wrong_firmware = 0;
extern unsigned char xdata n_box;
unsigned char xdata trip_24V_addr[16];

#define CPLD_FIRMWARE_REQUIRED 1

unsigned char power_management(void)
{
static unsigned long xdata last_pwr;
unsigned char status, return_status, i, j;

   return_status = 0;
   if (!trip_24V)
      for (i=0 ; i<16 ; i++)
         trip_24V_addr[i] = 0;

   /* only 10 Hz */
   if (time() > last_pwr+10 || time() < last_pwr) {
      last_pwr = time();
    
      for (i=0 ; i<1 ; i++) {

         status = power_status(i);
         
         if ((status >> 4) != CPLD_FIRMWARE_REQUIRED) {
            if (!wrong_firmware)
               lcd_clear();
            led_blink(1, 1, 100);
            lcd_goto(0, 0);
            puts("Wrong CPLD firmware");
            lcd_goto(0, 1);
            if (i > 0) 
              printf("Slave addr: %bd", i);
            lcd_goto(0, 2);
            printf("Req: %02bd != Act: %02bd", CPLD_FIRMWARE_REQUIRED, status >> 4);
            wrong_firmware = 1;
            return_status = 1;
         }

         if ((status & 0x01) == 0) {
            if (!trip_5V)
               lcd_clear();
            led_blink(1, 1, 100);
            lcd_goto(0, 0);
            printf("Overcurrent >0.5A on");
            lcd_goto(0, 1);
            printf("    5V output !!!   ");
            lcd_goto(0, 2);
            if (i > 0) 
              printf("    Slave addr: %bd", i);
            trip_5V = 1;
            return_status = 1;
         } else if (trip_5V) {
            trip_5V = 0;
            lcd_clear();
         }
         
         if ((status & 0x02) == 0 || trip_24V_addr[i]) {
            if (!trip_24V_addr[i]) {
               /* check again to avoid spurious trips */
               status = power_status(i);
               if ((status & 0x02) > 0)
                  continue;

               /* turn off 24 V */
               power_24V(i, 0);
               trip_24V = 1;
               trip_24V_addr[i] = 1;
               lcd_clear();
            }
            led_blink(1, 1, 100);
            lcd_goto(0, 0);
            printf("   Overcurrent on   ");
            lcd_goto(0, 1);
            printf("   24V output !!!   ");
            lcd_goto(0, 2);
            if (i > 0) 
              printf("   Slave addr: %bd", i);
            lcd_goto(0, 3);
            printf("RESET               ");
         
            if (button(0)) {
               for (j=0 ; j<16 ; j++)
                  if (trip_24V_addr[j]) {
                     power_24V(j, 1);   // turn on power
                     trip_24V_addr[j] = 0;
                  }
               trip_24V = 0;
               while (button(0)); // wait for button to be released
               lcd_clear();
            }

            return_status = 1;
         }
      }
   } else if (trip_24V || trip_5V || wrong_firmware)
      return_status = 1; // do not go into application_display
  
   return return_status;
}

/*---- Application display -----------------------------------------*/

bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0;

unsigned char application_display(bit init)
{
   /* clear LCD display on startup */
   if (init)
      lcd_clear();

   /* check regularly for power short */
   if (power_management())
      return 0;

   /* display ADCs */
   lcd_goto(0, 0);
   printf("ADC0: %1.3f V", user_data.adc[0]);
   lcd_goto(0, 1);
   printf("ADC1: %1.3f V", user_data.adc[1]);
   lcd_goto(0, 3);
   printf("VARS");

   /* enter menu on release of button 0 */
   if (!init)
      if (!b0 && b0_old)
         return 1;

   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

/*---- User control ------------------------------------------------*/

void user_control()
{
unsigned char i;

   /* simply level control */

   if (user_data.adc[0] > 2)
      user_data.valve[0] = 1;
   else
      user_data.valve[0] = 0;

   /* propagate user_data to hardware */
   for (i=24 ; i<32 ; i++)
      user_write(i);

}

/*---- User loop function ------------------------------------------*/

void set_float(float *d, float s)
{
  /* copy float value to user_data without intterupt */
  DISABLE_INTERRUPTS;
  *d = s;
  ENABLE_INTERRUPTS;
}

void user_loop(void)
{
unsigned char xdata i, n;
float xdata value;

   /* check if anything to write */
   for (i=24 ; i<32 ; i++) {
      if (update_data[i]) {
         update_data[i] = 0;
         dr_dout_bits(0x40, MC_WRITE, 0, 4, i-24, &user_data.valve[0]);
      }
   }

   /* read ADCs */
   for (i=0 ; i<8 ; i++) {
      n = dr_ad7718(0x61, MC_READ, 0, 0, i, &value);
      if (n > 0)
         set_float(&user_data.adc[i], value);
      n = dr_ad7718(0x61, MC_READ, 0, 1, i, &value);
      if (n > 0)
         set_float(&user_data.adc[8+i], value);
      n = dr_ad7718(0x61, MC_READ, 0, 2, i, &value);
      if (n > 0)
         set_float(&user_data.adc[16+i], value);
   }

   /* do control */
   user_control();

   /* backup data */
   memcpy(&backup_data, &user_data, sizeof(user_data));

   /* read buttons */
   b0 = button(0);
   b1 = button(1);
   b2 = button(2);
   b3 = button(3);

  /* manage menu on LCD display */
   lcd_menu();
}

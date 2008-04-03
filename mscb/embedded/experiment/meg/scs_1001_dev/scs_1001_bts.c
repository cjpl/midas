/********************************************************************\

  Name:         scs_1001_bts.c
  Created by:   Stefan Ritt


  Contents:     BTS control program for SCS-1001 connected to one
                SCS-910

  $Id: scs_1001_bts.c 3083 2006-04-26 14:19:48Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;
extern bit flash_param; 
extern unsigned char idata _flkey;
extern SYS_INFO idata sys_info;

char code node_name[] = "BTS";
char code svn_revision[] = "$Id: scs_1001_bts.c 3083 2006-04-26 14:19:48Z ritt $";

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

typedef struct {
   unsigned char error;
   unsigned char bts_state;          // 0: manual, 1:automatic
   unsigned char ln2_valve_state;
   unsigned char heater_state;

   unsigned short ln2_on;
   unsigned short ln2_off;
   unsigned short preheat;

   unsigned char ka_out;
   unsigned char ka_in;
   float         ka_level;

   unsigned char ln2_valve;
   unsigned char ln2_heater;
   float         ln2_valve_temp;
   float         ln2_heater_temp;

   float         jt_forerun_valve;
   float         jt_bypass_valve;
   float         jt_temp;

   float         lhe_level1; // main
   float         lhe_level2; // reserve

   float         ln2_temp_left;     // screen left
   float         ln2_temp_right;    // screen right
   float         ln2_temp_tower;    // screen tower
   float         ln2_temp_top;      // screen top

   float         lhe_temp_d_center; // middle D9K
   float         lhe_temp_d_left;   // left D9K
   float         lhe_temp_d_right;  // right D9K
   float         lhe_temp_r_center; // middle TVO
   float         lhe_temp_r_left;   // left TVO
   float         lhe_temp_r_right;  // right TVO

   float         ln2_mbar;
   float         lhe_bar;

   float         quench1;
   float         quench2;

   float         lhe_demand;

   float         adc[8];
   float         aofs[8];
   float         again[8];

   float         scs_910[20];

} USER_DATA;

USER_DATA xdata user_data;
USER_DATA xdata backup_data;

MSCB_INFO_VAR code vars[] = {

   { 1, UNIT_BYTE,    0, 0, 0,                         "Error",    &user_data.error },                     // 0
   { 1, UNIT_BYTE,    0, 0, 0,                         "State",    &user_data.bts_state, 0, 9, 1 },        // 1
                                                                                                         
   { 1, UNIT_BYTE,    0, 0, 0,                         "LN2VS",    &user_data.ln2_valve_state, 0, 2, 1 },  // 2
   { 1, UNIT_BYTE,    0, 0, 0,                         "HTRS",     &user_data.heater_state, 0, 2, 1 },     // 3

   { 2, UNIT_SECOND,  0, 0, 0,                         "LN2 on",   &user_data.ln2_on, 0, 120, 10 },        // 4
   { 2, UNIT_SECOND,  0, 0, 0,                         "LN2 off",  &user_data.ln2_off, 0, 3600, 10 },      // 5
   { 2, UNIT_SECOND,  0, 0, 0,                         "Preheat",  &user_data.preheat, 0, 60, 1 },         // 6

   { 1, UNIT_BYTE,    0, 0, 0,                         "KA Out",   &user_data.ka_out, 0, 7, 1 },           // 7
   { 1, UNIT_BYTE,    0, 0, 0,                         "KA In",    &user_data.ka_in },                     // 8
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,               "KA Level", &user_data.ka_level },                  // 9  

   { 1, UNIT_BOOLEAN, 0, 0, 0,                         "LN2 vlve", &user_data.ln2_valve, 0, 1, 1 },        // 10
   { 1, UNIT_BOOLEAN, 0, 0, 0,                         "LN2 htr",  &user_data.ln2_heater, 0, 1, 1 },       // 11
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LN2vlv T", &user_data.ln2_valve_temp },            // 12
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LN2htr T", &user_data.ln2_heater_temp },           // 13
                                                                                                         
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,               "JT Fvlve", &user_data.jt_forerun_valve, 0, 100, 1},// 14
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,               "JT Bvlve", &user_data.jt_bypass_valve, 0, 100, 1}, // 15
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "JT T",     &user_data.jt_temp },                   // 16

   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,               "LHe lvl1", &user_data.lhe_level1 },                // 17
   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,               "LHe lvl2", &user_data.lhe_level2 },                // 18

   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LN2 Tl",   &user_data.ln2_temp_left },             // 19
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LN2 Tr",   &user_data.ln2_temp_right },            // 20
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LN2 Tw",   &user_data.ln2_temp_tower },            // 21
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LN2 Tt",   &user_data.ln2_temp_top },              // 22

   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LHE TDc",  &user_data.lhe_temp_d_center },         // 23
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LHE TDl",  &user_data.lhe_temp_d_left },           // 24
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LHE TDr",  &user_data.lhe_temp_d_right },          // 25
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LHE TRc",  &user_data.lhe_temp_r_center },         // 26
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LHE TRl",  &user_data.lhe_temp_r_left },           // 27
   { 4, UNIT_KELVIN,  0, 0, MSCBF_FLOAT,               "LHE TRr",  &user_data.lhe_temp_r_right },          // 28

   { 4, UNIT_BAR, PRFX_MILLI, 0, MSCBF_FLOAT,          "LN2 P",    &user_data.ln2_mbar },                  // 29
   { 4, UNIT_BAR, 0,          0, MSCBF_FLOAT,          "LHE P",    &user_data.lhe_bar },                   // 30

   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT,               "Quench1",  &user_data.quench1 },                   // 31
   { 4, UNIT_VOLT,    0, 0, MSCBF_FLOAT,               "Quench2",  &user_data.quench2 },                   // 32

   { 4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT,               "LHE Dmd",  &user_data.lhe_demand },                // 33

   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC0",     &user_data.adc[0] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC1",     &user_data.adc[1] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC2",     &user_data.adc[2] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC3",     &user_data.adc[3] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC4",     &user_data.adc[4] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC5",     &user_data.adc[5] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC6",     &user_data.adc[6] },                    
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "ADC7",     &user_data.adc[7] },                    
                                                                                                          
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS0",    &user_data.aofs[0], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS1",    &user_data.aofs[1], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS2",    &user_data.aofs[2], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS3",    &user_data.aofs[3], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS4",    &user_data.aofs[4], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS5",    &user_data.aofs[5], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS6",    &user_data.aofs[6], -0.1, 0.1, 0.001 }, 
   { 4, UNIT_VOLT,   0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AOFS7",    &user_data.aofs[7], -0.1, 0.1, 0.001 }, 
                                                                                                          
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN0",   &user_data.again[0], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN1",   &user_data.again[1], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN2",   &user_data.again[2], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN3",   &user_data.again[3], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN4",   &user_data.again[4], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN5",   &user_data.again[5], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN6",   &user_data.again[6], 0.9, 1.1, 0.001 }, 
   { 4, UNIT_FACTOR, 0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "AGAIN7",   &user_data.again[7], 0.9, 1.1, 0.001 }, 

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

MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;

/*---- User init function ------------------------------------------*/

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

	  user_data.ln2_on     = 60;
	  user_data.ln2_off    = 250;
     user_data.preheat    = 10;
     user_data.lhe_demand = 60;
   }

   /* retrieve backup data from RAM if not reset by power on */
   SFRPAGE = LEGACY_PAGE;
   if ((RSTSRC & 0x02) == 0)
      memcpy(&user_data, &backup_data, sizeof(user_data));
   user_data.error = 0;

   /* write outputs */
   for (i=0 ; i<99 ; i++)
      user_write(i);

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
   // keep states from EEPROM if ok
   if (user_data.bts_state > 3) 
      user_data.bts_state = 0;
   if (user_data.ln2_valve_state > 2) 
      user_data.ln2_valve_state = 0;
   if (user_data.heater_state > 2) 
      user_data.heater_state = 0;
}

#pragma NOAREGS

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
float curr;
unsigned short d;

   switch (index) {

   case 2:  
      /* propagate LN2 valve on/off to relais value */
      if (user_data.ln2_valve_state == 0)
         user_data.ln2_valve = 0;
      else if (user_data.ln2_valve_state == 1)
         user_data.ln2_valve = 1;
      user_write(10);
	  break;

   /* DOUT goes through inverter, bits are switched at KA */
   case 7: 
      DOUT3 = (user_data.ka_out & (1<<0)) == 0;
      DOUT2 = (user_data.ka_out & (1<<1)) == 0;
      DOUT1 = (user_data.ka_out & (1<<2)) == 0;
      if (user_data.ka_out != backup_data.ka_out) {
         flash_param = 1;
         _flkey = 0xF1;
      }
      break;
   
   /* LN2 valve has inverse logic */
   case 10: RELAIS0 = !user_data.ln2_valve; break; 

   case 11: DOUT0   = !user_data.ln2_heater; break;
   
   /* "Vorlauf" JT-valve */
   case 14:
      /* convert % to 4-20mA current */
	   curr = 4+16*user_data.jt_forerun_valve/100.0;

      /* correct for measured offset */
      curr -= 0.22;

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
   case 15:
      /* convert % to 4-20mA current */
	   curr = 4+16*user_data.jt_bypass_valve/100.0;

      /* correct for measured offset */
      curr -= 0.32;

      /* 0...24mA range */
      d = (curr / 24.0) * 0x1000;
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

/*------------------------------------------------------------------*/

void set_float(float *d, float s)
{
  /* copy float value to user_data without intterupt */
  DISABLE_INTERRUPTS;
  *d = s;
  ENABLE_INTERRUPTS;
}

/*---- ADC read function -------------------------------------------*/

void dround(float *v, unsigned char digits)
{
unsigned char xdata i;
float xdata b;

   for (i=0,b=1 ; i<digits ; i++)
      b *= 10;
   *v = floor((*v * b) + 0.5) / b;
}

void adc_read(channel, float *d)
{
   unsigned long xdata value;
   unsigned int xdata i, n;
   float xdata gvalue;

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

   /* KA inputs are inverted (relais vs. pull-up) */
   user_data.ka_in = ~d & 0x07;

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
   printf("PH:%5.2fb", user_data.lhe_bar);
   lcd_goto(10, 0);
   printf("PI:%5.1g", user_data.ln2_mbar);
   
   lcd_goto(0, 1);
   printf("He:%5.1f%%", user_data.lhe_level1);
   lcd_goto(10, 1);
   printf("TH:%5.1f%K", user_data.lhe_temp_r_center);

   lcd_goto(0, 2);
   printf("FV:%5.1f%%", user_data.jt_forerun_valve);
   lcd_goto(10, 2);
   printf("BV:%5.1f%%", user_data.jt_bypass_valve);

   lcd_goto(0, 3);
   if (user_data.ln2_valve_state == 0)
      printf("LN2M0");
   else if (user_data.ln2_valve_state == 1)
      printf("LN2M1 ");
   else {
      if (user_data.ln2_valve)
         printf("LN2A1");
      else
         printf("LN2A0");
   }

   lcd_goto(6, 3);
   if (user_data.heater_state == 0)
      printf("HTRM0");
   else if (user_data.heater_state == 1)
      printf("HTRM1 ");
   else {
      if (user_data.ln2_heater)
         printf("HTRA1");
      else
         printf("HTRA0");
   }

   lcd_goto(13, 3);
   if (user_data.bts_state == 1)
      printf("+ HEA-");
   else
      printf("+ HE -");

   /* toggle ln2 valve state with button 0 */
   if (b0 && !b0_old) {
      last_ln2time = time();
      user_data.ln2_valve_state = (user_data.ln2_valve_state + 1) % 3;

      if (user_data.ln2_valve_state == 0)
         user_data.ln2_valve = 0;
      else if (user_data.ln2_valve_state == 1)
         user_data.ln2_valve = 1;
      user_write(10);
   }

   /* toggle heater state with button 0 */
   if (b1 && !b1_old) {
      user_data.heater_state = (user_data.heater_state + 1) % 3;

      if (user_data.heater_state == 0)
         user_data.ln2_heater = 0;
      else if (user_data.heater_state == 1)
         user_data.ln2_heater = 1;
      user_write(11);
   }

   /* increase HE flow with button 2 */
   if (b2) {
      if (!b2_old) {
         user_data.jt_forerun_valve += 1;
         last_b = time();
      }
      if (time() > last_b + 70)
         user_data.jt_forerun_valve += 1;
      if (time() > last_b + 300)
         user_data.jt_forerun_valve += 9;
      if (user_data.jt_forerun_valve > 100)
         user_data.jt_forerun_valve = 100;
      user_write(14);
   }

   /* decrease HE flow with button 3 */
   if (b3) {
      if (!b3_old) {
         user_data.jt_forerun_valve -= 1;
         last_b = time();
      }
      if (time() > last_b + 70)
         user_data.jt_forerun_valve -= 1;
      if (time() > last_b + 300)
         user_data.jt_forerun_valve -= 9;
      if (user_data.jt_forerun_valve < 0)
         user_data.jt_forerun_valve = 0;
      user_write(14);
   }

   /* enter menu on release of button 2 & 3 */
   if (b2 && b3) {
      while (b2 || b3)
         sr_read();
      return 1;
   }

   /* switch heater on "preheat" seconds before ln2 valve */
   if (user_data.ln2_valve_state == 2 && user_data.heater_state == 2 &&
       user_data.ln2_valve == 0 && 
       time() - last_ln2time > user_data.ln2_off * 100 - user_data.preheat * 100) {
	  user_data.ln2_heater = 1;
	  user_write(11);
   }

   /* switch ln2 valve on by specified time */
   if (user_data.ln2_valve_state == 2 && 
       user_data.ln2_valve == 0 && 
       time() - last_ln2time > user_data.ln2_off * 100) {
      last_ln2time = time();
	  user_data.ln2_valve = 1;
	  user_write(10);
   }

   /* swith ln2 valve off by specified time */
   if (user_data.ln2_valve_state == 2 && 
       user_data.ln2_valve == 1 && 
       time() - last_ln2time > user_data.ln2_on * 100) {
      last_ln2time = time();
	  user_data.ln2_valve = 0;
     user_data.ln2_heater = 0;
	  user_write(10);
	  user_write(11);
   }

   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

/*------------------------------------------------------------------*/

unsigned long xdata last_control = 0;

void lhe_control()
{
   float xdata v;

   /* do nothing in manual and labview mode */
   if (user_data.bts_state != 1)
      return;

   /* execute once every 60 sec */
   if (time() > last_control + 60*100) {
      last_control = time();
      
      v = user_data.jt_forerun_valve;
      if (user_data.lhe_level1 > user_data.lhe_demand ||
          user_data.ka_level < 55 ||
          user_data.lhe_bar > 1.4)
         v = v - 1;
      else
         v = v + 0.5; 

      if (v > 17)
         v = 17;
      if (v < 8)
         v = 8;

      set_float(&user_data.jt_forerun_valve, v);
      user_write(14);
   }
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

   if (adc_chn == 2) {
      // convert CLTS voltage to Ohm (1mA current)
      x = user_data.adc[2] / 0.001;

      // convert Ohm to Temperature, 24deg: 290 Ohm, -270deg: 220 Ohm
      x = (x-220.0)*(24.0+270.0)/(290.0-220.0) - 270.0;

      // convert to Kelvin
      x += 273;
      if (x < 0)
         x = 0;
      set_float(&user_data.jt_temp, x); 
   }

   if (adc_chn == 4) {
      // convert voltage to current im mA (100 Ohm)
      x = user_data.adc[4] * 10;

      // convert current to percent
      x = (x-4)/16.0 * 100;

      set_float(&user_data.ka_level, x); 
   }

   if (adc_chn == 5) {
      // convert voltage to current im mA (100 Ohm)
      x = user_data.adc[5] * 10;

      // convert current to 0...2.5 bar
      x = (x-4)/16.0 * 2.5;

      set_float(&user_data.lhe_bar, x); 
   }

   adc_chn = (adc_chn + 1) % 8;

   /* convert SCS_910 voltges to degree */
   temp = (user_data.scs_910[0]*1000.0 - 224) / -2.064 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.ln2_valve_temp, temp);
   temp = (user_data.scs_910[1]*1000.0 - 224) / -2.064 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.ln2_heater_temp, temp); 

   temp = (user_data.scs_910[2]*1000.0 - 224) / -2.064 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.ln2_temp_left, temp); 
   temp = (user_data.scs_910[3]*1000.0 - 213) / -2.087 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.ln2_temp_right, temp); 
   temp = (user_data.scs_910[4]*1000.0 - 229) / -1.984 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.ln2_temp_tower, temp); 
   temp = (user_data.scs_910[5]*1000.0 - 243) / -1.946 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.ln2_temp_top, temp); 

   temp = (user_data.scs_910[6]*1000.0 - 228) / -2.018 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.lhe_temp_d_center, temp); 
   temp = (user_data.scs_910[7]*1000.0 - 230) / -1.996 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.lhe_temp_d_left, temp); 
   temp = (user_data.scs_910[8]*1000.0 - 230) / -1.981 + 270;
   if (temp < 0 || temp > 999) temp = 0;
   set_float(&user_data.lhe_temp_d_right, temp); 

   if (user_data.scs_910[9] != 0) {
      x = 1/user_data.scs_910[9];
	   temp = 0.1189*x*x*x*x-1.8979*x*x*x+12.401*x*x-35.427*x+40.5; // R5
      if (temp < 0 || temp > 999) temp = 0;
   } else 
      temp = 0;
   set_float(&user_data.lhe_temp_r_center, temp); 

   if (user_data.scs_910[10] != 0) {
      x = 1/user_data.scs_910[10];
	   temp = 0.1403*x*x*x*x-2.3273*x*x*x+15.733*x*x-46.722*x+54.5; // R6
      if (temp < 0 || temp > 999) temp = 0;
   } else
      temp = 0;
   set_float(&user_data.lhe_temp_r_left, temp); 

   if (user_data.scs_910[11] != 0) {
      x = 1/user_data.scs_910[11];
	   temp = 0.2491*x*x*x*x-4.3396*x*x*x+29.209*x*x-85.018*x+93.4; // R1
      if (temp < 0 || temp > 999) temp = 0;
   } else
      temp = 0;
   set_float(&user_data.lhe_temp_r_right, temp); 
   
   x = user_data.scs_910[12]*6;
   x = (1-x/7.32)*100;
   if (x < 0)
      x = 0;
   set_float(&user_data.lhe_level1, x); 

   /* read buttons and digital input */
   sr_read();
   
   /* manage menu on LCD display */
   lcd_menu();

   /* call LHe control loop */
   lhe_control();

   /* backup data */
   memcpy(&backup_data, &user_data, sizeof(user_data));

   /* watchdog test */
   if (user_data.error == 123)
      while(1);
}

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
#include "mscb.h"
#include "scs_2000.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-2000";
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

MSCB_INFO_VAR *variables;
float xdata user_data[64];
float xdata backup_data[64];


/* different application display modes */
unsigned char xdata app_mode;

#define APM_NORMAL   0
#define APM_PULSER   1

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

MSCB_INFO_VAR xdata vars[65];

unsigned char xdata module_nvars[8];
unsigned char xdata module_id[8];
unsigned char xdata module_index[8];

extern SCS_2000_MODULE code scs_2000_module[];
extern unsigned char idata n_variables;

void user_write(unsigned char index) reentrant;
void scan_modules(void);

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;

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

   /* initial EEPROM value */
   if (init) {
   }

   /* retrieve backup data from RAM if not reset by power on */
   SFRPAGE = LEGACY_PAGE;
   if ((RSTSRC & 0x02) == 0)
      memcpy(&user_data, &backup_data, sizeof(user_data));

   /* initialize drivers */
   for (i=0 ; i<8 ; i++)
      if (module_index[i] != 0xFF && scs_2000_module[module_index[i]].driver)
         scs_2000_module[module_index[i]].driver(module_id[i], MC_INIT, 0, i, 0, NULL);

   /* write digital outputs */
   for (i=0 ; i<n_variables ; i++)
      user_write(i);

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
   strncpy(str, svn_revision + 16, 6);
   *strchr(str, ' ') = 0;
   printf("  Revision:  %s", str);
}

#pragma NOAREGS

/*---- Special data for LED pulser ---------------------------------*/

unsigned char xdata pulser_ampl = 0;
unsigned char xdata pulser_freq = 0;
unsigned char xdata n_var_int = 0;

typedef struct {
  unsigned long  count;
  unsigned short freq;
  unsigned char  exp;
} FREQ_TABLE;

FREQ_TABLE code freq_table[] = {
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

MSCB_INFO_VAR code pulser_var[] = {
   { 1, UNIT_BYTE,    0, 0, 0, "Freq",  &pulser_freq,  0,  18,  1 },
   { 1, UNIT_PERCENT, 0, 0, 0, "Ampl",  &pulser_ampl,  0, 100,  1 },
};

/*---- Module scan -------------------------------------------------*/

void setup_variables(void)
{
unsigned char xdata port, id, i, j, n_var, n_mod, n_var_mod, changed;
char xdata * pvardata;

   app_mode = APM_NORMAL;
   changed = 0;
   n_var = n_mod = 0;
   pvardata = (char *)user_data;

   /* set "variables" pointer to array in xdata */
   variables = vars;

   /* setup special variables for LED pulser */
   read_eeprom(0, 0, &id);
   if (id == 0x02) {
      app_mode = APM_PULSER;
      memcpy(variables, pulser_var, sizeof(pulser_var));
      n_var += sizeof(pulser_var)/sizeof(MSCB_INFO_VAR);
      n_var_int = n_var;
   }

   for (port=0 ; port<8 ; port++) {
      module_id[port] = 0;
      module_nvars[port] = 0;
      module_index[port] = 0xFF;
      if (module_present(0, port)) {
         read_eeprom(0, port, &id);
   
         for (i=0 ; scs_2000_module[i].id ; i++) {
            if (scs_2000_module[i].id == id)
               break;
         }

         if (scs_2000_module[i].id == id) {
            n_var_mod = (unsigned char)scs_2000_module[i].var[0].ud;
            
            if (n_var_mod) {
               /* clone single variable definition */
               for (j=0 ; j<n_var_mod ; j++) {
                  memcpy(variables+n_var, &scs_2000_module[i].var[0], sizeof(MSCB_INFO_VAR));
                  if (strchr(variables[n_var].name, '%'))
                     *strchr(variables[n_var].name, '%') = '0'+port;
                  if (strchr(variables[n_var].name, '#'))
                     *strchr(variables[n_var].name, '#') = '0'+j;
                  variables[n_var].ud = pvardata;
                  pvardata += variables[n_var].width;
                  n_var++;
               }
            } else {
               /* copy over variable definition */
               for (j=0 ; scs_2000_module[i].var[j].width > 0 ; j++) {
                  memcpy(variables+n_var, &scs_2000_module[i].var[j], sizeof(MSCB_INFO_VAR));
                  if (strchr(variables[n_var].name, '%'))
                     *strchr(variables[n_var].name, '%') = '0'+port;
                  variables[n_var].ud = pvardata;
                  pvardata += variables[n_var].width;
                  n_var++;
               }
            }

            module_id[port] = id;
            module_nvars[port] = j;
            module_index[port] = i;
            n_mod++;
         
         } else {

           /* initialize new/unknown module */
            lcd_clear();
            lcd_goto(0, 0);
            printf("New module in port %bd", port);
            lcd_goto(0, 1);
            printf("Plese select:");

            i = 0;
            while (1) {
               lcd_goto(0, 2);
               printf(">%02bX %s            ", scs_2000_module[i].id, scs_2000_module[i].name);

               lcd_goto(0, 3);
               if (i == 0)
                  printf("SEL             NEXT");
               else if (scs_2000_module[i+1].id == 0)
                  printf("SEL        PREV     ");
               else
                  printf("SEL        PREV NEXT");

               do {
                  watchdog_refresh(0);
                  b0 = !B0;
                  b1 = !B1;
                  b2 = !B2;
                  b3 = !B3;
               } while (!b0 && !b1 && !b2 && !b3);

               if (b0) {
                  /* write module id and re-evaluate module */
                  write_eeprom(0, port, scs_2000_module[i].id);
                  port--;
                  while (b0) b0 = !B0;
                  changed = 1;
                  break;
               }

               if (b2) {
                  if (i > 0)
                     i--;
                  while (b2) b2 = !B2;
               }

               if (b3) {
                  if (scs_2000_module[i+1].id)
                     i++;
                  while (b3) b3 = !B3;
               }
            }
         }
      }
   }

   /* mark end of variables list */
   variables[n_var].width = 0;

   /* reboot if any module id has been written */
   if (changed) {
      SFRPAGE = LEGACY_PAGE;
      RSTSRC = 0x10;         // force software reset
   }

}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
unsigned char i, j, port;

   /* internal variables */
   if (app_mode == APM_PULSER) {
      if (index == 0) {
         j = module_index[0];
         scs_2000_module[j].driver(scs_2000_module[j].id, MC_SPECIAL, 0, 0, 0, &freq_table[pulser_freq].count);
      }
   }

   /* find module to which this variable belongs */
   i = n_var_int;
   for (port=0 ; port<8 ; port++) {
      if (i + module_nvars[port] > index) {
         i = index - i;
         j = module_index[port];

         if (j != 0xFF)
            scs_2000_module[j].driver(scs_2000_module[j].id, MC_WRITE, 0, port, i, variables[index].ud);

         break;

      } else
         i += module_nvars[port];
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
   /* erase eeprom of specific port */
   write_eeprom(0, data_in[0], 0xFF);
   data_out[0] = 1;
   return 1;
}

/*---- Application display -----------------------------------------*/

bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0;
unsigned long xdata last_b2 = 0, last_b3 = 0;

unsigned char ad_pulser()
{
unsigned char pulser_ampl_old, pulser_freq_old, i;

   lcd_goto(0, 0);
   printf("Frequency: %d ", freq_table[pulser_freq].freq);
   if (freq_table[pulser_freq].exp == 0)
      printf("Hz   ");
   else if (freq_table[pulser_freq].exp == 1)
      printf("kHz   ");
   else
      printf("MHz   ");

   lcd_goto(0, 1);
   printf("Amplitude: %bd %%   ", pulser_ampl);

   lcd_goto(0, 3);
   printf(" - Hz +     - Amp +");

   if (b0 && !b0_old && pulser_freq > 0)
      pulser_freq--;

   if (b1 && !b1_old && pulser_freq < 18)
      pulser_freq++;

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

   if (pulser_freq != pulser_freq_old)
      user_write(0);

   if (pulser_ampl != pulser_ampl_old) {
      for (i=2 ; i<n_variables ; i++) {
         *((unsigned char *)variables[i].ud) = pulser_ampl;
         user_write(i);
      }
   }

   pulser_ampl_old = pulser_ampl;
   pulser_freq_old = pulser_freq;
   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

unsigned char application_display(bit init)
{
static bit next, prev;
static unsigned char xdata index=0, last_index=1;
unsigned char xdata i, j, n, col;

   if (init)
      lcd_clear();
   
   if (app_mode == APM_PULSER)
      return ad_pulser();

   /* display list of modules */
   if (index != last_index) {

      next = prev = 0;

      for (n=i=0 ; i<8  ; i++);
         if (module_id[i])
            n++;

      for (col=i=0 ; i<8 ; i++) {

         if (!module_id[i])
            continue;

         if (i < index)
            continue;

         lcd_goto(0, col);
         for (j=0 ; scs_2000_module[j].id && scs_2000_module[j].id != module_id[i]; j++);
         if (scs_2000_module[j].id) {
            if (col == 3) {
               next = 1;
               break;
            }
            printf("P%bd:%02bX %s      ", i, scs_2000_module[j].id, scs_2000_module[j].name);
         } else
            printf("                    ");

         col++;
      }

      lcd_goto(0, 3);
      printf("VARS");

      lcd_goto(16, 3);
      if (next)
         printf("NEXT");
      else
         printf("    ");
        
      lcd_goto(10, 3);
      if (index > 0) 
         printf("PREV");
      else
         printf("    ");

      last_index = index;
   }


   if (next && b3 && !b3_old)
      index++;

   if (index > 0 && b2 && !b2_old)
      index--;

   /* enter menu on release of button 0 */
   if (!init && !b0 && b0_old) {
      last_index = index+1;
      return 1;
   }

   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

/*---- User loop function ------------------------------------------*/

static unsigned char idata port_index = 0, first_var_index = 0;

void user_loop(void)
{
unsigned char xdata i;

   /* read next port */
   if (module_index[port_index] != 0xFF) {

      i = module_index[port_index];
      scs_2000_module[i].driver(scs_2000_module[i].id, MC_READ, 
                                0, port_index, 0, variables[first_var_index].ud);
   }

   /* go to next port */
   first_var_index += module_nvars[port_index];
   port_index++;
   if (port_index == 8) {
      port_index = 0;
      first_var_index = 0;
   }

   /* backup data */
   memcpy(&backup_data, &user_data, sizeof(user_data));

   /* read buttons */
   b0 = !B0;
   b1 = !B1;
   b2 = !B2;
   b3 = !B3;

   /* manage menu on LCD display */
   lcd_menu();
}

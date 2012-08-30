/********************************************************************\

  Name:         scs_3000.c
  Created by:   Stefan Ritt


  Contents:     General-purpose framework for SCS-3000 control unit.

  $Id: scs_3000.c 4147 2008-03-31 13:35:08Z ritt@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"
#include "scs_3000.h"
#include "lcd.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

char code node_name[] = "SCS-3000";
char code svn_rev_3000[] = "$Rev: 4147 $";
extern char code svn_rev_main[];
extern char code svn_rev_lib[];

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

extern void lcd_menu(void);

/*---- Port definitions ----*/

bit b0, b1, b2, b3, master, module_list, beeper_on, main_startup;

/*---- Define variable parameters returned to CMD_GET_INFO command ----*/

#define N_PORT 16 /* maximal one master and one slave module */

/* data buffer (mirrored in EEPROM) */

MSCB_INFO_VAR *variables;
float xdata user_data[N_PORT*8];

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

unsigned char xdata n_box;
unsigned char xdata erase_module;

MSCB_INFO_VAR xdata vars[N_PORT*8+1];

unsigned char xdata update_data[N_PORT];
unsigned char xdata module_nvars[N_PORT];
unsigned char xdata module_id[N_PORT];
unsigned char xdata module_index[N_PORT];

unsigned char xdata memsize;
extern SCS_2001_MODULE code scs_2001_module[];
extern unsigned char idata n_variables;

void user_write(unsigned char index) reentrant;
void scan_modules(void);

/*---- User init function ------------------------------------------*/

extern SYS_INFO idata sys_info;

void user_init(unsigned char init)
{
   unsigned char xdata i, j, index, var_index;
   char xdata str[6];

   erase_module = 0xFF;

   /* green (lower) LED on by default */
   led_mode(0, 1);
   /* red (upper) LED off by default */
   led_mode(1, 0);

   /* initialize power monitor */
   for (i=0 ; i<N_PORT/8 ; i++)
      monitor_init(i);

   /* check if master or slave */
   master = is_master();
   main_startup = 1;

   if (master) {
      /* initial EEPROM value */
      if (init) {
         var_index = 0;
         for (i=0 ; i<N_PORT ; i++) {
            index = module_index[i];
            if (index != 0xFF && scs_2001_module[index].driver) {
               for (j=0 ; j<module_nvars[i] ; j++) {
                  /* default variables to zero */
                  memset(variables[var_index].ud, 0, variables[var_index].width);
                  /* allow driver to overwrite */
                  scs_2001_module[index].driver(module_id[i], MC_GETDEFAULT, i/8, i%8, j, 
                     variables[var_index].ud);
                  var_index++;
               }
            } 
         }
      }
   
      /* initialize drivers */
      for (i=0 ; i<N_PORT ; i++)
         if (module_index[i] != 0xFF && scs_2001_module[module_index[i]].driver)
            scs_2001_module[module_index[i]].driver(module_id[i], MC_INIT, i/8, i%8, 0, NULL);
   
      /* write digital outputs */
      for (i=0 ; i<n_variables ; i++)
         user_write(i);
   
      /* write remote variables */
      for (i = 0; variables[i].width; i++)
         if (variables[i].flags & MSCBF_REMOUT)
            send_remote_var(i);
   }

   /* display startup screen */
   lcd_font(6);
   lcd_fontzoom(1, 1);
   lcd_fontcolor(WHITE, BLUE);
   lcd_text(272/2, 10, 'C', sys_info.node_name);
   
   lcd_goto(0, 3);
   if (master)
      printf("   Address:  %04X", sys_info.node_addr);
   else
      printf("     Slave mode ");
   lcd_goto(0, 4);
   strncpy(str, svn_rev_3000 + 6, 6);
   *strchr(str, ' ') = 0;
   printf("  Revision:  %s", str);
   lcd_goto(0, 5);
   strncpy(str, svn_rev_lib + 6, 6);
   *strchr(str, ' ') = 0;
   printf("  Rev. Lib:  %s", str);
}

#pragma NOAREGS

/*---- Front panel button read -------------------------------------*/

unsigned char button(unsigned char i)
{
   if (i);
   return 0;
}

/*---- Power management --------------------------------------------*/

bit trip_5V = 0, trip_24V = 0, wrong_firmware = 0;
unsigned char xdata trip_5V_box;
extern unsigned char xdata n_box;

#define CPLD_FIRMWARE_REQUIRED 2

unsigned char power_management(void)
{
static unsigned long xdata last_pwr;
unsigned char xdata status, return_status, i, a[3];
unsigned short xdata d;

   if (!master)
      return 1;

   return_status = 0;

   /* only 10 Hz */
   if (time() > last_pwr+10 || time() < last_pwr) {
      last_pwr = time();
    
      for (i=0 ; i<n_box ; i++) {

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

/*---- Module scan -------------------------------------------------*/

void setup_variables(void)
{
unsigned char xdata port, id, i, j, k, n_var, n_mod, n_var_mod, changed;
char xdata * pvardata;

   /* open drain(*) / push-pull: 
      P0.0 TX1         P1.0 CS1          P2.0*LCD_SBUF    P3.0 OPT_CLK
      P0.1*RX1         P1.1 CS2          P2.1 LCD_CLK     P3.1 OPT_ALE
      P0.2 TX2         P1.2 RTC_CE       P2.2*LCD_MISO    P3.2 OPT_STR
      P0.3*RX2         P1.3 RTC_IO       P2.3 LCD_MOSI    P3.3 OPT_DATAO
                                                                      
      P0.4 EN1         P1.4 RTC_CLK      P2.4 LCD_SS      P3.4*OPT_DATAI
      P0.5 EN0         P1.5 WATCHDOG     P2.5             P3.5*OPT_STAT
      P0.6 EN_EXT/USB  P1.6              P2.6 LED0        P3.6*OPT_SPARE1
      P0.7             P1.7              P2.7 LED1        P3.7*OPT_SPARE2
    */
   SFRPAGE = CONFIG_PAGE;
   P0MDOUT = 0xF5;
   P1MDOUT = 0xFF;
   P2MDOUT = 0xFA;
   P3MDOUT = 0x0F;

   /* enable ADC */
   SFRPAGE = ADC0_PAGE;
   AMX0CF  = 0x00;               // select single ended analog inputs
   ADC0CF  = 0x98;               // ADC Clk 2.5 MHz @ 98 MHz, gain 1
   ADC0CN  = 0x80;               // enable ADC 
   REF0CN  = 0x00;               // use external voltage reference

   SFRPAGE = LEGACY_PAGE;
   REF0CN  = 0x03;               // select internal voltage reference

   /* initizlize real time clock */
   rtc_init();

   changed = 0;
   n_var = n_mod = 0;
   pvardata = (char *)user_data;

   /* count number of active boxes */
   for (i=0 ; i<N_PORT/8 ; i++)
      if (!is_present(i))
         break;
   n_box = i;

   /* turn on power and beeper (turn off later) */
   for (i=0 ; i<n_box ; i++) {
      beeper_on = 1;
      power_beeper(i, 1);
   }

   /* set "variables" pointer to array in xdata */
   variables = vars;

   for (port=0 ; port<N_PORT ; port++) {
      module_id[port] = 0;
      module_nvars[port] = 0;
      module_index[port] = 0xFF;
      read_eeprom(port/8, port%8, &id);

      if (id > 0) {
   
         for (i=0 ; scs_2001_module[i].id ; i++) {
            if (scs_2001_module[i].id == id)
               break;
         }

         if (scs_2001_module[i].id == id) {
            module_nvars[port] = 0;
            for (k=0 ; k<scs_2001_module[i].n_var ; k++) {
               n_var_mod = (unsigned char)scs_2001_module[i].var[k].ud;
               
               if (n_var_mod) {
                  /* clone single variable definition */
                  for (j=0 ; j<n_var_mod ; j++) {
                     memcpy(variables+n_var, &scs_2001_module[i].var[k], sizeof(MSCB_INFO_VAR));
                     if (strchr(variables[n_var].name, '%')) {
                        if (port > 9)
                           *strchr(variables[n_var].name, '%') = 'A'-10+port;
                        else
                           *strchr(variables[n_var].name, '%') = '0'+port;
                     }
                     if (strchr(variables[n_var].name, '#'))
                        *strchr(variables[n_var].name, '#') = '0'+j;
                     variables[n_var].ud = pvardata;
                     pvardata += variables[n_var].width;
                     n_var++;
                     module_nvars[port]++;
                  }
               } else {
                  /* copy over variable definition */
                  memcpy(variables+n_var, &scs_2001_module[i].var[k], sizeof(MSCB_INFO_VAR));
                  if (strchr(variables[n_var].name, '%')) {
                     if (port > 9)
                        *strchr(variables[n_var].name, '%') = 'A'-10+port;
                     else
                        *strchr(variables[n_var].name, '%') = '0'+port;
                  }
                  variables[n_var].ud = pvardata;
                  pvardata += variables[n_var].width;
                  n_var++;
                  module_nvars[port]++;
               }
            }

            module_id[port] = id;
            module_index[port] = i;
            n_mod++;
         
         } else {


            /* turn off any beeper */
            for (i=0 ; i<n_box ; i++)
               power_beeper(i, 0);

            /* initialize new/unknown module */
            lcd_clear();
            lcd_goto(0, 0);
            printf("New module in port %bd", port);
            lcd_goto(0, 1);
            printf("Plese select:");

            i = 0;
            while (1) {
               lcd_goto(0, 2);
               printf(">%02bX %s            ", scs_2001_module[i].id, scs_2001_module[i].name);

               lcd_goto(0, 3);
               if (i == 0)
                  printf("SEL             NEXT");
               else if (scs_2001_module[i+1].id == 0)
                  printf("SEL        PREV     ");
               else
                  printf("SEL        PREV NEXT");

               do {
                  watchdog_refresh(0);
                  b0 = button(0);
                  b1 = button(1);
                  b2 = button(2);
                  b3 = button(3);
               } while (!b0 && !b1 && !b2 && !b3);

               if (b0) {
                  /* write module id and re-evaluate module */
                  write_eeprom(port/8, port%8, scs_2001_module[i].id);
                  port--;
                  while (b0) b0 = button(0);
                  changed = 1;
                  break;
               }

               if (b2) {
                  if (i > 0)
                     i--;
                  while (b2) b2 = button(2);
               }

               if (b3) {
                  if (scs_2001_module[i+1].id)
                     i++;
                  while (b3) b3 = button(3);
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
      RSTSRC  = 0x10;         // force software reset
   }

}

/*---- User write function -----------------------------------------*/

void user_write(unsigned char index) reentrant
{
   /* will be updated in main loop */
   update_data[index/8] |= (1 << (index % 8));
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
   /* erase module eeprom of specific port */
   erase_module = data_in[0];
   data_out[0] = 1;
   return 1;
}

/*---- Application display -----------------------------------------*/

bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0;
unsigned long xdata last_b2 = 0, last_b3 = 0;

unsigned char application_display(bit init)
{
static bit next;
static unsigned char xdata index, last_index, port;
unsigned char xdata i, n, j, col;

   if (init) {
      lcd_clear();
      index = 0;
      last_index = 255;
   }

   if (!master)
      return 0;

   if (!module_list) {
      lcd_goto(0, 0);
      for (i=0 ; i<7-strlen(sys_info.node_name)/2 ; i++)
         puts(" ");
      puts("** ");
      puts(sys_info.node_name);
      puts(" ** ");
      
      lcd_goto(0, 1);
      printf("   Address:  %04X", sys_info.node_addr);

      lcd_goto(1, 2);
      rtc_print();

      lcd_goto(0, 3);
      puts("MOD  VARS  SYS      ");
   }

   if (module_list) {
      /* display list of modules */
      if (index != last_index) {
   
         next = 0;
   
         for (col=i=n=0 ; i<N_PORT ; i++) {
   
            if (!module_id[i])
               continue;
   
            n++;
            if (n <= index)
               continue;
   
            lcd_goto(0, col);
            for (j=0 ; scs_2001_module[j].id && scs_2001_module[j].id != module_id[i]; j++);
            if (scs_2001_module[j].id) {
               if (col == 3) {
                  next = 1;
                  break;
               }
               printf("P%bd:%02bX %s          ", i, scs_2001_module[j].id, scs_2001_module[j].name);
            } else
               printf("                    ");
   
            col++;
         }
   
         lcd_goto(0, 3);
         puts("ESC      ");
   
         lcd_goto(16, 3);
         if (next)
            printf("  %c ", 0x13); // V
         else
            puts("    ");
           
         lcd_goto(10, 3);
         if (index > 0)
            printf("  %c ", 0x12); // ^
         else
            puts("    ");
   
         last_index = index;
      }
   
   
      if (next && b3 && !b3_old)
         index++;
   
      if (index > 0 && b2 && !b2_old)
         index--;
   
      /* exit on button 0 */
      if (b0 && !b0_old) {
         module_list = 0;
         b0_old = b0;
         lcd_clear();
      }

      /* erase EEPROM on release of button 1 (hidden functionality) */
      if (!b1 && b1_old) {
   
         for (port=0,i=0 ; port<N_PORT ; port++)
            if (module_present(port/8, port%8))
               i++;
   
         for (port=0 ; port<N_PORT ; port++)
            if (module_present(port/8, port%8))
               break;
   
         if (i == 0)
            return 0;
         
         lcd_clear();
   
         lcd_goto(0, 3);
         if (i == 0)
            printf("ESC ERASE           ");
         else
            printf("ESC ERASE  PREV NEXT");
   
         while (1) {
   
            lcd_goto(0, 0);
            printf("    Erase module    ");
            lcd_goto(0, 1);
            printf("    in port %bd ?   ", port);
   
            do {
               watchdog_refresh(0);
               b0 = button(0);
               b1 = button(1);
               b2 = button(2);
               b3 = button(3);
            } while (!b0 && !b1 && !b2 && !b3);
   
            if (b0) {
               while (b0) b0 = button(0);
               lcd_clear();
               last_index = 255; // force re-display
               b0_old = b1_old = 0;
               return 0;
            }
            
            if (b1) {
               write_eeprom(0, port, 0xFF);
               while (b1) b1 = button(1);
               SFRPAGE = LEGACY_PAGE;
               RSTSRC  = 0x10;   // force software reset
               break;
            }
   
            if (b2) {
               port = (port+N_PORT-1) % N_PORT;
   
               while (!module_present(port/8, N_PORT))
                  port = (port+N_PORT-1) % N_PORT;
   
               while (b2) b2 = button(2);
            }
   
            if (b3) {
               port = (port+1) % N_PORT;
   
               while (!module_present(port/8, port%8))
                  port = (port+1) % N_PORT;
   
               while (b3) b3 = button(3);
            }
         }
      }
   }

   /* enter variables menu on release of button 1 */
   if (!module_list && !b1 && b1_old) {
      b1_old = b1;
      return 1;
   }

   /* enter system variables menu on release of button 2 */
   if (!module_list && !b2 && b2_old) {
      b2_old = b2;
      return 2;
   }

   /* show modules on button 0 */
   if (b0 && !b0_old) {
      module_list = 1;
      lcd_clear();
      last_index = index + 1; // force display
   }

   b0_old = b0;
   b1_old = b1;
   b2_old = b2;
   b3_old = b3;

   return 0;
}

/*---- User loop function ------------------------------------------*/

static unsigned char xdata port_index = 0, first_var_index = 0;
static xdata unsigned long last_menu;
 
void user_loop(void)
{
unsigned char xdata index, i, j, n, port;
float xdata value;

   if (master) {

      /* check if variabled needs to be written */
      for (index=0 ; index<N_PORT*8 ; index++) {
         if (update_data[index/8] & (1 << (index % 8))) {
            /* find module to which this variable belongs */
            i = 0;
            for (port=0 ; port<N_PORT ; port++) {
               if (i + module_nvars[port] > index) {
                  i = index - i;
                  j = module_index[port];
         
                  if (j != 0xFF && scs_2001_module[j].driver)
                     scs_2001_module[j].driver(scs_2001_module[j].id, MC_WRITE, 
                                               port/8, port%8, i, variables[index].ud);
         
                  break;
         
               } else
                  i += module_nvars[port];
            }
            update_data[index/8] &= ~(1 << (index % 8));
         }
      }
   
      /* read next port */
      if (module_index[port_index] != 0xFF) {
   
         i = module_index[port_index];
         for (j=0 ; j<module_nvars[port_index] ; j++) {
            if (scs_2001_module[i].driver)
               n = scs_2001_module[i].driver(scs_2001_module[i].id, MC_READ, 
                                             port_index/8, port_index%8, j, &value);
            else
               n = 0;
   
            if (n>0) {
               DISABLE_INTERRUPTS;
               memcpy(variables[first_var_index+j].ud, &value, n);
               ENABLE_INTERRUPTS;
            }
         }
      }
   
      /* go to next port */
      first_var_index += module_nvars[port_index];
      port_index++;
      if (port_index == N_PORT) {
         port_index = 0;
         first_var_index = 0;
      }
   
      /* check for erase module eeprom */
      if (erase_module != 0xFF) {
         write_eeprom(erase_module/8, erase_module%8, 0xFF);
         erase_module = 0xFF;
      }
   
   } /* if (master) */

   /* do menu business every 100 ms */
   if (time() > last_menu+10 || time() < last_menu) {
      last_menu = time();

      /* read buttons */
      b0 = button(0);
      b1 = button(1);
      b2 = button(2);
      b3 = button(3);
   
      /* manage menu on LCD display */
      if (master)
         lcd_menu();
      else {
      
         if (time() > 300) {
            if (main_startup) {
               main_startup = 0;
               lcd_clear();
            }

            lcd_goto(5, 1);
            printf("SLAVE MODE");
            lcd_goto(5, 2);
            printf("Address %2bd", slave_addr());
         }
      }
   }

   /* turn off beeper after 100 ms */
   if (beeper_on && time() > 10) {
      beeper_on = 0;
      for (i=0 ; i<n_box ; i++)
         power_beeper(i, 0);
   }
}

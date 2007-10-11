/********************************************************************\

  Name:         lcd_menu.c
  Created by:   Stefan Ritt


  Contents:     Menu functions for SCS-1000 using LCD display
                and four buttons

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include "mscbemb.h"

/*------------------------------------------------------------------*/

extern unsigned char application_display(bit init);
extern void user_write(unsigned char index) reentrant;
extern MSCB_INFO_VAR *variables;
extern SYS_INFO sys_info;
extern bit flash_param;
extern unsigned char idata _flkey;

extern bit b0, b1, b2, b3;     // buttons defined in scs-1000.c
extern unsigned char idata n_variables;

/*------------------------------------------------------------------*/

bit startup = 1;                     // true for the first 3 seconds
bit in_menu = 0;                     // false for application display
bit system_menu = 0;                 // true if in system menu
bit enter_mode = 0;                  // true if editing vars
bit enter_mode_date = 0;             // true if editing date/time
unsigned char xdata var_index;       // variable to display
unsigned char xdata date_index;      // day/month/year index

float xdata f_var;
unsigned char xdata dt_bcd[6];
char xdata time_str[10];
char xdata date_str[10];

/*------------------------------------------------------------------*/

typedef struct {
   unsigned char id;
   char name[4];
} NAME_TABLE;

NAME_TABLE code prefix_table[] = {
   {PRFX_PICO, "p",},
   {PRFX_NANO, "n",},
   {PRFX_MICRO, "\344",},
   {PRFX_MILLI, "m",},
   {PRFX_NONE, "",},
   {PRFX_KILO, "k",},
   {PRFX_MEGA, "M",},
   {PRFX_GIGA, "G",},
   {PRFX_TERA, "T",},
   {0}
};

NAME_TABLE code unit_table[] = {

   {UNIT_METER, "m",},
   {UNIT_GRAM, "g",},
   {UNIT_SECOND, "s",},
   {UNIT_MINUTE, "m",},
   {UNIT_HOUR, "h",},
   {UNIT_AMPERE, "A",},
   {UNIT_KELVIN, "K",},
   {UNIT_CELSIUS, "\337C",},
   {UNIT_FARENHEIT, "\337F",},

   {UNIT_HERTZ, "Hz",},
   {UNIT_PASCAL, "Pa",},
   {UNIT_BAR, "ba",},
   {UNIT_WATT, "W",},
   {UNIT_VOLT, "V",},
   {UNIT_OHM, "\364",},
   {UNIT_TESLA, "T",},
   {UNIT_LITERPERSEC, "l/s",},
   {UNIT_RPM, "RPM",},
   {UNIT_FARAD, "F",},
   {UNIT_JOULE, "J",},

   {UNIT_PERCENT, "%",},
   {UNIT_PPM, "RPM",},
   {UNIT_COUNT, "cnt",},
   {UNIT_FACTOR, "x",},
   {0}
};

/*------------------------------------------------------------------*/

unsigned char n_sysvar = 6;

MSCB_INFO_VAR code sysvar[] = {

   {  2, UNIT_WORD, 0, 0, 0, "Node Adr", &sys_info.node_addr, 0, 0, 1 },
   {  2, UNIT_WORD, 0, 0, 0, "Grp Adr",  &sys_info.group_addr, 0, 0, 1 },

   { 10, UNIT_STRING,0,0, 0, "Time",     &time_str, 0, 0, 1},
   { 10, UNIT_STRING,0,0, 0, "Date",     &date_str, 0, 0, 1},

   {  0, UNIT_BOOLEAN, 0, 0, MSCBF_DATALESS, "Flash" },
   {  0, UNIT_BOOLEAN, 0, 0, MSCBF_DATALESS, "Reboot" },

};

/*------------------------------------------------------------------*/

static char xdata str[12];

void display_value(MSCB_INFO_VAR *pvar, void *pd)
{
   unsigned char i, unit_len;
   
   if (pvar->unit == UNIT_STRING) {
      strncpy(str, (char *)pd, 10);
      if (strlen(str) < 10)
         for (i=0 ; i<10-strlen(str) ; i++)
           putchar(' ');
      puts(str);
   } else {

      str[0] = 0;

      /* evaluate prefix */
      if (pvar->prefix) {
         for (i = 0; prefix_table[i].id; i++)
            if (prefix_table[i].id == pvar->prefix)
               break;
         if (prefix_table[i].id)
            strcat(str, prefix_table[i].name);
      }
   
      /* evaluate unit */
      if (pvar->unit) {
         for (i = 0; unit_table[i].id; i++)
            if (unit_table[i].id == pvar->unit)
               break;
         if (unit_table[i].id)
            strcat(str, unit_table[i].name);
      }

      unit_len = strlen(str);

      switch (pvar->width) {
      case 0:
         puts("          ");
         break;

      case 1:
         if (pvar->flags & MSCBF_SIGNED)
            printf("%b*bd", 10-unit_len, *((char *) pd));
         else
            printf("%b*bu", 10-unit_len, *((unsigned char *) pd));
         break;

      case 2:
         if (pvar->flags & MSCBF_SIGNED)
            printf("%b*d", 10-unit_len, *((short *) pd));
         else
            printf("%b*u", 10-unit_len, *((unsigned short *) pd));
         break;

      case 4:
         if (pvar->flags & MSCBF_FLOAT)
            printf("%b*.3f", 10-unit_len, *((float *) pd));
         else {
            if (pvar->flags & MSCBF_SIGNED)
               printf("%b*ld", 10-unit_len, *((long *) pd));
            else
               printf("%b*lu", 10-unit_len, *((unsigned long *) pd));
         }
         break;
      }

      puts(str);
   }
}

/*------------------------------------------------------------------*/

void date_inc(unsigned char index, unsigned char pos, char delta)
{
   char d;

   if (index == 0) {
      /* time */
      d = time_str[pos*3+1]-'0' + (time_str[pos*3]-'0')*10;
      d += delta;
      if (pos == 0) {        // hour
         if (d > 23) d = 0;
         if (d < 0) d = 23;
      } else {               // minute, second
         if (d > 59) d = 0; 
         if (d < 0) d = 59;
      }
      d = (d / 10)*0x10 + d % 10; // convert back to BCD
      rtc_write_item(pos+3, d);
   } else {
      /* date */
      d = date_str[pos*3+1]-'0' + (date_str[pos*3]-'0')*10;
      d += delta;
      if (pos == 0) {        // day
         if (d > 31) d = 0;
         if (d < 0) d = 31;
      } else if (pos == 1) { // month
         if (d > 12) d = 0;
         if (d < 0) d = 12;
      } else {               // year
         if (d > 99) d = 0;
         if (d < 0) d = 99;
      }
      d = (d / 10)*0x10 + d % 10; // convert back to BCD
      rtc_write_item(pos, d);
   }
}

/*------------------------------------------------------------------*/

void var_inc(MSCB_INFO_VAR *pvar, char sign)
{
   if (pvar->unit == UNIT_STRING)
      return;

   switch (pvar->width) {
   case 0:
      return;
      break;

   case 1:
      if (pvar->flags & MSCBF_SIGNED) {
         *((char *) &f_var) += pvar->delta * sign;
         if (pvar->min != pvar->max) {
            if (*((char *) &f_var) < pvar->min)
               *((char *) &f_var) = pvar->min;
            if (*((char *) &f_var) > pvar->max)
               *((char *) &f_var) = pvar->max;
         }
      } else {
         if (sign == -1 && *((unsigned char *) &f_var) == pvar->min &&
             pvar->min != pvar->max)
            break;
         *((unsigned char *) &f_var) += pvar->delta * sign;
         
         if (pvar->min != pvar->max) {
            if (*((unsigned char *) &f_var) < pvar->min)
               *((unsigned char *) &f_var) = pvar->min;
            if (*((unsigned char *) &f_var) > pvar->max)
               *((unsigned char *) &f_var) = pvar->max;
         }
      }

      break;

   case 2:
      if (pvar->flags & MSCBF_SIGNED) {
         *((short *) &f_var) += pvar->delta * sign;
         if (pvar->min != pvar->max) {
            if (*((short *) &f_var) < pvar->min)
               *((short *) &f_var) = pvar->min;
            if (*((short *) &f_var) > pvar->max)
               *((short *) &f_var) = pvar->max;
          }
      } else {
         if (sign == -1 && *((unsigned short *) &f_var) == pvar->min &&
             pvar->min != pvar->max)
            break;
         *((unsigned short *) &f_var) += pvar->delta * sign;

         if (pvar->min != pvar->max) {
            if (*((unsigned short *) &f_var) < pvar->min)
               *((unsigned short *) &f_var) = pvar->min;
            if (*((unsigned short *) &f_var) > pvar->max)
               *((unsigned short *) &f_var) = pvar->max;
         }
      }
      break;


   case 4:
      if (pvar->flags & MSCBF_FLOAT) {
         f_var += pvar->delta * sign;
         if (f_var < pvar->min)
            f_var = pvar->min;
         if (f_var > pvar->max)
            f_var = pvar->max;
      } else {
         if (pvar->flags & MSCBF_SIGNED) {
            *((long *) &f_var) += pvar->delta * sign;
            if (pvar->min != pvar->max) {
               if (*((long *) &f_var) < pvar->min)
                  *((long *) &f_var) = pvar->min;
               if (*((long *) &f_var) > pvar->max)
                  *((long *) &f_var) = pvar->max;
            }
         } else {
            if (sign == -1 && *((unsigned long *) &f_var) == pvar->min &&
                pvar->min != pvar->max)
               break;
            *((unsigned long *) &f_var) += pvar->delta * sign;

            if (pvar->min != pvar->max) {
               if (*((unsigned long *) &f_var) < pvar->min)
                  *((unsigned long *) &f_var) = pvar->min;
               if (*((unsigned long *) &f_var) > pvar->max)
                  *((unsigned long *) &f_var) = pvar->max;
            }
         }
      }
      break;
   }
}

/*------------------------------------------------------------------*/

void display_name(unsigned char index, MSCB_INFO_VAR *pvar)
{
   /* display variable name */
   memcpy(str, pvar->name, 8);
   str[8] = 0;
   printf("%2bu:%s", index, str);
   puts("        ");
}

/*------------------------------------------------------------------*/

#if defined(SCS_1001) || defined(SCS_2000)

static xdata unsigned long last_disp, last_b2, last_b3;
static bit b0_old, b1_old, b2_old, b3_old;

void lcd_menu()
{
   MSCB_INFO_VAR * xdata pvar;
   unsigned char idata i, max_index, start_index;

   /* clear startup screen after 3 sec. */
   if (startup) {

      /* initialize xdata memory */
      var_index = 0;
      last_disp = 0;
      last_b2 = 0;
      last_b3 = 0;
      b0_old = 0;
      b1_old = 0;
      b2_old = 0;
      b3_old = 0;

      if(time() > 300) {
         startup = 0;
         application_display(1);
      } else                
        return;
   }

   /* do menu business only every 100ms */
   if (time() > last_disp+10) {
      last_disp = time();

      if (!in_menu) {

         /* if not in menu, call application display */
         if (application_display(0)) {
            in_menu = 1;
         }
      
      } else {

         /* update date/time in system menu */
         if (system_menu) {
            rtc_read(dt_bcd);
            rtc_conv_date(dt_bcd, date_str);
            rtc_conv_time(dt_bcd, time_str);
         }

         /* display variables */

         if (system_menu) {
            pvar = sysvar;
            max_index = n_sysvar-1;
         } else {
            pvar = variables;
            max_index = n_variables-1;
         }
   
         if (max_index < 3)
            start_index = 0;
         else {
            if (var_index == 0)
               start_index = 0;
            else if (var_index == max_index)
               start_index = var_index-2;
            else 
               start_index = var_index-1;
         }

         if (n_variables == 0 && !system_menu) {
            lcd_goto(0, 0);
            printf("No variables defined");
         } else {
            for (i=0 ; i<3 ; i++) {
   
               lcd_goto(0, i);
               if (start_index+i == var_index) 
                  putchar(0x15);
               else
                  putchar(' ');
   
               lcd_goto(1, i);
               if (start_index+i > max_index)
                  printf("                   ");
               else {
                  display_name(start_index+i, &pvar[start_index+i]);
                  lcd_goto(10, i);
   
                  if (enter_mode && start_index+i == var_index)
                     display_value(&pvar[start_index+i], &f_var);
                  else
                     display_value(&pvar[start_index+i], pvar[start_index+i].ud);
               }
            }
         }
   
         if (system_menu)
            pvar = &sysvar[var_index];
         else
            pvar = &variables[var_index];

         if (enter_mode_date) {

            lcd_goto(0, 3);
            puts("ESC ENTER   -    +  ");
            lcd_goto(13+date_index*3, var_index-start_index);

            /* evaluate ESC button */
            if (!b0 && b0_old) {
               enter_mode_date = 0;
               lcd_cursor(0);
            }

            /* evaluate ENTER button */
            if (b1 && !b1_old) {
               date_index++;

               if (date_index == 3) {
                  enter_mode_date = 0;
                  lcd_cursor(0);
               }
            }

            /* evaluate "-" button */
            if (b2 && !b2_old) {
               date_inc(var_index-2, date_index, -1);
               last_b2 = time();
            }
            if (b2 && time() > last_b2 + 70)
               date_inc(var_index-2, date_index, -1);
            if (!b2)
               last_b2 = 0;

            /* evaluate "+" button */
            if (b3 && !b3_old) {
               date_inc(var_index-2, date_index, 1);
               last_b3 = time();
            }
            if (b3 && time() > last_b3 + 70)
               date_inc(var_index-2, date_index, 1);
            if (!b3)
               last_b3 = 0;
         }

         else if (enter_mode) {

            lcd_goto(0, 3);
            puts("ESC ENTER   -     + ");
            lcd_goto(19, var_index-start_index);

            /* evaluate ESC button */
            if (!b0 && b0_old) {
               enter_mode = 0;
               lcd_cursor(0);
            }

            /* evaluate ENTER button */
            if (b1 && !b1_old) {
               enter_mode = 0;
               lcd_cursor(0);
               memcpy(pvar->ud, &f_var, pvar->width);
               
               if (system_menu) {
                  /* flash new address */
                  if (var_index == 0 || var_index == 1) {
                     flash_param = 1;
                     _flkey = 0xF1;
                  }
               } else {
                  user_write(var_index);
#ifdef UART1_MSCB
                  if (pvar->flags & MSCBF_REMOUT)
                     send_remote_var(var_index);
#endif
               }
            }

            /* evaluate "-" button */
            if (b2 && !b2_old) {
               var_inc(pvar, -1);
               last_b2 = time();
            }
            if (b2 && time() > last_b2 + 70)
               var_inc(pvar, -1);
            if (b2 && time() > last_b2 + 500)
               var_inc(pvar, -9);
            if (!b2)
               last_b2 = 0;

            /* evaluate "+" button */
            if (b3 && !b3_old) {
               var_inc(pvar, 1);
               last_b3 = time();
            }
            if (b3 && time() > last_b3 + 70)
               var_inc(pvar, 1);
            if (b3 && time() > last_b3 + 500)
               var_inc(pvar, 9);
            if (!b3)
               last_b3 = 0;

         } else {
            
            lcd_goto(0, 3);
            if ((n_variables > 0 || system_menu) && (pvar->delta > 0 || (pvar->flags & MSCBF_DATALESS)))
               puts("ESC ENTER   ");
            else
               puts("ESC         ");
            putchar(0x12); // ^
            puts("     ");
            putchar(0x13); // v
            puts(" ");
   
            /* enter application display on release of ESC button */
            if (!b0 && b0_old) {
               if (system_menu) {
                  system_menu = 0;
                  var_index = 0;
                  lcd_clear();
               } else {
                  in_menu = 0;
                  application_display(1);
               }
            }
   
            /* evaluate ENTER button */
            if (b1 && !b1_old) {

               if (n_variables > 0 && pvar->width <= 4 && pvar->width > 0 && pvar->delta > 0) {
                  enter_mode = 1;
                  memcpy(&f_var, pvar->ud, pvar->width);
                  lcd_goto(19, var_index-start_index);
                  lcd_cursor(1);
               }

               if (system_menu && (var_index == 2 || var_index == 3)) {
                  enter_mode_date = 1;
                  date_index = 0;

                  /* display cursor */
                  lcd_goto(13, var_index-start_index);
                  lcd_cursor(1);
               }

               /* check for flash command */
               if (system_menu && var_index == 4) {
                  flash_param = 1;
                  _flkey = 0xF1;
                  lcd_clear();
                  lcd_goto(0, 0);
                  puts("Parameters written");
                  lcd_goto(0, 1);
                  puts("to flash memory.");
                  delay_ms(3000);
               }
 
               /* check for reboot command */
               if (system_menu && var_index == 5) {
#ifdef CPU_C8051F120
                  SFRPAGE = LEGACY_PAGE;
#endif
                  RSTSRC = 0x10;         // force software reset
               }
            }

            /* evaluate prev button */
            if (b2 && !b2_old) {
               if (var_index == 0) {
                 if (!system_menu)
                    var_index = n_variables-1;
                 else
                    var_index = n_sysvar-1;
               } else
                 var_index--;
               last_b2 = time();
            }
            if (b2 && time() > last_b2 + 70) {
               if (var_index == 0) {
                 if (!system_menu)
                    var_index = n_variables-1;
                 else
                    var_index = n_sysvar-1;
               } else
                 var_index--;
            }
            if (!b2)
               last_b2 = 0;

            if (system_menu)
               i = n_sysvar;
            else
               i = n_variables;

            /* evaluate next button */
            if (b3 && !b3_old) {
               var_index = (var_index + 1) % i;
               last_b3 = time();
            }
            if (b3 && time() > last_b3 + 70)
               var_index = (var_index + 1) % i;
            if (!b3)
               last_b3 = 0;

            /* check for system menu */
            if (b2 && b3 && !system_menu) {
               var_index = 0;
               system_menu = 1;
            }
         }
      }

      b0_old = b0;
      b1_old = b1;
      b2_old = b2;
      b3_old = b3;
   }
}

#else // SCS_1001

static xdata unsigned long last_disp = 0, last_b2 = 0, last_b3 = 0;
static bit b0_old = 0, b1_old = 0, b2_old = 0, b3_old = 0;

void lcd_menu()
{
   MSCB_INFO_VAR * xdata pvar;
   unsigned char idata i;

   /* clear startup screen after 3 sec. */
   if (startup) {
      if(time() > 300) {
         startup = 0;
         lcd_clear();
      } else
        return;
   }

   /* do menu business only every 100ms */
   if (time() > last_disp+10) {
      last_disp = time();

      if (system_menu)
         pvar = &sysvar[var_index];
      else
         pvar = &variables[var_index];

      if (!in_menu) {

         /* if not in menu, call application display */
         if (application_display(0))
            in_menu = 1;
      
      } else {

         if (enter_mode) {

            /* display variables */
            lcd_goto(10, 0);
            display_value(pvar, &f_var);
            lcd_goto(0, 1);
            puts("ESC ENTER   -    +  ");

            /* evaluate ESC button */
            if (b0 && !b0_old)
               enter_mode = 0;

            /* evaluate ENTER button */
            if (b1 && !b1_old) {
               enter_mode = 0;
               memcpy(pvar->ud, &f_var, pvar->width);
               
               if (system_menu) {
                  /* flash new address */
                  if (var_index == 0 || var_index == 1) {
                     flash_param = 1;
                     _flkey = 0xF1;
                  }
               } else {
                  user_write(var_index);
#ifdef UART1_MSCB
                  if (variables[var_index].flags & MSCBF_REMOUT)
                     send_remote_var(var_index);
#endif
               }
            }

            /* evaluate "-" button */
            if (b2 && !b2_old) {
               var_inc(pvar, -1);
               last_b2 = time();
            }
            if (b2 && time() > last_b2 + 70)
               var_inc(pvar, -1);
            if (!b2)
               last_b2 = 0;

            /* evaluate "+" button */
            if (b3 && !b3_old) {
               var_inc(pvar, 1);
               last_b3 = time();
            }
            if (b3 && time() > last_b3 + 70)
               var_inc(pvar, 1);
            if (!b3)
               last_b3 = 0;

         } else {
            
            /* display variables */
            lcd_goto(0, 0);
            display_name(var_index, pvar);
            lcd_goto(10, 0);
            display_value(pvar, pvar->ud);
            lcd_goto(0, 1);

            if (pvar->delta > 0 || (pvar->flags & MSCBF_DATALESS))
               puts("ESC ENTER  PREV NEXT");
            else
               puts("ESC        PREV NEXT");
   
            /* evaluate ESC button */
            if (b0 && !b0_old) {
               if (system_menu) {
                  system_menu = 0;
                  var_index = 0;
               } else {
                  in_menu = 0;
                  application_display(1);
               }
            }
   
            /* evaluate ENTER button */
            if (b1 && !b1_old) {

               if (pvar->width <= 4 && pvar->width > 0 && pvar->delta > 0) {
                  enter_mode = 1;
                  memcpy(&f_var, pvar->ud, pvar->width);
               }

               /* check for flash command */
               if (system_menu && var_index == 2) {
                  flash_param = 1;
                  lcd_clear();
                  lcd_goto(0, 0);
                  puts("Parameters written");
                  lcd_goto(0, 1);
                  puts("to flash memory.");
                  delay_ms(3000);
               }
 
               /* check for reboot command */
               if (system_menu && var_index == 3) {
#ifdef CPU_C8051F120
                  SFRPAGE = LEGACY_PAGE;
#endif
                  RSTSRC = 0x10;         // force software reset
               }
            }

            /* evaluate prev button */
            if (b2 && !b2_old) {
               if (var_index == 0) {
                 if (!system_menu)
                    var_index = n_variables-1;
                 else
                    var_index = n_sysvar-1;
               } else
                 var_index--;
               last_b2 = time();
            }
            if (b2 && time() > last_b2 + 70) {
               if (var_index == 0) {
                 if (!system_menu)
                    var_index = n_variables-1;
                 else
                    var_index = n_sysvar-1;
               } else
                 var_index--;
            }
            if (!b2)
               last_b2 = 0;

            if (system_menu)
               i = n_sysvar;
            else
               i = n_variables;

            /* evaluate next button */
            if (b3 && !b3_old) {
               var_index = (var_index + 1) % i;
               last_b3 = time();
            }
            if (b3 && time() > last_b3 + 70)
               var_index = (var_index + 1) % i;
            if (!b3)
               last_b3 = 0;

            /* check for system menu */
            if (b2 && b3 && !system_menu) {
               var_index = 0;
               system_menu = 1;
            }
         }
      }

      b0_old = b0;
      b1_old = b1;
      b2_old = b2;
      b3_old = b3;
   }
}

#endif

/********************************************************************\

  Name:         calib_hvr.c
  Created by:   Stefan Ritt

  Contents:     Calibration program for HVR-200

  $Id$

\********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include "mscb.h"

/*------------------------------------------------------------------*/

#define CH_CONTROL      0
#define CH_VDEMAND      1
#define CH_VMEAS        2
#define CH_IMEAS        3
#define CH_STATUS       4
#define CH_TRIPCNT      5
#define CH_RAMPUP       6
#define CH_RAMPDOWN     7
#define CH_ULIMIT       8
#define CH_ILIMIT       9
#define CH_RILIMIT     10
#define CH_TRIPMAX     11
#define CH_TRIPTIME    12
#define CH_ADCGAIN     13
#define CH_ADCOFS      14
#define CH_DACGAIN     15
#define CH_DACOFS      16
#define CH_CURVGAIN    17
#define CH_CURGAIN     18
#define CH_CUROFS      19

#define HV_SET_DELAY   5000 /* five seconds for voltage to stabilize */

#define STATUS_NEGATIVE    (1<<0)
#define STATUS_LOWCUR      (1<<1)

/*------------------------------------------------------------------*/

void eprintf(const char *format, ...)
{
   va_list ap;
   char msg[1000];
   FILE *f;

   va_start(ap, format);
   vsprintf(msg, format, ap);
   va_end(ap);

   fputs(msg, stdout);
   f = fopen("calib_hvr.log", "at");
   if (f) {
      fprintf(f, "%s", msg);
      fclose(f);
   }
}

/*------------------------------------------------------------------*/

void stop()
{
   char str[80];

   eprintf("\007--- hit any key to exit ---");
   fgets(str, sizeof(str), stdin);

   exit(0);
}

/*------------------------------------------------------------------*/

float read_voltage(int fd, unsigned short adr)
{
   int status, size;
   float value;
   char str[80];

   status = 0;
   value = 0;
   if (adr != 0xFFFF) {
      do {
         size = sizeof(float);
         status = mscb_read(fd, adr, 1, &value, &size);
         if (status != MSCB_SUCCESS)
            eprintf("Error reading DVM (status=%d)\n", status);
         value = (float) fabs(value);
         if (value == 0) {
            eprintf("Error reading DVM, re-trying...\n");
            Sleep(1000);
         }
      } while (value == 0); /* repeat until valid reading */
   }

   if (status != MSCB_SUCCESS) {
      eprintf("Please enter absolute voltage from DVM: ");
      fgets(str, sizeof(str), stdin);
      value = (float) atof(str);
      value = (float) fabs(value);
   }

   return value;
}

/*------------------------------------------------------------------*/

void reset_param(int fd, unsigned short adr)
{
   float f;
   int   d;

   /* init variables */
   f = 0;
   d = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   mscb_write(fd, adr, CH_ADCOFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_DACOFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_CUROFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_CURVGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_RAMPUP, &f, sizeof(float));
   mscb_write(fd, adr, CH_RAMPDOWN, &f, sizeof(float));

   f = 1;
   mscb_write(fd, adr, CH_ADCGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_DACGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_CURGAIN, &f, sizeof(float));
}

/*------------------------------------------------------------------*/

int quick_check(int fd, unsigned short adr, unsigned short adr_dvm, unsigned short adr_mux)
{
   float f;
   int size, d;
   float v_adc;

   eprintf("\n**** Checking channel %d ****\n\n", adr);

   /*--- initialization ----*/

   /* init variables */
   reset_param(fd, adr);

   /* disable hardware current limit */
   f = 9999;
   mscb_write(fd, adr, CH_ILIMIT, &f, sizeof(float));

   /* set CSR to HV on, no regulation */
   d = 1;
   mscb_write(fd, adr, CH_CONTROL, &d, 1);

   /* set demand to 500V */
   eprintf("Setting       500.0 Volt\n");
   f = 500;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   Sleep(HV_SET_DELAY);

   /* read ADC */
   size = sizeof(float);
   mscb_read(fd, adr, CH_VMEAS, &v_adc, &size);
   eprintf("HVR ADC reads %5.1lf Volt\n", v_adc);

   if (v_adc < 300) {
      eprintf("HVR ADC voltage too low, aborting.\n");
      stop();
   }

   f = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));

   return 1;
}

/*------------------------------------------------------------------*/

int calib(int fd, unsigned short adr, unsigned short adr_dvm, unsigned short adr_mux, 
          float rin_dvm, int first, int next_adr)
{
   int size, d, status, low_current;
   char str[80];
   MSCB_INFO info;
   float f1, v_adc1, v_adc2, v_multi1, v_multi2, adc_gain, adc_ofs, 
      dac_gain, dac_ofs, i1, i2, i_high, i_low, i_gain, i_vgain, i_ofs, u_high, u_low;

   eprintf("\n**** Calibrating channel %d ****\n\n", adr);

   /*--- initialization ----*/

   /* init variables */
   reset_param(fd, adr);

   /* read status */
   d = 0;
   size = 1;
   mscb_read(fd, adr, CH_STATUS, &d, &size);
   low_current = (d & STATUS_LOWCUR) > 0;

   /* read type */
   mscb_info(fd, adr, &info);
   if (stricmp(info.node_name, "HVR-800") == 0)
      low_current = 1;

   if (low_current)
      eprintf("#### Found low current module\n");

   /* repeat until correct */
   do {
      /* disable hardware current limit */
      f1 = 9999;
      mscb_write(fd, adr, CH_ILIMIT, &f1, sizeof(float));

      /* set CSR to HV on, no regulation */
      d = 1;
      mscb_write(fd, adr, CH_CONTROL, &d, 1);

      if (first || low_current) {
         if (adr_mux == 0xFFFF) {
            eprintf("\007Please connect multimeter to channel %d and press ENTER\n", adr);
            fgets(str, sizeof(str), stdin);
         } else {
            d = adr % 10;
            status = mscb_write(fd, adr_mux, 0, &d, 1);
            if (status != MSCB_SUCCESS) {
               eprintf("Error %d setting multiplexer at address %d.\n", status, adr_mux);
               stop();
            }
            eprintf("\nMultiplexer set to channel %d\n\n", adr);
         }
      }

      /*--- measure U calibration ----*/

      if (stricmp(info.node_name, "HVR-800") == 0) {
         u_low  = 100;       
         u_high = 500;
      } else {
         u_low  = 100;
         u_high = 900;
      }

      /* set demand to low value */
      eprintf("Setting       %5.1lf Volt\n", u_low);
      mscb_write(fd, adr, CH_VDEMAND, &u_low, sizeof(float));
      Sleep(HV_SET_DELAY);

      v_multi1 = read_voltage(fd, adr_dvm);
      if (adr_dvm > 0)
         eprintf("DVM reads     %5.1lf Volt\n", v_multi1);

      if (v_multi1 < u_low-50) {
         eprintf("HVR ADC voltage too low, aborting.\n");
         stop();
      }

      /* read ADC */
      size = sizeof(float);
      mscb_read(fd, adr, CH_VMEAS, &v_adc1, &size);
      eprintf("HVR ADC reads %5.1lf Volt\n", v_adc1);

      if (v_adc1 < u_low-50) {
         eprintf("HVR ADC voltage too low, aborting.\n");
         stop();
      }

      eprintf("\nSetting       %5.1lf Volt\n", u_high);
      do {
         /* set demand voltage */
         mscb_write(fd, adr, CH_VDEMAND, &u_high, sizeof(float));

         /* wait voltage to settle */
         Sleep(HV_SET_DELAY);

         /* read voltage */
         size = sizeof(float);
         mscb_read(fd, adr, CH_VMEAS, &v_adc2, &size);

         if (v_adc2 < u_high-10) {
            eprintf("Only %1.1lf V can be reached on output,\n", v_adc2);
            eprintf("please increase input voltage and press ENTER.\n");
            fgets(str, sizeof(str), stdin);
         }

      } while (v_adc2 < u_high-10);

      v_multi2 = read_voltage(fd, adr_dvm);
      if (adr_dvm > 0)
         eprintf("DVM reads     %5.1lf Volt\n", v_multi2);

      /* read ADC */
      size = sizeof(float);
      mscb_read(fd, adr, CH_VMEAS, &v_adc2, &size);
      eprintf("HVR ADC reads %5.1lf Volt\n", v_adc2);

      if (v_adc2 < u_high-10) {
         eprintf("HVR ADC voltage too low, aborting.\n");
         stop();
      }

      /* calculate corrections */
      dac_gain = (float) ((u_high - u_low) / (v_multi2 - v_multi1));
      dac_ofs = (float) (u_low - dac_gain * v_multi1);

      adc_gain = (float) ((v_multi2 - v_multi1) / (v_adc2 - v_adc1));
      adc_ofs = (float) (v_multi1 - adc_gain * v_adc1);

      eprintf("\nCalculated calibration constants:\n");
      eprintf("DAC gain   : %10.5lf\n", dac_gain);
      eprintf("DAC offset : %10.5lf\n", dac_ofs);
      eprintf("ADC gain   : %10.5lf\n", adc_gain);
      eprintf("ADC offset : %10.5lf\n", adc_ofs);

      if (fabs(dac_gain) > 3 || fabs(adc_gain) > 3) {
         eprintf("\nERROR: The DAC offset is too high,\nprobably voltage measurement is not working\n");
         eprintf("Trying again ....\n");
         continue;
      }

      if (fabs(dac_ofs) > 50) {
         eprintf("\nERROR: The ADC offset is too high,\nprobably voltage measurement is not working\n");
         eprintf("Aborting calibration.\n");
         continue;
      }

      mscb_write(fd, adr, CH_ADCGAIN, &adc_gain, sizeof(float));
      mscb_write(fd, adr, CH_ADCOFS, &adc_ofs, sizeof(float));
      mscb_write(fd, adr, CH_DACGAIN, &dac_gain, sizeof(float));
      mscb_write(fd, adr, CH_DACOFS, &dac_ofs, sizeof(float));

      /* now test u_high/2 */

      eprintf("\nSetting       %5.1lf Volt\n", u_high/2);
      f1 = u_high/2;
      mscb_write(fd, adr, CH_VDEMAND, &f1, sizeof(float));

      /* wait voltage to settle */
      Sleep(HV_SET_DELAY);

      /* read voltage */
      size = sizeof(float);
      mscb_read(fd, adr, CH_VMEAS, &v_adc1, &size);
      eprintf("HVR ADC reads %5.1lf Volt\n", v_adc1);

      v_multi1 = read_voltage(fd, adr_dvm);
      if (adr_dvm > 0)
         eprintf("DVM reads     %5.1lf Volt\n", v_multi1);

      if (fabs(v_adc1 - u_high/2) > 1)
         eprintf("\nERROR: ADC does not read 500V, re-doing calibration\n");

      if (fabs(v_multi1 - u_high/2) > 1)
         eprintf("\nERROR: DVM does not read 500V, re-doing calibration\n");

   } while (fabs(v_adc1 - u_high/2) > 1 || fabs(v_multi1 - u_high/2) > 1);

   if (low_current) {
      /* set demand to 0V */
      f1 = 0;
      mscb_write(fd, adr, CH_VDEMAND, &f1, sizeof(float));
      eprintf("\007Please connect 100 MOhm resistor to channel %d and press ENTER\n", adr);
      fgets(str, sizeof(str), stdin);
      rin_dvm = 100E6;
   }

   /* set voltage exactly with regulation */
   eprintf("\nSetting       %5.1lf Volt\n", u_high);
   d = 3;
   mscb_write(fd, adr, CH_CONTROL, &d, 1);
   mscb_write(fd, adr, CH_VDEMAND, &u_high, sizeof(float));

   /* wait voltage to settle */
   Sleep(HV_SET_DELAY);

   /* read current */
   size = sizeof(float);
   mscb_read(fd, adr, CH_IMEAS, &i_high, &size);
   eprintf("\nI at %3.0lfV  :  %5.3lf uA\n", u_high, i_high);

   /* remove voltage */
   f1 = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f1, sizeof(float));

   /* set current limit to 2.5mA */
   f1 = 2500;
   mscb_write(fd, adr, CH_ILIMIT, &f1, sizeof(float));

   if (!low_current) {
      if (adr_mux == 0xFFFF) {
         if (next_adr > 0) {
            f1 = 0;
            mscb_write(fd, next_adr, CH_VDEMAND, &f1, sizeof(float));
            eprintf("\n\007Please connect multimeter to channel %d and press ENTER\n", next_adr);
         } else
            eprintf("\n\007Please disconnect multimeter from channel %d and press ENTER\n", adr);
         fgets(str, sizeof(str), stdin);
      } else {
         if (next_adr > 0)
            d = next_adr % 10;
         else
            d = 10;
         status = mscb_write(fd, adr_mux, 0, &d, 1);
         if (status != MSCB_SUCCESS) {
            eprintf("Error %d setting multiplexer at address %d.\n", status, adr_mux);
            stop();
         }
         if (next_adr > 0)
            eprintf("\nMultiplexer set to channel %d\n\n", next_adr);
         else
            eprintf("\nMultiplexer set to disconnected\n\n");
      }
   }

   if (low_current) {

      /* remove voltage */
      u_low = 0;
      mscb_write(fd, adr, CH_VDEMAND, &u_low, sizeof(float));
      Sleep(HV_SET_DELAY);
      
      eprintf("\n\007Please disconnect resistor from channel %d and press ENTER\n", adr);
      fgets(str, sizeof(str), stdin);

      /* read current */
      size = sizeof(float);
      mscb_read(fd, adr, CH_IMEAS, &i_low, &size);
      eprintf("I at %1.0lfV  :  %5.3lf uA\n", u_low, i_low);

      i_vgain = 0;

      /* calculate current gain */
      i_gain = (float) ((u_high - u_low)/rin_dvm*1E6) / (i_high - i_low);

      /* calcualte offset */
      i_ofs = (float) ((i_high - u_high/(rin_dvm * i_gain)*1E6));

      eprintf("\nI vgain    : %10.5lf\n", i_vgain);
      eprintf("I offset   : %10.5lf\n", i_ofs);
      eprintf("I gain     : %10.5lf\n", i_gain);
      mscb_write(fd, adr, CH_CURVGAIN, &i_vgain, sizeof(float));
      mscb_write(fd, adr, CH_CUROFS, &i_ofs, sizeof(float));
      mscb_write(fd, adr, CH_CURGAIN, &i_gain, sizeof(float));

   } else {

      /* set CSR to HV on, no regulation */
      d = 1;
      mscb_write(fd, adr, CH_CONTROL, &d, 1);

      /*---- measure I-V gain ----*/

      /* set low demand */
      mscb_write(fd, adr, CH_VDEMAND, &u_low, sizeof(float));
      Sleep(HV_SET_DELAY);

      /* read current */
      size = sizeof(float);
      mscb_read(fd, adr, CH_IMEAS, &i1, &size);
      eprintf("I at %1.0lfV  :  %5.1lf uA\n", u_low, i1);

      do {
         /* set demand to high value */
         mscb_write(fd, adr, CH_VDEMAND, &u_high, sizeof(float));
         Sleep(HV_SET_DELAY);

         /* read voltage */
         size = sizeof(float);
         mscb_read(fd, adr, CH_VMEAS, &v_adc1, &size);

         if (v_adc1 < u_high-10) {
            v_adc2 = u_high - v_adc1 + u_low;
            v_adc2 = (float) ((int) (v_adc2 / u_low) * 100);

            eprintf("Only %1.1lf V can be reached on output,\n", v_adc1);
            eprintf("please increase input voltage by %1.0lf V and press ENTER.\n", v_adc2);
            fgets(str, sizeof(str), stdin);
         }

      } while (v_adc1 < u_high-10);

      /* read current */
      size = sizeof(float);
      mscb_read(fd, adr, CH_IMEAS, &i2, &size);
      eprintf("I at %1.0lfV  :  %5.1lf uA\n", u_high, i2);

      if (i_high - i2 < 10) {
         eprintf("\nERROR: The current variation is too small, probably current measurement is not working\n");
         eprintf("Aborting calibration.\n");
         stop();
      }

      /* calculate voltage-current correction */
      i_vgain = (float) (i2 - i1) / (u_high - u_low);

      /* calcualte offset */
      i_ofs = (float) ((i2 - i1 * u_high/u_low) / (1 - u_high/u_low));

      /* calculate current gain */
      i_gain = (float) (u_high/rin_dvm*1E6) / (i_high - i2);

      eprintf("\nI vgain    : %10.5lf\n", i_vgain);
      eprintf("I offset   : %10.5lf\n", i_ofs);
      eprintf("I gain     : %10.5lf\n", i_gain);
      mscb_write(fd, adr, CH_CURVGAIN, &i_vgain, sizeof(float));
      mscb_write(fd, adr, CH_CUROFS, &i_ofs, sizeof(float));
      mscb_write(fd, adr, CH_CURGAIN, &i_gain, sizeof(float));

      if (i_ofs < 0) {
         eprintf("\nWARNING: The current offset is negative, which means that a current\n");
         eprintf("below %1.0lfuA cannot be measured. A compensation resistor to Vref is missing.\n\n", i_ofs);
      }
   }

   /* remove voltage */
   f1 = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f1, sizeof(float));

   /* write constants to EEPROM */
   mscb_flash(fd, adr, -1, 0);
   Sleep(200);

   return 1;
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   int i, fd, status;
   unsigned short adr, adr_start, adr_end, adr_dvm, adr_mux;
   char str[80], device[80], password[80];
   MSCB_INFO_VAR info;

   device[0] = password[0] = 0;

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         else if (argv[i][1] == 'd')
            strcpy(device, argv[++i]);
         else if (argv[i][1] == 'p')
            strcpy(password, argv[++i]);
      } else {
         usage:
         printf
               ("usage: calib_hvr [-d host:device] [-p password]]\n\n");
         eprintf("       -d     device, usually usb0 for first USB port,\n");
         eprintf("              or \"<host>:usb0\" for RPC connection\n");
         eprintf("              or \"mscb<xxx>\" for Ethernet connection to SUBM_260\n");
         eprintf("       -p     optional password for SUBM_260\n");
         return 0;
      }
   }


   /* open port */
   fd = mscb_init(device, sizeof(str), password, 0);
   if (fd < 0) {
      if (device[0])
         eprintf("Cannot connect to device \"%s\".\n", device);
      else
         eprintf("No MSCB submaster found.\n");

      stop();
   }

   /* delete log file */
   unlink("calib_hvr.log");

   printf("Enter address of first HVR-200/400/500/800 channel    [  0] : ");
   fgets(str, sizeof(str), stdin);
   adr_start = (unsigned short)atoi(str);

   printf("Enter address of last HVR-200/400/500/800 channel     [%3d] : ", adr_start + 9);
   fgets(str, sizeof(str), stdin);
   if (!isdigit(str[0]))
      adr_end = adr_start + 9;
   else
      adr_end = (unsigned short)atoi(str);

   printf("Enter address of Keithley DVM, or 'manual' for none   [100] : ");
   fgets(str, sizeof(str), stdin);
   if (str[0] == 'm')
      adr_dvm = 0xFFFF;
   else if (isdigit(str[0]))
      adr_dvm = (unsigned short)atoi(str);
   else
      adr_dvm = 100;

   printf("Enter address of HV Multiplexer, or 'manual' for none [200] : ");
   fgets(str, sizeof(str), stdin);
   if (str[0] == 'm')
      adr_mux = 0xFFFF;
   else if (isdigit(str[0]))
      adr_mux = (unsigned short)atoi(str);
   else
      adr_mux = 200;

   /* check if module at 0xFF00 */
   if (adr_start != 0xFF00) {
      if (mscb_ping(fd, 0xFF00, 0) == MSCB_SUCCESS) {
         status = mscb_set_node_addr(fd, 0xFF00, 0, 0, 0);
         if (status == MSCB_SUCCESS)
            eprintf("\nHVR found at address 0xFF00, moved to 0\n");
         else {
            eprintf("\nError %d in setting node address\n", status);
            stop();
         }
      }
   }

   /* check if module at 0xFF05 */
   if (adr_start != 0xFF05) {
      if (mscb_ping(fd, 0xFF05, 0) == MSCB_SUCCESS) {
         status = mscb_set_node_addr(fd, 0xFF05, 0, 0, 5);
         if (status == MSCB_SUCCESS)
            eprintf("HVR found at address 0xFF05, moved to 5\n");
         else {
            eprintf("Error %d in setting node address\n", status);
            stop();
         }
      }
   }

   /* check if all channels alive and of right type */
   for (adr = adr_start; adr <= adr_end; adr++) {

      if (mscb_ping(fd, adr, 0) != MSCB_SUCCESS) {
         eprintf("No device at address %d found.\n", adr);
         stop();
      }

      mscb_info_variable(fd, adr, CH_CUROFS, &info);
      memset(str, 0, sizeof(str));
      memcpy(str, info.name, 8);
      if (strcmp(str, "CURofs") != 0) {
         eprintf("Incorrect software version HVR-200/400/500. Expect \"CURofs\" on var #%d.\n", CH_CUROFS);
         stop();
      }
   }

   for (adr = adr_start; adr <= adr_end; adr++) {
      if (!quick_check(fd, adr, adr_dvm, adr_mux))
         break;
   }

   for (adr = adr_start; adr <= adr_end; adr++) {
      if (!calib(fd, adr, adr_dvm, adr_mux, 10E6, adr == adr_start, adr<adr_end ? adr+1 : -1))
         break;
   }

   if (adr > adr_end)
      eprintf("\007\nCalibration finished.\n\n");

   mscb_exit(fd);

   stop();
}

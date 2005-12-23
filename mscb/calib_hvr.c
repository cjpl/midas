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
#define CH_VLIMIT       8
#define CH_ILIMIT       9
#define CH_TRIPMAX     10
#define CH_ADCGAIN     11
#define CH_ADCOFS      12
#define CH_DACGAIN     13
#define CH_DACOFS      14
#define CH_CURVGAIN    15
#define CH_CURGAIN     16
#define CH_CUROFS      17

#define HV_SET_DELAY   5000 /* five seconds for voltage to stabilize */

/*------------------------------------------------------------------*/

void stop()
{
   char str[80];

   printf("--- hit any key to exit ---");
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
   if (adr > 0) {
      size = sizeof(float);
      status = mscb_read(fd, adr, 1, &value, &size);
      if (status != MSCB_SUCCESS)
         printf("Error reading DVM (status=%d)\n", status);
      value = (float) fabs(value);
   }

   if (status != MSCB_SUCCESS) {
      printf("Please enter absolute voltage from DVM: ");
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
   mscb_write(fd, adr, CH_RAMPUP, &d, sizeof(short));
   mscb_write(fd, adr, CH_RAMPDOWN, &d, sizeof(short));

   f = 1;
   mscb_write(fd, adr, CH_ADCGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_DACGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_CURGAIN, &f, sizeof(float));
}

/*------------------------------------------------------------------*/

int calib(int fd, unsigned short adr, unsigned short adr_dvm, float rin_dvm, int first, int next_adr)
{
   float f;
   int size, d;
   char str[80];
   float v_adc1, v_adc2, v_multi1, v_multi2, adc_gain, adc_ofs, 
      dac_gain, dac_ofs, i1, i2, i_900, i_gain, i_vgain, i_ofs;

   printf("\n**** Calibrating channel %d ****\n\n", adr);

   /*--- initialization ----*/

   /* init variables */
   f = 0;
   d = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   mscb_write(fd, adr, CH_ADCOFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_DACOFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_CUROFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_CURVGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_RAMPUP, &d, sizeof(short));
   mscb_write(fd, adr, CH_RAMPDOWN, &d, sizeof(short));

   f = 1;
   mscb_write(fd, adr, CH_ADCGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_DACGAIN, &f, sizeof(float));
   mscb_write(fd, adr, CH_CURGAIN, &f, sizeof(float));

   /* disable hardware current limit */
   f = 9999;
   mscb_write(fd, adr, CH_ILIMIT, &f, sizeof(float));

   /* set CSR to HV on, no regulation */
   d = 1;
   mscb_write(fd, adr, CH_CONTROL, &d, 1);

   if (first) {
      printf("\007Please connect multimeter to channel %d and press ENTER\n", adr);
      fgets(str, sizeof(str), stdin);
   }

   /*--- measure U calibration ----*/

   /* set demand to 100V */
   printf("Setting       100.0 Volt\n");
   f = 100;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   Sleep(HV_SET_DELAY);

   v_multi1 = read_voltage(fd, adr_dvm);
   if (adr_dvm > 0)
      printf("DVM reads     %1.1lf Volt\n", v_multi1);

   /* read ADC */
   size = sizeof(float);
   mscb_read(fd, adr, CH_VMEAS, &v_adc1, &size);
   printf("HVR ADC reads %1.1lf Volt\n", v_adc1);

   if (v_adc1 < 50) {
      printf("HVR ADC voltage too low, aborting.\n");
      stop();
   }

   printf("\nSetting       900.0 Volt\n");
   do {
      /* set demand to 900V */
      f = 900;
      mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));

      /* wait voltage to settle */
      Sleep(HV_SET_DELAY);

      /* read voltage */
      size = sizeof(float);
      mscb_read(fd, adr, CH_VMEAS, &v_adc2, &size);

      if (v_adc2 < 890) {
         printf("Only %1.1lf V can be reached on output,\n", v_adc2);
         printf("please increase input voltage and press ENTER.\n");
         fgets(str, sizeof(str), stdin);
      }

   } while (v_adc2 < 890);

   v_multi2 = read_voltage(fd, adr_dvm);
   if (adr_dvm > 0)
      printf("DVM reads     %1.1lf Volt\n", v_multi2);

   /* read ADC */
   size = sizeof(float);
   mscb_read(fd, adr, CH_VMEAS, &v_adc2, &size);
   printf("HVR ADC reads %1.1lf Volt\n", v_adc2);

   if (v_adc2 < 500) {
      printf("HVR ADC voltage too low, aborting.\n");
      stop();
   }

   /* calculate corrections */
   dac_gain = (float) ((900.0 - 100.0) / (v_multi2 - v_multi1));
   dac_ofs = (float) (100 - dac_gain * v_multi1);

   adc_gain = (float) ((v_multi2 - v_multi1) / (v_adc2 - v_adc1));
   adc_ofs = (float) (v_multi1 - adc_gain * v_adc1);

   printf("\nCalculated calibration constants:\n");
   printf("DAC gain   : %10.5lf\n", dac_gain);
   printf("DAC offset : %10.5lf\n", dac_ofs);
   printf("ADC gain   : %10.5lf\n", adc_gain);
   printf("ADC offset : %10.5lf\n", adc_ofs);

   if (fabs(dac_gain) > 3 || fabs(adc_gain) > 3) {
      printf("\nERROR: The voltage gain is too high,\nprobably voltage measurement is not working\n");
      printf("Aborting calibration.\n");
      stop();
   }

   mscb_write(fd, adr, CH_ADCGAIN, &adc_gain, sizeof(float));
   mscb_write(fd, adr, CH_ADCOFS, &adc_ofs, sizeof(float));
   mscb_write(fd, adr, CH_DACGAIN, &dac_gain, sizeof(float));
   mscb_write(fd, adr, CH_DACOFS, &dac_ofs, sizeof(float));

   /* set voltage to exactly 900 V with regulation */
   d = 3;
   mscb_write(fd, adr, CH_CONTROL, &d, 1);
   f = 900;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));

   /* wait voltage to settle */
   Sleep(HV_SET_DELAY);

   /* read current */
   size = sizeof(float);
   mscb_read(fd, adr, CH_IMEAS, &i_900, &size);
   printf("\nCurrent at 900V: %1.1lf uA\n", i_900);

   /* remove voltage */
   f = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));

   /* set current limit to 2.5mA */
   f = 2500;
   mscb_write(fd, adr, CH_ILIMIT, &f, sizeof(float));

   if (next_adr > 0) {
      f = 0;
      mscb_write(fd, next_adr, CH_VDEMAND, &f, sizeof(float));
      printf("\n\007Please connect multimeter to channel %d and press ENTER\n", next_adr);
   } else
      printf("\n\007Please disconnect multimeter from channel %d and press ENTER\n", adr);
   fgets(str, sizeof(str), stdin);

   /* set CSR to HV on, no regulation */
   d = 1;
   mscb_write(fd, adr, CH_CONTROL, &d, 1);

   /*---- measure I-V gain ----*/

   /* set demand to 100V */
   f = 100;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   Sleep(HV_SET_DELAY);

   /* read current */
   size = sizeof(float);
   mscb_read(fd, adr, CH_IMEAS, &i1, &size);
   printf("Current at 100V: %1.1lf uA\n", i1);

   do {
      /* set demand to 900V */
      f = 900;
      mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
      Sleep(HV_SET_DELAY);

      /* read voltage */
      size = sizeof(float);
      mscb_read(fd, adr, CH_VMEAS, &v_adc1, &size);

      if (v_adc1 < 890) {
         v_adc2 = 900 - v_adc1 + 100;
         v_adc2 = (float) ((int) (v_adc2 / 100.0) * 100);

         printf("Only %1.1lf V can be reached on output,\n", v_adc1);
         printf("please increase input voltage by %1.0lf V and press ENTER.\n", v_adc2);
         fgets(str, sizeof(str), stdin);
      }

   } while (v_adc1 < 890);

   /* read current */
   size = sizeof(float);
   mscb_read(fd, adr, CH_IMEAS, &i2, &size);
   printf("Current at 900V: %1.1lf uA\n", i2);

   if (i_900 - i2 < 10) {
      printf("\nERROR: The current variation is too small, probably current measurement is not working\n");
      printf("Aborting calibration.\n");
      stop();
   }

   /* calculate voltage-current correction */
   i_vgain = (float) (i2 - i1) / (900 - 100);

   /* calcualte offset */
   i_ofs = (float) ((i2 - i1 * 900.0/100.0) / (1 - 900.0/100.0));

   /* calculate current gain */
   i_gain = (float) (900/rin_dvm*1E6) / (i_900 - i2);

   printf("\nI vgain    :  %10.5lf\n", i_vgain);
   printf("I offset   :  %10.5lf\n", i_ofs);
   printf("I gain     :  %10.5lf\n", i_gain);
   mscb_write(fd, adr, CH_CURVGAIN, &i_vgain, sizeof(float));
   mscb_write(fd, adr, CH_CUROFS, &i_ofs, sizeof(float));
   mscb_write(fd, adr, CH_CURGAIN, &i_gain, sizeof(float));

   if (i_ofs < 0) {
      printf("\nWARNING: The current offset is negative, which means that a current\n");
      printf("below %1.0lfuA cannot be measured. A compensation resistor to Vref is missing.\n\n", i_ofs);
   }

   /* remove voltage */
   f = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));

   /* write constants to EEPROM */
   mscb_flash(fd, adr, -1, 0);
   Sleep(200);

   return 1;
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   int i, fd;
   unsigned short adr, adr_start, adr_end, adr_dvm;
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
         printf("       -d     device, usually usb0 for first USB port,\n");
         printf("              or \"<host>:usb0\" for RPC connection\n");
         printf
               ("              or \"mscb<xxx>\" for Ethernet connection to SUBM_260\n");
         printf("       -p     optional password for SUBM_260\n");
         return 0;
      }
   }


   /* open port */
   fd = mscb_init(device, sizeof(str), password, 0);
   if (fd < 0) {
      if (device[0])
         printf("Cannot connect to device \"%s\".\n", device);
      else
         printf("No MSCB submaster found.\n");

      stop();
   }

   printf("Enter address of first HVR-400/500 channel          [  0] : ");
   fgets(str, sizeof(str), stdin);
   adr_start = (unsigned short)atoi(str);

   printf("Enter address of last HVR-400/500 channel           [%3d] : ", adr_start + 9);
   fgets(str, sizeof(str), stdin);
   if (!isdigit(str[0]))
      adr_end = adr_start + 9;
   else
      adr_end = (unsigned short)atoi(str);

   printf("Enter address of Keithley DVM, or 'manual' for none [100] : ");
   fgets(str, sizeof(str), stdin);
   if (str[0] == 'm')
      adr_dvm = 0xFFFF;
   else if (isdigit(str[0]))
      adr_dvm = (unsigned short)atoi(str);
   else
      adr_dvm = 100;

   /* check if all channels alive and of right type */
   for (adr = adr_start; adr <= adr_end; adr++) {

      if (mscb_ping(fd, adr) != MSCB_SUCCESS) {
         printf("No device at address %d found.\n", adr);
         stop();
      }

      mscb_info_variable(fd, adr, CH_CUROFS, &info);
      memset(str, 0, sizeof(str));
      memcpy(str, info.name, 8);
      if (strcmp(str, "CURofs") != 0) {
         printf("Incorrect software version HVR-400/500. Expect \"CURofs\" on var #%d.\n", CH_CUROFS);
         stop();
      }
   }

   /*
   printf("\nPlease disconnect everything from outputs,\n");
   printf("connect 1300V to input and press ENTER");
   fgets(str, sizeof(str), stdin);
   printf("\n");
   */

   for (adr = adr_start; adr <= adr_end; adr++) {

      if (!calib(fd, adr, adr_dvm, 10E6, adr == adr_start, adr<adr_end ? adr+1 : -1))
         break;

   }

   if (adr > adr_end)
      printf("\nCalibration finished.\n\n");

   mscb_exit(fd);

   stop();
}

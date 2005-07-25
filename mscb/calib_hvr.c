/********************************************************************\

  Name:         calib_hvr.c
  Created by:   Stefan Ritt

  Contents:     Calibration program for HVR-200

  $Log$
  Revision 1.14  2005/07/25 10:11:20  ritt
  Fixed compiler warnings

  Revision 1.13  2005/05/02 10:50:12  ritt
  Version 2.1.1

  Revision 1.12  2005/03/21 10:57:25  ritt
  Version 2.0.0

  Revision 1.11  2005/02/22 13:21:35  ritt
  Calibration with hvr_500

  Revision 1.10  2004/12/22 16:03:24  midas
  Implemented VGain calibration

  Revision 1.9  2004/07/29 15:47:38  midas
  Fixed wrong warning with current offset

  Revision 1.8  2004/05/14 15:09:46  midas
  Increased wait time

  Revision 1.7  2004/03/10 13:28:25  midas
  mscb_init returns device name

  Revision 1.6  2004/03/04 15:28:57  midas
  Changed HVR-500 to HVR-200

  Revision 1.5  2004/01/07 12:56:15  midas
  Chaned line length

  Revision 1.4  2004/01/07 12:52:23  midas
  Changed indentation

  Revision 1.3  2003/11/13 12:26:24  midas
  Remove HV before plugging in 1MOhm resistor

  Revision 1.2  2003/09/23 09:24:07  midas
  Added current calibration

  Revision 1.1  2003/09/12 10:07:00  midas
  Initial revision

\********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
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
   }

   if (status != MSCB_SUCCESS) {
      printf("Please enter voltage from DVM: ");
      fgets(str, sizeof(str), stdin);
      value = (float) atof(str);
   }

   return value;
}

/*------------------------------------------------------------------*/

int calib_ui(int fd, unsigned short adr, unsigned short adr_dvm)
{
   float f;
   int size, d;
   char str[80];
   float v_adc1, v_adc2, i1, i2, i_vgain;

   if (adr_dvm);

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

   /*---- measure I-V gain ----*/

   f = 0;
   mscb_write(fd, adr, CH_CURVGAIN, &f, sizeof(float));
      
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

   /* calculate voltage-current correction */
   i_vgain = (float) (i1 - i2) / (900 - 100);

   printf("I vgain        :  %1.5lf\n", i_vgain);
   mscb_write(fd, adr, CH_CURVGAIN, &i_vgain, sizeof(float));

   /* remove voltage */
   f = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));

   return 1;
}

/*------------------------------------------------------------------*/

int calib(int fd, unsigned short adr, unsigned short adr_dvm, float rin_dvm)
{
   float f;
   int size, d;
   char str[80];
   float v_adc1, v_adc2, v_multi1, v_multi2, adc_gain, adc_ofs, 
      dac_gain, dac_ofs, i1, i2, i_gain, i_ofs;

   printf("\n**** Calibrating channel %d ****\n\n", adr);

   printf("\007Please connect multimeter to channel %d and press ENTER\n", adr);
   fgets(str, sizeof(str), stdin);

   /*--- initialization ----*/

   /* init variables */
   f = 0;
   d = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   mscb_write(fd, adr, CH_ADCOFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_DACOFS, &f, sizeof(float));
   mscb_write(fd, adr, CH_CUROFS, &f, sizeof(float));
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

   /*--- measure U and I calibrations ----*/

   /* set demand to 100V */
   f = 100;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   Sleep(HV_SET_DELAY);

   v_multi1 = read_voltage(fd, adr_dvm);
   if (adr_dvm > 0)
      printf("DVM reads %1.1lf Volt\n", v_multi1);

   /* read ADC */
   size = sizeof(float);
   mscb_read(fd, adr, CH_VMEAS, &v_adc1, &size);
   printf("HVR ADC reads %1.1lf Volt\n", v_adc1);

   /* read current */
   size = sizeof(float);
   mscb_read(fd, adr, CH_IMEAS, &i1, &size);
   printf("Current at 100V: %1.1lf uA\n", i1);

   if (v_adc1 < 50) {
      printf("HVR ADC voltage too low, aborting.\n");
      return 0;
   }

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
      printf("DVM reads %1.1lf Volt\n", v_multi2);

   /* read ADC */
   size = sizeof(float);
   mscb_read(fd, adr, CH_VMEAS, &v_adc2, &size);
   printf("HVR ADC reads %1.1lf\n", v_adc2);

   if (v_adc2 < 500) {
      printf("HVR ADC voltage too low, aborting.\n");
      return 0;
   }

   /* read current */
   size = sizeof(float);
   mscb_read(fd, adr, CH_IMEAS, &i2, &size);
   printf("Current at 900V: %1.1lf uA\n", i2);

   /* calculate corrections */
   dac_gain = (float) ((900.0 - 100.0) / (v_multi2 - v_multi1));
   dac_ofs = (float) (100 - dac_gain * v_multi1);

   adc_gain = (float) ((v_multi2 - v_multi1) / (v_adc2 - v_adc1));
   adc_ofs = (float) (v_multi1 - adc_gain * v_adc1);

   i_gain = (float) ((900 - 100)/rin_dvm*1E6 / (i2 - i1));
   i_ofs = (float) (100/rin_dvm*1E6 - i_gain * i1);

   printf("\n\nCalculated calibration constants:\n");
   printf("DAC gain:   %1.5lf\n", dac_gain);
   printf("DAC offset: %1.5lf\n", dac_ofs);
   printf("ADC gain:   %1.5lf\n", adc_gain);
   printf("ADC offset: %1.5lf\n", adc_ofs);
   printf("I gain:     %1.5lf\n", i_gain);
   printf("I offset:   %1.5lf\n", i_ofs);

   if (i_ofs > 0) {
      printf("\nWARNING: The current offset is positive, which means that a current\n");
      printf("below %1.0lfuA cannot be measured. A compensation resistor to Vref is missing.\n\n", i_ofs);
   }

   /*
   printf("\nWrite values to flash? ([y]/n) ");
   fgets(str, sizeof(str), stdin);
   if (str[0] == 'n') {
      printf("Calibration aborted.\n");
      return 0;
   }
   */

   mscb_write(fd, adr, CH_ADCGAIN, &adc_gain, sizeof(float));
   mscb_write(fd, adr, CH_ADCOFS, &adc_ofs, sizeof(float));
   mscb_write(fd, adr, CH_DACGAIN, &dac_gain, sizeof(float));
   mscb_write(fd, adr, CH_DACOFS, &dac_ofs, sizeof(float));
   mscb_write(fd, adr, CH_CURGAIN, &i_gain, sizeof(float));
   mscb_write(fd, adr, CH_CUROFS, &i_ofs, sizeof(float));

   /* remove voltage */
   f = 0;
   mscb_write(fd, adr, CH_VDEMAND, &f, sizeof(float));
   d = 2;
   mscb_write(fd, adr, CH_CONTROL, &d, 1);

   /* set current limit to 2.5mA */
   f = 2500;
   mscb_write(fd, adr, CH_ILIMIT, &f, sizeof(float));

   /* write constants to EEPROM */
   mscb_flash(fd, adr);

   return 1;
}

/*------------------------------------------------------------------*/

void stop()
{
   char str[80];

   printf("--- hit any key to exit ---");
   fgets(str, sizeof(str), stdin);

   exit(0);
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   int fd;
   unsigned short adr, adr_start, adr_end, adr_dvm;
   char str[80];
   MSCB_INFO_VAR info;

   /* open port */
   *str = 0;
   fd = mscb_init(str, sizeof(str), "", 0);
   if (fd < 0) {
      if (str[0])
         printf("Cannot connect to device \"%s\".\n", str);
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
         printf("Incorrect software versionon HVR-400/500. Expect \"CURofs\" on var #%d.\n", CH_CUROFS);
         stop();
      }
   }

   printf("\nPlease disconnect everything from outputs,\n");
   printf("connect 1300V to input and press ENTER");
   fgets(str, sizeof(str), stdin);
   printf("\n");

   for (adr = adr_start; adr <= adr_end; adr++) {
      if (!calib_ui(fd, adr, adr_dvm))
         break;
   }

   for (adr = adr_start; adr <= adr_end; adr++) {
      if (!calib(fd, adr, adr_dvm, 10E6))
         break;
   }

   if (adr > adr_end)
      printf("\nCalibration finished.\n");

   mscb_exit(fd);

   stop();
}

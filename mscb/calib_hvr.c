/********************************************************************\

  Name:         calib_hvr.c
  Created by:   Stefan Ritt

  Contents:     Calibration program for HVR-500

  $Log$
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

int main(int argc, char *argv[])
{
float f;
int   fd, adr, size, d;
char  str[80];
float v_adc1, v_adc2, v_multi1, v_multi2, 
      adc_gain, adc_ofs, dac_gain, dac_ofs, i1, i2, i_gain, i_ofs;
MSCB_INFO_VAR info;

  /* open port */
  fd = mscb_init(DEF_DEVICE, 0);
  if (fd < 0)
    {
    if (fd == -2)
      printf("No MSCB submaster present at port \"%s\".\n", DEF_DEVICE);
    else
      printf("Cannot connect to device \"%s\".\n", DEF_DEVICE);

    return 0;
    }

  printf("Enter address of HVR-520 device: ");
  fgets(str, sizeof(str), stdin);
  adr = atoi(str);

  if (mscb_ping(fd, adr) != MSCB_SUCCESS)
    {
    printf("No device at address %d found.\n", adr);
    return 0;
    }

  mscb_info_variable(fd, adr, 17, &info);
  memset(str, 0, sizeof(str));
  memcpy(str, info.name, 8);
  if (strcmp(str, "VDAC") != 0)
    {
    printf("Incorrect software versionon SCS-520. Expect \"VDAC\" on var #17.\n");
    return 0;
    }

  printf("\nPlease connect multimeter with 1000V range to output,\n");
  printf("connect 1000V to input and press ENTER\n");
  fgets(str, sizeof(str), stdin);

  /* init variables */
  f = 0;
  mscb_write(fd, adr,  0, &f, sizeof(float)); /* Vdemand */
  mscb_write(fd, adr, 11, &f, sizeof(float)); /* ADCofs  */
  mscb_write(fd, adr, 13, &f, sizeof(float)); /* DACofs  */
  mscb_write(fd, adr, 15, &f, sizeof(float)); /* CURofs  */

  f = 1;
  mscb_write(fd, adr, 10, &f, sizeof(float)); /* ADCgain */
  mscb_write(fd, adr, 12, &f, sizeof(float)); /* DACgain */
  mscb_write(fd, adr, 14, &f, sizeof(float)); /* CURgain */

  /* set current limit to 3mA */
  f = 3000;
  mscb_write(fd, adr,  8, &f, sizeof(float)); /* Vdemand */

  /* set demand to 100V */
  f = 100;
  mscb_write(fd, adr, 1, &f, sizeof(float));
  
  /* set CSR to HV on, no regulation */
  d = 1;
  mscb_write(fd, adr, 0, &d, 1);

  printf("Please enter voltage from multimeter: ");
  fgets(str, sizeof(str), stdin);
  v_multi1 = (float)atof(str);

  /* read ADC */
  size = sizeof(float);
  mscb_read(fd, adr, 2, &v_adc1, &size);

  printf("HVR-500 ADC reads %1.1lf Volt\n", v_adc1);

  if (v_adc1 < 50)
    {
    printf("HVR-500 ADC voltage too low, aborting.\n");
    exit(0);
    }

  /* set demand to 900V */
  f = 900;
  mscb_write(fd, adr, 1, &f, sizeof(float));
  
  printf("Please enter voltage from multimeter: ");
  fgets(str, sizeof(str), stdin);
  v_multi2 = (float)atof(str);

  /* read ADC */
  size = sizeof(float);
  mscb_read(fd, adr, 2, &v_adc2, &size);

  printf("HVR-500 ADC reads %1.1lf\n", v_adc2);

  if (v_adc2 < 500)
    {
    printf("HVR-500 ADC voltage too low, aborting.\n");
    exit(0);
    }

  /* calculate corrections */
  dac_gain = (float)((900.0-100.0) / (v_multi2-v_multi1));
  dac_ofs  = (float)(100 - dac_gain*v_multi1);

  adc_gain = (float)((v_multi2-v_multi1) / (v_adc2-v_adc1));
  adc_ofs  = (float)(v_multi1 - adc_gain*v_adc1);

  printf("\n\nCalculated calibration constants:\n");
  printf("DAC gain:   %1.5lf\n", dac_gain);
  printf("DAC offset: %1.5lf\n", dac_ofs);
  printf("ADC gain:   %1.5lf\n", adc_gain);
  printf("ADC offset: %1.5lf\n", adc_ofs);

  printf("\nWrite values to flash? ([y]/n) ");
  fgets(str, sizeof(str), stdin);
  if (str[0] == 'n')
    {
    printf("Calibration aborted.\n");
    exit(0);
    }

  mscb_write(fd, adr, 10, &adc_gain, sizeof(float)); /* ADCgain */
  mscb_write(fd, adr, 11, &adc_ofs, sizeof(float));  /* ADCofs  */
  mscb_write(fd, adr, 12, &dac_gain, sizeof(float)); /* DACgain */
  mscb_write(fd, adr, 13, &dac_ofs, sizeof(float)); /* DACofs  */

  /* init variables */
  f = 0;
  mscb_write(fd, adr, 15, &f, sizeof(float)); /* CURofs  */
  f = 1;
  mscb_write(fd, adr, 14, &f, sizeof(float)); /* CURgain */

  printf("\nPlease connect 1MOhm resistor to output,\n");
  printf("connect 1300V to input and press ENTER\n");
  fgets(str, sizeof(str), stdin);

  /* set demand to 100V */
  f = 100;
  mscb_write(fd, adr, 1, &f, sizeof(float));

  /* wait voltage to settle */
  Sleep(3000);

  /* read current */
  size = sizeof(float);
  mscb_read(fd, adr, 3, &i1, &size);
  printf("Current at 100V: %1.1lf uA\n", i1);

  /* set demand to 900V */
  f = 900;
  mscb_write(fd, adr, 1, &f, sizeof(float));

  /* wait voltage to settle */
  Sleep(3000);

  do
    {
    /* read voltage */
    size = sizeof(float);
    mscb_read(fd, adr, 2, &v_adc1, &size);

    if (v_adc1 < 890)
      {
      v_adc2 = 900-v_adc1+100;
      v_adc2 = (float)((int)(v_adc2/100.0)*100);

      printf("Only %1.1lf V can be reached on output,\n", v_adc1);
      printf("please increase input voltage by %1.0lf V and press ENTER.\n", v_adc2);
      fgets(str, sizeof(str), stdin);
      }

    } while (v_adc1 < 890);

  /* read current */
  size = sizeof(float);
  mscb_read(fd, adr, 3, &i2, &size);
  printf("Current at 900V: %1.1lf uA\n", i2);

  /* calculate corrections */
  i_gain = (float)((900-100) / (i2-i1));
  i_ofs  = (float)(100 - i_gain*i1);

  printf("\n\nCalculated calibration constants:\n");
  printf("I gain:   %1.5lf\n", i_gain);
  printf("I offset: %1.5lf\n", i_ofs);

  printf("\nWrite values to flash? ([y]/n) ");
  fgets(str, sizeof(str), stdin);
  if (str[0] == 'n')
    {
    printf("Calibration aborted.\n");
    exit(0);
    }

  mscb_write(fd, adr, 14, &i_gain, sizeof(float));
  mscb_write(fd, adr, 15, &i_ofs, sizeof(float));

  /* remove voltage */
  f = 0;
  mscb_write(fd, adr, 1, &f, sizeof(float));
  d = 2;
  mscb_write(fd, adr, 0, &d, 1);

  /* write constants to EEPROM */
  mscb_flash(fd, adr);

  printf("\nCalibration finished.\n");

  mscb_exit(fd);

  return 0;
}

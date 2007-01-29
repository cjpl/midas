/********************************************************************\

  Name:         calib_1000.c
  Created by:   Stefan Ritt

  Contents:     Calibration program for SCS-900

  $Id$

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
   float v;
   double d;
   int i, fd, size;
   unsigned short adr;
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

      return 0;
   }

   printf("Enter address of SCS-1000/1001 device: ");
   fgets(str, sizeof(str), stdin);
   adr = (unsigned short)atoi(str);

   if (mscb_ping(fd, adr, 0) != MSCB_SUCCESS) {
      printf("No device at address %d found.\n", adr);
      return 0;
   }

   mscb_info_variable(fd, adr, 41, &info);
   memset(str, 0, sizeof(str));
   memcpy(str, info.name, 8);
   if (strcmp(str, "AGAIN7") != 0) {
      printf("Incorrect software versionon SCS-1000/1001. Expect \"AGAIN7\" on var #41.\n");
      return 0;
   }

   /*---- calibrate DAC0 ----*/

   /* reset old gain and offset */
   v = 0;
   mscb_write(fd, adr, 22, &v, sizeof(float));
   v = 1;
   mscb_write(fd, adr, 24, &v, sizeof(float));

   /* measure at zero */
   v = 0;
   mscb_write(fd, adr,  8, &v, sizeof(float));
   printf("Please connect multimeter to DAC0 and enter voltage: ");
   scanf("%lf", &d);

   /* write new offset */
   v = (float)-d;
   mscb_write(fd, adr, 22, &v, sizeof(float));

   /* measure at 9.9 V */
   v = 9.9f;
   mscb_write(fd, adr,  8, &v, sizeof(float));
   printf("Please enter voltage: ");
   scanf("%lf", &d);

   /* write new gain */
   v = (float) (9.9/d);
   mscb_write(fd, adr, 24, &v, sizeof(float));

   /*---- calibrate DAC1 ----*/

   /* reset old gain and offset */
   v = 0;
   mscb_write(fd, adr, 23, &v, sizeof(float));
   v = 1;
   mscb_write(fd, adr, 25, &v, sizeof(float));

   /* measure at zero */
   v = 0;
   mscb_write(fd, adr,  9, &v, sizeof(float));
   printf("Please connect multimeter to DAC1 and enter voltage: ");
   scanf("%lf", &d);

   /* write new offset */
   v = (float)-d;
   mscb_write(fd, adr, 23, &v, sizeof(float));

   /* measure at 9.9 V */
   v = 9.9f;
   mscb_write(fd, adr,  9, &v, sizeof(float));
   printf("Please enter voltage: ");
   scanf("%lf", &d);

   /* write new gain */
   v = (float) (9.9/d);
   mscb_write(fd, adr, 25, &v, sizeof(float));

   /*---- calibrate 8 DAC channels ----*/

   /* calibrate offset around zero */
   v = 0;
   mscb_write(fd, adr, 8, &v, sizeof(float));
   mscb_write(fd, adr, 9, &v, sizeof(float));

   for (i = 0; i < 8; i++) {
      /* reset old gain and offset */
      v = 0;
      mscb_write(fd, adr, (unsigned char) (i + 26), &v, sizeof(float));
      v = 1;
      mscb_write(fd, adr, (unsigned char) (i + 34), &v, sizeof(float));
   }

   /* let voltage settle */
   Sleep(3000);

   for (i = 0; i < 8; i++) {
      size = sizeof(float);
      mscb_read(fd, adr, (unsigned char) (i+14), &v, &size);

      /* write new offset */
      v = (float) v;
      mscb_write(fd, adr, (unsigned char) (i + 26), &v, sizeof(float));

      printf("AOFS%d: %f\n", i, v);
   }

   /* calibrate gain around 10V */
   v = 9.9f;
   mscb_write(fd, adr, 8, &v, sizeof(float));
   Sleep(3000);

   for (i = 0; i < 8; i++) {
      size = sizeof(float);
      mscb_read(fd, adr, (unsigned char) (i+14), &v, &size);

      if (v < 9 || v > 11) {
         printf("Invalid voltage on ADC%d: %lf, should be 9...11V, aborting.\n", i, v);
         exit(0);
      }
      /* write new gain */
      v = (float) (9.9/v);
      mscb_write(fd, adr, (unsigned char) (i + 34), &v, sizeof(float));

      printf("AGAIN%d: %f\n", i, v);
   }

   v = 0.f;
   mscb_write(fd, adr, 8, &v, sizeof(float));
   Sleep(100);

   /* write constants to EEPROM */
   mscb_flash(fd, adr, -1, 0);

   printf("\nCalibration finished.\n");

   mscb_exit(fd);

   return 0;
}

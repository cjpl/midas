/********************************************************************\

  Name:         calib_900.c
  Created by:   Stefan Ritt

  Contents:     Calibration program for SCS-900

  $Log$
  Revision 1.4  2005/03/21 10:57:25  ritt
  Version 2.0.0

  Revision 1.3  2005/02/07 14:34:20  ritt
  Added not about connection DAC to ADC

  Revision 1.2  2004/12/22 16:03:01  midas
  Fixed compiler warning

  Revision 1.1  2004/12/08 10:39:40  midas
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
   float v;
   double d;
   int i, j, fd, adr, size;
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

   printf("Enter address of SCS-900 device: ");
   fgets(str, sizeof(str), stdin);
   adr = atoi(str);

   if (mscb_ping(fd, adr) != MSCB_SUCCESS) {
      printf("No device at address %d found.\n", adr);
      return 0;
   }

   mscb_info_variable(fd, adr, 18, &info);
   memset(str, 0, sizeof(str));
   memcpy(str, info.name, 8);
   if (strcmp(str, "2.5V ADC") != 0) {
      printf("Incorrect software versionon SCS-900. Expect \"2.5V ADC \" on var #18.\n");
      return 0;
   }

   /* calibrate 8 DAC channels */
   for (i = 0; i < 8; i++) {
      
      /* reset old gain and offset */
      v = 0;
      mscb_write(fd, adr, (unsigned char) (i + 35), &v, sizeof(float));
      v = 1;
      mscb_write(fd, adr, (unsigned char) (i + 43), &v, sizeof(float));

      /* calibrate offset around zero */
      v = 0;
      size = sizeof(v);
      mscb_write(fd, adr, (unsigned char) (i + 8), &v, sizeof(float));

      printf("Please measure and enter DAC%d: ", i);
      scanf("%lf", &d);

      /* write new offset */
      v = (float) d;
      mscb_write(fd, adr, (unsigned char) (i + 35), &v, sizeof(float));

      /* calibrate ADCs */
      if (i==0) {
         /* reset gain and offset */
         for (j=0 ; j<8 ; j++) {
            v = 0;
            mscb_write(fd, adr, (unsigned char) (j + 19), &v, sizeof(float));
            v = 1;
            mscb_write(fd, adr, (unsigned char) (j + 27), &v, sizeof(float));
         }

         printf("Please connect DAC0 to ADC0-7 and press ENTER\n");
         fgets(str, sizeof(str), stdin);

         /* calibrate ADC offset */
         v = 0;
         size = sizeof(v);
         mscb_write(fd, adr, (unsigned char) (i + 8), &v, sizeof(float));
         Sleep(3000);

         for (j=0 ; j<8 ; j++) {
            size = sizeof(float);
            mscb_read(fd, adr, (unsigned char) j, &v, &size);
            mscb_write(fd, adr, (unsigned char) (j + 19), &v, sizeof(float));

            printf("Setting ADC%d offset to %1.3lf\n", j, v);
         }
      }

      /* calibrate gain around 10V */
      v = 9.9f;
      size = sizeof(v);
      mscb_write(fd, adr, (unsigned char) (i + 8), &v, sizeof(float));

      printf("Please measure and enter DAC%d: ", i);
      scanf("%lf", &d);

      /* write new gain */
      v = (float) (9.9/d);
      mscb_write(fd, adr, (unsigned char) (i + 43), &v, sizeof(float));

      if (i==0) {
         /* calibrate ADC gain */
         v = 9.9f;
         size = sizeof(v);
         mscb_write(fd, adr, (unsigned char) (i + 8), &v, sizeof(float));
         Sleep(3000);

         for (j=0 ; j<8 ; j++) {
            size = sizeof(float);
            mscb_read(fd, adr, (unsigned char) j, &v, &size);

            if (v > 9.8 && v < 10) {
               v = (float) (9.9/v);
               mscb_write(fd, adr, (unsigned char) (j + 27), &v, sizeof(float));

               printf("Setting ADC%d gain to %1.3lf\n", j, v);
            } else
               printf("Invalid reading for ADC%d: %1.3lf, skipping channel\n", j, v);
         }
      }
   }

   /* write constants to EEPROM */
   mscb_flash(fd, adr);

   printf("\nCalibration finished.\n");

   mscb_exit(fd);

   return 0;
}

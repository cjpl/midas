/********************************************************************\
  Name:         test_ccusb.cxx
  Created by:   Konstantin Olchanski & P.-A. Amaudruz

  Contents:     Test various CCUSB functions
\********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "ccusb.h"
#include "mcstd.h"

int main(int argc, char *argv[])
{
   int status;
   int c = 0;

   status = cam_init();
   if (status != 1) {
      printf("Error: Cannot initialize camac: cam_init status %d\n", status);
      return 1;
   }

   // Initialization

   ccusb_status(ccusb_getCrateStruct(c));

   if (0) {
     for (int i = 0; i < 4; i++) {
       cam_inhibit_set(c);
       printf("Inhibit Set\n");
       sleep(1);
       cam_inhibit_clear(c);
       printf("Inhibit Clear\n");
       sleep(1);
     }
   }

   // LAM test

   if (0) {
     for (int i = 1; i < 24; i++)
       cam_lam_disable(c, i);
     
     while (1) {
       DWORD lam;
       cam_lam_enable(c, 22);
       cam_lam_enable(c, 23);
       ccusb_status(ccusb_getCrateStruct(c));
       camc(c, 22, 0, 9);
       camc(c, 22, 0, 25);
       sleep(1);
       cam_lam_read(c, &lam);
       printf("lam status 0x%x expected 0x%x\n", lam, 1 << (22 - 1));
     }
   }
     
   // Scan crate for Read 24 /q/x
   while (1) {
      for (int n = 1; n <= 23; n++) {
         DWORD data;
         int q, x;
         cam24i_q(c, n, 0, 0, &data, &x, &q);
         printf("station %2d, data 0x%06x, q %d, x %d\n", n, data, q, x);
         sleep(1);
      }
   }

   // Access/Speed test  
   // cami/o ~370us per access (USB limitation in non-block mode)
   while (0) {
      static DWORD data;
      cam24o(c, 19, 0, 16, 0x0);
      cam24o(c, 19, 0, 16, 0x1);
      cam24o(c, 19, 0, 16, 0x0);
      cam24i(c, 22, 0, 0, &data);
      cam24o(c, 19, 0, 16, 0x2);
      cam24o(c, 19, 0, 16, 0x0);
   }

   return 0;
}

// end

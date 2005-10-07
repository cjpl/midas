/********************************************************************

  Name:         dio.c
  Created by:   Pierre Amaudruz

  Contents:     ioperm() wrapper for frontends which access CAMAC
                directly under Linux. Compile dio.c, change owner
                to root and set setuid-bit (chmod a+s dio). Then
                start the frontend with

                dio frontend

  $Id:$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>

int main(int argc, char **argv)
{
   int status;

   /* Grant access to the device's ioports */
   status = iopl(3);
   if (status < 0) {
      perror("iopl()");
      exit(2);
   }

   /* Surrender root privileges - the exec()ed program will keep IO
      access to the IO ports (see "man 2 iopl") */
   if (setuid(getuid()) < 0) {
      perror("setuid()");
      exit(2);
   }

   /* Check command arguments */
   if (argc < 2) {
      fprintf(stderr, "Usage: %s program [arguments]\n", argv[0]);
      exit(1);
   }

   /* Execute the program (with any supplied command line args) */
   if (execvp(argv[1], &argv[1]) < 0) {
      perror(argv[1]);
      exit(2);
   }
   return 0;
}

/********************************************************************

  Name:         dio.c
  Created by:   Pierre Amaudruz

  Contents:     ioperm() wrapper for frontends which access CAMAC
                directly under Linux. Compile dio.c, change owner
                to root and set setuid-bit (chmod a+s dio). Then
                start the frontend with

                dio frontend

  $Log$
  Revision 1.1  1999/12/20 09:19:11  midas
  Moved dio.c from drivers to utils direcotry

  Revision 1.2  1999/05/06 18:44:16  pierre
  - Added ioperm to port 0x80 for slow IO access (linux asm/io.h)

  Revision 1.1  1998/11/25 15:57:37  midas
  Added dio.c program as ioperm() wrapper for frontends


\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* open all ports between 0x200 and ox3FF */
static const int iobase = 0x200;
static const int num_ports = 0x200;

int main( int argc, char **argv )
{
  int i;
  int status;

  /* Grant access to the 0x80 ioports 
     This is for being able to use the i/o port function with
     _P (pause) for slow access.
     See macros in hyt1331.c
     See asm/io.h  port 0x80 is not supposed to be used */
  status = ioperm( 0x80, 1, 1 );

  /* Grant access to the device's ioports */
  status = ioperm( iobase, num_ports, 1 );
  if (status < 0) {
    perror( "ioperm()" );
    exit( 2 );
  }

  /* Surrender root privileges - the exec()ed program will keep IO
     access to the IO ports (see "man 2 ioperm") */
  if (setuid( getuid() ) < 0) {
    perror( "setuid()" );
    exit( 2 );
  }

  /* Check command arguments */
  if (argc < 2) {
    fprintf( stderr, "Usage: %s program [arguments]\n",
             argv[0] );
    exit( 1 );
  }

  /* Execute the program (with any supplied command line args) */
  if (execvp( argv[1], &argv[1] ) < 0) {
    perror( argv[1] );
    exit( 2 );
  }
  return 0;
}

/********************************************************************\

  Name:         lcwp950.c
  Created by:   Stefan Ritt

  Contents:     LeCroy WavePro 950 Digital Storage Oscilloscope 
                header file

  $Log$
  Revision 1.2  2004/01/08 08:40:08  midas
  Implemented standard indentation

  Revision 1.1  2002/01/14 16:49:53  midas
  Initial revisioin


\********************************************************************/

#define SCOPE_MAX_CHN   16
#define SCOPE_MEM_SIZE 500      /* # of points to read out */

#define SCOPE_PORT    1861      /* TCP/IP port of scope */

int scope_open(char *addr, int channel);
void scope_close(int fh);
void scope_arm();
void scope_wait(int fh);
int scope_read(int fh, double x[SCOPE_MEM_SIZE], double y[SCOPE_MEM_SIZE]);

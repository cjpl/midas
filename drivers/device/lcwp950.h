/********************************************************************\

  Name:         lcwp950.c
  Created by:   Stefan Ritt

  Contents:     LeCroy WavePro 950 Digital Storage Oscilloscope 
                header file

  $Id:$

\********************************************************************/

#define SCOPE_MAX_CHN   16
#define SCOPE_MEM_SIZE 500      /* # of points to read out */

#define SCOPE_PORT    1861      /* TCP/IP port of scope */

int scope_open(char *addr, int channel);
void scope_close(int fh);
void scope_arm();
int scope_wait(int fh, unsigned int timeout);
int scope_read(int fh, double x[SCOPE_MEM_SIZE], double y[SCOPE_MEM_SIZE]);

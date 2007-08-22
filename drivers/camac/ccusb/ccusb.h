/********************************************************************\
  Name:         ccusb.h
  Created by:   Konstantin Olchanski & P.-A. Amaudruz

  Contents:     Wiener CAMAC Controller USB 
                Using musbstd.h
\********************************************************************/

#ifndef CCUSB_H
#define CCUSB_H

#include "musbstd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCUSB_SUCCESS   0
#define CCUSB_ERROR    -1
#define CCUSB_TIMEOUT  -2

   int ccusb_init();
   MUSB_INTERFACE *ccusb_getCrateStruct(int crate);

   int ccusb_flush(MUSB_INTERFACE *);
   int ccusb_status(MUSB_INTERFACE *);

#define CCUSB_ACTION   1

   int ccusb_readReg(MUSB_INTERFACE *, int ireg);
   int ccusb_writeReg(MUSB_INTERFACE *, int ireg, int value);

#define CCUSB_VERSION    0
#define CCUSB_MODE       1
#define CCUSB_DELAYS     2
#define CCUSB_SCALERCTL  3
#define CCUSB_LEDSOURCE  4
#define CCUSB_NIMSOURCE  5
#define CCUSB_USRSOURCE  6
#define CCUSB_GATEGA     7
#define CCUSB_GATEGB     8
#define CCUSB_LAMMASK    9
#define CCUSB_LAM       10
#define CCUSB_SCALERA   11
#define CCUSB_SCALERB   12
#define CCUSB_EXTDELAYS 13
#define CCUSB_USBSETUP  14
#define CCUSB_BCASTMAP  15

   int ccusb_readCamacReg(MUSB_INTERFACE *, int ireg);
   int ccusb_writeCamacReg(MUSB_INTERFACE *, int ireg, int value);

#define CCUSB_D16  0
#define CCUSB_D24  1

   int ccusb_naf(MUSB_INTERFACE *, int n, int a, int f, int d24, int data);

   int ccusb_startStack();
   int ccusb_pushStack(int data);
   int ccusb_readStack(MUSB_INTERFACE *musb, int iaddr, uint16_t buf[], int bufsize, int verbose);
   int ccusb_writeStack(MUSB_INTERFACE *musb, int iaddr, int verbose);
   int ccusb_writeDataStack(MUSB_INTERFACE *musb, int verbose);
   int ccusb_writeScalerStack(MUSB_INTERFACE *musb, int verbose);

   int ccusb_AcqStart(MUSB_INTERFACE *c);
   int ccusb_AcqStop(MUSB_INTERFACE *c);

   int ccusb_readData(MUSB_INTERFACE *musb, char* buffer, int bufferSize, int timeout_ms);

#ifdef __cplusplus
}
#endif
#endif                          // CCUSB_H
//end

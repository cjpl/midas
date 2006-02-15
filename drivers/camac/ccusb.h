/********************************************************************\
  Name:         ccusb.h
  Created by:   Konstantin Olchanski & P.-A. Amaudruz

  Contents:     Wiener CAMAC Controller USB 
                Using musbstd.h
\********************************************************************/
// CC-USB ccusb.h

#include "musbstd.h"

#ifndef CCUSB_H
#define CCUSB_H

#ifdef __cplusplus
extern "C" {
#endif

int   ccusb_init();
MUSB_INTERFACE* ccusb_getCrateStruct(int crate);
int   ccusb_flush(MUSB_INTERFACE *);
int   ccusb_readReg(MUSB_INTERFACE *, int ireg);
int   ccusb_writeReg(MUSB_INTERFACE *, int ireg, int value);
int   ccusb_reset(MUSB_INTERFACE *);
int   ccusb_status(MUSB_INTERFACE *);
int   ccusb_naf(MUSB_INTERFACE *, int n, int a, int f, int d24, int data);

#ifdef __cplusplus
}
#endif
#endif // CCUSB_H
//end

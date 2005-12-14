/*********************************************************************

  Name:         vmeio.h
  Created by:   Pierre-Andre Amaudruz

  Cotents:      Routines for accessing the VMEIO Triumf board
                
  $Log: vmeio.h,v $
*********************************************************************/

#include "mvmestd.h"

#ifndef VMEIO_INCLUDE_H
#define VMEIO_INCLUDE_H

#define VMEIO_IRQENBL    (0x00
#define VMEIO_INTSRC     (0x04)
#define VMEIO_OUTSET     (0x08)
#define VMEIO_OUTPULSE   (0x0c)
#define VMEIO_OUTLATCH   (0x10)
#define VMEIO_RDSYNC     (0x14)
#define VMEIO_RDASYNC    (0x18)
#define VMEIO_RDCNTL     (0x1c)
#define VMEIO_CLSTB      (0x1c)

void vmeio_SyncWrite(MVME_INTERFACE *mvme, DWORD base, DWORD data);
void vmeio_AsyncWrite(MVME_INTERFACE *mvme, DWORD base, DWORD data);
void vmeio_OutputSet(MVME_INTERFACE *mvme, DWORD base, DWORD data);
int  vmeio_CsrRead(MVME_INTERFACE *mvme, DWORD base);
int  vmeio_AsyncRead(MVME_INTERFACE *myvme, DWORD base);
int  vmeio_SyncRead(MVME_INTERFACE *myvme, DWORD base);
void vmeio_StrobeClear(MVME_INTERFACE *mvme, DWORD base);

#endif  //  VMEIO_INCLUDE_H

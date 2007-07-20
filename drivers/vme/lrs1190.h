/*********************************************************************
  Name:         lrs1190.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     lrs1190 LeCroy 32bits Dual port memory (32K)

  $Id$
*********************************************************************/
#ifndef  LRS1190_INCLUDE_H
#define  LRS1190_INCLUDE_H

#include <stdio.h>
#include <string.h>
#include "mvmestd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  LRS1190_ENABLE_RW       (DWORD) (0x8002)
#define  LRS1190_RESET_WO        (DWORD) (0x0000)
#define  LRS1190_DATA_RO         (DWORD) (0x0000)
#define  LRS1190_COUNT_RO        (DWORD) (0x8000)

void lrs1190_Reset(MVME_INTERFACE *mvme, DWORD base);
void lrs1190_Enable(MVME_INTERFACE *mvme, DWORD base);
void lrs1190_Disable(MVME_INTERFACE *mvme, DWORD base);
int  lrs1190_CountRead(MVME_INTERFACE *mvme, DWORD base);
int  lrs1190_I4Read(MVME_INTERFACE *mvme, DWORD base, DWORD *data, int);
int  lrs1190_L2Read(MVME_INTERFACE *mvme, DWORD base, WORD *data, int);
int  lrs1190_H2Read(MVME_INTERFACE *mvme, DWORD base, WORD *data, int);

#ifdef __cplusplus
}
#endif

#endif // V792_INCLUDE_H

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

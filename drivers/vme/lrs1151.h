/*********************************************************************
  Name:         lrs1151.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     lrs1151 LeCroy 16channels scaler

  $Id:$
*********************************************************************/
#ifndef  LRS1151_INCLUDE_H
#define  LRS1151_INCLUDE_H

#include <stdio.h>
#include <string.h>
#include "mvmestd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  LRS1151_CLEAR_WO        (DWORD) (0x0040)
#define  LRS1151_DATA_RO         (DWORD) (0x0080)

void lrs1151_Clear(MVME_INTERFACE *mvme, DWORD base);
void  lrs1151_Read(MVME_INTERFACE *mvme, DWORD base, DWORD *data);

#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_H

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

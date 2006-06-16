/*********************************************************************
  Name:         vxVME.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     VME interface for the vxworks board processor
                using the mvmestd vme call convention.
                
  $Log: vxVME.h,v $
  Revision 1.2  2005/09/29 03:35:38  amaudruz
  VX VME driver following the mvmestd

*********************************************************************/

#include <stdio.h>
#include <string.h>

#include "mvmestd.h"

#ifndef  VXVME_INCLUDE_H
#define  VXVME_INCLUDE_H

#ifndef MIDAS_TYPE_DEFINED
typedef  unsigned long int   DWORD;
typedef  unsigned short int   WORD;
#endif

#ifndef  SUCCESS
#define  SUCCESS             (int) 1
#endif

#define  MAX_VME_SLOTS       (int) 32

#define  DEFAULT_SRC_ADD     0x200000
#define  DEFAULT_NBYTES      0x100000    /* 1MB */

/* Used for managing the map segments.
*/
typedef struct {
  int              am;
  mvme_size_t  nbytes;
  void           *ptr;
  mvme_addr_t     low;
  mvme_addr_t    high;
  int           valid;
} VME_TABLE;

/* function declarations */
int vxworks_mmap(MVME_INTERFACE *, mvme_addr_t, mvme_size_t);
mvme_addr_t vxworks_mapcheck (MVME_INTERFACE *, mvme_addr_t, mvme_size_t);

#endif // VXVME_INCLUDE_H

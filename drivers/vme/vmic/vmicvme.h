/*********************************************************************
  Name:         vmicvme.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     VME interface for the VMIC VME board processor
                using the mvmestd vme call convention.
                
  $Log: vmicvme.h,v $
  Revision 1.2  2005/09/29 03:35:38  amaudruz
  VMIC VME driver following the mvmestd

*********************************************************************/

#ifndef  __VMICVME_H__
#define  __VMICVME_H__

#include <stdio.h>
#include <string.h>
#include <vme/universe.h>
#include <vme/vme.h>
#include <vme/vme_api.h>

#include "mvmestd.h"

#ifndef MIDAS_TYPE_DEFINED
typedef  unsigned long int   DWORD;
typedef  unsigned short int   WORD;
typedef  unsigned char        BYTE;
#endif

#ifndef  SUCCESS
#define  SUCCESS             (int) 1
#endif
#define  ERROR               (int) -1000
#define  MVME_ERROR          (int) -1000

#define  MAX_VME_SLOTS       (int) 32

#define  DEFAULT_SRC_ADD     0x000000
#define  DEFAULT_NBYTES      0xFFFFFF    /* 16MB */
#define  DEFAULT_DMA_NBYTES  0x100000    /* max DMA size in bytes */

/* Used for managing the map segments.
   DMA_INFO is setup using internal defined memory block for now. 
*/
typedef struct {
  vme_master_handle_t  wh;
  int              am;
  mvme_size_t  nbytes;
  void           *ptr;
  mvme_addr_t     low;
  mvme_addr_t    high;
  int           valid;
} VME_TABLE;

typedef struct {
  vme_dma_handle_t  dma_handle;
  void             *dma_ptr;
} DMA_INFO;

typedef struct {
  vme_interrupt_handle_t  handle;
  int               level;
  int               vector;
  int               flags;
} INT_INFO;

#endif

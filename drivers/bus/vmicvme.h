/*********************************************************************
  Name:         vmicvme.h
  Created by:   Pierre-Andre Amaudruz
  Created by:   Konstantin Olchanski

  Contents:     VME interface for the VMIC VME board processor
                
  $Log$
  Revision 1.1  2005/08/23 22:29:45  olchanski
  TRIUMF VMIC-VME driver. Requires Linux drivers library from VMIC

*********************************************************************/

#ifndef  __VMICVME_H__
#define  __VMICVME_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <vme/vme.h>
#include <vme/vme_api.h>

#include <midas.h>
#include <mvmestd.h>

/* these declarations are to become the new mvmestd.h */

#ifdef __cplusplus
extern "C" {
#endif

void* mvme_getHandleA16(int crate,mvme_addr_t vmeA16addr,int numbytes,int vmeamod);
void* mvme_getHandleA24(int crate,mvme_addr_t vmeA24addr,int numbytes,int vmeamod);
void* mvme_getHandleA32(int crate,mvme_addr_t vmeA32addr,int numbytes,int vmeamod);

void mvme_writeD8(void* handle,int offset,int data);
void mvme_writeD16(void* handle,int offset,int data);
void mvme_writeD32(void* handle,int offset,int data);

int  mvme_readD8(void* handle,int offset);
int  mvme_readD16(void* handle,int offset);
int  mvme_readD32(void* handle,int offset);

#ifdef __cplusplus
}
#endif

/* these declarations would go away after all drivers are converted
   to use the new mvmestd.h interface (as above) */

#ifndef  SUCCESS
#define  SUCCESS             (int) 1
#endif
#define  ERROR               (int) -1000

#define  VME_IOCTL_SLOT_GET  (DWORD) 0x7000
#define  VME_IOCTL_AMOD_SET  (DWORD) 0x7001
#define  VME_IOCTL_AMOD_GET  (DWORD) 0x7002
#define  VME_IOCTL_PTR_GET   (DWORD) 0x7003
#define  MAX_VME_SLOTS       (int) 16

#define  DMA_MODE            (int) 1
#define  DEFAULT_SRC_ADD     0x2000
#define  DEFAULT_AMOD        0x39
#define  DEFAULT_NBYTES      0x10000    /* 1MB */

typedef struct {
  vme_master_handle_t  wh;
  DWORD           am;
  DWORD        nbytes;
  DWORD          *ptr;
  DWORD          low;
  DWORD          high;
  int           valid;
} VMETABLE;

int xmvme_mmap  (DWORD vme_addr, DWORD nbytes, DWORD am);
int xmvme_unmap (DWORD src, DWORD nbytes);
int xmvme_ioctl (DWORD req, DWORD *param);

/* inline declarations of mvme_readDnn(), mvme_writeDnn() */

extern inline int mvme_readD8(void* handle,int offset)
{
  return *(uint8_t*)(((char*)handle)+offset);
}

extern inline int mvme_readD16(void* handle,int offset)
{
  return *(uint16_t*)(((char*)handle)+offset);
}

extern inline int mvme_readD32(void* handle,int offset)
{
  return *(uint32_t*)(((char*)handle)+offset);
}

extern inline void mvme_writeD8(void* handle,int offset,int data)
{
  *(uint8_t*)(((char*)handle)+offset) = data;
}

extern inline void mvme_writeD16(void* handle,int offset,int data)
{
  *(uint16_t*)(((char*)handle)+offset) = data;
}

extern inline void mvme_writeD32(void* handle,int offset,int data)
{
  *(uint32_t*)(((char*)handle)+offset) = data;
}

#endif

/********************************************************************

  Name:         sis3100.c
  Created by:   Stefan Ritt

  Contents:     Midas VME standard (MVMESTD) layer for SIS 3100/1100
                VME controller using sis1100w.lib

  $Id$

\********************************************************************/

#ifndef EXCLUDE_VME // let applications exlucde all VME code

#ifdef __linux__
#ifndef OS_LINUX
#define OS_LINUX
#endif
#endif
#ifdef _MSC_VER
#define OS_WINNT
#endif

#ifdef OS_WINNT

#define PLX_9054
#define PCI_CODE
#define LITTLE_ENDIAN

#include <stdio.h>
#include <string.h>
#include "PlxApi.h"
#include "sis1100w.h" // Header file for sis1100w.dll
#include "sis3100_vme_calls.h"

#endif // OS_WINNT

#ifdef OS_LINUX

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sis3100_vme_calls.h"

#endif // OS_LINUX

#include "mvmestd.h"

/*------------------------------------------------------------------*/

int mvme_open(MVME_INTERFACE **vme, int idx)
{
   *vme = (MVME_INTERFACE *) malloc(sizeof(MVME_INTERFACE));
   if (*vme == NULL)
      return MVME_NO_MEM;

   memset(*vme, 0, sizeof(MVME_INTERFACE));

#ifdef OS_WINNT
   {
   int n_sis3100;

   sis1100w_Find_No_Of_sis1100(&n_sis3100);
   if (idx >= n_sis3100)
      return MVME_NO_INTERFACE;

   (*vme)->info = malloc(sizeof(struct SIS1100_Device_Struct));
   if ((*vme)->info == NULL)
      return MVME_NO_MEM;

   if (sis1100w_Get_Handle_And_Open(idx+1, (struct SIS1100_Device_Struct *) (*vme)->info) != 0)
      return MVME_NO_INTERFACE;

   if (sis1100w_Init((struct SIS1100_Device_Struct *) (*vme)->info, 0) != 0)
      return MVME_NO_INTERFACE;

   if (sis1100w_Init_sis3100((struct SIS1100_Device_Struct *) (*vme)->info, 0) != 0)
      return MVME_NO_CRATE; 
   }
#endif // OS_WINNT

#ifdef OS_LINUX
   {
   char str[256];

   /* open VME */
   sprintf(str, "/dev/sis1100_%02dremote", idx);
   (*vme)->handle = open(str, O_RDWR, 0);
   if ((*vme)->handle < 0)
      return MVME_NO_INTERFACE;
   }
#endif

   /* default values */
   (*vme)->initialized = 1;
   (*vme)->am          = MVME_AM_DEFAULT;
   (*vme)->dmode       = MVME_DMODE_D32;
   (*vme)->blt_mode    = MVME_BLT_NONE;
   (*vme)->table       = NULL; // not used

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_close(MVME_INTERFACE *vme)
{
   if (vme->initialized) {
#ifdef OS_WINNT
      sis1100w_Close((struct SIS1100_Device_Struct *) vme->info);
#endif

#ifdef OS_LINUX
      close(vme->handle);
#endif
   }

   if (vme->info)
      free(vme->info);

   free(vme);

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_sysreset(MVME_INTERFACE *vme)
{
#ifdef OS_WINNT
   sis1100w_VmeSysreset((struct SIS1100_Device_Struct *) vme->info);
#endif
#ifdef OS_LINUX
   vmesysreset(vme->handle);
#endif
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_write(MVME_INTERFACE *vme, mvme_addr_t vme_addr, void *src, mvme_size_t n_bytes)
{
   mvme_size_t n;
   DWORD status=0, data;
#ifdef OS_WINNT
   struct SIS1100_Device_Struct *hvme;
   hvme = (struct SIS1100_Device_Struct *) vme->info;
#else
   int hvme;
   hvme = vme->handle;
#endif

   if (n_bytes <= 4) {
      data = n = 0;
      memcpy(&data, src, n_bytes);

      /* A32 */
      if (vme->am >= 0x08 && vme->am <= 0x0F) {
         if (vme->dmode == MVME_DMODE_D8)
            status = vme_A32D8_write(hvme, (u_int32_t) vme_addr, (u_int8_t) data);
         else if (vme->dmode == MVME_DMODE_D16)
            status = vme_A32D16_write(hvme, (u_int32_t) vme_addr, (u_int16_t) data);
         else if (vme->dmode == MVME_DMODE_D32)
            status = vme_A32D32_write(hvme, (u_int32_t) vme_addr, (u_int32_t) data);
      }

      /* A16 */
      else if (vme->am >= 0x29 && vme->am <= 0x2D) {
         if (vme->dmode == MVME_DMODE_D8)
            status = vme_A16D8_write(hvme, (u_int32_t) vme_addr, (u_int8_t) data);
         else if (vme->dmode == MVME_DMODE_D16)
            status = vme_A16D16_write(hvme, (u_int32_t) vme_addr, (u_int16_t) data);
         else if (vme->dmode == MVME_DMODE_D32)
            status = vme_A16D32_write(hvme, (u_int32_t) vme_addr, (u_int32_t) data);
      }
      
      /* A24 */
      else if (vme->am >= 0x38 && vme->am <= 0x3F) {
         if (vme->dmode == MVME_DMODE_D8)
            status = vme_A24D8_write(hvme, (u_int32_t) vme_addr, (u_int8_t) data);
         else if (vme->am >= 0x38 && vme->am <= 0x3F && vme->dmode == MVME_DMODE_D16)
            status = vme_A24D16_write(hvme, (u_int32_t) vme_addr, (u_int16_t) data);
         else if (vme->am >= 0x38 && vme->am <= 0x3F && vme->dmode == MVME_DMODE_D32)
            status = vme_A24D32_write(hvme, (u_int32_t) vme_addr, (u_int32_t) data);
      } 
      
      if (status == 0)
         n = n_bytes;
      else
         n = 0;
   } else {

      n = 0;

      /* A32 */
      if (vme->am >= 0x08 && vme->am <= 0x0f) {
         if (vme->blt_mode == MVME_BLT_NONE)
            status = vme_A32DMA_D32_write(hvme, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_BLT32)
            status = vme_A32BLT32_write(hvme, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_MBLT64)
            status = vme_A32MBLT64_write(hvme, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
      }

      /* A24 */
      else if (vme->am >= 0x38 && vme->am <= 0x3f) {
         if (vme->blt_mode == MVME_BLT_BLT32)
            status = vme_A24BLT32_write(hvme, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_MBLT64)
            status = vme_A24MBLT64_write(hvme, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
      }

      else 
         n = 0;

      n = n*4;
   }

   return n;
}

/*------------------------------------------------------------------*/

int mvme_write_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr, unsigned int value)
{
   mvme_size_t n;
   DWORD status=0;
#ifdef OS_WINNT
   struct SIS1100_Device_Struct *hvme;
   hvme = (struct SIS1100_Device_Struct *) vme->info;
#else
   int hvme;
   hvme = vme->handle;
#endif

   if (vme->dmode == MVME_DMODE_D8)
      n = 1;
   else if (vme->dmode == MVME_DMODE_D16)
      n = 2;
   else
      n = 4;

   /* A16 */
   if (vme->am >= 0x29 && vme->am <= 0x2D) {
      if (vme->dmode == MVME_DMODE_D8)
         status = vme_A16D8_write(hvme, (u_int32_t) vme_addr, (u_int8_t) value);
      else if (vme->dmode == MVME_DMODE_D16)
         status = vme_A16D16_write(hvme, (u_int32_t) vme_addr, (u_int16_t) value);
      else if (vme->dmode == MVME_DMODE_D32)
         status = vme_A16D32_write(hvme, (u_int32_t) vme_addr, (u_int32_t) value);
   }
   
   /* A24 */
   else if (vme->am >= 0x38 && vme->am <= 0x3F) {
      if (vme->dmode == MVME_DMODE_D8)
         status = vme_A24D8_write(hvme, (u_int32_t) vme_addr, (u_int8_t) value);
      else if (vme->dmode == MVME_DMODE_D16)
         status = vme_A24D16_write(hvme, (u_int32_t) vme_addr, (u_int16_t) value);
      else if (vme->dmode == MVME_DMODE_D32)
         status = vme_A24D32_write(hvme, (u_int32_t) vme_addr, (u_int32_t) value);
   }
   
   /* A32 */
   else if (vme->am >= 0x08 && vme->am <= 0x0F) {
      if (vme->dmode == MVME_DMODE_D8)
         status = vme_A32D8_write(hvme, (u_int32_t) vme_addr, (u_int8_t) value);
      else if (vme->dmode == MVME_DMODE_D16)
         status = vme_A32D16_write(hvme, (u_int32_t) vme_addr, (u_int16_t) value);
      else if (vme->dmode == MVME_DMODE_D32)
         status = vme_A32D32_write(hvme, (u_int32_t) vme_addr, (u_int32_t) value);
   }

   if (status != 0)
      n = 0;

   return n;
}

/*------------------------------------------------------------------*/

int mvme_read(MVME_INTERFACE *vme, void *dst, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
   mvme_size_t i, n;
   DWORD data;
   WORD data16;
   int status=0;
#ifdef OS_WINNT
   struct SIS1100_Device_Struct *hvme;
   hvme = (struct SIS1100_Device_Struct *) vme->info;
#else
   int hvme;
   hvme = vme->handle;
#endif

   if (n_bytes <= 4) {
      data = 0;

      /* A32 */
      if (vme->am >= 0x08 && vme->am <= 0x0F) {
         if (vme->dmode == MVME_DMODE_D8)
            status = vme_A32D8_read(hvme, (u_int32_t) vme_addr, (u_int8_t *) &data);
         else if (vme->dmode == MVME_DMODE_D16) {
            status = vme_A32D16_read(hvme, (u_int32_t) vme_addr, (u_int16_t *) &data16);
            data = (DWORD)data16;
         } else if (vme->dmode == MVME_DMODE_D32)
            status = vme_A32D32_read(hvme, (u_int32_t) vme_addr, (u_int32_t *) &data);
      }

      /* A16 */
      else if (vme->am >= 0x29 && vme->am <= 0x2D) {
         if (vme->dmode == MVME_DMODE_D8)
            status = vme_A16D8_read(hvme, (u_int32_t) vme_addr, (u_int8_t *) &data);
         else if (vme->dmode == MVME_DMODE_D16) {
            status = vme_A16D16_read(hvme, (u_int32_t) vme_addr, (u_int16_t *) &data16);
            data = (DWORD)data16;
         } else if (vme->dmode == MVME_DMODE_D32)
            status = vme_A16D32_read(hvme, (u_int32_t) vme_addr, (u_int32_t *) &data);
      }
      
      /* A24 */
      else if (vme->am >= 0x38 && vme->am <= 0x3F) {
         if (vme->dmode == MVME_DMODE_D8)
            status = vme_A24D8_read(hvme, (u_int32_t) vme_addr, (u_int8_t *) &data);
         else if (vme->dmode == MVME_DMODE_D16) {
            status = vme_A24D16_read(hvme, (u_int32_t) vme_addr, (u_int16_t *) &data16);
            data = (DWORD)data16;
         } else if (vme->dmode == MVME_DMODE_D32)
            status = vme_A24D32_read(hvme, (u_int32_t) vme_addr, (u_int32_t *) &data);
      }
      
      memcpy(dst, &data, n_bytes);
      if (status == 0)
         n = n_bytes;
      else
         n = 0;

   } else {

      n = 0;

      /* A32 */
      if (vme->am >= 0x08 && vme->am <= 0x0F) {
         /* 2EVME has a problem with buffer size < 64 bytes */
         if (vme->blt_mode == MVME_BLT_NONE || (vme->blt_mode == MVME_BLT_2EVME && n_bytes < 64))
            status = vme_A32DMA_D32_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_BLT32)
            status = vme_A32BLT32_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_BLT32FIFO)
            status = vme_A32BLT32FIFO_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_MBLT64)
            status = vme_A32MBLT64_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_MBLT64FIFO)
            status = vme_A32MBLT64FIFO_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_2EVME)
            status = vme_A32_2EVME_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_2EVMEFIFO)
            status = vme_A32_2EVMEFIFO_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
      }

      /* A24 */
      else if (vme->am >= 0x38 && vme->am <= 0x3F) {
         if (vme->blt_mode == MVME_BLT_BLT32)
            status = vme_A24BLT32_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_BLT32FIFO)
            status = vme_A24BLT32FIFO_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_MBLT64)
            status = vme_A24MBLT64_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
         else if (vme->blt_mode == MVME_BLT_MBLT64FIFO)
            status = vme_A24MBLT64FIFO_read(hvme, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes/4, (u_int32_t*)&n);
      }

      /* A16 */
      else if (vme->am >= 0x29 && vme->am <= 0x2D) {
         if (vme->dmode == MVME_DMODE_D8)
            for (i=0 ; i<n_bytes ; i++)
               status = vme_A16D8_read(hvme, (u_int32_t) vme_addr, ((u_int8_t *) dst)+i);
         else if (vme->dmode == MVME_DMODE_D16)
            for (i=0 ; i<n_bytes/2 ; i++)
               status = vme_A16D16_read(hvme, (u_int32_t) vme_addr, ((u_int16_t *) dst)+i);
         else if (vme->dmode == MVME_DMODE_D32)
            for (i=0 ; i<n_bytes/4 ; i++)
               status = vme_A16D32_read(hvme, (u_int32_t) vme_addr, ((u_int32_t *) dst)+i);
      }

      else
         status = 1;

      n = n*4;
   }

   return n;
}

/*------------------------------------------------------------------*/

unsigned int mvme_read_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr)
{
   unsigned int data;
   WORD data16;
   int status;
#ifdef OS_WINNT
   struct SIS1100_Device_Struct *hvme;
   hvme = (struct SIS1100_Device_Struct *) vme->info;
#else
   int hvme;
   hvme = vme->handle;
#endif

   data = 0;

   /* A16 */
   if (vme->am >= 0x29 && vme->am <= 0x2D) {
      if (vme->dmode == MVME_DMODE_D8)
         status = vme_A16D8_read(hvme, (u_int32_t) vme_addr, (u_int8_t *) &data);
      else if (vme->dmode == MVME_DMODE_D16) {
         status = vme_A16D16_read(hvme, (u_int32_t) vme_addr, (u_int16_t *) &data16);
         data = (DWORD)data16;
      } else if (vme->dmode == MVME_DMODE_D32)
         status = vme_A16D32_read(hvme, (u_int32_t) vme_addr, (u_int32_t *) &data);
   }

   /* A24 */
   else if (vme->am >= 0x38 && vme->am <= 0x3F) {
      if (vme->dmode == MVME_DMODE_D8)
         status = vme_A24D8_read(hvme, (u_int32_t) vme_addr, (u_int8_t *) &data);
      else if (vme->dmode == MVME_DMODE_D16) {
         status = vme_A24D16_read(hvme, (u_int32_t) vme_addr, (u_int16_t *) &data16);
         data = (DWORD)data16;
      } else if (vme->dmode == MVME_DMODE_D32)
         status = vme_A24D32_read(hvme, (u_int32_t) vme_addr, (u_int32_t *) &data);
   }

   /* A32 */
   else if (vme->am >= 0x08 && vme->am <= 0x0F) {
      if (vme->dmode == MVME_DMODE_D8)
         status = vme_A32D8_read(hvme, (u_int32_t) vme_addr, (u_int8_t *) &data);
      else if (vme->dmode == MVME_DMODE_D16) {
         status = vme_A32D16_read(hvme, (u_int32_t) vme_addr, (u_int16_t *) &data16);
         data = (DWORD)data16;
      } else if (vme->dmode == MVME_DMODE_D32)
         status = vme_A32D32_read(hvme, (u_int32_t) vme_addr, (u_int32_t *) &data);
   }

   if (status);

   return data;
}

/*------------------------------------------------------------------*/

int mvme_set_am(MVME_INTERFACE *vme, int am)
{
   vme->am = am;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_get_am(MVME_INTERFACE *vme, int *am)
{
   *am = vme->am;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_set_dmode(MVME_INTERFACE *vme, int dmode)
{
   vme->dmode = dmode;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_get_dmode(MVME_INTERFACE *vme, int *dmode)
{
   *dmode = vme->dmode;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_set_blt(MVME_INTERFACE *vme, int mode)
{
   vme->blt_mode = mode;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_get_blt(MVME_INTERFACE *vme, int *mode)
{
   *mode = vme->blt_mode;
   return MVME_SUCCESS;
}

#endif // EXCLUDE_VME

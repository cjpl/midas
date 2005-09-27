/********************************************************************

  Name:         sis3100.c
  Created by:   Stefan Ritt

  Contents:     Midas VME standard (MVMESTD) layer for SIS 3100/1100
                VME controller using sis1100w.lib

  $Log$
  Revision 1.5  2005/09/27 10:05:26  ritt
  Implemented 'new' mvmestd

  Revision 1.4  2004/12/07 08:28:42  midas
  Revised MVMESTD

  Revision 1.3  2004/11/25 07:13:20  midas
  Added PLX defines

  Revision 1.2  2004/08/11 15:31:35  midas
  Adjusted inlude directories

  Revision 1.1  2004/08/11 15:30:07  midas
  Initial revision

\********************************************************************/

#ifdef __linux__
#define OS_LINUX
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

int mvme_open(MVME_INTERFACE *vme, int index)
{
   int n_sis3100;

#ifdef OS_WINNT
   if (vme == NULL)
      return MVME_INVALID_PARAM;

	sis1100w_Find_No_Of_sis1100(&n_sis3100);
   if (index >= n_sis3100)
      return MVME_NO_INTERFACE;

   vme->info = malloc(sizeof(struct SIS1100_Device_Struct));
   if (vme->info == NULL)
      return MVME_NO_MEM;

   if (sis1100w_Get_Handle_And_Open(index+1, (struct SIS1100_Device_Struct *) vme->info) != 0) 
      return MVME_NO_INTERFACE;

   if (sis1100w_Init((struct SIS1100_Device_Struct *) vme->info, 0) != 0)
      return MVME_NO_INTERFACE;

   if (sis1100w_Init_sis3100((struct SIS1100_Device_Struct *) vme->info, 0) != 0)
      return MVME_NO_CRATE; 
#endif // OS_WINNT

#ifdef OS_LINUX
   char str[256];

   if (vme == NULL)
      return MVME_INVALID_PARAM;

   /* open VME */
   if (index == 0)
      strcpy(str, "/tmp/sis1100");
   else
      sprintf(str, "/tmp/sis1100_%d", index+1);

   vme->handle = open(str, O_RDWR, 0);
   if (vme->handle < 0)
      return MVME_NO_INTERFACE;
#endif

   /* default values */
   vme->am        = MVME_AMOD_A32;
   vme->dmode     = MVME_DMODE_D32;
   vme->blt_mode  = MVME_BLT_NONE;
   vme->vme_table = NULL; // not used

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_close(MVME_INTERFACE *vme)
{
#ifdef OS_WINNT
   sis1100w_Close((struct SIS1100_Device_Struct *) vme->info);
   free(vme->info);
#endif

#ifdef OS_LINUX
   close(vme->handle);
#endif

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
   DWORD status, data;

#ifdef OS_WINNT

   if (n_bytes <= 4) {
      data = 0;
      memcpy(&data, src, n_bytes);
      status = sis1100w_Vme_Single_Write((struct SIS1100_Device_Struct *) vme->info, 
                                         vme_addr, vme->am, n_bytes, data);
      if (status == 0)
         n = n_bytes;
      else
         n = 0;
   } else {

      status = sis1100w_Vme_Dma_Write((struct SIS1100_Device_Struct *) vme->info, 
                                      (U32) vme_addr, 
                                      (U32) vme->am, 
                                      (U32) 4, 
                                      (U32) (vme->blt_mode == MVME_BLT_FIFO),
                                      (U32*) src, (U32) n_bytes/4, (U32*) &n);
   }

   return n;
#endif // OS_WINNT

#ifdef OS_LINUX

   if (n_bytes <= 4) {
      data = 0;
      memcpy(&data, src, n_bytes);

      if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D8)
         status = vme_A16D8_write(vme->handle, (u_int32_t) vme_addr, (u_int8_t) data);
      else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D16)
         status = vme_A16D16_write(vme->handle, (u_int32_t) vme_addr, (u_int16_t) data);
      else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D32)
         status = vme_A16D32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t) data);
      
      else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D8)
         status = vme_A24D8_write(vme->handle, (u_int32_t) vme_addr, (u_int8_t) data);
      else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D16)
         status = vme_A24D16_write(vme->handle, (u_int32_t) vme_addr, (u_int16_t) data);
      else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D32)
         status = vme_A24D32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t) data);
      
      else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D8)
         status = vme_A32D8_write(vme->handle, (u_int32_t) vme_addr, (u_int8_t) data);
      else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D16)
         status = vme_A32D16_write(vme->handle, (u_int32_t) vme_addr, (u_int16_t) data);
      else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D32)
         status = vme_A32D32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t) data);

      if (status == 0)
         n = n_bytes;
      else
         n = 0;
   } else {

      if (vme->am == MVME_AMOD_A24 && vme->blt_mode == MVME_BLT_BLT32)
         status = vme_A24BLT32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A24 && vme->blt_mode == MVME_BLT_MBLT64)
         status = vme_A24MBLT64_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes, (u_int32_t*)&n);

      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_BLT32)
         status = vme_A32BLT32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_MBLT64)
         status = vme_A32MBLT64_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) src, (u_int32_t) n_bytes, (u_int32_t*)&n);

   }

#endif // OS_LINUX
}

/*------------------------------------------------------------------*/

int mvme_write_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr, DWORD value)
{
   mvme_size_t n;
   DWORD status;

#ifdef OS_WINNT
   if (vme->dmode == MVME_DMODE_D8)
      n = 1;
   else if (vme->dmode == MVME_DMODE_D16)
      n = 2;
   else
      n = 4;

   status = sis1100w_Vme_Single_Write((struct SIS1100_Device_Struct *) vme->info, 
                                       vme_addr, vme->am, n, value);

   if (status != 0)
      n = 0;
#endif // OS_WINNT

#ifdef OS_LINUX

   if (vme->dmode == MVME_DMODE_D8)
      n = 1;
   else if (vme->dmode == MVME_DMODE_D16)
      n = 2;
   else
      n = 4;

   if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D8)
      status = vme_A16D8_write(vme->handle, (u_int32_t) vme_addr, (u_int8_t) value);
   else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D16)
      status = vme_A16D16_write(vme->handle, (u_int32_t) vme_addr, (u_int16_t) value);
   else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D32)
      status = vme_A16D32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t) value);
   
   else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D8)
      status = vme_A24D8_write(vme->handle, (u_int32_t) vme_addr, (u_int8_t) value);
   else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D16)
      status = vme_A24D16_write(vme->handle, (u_int32_t) vme_addr, (u_int16_t) value);
   else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D32)
      status = vme_A24D32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t) value);
   
   else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D8)
      status = vme_A32D8_write(vme->handle, (u_int32_t) vme_addr, (u_int8_t) value);
   else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D16)
      status = vme_A32D16_write(vme->handle, (u_int32_t) vme_addr, (u_int16_t) value);
   else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D32)
      status = vme_A32D32_write(vme->handle, (u_int32_t) vme_addr, (u_int32_t) value);

   if (status != 0)
      n = 0;

#endif // OS_LINUX

   return n;
}

/*------------------------------------------------------------------*/

int mvme_read(MVME_INTERFACE *vme, void *dst, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
   mvme_size_t n;
   DWORD data;

#ifdef OS_WINNT

   if (n_bytes <= 4) {
      data = 0;
      n = sis1100w_Vme_Single_Read((struct SIS1100_Device_Struct *) vme->info, 
                                    vme_addr, vme->am, n_bytes, &data);
      memcpy(dst, &data, n_bytes);
   } else {

      sis1100w_Vme_Dma_Read((struct SIS1100_Device_Struct *) vme->info, 
                            (U32) vme_addr, 
                            (U32) vme->am, 
                            (U32) n_bytes, 
                            (U32) (vme->blt_mode == MVME_BLT_FIFO),
                            (U32*) dst, (U32) n_bytes/4, (U32*) &n);
   }

#endif // OS_WINNT

#ifdef OS_LINUX
   int status;

   if (n_bytes <= 4) {
      data = 0;

      if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D8)
         status = vme_A16D8_read(vme->handle, (u_int32_t) vme_addr, (u_int8_t *) &data);
      else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D16)
         status = vme_A16D16_read(vme->handle, (u_int32_t) vme_addr, (u_int16_t *) &data);
      else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D32)
         status = vme_A16D32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t *) &data);
      
      else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D8)
         status = vme_A24D8_read(vme->handle, (u_int32_t) vme_addr, (u_int8_t *) &data);
      else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D16)
         status = vme_A24D16_read(vme->handle, (u_int32_t) vme_addr, (u_int16_t *) &data);
      else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D32)
         status = vme_A24D32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t *) &data);
      
      else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D8)
         status = vme_A32D8_read(vme->handle, (u_int32_t) vme_addr, (u_int8_t *) &data);
      else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D16)
         status = vme_A32D16_read(vme->handle, (u_int32_t) vme_addr, (u_int16_t *) &data);
      else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D32)
         status = vme_A32D32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t *) &data);

      memcpy(dst, &data, n_bytes);
      if (status == 0)
         n = n_bytes;
      else
         n = 0;

   } else {

      if (vme->am == MVME_AMOD_A24 && vme->blt_mode == MVME_BLT_BLT32)
         status = vme_A24BLT32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A24 && vme->blt_mode == MVME_BLT_BLT32FIFO)
         status = vme_A24BLT32FIFO_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A24 && vme->blt_mode == MVME_BLT_MBLT64)
         status = vme_A24MBLT64_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A24 && vme->blt_mode == MVME_BLT_MBLT64FIFO)
         status = vme_A24MBLT64FIFO_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);

      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_BLT32)
         status = vme_A32BLT32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_BLT32FIFO)
         status = vme_A32BLT32FIFO_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_MBLT64)
         status = vme_A32MBLT64_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_MBLT64FIFO)
         status = vme_A32MBLT64FIFO_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);

      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_2EVME)
         status = vme_A32_2EVME_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
      else if (vme->am == MVME_AMOD_A32 && vme->blt_mode == MVME_BLT_2EVMEFIFO)
         status = vme_A32_2EVMEFIFO_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t*) dst, (u_int32_t) n_bytes, (u_int32_t*)&n);
   }

#endif // OS_LINUX
   return n;
}

/*------------------------------------------------------------------*/

DWORD mvme_read_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr)
{
   DWORD data;

#ifdef OS_WINNT
   data = 0;
   if (vme->dmode == MVME_DMODE_D8)
      sis1100w_Vme_Single_Read((struct SIS1100_Device_Struct *) vme->info, 
                                vme_addr, vme->am, 1, &data);
   else if (vme->dmode == MVME_DMODE_D16)
      sis1100w_Vme_Single_Read((struct SIS1100_Device_Struct *) vme->info, 
                                vme_addr, vme->am, 2, &data);
   else
      sis1100w_Vme_Single_Read((struct SIS1100_Device_Struct *) vme->info, 
                                vme_addr, vme->am, 4, &data);

#endif // OS_WINNT

#ifdef OS_LINUX
   int status;

   data = 0;

   if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D8)
      status = vme_A16D8_read(vme->handle, (u_int32_t) vme_addr, (u_int8_t *) &data);
   else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D16)
      status = vme_A16D16_read(vme->handle, (u_int32_t) vme_addr, (u_int16_t *) &data);
   else if (vme->am == MVME_AMOD_A16 && vme->dmode == MVME_DMODE_D32)
      status = vme_A16D32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t *) &data);
   
   else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D8)
      status = vme_A24D8_read(vme->handle, (u_int32_t) vme_addr, (u_int8_t *) &data);
   else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D16)
      status = vme_A24D16_read(vme->handle, (u_int32_t) vme_addr, (u_int16_t *) &data);
   else if (vme->am == MVME_AMOD_A24 && vme->dmode == MVME_DMODE_D32)
      status = vme_A24D32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t *) &data);
   
   else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D8)
      status = vme_A32D8_read(vme->handle, (u_int32_t) vme_addr, (u_int8_t *) &data);
   else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D16)
      status = vme_A32D16_read(vme->handle, (u_int32_t) vme_addr, (u_int16_t *) &data);
   else if (vme->am == MVME_AMOD_A32 && vme->dmode == MVME_DMODE_D32)
      status = vme_A32D32_read(vme->handle, (u_int32_t) vme_addr, (u_int32_t *) &data);

#endif // OS_LINUX

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

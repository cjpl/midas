/*********************************************************************

  Name:         vxVME.c
  Created by:   Pierre-Andre Amaudruz

  Cotents:      Routines for accessing VME under VxWorks
                
  $Log$
  Revision 1.3  2004/02/06 01:15:27  pierre
  fix new definition

  Revision 1.2  2004/01/08 08:40:08  midas
  Implemented standard indentation

  Revision 1.1  2001/10/05 22:28:34  pierre
  Initial version

*********************************************************************/
#include <stdio.h>
#include "vme.h"

#ifdef OS_VXWORKS
#include "vxWorks.h"
#include "intLib.h"
#include "sys/fcntlcom.h"
#ifdef PPCxxx
#include "arch/ppc/ivPpc.h"
#else
#include "arch/mc68k/ivMc68k.h"
#endif
#include "taskLib.h"

#ifdef PPCxxx
#define A24D24         0xfa000000       /* A24D24 camac base address */
#define A24D16         0xfa000002       /* A24D16 camac base address */
#else
#define A24D24         0xf0000000       /* A24D24 camac base address */
#define A24D16         0xf0000002       /* A24D16 camac base address */
#endif
#endif

#include    "mvmestd.h"

/*------------------------------------------------------------------*/
int vme_open(int device, int mode)
{
   /* derive device name */
   switch (mode) {
   case VME_A16D16:
   case VME_A16D32:

      break;
   case VME_A24D16:
   case VME_A24D32:

      break;
   case VME_A32D16:
   case VME_A32D32:

      break;

   default:
      printf("Unknown VME mode %d\n", mode);
      return -1;
   }

   /* open device */

   /* select swapping mode */

   /* set data mode */

   /* switch on block transfer */

   return (int) 1;
}

/*------------------------------------------------------------------*/
int vme_close(int vh)
{
   return (int) 1;
}

/*------------------------------------------------------------------*/
static int last_dma = -1;

int vme_read(int vh, void *dst, int vme_addr, int size, int dma)
{
   return (int) 1;
}

/*------------------------------------------------------------------*/

int vme_write(int vh, void *src, int vme_addr, int size, int dma)
{
   return (int) 1;
}

/*------------------------------------------------------------------*/
int vme_mmap(int vh, void **ptr, int vme_addr, int size)
{
   int status;
   status = sysBusToLocalAdrs(VME_AM_STD_USR_DATA, vme_addr, (int *) ptr);
   return status == SUCCESS ? 1 : (int) status;
}

/*------------------------------------------------------------------*/
int vme_unmap(int vh, void *ptr, int size)
{
   return 1;
}

/*------------------------------------------------------------------*/
int vme_ioctl(int vh, int request, int *param)
{
   switch (request) {
   case VME_IOCTL_AMOD_SET:
      break;
   case VME_IOCTL_AMOD_GET:
      break;
   }
   return SUCCESS;
}

/*********************************************************************

  Name:         mvmestd.h
  Created by:   Stefan Ritt

  Cotents:      MIDAS VME standard routines. Have to be combined
                with bt617.c
                
  $Log$
  Revision 1.7  2004/01/08 08:40:09  midas
  Implemented standard indentation

  Revision 1.6  2003/11/24 08:22:45  midas
  Changed timeouts from INT to DWORD, added ignore_timeout to cm_cleanup, adde '-f' flag to ODBEdit 'cleanup'

  Revision 1.5  2001/06/27 12:16:30  midas
  Added OS_IRIX

  Revision 1.4  2001/04/05 05:51:36  midas
  Added VME_LM

  Revision 1.3  2000/09/28 11:12:15  midas
  Added DMA flag to vme_read/vme_write

  Revision 1.2  2000/09/26 07:45:19  midas
  Added vme_write

  Revision 1.1  2000/09/26 07:26:56  midas
  Added file


*********************************************************************/

/*---- replacements if not running under MIDAS ---------------------*/

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned short int WORD;

#ifdef __alpha
typedef unsigned int DWORD;
#else
typedef unsigned long int DWORD;
#endif

#define SUCCESS  1

#endif                          /* MIDAS_TYPE_DEFINED */

/* make functions under WinNT dll exportable */
#if defined(_MSC_VER) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

/*---- constants ---------------------------------------------------*/

/* data modes */
#define VME_A16D16    1
#define VME_A16D32    2
#define VME_A24D16    3
#define VME_A24D32    4
#define VME_A32D16    5
#define VME_A32D32    6
#define VME_RAMD16    7         /* RAM memory of VME adapter */
#define VME_RAND32    8
#define VME_LM        9         /* local memory mapped to VME */

/* vme_ioctl commands */
#define VME_IOCTL_AMOD_SET           0
#define VME_IOCTL_AMOD_GET           1

/* vme bus address modifiers */
#define VME_AMOD_A32_SB     (0x0F)      /* A32 Extended Supervisory Block */
#define VME_AMOD_A32_SP     (0x0E)      /* A32 Extended Supervisory Program */
#define VME_AMOD_A32_SD     (0x0D)      /* A32 Extended Supervisory Data */
#define VME_AMOD_A32_NB     (0x0B)      /* A32 Extended Non-Privileged Block */
#define VME_AMOD_A32_NP     (0x0A)      /* A32 Extended Non-Privileged Program */
#define VME_AMOD_A32_ND     (0x09)      /* A32 Extended Non-Privileged Data */
#define VME_AMOD_A32_SMBLT  (0x0C)      /* A32 Multiplexed Block Transfer (D64) */
#define VME_AMOD_A32_NMBLT  (0x08)      /* A32 Multiplexed Block Transfer (D64) */

#define VME_AMOD_A32     VME_AMOD_A32_SD
#define VME_AMOD_A32_D64 VME_AMOD_A32_SMBLT

#define VME_AMOD_A24_SB     (0x3F)      /* A24 Standard Supervisory Block Transfer      */
#define VME_AMOD_A24_SP     (0x3E)      /* A24 Standard Supervisory Program Access      */
#define VME_AMOD_A24_SD     (0x3D)      /* A24 Standard Supervisory Data Access         */
#define VME_AMOD_A24_NB     (0x3B)      /* A24 Standard Non-Privileged Block Transfer   */
#define VME_AMOD_A24_NP     (0x3A)      /* A24 Standard Non-Privileged Program Access   */
#define VME_AMOD_A24_ND     (0x39)      /* A24 Standard Non-Privileged Data Access      */
#define VME_AMOD_A24_SMBLT  (0x3C)      /* A24 Multiplexed Block Transfer (D64) */
#define VME_AMOD_A24_NMBLT  (0x38)      /* A24 Multiplexed Block Transfer (D64) */

#define VME_AMOD_A24     VME_AMOD_A24_SD
#define VME_AMOD_A24_D64 VME_AMOD_A24_SMBLT

#define VME_AMOD_A16_SD  (0x2D) /* A16 Short Supervisory Data Access            */
#define VME_AMOD_A16_ND  (0x29) /* A16 Short Non-Privileged Data Access         */

#define VME_AMOD_A16     BT_AMOD_A16_SD

/*---- function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

   int EXPRT vme_open(int device, int mode);
   int EXPRT vme_ioctl(int vh, int req, int *parm);
   int EXPRT vme_close(int vh);
   int EXPRT vme_read(int vh, void *dst, int vme_addr, int size, int dma);
   int EXPRT vme_write(int vh, void *src, int vme_addr, int size, int dma);
   int EXPRT vme_mmap(int vh, void **ptr, int vme_addr, int size);
   int EXPRT vme_unmap(int vh, void *ptr, int size);


#ifdef __cplusplus
}
#endif

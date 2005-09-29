/*********************************************************************

  Name:         mvmestd.h
  Created by:   Stefan Ritt

  Cotents:      Midas VME standard routines (MVMESTD) supplying an
                abstract layer to all supported VME interfaces.
                
  $Log$
  Revision 1.12  2005/09/29 03:42:31  amaudruz
  add new error type

  Revision 1.11  2005/09/28 04:36:02  ritt
  vme structure gets allocated in driver

  Revision 1.10  2005/09/27 10:05:52  ritt
  Implemented 'new' mvmestd

  Revision 1.9  2004/12/07 09:59:04  midas
  Revised MVMESTD

  Revision 1.8  2004/09/10 12:33:47  midas
  Implemented SIS3100/1100

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

/*---- status codes ------------------------------------------------*/

#define MVME_SUCCESS                  1
#define MVME_NO_INTERFACE             2
#define MVME_NO_CRATE                 3
#define MVME_UNSUPPORTED              4
#define MVME_INVALID_PARAM            5
#define MVME_NO_MEM                   6
#define MVME_ACCESS_ERROR             7

/*---- types -------------------------------------------------------*/

typedef unsigned long mvme_addr_t;
typedef unsigned long mvme_size_t;

/*---- constants ---------------------------------------------------*/

/* data modes */
#define MVME_DMODE_D8                 1
#define MVME_DMODE_D16                2
#define MVME_DMODE_D32                3
#define MVME_DMODE_D64                4
#define MVME_DMODE_RAMD16             5   /* RAM memory of VME adapter */
#define MVME_DMODE_RAMD32             6
#define MVME_DMODE_LM                 7   /* local memory mapped to VME */

/* block transfer modes */
#define MVME_BLT_NONE                 1   /* normal programmed IO */
#define MVME_BLT_BLT32                2   /* 32-bit block transfer */
#define MVME_BLT_MBLT64               3   /* multiplexed 64-bit block transfer */
#define MVME_BLT_2EVME                4   /* two edge block transfer */
#define MVME_BLT_2ESST                5   /* two edge source synchrnous transfer */
#define MVME_BLT_BLT32FIFO            6   /* FIFO mode, don't increment address */
#define MVME_BLT_MBLT64FIFO           7   /* FIFO mode, don't increment address */
#define MVME_BLT_2EVMEFIFO            8   /* two edge block transfer with FIFO mode */

/* vme bus address modifiers */
#define MVME_AM_A32_SB     (0x0F)      /* A32 Extended Supervisory Block */
#define MVME_AM_A32_SP     (0x0E)      /* A32 Extended Supervisory Program */
#define MVME_AM_A32_SD     (0x0D)      /* A32 Extended Supervisory Data */
#define MVME_AM_A32_NB     (0x0B)      /* A32 Extended Non-Privileged Block */
#define MVME_AM_A32_NP     (0x0A)      /* A32 Extended Non-Privileged Program */
#define MVME_AM_A32_ND     (0x09)      /* A32 Extended Non-Privileged Data */
#define MVME_AM_A32_SMBLT  (0x0C)      /* A32 Multiplexed Block Transfer (D64) */
#define MVME_AM_A32_NMBLT  (0x08)      /* A32 Multiplexed Block Transfer (D64) */

#define MVME_AM_A32     MVME_AM_A32_SD
#define MVME_AM_A32_D64 MVME_AM_A32_SMBLT

#define MVME_AM_A24_SB     (0x3F)      /* A24 Standard Supervisory Block Transfer      */
#define MVME_AM_A24_SP     (0x3E)      /* A24 Standard Supervisory Program Access      */
#define MVME_AM_A24_SD     (0x3D)      /* A24 Standard Supervisory Data Access         */
#define MVME_AM_A24_NB     (0x3B)      /* A24 Standard Non-Privileged Block Transfer   */
#define MVME_AM_A24_NP     (0x3A)      /* A24 Standard Non-Privileged Program Access   */
#define MVME_AM_A24_ND     (0x39)      /* A24 Standard Non-Privileged Data Access      */
#define MVME_AM_A24_SMBLT  (0x3C)      /* A24 Multiplexed Block Transfer (D64) */
#define MVME_AM_A24_NMBLT  (0x38)      /* A24 Multiplexed Block Transfer (D64) */

#define MVME_AM_A24     MVME_AM_A24_SD
#define MVME_AM_A24_D64 MVME_AM_A24_SMBLT

#define MVME_AM_A16_SD  (0x2D) /* A16 Short Supervisory Data Access            */
#define MVME_AM_A16_ND  (0x29) /* A16 Short Non-Privileged Data Access         */

#define MVME_AM_A16     MVME_AM_A16_SD

#define MVME_AM_DEFAULT MVME_AM_A32

/*---- interface structure -----------------------------------------*/

#define MAX_CRATE         10             /* maximum number of crates */

typedef struct {
   int  handle;              // internal handle
   void *info;               // internal info structure
   int  am;                  // Address modifier
   int  dmode;
   int  blt_mode;
   void *table;
} MVME_INTERFACE;

/*---- function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

   int EXPRT mvme_open(MVME_INTERFACE **vme, int index);
   int EXPRT mvme_close(MVME_INTERFACE *vme);
   int EXPRT mvme_sysreset(MVME_INTERFACE *vme);
   int EXPRT mvme_read(MVME_INTERFACE *vme, void *dst, mvme_addr_t vme_addr, mvme_size_t n_bytes);
   DWORD EXPRT mvme_read_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr);
   int EXPRT mvme_write(MVME_INTERFACE *vme, mvme_addr_t vme_addr, void *src, mvme_size_t n_bytes);
   int EXPRT mvme_write_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr, DWORD value);
   int EXPRT mvme_set_am(MVME_INTERFACE *vme, int am);
   int EXPRT mvme_get_am(MVME_INTERFACE *vme, int *am);
   int EXPRT mvme_set_dmode(MVME_INTERFACE *vme, int dmode);
   int EXPRT mvme_get_dmode(MVME_INTERFACE *vme, int *dmode);
   int EXPRT mvme_set_blt(MVME_INTERFACE *vme, int mode);
   int EXPRT mvme_get_blt(MVME_INTERFACE *vme, int *mode);

#ifdef __cplusplus
}
#endif

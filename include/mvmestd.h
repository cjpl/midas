/*********************************************************************

  Name:         mvmestd.h
  Created by:   Stefan Ritt

  Cotents:      MIDAS VME standard routines. Have to be combined
                with bt617.c
                
  $Log$
  Revision 1.1  2000/09/26 07:26:56  midas
  Added file


*********************************************************************/

/*---- replacements if not running under MIDAS ---------------------*/

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned short int WORD;

#ifdef __alpha
typedef unsigned int       DWORD;
#else
typedef unsigned long int  DWORD;
#endif

#define SUCCESS  1

#endif /* MIDAS_TYPE_DEFINED */

/* make functions under WinNT dll exportable */
#if defined(_MSC_VER) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

/*---- constants ---------------------------------------------------*/

#define VME_A16D16    1
#define VME_A16D32    2
#define VME_A24D16    3
#define VME_A24D32    4
#define VME_A32D16    5
#define VME_A32D32    6
#define VME_RAMD16    7
#define VME_RAND32    8

/*---- function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

int EXPRT vme_open(int device, int mode);
int EXPRT vme_close(int vh);
int EXPRT vme_read(int vh, void *dst, int vme_addr, int size);
int EXPRT vme_mmap(int vh, void **ptr, int vme_addr, int size);
int EXPRT vme_unmap(int vh, void *ptr, int size);


#ifdef __cplusplus
}
#endif

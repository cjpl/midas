/*-----------------------------------------------------------------------------
 *  Copyright (c) 1998      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 * 
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca           Tel: (604) 222-1047  Fax: (604) 222-1074
 *         amaudruz@triumf.ca
 * -----------------------------------------------------------------------------
 *  
 *  Description : Esone Standard calls. 
 *  Requires : 
 *  Application : Used in any camac driver
 *  Author:  Pierre-Andre Amaudruz Data Acquisition Group
 *
 *  $Log$
 *  Revision 1.3  2000/04/26 19:14:01  pierre
 *  - Moved doc++ comments to drivers/bus/esone.c
 *
 *  Revision 1.2  2000/04/17 17:33:59  pierre
 *  - First round of doc++ comments.
 *
 *  Revision 1.1  1999/02/22 18:55:10  pierre
 *  - esone standard
 *
 *  Revision 1.3  1998/10/13 07:04:29  midas
 *  Added Log in header
 *
 *  Previous revision 1.0  1998        Pierre	 Initial revision
 *---------------------------------------------------------------------------*/

#ifndef INLINE
#if defined( _MSC_VER )
#define INLINE __inline
#elif defined(__GNUC__)
#define INLINE __inline__
#else 
#define INLINE
#endif
#endif

#define EXTERNAL extern 

/* make functions under WinNT dll exportable */
#if defined(_MSC_VER) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

/*------------------------------------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

/* -- ESONE standard ---------------------------------------------------*/
EXTERNAL INLINE void cdreg(int *ext, const int b, const int c, const int n, const int a);
EXTERNAL INLINE void cssa (const int f, int ext, unsigned short *d, int *q);
EXTERNAL INLINE void cfsa (const int f, const int ext, unsigned long *d, int *q);
EXTERNAL INLINE void cccc (const int ext);
EXTERNAL INLINE void cccz (const int ext);
EXTERNAL INLINE void ccci (const int ext, int l);
EXTERNAL INLINE void cccd (const int ext, int l);
EXTERNAL INLINE void ctcd (const int ext, int *l);
EXTERNAL INLINE void cdlam(int *lam, const int b, const int c, const int n, const int a, const int inta[2]);
EXTERNAL INLINE void ctgl (const int lam, int *l);
EXTERNAL INLINE void cclm (const int lam, int l);
EXTERNAL INLINE void cclc (const int lam);
EXTERNAL INLINE void ctlm (const int lam, int *l);
EXTERNAL INLINE void cfga (int f[], int exta[], int intc[], int qa[], int cb[]);
EXTERNAL INLINE void cfmad(int f, int extb[], int intc[], int cb[]);
EXTERNAL INLINE void cfubc(const int f, int ext, int intc[], int cb[]);
EXTERNAL INLINE void cfubr(const int f,int ext,int intc[],int cb[]);
/*-----------------------------------------------------------------------*/

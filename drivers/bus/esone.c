/*-----------------------------------------------------------------------------
 *  Copyright (c) 1998      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 * 
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca           Tel: (604) 222-1047  Fax: (604) 222-1074
 *         amaudruz@triumf.ca
 * -----------------------------------------------------------------------------
 *  
 *  Description	: CAMAC interface for ESONE standard using mcstd.h
 *	
 *  Requires 	: 
 *
 *  Application : Any CAMAC application
 *
 *  Author:  Stefan Ritt & Pierre-Andre Amaudruz
 * 
 *  Revision 1.0  1998        Pierre	 Initial revision
   $Log$
   Revision 1.2  2000/04/26 19:14:41  pierre
   - Moved doc++ comments from esone.h to here.

   Revision 1.1  1999/12/20 10:18:11  midas
   Reorganized driver directory structure

   Revision 1.3  1999/02/20 00:41:38  pierre
   - include CVS Log

 *
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include "mcstd.h"
#include "esone.h"
#ifndef INLINE 
#if defined( _MSC_VER )
#define INLINE __inline
#elif defined(__GNUC__)
#define INLINE __inline__
#else 
#define INLINE
#endif
#endif

/*--external-----------------------------------------------------*/
INLINE void came_cn(int *ext, const int b, const int c, const int n, const int a){
  *ext = (b<<24 | (c<<16) | (n<<8) | a);
}

/*--external-----------------------------------------------------*/
INLINE void came_ext(const int ext, int *b, int *c, int *n, int *a){
  *b = (ext >> 24) & 0x7;
  *c = (ext >> 16) & 0x7;
  *n = (ext >>  8) & 0x1f;
  *a = (ext >>  0) & 0xf;
}

/*-- REAL ESONE INTERFACE ---------------------------------------*/
/*--External-----------------------------------------------------*/
/** cdreg
    Control Declaration REGister.

    Compose an external address from BCNA for later use.
    Accessing CAMAC through ext could be faster if the external address is
    memory mapped to the processor (hardware dependent). Some CAMAC controller
    do not have this option see \Ref{appendix D: Supported hardware}. In this case
    @memo External Address register.
    @param ext external address
    @param b branch number (0..7)
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @return void
*/
INLINE void cdreg (int *ext, const int b, const int c, const int n, const int a)
{ /* ext encoding */
  came_cn (ext, b, c, n, a);
}

/*--16bit ESONE--------------------------------------------------*/
/** cssa
    Control Short Operation.

    16 bit operation on a given external CAMAC address.

    The range of the f is hardware dependent. The number indicated below are for
    standard ANSI/IEEE Std (758-1979) \\
    Execute cam16i for f<8, cam16o for f>15, camc\_q for (f>7 or f>23)
    
    @memo 16 bit function.
    @param f function code (0..31)
    @param ext external address
    @param d data word
    @param q Q response
    @return void
*/
INLINE void cssa (const int f, int ext, unsigned short *d, int *q)
{ /* 16bit action */
  static int b,c,n,a,x;
  
  if (f < 8)
  {
    /* read */
    came_ext(ext, &b, &c, &n, &a);
    cam16i_q(c,n,a,f,d,&x,q);
  }
  else if (f >15)
  {
    /* write */
    came_ext(ext, &b, &c, &n, &a);
    cam16o_q(c,n,a,f,*d,&x,q);
  }
  else if ((f > 7) || (f > 23))
  {
    /* command */
    came_ext(ext, &b, &c, &n, &a);
    camc_q(c,n,a,f,q);
  }
}

/*--24bit ESONE--------------------------------------------------*/
/** cfsa
    Control Full Operation.
    
    24 bit operation on a given external CAMAC address.
    
    The range of the f is hardware dependent. The number indicated below are for
    standard ANSI/IEEE Std (758-1979) \\
    Execute cam24i for f<8, cam24o for f>15, camc\_q for (f>7 or f>23)
    
    @memo 24 bit function.
    @param f function code (0..31)
    @param ext external address
    @param d data long word
    @param q Q response
    @return void
*/
INLINE void cfsa (const int f, const int ext, unsigned long *d, int *q)
{ /* 24bit action */
  static int b,c,n,a,x;
  
  if (f < 8)
  {
    /* read */
    came_ext(ext, &b, &c, &n, &a);
    cam24i_q(c,n,a,f,d,&x,q);
  }
  else if (f >15)
  {
    /* write */
    came_ext(ext, &b, &c, &n, &a);
    cam24o_q(c,n,a,f,*d,&x,q);
  }
  else if ((f > 7) || (f > 23))
  {
    /* command */
    came_ext(ext, &b, &c, &n, &a);
    camc_q(c,n,a,f,q);
  }
}

/*--ESONE General functions--------------------------------------*/
/** cccc
    Control Crate Clear.

    Generate Crate Clear function. Execute cam\_crate\_clear()
    
    @memo Crate Clear.
    @param ext external address
    @return void
*/
INLINE void cccc(const int ext)
{ /* C cycle */
  int b, c, n, a;
  
  came_ext(ext, &b, &c, &n, &a);
  cam_crate_clear (c);
}	

/*--ESONE General functions--------------------------------------*/
/** cccz
    Control Crate Z.

    Generate Dataway Initialize. Execute cam\_crate\_zinit()
    
    @memo Crate Z.
    @param ext external address
    @return void
*/
INLINE void cccz(const int ext)
{ /* Z cycle */
  int b, c, n, a;
  
  came_ext(ext, &b, &c, &n, &a);
  cam_crate_zinit (c);
}	

/*--ESONE General functions--------------------------------------*/
/** ccci
    Control Crate I.

    Set or Clear Dataway Inhibit, Execute cam\_inhinit\_set() /clear()
    which is equivalent to c=c,n=30,a=9,f26/f24
    
    @memo Crate I.
    @param ext external address
    @param l action l=0 -> Clear I, l=1 -> Set I
    @return void
*/
INLINE void ccci(const int ext, int l)
{ /* Set / Reset Inhibit */
  int b, c, n, a;
  
  came_ext(ext, &b, &c, &n, &a);
  if (l)
    cam_inhibit_set(c);
  else
    cam_inhibit_clear(c);
}	

/*--ESONE General functions--------------------------------------*/
/** cccd
    Control Crate D.

    Enable or Disable Crate Demand. Execute c=c,n=30,a=10,f=26 for enable
    , f24 for disable.
    \\ {\bf Has not been tested}
    
    @memo Crate D.
    @param ext external address
    @param l action l=0 -> Clear D, l=1 -> Set D
    @return void
*/
INLINE void  cccd(const int ext, int l)
{ /* Enable / Disable Crate Demand */
  int b, c, n, a;
  
  came_ext(ext, &b, &c, &n, &a);
  
  if (l)
    camc (c,30,10,26);          /* c=c,n=30,a=10,f=26 */
  else
    camc (c,30,10,24);          /* c=c,n=30,a=10,f=24 */
}

/*--ESONE General functions--------------------------------------*/
/** ctcd
    Control Test Crate D.

    Test Crate Demand. Execute c=c,n=30,a=10,f=27.
    \\ {\bf Has not been tested}
    
    @memo Test Crate D.
    @param ext external address
    @param l D cleared -> l=0, D set -> l=1
    @return void
*/
INLINE void  ctcd(const int ext, int *l)
{ /* Test Crate Demand Enabled */
  int b, c, n, a, x;
  unsigned short dtemp;

  came_ext(ext, &b, &c, &n, &a);
  cam16i_q (c,30,10,27,&dtemp,&x,l);        /* c=c,n=30,a=10,f=27 */
}

/*--ESONE General functions--------------------------------------*/
/** cdlam
    Control Declare LAM.

    Declare LAM, Identical to cdreg.
    \\ {\bf Has not been tested}
    
    @memo Declare LAM.
    @param lam external LAM address
    @param b branch number (0..7)
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param inta[2] implementation dependent
    @return void
*/
INLINE void  cdlam(int *lam, const int b, const int c, const int n, const int a
                   , const int inta[2])
{ /* Declare LAM */
  /* inta[2] ignored */
  cdreg (lam, b, c, n, a);
}

/*--ESONE General functions--------------------------------------*/
/** ctgl
    Control Test Demand Present.

    Test the Graded LAM register. Execute c=c,n=30,a=11,f=27.
    \\ {\bf Has not been tested}
    
    @memo Test GL.
    @param lam external LAM register address
    @param l  l !=0 if any LAM is set.
    @return void
*/
INLINE void  ctgl(const int ext, int *l)
{ /* Test Crate Demand Present */
  int b, c, n, a;

  came_ext(ext, &b, &c, &n, &a);
  camc_q (c,30,11,27,l);        /* c=c,n=30,a=11,f=27 */
}

/*--ESONE General functions--------------------------------------*/
/** cclm
    Control Crate LAM.

    Enable or Disable LAM. Execute F24 for disable, F26 for enable.
    \\ {\bf Has not been tested}
    
    @memo dis/enable LAM.
    @param lam external address
    @param l action l=0 -> disable LAM , l=1 -> enable LAM
    @return void
*/
INLINE void  cclm(const int lam, int l)
{ /* Enable/Disable LAM */
  int b, c, n, a;
  /*  may requires a0 */
  came_ext(lam, &b, &c, &n, &a);
  
  if (l)
    camc (c,n,0,26);
  else
    camc (c,n,0,24);
}

/*--ESONE General functions--------------------------------------*/
/** cclc
    Control Clear LAM.
    Clear the LAM of th estaion pointer by the lam address.
    \\ {\bf Has not been tested}

    @memo Clear LAM.
    @param lam external address
    @return void
*/
INLINE void  cclc(const int lam)
{ /* Clear LAM */
  int b, c, n, a;
  /*  may requires a0 */
  came_ext(lam, &b, &c, &n, &a);
  camc (c,n,0,10);
}

/*--ESONE General functions--------------------------------------*/
/** ctlm
    Test LAM.

    Test the LAM of the station pointed by lam. Performs an F10
    \\ {\bf Has not been tested}
    
    @memo Test LAM.
    @param lam external address
    @param l No LAM-> l=0, LAM present-> l=1
    @return void
*/
INLINE void  ctlm(const int lam, int *l)
{ /* Test LAM */
  /* inta[2] ignored */
  int x;
  static int b,c,n,a;
  static WORD dtemp;
  
  came_ext(lam, &b, &c, &n, &a);
  cam16i_q(c,n,a,10,&dtemp,&x,l);
}

/*--ESONE General functions--------------------------------------*/
/** cfga
    Control Full (24bit) word General Action.
    \\ {\bf Has not been tested}

    @memo General external address scan function.
    @param f function code
    @param exta[] external address array
    @param intc[] data array
    @param qa[] Q response array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
INLINE void  cfga(int f[], int exta[], int intc[], int qa[], int cb[])
{ /* Multiple Action */
  int i;
  for (i=0;i<cb[0];i++)
  {
    cfsa(f[i],exta[i],(unsigned long *)(&(intc[i])),&(qa[i]));
  }
  cb[1] = cb[0];
}

/*--ESONE General functions--------------------------------------*/
/** cfmad
    Control Full (24bit) Address Q scan.

    Scan all sub-address while Q=1 from a0..a15 max from address extb[0] and store
    corresponding data in intc[]. If Q=0 while A<15 or A=15 then cross station boundary is applied
    (n-> n+1) and sub-address is reset (a=0). Perform action until either cb[0] action are performed
    or current external address exceeds extb[1].

    {\it implementation of cb[2] for LAM recognition is not implemented.}
    \\ {\bf Has not been tested}
    
    @memo Address scan function.
    @param f function code
    @param extb[] external address array \\
    extb[0] : first valid external address \\
    extb[1] : last valid external address
    @param intc[] data array
    @param qa[] Q response array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
INLINE void  cfmad(int f, int extb[], int intc[], int cb[])
{ /* Address scan */
  int j, count;
  int x, q, b, c, n, a;
  unsigned long exts, extc, exte;
  
  exts = extb[0];
  exte = extb[1];
  count = cb[0];
  j = 0;
  came_ext(exts, &b, &c, &n, &a);
  do
  {
    cam24i_q(c,n,a,f,(unsigned long *)&intc[j],&x, &q);
    if (q == 0)
    {
      a = 0;                /* set subaddress to zero */
      n++;                  /* select next slot */
      j++;
    }
    else
    {
      a++;                  /* increment address */
      ++cb[1];		/* increment tally count */
      ++intc;		/* next data array */
      --count;
    }
    came_cn ((int *)&extc,b,c,n,a);
    if (extc > exte) count = 0; /* force exit */
  }
  while (count);
}

/*--ESONE General functions--------------------------------------*/
/** cfubc
    Control Full (24bit) Block Repeat with Q-stop.

    Execute function f on address ext with data intc[] while Q.
    \\ {\bf Has not been tested}
    
    @memo Repeat function Q-stop.
    @param f function code
    @param ext external address array
    @param intc[] data array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
INLINE void  cfubc(const int f, int ext, int intc[], int cb[])
{ /* Q-Stop mode block transfer */
  int count, q;
  
  count = cb[0];
  do
  {
    cfsa (f,ext,(unsigned long *)intc, &q);
    if (q == 0)
      count = 0;	/* stop on no q */
    else
    {
      ++cb[1];		/* increment tally count */
      ++intc;		/* next data array */
      --count;
    }
  }
  while (count);
}

/*--ESONE General functions--------------------------------------*/
/** cfubr
    Control Full (24bit) Block Repeat.

    Execute function f on address ext with data intc[] if Q.
    If noQ keep current intc[] data. Repeat cb[0] times. 
    \\ {\bf Has not been tested}
    @memo  Repeat function.
    @param f function code
    @param ext external address array
    @param intc[] data array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
INLINE void cfubr(const int f,int ext,int intc[],int cb[])
{ /* Repeat mode block transfer */
  int q, count;
  
  count = cb[0];
  do
  {
    do
      cfsa (f, ext,(unsigned long *)intc, &q);
    while (q == 0);
    ++cb[1];		/* increment tally count */
    ++intc;		/* next data array */
    --count;
  }
  while (count);
}

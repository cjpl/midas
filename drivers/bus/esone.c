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
INLINE void cdreg (int *ext, const int b, const int c, const int n, const int a)
{ /* ext encoding */
  came_cn (ext, b, c, n, a);
}

/*--16bit ESONE--------------------------------------------------*/
INLINE void cssa (const int f, int ext, unsigned short *d, int *q)
{ /* 16bit action */
  int x;
  static int b,c,n,a;
  static unsigned short dtemp;
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
      cam16i_q(c,n,a,f,&dtemp,&x,q);
    }
}

/*--24bit ESONE--------------------------------------------------*/
INLINE void cfsa (const int f, const int ext, unsigned long *d, int *q)
{ /* 24bit action */
  int x;
  static int b,c,n,a;
  static unsigned short dtemp;

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
      cam16i_q(c,n,a,f,&dtemp,&x,q);
    }
}

/*--ESONE General functions--------------------------------------*/
INLINE void cccc(const int ext)
{ /* C cycle */
  int b, c, n, a;

  came_ext(ext, &b, &c, &n, &a);
  cam_crate_clear (c);
}	

/*--ESONE General functions--------------------------------------*/
INLINE void cccz(const int ext)
{ /* Z cycle */
  int b, c, n, a;

  came_ext(ext, &b, &c, &n, &a);
  cam_crate_zinit (c);
}	

/*--ESONE General functions--------------------------------------*/
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
INLINE void  ctcd(const int ext, int *l)
{ /* Test Crate Demand Enabled */
  int b, c, n, a, x;
  unsigned short dtemp;

  came_ext(ext, &b, &c, &n, &a);
  cam16i_q (c,30,10,27,&dtemp,&x,l);        /* c=c,n=30,a=10,f=27 */
}

/*--ESONE General functions--------------------------------------*/
INLINE void  cdlam(int *lam, const int b, const int c, const int n, const int a
                   , const int inta[2])
{ /* Declare LAM */
  /* inta[2] ignored */
  cdreg (lam, b, c, n, a);
}

/*--ESONE General functions--------------------------------------*/
INLINE void  ctgl(const int ext, int *l)
{ /* Test Crate Demand Present */
  int b, c, n, a;

  came_ext(ext, &b, &c, &n, &a);
  camc_q (c,30,11,27,l);        /* c=c,n=30,a=11,f=27 */
}

/*--ESONE General functions--------------------------------------*/
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
INLINE void  cclc(const int lam)
{ /* Clear LAM */
  int b, c, n, a;
  /*  may requires a0 */
  came_ext(lam, &b, &c, &n, &a);
  camc (c,n,0,10);
}

/*--ESONE General functions--------------------------------------*/
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

/*-----------------------------------------------------------------------------
 *  Copyright (c) 1998      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 * 
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca           Tel: (604) 222-1047  Fax: (604) 222-1074
 *         amaudruz@triumf.ca
 * -----------------------------------------------------------------------------
 *  
 *  Description	: CAMAC interface for CES8210 CAMAC controller using
 *		  Midas Camac Standard calls. includes the interrupt handling.
 *		  No reference to Midas.
 *                interrupt level 2 INT2 at vector  85  <---- not implemented
 *                interrupt level 4 INT4 at vector 170  <---- current setting
 *  Requires 	: vxWorks lib calls for the interrupts
 *  Application : VME + VxWorks
 *  Author      : Pierre-Andre Amaudruz Data Acquisition Group
 *  Revision    : 1.0  1998        Pierre	 Initial revision
 *
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

#include "mcstd.h"

#ifdef OS_VXWORKS
#include "vme.h"
#include "vxWorks.h"
#include "intLib.h"
#include "sys/fcntlcom.h"
#include "arch/mc68k/ivMc68k.h"
#include "taskLib.h"
#endif

/* Control and Status Register */
#define IT4	0x0001                          /* front panel int IT4 status */
#define IT2	0x0002	                        /* front panel int IT2 status */
#define MIT4	0x0004	                        /* IT4 mask 0 = enabled */
#define MIT2	0x0008		                /* IT2 mask 0 = enabled */
#define MLAM    0x0010				/* Mask LAM 0 = enabled */

/* Hardware setting dependent */
#define INT_SOURCE_FRONT           2             /*  level  4 */
#define INT_SOURCE_VECTOR         85             /*    170 */
#define INT_SOURCE_MASK         MIT2             /*   MIT4 */
#define BASE	       0x00800000	       /* camac base address */

/* VME CAMAC CBD8210 base address */
#define BASE	       0x00800000	       /* camac base address */
#define A32D24	       0xf0000000              /* A32D24 camac base address */
#define A32D16	       0xf0000002              /* A32D16 camac base address */
#define CSR_OFFSET     0x0000e800              /* camac control and status reg */
#define CAR_OFFSET     0x0000e820              /* Crate Address Register */
#define BTB_OFFSET     0x0000e824              /* on-line status (R) */
#define IFR_OFFSET     0x0080e810  /* interrupt flag register */
#define ICR1_C         0x0080e814  /* interrupt control reg 1: Control */
#define ICR1_D         0x0080e804  /* interrupt control reg 1: data */
#define ICR2_C         0x0080e818  /* interrupt control reg 2: Control */
#define ICR2_D         0x0080e808  /* interrupt control reg 2: data */
#define ICR3_C         0x0080e81c  /* interrupt control reg 3: Control */
#define ICR3_D         0x0080e80c  /* interrupt control reg 3: data */
#define BZ	       0x0080e824  /* reset camac branch BZ (W) */

#define GL	       0x0080e828  /* GL Register */

#define GLR     BASE |     GL     | A32D24
#define CSR     BASE | CSR_OFFSET | A32D16
#define CAR     BASE | CAR_OFFSET | A32D16
#define BTB     BASE | BTB_OFFSET | A32D16
#define IFR     BASE | IFR_OFFSET | A32D16

#define    cbd_bas_ladr     0xf0800000
#define    am_reset         0        /* clear all IRR and all IMR bits */
#define    am_clirr         0x40     /* clear all IRR  */
#define    am_clirrb        0x48     /* clear IRR bit (0,1,2) */
#define    am_clirrmrb      0x18     /* clear IRR and IMR bit (0,1,2) */
#define    am_clisrb        0x78     /* clear ISR bit (0,1,2)  */
#define    am_load04        0x80     /* load mode bits 0-4 */
#define    am_load56        0xa1     /* load mode bits 5,6 set bit 7 */
#define    am_rsmem         0xe0     /* load bits 3,4 into byte count reg. and
                                        select response memory(bits 0,1,2) */ 
#define    am_sirr          0x58     /* set IRR bits (0,1,2)  */
#define    am_stimrb        0x38     /* set single imr bit   */
#define    am_prio          6
#define    am_vect          1
#define    am_irm           2
#define    am_gint          3
#define    am_ireq          4
#define    am_md_ini        1<<am_ireq

/* stub for ISR restoration */
/* extern excStub; */

/*--Macros---------------------------------------------------------*/
#define CAM_CSRCHK(_w){\
			 DWORD cbd_csr = CSR;\
			 *_w = *(WORD *) cbd_csr;}

#define CAM_CARCHK(_w){\
			 DWORD cbd_car = CAR;\
			 *_w = *(WORD *) cbd_car;}

#define CAM_BTBCHK(_w){\
			 DWORD cbd_btb = BTB;\
			 *_w = *(WORD *) cbd_btb;}

#define CAM_QXCHK(_q,_x){\
			 DWORD cbd_csr = CSR;\
			 WORD csr = *((WORD *) cbd_csr);\
			 *_q = ((csr & 0x8000)>>15);\
			 *_x = ((csr & 0x4000)>>14);}
			 
#define CAM_GLCHK(_w){\
			 DWORD cbd_gl = GLR;\
			 *_w = *(DWORD *) cbd_gl;}

#define CAM_QCHK(_q){\
			 DWORD cbd_csr = CSR;\
			 WORD csr = *((WORD *) cbd_csr);\
			 *_q = ((csr & 0x8000)>>15);}
#define CAM_XCHK(_x){\
			 DWORD cbd_csr = CSR;\
			 WORD csr = *((WORD *) cbd_csr);\
			 *_x = ((csr & 0x4000)>>14);}
/*-----------------------------------------------------------------*/

/*-input---------------------------------------------------------*/
INLINE void cam16i(const int c, const int n, const int a, const int f, WORD *d)
{
  volatile WORD * ext;
  
  ext = (WORD *)(A32D16 | BASE | c<<16 | n<<11 | a<<7 | f<<2);
  *d = *ext;
}

/*--input--------------------------------------------------------*/
INLINE void cam24i(const int c, const int n, const int a, const int f, DWORD *d)
{
  volatile DWORD ext;
  
  ext = A32D24 | BASE | c<<16 | n<<11 | a<<7 | f<<2;
  *d = *((DWORD *) ext);
}

/*--input--------------------------------------------------------*/
INLINE void cam16i_q(const int c, const int n, const int a, const int f, WORD *d, int *x, int *q)
{
  cam16i(c,n,a,f, d);
  CAM_QXCHK(q, x);
}

/*--input--------------------------------------------------------*/
INLINE void cam24i_q(const int c, const int n, const int a, const int f, DWORD *d, int *x, int *q)
{
  cam24i(c,n,a,f, d);
  CAM_QXCHK(q, x);
}

/*--input--------------------------------------------------------*/
INLINE void cam16i_r(const int c, const int n, const int a, const int f, WORD **d, const int r)
{
  static DWORD ext;
  static int i;

  ext = A32D16 | BASE | (c<<16) | (n<<11) | (a<<7) |(f<<2);
  for (i=0;i<r;i++)
    *((*d)++) = *((WORD *) ext);
}
/*--input--------------------------------------------------------*/
INLINE void cam24i_r(const int c, const int n, const int a, const int f, DWORD **d, const int r)
{
  static DWORD ext;
  static int i;
  
  ext = A32D24 | BASE | (c<<16) | (n<<11) | (a<<7) | (f<<2);
  for (i=0;i<r;i++)
    *((*d)++) = *((DWORD *) ext);
}

/*--input--------------------------------------------------------*/
INLINE void cam16i_rq(const int c, const int n, const int a, const int f, WORD **d, const int r)
{
  static int   i, q;
  static DWORD ext;
  static WORD  dtemp;
  
  ext = A32D16 | BASE | (c<<16) | (n<<11) | (a<<7) |(f<<2);
  for (i=0;i<r;i++)
  {
    dtemp = *((WORD *) ext);
    CAM_QCHK (&q);
    if (q)
      *((*d)++) = dtemp;
    else
      break;
  }
}

/*--input--------------------------------------------------------*/
INLINE void cam24i_rq(const int c, const int n, const int a, const int f, DWORD **d, const int r)
{
  static int   i, q;
  static DWORD ext;
  static DWORD dtemp;
  
  ext = A32D24 | BASE | (c<<16) | (n<<11) | (a<<7) |(f<<2);
  for (i=0;i<r;i++)
  {
    dtemp = *((DWORD *) ext);
    CAM_QCHK (&q);
    if (q)
      *((*d)++) = dtemp;
    else
      break;
  }
}

/*--input--------------------------------------------------------*/
INLINE void cam16i_sa(const int c, const int n, const int a, const int f, WORD **d, const int r)
{
  static DWORD ext;
  static int i, aa;
  
  
  ext = A32D16 | BASE | (c<<16) | (n<<11) | (f<<2);
  for (aa=a, i=0; i<r; i++, aa++) {
    ext = ((ext & ~(0xf << 7)) | (aa<<7));
    *((*d)++) = *((WORD *) ext);
  }
}

/*--input--------------------------------------------------------*/
INLINE void cam24i_sa(const int c, const int n, const int a, const int f, unsigned long **d, const int r)
{
  static unsigned long ext;
  static i, aa;  
  ext = A32D24 | BASE | (c<<16) | (n<<11) |(f<<2);
  for (aa=a, i=0; i<r; i++, aa++) {
    ext = ((ext & ~(0xf << 7)) | (aa<<7));
    *((*d)++) = *((DWORD *) ext);
  }
}

/*--input--------------------------------------------------------*/
INLINE void cam16i_sn(const int c, const int n, const int a, const int f, WORD **d, const int r)
{
  static DWORD ext;
  static nn,i;
  
  ext = A32D16 | BASE | (c<<16) | (a<<7) |(f<<2);
  for (nn=n, i=0; i<r; i++, nn++) {
    ext = ((ext & ~(0x1f << 11)) | (nn<<11));
    *((*d)++)= *((WORD *) ext);
  }
}

/*--input--------------------------------------------------------*/
INLINE void cam24i_sn(const int c, const int n, const int a, const int f, DWORD **d, const int r)
{
  static DWORD ext;
  static int i, nn;
  
  ext = A32D24 | BASE | (c<<16) | (a<<7) |(f<<2);
  for (nn=n,i=0; i<r; i++, nn++) {
    ext = ((ext & ~(0x1f << 11)) | (nn<11));
    *((*d)++) = *((DWORD *) ext);
  }
}

/*--input--------------------------------------------------------*/
INLINE void cami(const int c, const int n, const int a, const int f, WORD *d)
{
  cam16i(c,n,a,f, d);
}

/*--output-------------------------------------------------------*/
INLINE void cam16o(const int c, const int n, const int a, const int f, WORD d)
{
  static DWORD ext;
  
  ext = A32D16 | BASE | (c<<16) | (n<<11) | (a<<7) |(f<<2);
  *((WORD *) ext) = d;
}

/*--output-------------------------------------------------------*/
INLINE void cam24o(const int c, const int n, const int a, const int f, DWORD d)
{
  static DWORD ext;
  
  ext = A32D24 | BASE | (c<<16) | (n<<11) | (a<<7) |(f<<2);
  *((DWORD *) ext) = d;
}

/*--output-------------------------------------------------------*/
INLINE void cam16o_q(const int c, const int n, const int a, const int f, WORD d, int *x, int *q)
{
  cam16o (c,n,a,f,d);
  CAM_QXCHK (q,x);
}
/*--output-------------------------------------------------------*/
INLINE void cam24o_q(const int c, const int n, const int a, const int f, DWORD d, int *x, int *q)
{
  cam24o (c,n,a,f,d);
  CAM_QXCHK (q,x);
}
/*--output-------------------------------------------------------*/
INLINE void camo(const int c, const int n, const int a, const int f, WORD d)
{
  cam16o (c,n,a,f,d);
}

/*--command------------------------------------------------------*/
INLINE int camc_chk(const int c)
{
  volatile int crate_status;
  
  CAM_BTBCHK (&crate_status);
  if ( ((crate_status >> c) & 0x1) != 1)
	return -1;
  return 0;
}

/*--command------------------------------------------------------*/
INLINE void camc(const int c, const int n, const int a, const int f)
{
  volatile DWORD ext;
  volatile WORD dtemp;
  
  ext = A32D16 | BASE | (c<<16) | (n<<11) | (a<<7) |(f<<2);
  dtemp = *((WORD *) ext);
}

/*--command------------------------------------------------------*/
INLINE void camc_q(const int c, const int n, const int a, const int f, int *q)
{
  camc (c,n,a,f);
  CAM_QCHK (q);
}
/*--command------------------------------------------------------*/
INLINE void camc_sa(const int c, const int n, const int a, const int f, const int r)
{
  static DWORD ext;
  static WORD dtemp;
  static int i,aa;
  
  ext = A32D16 | BASE | (c<<16) | (n<<11) | (f<<2);
  for (aa=a, i=0; i<r; i++, aa++) {
    ext = ((ext & ~(0xf << 7)) | (aa<<7));
    dtemp = *((WORD *) ext);
  }
}
/*--command------------------------------------------------------*/
INLINE void camc_sn(const int c, const int n, const int a, const int f, const int r)
{
  static DWORD ext;
  static WORD dtemp;
  static int nn,i;
  
  ext = A32D16 | BASE | (c<<16) | (a<<7) | (f<<2);
  for (nn=n, i=0; i<r; i++, nn++) {
    ext = ((ext & ~(0x1f << 11)) | (nn<<11));
    dtemp = *((WORD *) ext);
  }
}

/*--General functions--------------------------------------------*/
INLINE int cam_init(void)
{
  /* nothing to do for CES8210 */
  return 0;
}
/*--General functions--------------------------------------------*/
INLINE void cam_exit(void)
{
  /* nothing to do for CES8210 */
}
/*--General functions--------------------------------------------*/
INLINE void cam_inhibit_set(const int c)
{
  camc (c,30,9,26);
}

/*--General functions--------------------------------------------*/
INLINE void cam_inhibit_clear(const int c)
{
  camc (c,30,9,24);
}

/*--General functions--------------------------------------------*/
INLINE void cam_crate_clear(const int c)
{
  camc (c,28,9,26);
}

/*--General functions--------------------------------------------*/
INLINE void cam_crate_zinit(const int c)
{
  camc (c,28,8,26);
}

/*--General functions--------------------------------------------*/
INLINE void cam_lam_enable(const int c, const int n)
{
  camc (c,n,0,26);
}

/*--General functions--------------------------------------------*/
INLINE void cam_lam_disable(const int c, const int n)
{
  camc (c,n,0,24);
}

/*--General functions--------------------------------------------*/
INLINE void cam_lam_read(const int c, DWORD *lam)
{
  CAM_GLCHK(lam);
}

/*--General functions--------------------------------------------*/
INLINE void cam_lam_clear(const int c, const int n)
{
  camc (c,n,0,10);
}

/*--external-----------------------------------------------------*/
INLINE void came_cn(int *ext, const int b, const int c, const int n, const int a)
{
  *ext = A32D24 | BASE | (b<<19) | (c<<16) | (n<<11) | (a<<7);
}

/*--external-----------------------------------------------------*/
INLINE void came_ext(const int ext, int *b, int *c, int *n, int *a)
{
  *b = (ext >> 19) & 0x7;
  *c = (ext >> 16) & 0xf;
  *n = (ext >> 11) & 0x1f;
  *a = (ext >>  7) & 0xf;
}

/*--external-----------------------------------------------------*/
INLINE void cam16ei(int ext, const int f, WORD *d)
{
  static int b,c,n,a;
  came_ext(ext, &b, &c, &n, &a);
  cam16i(c,n,a,f,d);
/*
  ext &=  ~(0x3f);                  
  ext |= ((f << 2) | 0x2);
  *d = *((WORD *) ext);
  */
}

/*--external-----------------------------------------------------*/
INLINE void cam24ei(int ext, const int f, DWORD *d)
{
  static int b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  cam24i(c,n,a,f,d);
}

/*--external-----------------------------------------------------*/
INLINE void cam16ei_q(const int ext, const int f, WORD *d, int *x, int *q)
{
  cam16ei (ext,f,d);
  CAM_QXCHK (q,x);
}

/*--external-----------------------------------------------------*/
INLINE void cam24ei_q(const int ext, const int f, DWORD *d, int *x, int *q)
{
  cam24ei (ext,f,d);
  CAM_QXCHK (q,x);
}

/*--external-----------------------------------------------------*/
INLINE void cam16ei_r(int ext, const int f, WORD **d, const int r)
{
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++)
    cam16i(c,n,a,f,(*d)++);
}

/*--external-----------------------------------------------------*/
INLINE void cam24ei_r(int ext, const int f, DWORD **d, const int r)
{
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++)
    cam24i(c,n,a,f,(*d)++);
}

/*--external-----------------------------------------------------*/
INLINE void cam16ei_rq(int ext, const int f, WORD **d, const int r)
{
  static WORD dtemp;
  static int i,b,c,n,a,q,x;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++){
    cam16i_q(c,n,a,f,&dtemp,&x,&q);
    if (q)
		  *((*d)++) = dtemp;
    else
		  break;
  }
}
/*--external-----------------------------------------------------*/
INLINE void cam24ei_rq(int ext, const int f, DWORD **d, const int r)
{
  static DWORD dtemp;
  static int i,b,c,n,a,q,x;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++){
    cam24i_q(c,n,a,f,&dtemp,&x,&q);
    if (q)
		  *((*d)++) = dtemp;
    else
		  break;
  } 
}

/*--external-----------------------------------------------------*/
INLINE void cam16ei_sa(int ext, const int f, WORD **d, const int r)
{
  static WORD dtemp;
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++){
    cam16i(c,n,a++,f,(*d)++);
  }
}

/*--external-----------------------------------------------------*/
INLINE void cam24ei_sa(int ext, const int f, DWORD **d, const int r)
{
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++){
    cam24i(c,n,a++,f,(*d)++);
  }
}

/*--external-----------------------------------------------------*/
INLINE void cam16ei_sn(int ext, const int f, WORD **d, const int r)
{
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++){
    cam16i(c,n++,a,f,(*d)++);
  }
}

/*--external-----------------------------------------------------*/
INLINE void cam24ei_sn(int ext, const int f, DWORD **d, const int r)
{
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++){
    cam24i(c,n++,a,f,(*d)++);
  }
}

/*--external-----------------------------------------------------*/
INLINE void cam16eo(int ext, const int f, WORD d)
{
  static int b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  cam16o(c,n,a,f,d);
}

/*--external-----------------------------------------------------*/
INLINE void cam24eo(int ext, const int f, DWORD d)
{
  static int b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  cam24o(c,n,a,f,d);
}

/*--external-----------------------------------------------------*/
INLINE void cam16eo_q(const int ext, const int f, WORD d, int *x, int *q)
{
  static int b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  cam16o_q(c,n,a,f,d,x,q);
}

/*--external-----------------------------------------------------*/
INLINE void cam24eo_q(const int ext, const int f, DWORD d, int *x, int *q)
{
  static int b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  cam24o_q(c,n,a,f,d,x,q);
}

/*--external-----------------------------------------------------*/
INLINE void camec(int ext, const int f)
{
  static int b,c,n,a;
  static WORD dtemp;

  came_ext(ext, &b, &c, &n, &a);
  cam16i(c,n,a,f,&dtemp);
}

/*--external-----------------------------------------------------*/
INLINE void camec_q(int ext, const int f, int *x, int *q)
{
  static int b,c,n,a;
  static WORD dtemp;

  came_ext(ext, &b, &c, &n, &a);
  cam16i_q(c,n,a,f,&dtemp,x,q);
}

/*--external-----------------------------------------------------*/
INLINE void camec_sa(int ext, const int f, const int r)
{
  static WORD dtemp;
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++)
    cam16i(c,n,a++,f,&dtemp);
}

/*--external-----------------------------------------------------*/
INLINE void camec_sn(int ext, const int f, const int r)
{
  static WORD dtemp;
  static int i,b,c,n,a;

  came_ext(ext, &b, &c, &n, &a);
  for (i=0; i<r; i++)
    cam16i(c,n++,a,f,&dtemp);
}

/*--Interrupt functions------------------------------------------*/
INLINE void cam_interrupt_enable(void)
{
  /* For now interrupt source can only be coming from INTx */

  static DWORD cbd_csr_r, cbd_ifr;
  static WORD csr;
  
  cbd_csr_r= CSR;	       	/* address: camac control/status reg */
  csr = *((WORD *) cbd_csr_r);  /* read csr */
  csr &= ~INT_SOURCE_MASK;                 /* mask off intx */
  *(volatile WORD *)cbd_csr_r = csr;     /* */
#ifdef OS_VXWORKS
  sysIntEnable(INT_SOURCE_FRONT);	        /* interrupt level x */
#endif
}

/*--Interrupt functions------------------------------------------*/
INLINE void cam_interrupt_disable(void)
{
  static DWORD cbd_csr_r;
  static WORD csr;

  cbd_csr_r= CSR;		/* address: camac control/status reg */
  csr = *(WORD *)cbd_csr_r;	/* read csr */
  csr = csr | INT_SOURCE_MASK;             /* mask off intx */
  *(volatile WORD *)cbd_csr_r = csr;     /* */
#ifdef OS_VXWORKS
  sysIntDisable(INT_SOURCE_FRONT);	        /* interrupt level x */
#endif
}

/*--Interrupt functions------------------------------------------*/
INLINE void cam_interrupt_attach(void (*isr)(void))
{
  static DWORD cbd_ifr;
  
  cbd_ifr = IFR;		/* address: interrupt flag register */
  *(volatile WORD *)cbd_ifr = 0;         /* clear Interrupt register */
  cam_interrupt_disable();	/* mask 8210 external interrupt x */
#ifdef OS_VXWORKS
  sysIntDisable(INT_SOURCE_FRONT);          	/* disable bus interrupt level x */
  intConnect((VOIDFUNCPTR *) (INT_SOURCE_VECTOR<<2),(VOIDFUNCPTR)isr,INT_SOURCE_VECTOR);
#endif
}

/*--Interrupt functions------------------------------------------*/
INLINE void cam_interrupt_detach(void)
{
  static DWORD cbd_ifr;
  
  cbd_ifr = IFR;		/* address: interrupt flag register */
  *(volatile WORD *)cbd_ifr = 0;         /* clear Interrupt register */
  cam_interrupt_disable();	/* mask 8210 external interrupt x */
#ifdef OS_VXWORKS
  sysIntDisable(INT_SOURCE_FRONT);	        /* disable bus interrupt level x */
  intConnect((VOIDFUNCPTR *) (INT_SOURCE_VECTOR<2),(VOIDFUNCPTR) excStub,INT_SOURCE_VECTOR);
#endif
}
/*---------------------------------------------------------------*/

/*--Interrupt functions------------------------------------------*/
INLINE void cam_glint_enable(void)
{
  /* !!!!!!!!!!!! not yet working !!!!!!!!!!!!!!!!! */
  /* For now interrupt source can only be coming from INT2 */

  static DWORD cbd_csr_r, cbd_ifr;
  static WORD csr;
  
  cbd_csr_r= CSR;	       	/* address: camac control/status reg */
  csr = *((WORD *) cbd_csr_r);  /* read csr */
  csr &= ~MLAM;                 /* mask off Lam */
  *(WORD *)cbd_csr_r = csr;     /* */
#ifdef OS_VXWORKS
  sysIntEnable(3);	        /* interrupt level 3 */
#endif
}

/*--Interrupt functions------------------------------------------*/
INLINE void cam_glint_disable(void)
{
  /* !!!!!!!!!!!! not yet working !!!!!!!!!!!!!!!!! */

  static DWORD cbd_csr_r;
  static WORD csr;

  cbd_csr_r= CSR;		/* address: camac control/status reg */
  csr = *(WORD *)cbd_csr_r;	/* read csr */
  csr = csr | MLAM;             /* mask off Lam */
  *(WORD *)cbd_csr_r = csr;     /* */
#ifdef OS_VXWORKS
  sysIntDisable(3);	        /* interrupt level 3 */
#endif
}

/*--Interrupt functions------------------------------------------*/
INLINE void cam_glint_attach(int lam, void (*isr)())
{
  /* !!!!!!!!!!!! not yet working !!!!!!!!!!!!!!!!! */

  int      l, xvect = 200;
  unsigned  short *reg1;
  int      slot,b;
  int      cbd_icr_lam;
  
  {
    l = 1;
    slot = (lam>>11)&31;
    b = (lam>>19)&7;
    cbd_icr_lam = (cbd_bas_ladr|b<<19|29<<11|5<<2|2);
    reg1 = (WORD *)cbd_icr_lam;    
    while (slot > 7)
      {
	cbd_icr_lam += 4;            /*  7< slot <= 15 use INT_CONT.2 */
	                             /* 15< slot <= 23 use INT_CONT.3 */	
	slot -= 8;
      }

    *reg1 = am_clisrb | slot;	     /* clear ISR bits  */	

    if (l==1)                        /* enable single internal interrupt */
      {
	*reg1 = am_clirrmrb | slot;  /* clear IRR and IMR bits (0,1,2) */ 
	/*	*reg1 = am_sirr | slot;        set IRR bits  */
      }
    else
      {
	*reg1 = am_stimrb | slot;    /* set IMR bits (0,1,2)  */
      }
  } 
#ifdef OS_VXWORKS
  sysIntDisable(3);          	/* disable bus interrupt level 3 */
  intConnect((VOIDFUNCPTR *) (xvect<<2), (VOIDFUNCPTR) isr,0);
#endif
}

/*-Tests---------------------------------------------------------*/
void ces8210(void)
{
  printf("\n---> CES 8210 CAMAC Branch Driver (ces8210.c)<---\n");
  printf("Macro  : CAM_CSRCHK CAM_CARCHK CAM_BTBCHK CAM_QXCHK\n");
  printf("Macro  : CAM_GLCHK CAM_QCHK CAM_XCHK\n");
  printf("Inline : All mcstd functions\n");
  printf("Test   : camop()                <--- -> CSR, CAR, BTB\n");
  printf("Test   : camE_ext(ext)          <--- ext -> b,c,n,a\n");
  printf("Test   : camC(c,n,a,f)          <--- cmd -> c,n,a,f\n");
  printf("Test   : camI(c,n,a,f)   (16)   <--- c,n,a,f -> d\n");
  printf("Test   : camO(c,n,a,f,d) (16)   <--- d -> c,n,a,f\n");
  printf("Test   : camI24(c,n,a,f)        <--- c,n,a,f -> d\n");
  printf("Test   : camO24(c,n,a,f,d)      <--- d -> c,n,a,f\n");
  printf("Test   : camI24r(c,n,a,f,r)     <--- c,n,a,f,r -> d\n");
  printf("Test   : camI24sa(c,n,a,f,r)    <--- c,n,a++,f,r -> d\n");
  printf("Test   : attach_interrupt()     <--- attach, enable Int2\n");
}

void tint (void)
{
  static int private_counter=0;
  logMsg ("%tint external interrupt #%i received and served\n",private_counter++,0,0,0,0,0);
}

void attach_interrupt(void)
{
  cam_interrupt_attach(tint);
  cam_interrupt_enable();
  printf("Please generate an interrupt on FP INT2");
}

void csr(void)
{
  WORD online;

  CAM_CSRCHK(&online);
  printf("CSR Camac  status 0x%4.4x\n",online);
}

void car(void)
{
  WORD online;

  CAM_CARCHK(&online);
  printf("CAR Online status 0x%4.4x\n",online);
}

void btb(void)
{
  WORD online;

  CAM_BTBCHK(&online);
  printf("BTB Online status 0x%4.4x\n",online);
}

void glr(void)
{
  DWORD online;

  CAM_GLCHK(&online);
  printf("GL  reg.   status 0x%6.6x\n",online);
}

void camop(void)
{
  csr();
  car();
  btb();
  glr();
}

void camE_ext(int ext)
{
  int b,c,n,a;
  came_ext(ext,&b,&c,&n,&a);
  printf("ext:0x%x -> b:%2i c:%2i n:%2i a:%2i\n",ext,b,c,n,a);
}

void camI(int c, int n, int a, int f)
{
  WORD d;
  cami(c,n,a,f,&d);
  printf("c:%2i n:%2i a:%2i f:%2i -> d:0x%4x (%i)\n",c,n,a,f,d,d);
}

void camO(int c, int n, int a, int f, int d)
{
  camo(c,n,a,f,(WORD)d);
}

void camC(int c, int n, int a, int f)
{
  camc(c,n,a,f);
}

void camI24(int c, int n, int a, int f)
{
  DWORD d;
  cam24i(c,n,a,f,&d);
  printf("c%1i n%2.2i a%2.2i f%2.2i -> d:0x%6x (%i)\n",c,n,a,f,d,d);
}

void camO24(int c, int n, int a, int f, int d)
{
  camo(c,n,a,f,(DWORD)d);
}

void camI24r(int c, int n, int a,int f, int r)
{
  DWORD  *pd, *pdata;
  int i;

  pdata = pd = (DWORD *)malloc(sizeof(DWORD)*r);
  cam24i_r(c,n,a,f, (DWORD **)&pdata, r);
  pdata = pd;
  for (i=0;i<r;i++)
    {
      printf("[%i] c%1i n%2.2i a%2.2i f%2.2i -> d:0x%6.6x (%i)\n",i,c,n,a,f,*pdata,*pdata);
      pdata++;
    }
free (pd);   
}

void camI24sa(int c, int n, int a,int f, int r)
{
  DWORD  *pd, *pdata;
  int i;

  pdata = pd = (DWORD *)malloc(sizeof(DWORD)*r);
  cam24i_sa(c,n,a,f, (DWORD **)&pdata, r);
  pdata = pd;
  for (i=0;i<r;i++)
    {
      printf("[%i] c%1i n%2.2i a%2.2i f%2.2i -> d:0x%6.6x (%i)\n",i,c,n,a++,f,*pdata,*pdata);
      pdata++;
    }
free (pd);   
}

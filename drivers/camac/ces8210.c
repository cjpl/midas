/********************************************************************\
  Name:         ces8210.c
  Created by:   P.-A. Amaudruz

  Contents:     CES CBD 8210 CAMAC Branch Driver 
                Following mvmestd for mcstd
                Based on the original CES8210 for VxWorks
                Initialy ported to VMIC by Chris Pearson
                Modified to follow the latest mvmestd by PAA
  $Id$
\********************************************************************/
#ifndef INLINE
#if defined( _MSC_VER )
#define INLINE __inline
#elif defined(__GNUC__)
#define INLINE __inline__
#else
#define INLINE
#endif
#endif

#include <stdio.h>
#include <string.h>

#include "mcstd.h"       /* Provides CAMAC access */
#include "mvmestd.h"     /* Provides VME interface */
#include "ces8210.h"    /* Provides CBD declaration */

#ifdef OS_VXWORKS
#include "vxVME.h"
#else
#include "vmicvme.h"
#endif

MVME_INTERFACE *myvme;

#define CAM_CSRCHK(_w){\
          cam16i(0, 29, 0, 0, &_w);}

#define CAM_CARCHK(_w){\
          cam16i(0, 29, 0, 8, &_w);}

#define CAM_BTBCHK(_w){\
          cam16i(0, 29, 0, 9, &_w);}

#define CAM_GLCHK(_w){\
          cam24i(0, 29, 0, 10, &_w);}

/*****************************************************************\
*                                                                 *
* Input functions                                                 *
*                                                                 *
\*****************************************************************/
/*---------------------------------------------------------------*/
INLINE void cam16i(const int c, const int n, const int a, const int f, WORD * d)
{
  int ext, cmode;
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D16);
  ext = (CBDWL_D16 + (c << 16)  + (n << 11) + (a << 7) + (f << 2));
  *d = mvme_read_value(myvme, CBD8210_BASE + ext);
  mvme_set_dmode(myvme, cmode);
}

/*---------------------------------------------------------------*/
INLINE void cam24i(const int c, const int n, const int a, const int f, DWORD * d)
{
  int ext, cmode;
  WORD dh, dl;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D16);
  ext = (CBDWL_D24 + (c << 16) + (n << 11) + (a << 7) + (f << 2));
  dh = mvme_read_value(myvme, CBD8210_BASE+ext);
  dl = mvme_read_value(myvme, CBD8210_BASE+ext+2);
  *d = ((dh << 16) | dl) & 0xffffff;
  mvme_set_dmode(myvme, cmode);
}

/*---------------------------------------------------------------*/
INLINE void cam16i_q(const int c, const int n, const int a, const int f,
                     WORD * d, int *x, int *q)
{
   WORD csr;

   cam16i(c, n, a, f, d);
   cami(0, 29, 0, 0, &csr);
   *x = ((csr & 0x4000) != 0) ? 1 : 0;
   *q = ((csr & 0x8000) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------*/
INLINE void cam24i_q(const int c, const int n, const int a, const int f,
                     DWORD * d, int *x, int *q)
{
   WORD csr;

   cam24i(c, n, a, f, d);
   cami(0, 29, 0, 0, &csr);
   *x = ((csr & 0x4000) != 0) ? 1 : 0;
   *q = ((csr & 0x8000) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------*/
INLINE void cam16i_r(const int c, const int n, const int a, const int f,
                     WORD ** d, const int r)
{
   int i;

   for (i = 0; i < r; i++)
      cam16i(c, n, a, f, (*d)++);
}

/*---------------------------------------------------------------*/
INLINE void cam24i_r(const int c, const int n, const int a, const int f,
                     DWORD ** d, const int r)
{
   int i;

   for (i = 0; i < r; i++)
      cam24i(c, n, a, f, (*d)++);
}

/*---------------------------------------------------------------*/
INLINE void cam16i_rq(const int c, const int n, const int a, const int f,
                      WORD ** d, const int r)
{
   WORD dtemp;
   WORD csr, i;

   for (i = 0; i < r; i++) {
      cam16i(c, n, a, f, &dtemp);
      cami(0, 29, 0, 0, &csr);
      if ((csr & 0x8000) != 0)
         *((*d)++) = dtemp;
      else
         break;
   }
}

/*---------------------------------------------------------------*/
INLINE void cam24i_rq(const int c, const int n, const int a, const int f,
                      DWORD ** d, const int r)
{
   DWORD i, dtemp;
   WORD csr;

   for (i = 0; i < (DWORD) r; i++) {
      cam24i(c, n, a, f, &dtemp);
      cami(0, 29, 0, 0, &csr);
      if ((csr & 0x8000) != 0)
         *((*d)++) = dtemp;
      else
         break;
   }
}

/*---------------------------------------------------------------*/
INLINE void cam16i_sa(const int c, const int n, const int a, const int f,
                      WORD ** d, const int r)
{
   int aa;

   for (aa = a; aa < a + r; aa++)
      cam16i(c, n, aa, f, (*d)++);
}

/*--input--------------------------------------------------------*/
INLINE void cam24i_sa(const int c, const int n, const int a, const int f,
                      DWORD **d, const int r)
{
   int aa;

   for (aa = a; aa < a + r; aa++)
      cam24i(c, n, aa, f, (*d)++);
}

/*---------------------------------------------------------------*/
INLINE void cam16i_sn(const int c, const int n, const int a, const int f,
                      WORD ** d, const int r)
{
   int nn;

   for (nn = n; nn < n + r; nn++)
      cam16i(c, nn, a, f, (*d)++);
}

/*---------------------------------------------------------------*/
INLINE void cam24i_sn(const int c, const int n, const int a, const int f,
                      DWORD ** d, const int r)
{
   int nn;

   for (nn = n; nn < n + r; nn++)
      cam24i(c, nn, a, f, (*d)++);
}

/*---------------------------------------------------------------*/
INLINE void cami(const int c, const int n, const int a, const int f, WORD * d)
{
   cam16i(c, n, a, f, d);
}

/*****************************************************************\
*                                                                 *
* Output functions                                                *
*                                                                 *
\*****************************************************************/
/*---------------------------------------------------------------*/
INLINE void cam16o(const int c, const int n, const int a, const int f, WORD d)
{
  int ext, cmode;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D16);
  ext = (CBDWL_D16 + (c << 16)  + (n << 11) + (a << 7) + (f << 2));
  mvme_write_value(myvme, CBD8210_BASE+ext, d);
  mvme_set_dmode(myvme, cmode);
}

/*---------------------------------------------------------------*/

INLINE void cam24o(const int c, const int n, const int a, const int f, DWORD d)
{
  static int ext, cmode;
  static WORD dtemp;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D16);
  ext = (CBDWL_D24 + (c << 16)  + (n << 11) + (a << 7) + (f << 2));
  dtemp = (unsigned short) ((d >> 16) & 0xffff);
  mvme_write_value(myvme, CBD8210_BASE+ext, dtemp);
  dtemp = (unsigned short) (d & 0xffff);
  mvme_write_value(myvme, CBD8210_BASE+ext+2, dtemp);
  mvme_set_dmode(myvme, cmode);
}

/*---------------------------------------------------------------*/
INLINE void cam16o_q(const int c, const int n, const int a, const int f,
                     WORD d, int *x, int *q)
{
   WORD csr;

   cam16o(c, n, a, f, d);
   cami(0, 29, 0, 0, &csr);
   *x = ((csr & 0x4000) != 0) ? 1 : 0;
   *q = ((csr & 0x8000) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------*/
INLINE void cam24o_q(const int c, const int n, const int a, const int f,
                     DWORD d, int *x, int *q)
{
   static WORD csr;

   cam24o(c, n, a, f, d);
   cami(0, 29, 0, 0, &csr);
   *x = ((csr & 0x4000) != 0) ? 1 : 0;
   *q = ((csr & 0x8000) != 0) ? 1 : 0;
}

/********************************************************************/
void cam16o_r(const int c, const int n, const int a, const int f, WORD * d, const int r)
{
  int i;
  for (i=0; i<r; i++)
    cam16o(c,n,a,f,d[i]);
}

/********************************************************************/
void cam24o_r(const int c, const int n, const int a, const int f, DWORD * d, const int r)
{
  int i;
  for (i=0; i<r; i++)
    cam24o(c,n,a,f,d[i]);
}

/*---------------------------------------------------------------*/
INLINE void camo(const int c, const int n, const int a, const int f, WORD d)
{
   cam16o(c, n, a, f, d);
}

/*---------------------------------------------------------------*/
int cam_init_rpc(char *host_name, char *exp_name, char *fe_name,
                 char *client_name, char *rpc_server)
{
  /* dummy routine for compatibility */
  return 1;
}

/*****************************************************************\
*                                                                 *
* Command functions                                               *
*                                                                 *
\*****************************************************************/
/*---------------------------------------------------------------*/
INLINE int camc_chk(const int c)
{
   WORD crate_status;

   CAM_BTBCHK(crate_status);
   if (((crate_status >> c) & 0x1) != 1)
      return -1;
   return 0;
}

/*---------------------------------------------------------------*/
INLINE void camc(const int c, const int n, const int a, const int f)
{
   /* Following the CBD8210 manual */
   WORD dtmp;

   if (f < 16)
      cam16i(c, n, a, f, &dtmp);
   else if (f < 24)
      cam16o(c, n, a, f, 0);
   else
      cam16i(c, n, a, f, &dtmp);
}

/*---------------------------------------------------------------*/
INLINE void camc_q(const int c, const int n, const int a, const int f, int *q)
{
   WORD csr;

   camc(c, n, a, f);
   cami(0, 29, 0, 0, &csr);
   *q = ((csr & 0x8000) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------*/
INLINE void camc_sa(const int c, const int n, const int a, const int f, const int r)
{
   int aa;

   for (aa = a; aa < a + r; aa++)
      camc(c, n, aa, f);
}

/*---------------------------------------------------------------*/
INLINE void camc_sn(const int c, const int n, const int a, const int f, const int r)
{
   int nn;

   for (nn = n; nn < n + r; nn++)
      camc(c, nn, a, f);
}

/*****************************************************************\
*                                                                 *
* General functions                                               *
*                                                                 *
\*****************************************************************/
/*---------------------------------------------------------------*/
INLINE int cam_init(void)
{
  int crate = 0, status = 0;	

  /* VME initialization  */ 
  status = mvme_open(&myvme, 0);
	if (status != MVME_SUCCESS)
		return status;
		
  /* Set am to A24 non-privileged Data */
  mvme_set_am(myvme, MVME_AM_A24_ND);

  /* Set dmode to D16 */
  mvme_set_dmode(myvme, MVME_DMODE_D16);

#ifdef OS_VXWORKS
    /* vxWorks specific memory-mapping function calls */
    if (vxworks_mmap(myvme, CBD8210_BASE, 8 * 0x10000) != MVME_SUCCESS) {
      printf("vxworks_mmap error crate:%d\n", crate);
    }  
#else
  /* VMIC specific memory-mapping function calls */
  /*
  for (crate = 0; crate < 8; crate++) {
    vmecrate = (CBD8210_BASE | branch << 19 | crate << 16);
    if (vmic_mmap(myvme, vmecrate, 0x10000) != MVME_SUCCESS) {
      printf("vmic_mmap error crate:%d\n", crate);
    }  
  }
  */

  /*
   * Single mapping for all 8 crates of the branch 0 
   */

    if (vmic_mmap(myvme, CBD8210_BASE, 8 * 0x10000) != MVME_SUCCESS) {
      printf("vmic_mmap error crate:%d\n", crate);
    }  
#endif

  return SUCCESS;
}

/*---------------------------------------------------------------*/
INLINE void cam_exit(void)
{
   mvme_close(myvme);
}

/*---------------------------------------------------------------*/
INLINE void cam_inhibit_set(const int c)
{
   camc(c, 30, 9, 26);
}

/*---------------------------------------------------------------*/
INLINE int cam_inhibit_test(const int c)
{
   return 1;
}

/*---------------------------------------------------------------*/
INLINE void cam_inhibit_clear(const int c)
{
   camc(c, 30, 9, 24);
}

/*---------------------------------------------------------------*/
INLINE void cam_crate_clear(const int c)
{
   camc(c, 28, 9, 26);
}

/*---------------------------------------------------------------*/
INLINE void cam_crate_zinit(const int c)
{
   camc(c, 28, 8, 26);
}

/*---------------------------------------------------------------*/
INLINE void cam_lam_enable(const int c, const int n)
{
   camc(c, n, 0, 26);
}

/*---------------------------------------------------------------*/
INLINE void cam_lam_disable(const int c, const int n)
{
   camc(c, n, 0, 24);
}

/*---------------------------------------------------------------*/
INLINE void cam_lam_read(const int c, DWORD * lam)
{
   cam24i(0, 29, 0, 10, lam);
}

/*---------------------------------------------------------------*/
INLINE void cam_lam_clear(const int c, const int n)
{
/* Depend on the hardware LAM implementation
as this cmd should talk to the controller
but can include the LAM source module LAM clear
  camc (c,n,0,10); */
}

/*****************************************************************\
*                                                                 *
* Interrupt functions                                             *
*                                                                 *
\*****************************************************************/

INLINE void cam_interrupt_enable(const int c)
{
   printf("cam_interrupt_enable - Not yet implemented\n");
}

/*---------------------------------------------------------------*/
INLINE void cam_interrupt_disable(const int c)
{
   printf("cam_interrupt_disable - Not yet implemented\n");
}

/*---------------------------------------------------------------*/
INLINE int cam_interrupt_test(const int c)
{
   printf("cam_interrupt_test - Not yet implemented\n");
   return 0;
}

/*---------------------------------------------------------------*/
INLINE void cam_interrupt_attach(const int c, const int n, void (*isr) (void))
{
   printf("cam_interrupt_attach - Not yet implemented\n");
}

/*---------------------------------------------------------------*/
INLINE void cam_interrupt_detach(const int c, const int n)
{
   printf("cam_interrupt_detach - Not yet implemented\n");
}

/*---------------------------------------------------------------*/
INLINE void cam_glint_enable(void)
{
   printf("cam_glint_enable - Not yet implemented\n");
}

/*---------------------------------------------------------------*/
INLINE void cam_glint_disable(void)
{
   printf("cam_glint_disable - Not yet implemented\n");
}

/*---------------------------------------------------------------*/
INLINE void cam_glint_attach(int lam, void (*isr) (void))
{
   printf("cam_glint_attach - Not yet implemented\n");
}

/*---------------------------------------------------------------*/
void csr(void)
{
   WORD online;

   CAM_CSRCHK(online);
   printf("CSR Camac  status 0x%4.4x\n", online);
}

/*---------------------------------------------------------------*/
void car(void)
{
   WORD online;

   CAM_CARCHK(online);
   printf("CAR Online status 0x%4.4x\n", online);
}

/*---------------------------------------------------------------*/
void btb(void)
{
   WORD online;

   CAM_BTBCHK(online);
   printf("BTB Online status 0x%4.4x\n", online);
}

/*---------------------------------------------------------------*/
void gl(void)
{
   DWORD online;

   CAM_GLCHK(online);
   printf("GLam     register 0x%6.6lx\n", online);
}

/*---------------------------------------------------------------*/
void cam_op(void)
{
   csr();
   car();
   btb();
   gl();
}

/********************************************************************/
#ifdef OS_VXWORKS
int ces8210 () {
  int status;
  int i, n;

  status = cam_init();
  if (status != 1)
  {
    printf("Error: Cannot initialize camac: cam_init status %d\n",status);
    return 1;
  }
  /* Branch Status */
#if 1
  {
    cam_op(); 
    for(i=1;i<8;i++)
      printf("camc_chk(%d): %d\n", i, camc_chk(i));
  }

    cam_inhibit_clear(1);
    cam_inhibit_clear(2);

    for (n=0; n<=24; n++)
    {
      unsigned long data;
      int q, x;
      cam24i_q(1,n,0,0,&data,&x,&q);
    }

#endif

  return 0;
}

#endif
#ifdef MAIN_ENABLE
int main () {
  int status;
  int c = 0;
  int i, n;

  status = cam_init();
  if (status != 1)
  {
    printf("Error: Cannot initialize camac: cam_init status %d\n",status);
    return 1;
  }

  /* Initialization */
#if 0
  for (i=1;i<8;i++) {
    cam_inhibit_set(i);
    printf("Inhibit Set crate:%d\n",i);
    usleep(1000000);
    cam_inhibit_clear(i);
    printf("Inhibit Clear crate: %d\n",i);
    usleep(1000000);
  }
#endif

  /* Branch Status */
#if 1
  {
    cam_op(); 
    for(i=1;i<8;i++)
      printf("camc_chk(%d): %d\n", i, camc_chk(i));
  }
#endif

  /* LAM test */
#if 0
  for (i=1;i<24;i++)  cam_lam_disable(c,i);
  while (1)
  {
    unsigned long lam;
    cam_lam_enable(c,22);
    cam_lam_enable(c,23);
    camc(c,22,0,9);
    camc(c,22,0,25);
    sleep(1);
    cam_lam_read(c, &lam);
    printf("lam status 0x%x expected 0x%x\n",lam,1<<(22-1));
  }
#endif

  /* Scan crate for Read 24 /q/x */
  while (0)
  {
    for (n=0; n<=24; n++)
    {
      unsigned long data;
      int q, x;
      cam24i_q(4,n,0,0,&data,&x,&q);
      printf("station %d, data 0x%x, q %d, x %d\n",n,data,q,x);
      usleep(100000);
    }
  }

  /* Access/Speed test  
     cami/o ~2.5us per access. */
  c = 4;
  while (0)
  {
    static unsigned long data;
    cam24o(c,19,0,16,0x0);
    cam24o(c,19,0,16,0x1);
    cam24o(c,19,0,16,0x0);
    cam24i(c,22,0,0,&data);
    cam24o(c,19,0,16,0x2);
  }

  return 0;
}

#endif
/* end */

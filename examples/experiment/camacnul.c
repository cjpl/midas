/*-----------------------------------------------------------------------------
 *  Copyright (c) 1998      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 * 
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca           Tel: (604) 222-1047  Fax: (604) 222-1074
 *         amaudruz@triumf.ca
 * -----------------------------------------------------------------------------
 *  
 *  Description	: CAMAC interface following the ESONE CAMAC calls.
 *                for NUll drivers
 * 
 *  Requires 	: none
 *
 *  Application : null camac interface for debugging
 *     
 *  Author:  Pierre-Andre Amaudruz Data Acquisition Group
 * 
 *  Revision 1.0  1998        Pierre	 Initial revision
 *
 *------------------------------------------------------------------------ */

#include <mcstd.h>

/*------------------------------------------------------------------*/

INLINE void cam16i(const int c, const int n, const int a, const int f, WORD *d){*d=0;}
INLINE void cam24i(const int c, const int n, const int a, const int f, DWORD *d){*d=0;}
INLINE void cam16i_q(const int c, const int n, const int a, const int f, WORD *d, int *x, int *q){*d=0;*q=*x=1;}
INLINE void cam24i_q(const int c, const int n, const int a, const int f, DWORD *d, int *x, int *q){*d=0;*q=*x=1;}
INLINE void cam16i_r(const int c, const int n, const int a, const int f, WORD **d, const int r){}
INLINE void cam24i_r(const int c, const int n, const int a, const int f, DWORD **d, const int r){}
INLINE void cam16i_rq(const int c, const int n, const int a, const int f, WORD **d, const int r){}
INLINE void cam24i_rq(const int c, const int n, const int a, const int f, DWORD **d, const int r){}
INLINE void cam16i_sa(const int c, const int n, const int a, const int f, WORD **d, const int r){}
INLINE void cam24i_sa(const int c, const int n, const int a, const int f, DWORD **d, const int r){}
INLINE void cam16i_sn(const int c, const int n, const int a, const int f, WORD **d, const int r){}
INLINE void cam24i_sn(const int c, const int n, const int a, const int f, DWORD **d, const int r){}
INLINE void cami(const int c, const int n, const int a, const int f, WORD *d){*d=0;}
INLINE void cam16o(const int c, const int n, const int a, const int f, WORD d){}
INLINE void cam24o(const int c, const int n, const int a, const int f, DWORD d){}
INLINE void cam16o_q(const int c, const int n, const int a, const int f, WORD d, int *x, int *q){*q=*x=1;}
INLINE void cam24o_q(const int c, const int n, const int a, const int f, DWORD d, int *x, int *q){*q=*x=1;}
INLINE void cam16o_r(const int c, const int n, const int a, const int f, WORD *d, const int r){}
INLINE void cam24o_r(const int c, const int n, const int a, const int f, DWORD *d, const int r){}
INLINE void camo(const int c, const int n, const int a, const int f, WORD d){}
INLINE int  camc_chk(const int c){return 0;}
INLINE void camc(const int c, const int n, const int a, const int f){}
INLINE void camc_q(const int c, const int n, const int a, const int f, int *q){*q=1;}
INLINE void camc_sa(const int c, const int n, const int a, const int f, const int r){}
INLINE void camc_sn(const int c, const int n, const int a, const int f, const int r){}
INLINE int  cam_init(void){return 1;}
INLINE int  cam_init_rpc(char *host_name, char *exp_name, char *client_name, char *rpc_server){return 1;}
INLINE void cam_exit(void){}
INLINE void cam_inhibit_set(const int c){}
INLINE void cam_inhibit_clear(const int c){}
INLINE void cam_crate_clear(const int c){}
INLINE void cam_crate_zinit(const int c){}
INLINE void cam_lam_enable(const int c, const int n){}
INLINE void cam_lam_disable(const int c, const int n){}
INLINE void cam_lam_read(const int c, int *lam){*lam=1;}
INLINE void cam_lam_clear(const int c, const int n){}
INLINE void came_cn(int *ext, const int b, const int c, const int n, const int a){}
INLINE void came_ext(const int ext, int *b, int *c, int *n, int *a){}
INLINE void cam16ei(const int ext, const int f, WORD *d){}
INLINE void cam24ei(const int ext, const int f, DWORD *d){}
INLINE void cam16ei_q(const int ext, const int f, WORD *d, int *x, int *q){*q=*x=1;}
INLINE void cam24ei_q(const int ext, const int f, DWORD *d, int *x, int *q){*q=*x=1;}
INLINE void cam16ei_r(const int ext, const int f, WORD **d, const int r){}
INLINE void cam24ei_r(const int ext, const int f, DWORD **d, const int r){}
INLINE void cam16ei_rq(const int ext, const int f, WORD **d, const int r){}
INLINE void cam24ei_rq(const int ext, const int f, DWORD **d, const int r){}
INLINE void cam16ei_sa(const int ext, const int f, WORD **d, const int r){}
INLINE void cam24ei_sa(const int ext, const int f, DWORD **d, const int r){}
INLINE void cam16ei_sn(const int ext, const int f, WORD **d, const int r){}
INLINE void cam24ei_sn(const int ext, const int f, DWORD **d, const int r){}
INLINE void cam16eo(const int ext, const int f, WORD d){}
INLINE void cam24eo(const int ext, const int f, DWORD d){}
INLINE void cam16eo_q(const int ext, const int f, WORD d, int *x, int *q){*q=*x=1;}
INLINE void cam24eo_q(const int ext, const int f, DWORD d, int *x, int *q){*q=*x=1;}
INLINE void camec(const int ext, const int f){}
INLINE void camec_q(const int ext, const int f, int *x, int *q){*q=*x=1;}
INLINE void camec_sa(const int ext, const int f, const int r){}
INLINE void camec_sn(const int ext, const int f, const int r){}
INLINE void cam_interrupt_enable(void){}
INLINE void cam_interrupt_disable(void){}
INLINE void cam_interrupt_attach(void (*isr)()){}
INLINE void cam_interrupt_detach(void){}
INLINE void cam_glint_enable(void){}
INLINE void cam_glint_disable(void){}
INLINE void cam_glint_attach(int lam, void (*isr)()){}
INLINE void cam_glint_detach(){}

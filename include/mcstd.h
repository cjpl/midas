/*-----------------------------------------------------------------------------
 *  Copyright (c) 1998      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 * 
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca           Tel: (604) 222-1047  Fax: (604) 222-1074
 *         amaudruz@triumf.ca
 * -----------------------------------------------------------------------------
 *  
 *  Description	: Midas Camac Standard calls. 
 *  Requires 	: 
 *  Application : Used in any camac driver
 *  Author:  Pierre-Andre Amaudruz Data Acquisition Group
 *  Revision 1.0  1998        Pierre	 Initial revision
 *           1.1  JUL 98      SR         Added 8-bit functions, BYTE definition
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

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned char      BYTE;
typedef unsigned short int WORD;

#ifdef __alpha
typedef unsigned int       DWORD;
#else
typedef unsigned long int  DWORD;
#endif

#define SUCCESS  1

#endif /* MIDAS_TYPE_DEFINED */

/*------------------------------------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

EXTERNAL INLINE void EXPRT cam8i    (const int c, const int n, const int a, const int f, BYTE *d);
EXTERNAL INLINE void EXPRT cam16i   (const int c, const int n, const int a, const int f, WORD *d);
EXTERNAL INLINE void EXPRT cam24i   (const int c, const int n, const int a, const int f, DWORD *d);
EXTERNAL INLINE void EXPRT cam8i_q  (const int c, const int n, const int a, const int f, BYTE *d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam16i_q (const int c, const int n, const int a, const int f, WORD *d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam24i_q (const int c, const int n, const int a, const int f, DWORD *d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam8i_r  (const int c, const int n, const int a, const int f, BYTE **d, const int r);
EXTERNAL INLINE void EXPRT cam16i_r (const int c, const int n, const int a, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24i_r (const int c, const int n, const int a, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cam8i_rq (const int c, const int n, const int a, const int f, BYTE **d, const int r);
EXTERNAL INLINE void EXPRT cam16i_rq(const int c, const int n, const int a, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24i_rq(const int c, const int n, const int a, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cam8i_sa (const int c, const int n, const int a, const int f, BYTE **d, const int r);
EXTERNAL INLINE void EXPRT cam16i_sa(const int c, const int n, const int a, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24i_sa(const int c, const int n, const int a, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cam8i_sn (const int c, const int n, const int a, const int f, BYTE **d, const int r);
EXTERNAL INLINE void EXPRT cam16i_sn(const int c, const int n, const int a, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24i_sn(const int c, const int n, const int a, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cami     (const int c, const int n, const int a, const int f, WORD *d);
EXTERNAL INLINE void EXPRT cam8o    (const int c, const int n, const int a, const int f, BYTE  d);
EXTERNAL INLINE void EXPRT cam16o   (const int c, const int n, const int a, const int f, WORD  d);
EXTERNAL INLINE void EXPRT cam24o   (const int c, const int n, const int a, const int f, DWORD  d);
EXTERNAL INLINE void EXPRT cam8o_q  (const int c, const int n, const int a, const int f, BYTE d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam16o_q (const int c, const int n, const int a, const int f, WORD d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam24o_q (const int c, const int n, const int a, const int f, DWORD d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam8o_r  (const int c, const int n, const int a, const int f, BYTE *d, const int r);
EXTERNAL INLINE void EXPRT cam16o_r (const int c, const int n, const int a, const int f, WORD *d, const int r);
EXTERNAL INLINE void EXPRT cam24o_r (const int c, const int n, const int a, const int f, DWORD *d, const int r);
EXTERNAL INLINE void EXPRT camo     (const int c, const int n, const int a, const int f, WORD d);
EXTERNAL INLINE int  EXPRT camc_chk (const int c);
EXTERNAL INLINE void EXPRT camc     (const int c, const int n, const int a, const int f);
EXTERNAL INLINE void EXPRT camc_q   (const int c, const int n, const int a, const int f, int *q);
EXTERNAL INLINE void EXPRT camc_sa  (const int c, const int n, const int a, const int f, const int r);
EXTERNAL INLINE void EXPRT camc_sn  (const int c, const int n, const int a, const int f, const int r);
EXTERNAL INLINE int  EXPRT cam_init (void);
EXTERNAL INLINE int  EXPRT cam_init_rpc(char *host_name, char *exp_name, char *client_name, char *rpc_server);
EXTERNAL INLINE void EXPRT cam_exit (void);
EXTERNAL INLINE void EXPRT cam_inhibit_set  (const int c);
EXTERNAL INLINE void EXPRT cam_inhibit_clear(const int c);
EXTERNAL INLINE void EXPRT cam_crate_clear  (const int c);
EXTERNAL INLINE void EXPRT cam_crate_zinit  (const int c);
EXTERNAL INLINE void EXPRT cam_lam_enable   (const int c, const int n);
EXTERNAL INLINE void EXPRT cam_lam_disable  (const int c, const int n);
EXTERNAL INLINE void EXPRT cam_lam_read     (const int c, DOWRD *lam);
EXTERNAL INLINE void EXPRT cam_lam_clear    (const int c, const int n);
EXTERNAL INLINE void EXPRT came_cn   (int *ext, const int b, const int c, const int n, const int a);
EXTERNAL INLINE void EXPRT came_ext  (const int ext, int *b, int *c, int *n, int *a);
EXTERNAL INLINE void EXPRT cam16ei   (const int ext, const int f, WORD *d);
EXTERNAL INLINE void EXPRT cam24ei   (const int ext, const int f, DWORD *d);
EXTERNAL INLINE void EXPRT cam16ei_r (const int ext, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24ei_r (const int ext, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cam16ei_q (const int ext, const int f, WORD *d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam24ei_q (const int ext, const int f, DWORD *d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam16eo_q (const int ext, const int f, WORD d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam24eo_q (const int ext, const int f, DWORD d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam16ei_rq(const int ext, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24ei_rq(const int ext, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cam16ei_sa(const int ext, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24ei_sa(const int ext, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cam16ei_sn(const int ext, const int f, WORD **d, const int r);
EXTERNAL INLINE void EXPRT cam24ei_sn(const int ext, const int f, DWORD **d, const int r);
EXTERNAL INLINE void EXPRT cam16eo   (const int ext, const int f, WORD d);
EXTERNAL INLINE void EXPRT cam24eo   (const int ext, const int f, DWORD d);
EXTERNAL INLINE void EXPRT camec     (const int ext, const int f);
EXTERNAL INLINE void EXPRT camec_q   (const int ext, const int f, int *x, int *q);
EXTERNAL INLINE void EXPRT camec_sa  (const int ext, const int f, const int r);
EXTERNAL INLINE void EXPRT camec_sn  (const int ext, const int f, const int r);
EXTERNAL INLINE void EXPRT cam_interrupt_enable (void);
EXTERNAL INLINE void EXPRT cam_interrupt_disable(void);
EXTERNAL INLINE void EXPRT cam_interrupt_attach (void (*isr)(void));
EXTERNAL INLINE void EXPRT cam_interrupt_detach (void);
EXTERNAL INLINE void EXPRT cam_glint_enable     (void);
EXTERNAL INLINE void EXPRT cam_glint_disable    (void);
EXTERNAL INLINE void EXPRT cam_glint_attach     (int lam, void (*isr)(void));
EXTERNAL INLINE void EXPRT cam_glint_detach(void);

#ifdef __cplusplus
}
#endif

/********************************************************************

  Name:         hyt1331.c
  Created by:   Stefan Ritt

  Contents:     Device driver for HYTEC 1331 Turbo CAMAC controller
                following the MIDAS CAMAC Standard under DIRECTIO

  $Log$
  Revision 1.7  2000/09/28 10:49:57  midas
  Added OUTP_P/INP_P for NT and optimized cam16i_sa

  Revision 1.6  2000/09/27 12:05:57  midas
  Outcommented info about auto increment switch

  Revision 1.5  2000/09/26 07:10:50  midas
  Added DO_IOPERM for debugging as root

  Revision 1.4  2000/09/07 17:14:31  pierre
  - added fe_name

  Revision 1.3  2000/04/19 07:39:22  midas
  Outcommented "fast" cam24i_r because of problems

  Revision 1.2  2000/01/13 18:02:10  pierre
  - Fix version due to CVS re-organization

  Revision 1.6  1999/02/22 19:09:34  pierre
  - Remove came_xxx
  - Fix cam16i_sa() sub address increment.

  Revision 1.5  1998/11/25 15:57:01  midas
  Outcommented printf statements for auto subadress increment

  Revision 1.4  1998/11/19 22:35:17  pierre
  This version is not yet using all the features of controller.
  The _sa, _rq and maybe the _r will be revised soon.
  correct (*d)-- for the _rq in case of noQ
  Remove Q-stop for _sa and_sn

  Revision 1.3  1998/10/12 12:18:56  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "mcstd.h"

/*------------------------------------------------------------------*/
/*
  Base address of PC card. Must match jumper setting:
  ===================================================

  LK1    LK2    ADDRESS

  IN     IN     0x200 - 0x23F
  OUT    IN     0x280 - 0x3BF
  IN     OUT    0x300 - 0x33F
  OUT    OUT    0x380 - 0x3BF
*/

#define CAMAC_BASE 0x280

/*
  Interrupt request selection. Must match jumper setting:
  =======================================================

  LINK IN       IRQ line    Software interrupt

   9            IRQ 5       INT 0x0D (13)
  10            IRQ 3       INT 0x0B (11)
  11            IRQ 9       INT 0x02 (Cascaded)
  12            IRQ 12      INT 0x02 (Cascaded)
*/

#define LAM_INT 0x0D
#define LAM_IRQ 5

/*------------------------------------------------------------------*/

#if defined( __MSDOS__ )
#include <dos.h>
#define OUTP(_p, _d) outportb(_p, _d)
#define OUTPW(_p, _d) outport(_p, _d)
#define INP(_p) inportb(_p)
#define INPW(_p) inport(_p)
#define OUTP_P(_p, _d) outportb(_p, _d)
#define OUTPW_P(_p, _d) outport(_p, _d)
#define INP_P(_p) inportb(_p)
#define INPW_P(_p) inport(_p)
#elif defined( _MSC_VER )
#include <windows.h>
#include <conio.h>
#define OUTP(_p, _d) _outp((unsigned short) (_p), (int) (_d))
#define OUTPW(_p, _d) _outpw((unsigned short) (_p), (unsigned short) (_d))
#define INP(_p) _inp((unsigned short) (_p))
#define INPW(_p) _inpw((unsigned short) (_p))
#define OUTP_P(_p, _d) {_outp((unsigned short) (_p), (int) (_d)); _outp((unsigned short)0x80,0);}
#define OUTPW_P(_p, _d) {_outpw((unsigned short) (_p), (unsigned short) (_d)); _outp((unsigned short)0x80,0);}
#define INP_P(_p) _inp((unsigned short) (_p)); _outp((unsigned short)0x80,0);
#define INPW_P(_p) _inpw((unsigned short) (_p));_outp((unsigned short)0x80,0);
#elif defined( OS_LINUX )
#if !defined(__OPTIMIZE__)
#error Please compile hyt1331.c with the -O flag to make port access possible
#endif
#include <asm/io.h>
#include <unistd.h>
#define OUTP(_p, _d) outb(_d, _p)
#define OUTPW(_p, _d) outw(_d, _p)
#define INP(_p) inb(_p)
#define INPW(_p) inw(_p)
#define OUTP_P(_p, _d) outb_p(_d, _p)
#define OUTPW_P(_p, _d) outw_p(_d, _p)
#define INP_P(_p) inb_p(_p)
#define INPW_P(_p) inw_p(_p)
#endif

/*- Global var -----------------------------------------------------*/

BOOL  gbl_sw1d=FALSE;

/*------------------------------------------------------------------*/
INLINE void cam8i(const int c, const int n, const int a, const int f, 
                  unsigned char *d)
{
  WORD adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *((unsigned char *) d) = INP(adr);
}

/*------------------------------------------------------------------*/
INLINE void cami(const int c, const int n, const int a, const int f, 
                 WORD *d)
{
  WORD adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *((unsigned char *) d) = INP(adr);
  *(((unsigned char *) d)+1) = INP(adr+2);
}

/*------------------------------------------------------------------*/
INLINE void cam16i(const int c, const int n, const int a, const int f, 
                   WORD *d)
{
  cami(c, n, a, f, d);
}

/*------------------------------------------------------------------*/
INLINE void cam24i(const int c, const int n, const int a, const int f, 
                   DWORD *d)
{
  unsigned short adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *((unsigned char *) d) = INP(adr);
  *(((unsigned char *) d)+1) = INP(adr+2);
  *(((unsigned char *) d)+2) = INP(adr+4);
  *(((unsigned char *) d)+3) = 0;
}

/*------------------------------------------------------------------*/
INLINE void cam8i_q(const int c, const int n, const int a, const int f, 
                    unsigned char *d, int *x, int *q)
{
  unsigned short adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *((unsigned char *) d) = INP(adr);
  
  *q = status & 1;
  *x = status >> 7;
}

/*------------------------------------------------------------------*/
INLINE void cam16i_q(const int c, const int n, const int a, const int f, 
                     WORD *d, int *x, int *q)
{
  WORD adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *((unsigned char *) d) = INP(adr);
  *(((unsigned char *) d)+1) = INP(adr+2);
  
  *q = status & 1;
  *x = status >> 7;
}

/*------------------------------------------------------------------*/
INLINE void cam24i_q(const int c, const int n, const int a, const int f, 
                     DWORD *d, int *x, int *q)
{
  WORD adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *((unsigned char *) d) = INP(adr);
  *(((unsigned char *) d)+1) = INP(adr+2);
  *(((unsigned char *) d)+2) = INP(adr+4);
  *(((unsigned char *) d)+3) = 0;
  
  *q = status & 1;
  *x = status >> 7;
}

/*------------------------------------------------------------------*/
INLINE void cam16i_r(const int c, const int n, const int a, const int f, 
                     WORD **d, const int r)
{
  WORD adr, i, status;
  
  adr = CAMAC_BASE+(c<<4);
  
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP_P(adr+10,f);
  
  status = (unsigned char) INP(adr+6);
  *((unsigned char *) *d) = INP(adr);
  *(((unsigned char *) *d)+1) = INP(adr+2); /* read the first word */
  (*d)++;
  
  INPW_P(adr+12); /* trigger first cycle */
  
  for (i=0 ; i<(r-1) ; i++) 
    *((*d)++) = INPW(adr+12); /* read data and trigger next cycle */
}

/*------------------------------------------------------------------*/
INLINE void cam24i_r(const int c, const int n, const int a, const int f, 
                     DWORD **d, const int r)
{
WORD adr, i, status;

  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  
  for (i=0 ; i<r ; i++)
    {
    OUTP(adr+10, f);
    status = (unsigned char) INP(adr+6);
    *((unsigned char *) *d) = INP(adr);
    *(((unsigned char *) *d)+1) = INP(adr+2);
    *(((unsigned char *) *d)+2) = INP(adr+4);
    *(((unsigned char *) *d)+3) = 0;
    (*d)++;
    }
  
  /*
  gives unrealiable results, SR 6.4.00

  adr = CAMAC_BASE+(c<<4);
  
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP_P(adr+10,f);
  
  status = (unsigned char) INP(adr+6);
	*((unsigned char *) *d) = INP(adr);
  *(((unsigned char *) *d)+1) = INP(adr+2); // read the first word 
  *(((unsigned char *) *d)+2) = INP(adr+4);
  *(((unsigned char *) *d)+3) = 0;
  (*d)++;
  
  INPW_P(adr+12); // trigger first cycle
  
  for (i=0 ; i<(r-1) ; i++) 
  {
    *(((unsigned char *) *d)+2) = INP_P(adr+4);
    *(((unsigned char *) *d)+3) = 0;
				*((unsigned short *) *d) = INPW_P(adr+12); 
    (*d)++;
  }
  */
}

/*------------------------------------------------------------------*/
INLINE void cam16i_rq(const int c, const int n, const int a, const int f, 
                      WORD **d, const int r)
{
  WORD adr, i;
  int  fail;
  
  /* following code is disabled by above code */
  adr = CAMAC_BASE+(c<<4);
  
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  /* Turn on the Q-mode for repeat until Q=1 in the INPW(adr+12) */
  OUTP(adr+1, 0x10);
  OUTP_P(adr+10, f);
  INPW_P(adr+12); /* trigger first cycle */

  for (i=0 ; i<r ; i++)
    {
    /* read data and trigger next cycle fail = no valid Q within 12usec */
    **d = INPW_P(adr+12); 
    fail = ((unsigned char) INP(adr+6)) & 0x20; // going to test!
    if (fail)
      break;
    (*d)++;
    }
  /* Turn off the Q-mode for repeat until Q=1 in the INPW(adr+12) */
  OUTP(adr+1, 0x0);
}

/*------------------------------------------------------------------*/
INLINE void cam24i_rq(const int c, const int n, const int a, const int f, 
                      DWORD **d, const int r)
{
  WORD adr;
  int  i, fail;
  
  adr = CAMAC_BASE+(c<<4);
  
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  /* Turn on the Q-mode for repeat until Q=1 in the INPW(adr+12) */
  OUTP(adr+1, 0x10);
  OUTP_P(adr+10, f);
  INPW_P(adr+12); /* trigger first cycle */
  
  for (i=0 ; i<r ; i++)
    {
    /* read data and trigger next cycle fail = no valid Q within 12usec */
    *(((unsigned char *) *d)+2) = INP(adr+4);
    *(((unsigned char *) *d)+3) = 0;
				*((unsigned short *) *d) = INPW_P(adr+12); 
    fail = ((unsigned char) INP(adr+6)) & 0x20; // going to test!
    if (fail)
      break;
    (*d)++;
    }
  /* Turn off the Q-mode for repeat until Q=1 in the INPW(adr+12) */
  OUTP(adr+1, 0x0);
}

/*------------------------------------------------------------------*/
INLINE void cam16i_sa(const int c, const int n, const int a, const int f, 
                      WORD **d, const int r)
{
  WORD adr, i;
  
  if (gbl_sw1d)
    {
    adr = CAMAC_BASE+(c<<4);
    /* enable auto increment */
    OUTP(adr+10, 49);
    OUTP(adr+8, n);
    OUTP(adr+6, a-1);
    OUTP_P(adr+10, f);
    INPW(adr+12);
    
    for (i=0 ; i<r ; i++)
      *((*d)++) = INPW(adr+12); /* read data and trigger next cycle */
    
    /* disable auto increment */
    OUTP(adr+10, 48);
    }
  else
    for (i=0;i<r;i++)
      cami(c, n, a+i, f, (*d)++);
}

/*------------------------------------------------------------------*/
INLINE void cam24i_sa(const int c, const int n, const int a, const int f, 
                      DWORD **d, const int r)
{
  WORD adr, i;
  
  if (gbl_sw1d)
  {
    adr = CAMAC_BASE+(c<<4);
    
    /* enable auto increment */
    OUTP(adr+10, 49);
    OUTP(adr+8, n);
    OUTP(adr+6, a-1);
    OUTP_P(adr+10, f);
    INPW_P(adr+12); 
    for (i=0 ; i<r ; i++)
    {
      /* read data and trigger next cycle */
      *(((unsigned char *) *d)+2) = INP(adr+4);
      *(((unsigned char *) *d)+3) = 0;
      *((unsigned short *) *d) = INPW_P(adr+12); 
      (*d)++;
    }
    
    /* disable auto increment */
    OUTP_P(adr+10, 48);
  }
  else
    for (i=0;i<r;i++)
      cam24i(c, n, a+i, f, (*d)++);
}

/*------------------------------------------------------------------*/
INLINE void cam16i_sn(const int c, const int n, const int a, const int f, 
                      WORD **d, const int r)
{
  int i;
  
  for (i=0 ; i<r ; i++)
    cam16i(c, n+i, a, f, (*d)++);
}

/*------------------------------------------------------------------*/
INLINE void cam24i_sn(const int c, const int n, const int a, const int f, 
                      DWORD **d, const int r)
{
  int i;
  
  for (i=0 ; i<r ; i++)
    cam24i(c, n+i, a, f, (*d)++);
}

/*------------------------------------------------------------------*/
INLINE void cam8o(const int c, const int n, const int a, const int f, 
                  unsigned char d)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr, d);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
}

/*------------------------------------------------------------------*/
INLINE void camo(const int c, const int n, const int a, const int f, 
                 WORD d)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr, (unsigned char ) d);
  OUTP(adr+2, *(((unsigned char *) &d)+1));
  OUTP(adr+6, a);
  OUTP(adr+10, f);
}

/*------------------------------------------------------------------*/
INLINE void cam16o(const int c, const int n, const int a, const int f, 
                   WORD d)
{
  camo(c, n, a, f, d);
}

/*------------------------------------------------------------------*/
INLINE void cam24o(const int c, const int n, const int a, const int f, 
                   DWORD d)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr, (unsigned char ) d);
  OUTP(adr+2, *(((unsigned char *) &d)+1));
  OUTP(adr+4, *(((unsigned char *) &d)+2));
  OUTP(adr+6, a);
  OUTP(adr+10, f);
}

/*------------------------------------------------------------------*/
INLINE void cam16o_q(const int c, const int n, const int a, const int f, 
                     WORD d, int *x, int *q)
{
  unsigned int adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr, (unsigned char ) d);
  OUTP(adr+2, *(((unsigned char *) &d)+1));
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *q = status & 1;
  *x = status >> 7;
}

/*------------------------------------------------------------------*/
INLINE void cam24o_q(const int c, const int n, const int a, const int f, 
                     DWORD d, int *x, int *q)
{
  unsigned int adr, status;

  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr, (unsigned char ) d);
  OUTP(adr+2, *(((unsigned char *) &d)+1));
  OUTP(adr+4, *(((unsigned char *) &d)+2));
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *q = status & 1;
  *x = status >> 7;
}

/*------------------------------------------------------------------*/
INLINE void cam8o_r(const int c, const int n, const int a, const int f, 
                    BYTE *d, const int r)
{
  WORD adr, i;
  
  adr = CAMAC_BASE+(c<<4);
  
  /* trigger first cycle */
  OUTP(adr+8, n);
  OUTP(adr, *((unsigned char *) d));
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  for (i=0 ; i<r-1 ; i++)
    OUTPW(adr+12, (BYTE) *(++d)); /* write data and trigger next cycle */
}

/*------------------------------------------------------------------*/
INLINE void cam16o_r(const int c, const int n, const int a, const int f, 
                     WORD *d, const int r)
{
  WORD adr, i;
  
  adr = CAMAC_BASE+(c<<4);
  
  /* trigger first cycle */
  OUTP(adr+8, n);
  OUTP(adr, *((unsigned char *) d));
  OUTP(adr+2, *(((unsigned char *) d)+1));
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  for (i=0 ; i<r-1 ; i++)
    OUTPW(adr+12, *(++d)); /* write data and trigger next cycle */
}

/*------------------------------------------------------------------*/
INLINE void cam24o_r(const int c, const int n, const int a, const int f, 
                     DWORD *d, const int r)
{
  WORD adr, i;
  
  adr = CAMAC_BASE+(c<<4);
  
  /* trigger first cycle */
  OUTP(adr+8, n);
  OUTP(adr, *((unsigned char *) d));
  OUTP(adr+2, *(((unsigned char *) d)+1));
  OUTP(adr+4, *(((unsigned char *) d)+2));
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  for (i=0 ; i<r-1 ; i++)
  {
    d++;
    OUTP(adr+4, *(((unsigned char *) d)+2));
    OUTPW(adr+12, (WORD) *d); /* write data and trigger next cycle */
  }
}

/*------------------------------------------------------------------*/
INLINE int camc_chk(const int c)
{
  unsigned int adr, n, a, f;
  
  /* clear inhibit */
  camc(c, 1, 2, 32);
  
  /* read back naf */
  adr = CAMAC_BASE+(c<<4);
  a = (unsigned char) INP(adr+10);
  n = (unsigned char) INP(adr+10);
  f = (unsigned char) INP(adr+10);
  
  if (n != 1 || a != 2 || f != 32)
    return -1;
  
  return 0;
}

/*------------------------------------------------------------------*/
INLINE void camc(const int c, const int n, const int a, const int f)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
}

/*------------------------------------------------------------------*/
INLINE void camc_q(const int c, const int n, const int a, const int f, int *q)
{
  unsigned int adr, status;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
  
  status = (unsigned char) INP(adr+6);
  *q = status & 1;
}

/*------------------------------------------------------------------*/
INLINE void camc_sa(const int c, const int n, const int a, const int f, const int r)
{
  int i;
  
  for (i=0 ; i<r ; i++)
    camc(c, n, a+i, f);
}

/*------------------------------------------------------------------*/
INLINE void camc_sn(const int c, const int n, const int a, const int f, const int r)
{
  int i;
  
  for (i=0 ; i<r ; i++)
    camc(c, n+i, a, f);
}

/*------------------------------------------------------------------*/
#ifdef _MSC_VER
static HANDLE _hdio = 0;
#endif

INLINE int cam_init(void)
{
  unsigned char status;
  unsigned int adr;
  
#ifdef _MSC_VER
  OSVERSIONINFO vi;
  DWORD buffer1[] = {6, CAMAC_BASE, CAMAC_BASE+4*0x10, 0};
  DWORD buffer2[] = {6, 0x80, 0x80, 0};
  DWORD size;
  
  vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&vi);
  
  /* use DirectIO driver under NT to gain port access */
  if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    _hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL,
		       OPEN_EXISTING, 0, NULL);
    if (_hdio == INVALID_HANDLE_VALUE)
    {
      printf("hyt1331.c: Cannot access IO ports (No DirectIO driver installed)\n");
      return -1;
    }
  }
  
  if (!DeviceIoControl(_hdio, (DWORD) 0x9c406000, &buffer1, sizeof(buffer1), 
		       NULL, 0, &size, NULL))
    return -1;

  if (!DeviceIoControl(_hdio, (DWORD) 0x9c406000, &buffer2, sizeof(buffer2), 
		       NULL, 0, &size, NULL))
    return -1;

#endif _MSC_VER
#ifdef OS_LINUX

  /* 
  In order to access the IO ports of the CAMAC interface, one needs
  to call the ioperm() function for those ports. This requires root
  privileges. For normal operation, this is performed by the "dio"
  program, which is a "setuid" program having temporarily root privi-
  lege. So the frontend is started with "dio frontend". Since the
  frontend cannot be debugged through the dio program, we suplly here
  the direct ioperm call which requires the program to be run as root,
  making it possible to debug it. The program has then to be compiled
  with "gcc -DDO_IOPERM -o frontend frontend.c hyt1331.c ..."
  */

#ifdef DO_IOPERM
  ioperm(0x80, 1 , 1);
  if (ioperm(CAMAC_BASE, 4*0x10, 1) < 0)
    printf("hyt1331.c: Cannot call ioperm() (no root privileges)\n");
#endif  
  
#endif
  
  adr =  CAMAC_BASE; 
  
  status = INP(CAMAC_BASE+6);
  if (!(status & 0x40))
    {
    gbl_sw1d = FALSE;
    //printf("hyt1331.c: Auto increment disabled by SW1D. camxxi_sa won't work. 0x%x\n",status); 
    }
  else
    {
    gbl_sw1d = TRUE;
    //printf("hyt1331.c: Auto increment enabled by SW1D. camxxi_sa will work. 0x%x\n",status);
    }
  return SUCCESS;
}

/*------------------------------------------------------------------*/
INLINE void cam_exit(void)
{
#ifdef _MSC_VER
  DWORD buffer[] = {6, CAMAC_BASE, CAMAC_BASE+4*0x10, 0};
  DWORD size;
  
  if (_hdio <= 0)
    return;
  
  DeviceIoControl(_hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), 
		  NULL, 0, &size, NULL);
#endif
}

/*------------------------------------------------------------------*/
INLINE void cam_inhibit_set(const int c)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+10, 33);
}

/*------------------------------------------------------------------*/
INLINE void cam_inhibit_clear(const int c)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+10, 32);
}

/*------------------------------------------------------------------*/
INLINE void cam_crate_clear(const int c)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+10, 36);
}

/*------------------------------------------------------------------*/
INLINE void cam_crate_zinit(const int c)
{
  unsigned int adr;
  
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+10, 34);
}

/*------------------------------------------------------------------*/
INLINE void cam_lam_enable(const int c, const int n)
{ 
  /* enable the station number */
  unsigned int adr;
  
  /* enable LAM in controller */
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+10, 64+n);
  
  /* enable interrupts in controller */
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+10, 41);
}

/*------------------------------------------------------------------*/
INLINE void cam_lam_disable(const int c, const int n)
{ 
  /* disable the station number */
  unsigned int adr;
  
  /* disable LAM flip-flop in unit */
  /* camc(c, n, 0, 24); */
  
  /* disable LAM in controller */
  adr = CAMAC_BASE+(c<<4);
  OUTP(adr+10, 128+n);
}

/*------------------------------------------------------------------*/
INLINE void cam_lam_read(const int c, DWORD *lam)
{
  /* return a BITWISE coded station NOT the station number 
     i.e.: INP(adr+8) = 5 ==> lam = 0x10
     I mask off the upper 3 bit */
  unsigned int adr, csr; 
  
  adr = CAMAC_BASE+(c<<4);
  csr = (unsigned char) INP(adr+6);
  if (csr & 0x8)
  {
    *lam = ((unsigned char) INP(adr+8));
    *lam = 1<<((*lam&0x1f)-1);
  }
  else
    *lam = 0;
}

/*------------------------------------------------------------------*/
INLINE void cam_lam_clear(const int c, const int n)
{ 
  unsigned int adr;
  
  /* signal End-Of-Interrupt */
#ifdef __MSDOS__
  OUTP(0x20, 0x20);
#endif
  
  /* clear LAM flip-flop in unit */
  camc(c, n, 0, 10);
  
  /* restart LAM scanner in controller */
  adr = CAMAC_BASE+(c<<4);
  INP(adr+8);
}

/*------------------------------------------------------------------*/
INLINE void cam_interrupt_enable(void)
{
  unsigned char mask;
  
  mask = INP(0x21);
  mask &= (~(1<<LAM_IRQ));
  OUTP(0x21, mask);
}

/*------------------------------------------------------------------*/
INLINE void cam_interrupt_disable(void)
{
  unsigned char mask;
  
  mask = INP(0x21);
  mask |= 1<<LAM_IRQ;
  OUTP(0x21, mask);
}

/*------------------------------------------------------------------*/
static void (*old_handler)(void) = NULL;

INLINE void cam_interrupt_attach(void (*isr)(void))
{ 
#ifdef __MSDOS__
  old_handler = (void (*)()) getvect(LAM_INT);
  setvect(LAM_INT, (void interrupt (*)()) isr);
#endif
}

/*------------------------------------------------------------------*/
INLINE void cam_interrupt_detach(void)
{
#ifdef __MSDOS__
  if (old_handler)
    setvect(LAM_INT, (void interrupt (*)()) old_handler);
#endif
}

/*------------------------------------------------------------------*/
INLINE int cam_init_rpc(char *host_name, char *exp_name, char *fe_name, char *client_name, char *rpc_server){return 1;}

/*------------------------------------------------------------------*/

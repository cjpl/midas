/********************************************************************

  Name:         hyt1331pci.c
  Created by:   Stefan Ritt

  Contents:     Device driver for HYTEC 1331 Turbo CAMAC controller
                via 5331 PCI interface card following the 
                MIDAS CAMAC Standard under DIRECTIO

  $Log$
  Revision 1.1  2001/08/08 11:51:54  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include "mcstd.h"

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
#define OUTP(_p, _d) _outp((unsigned short) (_p), (unsigned char) (_d))
#define OUTPW(_p, _d) _outpw((unsigned short) (_p), (unsigned short) (_d))
#define OUTPD(_p, _d) _outpd((unsigned short) (_p), (unsigned int) (_d))
#define INP(_p) _inp((unsigned short) (_p))
#define INPW(_p) _inpw((unsigned short) (_p))
#define INPD(_p) _inpd((unsigned short) (_p))
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
WORD  camac_base;

/*------------------------------------------------------------------*/
INLINE void cam8i(const int c, const int n, const int a, const int f, 
                  unsigned char *d)
{
  WORD adr, status;
  
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
  
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

  adr = camac_base+(c<<4);
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

  adr = camac_base+(c<<4);
  
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
  adr = camac_base+(c<<4);
  
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
  
  adr = camac_base+(c<<4);
  
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
    adr = camac_base+(c<<4);
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
    adr = camac_base+(c<<4);
    
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
  
  adr = camac_base+(c<<4);
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

  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
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

  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
  
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
  
  adr = camac_base+(c<<4);
  
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
  
  adr = camac_base+(c<<4);
  
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
  adr = camac_base+(c<<4);
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
  
  adr = camac_base+(c<<4);
  OUTP(adr+8, n);
  OUTP(adr+6, a);
  OUTP(adr+10, f);
}

/*------------------------------------------------------------------*/
INLINE void camc_q(const int c, const int n, const int a, const int f, int *q)
{
  unsigned int adr, status;
  
  adr = camac_base+(c<<4);
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
  
#ifdef _MSC_VER
  OSVERSIONINFO vi;
  DWORD buffer1[] = {6, 0, 0, 0};
  DWORD buffer2[] = {8, 0x1196, 0x5331, 0}; /* VendorID and Device ID of 5331 PLX chip */
  DWORD retbuf[4];
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
  
  /* obtain base address of 5331 card */
  if (!DeviceIoControl(_hdio, (DWORD) 0x9c406000, &buffer2, sizeof(buffer2), 
		       retbuf, sizeof(retbuf), &size, NULL))
    return -1;

  /* open ports according to base address */
  camac_base = (WORD) (retbuf[3] & 0xFFF0);
  buffer1[1] = camac_base;
  buffer1[2] = camac_base + 4*0x10;
  if (!DeviceIoControl(_hdio, (DWORD) 0x9c406000, &buffer1, sizeof(buffer1), 
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
  if (ioperm(camac_base, 4*0x10, 1) < 0)
    printf("hyt1331.c: Cannot call ioperm() (no root privileges)\n");
#endif  
  
#endif
  
  status = INP(camac_base+6);
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
  DWORD buffer[] = {7, camac_base, camac_base+4*0x10, 0};
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
  
  adr = camac_base+(c<<4);
  OUTP(adr+10, 33);
}

/*------------------------------------------------------------------*/
INLINE void cam_inhibit_clear(const int c)
{
  unsigned int adr;
  
  adr = camac_base+(c<<4);
  OUTP(adr+10, 32);
}

/*------------------------------------------------------------------*/
INLINE void cam_crate_clear(const int c)
{
  unsigned int adr;
  
  adr = camac_base+(c<<4);
  OUTP(adr+10, 36);
}

/*------------------------------------------------------------------*/
INLINE void cam_crate_zinit(const int c)
{
  unsigned int adr;
  
  adr = camac_base+(c<<4);
  OUTP(adr+10, 34);
}

/*------------------------------------------------------------------*/
INLINE void cam_lam_enable(const int c, const int n)
{ 
  /* enable the station number */
  unsigned int adr;
  
  /* enable LAM in controller */
  adr = camac_base+(c<<4);
  OUTP(adr+10, 64+n);
  
  /* enable interrupts in controller */
  adr = camac_base+(c<<4);
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
  adr = camac_base+(c<<4);
  OUTP(adr+10, 128+n);
}

/*------------------------------------------------------------------*/
INLINE void cam_lam_read(const int c, DWORD *lam)
{
  /* return a BITWISE coded station NOT the station number 
     i.e.: INP(adr+8) = 5 ==> lam = 0x10
     I mask off the upper 3 bit */
  unsigned int adr, csr; 
  
  adr = camac_base+(c<<4);
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
  adr = camac_base+(c<<4);
  INP(adr+8);
}

/*------------------------------------------------------------------*/

INLINE int cam_init_rpc(char *host_name, char *exp_name, char *fe_name, char *client_name, char *rpc_server)
{
  return 1;
}

/*------------------------------------------------------------------*/

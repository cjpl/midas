/********************************************************************\

  Name:         ces2117.h
  Created by:   Stefan Ritt

  Cotents:      CAMAC functions for CES VCC 2117 controller

  $Log$
  Revision 1.1  1999/12/20 10:18:11  midas
  Reorganized driver directory structure

  Revision 1.2  1998/10/12 12:18:56  midas
  Added Log tag in header


\********************************************************************/

/*------------------------------------------------------------------*/

#define CAMO(n,a,f,d) *((unsigned long *) (0xC90000 | ((((f<<5)|n)<<4)|a)<<2)) = (d)
#define CAMI(n,a,f,d) *(d) = (unsigned short) *((unsigned long *) (0xC00000 | ((((f<<5)|n)<<4)|a)<<2)) & 0xFFFFFF
#define CAMI24(n,a,f,d) *(d) = *((unsigned long *) (0xC00000 | ((((f<<5)|n)<<4)|a)<<2)) & 0xFFFFFF
#define CAMIQ(n,a,f,d,x,q) { unsigned long tmp; \
                             tmp = *((unsigned long *) (0xC00000 | ((((f<<5)|n)<<4)|a)<<2)); \
                             *x = (tmp & 0x40000000) > 0; \
                             *q = (tmp & 0x80000000) > 0; \
                             *(d) = (unsigned short) tmp; }
#define CAMIQ24(n,a,f,d,x,q) { *((unsigned long *)(d)) = *((unsigned long *) (0xC00000 | ((((f<<5)|n)<<4)|a)<<2)); \
                             *x = (*(d) & 0x40000000) > 0; \
                             *q = (*(d) & 0x80000000) > 0; \
                             *(d) &= 0xFFFFFF; }
#define CAMQ(n,a,f,q)      { unsigned long tmp; \
                             tmp = *((unsigned long *) (0xC00000 | ((((f<<5)|n)<<4)|a)<<2)); \
                             *q = (tmp & 0x80000000) > 0; }
#define CAML(d)              *(d) = *((unsigned long *) (0xC00780))

#define CAMAC_INIT()
#define CAMAC_SET_INHIBIT()     *((unsigned long *) 0xC0E7A4) = 0
#define CAMAC_CLEAR_INHIBIT()   *((unsigned long *) 0xC0C7A4) = 0
#define CAMAC_C_CYCLE()         *((unsigned long *) 0xC0D724) = 0
#define CAMAC_Z_CYCLE()         *((unsigned long *) 0xC0D720) = 0
#define CAMAC_LAM_ENABLE(n)
#define CAMAC_LAM_RESET()

#define CAM_INTERRUPT_ATTACH(adr) \

    /* enable the CIO's interrupts */      
    lock = intLock();
    cioCtl = CIO1_CNTRL_ADRS;                    /* CIO #1                */
    i = *cioCtl;
    wait; *cioCtl = ZCIO_PACS;  wait; *cioCtl = ZCIO_CS_SIE;
    wait; *cioCtl = ZCIO_PBCS;  wait; *cioCtl = ZCIO_CS_SIE;   
    wait;
    cioCtl = CIO2_CNTRL_ADRS;                      /* CIO #2              */
    i = *cioCtl;
    wait; *cioCtl = ZCIO_PACS;  wait; *cioCtl = ZCIO_CS_SIE;
    wait;
  }
  else {
    QX  = *(int *)(VCCcamacBase + VCCcdlm); 

    /* ---------------------------------- disable the CIO's interrupts -- */ 
    lock = intLock();
    cioCtl = CIO2_CNTRL_ADRS;                    /* CIO #2                */
    i = *cioCtl;
    wait; *cioCtl = ZCIO_PACS;  wait; *cioCtl = ZCIO_CS_CLIE;
    wait;
    cioCtl = CIO1_CNTRL_ADRS;                    /* CIO #1                */
    i = *cioCtl;
    wait; *cioCtl = ZCIO_PBCS;  wait; *cioCtl = ZCIO_CS_CLIE;
    wait; *cioCtl = ZCIO_PACS;  wait; *cioCtl = ZCIO_CS_CLIE;   
    wait;
  }
  /* and clear the pending IRQs (as we don't clear the LAM as the         */
  /* source of the IRQ, we again get all the for now cleared interupts    */
  /* after we enable IRQ handling again!)                                 */
  cioCtl = CIO1_CNTRL_ADRS;                      /* CIO #2                */
  i = *cioCtl;
  wait; *cioCtl = ZCIO_PACS;  wait; *cioCtl = ZCIO_CS_CLIPIUS;  
  wait; *cioCtl = ZCIO_PBCS;  wait; *cioCtl = ZCIO_CS_CLIPIUS;  
  wait;
  cioCtl = CIO2_CNTRL_ADRS;                      /* CIO #1                */
  i = *cioCtl;
  wait; *cioCtl = ZCIO_PACS;  wait; *cioCtl = ZCIO_CS_CLIPIUS;  
  wait;
  intUnlock(lock);


/*------------------------------------------------------------------*/

#define CAMAC_TEST void camo(n,a,f,d)  \
{ CAMO(n,a,f,d); }                     \
void cami(n,a,f) {                     \
  unsigned long d,x,q;                 \
  CAMIQ(n,a,f,&d,&x,&q);               \
  printf("d=%lX,X=%d,Q=%d\n",d,x,q); } \
void caml() {                          \
  unsigned long d;                     \
  CAML(&d);                            \
  printf("l=%lX\n",d); }               \
void set_inhibit(void)                 \
{ *((unsigned long *) 0xC0E7A4) = 0; } \
void clear_inhibit(void)               \
{ *((unsigned long *) 0xC0C7A4) = 0; } \
void c_cycle(void)                     \
{ *((unsigned long *) 0xC0D724) = 0; } \
void z_cycle(void)                     \
{ *((unsigned long *) 0xC0D720) = 0; } 

/*---- ESONE calls -------------------------------------------------*/

#define CDREG(ext, b, c, n, a) *ext = (0xC00000 | (((n)<<4)|a)<<2)

#define CFSA(f, ext, d, q)     { if (f>15) *((unsigned long *) (ext | (f<<11)) = *(d); \
                                 else { *((unsigned long *)(d)) = *((unsigned long *) (ext | (f<<11)); \
                                        *q = (*(d) & 0x80000000) > 0; \
                                        *(d) &= 0xFFFFFF; }

#define CSSA(f, ext, d, q)     { if (f>15) *((unsigned long *) (ext | (f<<11)) = *(d); \
                                 else { unsigned long tmp; \
                                        tmp = *((unsigned long *) (ext | (f<<11); \
                                        *q = (tmp & 0x80000000) > 0; \
                                        *(d) = (unsigned short) tmp; }}
#define CCCZ(ext) CAMAC_Z_CYLE()

#define CCCC(ext) CAMAC_C_CYLE()

#define CCCI(ext, l) { if (l) CAMAC_SET_INHIBIT(); else CAMAC_CLEAR_INHIBIT(); }

#define CDLAM(lam,b,c,n,a,intarr) CDREG(lam,b,c,n,a)

#define CCLM(lam, l)              { int _q; if (l) CFSA(26,lam,0,&_q); else CFSA(24,lam,0,&_q); }

#define CCLC(lam)                 { int _q; CFSA(10,lam,0,&_q); }

#define CTLM(lam,_q)              { int _q; CFSA(8,lam,0,&_q); }



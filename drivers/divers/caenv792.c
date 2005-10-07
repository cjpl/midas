/*********************************************************************

  Name:         v792.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     V792 32ch. QDC
                
  $Id:$

*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vmicvme.h"
#include "caenv792.h"

/*****************************************************************/
int v792_EventRead(DWORD *pbase, DWORD *pdest, int *nentry)
{
  DWORD hdata;
  
  *nentry = 0;
  if (v792_DataReady((WORD *)pbase)) {
    do {
      hdata = *pbase;
    } while (!(hdata & 0x02000000)); // skip up to the header
    pdest[*nentry] = hdata;
    *nentry += 1;
    do {
      pdest[*nentry] = *pbase;
      *nentry += 1;
    } while (!(pdest[*nentry-1] & 0x04000000)); // copy until the trailer

    nentry--;
  }
  return *nentry;
}

/*****************************************************************/
int v792_DataRead(DWORD *pbase, DWORD *pdest, int *nentry)
{
  int k;

  *nentry = 0;
  if (v792_DataReady((WORD *)pbase)) {
    for (k=0 ; k<32 ; k++) {
      pdest[k] = pbase[k];
      *nentry += 1;
    }
  }
  return *nentry;
}

/*****************************************************************/
int v792_ThresholdWrite(WORD *pbase, WORD *threshold, int *nitems)
{
  int k;
  
  for (k=0; k<V792_MAX_CHANNELS ; k++) {
    pbase[V792_THRES_BASE+k] = threshold[k] & 0x1FF;
  }
  
  for (k=0; k<V792_MAX_CHANNELS ; k++) {
    threshold[k] = pbase[V792_THRES_BASE+k] & 0x1FF;
  }
  *nitems = V792_MAX_CHANNELS;
  return *nitems;
}

/*****************************************************************/
void v792_EvtCntRead(WORD *pbase, DWORD *evtcnt)
{
  *evtcnt = (pbase[V792_EVT_CNT_L_RO]
	     + (DWORD) (0x10000 * pbase[V792_EVT_CNT_H_RO]));
}

/*****************************************************************/
void v792_SingleShotReset(WORD *pbase)
{
  pbase[V792_SINGLE_RST_WO] = 1;
}

/*****************************************************************/
int  v792_DataReady(WORD *pbase)
{
  //printf("data ready: 0x%08x\n",pbase[V792_CSR1_RO]);
  return (pbase[V792_CSR1_RO] & 0x1);
}

/*****************************************************************/
void  v792_Status(WORD *pbase)
{
  int status;
  
  printf("v792 Status for %p\n", pbase);
  status = pbase[V792_CSR1_RO];
  printf("Amnesia: %s, ", status & 0x10 ? "Y" : "N");
  printf("Term ON: %s, ", status & 0x40 ? "Y" : "N");
  printf("TermOFF: %s\n", status & 0x80 ? "Y" : "N");
  status = pbase[V792_FIRM_REV];
  printf("Firmware: 0x%x\n",status);

  if (pbase[V792_FIRM_REV] == 0xFFFF)
    {
      printf("v792 at %p: Invalid firmware revision: 0x%x\n",pbase,pbase[V792_FIRM_REV]);
      abort();
    }
}

/*****************************************************************/
int v792_GeoWrite(WORD *pbase, int geo)
{
  pbase[V792_GEO_ADDR_RW] = (geo & 0x1F);
  return (int) (pbase[V792_GEO_ADDR_RW] & 0x1F); 
}

/* end */

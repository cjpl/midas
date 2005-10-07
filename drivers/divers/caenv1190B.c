/*********************************************************************

  Name:         v1190B.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     V1190B 64ch. TDC
                
  $Id:$

*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include <mvmestd.h>
#include "caenv1190B.h"

/*****************************************************************/
/*****************************************************************/
int v1190_EventRead(DWORD *pbase, DWORD *pdest, int * nentry)
{
  uint32_t hdata;

  *nentry = 0;
  if (v1190_DataReady((WORD *)pbase)) {
    do {
      hdata = *pbase;
    } while (!(hdata & 0x40000000));
    pdest[*nentry] = hdata;
    *nentry += 1;
    do {
      pdest[*nentry] = *pbase;
      *nentry += 1;
    } while (!(pdest[*nentry-1] & 0xc0000000)); 

    nentry--;
  }
  return *nentry;

/*
  header = *pbase & 0xFF000000;
  
  switch (header) {
  case 0x40000000:  // Global Header 
    break;
  case 0x00000000:  // TDC Header
    break;
  case 0x10000000:  // Data
    break;
  case 0x20000000:  // Error
    break;
  case 0x80000000:  // Trailer
    break;
  case 0xc0000000:  // Filler
    break;
  }
  
  return *nentry;
*/
}

/*****************************************************************/
int v1190_DataRead(DWORD *pbase, uint32_t *dest, int maxwords)
{
  int i;
  for (i=0; i<maxwords; i++)
    {
      if (!v1190_DataReady((uint16_t *)pbase))
	break;
      *dest++ = *pbase;
    }
  return i;
}

/*****************************************************************/
int v1190_GeoWrite(WORD *pbase, int geo)
{
  pbase[V1190_GEO_REG_RW] = (geo & 0x1F);
  return (int) (pbase[V1190_GEO_REG_RW] & 0x1F); 
}

/*****************************************************************/
void v1190_SoftReset(uint16_t *pbase)
{
  pbase[V1190_MODULE_RESET_WO] = 0;
}

/*****************************************************************/
void v1190_SoftClear(uint16_t *pbase)
{
  pbase[V1190_SOFT_CLEAR_WO] = 0;
}

/*****************************************************************/
void v1190_SoftTrigger(uint16_t *pbase)
{
  pbase[V1190_SOFT_TRIGGER_WO] = 0;
}

/*****************************************************************/
int  v1190_DataReady(uint16_t *pbase)
{
  return (pbase[V1190_SR_RO] & V1190_DATA_READY);
}

/*****************************************************************/
int  v1190_EvtStored(uint16_t *pbase)
{
  return (pbase[V1190_EVT_STORED_RO]);
}

/*****************************************************************/
int  v1190_EvtCounter(uint16_t *pbase)
{
  return (pbase[V1190_EVT_CNT_RO]);
}

/*****************************************************************/
int  v1190B_TdcIdList(uint16_t *pbase)
{
  uint16_t  i, code;
  uint32_t value;

//  v1190_MicroFlush(pbase);
  for (i=0; i<2 ; i++) {
    code   = 0x6000 | (i & 0x0F);
    value  = v1190_MicroWrite(pbase, code);
    value  = v1190_MicroRead(pbase);
    value = (v1190_MicroRead(pbase) << 16) | value;
    printf("Received :code: 0x%04x  0x%08x\n", code, value); 
  }
  return 1;
}

/*****************************************************************/
int  v1190B_ResolutionRead(uint16_t *pbase)
{
  uint16_t  i, code;
  uint32_t value;

//  v1190_MicroFlush(pbase);
  for (i=0; i<2 ; i++) {
    code = V1190_RESOLUTION_RO | (i & 0x0F);
    value = v1190_MicroWrite(pbase, code);
    value = v1190_MicroRead(pbase);
//    printf("Received :code: 0x%04x  0x%08x\n", code, value); 
  }
  return 1;
}

/*****************************************************************/
int  v1190B_AcqModeRead(uint16_t *pbase)
{
  uint16_t  code;
  uint32_t value;

//  v1190_MicroFlush(pbase);
  code = V1190_ACQ_MODE_RO;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
//  printf("Received :code: 0x%04x  0x%08x\n", code, value); 
  return 1;
}

/*****************************************************************/
int  v1190B_TriggerMatchingSet(uint16_t *pbase)
{
  uint16_t  code;
  uint32_t value;

//  v1190_MicroFlush(pbase);
  code = V1190_TRIGGER_MATCH_WO;
  value = v1190_MicroWrite(pbase, code);
//  printf("Received :code: 0x%04x  0x%08x\n", code, value); 
  
  return 1;
}

/*****************************************************************/
int  v1190B_ContinuousSet(uint16_t *pbase)
{
  uint16_t  code;
  uint32_t value;

//  v1190_MicroFlush(pbase);
  code = V1190_CONTINUOUS_WO;
  value = v1190_MicroWrite(pbase, code);
//  printf("Received :code: 0x%04x  0x%08x\n", code, value); 
  
  return 1;
}

/*****************************************************************/
void v1190_SetEdgeDetection(uint16_t *pbase,int enableLeading,int enableTrailing)
{
  int value = 0;

  if (enableLeading)
    value |= 2;

  if (enableTrailing)
    value |= 1;

  v1190_MicroFlush(pbase);
  v1190_MicroWrite(pbase, 0x2200);
  v1190_MicroWrite(pbase, value);
}

/*****************************************************************/
int v1190_MicroWrite(uint16_t *pbase, uint16_t data)
{
  int i;
  for (i=0; i<1000; i++)
  {
    uint16_t microHS = pbase[V1190_MICRO_HAND_RO];
    if (microHS & V1190_MICRO_WR_OK) {
      pbase[V1190_MICRO_RW] = data;
      return 0;
    }
    udelay(500);
  }
  
  printf("v1190_MicroWrite: Micro not ready for writing!\n");
  return -1;
}

/*****************************************************************/
int v1190_MicroRead(const uint16_t *pbase)
{
  int i;
  int reg=-1;

  for (i=100; i>0; i--) {
    uint16_t  microHS = pbase[V1190_MICRO_HAND_RO];
    if (microHS & V1190_MICRO_RD_OK) {
      reg = pbase[V1190_MICRO_RW];
//      printf("i:%d microHS:%d %x\n", i, microHS, reg);
      return (reg);
    }
    udelay(500);
  };
  return -1;
}

/*****************************************************************/
int v1190_MicroFlush(const uint16_t *pbase)
{
  int count = 0;
  while (1)
    {
      int data = v1190_MicroRead(pbase);
      printf("microData[%d]: 0x%04x\n",count,data);
      if (data < 0)
	break;
      count++;
    }
  return count;
}

/*****************************************************************/
int  v1190_Setup(uint16_t *pbase, int mode)
{
  uint16_t value;
  int      status = -1;
  
//  v1190_MicroFlush(pbase);
  switch (mode) {
  case 0x1:
    printf("Trigger Matching Setup (mode:%d)\n", mode);
    printf("Default setting + Width : 800ns, Offset : -400ns\n");
    printf("Time subtract, Leading Edge only\n");
    status = v1190_MicroWrite(pbase, 0x0000);
    if (status < 0)
      return status;
#if 0
    value = v1190_MicroWrite(pbase, 0x1000); // width
    value = v1190_MicroWrite(pbase, 0x60);     // Width : 800ns
    value = v1190_MicroWrite(pbase, 0x1100); // offset
    value = v1190_MicroWrite(pbase, 0xfe8);  // offset: -400ns
    value = v1190_MicroWrite(pbase, 0x1500); // subtraction flag
    value = v1190_MicroWrite(pbase, 0x2100); // leading edge detection
#endif
    break;
  case 0x2:
    value = v1190_MicroWrite(pbase, 0x0500); // default configuration
    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  v1190_Status(pbase);
  return 0;
}

/*****************************************************************/
int v1190_Status(uint16_t *pbase)
{
  uint16_t  i, code, pair=0;
  uint32_t value;
  
  printf("\nAcquisition mode:\n");
  value = v1190_MicroWrite(pbase, 0x0200);
  value = v1190_MicroRead(pbase);
  printf("  mode: 0x%02x\n", value);  

  code = 0x1600;
  printf("\nTrigger Section [code:0x%04x]:\n", code);
  if ((value = v1190_MicroWrite(pbase, code)) < 0)
    return -value;
  value = v1190_MicroRead(pbase);
  printf("  Match Window width       : 0x%04x\n", value);  
  value = v1190_MicroRead(pbase);
  printf("  Window offset            : 0x%04x\n", value);  
  value = v1190_MicroRead(pbase);
  printf("  Extra Search Window Width: 0x%04x\n", value);  
  value = v1190_MicroRead(pbase);
  printf("  Reject Margin            : 0x%04x\n", value);  
  value = v1190_MicroRead(pbase);
  printf("  Trigger Time subtration  : %s\n",(value & 0x1) ? "y" : "n");  

  printf("\nEdge Detection & Resolution Section:\n");
  code = 0x2300;
  value = v1190_MicroWrite(pbase, code);
  pair = value = v1190_MicroRead(pbase);
  printf("  Edge Detection (1:T/2:L/3:TL)           : 0x%02x\n", (value&0x3));  
  code = 0x2600;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  if (pair==0x3) {
    value = v1190_MicroRead(pbase);
    printf("  Leading Edge Resolution (see table)     : 0x%02x\n", (value&0x3));  
    printf("  Pulse Width Resolution (see table)      : 0x%02x\n", ((value>>8)&0xF));  
  } else {
    printf("  Resolution [ps] (0:800/1:200/2:100)     : 0x%02x\n", (value&0x3));  
  }
  code = 0x2900;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  printf("  Dead Time between hit [~ns](5/10/30/100): 0x%02x\n", (value&0x3));  
  
  printf("\nReadout Section:\n");
  code = 0x3200;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  printf("  Header/Trailer                            : %s\n",(value & 0x1) ? "y" : "n");  
  code = 0x3400;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  printf("  Max #hits per event 2^n-1 (>128:no limit) : %d\n", value&0xF);  
  code = 0x3a00;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  printf("  Internal TDC error type (see doc)         : 0x%04x\n", (value&0x7FF));  
  code = 0x3c00;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  printf("  Effective size of readout Fifo 2^n-1      : 0x%04x\n", (value&0xF));  
  
  printf("\nChannel Enable Section:\n");
  code = 0x4500;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  printf("  Read Enable Pattern [  0..15 ] : 0x%04x\n", value);  
  value = v1190_MicroRead(pbase);
  printf("  Read Enable Pattern [ 16..31 ] : 0x%04x\n", value);  
  value = v1190_MicroRead(pbase);
  printf("  Read Enable Pattern [ 32..47 ] : 0x%04x\n", value);  
  value = v1190_MicroRead(pbase);
  printf("  Read Enable Pattern [ 48..63 ] : 0x%04x\n", value);  
  code = 0x4700;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  value = (v1190_MicroRead(pbase)<<16) | value;
  printf("  Read Enable Pattern 32 (0) : 0x%08x\n", value);  
  code = 0x4701;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  value = (v1190_MicroRead(pbase)<<16) | value;
  printf("  Read Enable Pattern 32 (1) : 0x%08x\n", value);  
  printf("\nAdjust Section:\n");
  code = 0x5100;
  value = v1190_MicroWrite(pbase, code);
  value = v1190_MicroRead(pbase);
  printf("  Coarse Counter Offset: 0x%04x\n", (value&0x7FF));  
  value = v1190_MicroRead(pbase);
  printf("  Fine   Counter Offset: 0x%04x\n", (value&0x1F));  
  printf("\nMiscellaneous Section:\n");
  for (i=0; i<2 ; i++) {
    code = 0x6000 | (i & 0x0F);
    value = v1190_MicroWrite(pbase, code);
    value = v1190_MicroRead(pbase);
    value = (v1190_MicroRead(pbase) << 16) | value;
    printf("  TDC ID(%i)  0x%08x  [code:0x%04x]\n", i, value, code);  
  }
  return 0;
}

/*****************************************************************/
#ifndef HAVE_UDELAY
// this is the VMIC version of udelay()
int udelay(int usec)
{
  int i, j, k = 0;
  for (i=0; i<133; i++)
    for (j=0; j<usec; j++)
      k += (k+1)*j*k*(j+1);
  return k;
}
#endif



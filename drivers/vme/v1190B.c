/*********************************************************************

  Name:         v1190B.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     V1190B 64ch. TDC

  $Id: v1190B.c 4614 2009-10-28 18:34:23Z olchanski $
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "v1190B.h"

/*****************************************************************/
WORD v1190_Read16(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  return mvme_read_value(mvme, base+offset);
}

DWORD v1190_Read32(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base+offset);
}

void v1190_Write16(MVME_INTERFACE *mvme, DWORD base, int offset, WORD value)
{
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+offset, value);
}
/*****************************************************************/
/**
Read Data buffer for single event (check delimiters)
0x4... and 0xC...
@param *mvme VME structure
@param  base Module base address
@param *pdest destination pointer address
@param *nentry number of entries requested and returned.
@return
*/
int v1190_EventRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int *nentry)
{
  int cmode;
  DWORD hdata;

  *nentry = 0;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if (v1190_DataReady(mvme, base)) {
    do {
      hdata = mvme_read_value(mvme, base);
    } while (!(hdata & 0x40000000));
    pdest[*nentry] = hdata;
    *nentry += 1;

    do {
      pdest[*nentry] = mvme_read_value(mvme, base);
      *nentry += 1;
    } while (!(pdest[*nentry-1] & 0xc0000000));
  }
  mvme_set_dmode(mvme, cmode);
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
/**
Read data buffer for nentry data.
@param *mvme VME structure
@param  base Module base address
@param *pdest destination pointer address
@param *nentry number of entries requested and returned.
@return
*/
int v1190_DataRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int nentry)
{
  int cmode, status;

    mvme_get_dmode(mvme, &cmode);
    mvme_set_dmode(mvme, MVME_DMODE_D32);
    mvme_set_blt(mvme, MVME_BLT_BLT32);

    status = mvme_read(mvme, pdest, base, sizeof(DWORD) * nentry);

    mvme_set_dmode(mvme, cmode);
    return nentry;

    /*
      for (i=0 ; i<nentry ; i++) {
      if (!v1190_DataReady(mvme, base))
      break;
      pdest[i] = mvme_read_value(mvme, base);
      }
      mvme_set_dmode(mvme, cmode);
      return i;
    */
}

/*****************************************************************/
int v1190_GeoWrite(MVME_INTERFACE *mvme, DWORD base, int geo)
{
  int cmode, data;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1190_GEO_REG_RW, (geo & 0x1F));
  data = mvme_read_value(mvme, base+V1190_GEO_REG_RW);
  mvme_set_dmode(mvme, cmode);

  return (int) (data & 0x1F);
}

/*****************************************************************/
void v1190_SoftReset(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1190_MODULE_RESET_WO, 0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1190_SoftClear(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1190_SOFT_CLEAR_WO, 0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1190_SoftTrigger(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1190_SOFT_TRIGGER_WO, 0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int  v1190_DataReady(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, data;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  data = mvme_read_value(mvme, base+V1190_SR_RO);
  mvme_set_dmode(mvme, cmode);
  return (data & V1190_DATA_READY);
}

/*****************************************************************/
int  v1190_EvtStored(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, data;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  data = mvme_read_value(mvme, base+V1190_EVT_STORED_RO);
  mvme_set_dmode(mvme, cmode);
  return (data);
}

/*****************************************************************/
int  v1190_EvtCounter(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, data;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  data = mvme_read_value(mvme, base+V1190_EVT_CNT_RO);
  mvme_set_dmode(mvme, cmode);
  return (data);
}

/*****************************************************************/
void v1190_TdcIdList(MVME_INTERFACE *mvme, DWORD base)
{
  int  cmode, i, code;
  DWORD value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  for (i=0; i<2 ; i++) {
    code   = V1190_MICRO_TDCID | (i & 0x0F);
    value  = v1190_MicroWrite(mvme, base, code);
    value  = v1190_MicroRead(mvme, base);
    value = (v1190_MicroRead(mvme, base) << 16) | value;
    //    printf("Received :code: 0x%04x  0x%08lx\n", code, value);
  }
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int  v1190_ResolutionRead(MVME_INTERFACE *mvme, DWORD base)
{
  WORD  i, code;
  int   cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  for (i=0; i<2 ; i++) {
    code = V1190_RESOLUTION_RO | (i & 0x0F);
    value = v1190_MicroWrite(mvme, base, code);
    value = v1190_MicroRead(mvme, base);
    // printf("Received RR :code: 0x%04x  0x%08x\n", code, value);
  }
  mvme_set_dmode(mvme, cmode);
  return value;
}

/*****************************************************************/
void v1190_LEResolutionSet(MVME_INTERFACE *mvme, DWORD base, WORD le)
{
  int   cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  if ((le == LE_RESOLUTION_100) ||
      (le == LE_RESOLUTION_200) ||
      (le == LE_RESOLUTION_800)) {
    printf("le:%x\n", le);
    value = v1190_MicroWrite(mvme, base, V1190_LE_RESOLUTION_WO);
    value = v1190_MicroWrite(mvme, base, le);
  } else {
    printf("Wrong Leading Edge Resolution -> Disabled\n");
    value = v1190_MicroWrite(mvme, base, V1190_LE_RESOLUTION_WO);
    value = v1190_MicroWrite(mvme, base, 3);
  }
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1190_LEWResolutionSet(MVME_INTERFACE *mvme, DWORD base, WORD le, WORD width)
{
  printf("Not yet implemented\n");
}

/*****************************************************************/
void v1190_AcqModeRead(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  value = v1190_MicroWrite(mvme, base, V1190_ACQ_MODE_RO);
  value = v1190_MicroRead(mvme, base);
  //  printf("Received AR :code: 0x%04x  0x%08x\n", V1190_ACQ_MODE_RO, value);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1190_TriggerMatchingSet(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  value = v1190_MicroWrite(mvme, base, V1190_TRIGGER_MATCH_WO);
  //  printf("Received MS :code: 0x%04x  0x%08x\n", V1190_TRIGGER_MATCH_WO, value);

  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1190_ContinuousSet(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  value = v1190_MicroWrite(mvme, base, V1190_CONTINUOUS_WO);
  //  printf("Received CS :code: 0x%04x  0x%08x\n", V1190_CONTINUOUS_WO, value);

  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
Set the width of the matching Window. The width parameter should be
in the range of 1 to 4095 (0xFFF). Example 0x14 == 500ns.
@param *mvme VME structure
@param  base Module base address
@param width window width in ns units
@return
*/
void v1190_WidthSet(MVME_INTERFACE *mvme, DWORD base, WORD width)
{
  int   cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  //  v1190_MicroFlush(mvme, base);
  value = v1190_MicroWrite(mvme, base, V1190_WINDOW_WIDTH_WO);
  value = v1190_MicroWrite(mvme, base, width);
  //  printf("Received WS :code: 0x%04x  0x%08x\n", V1190_WINDOW_WIDTH_WO, value);

  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
Set the offset of the matching window with respect to the trigger.
The offset parameter should be in 25ns units. The range is
from -2048(0x800) to +40(0x28). Example 0xFE8 == 600ns.
@param *mvme VME structure
@param  base Module base address
@param  offset offset in ns units
*/
void v1190_OffsetSet(MVME_INTERFACE *mvme, DWORD base, WORD offset)
{
  int   cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  //  v1190_MicroFlush(mvme, base);
  value = v1190_MicroWrite(mvme, base, V1190_WINDOW_OFFSET_WO);
  value = v1190_MicroWrite(mvme, base, offset);
  //  printf("Received OS :code: 0x%04x  0x%08x\n", V1190_WINDOW_OFFSET_WO, value);

  mvme_set_dmode(mvme, cmode);
}

void v1190_OffsetSet_ns(MVME_INTERFACE *mvme, DWORD base, int offset_ns)
{
  unsigned w = abs(offset_ns)/25; // in units of 25 ns
  v1190_OffsetSet(mvme, base, -w);
}

void v1190_WidthSet_ns(MVME_INTERFACE *mvme, DWORD base, int width_ns)
{
  v1190_WidthSet(mvme, base, width_ns/25); // in units of 25 ns
}

/*****************************************************************/
void v1190_SetEdgeDetection(MVME_INTERFACE *mvme, DWORD base, int eLeading, int eTrailing)
{
  int cmode, value = 0;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  if (eLeading)
    value |= 2;

  if (eTrailing)
    value |= 1;

  //  v1190_MicroFlush(mvme, base);
  v1190_MicroWrite(mvme, base, V1190_EDGE_DETECTION_WO);
  v1190_MicroWrite(mvme, base, value);
  mvme_set_dmode(mvme, cmode);
}


/*****************************************************************/
int v1190_MicroWrite(MVME_INTERFACE *mvme, DWORD base, WORD data)
{
  int cmode, i;
  WORD microHS = 0;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  for (i=0; i<1000000; i++)
  {
    microHS = mvme_read_value(mvme, base+V1190_MICRO_HAND_RO);
    if (microHS & V1190_MICRO_WR_OK) {
      mvme_write_value(mvme, base+V1190_MICRO_RW, data);
      mvme_set_dmode(mvme, cmode);
      printf("v1190_MicroWrite: TDC at 0x%08x: write 0x%04x, wait %8d loops\n", base, data, i);
      return 1;
    }
    udelay(50);
  }

  printf("v1190_MicroWrite: Handshake timeout: Micro not ready for writing, TDC at 0x%08x, data 0x%04x, handshake register 0x%04x!\n", base, data, microHS);
  mvme_set_dmode(mvme, cmode);
  return -1;
}

/*****************************************************************/
int v1190_MicroRead(MVME_INTERFACE *mvme, const DWORD base)
{
  int cmode, i;
  int reg=-1;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  for (i=1000; i>0; i--) {
    WORD  microHS = mvme_read_value(mvme, base+V1190_MICRO_HAND_RO);
    if (microHS & V1190_MICRO_RD_OK) {
      reg = mvme_read_value(mvme, base+V1190_MICRO_RW);
//      printf("i:%d microHS:%d %x\n", i, microHS, reg);
      mvme_set_dmode(mvme, cmode);
      return (reg);
    }
    udelay(50);
  };

  printf("v1190_MicroRead: Handshake timeout: Micro not ready for reading, TDC at 0x%08x\n", base);
  mvme_set_dmode(mvme, cmode);
  return -1;
}

/*****************************************************************/
int v1190_MicroFlush(MVME_INTERFACE *mvme, const DWORD base)
{
  int cmode, count = 0;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  while (1)
    {
      int data = v1190_MicroRead(mvme, base);
      printf("microData[%d]: 0x%04x\n",count,data);
      if (data < 0)
  break;
      count++;
    }
  mvme_set_dmode(mvme, cmode);
  return count;
}

/*****************************************************************/
/**
Sets all the necessary paramters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@param *nentry number of entries requested and returned.
@return MVME_SUCCESS
*/
int  v1190_Setup(MVME_INTERFACE *mvme, DWORD base, int mode)
{
  WORD code, value;
  int      cmode, status = -1;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

//  v1190_MicroFlush(mvme, base);
  switch (mode) {
  case 0x1:
    printf("Trigger Matching Setup (mode:%d)\n", mode);
    printf("Default setting + Width : 800ns, Offset : -400ns\n");
    printf("Time subtract, Leading Edge only\n");
    code = 0x0000;  // Trigger matching Flag
    if ((status = v1190_MicroWrite(mvme, base, code)) < 0)
      return status;
    code = 0x1000;  // Width
    value = v1190_MicroWrite(mvme, base, code);
    value = v1190_MicroWrite(mvme, base, 0x20);   // Width : 800ns
    code = 0x1100;  // Offset
    value = v1190_MicroWrite(mvme, base, code);
    value = v1190_MicroWrite(mvme, base, 0xfe8);  // offset: -400ns
    code = 0x1500;  // Subtraction flag
    value = v1190_MicroWrite(mvme, base, code);
    code = 0x2100;  // Leading Edge Detection
    value = v1190_MicroWrite(mvme, base, code);
    break;
  case 0x2:
    code = 0x0500;  // Default configuration
    value = v1190_MicroWrite(mvme, base, code);
    break;
  default:
    printf("Unknown setup mode\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }
  //v1190_Status(mvme, base);
  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
/**
Read and display the curent status of the TDC.
@param *mvme VME structure
@param  base Module base address
@return MVME_SUCCESS, MicroCode error
*/
int v1190_Status(MVME_INTERFACE *mvme, DWORD base)
{
  WORD  i, code, pair=0;
  int   cmode, value;
  int ns = 25; // ns per tdc count

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  //-------------------------------------------------
  printf("\n");
  printf("V1190 TDC at VME A24 0x%06x:\n", base);
  printf("\n--- Trigger Section [0x1600]:\n");
  code = 0x1600;
  if ((value = v1190_MicroWrite(mvme, base, code)) < 0)
    return -value;
  value = v1190_MicroRead(mvme, base);
  printf("  Match Window width       : 0x%04x, %d ns\n", value, value*ns);
  value = v1190_MicroRead(mvme, base);
  printf("  Window offset            : 0x%04x, %d ns\n", value, ((WORD)value)*ns);
  value = v1190_MicroRead(mvme, base);
  printf("  Extra Search Window Width: 0x%04x, %d ns\n", value, value*ns);
  value = v1190_MicroRead(mvme, base);
  printf("  Reject Margin            : 0x%04x, %d ns\n", value, value*ns);
  value = v1190_MicroRead(mvme, base);
  printf("  Trigger Time subtraction : %s\n",(value & 0x1) ? "y" : "n");

  //-------------------------------------------------
  printf("\n--- Edge Detection & Resolution Section[0x2300/26/29]:\n");
  code = 0x2300;
  value = v1190_MicroWrite(mvme, base, code);
  pair = value = v1190_MicroRead(mvme, base);
  printf("  Edge Detection (1:T/2:L/3:TL)           : 0x%02x\n", (value&0x3));
  code = 0x2600;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  if (pair==0x3) {
    value = v1190_MicroRead(mvme, base);
    printf("  Leading Edge Resolution (see table)     : 0x%02x\n", (value&0x3));
    printf("  Pulse Width Resolution (see table)      : 0x%02x\n", ((value>>8)&0xF));
  } else {
    printf("  Resolution [ps] (0:800/1:200/2:100)     : 0x%02x\n", (value&0x3));
  }
  code = 0x2900;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  printf("  Dead Time between hit [~ns](5/10/30/100): 0x%02x\n", (value&0x3));

  //-------------------------------------------------
  printf("\n--- Readout Section[0x3200/34/3a/3c]:\n");
  code = 0x3200;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  printf("  Header/Trailer                            : %s\n",(value & 0x1) ? "y" : "n");
  code = 0x3400;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  printf("  Max #hits per event 2^n-1 (>128:no limit) : %d\n", value&0xF);
  code = 0x3a00;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  printf("  Internal TDC error type (see doc)         : 0x%04x\n", (value&0x7FF));
  code = 0x3c00;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  printf("  Effective size of readout Fifo 2^n-1      : 0x%04x\n", (value&0xF));

  //-------------------------------------------------
  printf("\n--- Channel Enable Section[0x4500/47/49]:\n");
  code = 0x4500;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  printf("  Read Enable Pattern [  0..15 ] : 0x%04x\n", value);
  value = v1190_MicroRead(mvme, base);
  printf("  Read Enable Pattern [ 16..31 ] : 0x%04x\n", value);
  value = v1190_MicroRead(mvme, base);
  printf("  Read Enable Pattern [ 32..47 ] : 0x%04x\n", value);
  value = v1190_MicroRead(mvme, base);
  printf("  Read Enable Pattern [ 48..63 ] : 0x%04x\n", value);
  code = 0x4700;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  value = (v1190_MicroRead(mvme, base)<<16) | value;
  printf("  Read Enable Pattern 32 (0) : 0x%08x\n", value);
  code = 0x4701;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  value = (v1190_MicroRead(mvme, base)<<16) | value;
  printf("  Read Enable Pattern 32 (1) : 0x%08x\n", value);

  //-------------------------------------------------
  printf("\n--- Adjust Section[0x5100/60]:\n");
  code = 0x5100;
  value = v1190_MicroWrite(mvme, base, code);
  value = v1190_MicroRead(mvme, base);
  printf("  Coarse Counter Offset: 0x%04x\n", (value&0x7FF));
  value = v1190_MicroRead(mvme, base);
  printf("  Fine   Counter Offset: 0x%04x\n", (value&0x1F));
  printf("\nMiscellaneous Section:\n");
  for (i=0; i<2 ; i++) {
    code = 0x6000 | (i & 0x0F);
    value = v1190_MicroWrite(mvme, base, code);
    value = v1190_MicroRead(mvme, base);
    value = (v1190_MicroRead(mvme, base) << 16) | value;
    printf("  TDC ID(%i)  0x%08x  [code:0x%04x]\n", i, value, code);
  }
  mvme_set_dmode(mvme, cmode);
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

/********************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main () {

  MVME_INTERFACE *myvme;

  DWORD VMEIO_BASE = 0x780000;
  DWORD V1190_BASE = 0xF10000;
  int status, csr, i;
  DWORD    cnt, array[10000];


  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D16
  mvme_set_dmode(myvme, MVME_DMODE_D16);

  // Get Firmware revision
  csr = mvme_read_value(myvme, V1190_BASE+V1190_FIRM_REV_RO);
  printf("Firmware revision: 0x%x\n", csr);

  // Print Current status
  v1190_Status(myvme, V1190_BASE);

  // Set mode 1
  // v1190_Setup(myvme, V1190_BASE, 1);

  csr = v1190_DataReady(myvme, V1190_BASE);
  printf("Data Ready: 0x%x\n", csr);

  // Read Event Counter
  cnt = v1190_EvtCounter(myvme, V1190_BASE);
  printf("Event counter: 0x%lx\n", cnt);

  memset(array, 0, sizeof(array));

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  // Set 0x3 in pulse mode for timing purpose
  mvme_write_value(myvme, VMEIO_BASE+0x8, 0xF);

  // Write pulse for timing purpose
  mvme_write_value(myvme, VMEIO_BASE+0xc, 0x2);

   mvme_set_blt(myvme, MVME_BLT_BLT32);

  // Read Data
  v1190_DataRead(myvme, V1190_BASE, array, 500);

  // Write pulse for timing purpose
  mvme_write_value(myvme, VMEIO_BASE+0xc, 0x8);

  for (i=0;i<12;i++)
    printf("Data[%i]=0x%lx\n", i, array[i]);

  memset(array, 0, sizeof(array));

  // Event Data
  /*
  status = 10;
  v1190_EventRead(myvme, V1190_BASE, array, &status);
  printf("count: 0x%x\n", status);
  for (i=0;i<12;i++)
    printf("Data[%i]=0x%x\n", i, array[i]);
  */

  status = mvme_close(myvme);
  return 0;
}
#endif


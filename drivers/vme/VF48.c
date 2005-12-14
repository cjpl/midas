/*********************************************************************

  Name:         VF48.c
  Created by:   Pierre-Andre Amaudruz / Jean-Pierre Martin

  Contents:     48 ch Flash ADC / 25..60msps from J.-P. Martin
                
  $Log: VF48.c,v $
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include "vmicvme.h"
#include "VF48.h"

/********************************************************************/
/**
Read one Event
@param myvme vme structure
@param base  VF48 base address
@return void
*/
int VF48_EventRead(MVME_INTERFACE *myvme, DWORD base, DWORD *pdest, int *nentry)
{
  int cmode, timeout, nframe, channel;
  DWORD hdata;
    
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
 
  *nentry = 0;
  nframe = mvme_read_value(myvme, base+VF48_NFRAME_R);
  printf("Event Nframe : %d - 0x%x\n", nframe, nframe);
  
  timeout=300;  
  if (nframe > 50) {
    do {
      timeout--;
      hdata = mvme_read_value(myvme, base+VF48_DATA_FIFO_R);
    } while (!((hdata & 0xE0000000) == VF48_HEADER) && (timeout)); // skip up to the header
    
    if (timeout == 0) {
      *nentry = 0;
      printf("timeout on header  data:0x%lx\n", hdata);
      mvme_set_dmode(myvme, cmode);
      return VF48_ERR_NODATA;
    }
    channel = (hdata >> 24) & 0xF;
    printf(">>>>>>>>>>>>>>> Channel : %d \n", channel);
    pdest[*nentry] = hdata;
    *nentry += 1;
    timeout=2000;
    do {
      pdest[*nentry] = mvme_read_value(myvme, base+VF48_DATA_FIFO_R);
      timeout--;
      *nentry += 1;
    } while (!((pdest[*nentry-1] & 0xE0000000) == VF48_TRAILER) && timeout); // copy until trailer (included)
    if (timeout == 0) {
      printf("timeout on Trailer  data:0x%lx\n", pdest[*nentry-1]);
      printf("nentry:%d data:0x%lx base:0x%lx \n", *nentry, pdest[*nentry], base+VF48_DATA_FIFO_R);
    }
    nentry--;
  }
  mvme_set_dmode(myvme, cmode);
  return (VF48_SUCCESS);
}

/********************************************************************/
/**
Read requested group (FPGA)
@param myvme vme structure
@param base  VF48 base address
@param pdest Destination pointer
@param grp   group to read
@param nentry number of entries
@return void
*/
int VF48_GroupRead(MVME_INTERFACE *myvme, DWORD base, DWORD *pdest, int grp, int *nentry)
{
  int cmode=0, timeout;
  DWORD hdata;
    
  if (grp >= 6) {
    printf("Group#:%d out of range\n", grp);
    return VF48_ERR_PARM;
  }

  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
 
  // Skip until condition satisfied
  // HEADER AND GROUP match
  *nentry = 0;
  timeout=4000;  
  do {
    timeout--;
    hdata = mvme_read_value(myvme, base+VF48_DATA_FIFO_R);
  } while ((hdata & 0xFF000000) != (VF48_HEADER|((grp&0xF)<<24))
	   && (timeout)); // skip up to the header
    
  if (timeout == 0) {
    *nentry = 0;
    printf("timeout on header data:%lx\n", hdata);
    //    mvme_set_dmode(myvme, cmode);
    //    return VF48_ERR_NODATA;
  }
  // Save header
  //  printf("Group Header:0x%lx\n", hdata);
  pdest[*nentry] = hdata;
  *nentry += 1;

  // Read group event until TRAILER found
  timeout=2000;
  do {
    pdest[*nentry] = mvme_read_value(myvme, base+VF48_DATA_FIFO_R);
    timeout--;
    *nentry += 1;
  } while (!((pdest[*nentry-1] & 0xE0000000) == VF48_TRAILER) && timeout); // copy until trailer (included)
  if (timeout == 0) {
    printf("timeout on Trailer nentry:%d data-1:0x%lx data:0x%lx base:0x%lx \n"
	   , *nentry,  pdest[*nentry-1], pdest[*nentry], base+VF48_DATA_FIFO_R);
  }
  nentry--;
  
  mvme_set_dmode(myvme, cmode);
  return (VF48_SUCCESS);
}

/********************************************************************/
/**
Read N entries from the VF48 data FIFO
@param myvme vme structure
@param base  VF48 base address
@param pdest Destination pointer
@return nentry
*/
int VF48_DataRead(MVME_INTERFACE *myvme, DWORD base, DWORD *pdest, int *nentry)
{
  int cmode, status;  

  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
 
  status = mvme_read_value(myvme, base+VF48_NFRAME_R);
  if (*nentry > 256) *nentry = 256;  // limit for the FIFO size
  printf("DataRead:%d\n", *nentry);
  mvme_read(myvme, pdest, base+VF48_DATA_FIFO_R, *nentry*4);
  mvme_set_dmode(myvme, cmode);
  return (*nentry);
}

/********************************************************************/
/**
Set External Trigger enable
@param myvme vme structure
@param base  VF48 base address
@return VF48_SUCCESS
*/
int VF48_ExtTrgSet(MVME_INTERFACE *myvme, DWORD base)
{
  int cmode, csr;  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VF48_CSR_REG_RW);
  csr |= VF48_CSR_EXT_TRIGGER;  // Set
  mvme_write_value(myvme, base+VF48_CSR_REG_RW, csr);
  mvme_set_dmode(myvme, cmode);
  return VF48_SUCCESS;

#if 0
  status = mvme_read_value(myvme, base+VF48_NFRAME_R);
  if (status <= 0xFF) {
    mvme_write_value(myvme, base+VF48_SELECTIVE_SET_W, VF48_CSR_EXT_TRIG_SELECT);
  }
  mvme_set_dmode(myvme, cmode);
  return (status > 0xFF ? VF48_ERR_HW : VF48_SUCCESS);
#endif
}

/********************************************************************/
/**
Clear External Trigger enable
@param myvme vme structure
@param base  VMEIO base address
@return VF48_SUCCESS
*/
int VF48_ExtTrgClr(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, csr;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VF48_CSR_REG_RW);
  csr &= 0xFFFFFF7F;  // Clear
  mvme_write_value(myvme, base+VF48_CSR_REG_RW, csr);
  mvme_set_dmode(myvme, cmode);
  return VF48_SUCCESS;

#if 0
  status = mvme_read_value(myvme, base+VF48_NFRAME_R);
  if (status <= 0xFF) {
    mvme_write_value(myvme, base+VF48_SELECTIVE_CLR_W, VF48_CSR_EXT_TRIG_SELECT);
  }
  mvme_set_dmode(myvme, cmode);
  return (status > 0xFF ? VF48_ERR_HW : VF48_SUCCESS);
#endif
}

/********************************************************************/
void VF48_Reset(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  mvme_write_value(myvme, base+VF48_GLOBAL_RESET_W, 0);
  mvme_set_dmode(myvme, cmode);
}

/********************************************************************/
/**
Set the Pre-trigger time for a given group. Each group (0..5) handles
8 consecutive input channels.
@param myvme vme structure
@param base  VMEIO base address
@param grp   group number (0..5)
@param ptrig pre trigger time in number of clock unit (depend on the sampling speed) 
@return VF48_SUCCESS, VF48_ERR_PARM
*/
int VF48_PreTriggerSet(MVME_INTERFACE *myvme, DWORD base, int grp, int prtr)
{
  int  cmode;
  
  if (grp < 6) {
    mvme_get_dmode(myvme, &cmode);
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_write_value(myvme, base+VF48_PARAM_ID_W, VF48_PRE_TRIGGER | grp<<VF48_GRP_OFFSET);
    mvme_write_value(myvme, base+VF48_PARAM_DATA_RW, prtr);
    mvme_set_dmode(myvme, cmode);
    return VF48_SUCCESS;
  }
  return VF48_ERR_PARM;
}

/********************************************************************/
/**
Set the Segment size length for a given group. Each group (0..5) handles
8 consecutive input channels.
@param myvme vme structure
@param base  VMEIO base address
@param grp   group number (0..5)
@param segsz number of clock unit (depend on the sampling speed) 
@return VF48_SUCCESS, VF48_ERR_PARM
*/
int VF48_SegSizeSet(MVME_INTERFACE *myvme, DWORD base, int grp, int segsz)
{
  int  cmode;
  
  if (grp < 6) {
    mvme_get_dmode(myvme, &cmode);
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_write_value(myvme, base+VF48_PARAM_ID_W, VF48_SEGMENT_SIZE | grp<<VF48_GRP_OFFSET);
    mvme_write_value(myvme, base+VF48_PARAM_DATA_RW, segsz);
    mvme_set_dmode(myvme, cmode);
    return VF48_SUCCESS;
  }
  return VF48_ERR_PARM;
}

/********************************************************************/
/**
Set the Segment size length for a given group. Each group (0..5) handles
8 consecutive input channels.
@param myvme vme structure
@param base  VMEIO base address
@param grp   group number (0..5)
@param segsz number of clock unit (depend on the sampling speed) 
@return VF48_SUCCESS, VF48_ERR_PARM
*/
int VF48_SegSizeRead(MVME_INTERFACE *myvme, DWORD base, int grp)
{
  int  cmode, segsz, j;
  
  if (grp < 6) {
    mvme_get_dmode(myvme, &cmode);
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    j = VF48_PARMA_BIT_RD | VF48_SEGMENT_SIZE | grp<<VF48_GRP_OFFSET;
    printf("J:0x%x\n", j);
    mvme_write_value(myvme, base+VF48_PARAM_ID_W, j);
    mvme_write_value(myvme, base+VF48_PARAM_DATA_RW, 0);
    segsz = mvme_read_value(myvme, base+VF48_PARAM_DATA_RW);
    mvme_set_dmode(myvme, cmode);
    return segsz;
  }
  return VF48_ERR_PARM;
}

/********************************************************************/
/**
Set the hit threshold for a given group. Each group (0..5) handles
8 consecutive input channels.
@param myvme vme structure
@param base  VMEIO base address
@param grp   group number (0..5)
@param hth   Hit threshold in bin count (10bit ADCs)
@return VF48_SUCCESS, VF48_ERR_PARM
*/
int VF48_HitThresholdSet(MVME_INTERFACE *myvme, DWORD base, int grp, int hth)
{
  int  cmode;
  
  if (grp < 6) {
    mvme_get_dmode(myvme, &cmode);
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_write_value(myvme, base+VF48_PARAM_ID_W, VF48_THRESHOLD | grp<<VF48_GRP_OFFSET);
    mvme_write_value(myvme, base+VF48_PARAM_DATA_RW, hth);
    mvme_set_dmode(myvme, cmode);
    return VF48_SUCCESS;
  }
  return VF48_ERR_PARM;
}

/********************************************************************/
int VF48_LatencySet(MVME_INTERFACE *myvme, DWORD base, int grp, int lat)
{
  int  cmode;
  
  if (grp < 6) {
    mvme_get_dmode(myvme, &cmode);
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_write_value(myvme, base+VF48_PARAM_ID_W, VF48_LATENCY | grp<<VF48_GRP_OFFSET);
    mvme_write_value(myvme, base+VF48_PARAM_DATA_RW, lat);
    mvme_set_dmode(myvme, cmode);
    return VF48_SUCCESS;
  }
  return VF48_ERR_PARM;
}

/********************************************************************/
int VF48_AcqStart(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, csr;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VF48_CSR_REG_RW);
  csr |= VF48_CSR_START_ACQ;  // start ACQ
  mvme_write_value(myvme, base+VF48_CSR_REG_RW, csr);
  mvme_set_dmode(myvme, cmode);
  return VF48_SUCCESS;	

#if 0
  status = mvme_read_value(myvme, base+VF48_NFRAME_R);
  if (status <= 0xFF) {
    mvme_write_value(myvme, base+VF48_SELECTIVE_SET_W, VF48_CSR_ACTIVE_ACQ);
  }
  mvme_set_dmode(myvme, cmode);
  return (status > 0xFF ? VF48_ERR_HW : VF48_SUCCESS);
#endif
}

/********************************************************************/
int VF48_AcqStop(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, csr;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VF48_CSR_REG_RW);
  csr &= ~(VF48_CSR_START_ACQ);  // stop ACQ
  mvme_write_value(myvme, base+VF48_CSR_REG_RW, csr);
  mvme_set_dmode(myvme, cmode);
  return VF48_SUCCESS;

#if 0
  status = mvme_read_value(myvme, base+VF48_NFRAME_R);
  if (status <= 0xFF) {
    mvme_write_value(myvme, base+VF48_SELECTIVE_CLR_W, VF48_CSR_ACTIVE_ACQ);
  }	
  mvme_set_dmode(myvme, cmode);
  return (status > 0xFF ? VF48_ERR_HW : VF48_SUCCESS);
#endif
}

/********************************************************************/
int VF48_ExtTrigSet(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, csr;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VF48_CSR_REG_RW);
  csr |= (VF48_CSR_EXT_TRIGGER);  // set
  mvme_write_value(myvme, base+VF48_CSR_REG_RW, csr);
  mvme_set_dmode(myvme, cmode);
  return VF48_SUCCESS;
}

/********************************************************************/
int VF48_ExtTrigClr(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, csr;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VF48_CSR_REG_RW);
  csr &= ~(VF48_CSR_EXT_TRIGGER);  // clr
  mvme_write_value(myvme, base+VF48_CSR_REG_RW, csr);
  mvme_set_dmode(myvme, cmode);
  return VF48_SUCCESS;
}

/********************************************************************/
int VF48_NFrameRead(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, nframe;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  nframe =  mvme_read_value(myvme, base+VF48_NFRAME_R);
  mvme_set_dmode(myvme, cmode);
  return nframe;
}

/********************************************************************/
int VF48_CsrRead(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, csr;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D16);
  csr = mvme_read_value(myvme, base+VF48_CSR_REG_RW);
  mvme_set_dmode(myvme, cmode);
  return csr;
}

/********************************************************************/
int VF48_FeFull(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, fefull;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  fefull = mvme_read_value(myvme, base+VF48_CSR_REG_RW) & VF48_CSR_FE_FULL;
  mvme_set_dmode(myvme, cmode);
  return fefull;
}

/********************************************************************/
int VF48_FeNotEmpty(MVME_INTERFACE *myvme, DWORD base)
{
  int  cmode, fenotempty;
  
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  fenotempty = mvme_read_value(myvme, base+VF48_CSR_REG_RW) & VF48_CSR_FE_NOTEMPTY;
  mvme_set_dmode(myvme, cmode);
  return fenotempty;
}

/********************************************************************/
#ifdef MAIN_ENABLE
int main () {
  
  MVME_INTERFACE *myvme;

  DWORD VF48_BASE = 0xA00000;
  DWORD buf[1000];
  int status, csr, i;
  int data=0xf;

  // Test under vmic   
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D16);

  VF48_Reset(myvme, VF48_BASE);

  csr = VF48_CsrRead(myvme, VF48_BASE);
  printf("CSR Buffer: 0x%x\n", csr);

  VF48_ExtTrgSet(myvme, VF48_BASE);

  VF48_AcqStart(myvme, VF48_BASE);

  csr = VF48_CsrRead(myvme, VF48_BASE);
  printf("CSR Buffer: 0x%x\n", csr);

  for (i=0;i<6;i++)
    VF48_SegSizeSet(myvme, VF48_BASE, i, 16);

  for (i=0;i<6;i++)
    printf("SegSize grp %i:%d\n", i, VF48_SegSizeRead(myvme, VF48_BASE, i));

  do {
    // Read Frame counter
    data = VF48_NFrameRead(myvme, VF48_BASE);
  } while (data < 128);
  printf("NFrame: 0x%x\n", data);

  /*
  VF48_DataRead(myvme, VF48_BASE, (DWORD *)&buf, &data);
  for (i=0;i<data;i++) {
    printf("data: 0x%x  0x%x  0x%lx\n", data, i, buf[i]);
  }
  printf("End of Data\n");
  */

  VF48_EventRead(myvme, VF48_BASE, (DWORD *)&buf, &data);
  for (i=0;i<data;i++) {
    printf("data: 0x%x  0x%x  0x%lx\n", data, i, buf[i]);
  }
  VF48_EventRead(myvme, VF48_BASE, (DWORD *)&buf, &data);
  for (i=0;i<data;i++) {
    printf("data: 0x%x  0x%x  0x%lx\n", data, i, buf[i]);
  }
  printf("End of Event\n");
  
  // Close VME   
  status = mvme_close(myvme);
  return 1;
}	
#endif

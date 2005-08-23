/*********************************************************************
  Name:         vmicvme.c
  Created by:   Pierre-Andre Amaudruz
  Created by:   Konstantin Olchanski

  Contents:     VME interface for the VMIC VME board processor
                
  $Log$
  Revision 1.1  2005/08/23 22:29:45  olchanski
  TRIUMF VMIC-VME driver. Requires Linux drivers library from VMIC

*********************************************************************/

#include <stdio.h>
#include <string.h>
#include "vmicvme.h"

DWORD                gbl_am = DEFAULT_AMOD;
vme_bus_handle_t     gbl_bh;

/* Hanlde table for VME access */
VMETABLE vmetable[MAX_VME_SLOTS];

/********************************************************************/
/** 
Initialize the VME access
One bus handle per application.
@return SUCCESS, ERROR              
*/
int mvme_init(void)
{
  /* Going through init once only */
  if(0 > vme_init( (vme_bus_handle_t *) &gbl_bh) ) {
    perror("Error initializing the VMIC VMEbus");
    gbl_bh = 0;
    return(ERROR);
  }
  
  /* valid bus handle */
  
  /* initialize the vmetable */
  memset((char *) &vmetable[0], 0, sizeof(vmetable));
  
  /* create a default master window */
  if ((xmvme_mmap(DEFAULT_SRC_ADD, DEFAULT_NBYTES, DEFAULT_AMOD)) != SUCCESS) {
    perror("cannot create vme map window");
    return(ERROR);
  }
  
  return(SUCCESS);
}

/********************************************************************/
/**
Close and release ALL the opened VME channel.

@return SUCCESS, ERROR              
*/
int mvme_exit(void)
{
  int j;

  /* Close all the window handles */
  for (j=0; j< MAX_VME_SLOTS; j++) {
    if (vmetable[j].valid) {
      vme_master_window_release( (vme_bus_handle_t )gbl_bh
				 , vmetable[j].wh );
      
      memset((char *) &(vmetable[j]), 0, sizeof(vmetable[j]));
    }
  }
  
  /* close the bus handle */
  if (gbl_bh != 0) {
    if (0 > vme_term( (vme_bus_handle_t) gbl_bh)) {
      perror("Error during VME termination");
      return(ERROR);
    }
  }
  return(SUCCESS);
}

/********************************************************************/
/**
Map memory to VME region.
@param vme_addr  VME address to be mapped (VME location).
@param nbytes requested transfer size.
@param am     Address modifier 
@return SUCCESS, ERROR              
*/
int xmvme_mmap(DWORD vme_addr, DWORD nbytes, DWORD am)
{
  int j;
  void *phys_addr = NULL;
  
  /* Find new slot */
  j=0;
  while (vmetable[j].valid)  j++;
  
  if (j < MAX_VME_SLOTS) {
    /* Create a new window */
    vmetable[j].low    = vme_addr;
    vmetable[j].am     = ((am == 0) ? DEFAULT_AMOD : am);
    vmetable[j].nbytes = nbytes;
    
    if(0 > vme_master_window_create( gbl_bh
				     , &(vmetable[j].wh)
				     , vmetable[j].low
				     , vmetable[j].am
				     , vmetable[j].nbytes
				     , VME_CTL_PWEN
				     , NULL) ) {
      perror("Error creating the window");
      return(ERROR);
    }
    
    /* Create the mapped window */
    if(NULL == (vmetable[j].ptr = (DWORD *)vme_master_window_map( gbl_bh, vmetable[j].wh, 0) ) ) {
      perror("Error mapping the window");
      vme_master_window_release( (vme_bus_handle_t )gbl_bh
				 , vmetable[j].wh );
      
      memset((char *) vmetable[j].wh, 0, sizeof(vmetable[j]));
      return(ERROR);
    }

    if (NULL == (phys_addr = vme_master_window_phys_addr (gbl_bh, vmetable[j].wh)))
    {
      perror ("vme_master_window_phys_addr");
    }
    printf("Window physical address = 0x%lx\n", (unsigned long) phys_addr);
    vmetable[j].valid = 1;
    vmetable[j].high  = (vmetable[j].low + vmetable[j].nbytes);
  }
  else {
    /* No more slot available */
    return(ERROR);
  }
  
  return(SUCCESS);
}

/********************************************************************/
/**
Unmap VME region.
@param ptr destination pointer to be unmap (PCI location).
@param nbytes requested transfer size.
@return SUCCESS, ERROR              
*/
int xmvme_unmap(DWORD src, DWORD nbytes)
{
  int j;
  
  /* Search for map window */
  for (j=0; vmetable[j].valid; j++) {
    /* Check the vme address range */
    if ((src == vmetable[j].low) && ((src+nbytes) == vmetable[j].high)) {
      /* window found */
      break;
    }
  }
  if (!vmetable[j].valid) {
    /* address not found => nothing to do */
    return(SUCCESS);
  }
  
  /* Remove map */
  if (vmetable[j].ptr) {
    if (0 > vme_master_window_unmap (gbl_bh, vmetable[j].wh)) {
      perror ("vme_master_window_unmap");
      return(ERROR);
    }
    
    if (0 > vme_master_window_release (gbl_bh, vmetable[j].wh)) {
      perror ("vme_master_window_release");
      return(ERROR);
    }
  }
  
  /* Cleanup slot */
  memset((char *) vmetable[j].wh, 0, sizeof(vmetable[j]));
  
  return(SUCCESS);
}

/********************************************************************/
/**
VME I/O control.
@param req  request.
@param param pointer to parameter struct
@return SUCCESS, ERROR              
*/
int xmvme_ioctl(DWORD req, DWORD *param)
{
  int j;
  DWORD src;
  
  switch(req) {
  case VME_IOCTL_SLOT_GET :
    for (j=0; vmetable[j].valid; j++) {
      printf("\nSlot#: %i\n", j);
      printf("      - Window Handle:          0x%lx\n", (DWORD) vmetable[j].wh);
      printf("      - VME Window              0x%08lx .. 0x%08lx\n"
	     , (DWORD) vmetable[j].low, (DWORD) vmetable[j].high);
      printf("      - Window Size:            %ld (0x%lx)\n"
	     , vmetable[j].nbytes, vmetable[j].nbytes);
      printf("      - Address Modifier    :   0x%lx\n", vmetable[j].am);
      printf("      - Local Mapped Address:   %p\n", vmetable[j].ptr);
    }
    break;
  case VME_IOCTL_AMOD_SET :
    gbl_am = *param;
    break;
  case VME_IOCTL_AMOD_GET :
    *param = gbl_am;
    break;
  case VME_IOCTL_PTR_GET :
    src =  param[0];
  
    /* Search for map window */
    for (j=0; vmetable[j].valid; j++) {
      /* Check the vme address range */
      if ((src >= vmetable[j].low) && (src < vmetable[j].high)) {
	/* window found */
	break;
      }
    }
    if (!vmetable[j].valid) {
      /* address not found => nothing to do */
      return(ERROR);
    }

    param[1] = j;
    param[2] = (DWORD) vmetable[j].ptr;
    
    break;
  default :
    perror("vme_ioctl - Unknown request");
  }

  return(SUCCESS);
}

/********************************************************************/

/**
Get handle (pointer) to a block of VME A24 addresses
@param vmeA24addr VME A24 address
@param numbytes size of the requested block in bytes
@param vmeam VME address modifier
*/
void* mvme_getHandleA24(int crate,mvme_addr_t vmeA24addr,int numbytes,int vmeamod)
{
  int status;
  DWORD param[3]={0,0,0};

  if ((xmvme_mmap(vmeA24addr, numbytes, vmeamod)) != SUCCESS) {
    perror("cannot create vme map window");
    return NULL;
  }

  param[0] = vmeA24addr;
  status = xmvme_ioctl(VME_IOCTL_PTR_GET, param);

  return (void*) param[2];
}

/********************************************************************/
/* For test purpose only */

#ifdef NOSKIP
int main () {
  
  DWORD *pData;
  WORD *pReg;
  int status;
  int  k;
  DWORD param[3]={0,0,0};                 // For reg access

  /* Local variables */
  WORD *src=NULL;
  WORD reg=0;
  
  /* Init VME access */
   status = mvme_init();
   
  /* Map whole V792 region */
  if ((mvme_mmap(V792_BASE, 0x10C0, 0x3d)) != SUCCESS) {
    perror("cannot create vme map window");
    return(ERROR);
  }

   /* Dump the booked handle table */
   status = mvme_ioctl(VME_IOCTL_SLOT_GET, NULL);
   
  /* Retrieve the mapped window matching the base address */
  param[0] = V792_BASE;
  status = mvme_ioctl(VME_IOCTL_PTR_GET, param);
  printf("Base pointer for 0x%x is 0x%8x of slot %d \n"
	 , param[0], param[2], param[1]);

  /* Setup Local Base pointers */
  pReg = (WORD *) (param[2]);
  pData= (DWORD *) param[2];
  printf("pReg : %p - pData : %p \n",pReg, pData);
  
  pReg[V792_INCR_EVT_WO] = 0;
  pReg[V792_INCR_EVT_WO] = 0;

  pReg[V792_THRES_BASE+0] = 1;
  pReg[V792_THRES_BASE+4] = 5;
  pReg[V792_THRES_BASE+12] = 0x13;
  pReg[V792_THRES_BASE+13] = 0x34;
  pReg[V792_THRES_BASE+31] = 0x77;
       
  /* Dump registers */
  for (k=V792_REG_BASE; k < V792_REG_BASE+0x60; k+=1) {
    reg  = pReg[k];
    printf("Offset:0x%04x -  *[%p] = 0x%04x\n", k<<1, &(pReg[k]), reg);
  }

  /* Single Reg access */
  printf("VERSION: 0x%04x \n", pReg[V792_FIRM_REV]);
  printf("CSR1   : 0x%04x \n", pReg[V792_CSR1_RO]);
  pReg[V792_GEO_ADDR_RW] = 0x0;
  printf("GEO    : 0x%04x \n", pReg[V792_GEO_ADDR_RW]);
  pReg[V792_EVT_CNT_RST_WO] = 0;
  /* Acquisition Test Mode */
  /* Set bit 6 */
   pReg[V792_BIT_SET2] = 0x20;
  
  /* Clear bit 6 */
   pReg[V792_BIT_CLEAR2] = 0x20;

  /* Write to Test data in TEST REG */
  for (k=0;k<16;k++) {
     pReg[V792_TEST_EVENT_WO] = (k & 0x1FFF);
     pReg[V792_TEST_EVENT_WO+16] = ((k+128) & 0x1FFF);
  }
  
  /* Acquisition Test Mode  Set bit 6 */
   pReg[V792_BIT_SET2] = 0x20;
  
  pReg[V792_INCR_EVT_WO] = 0;
  
  /* Dump the Data */
  src = (WORD *) (param[2]);
  for (k=0 ; k<32 ; k++) {
   printf("Data[%02d]: 0x%08x \n", k, pData[k]);
  }

  /* Unbook and close VME connection */
  status = mvme_exit();
  return 1;
}
#endif

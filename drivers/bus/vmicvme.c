/*********************************************************************
Name:         vmicvme.c
Created by:   Pierre-Andre Amaudruz

Contents:     VME interface for the VMIC VME board processor
              using the mvmestd VME call convention.

  $Id:$

*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <vme/vme.h>
#include <vme/vme_api.h>
#include "vmicvme.h"

int vmic_mmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t size);
int vmic_unmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t size);
mvme_addr_t vmic_mapcheck (MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes);

/********************************************************************/
/** 
Open a VME channel
One bus handle per crate.
@param *mvme     VME structure.
@param index    VME interface index
@return MVME_SUCCESS, ERROR              
*/
int mvme_open(MVME_INTERFACE **mvme, int index)
{
  /* index can be only 0 for VMIC */
  if (index != 0) {
    perror("Invalid parameter (index != 0");
    (*mvme)->handle = 0;
    return(MVME_INVALID_PARAM);
  }

  /* Allocate MVME_INTERFACE */
  *mvme = (MVME_INTERFACE *) malloc(sizeof(MVME_INTERFACE));
  memset((char *) *mvme, 0, sizeof(MVME_INTERFACE));
  /* Going through init once only */
  if(0 > vme_init( (vme_bus_handle_t *) &((*mvme)->handle))) {
    perror("Error initializing the VMIC VMEbus");
    (*mvme)->handle = 0;
    return(MVME_NO_INTERFACE);
  }

  /* book space for VME table */
  (*mvme)->table = (char *) malloc(MAX_VME_SLOTS * sizeof(VME_TABLE));

  /* initialize the table */
  memset((char *) (*mvme)->table, 0, MAX_VME_SLOTS * sizeof(VME_TABLE));

  /* Set default parameters */
  (*mvme)->am = MVME_AM_DEFAULT;

  /* create a default master window */
  if ((vmic_mmap(*mvme, DEFAULT_SRC_ADD, DEFAULT_NBYTES)) != MVME_SUCCESS) {
    perror("cannot create vme map window");
    return(MVME_ACCESS_ERROR);
  }

  return(MVME_SUCCESS);
}

/********************************************************************/
/**
Close and release ALL the opened VME channel.
@param *mvme     VME structure.
@return MVME_SUCCESS, ERROR              
*/
int mvme_close(MVME_INTERFACE *mvme)
{
  int j;
  VME_TABLE *table;

  table = ((VME_TABLE *)mvme->table);

  /* Close all the window handles */
  for (j=0; j< MAX_VME_SLOTS; j++) {
    if (table[j].valid) {
      vme_master_window_release( (vme_bus_handle_t ) mvme->handle
         , table[j].wh );
    }
  } // scan
  
  /* Free table pointer */
  free (mvme->table);
  mvme->table = NULL;

  /* close the bus handle */
  if (mvme->handle != 0) {
    if (0 > vme_term( (vme_bus_handle_t) mvme->handle)) {
      perror("Error during VME termination");
      return(MVME_ACCESS_ERROR);
    }
  }

  /* Free mvme block */
  free (mvme);
  return(MVME_SUCCESS);
}

/********************************************************************/
/**
VME bus reset.
@param *mvme     VME structure.
@return MVME_SUCCESS, ERROR              
*/
int mvme_sysreset(MVME_INTERFACE *mvme)
{
  if (vme_sysreset((vme_bus_handle_t )mvme->handle)) {
    perror("vmeSysreset");
    return  MVME_ACCESS_ERROR;
  }
  return MVME_SUCCESS;
}

/********************************************************************/
/**
Read from VME bus.
@param *mvme VME structure
@param *dst  destination pointer
@param vme_addr source address (VME location).
@param n_bytes requested transfer size.
@return MVME_SUCCESS, ERROR              
*/
int mvme_read(MVME_INTERFACE *mvme, void *dst, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
  int i;
  mvme_addr_t addr;

  addr = vmic_mapcheck(mvme, vme_addr, n_bytes);

  /* Perform read */
  if (mvme->dmode == MVME_DMODE_D8)
    for (i=0 ; i < n_bytes/sizeof(char) ; i++) 
      *((char *)dst)++  = *((char *)addr)++;

  if (mvme->dmode == MVME_DMODE_D16)
    for (i=0 ; i < n_bytes/sizeof(WORD) ; i++) 
      *((WORD *)dst)++  = *((WORD *)addr)++;

  if (mvme->dmode == MVME_DMODE_D32)
    for (i=0 ; i < n_bytes/sizeof(DWORD) ; i++) 
      *((DWORD *)dst)++  = *((DWORD *)addr)++;

  return(MVME_SUCCESS);
}

/********************************************************************/
Read single data from VME bus.
@param *mvme VME structure
@param vme_addr source address (VME location).
@return MVME_SUCCESS, ERROR              
DWORD mvme_read_value(MVME_INTERFACE *mvme, mvme_addr_t vme_addr)
{
  int i;
  mvme_addr_t addr;
  DWORD dst;

  addr = vmic_mapcheck(mvme, vme_addr, 4);

  /* Perform read */
  if (mvme->dmode == MVME_DMODE_D8)
    (char)dst  = *((char *)addr);

  if (mvme->dmode == MVME_DMODE_D16)
    (WORD)dst  = *((WORD *)addr);

  if (mvme->dmode == MVME_DMODE_D32)
    dst = *((DWORD *)addr);

  return(dst);
}

/********************************************************************/
Write data to VME bus.
@param *mvme VME structure
@param vme_addr source address (VME location).
@param *src source array
@param n_bytes  size of the array in bytes
@return MVME_SUCCESS, ERROR              
int mvme_write(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, void *src, mvme_size_t n_bytes)
{
  int i;
  mvme_addr_t addr;

  addr = vmic_mapcheck(mvme, vme_addr, n_bytes);
  
  /* Perform write */
  if (mvme->dmode == MVME_DMODE_D8)
    for (i=0 ; i < n_bytes/sizeof(char) ; i++) 
      *((char *)addr)++  = *((char *)src)++;

  if (mvme->dmode == MVME_DMODE_D16)
    for (i=0 ; i < n_bytes/sizeof(WORD) ; i++) 
      *((WORD *)addr)++  = *((WORD *)src)++;

  if (mvme->dmode == MVME_DMODE_D32)
    for (i=0 ; i < n_bytes/sizeof(DWORD) ; i++) 
      *((DWORD *)addr)++  = *((DWORD *)src)++;

  return MVME_SUCCESS;
}

/********************************************************************/
Write single data to VME bus.
@param *mvme VME structure
@param vme_addr source address (VME location).
@param value  data to write
@return MVME_SUCCESS, ERROR              
int mvme_write_value(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, DWORD value)
{
  int i;
  mvme_addr_t addr;

  addr = vmic_mapcheck(mvme, vme_addr, 4);
  
  /* Perform write */
  if (mvme->dmode == MVME_DMODE_D8)
    *((char *)addr)  = (char) (value &  0xFF);

  if (mvme->dmode == MVME_DMODE_D16)
    *((WORD *)addr)  = (WORD) (value &  0xFFFF);

  if (mvme->dmode == MVME_DMODE_D32)
    *((DWORD *)addr)  = value;

  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_set_am(MVME_INTERFACE *mvme, int am)
{
  mvme->am = am;
  return MVME_SUCCESS;
}

/********************************************************************/
int EXPRT mvme_get_am(MVME_INTERFACE *mvme, int *am)
{
  *am = mvme->am;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_set_dmode(MVME_INTERFACE *mvme, int dmode)
{
  mvme->dmode = dmode;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_get_dmode(MVME_INTERFACE *mvme, int *dmode)
{
  *dmode = mvme->dmode;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_set_blt(MVME_INTERFACE *mvme, int mode)
{
  printf("Not yet implemeted\n");
  return MVME_UNSUPPORTED;
}

/********************************************************************/
int mvme_get_blt(MVME_INTERFACE *mvme, int *mode)
{
  printf("Not yet implemeted\n");
  return MVME_UNSUPPORTED;
}


/********************************************************************/
int vmic_mmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t size)
{
  int j;
  void *phys_addr = NULL;
  VME_TABLE *table;

  table = (VME_TABLE *) mvme->table;

  /* Find new slot */
  j=0;
  while (table[j].valid)  j++;

  if (j < MAX_VME_SLOTS) {
    /* Create a new window */
    table[j].low    = vme_addr;
    table[j].am     = ((mvme->am == 0) ? MVME_AM_DEFAULT : mvme->am);
    table[j].nbytes = size;

    if(0 > vme_master_window_create( (vme_bus_handle_t )mvme->handle
      , &(table[j].wh)
      , table[j].low
      , table[j].am
      , table[j].nbytes
      , VME_CTL_PWEN
      , NULL) ) {
        perror("Error creating the window");
        return(ERROR);
      }

      /* Create the mapped window */
      if(NULL == (table[j].ptr = (DWORD *)vme_master_window_map((vme_bus_handle_t )mvme->handle, table[j].wh, 0) ) ) {
        perror("Error mapping the window");
        vme_master_window_release( (vme_bus_handle_t )mvme->handle
          , table[j].wh );

  /* Cleanup slot */
  table[j].wh = 0;
        return(ERROR);
      }

      if (NULL == (phys_addr = vme_master_window_phys_addr ((vme_bus_handle_t )mvme->handle, table[j].wh)))
      {
        perror ("vme_master_window_phys_addr");
      }
      printf("Window physical address = 0x%lx\n", (unsigned long) phys_addr);
      table[j].valid = 1;
      table[j].high  = (table[j].low + table[j].nbytes);
  }
  else {
    /* No more slot available */
    return(ERROR);
  }

  return(MVME_SUCCESS);
}

/********************************************************************/
/**
Unmap VME region.
@param *mvmeptr destination pointer to be unmap (PCI location).
@param nbytes requested transfer size.
@return MVME_SUCCESS, ERROR              
*/
int vmic_unmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t size)
{
  int j;
  VME_TABLE *table;

  table = (VME_TABLE *) mvme->table;

  /* Search for map window */
  for (j=0; table[j].valid; j++) {
    /* Check the vme address range */
    if ((vme_addr == table[j].low) && ((vme_addr+size) == table[j].high)) {
      /* window found */
      break;
    }
  }
  if (!table[j].valid) {
    /* address not found => nothing to do */
    return(MVME_SUCCESS);
  }

  /* Remove map */
  if (table[j].ptr) {
    if (0 > vme_master_window_unmap ((vme_bus_handle_t )mvme->handle, table[j].wh)) {
      perror ("vme_master_window_unmap");
      return(ERROR);
    }

    if (0 > vme_master_window_release ((vme_bus_handle_t )mvme->handle, table[j].wh)) {
      perror ("vme_master_window_release");
      return(ERROR);
    }
  }

  /* Cleanup slot */
  table[j].wh = 0;

  return(MVME_SUCCESS);
}

/********************************************************************/
mvme_addr_t vmic_mapcheck (MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
  int j;
  VME_TABLE *table;
  mvme_addr_t addr;

  table = (VME_TABLE *) mvme->table;
  /* Extract window handle from table based on the VME address to read */
  for (j=0; table[j].valid; j++) {
    
    /* Check for the proper am */
    if (mvme->am != table[j].am)
      continue;
    
    /* Check the vme address range */
    if ((vme_addr >= table[j].low) && ((vme_addr+n_bytes) < table[j].high)) {
      /* valid range */
     break;
    }
  } // scan table
  
  /* Check if handle found or need to create new one */
  if (!table[j].valid) {
    /* Adjust vme_addr start point */
    addr = vme_addr & 0xFFF00000;
    
    /* Create a new window */
    if (( j = vmic_mmap(mvme, addr, DEFAULT_NBYTES)) != MVME_SUCCESS) {
      perror("cannot create vme map window");
      return(MVME_ACCESS_ERROR);
    }
  }

  addr = (mvme_addr_t) (table[j].ptr) + (mvme_addr_t) (vme_addr - table[j].low);
  
  return addr;
}

/********************************************************************/
/* For test purpose only */

#ifdef NOSKIP
int main () {
  int status;
  int data;

  MVME_INTERFACE *myvme;
  int vmeio_status;

  /* Open VME channel */
  status = mvme_open(&myvme, 0);

  /* Reset VME */
  //  status = mvme_sysreset(myvme);

  /* Setup AM */
  status = mvme_set_am(myvme, 0x39);

  /* Setup Data size */
  status = mvme_set_dmode(myvme, MVME_DMODE_D32);

  /* Read VMEIO status */
  status = mvme_read(myvme, &vmeio_status, 0x78001C, 4); 
  printf("VMEIO status : 0x%x\n", vmeio_status);

  /* Write Single value */
  mvme_write_value(myvme, 0x780010, 0x0);

  /* Read Single Value */
  printf("Value : 0x%x\n", mvme_read_value(myvme, 0x780018));

  /* Write Single value */
  mvme_write_value(myvme, 0x780010, 0x3);

  /* Read Single Value */
  printf("Value : 0x%x\n", mvme_read_value(myvme, 0x780018));

  /* Write to the VMEIO in latch mode */
  for (;;) {
    data = 0xF;
    status = mvme_write(myvme, 0x780010, &data, 4);
    data = 0x0;
    status = mvme_write(myvme, 0x780010, &data, 4);
  }

  /* Close VME channel */
  status = mvme_close(myvme);
  return 1;
}
#endif

/*********************************************************************

  Name:         vxVME.c
  Created by:   Pierre-Andre Amaudruz

  Cotents:      Routines for accessing VME under VxWorks
                
  $Id: vxVME.c 2759 2005-10-10 15:25:44Z ritt $

*********************************************************************/
#include <stdio.h>
#include "vme.h"

#include "vxVME.h"

#ifdef OS_VXWORKS
#include "vxWorks.h"
#include "sysLib.h"
#include "intLib.h"
#include "sys/fcntlcom.h"
#ifdef PPCxxx
#include "arch/ppc/ivPpc.h"
#else
#include "arch/mc68k/ivMc68k.h"
#endif
#include "taskLib.h"

#ifdef PPCxxx
#define A24D24         0xfa000000       /* A24D24 camac base address */
#define A24D16         0xfa000002       /* A24D16 camac base address */
#else
#define A24D24         0xf0000000       /* A24D24 camac base address */
#define A24D16         0xf0000002       /* A24D16 camac base address */
#endif
#endif

#include    "mvmestd.h"

/********************************************************************/
/**
Retrieve mapped address from mapped table.
Check if the given address belongs to an existing map. If not will
create a new map based on the address, current address modifier and
the given size in bytes.
@param *mvme VME structure
@param  nbytes requested transfer size.
@return MVME_SUCCESS, ERROR
*/
mvme_addr_t vxworks_mapcheck (MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes)
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
  }
  /* Check if handle found or need to create new one */
  if (!table[j].valid) {
    /* Adjust vme_addr start point */
    addr = vme_addr & 0xFFF00000;
    /* Create a new window */
    if (vxworks_mmap(mvme, addr, DEFAULT_NBYTES) != MVME_SUCCESS) {
      perror("cannot create vme map window");
      return(MVME_ACCESS_ERROR);
    }
  }
  /* Get index in table in case new mapping has been requested */
  for (j=0; table[j].valid; j++) {
    /* Check for the proper am */
    if (mvme->am != table[j].am)
      continue;
    /* Check the vme address range */
    if ((vme_addr >= table[j].low) && ((vme_addr+n_bytes) < table[j].high)) {
      /* valid range */
      break;
    }
  }
  if (!table[j].valid) {
    perror("Map not found");
    return MVME_ACCESS_ERROR;
  }
  addr = (mvme_addr_t) (table[j].ptr) + (mvme_addr_t) (vme_addr - table[j].low);
  return addr;
}


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
  /* Allocate MVME_INTERFACE */
  *mvme = (MVME_INTERFACE *) malloc(sizeof(MVME_INTERFACE));
  memset((char *) *mvme, 0, sizeof(MVME_INTERFACE));
  
  /* Allocte VME_TABLE */
  (*mvme)->table = (char *) malloc(MAX_VME_SLOTS * sizeof(VME_TABLE));
  
  /* initialize the table */
  memset((char *) (*mvme)->table, 0, MAX_VME_SLOTS * sizeof(VME_TABLE));
  
  /* Set default parameters */
  (*mvme)->am = MVME_AM_DEFAULT;
  
  /* create a default master window */
  if ((vxworks_mmap(*mvme, DEFAULT_SRC_ADD, DEFAULT_NBYTES)) != MVME_SUCCESS) {
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
  /* debug */
  printf("mvme_close\n");
  
  /* Free table pointer */
  free (mvme->table);
  
  mvme->table = NULL;
  
  /* Free mvme block */
  free (mvme);
  
  return(MVME_SUCCESS);
}

/********************************************************************/
/**
Read single data from VME bus.
@param *mvme VME structure
@param vme_addr source address (VME location).
@return value return VME data
*/
DWORD mvme_read_value(MVME_INTERFACE *mvme, mvme_addr_t vme_addr)
{
  mvme_addr_t addr;
  DWORD dst = 0;
  
  addr = vxworks_mapcheck(mvme, vme_addr, 4);

  /* Perform read */
  if (mvme->dmode == MVME_DMODE_D8)
    dst  = *((char *)addr);
  else if (mvme->dmode == MVME_DMODE_D16)
    dst  = *((WORD *)addr);
  else if (mvme->dmode == MVME_DMODE_D32)
    dst = *((DWORD *)addr);

  return dst;
}



/********************************************************************/
/**
Write single data to VME bus.
@param *mvme VME structure
@param vme_addr destination address (VME location).
@param value  data to write to the VME address.
@return MVME_SUCCESS
*/
int mvme_write_value(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, DWORD value)
{
  mvme_addr_t addr;
  
  addr = vxworks_mapcheck(mvme, vme_addr, 4);
  
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

/*------------------------------------------------------------------*/
int vxworks_mmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
  int status;
  int j;
  VME_TABLE *table;
  
  table = (VME_TABLE *) mvme->table;
  
  /* Find new slot */
  j=0;
  while (table[j].valid)  j++;
  
  if (j < MAX_VME_SLOTS) {
    /* Create a new window */
    table[j].low    = vme_addr;
    table[j].am     = ((mvme->am == 0) ? MVME_AM_DEFAULT : mvme->am);
    table[j].nbytes = n_bytes;
    
    status = sysBusToLocalAdrs(VME_AM_STD_USR_DATA, (char *)table[j].low, (char **)&(table[j].ptr));
 
    if (status == ERROR) {
      memset(&table[j], 0, sizeof(VME_TABLE));
      return(MVME_ACCESS_ERROR);
    }

    table[j].valid = 1;
    table[j].high  = (table[j].low + table[j].nbytes);
  }
  else {
    /* No more slot available */
    return(MVME_ACCESS_ERROR);
  }
  
  return(MVME_SUCCESS);
}


/*********************************************************************
Name:         vmicvme.c
Created by:   Pierre-Andre Amaudruz

Contents:     VME interface for the VMIC VME board processor
              using the mvmestd VME call convention.

$Id$

*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "vmicvme.h"

vme_bus_handle_t bus_handle;
vme_interrupt_handle_t int_handle;

struct sigevent event;
struct sigaction handler_act;

INT_INFO int_info;

int vmicvme_max_dma_nbytes = DEFAULT_DMA_NBYTES;

mvme_size_t FullWsze (int am);
int vmic_mmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t size);
int vmic_unmap(MVME_INTERFACE *mvme, mvme_addr_t vmE_addr, mvme_size_t size);
mvme_addr_t vmic_mapcheck (MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes);

#define A32_CHUNK 0x00FFFFFF

/********************************************************************/
/**
Return the largest window size based on the address modifier.
For A32, VMIC cannot map a full 32bit address space.
We map it in chunks of 16 Mbytes instead
*/
mvme_size_t FullWsze (int am) {
  switch (am & 0xF0) {
  case 0x00:
    return A32_CHUNK; // map A32 space in chunks of 16Mibytes
  case 0x30:
    return 0xFFFFFF; // map all of the A24 address space: 16Mibytes
  case 0x20:
    return 0xFFFF; // map all of the A16 address space: 64kibytes
  default:
    return 0;
  }
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
  DMA_INFO *info;
  
  /* index can be only 0 for VMIC */
  if (index != 0) {
    perror("Invalid parameter (index != 0");
    (*mvme)->handle = 0;
    return(MVME_INVALID_PARAM);
  }
  
  /* Allocate MVME_INTERFACE */
  *mvme = (MVME_INTERFACE *) calloc(1, sizeof(MVME_INTERFACE));
  
  /* Going through init once only */
  if(0 > vme_init( (vme_bus_handle_t *) &((*mvme)->handle))) {
    perror("Error initializing the VMIC VMEbus");
    (*mvme)->handle = 0;
    return(MVME_NO_INTERFACE);
  }
  
  /* Allocte VME_TABLE */
  (*mvme)->table = (char *) calloc(MAX_VME_SLOTS, sizeof(VME_TABLE));
  
  /* Set default parameters */
  (*mvme)->am = MVME_AM_DEFAULT;

  /* create a default master window */
  if (vmic_mmap(*mvme, DEFAULT_SRC_ADD, FullWsze((*mvme)->am)) != MVME_SUCCESS) {
    printf("mvme_open: cannot create vme map window");
    return(MVME_ACCESS_ERROR);
  }
  
  /* Allocate DMA_INFO */
  info = (DMA_INFO *) calloc(1, sizeof(DMA_INFO));
  
  (*mvme)->info = info;

  while (1) {

    /* Create DMA buffer */
    int status = vme_dma_buffer_create( (vme_bus_handle_t) (*mvme)->handle
                                        , &(info->dma_handle)
                                        , vmicvme_max_dma_nbytes, 0, NULL);
    if (status >= 0)
      break;

    fprintf(stderr,"mvme_open: Cannot create VME DMA buffer of size %d, errno %d (%s)\n", vmicvme_max_dma_nbytes, errno, strerror(errno));
    vmicvme_max_dma_nbytes /= 2;

    if (vmicvme_max_dma_nbytes >= 1024) {
      fprintf(stderr,"mvme_open: Allocated VME DMA buffer of size %d\n", vmicvme_max_dma_nbytes);
      continue;
    }

    assert(!"Cannot allocate VME DMA buffer!");
  }
  
  if(NULL == (info->dma_ptr = vme_dma_buffer_map(
             (vme_bus_handle_t) (*mvme)->handle
             , info->dma_handle
             , 0)))
    {
      perror("mvme_open: Error mapping the DMA buffer");
      vme_dma_buffer_release((vme_bus_handle_t) (*mvme)->handle, info->dma_handle);
      assert(!"Cannot map the DMA buffer!");
      return(ERROR);
    }

  printf("mvme_open:\n");
  printf("Bus handle              = 0x%x\n", (*mvme)->handle);
  printf("DMA handle              = 0x%x\n", (int)info->dma_handle);
  printf("DMA area size           = %d bytes\n", vmicvme_max_dma_nbytes);
  printf("DMA    physical address = %p\n", (unsigned long *) info->dma_ptr);
  
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
  DMA_INFO  *info;
  
  table = ((VME_TABLE *)mvme->table);
  info  = ((DMA_INFO  *)mvme->info);
  
  // Debug
  printf("mvme_close:\n");
  printf("Bus handle              = 0x%x\n", mvme->handle);
  printf("DMA handle              = 0x%x\n", (int)info->dma_handle);
  printf("DMA    physical address = %p\n", (unsigned long *) info->dma_ptr);
  
  /*------------------------------------------------------------*/
  /* Close DMA channel */
  /*-PAA- The code below generate a crash on the VMIC7805 running
    Linux 2.6.12.3. Seems to be traced to a wrong link list handling
    in vme_dma.c module.
    Running without deallocation seems not to cause problem for now.
    Will investigate further.
    status = vme_dma_buffer_unmap((vme_bus_handle_t ) mvme->handle, info->dma_handle);
    if (status > 0) {
    perror("Error unmapping the buffer");
    }
    
    status = vme_dma_buffer_release((vme_bus_handle_t ) mvme->handle, info->dma_handle);
    if (status > 0) {
    perror("Error releasing the buffer");
    }
  */
  
  /* Free info (DMA) allocation */
  free (mvme->info);
  
  /*------------------------------------------------------------*/
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
For the VMIC system this will reboot the CPU!
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
Read from VME bus. Uses MVME_BLT_BLT32 for enabling the DMA
or size to transfer larger the 128 bytes (32 DWORD)
VME access measurement on a VMIC 7805 is :
Single read access : 1us
DMA    read access : 300ns / DMA latency 28us
@param *mvme VME structure
@param *dst  destination pointer
@param vme_addr source address (VME location).
@param n_bytes requested transfer size.
@return MVME_SUCCESS, ERROR
*/
int mvme_read(MVME_INTERFACE *mvme, void *dst, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
  int         status;

  /* Perform read */
  /*--------------- DMA --------------*/
  if ((mvme->blt_mode == MVME_BLT_BLT32) ||
      (mvme->blt_mode == MVME_BLT_MBLT64) ||
      (n_bytes > 127)) {

    int am = mvme->am;

#ifdef DO_TIMING
    struct timeval t1, t2, t3;
    int dt1, dt2;
#endif

    DMA_INFO *info = ((DMA_INFO*)mvme->info);

    if (n_bytes >= vmicvme_max_dma_nbytes)
      {
	fprintf(stderr,"mvme_read: Attempt to DMA %d bytes more than DEFAULT_DMA_NBYTES=%d\n", (int)n_bytes, vmicvme_max_dma_nbytes);
	return(ERROR);
      }

    if (mvme->blt_mode == MVME_BLT_BLT32)
      am = MVME_AM_A24_NB;
    else if (mvme->blt_mode == MVME_BLT_MBLT64)
      am = MVME_AM_A24_NMBLT;

#ifdef DO_TIMING
    gettimeofday(&t1,NULL);
#endif

    status = vme_dma_read((vme_bus_handle_t )mvme->handle
        , info->dma_handle
        , 0
        , vme_addr
        , am
        , n_bytes
        , 0);
    if (status != 0) {
      fprintf(stderr,"mvme_read: dma read: addr 0x%x, am 0x%x, bytes: %d, status: %d\n", vme_addr, mvme->am, n_bytes, status);
      perror("mvme_read: Error in vme_dma_read()");
      return(ERROR);
    }

#ifdef DO_TIMING
    gettimeofday(&t2,NULL);
#endif

    memcpy(dst,info->dma_ptr,n_bytes);

#ifdef DO_TIMING
    gettimeofday(&t3,NULL);

    dt1 = t2.tv_usec - t1.tv_usec;
    dt2 = t3.tv_usec - t2.tv_usec;

    //fprintf(stderr,"dma read: addr 0x%x, am 0x%x, bytes: %d, status: %d, dma time: %d, memcpy time: %d, dma rate: %f, effective rate: %f\n", vme_addr, mvme->am, n_bytes, status, dt1, dt2, (double)n_bytes/(double)dt1,(double)n_bytes/(double)(dt1+dt2));
#endif
  } else {
    int i;
    mvme_addr_t addr;

    /*--------------- Single Access -------------*/
    addr = vmic_mapcheck(mvme, vme_addr, n_bytes);
    
    if (mvme->dmode == MVME_DMODE_D8) {
      char* cdst = (char*)dst;
      char* csrc = (char*)addr;
      for (i=0; i<n_bytes; i+=sizeof(char))
        *cdst++  = *csrc++;
    } else if (mvme->dmode == MVME_DMODE_D16) {
      WORD* sdst = (WORD*)dst;
      WORD* ssrc = (WORD*)addr;
      for (i=0; i<n_bytes; i+=sizeof(WORD))
        *sdst++  = *ssrc++;
    } else if (mvme->dmode == MVME_DMODE_D32) {
      DWORD* ddst = (DWORD*)dst;
      DWORD* dsrc = (DWORD*)addr;
      for (i=0; i<n_bytes; i+=sizeof(DWORD))
        *ddst++  = *dsrc++;
    } else {
      fprintf(stderr,"mvme_read: Invalid dmode %d\n",mvme->dmode);
      return(ERROR);
    }
  }
  
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
  DWORD dst = 0xFFFFFFFF;
  
  addr = vmic_mapcheck(mvme, vme_addr, 4);
  
  /* Perform read */
  if (mvme->dmode == MVME_DMODE_D8)
    dst  = *((char *)addr);
  else if (mvme->dmode == MVME_DMODE_D16)
    dst  = *((WORD *)addr);
  else if (mvme->dmode == MVME_DMODE_D32)
    dst = *((DWORD *)addr);
  
  return(dst);
}

/********************************************************************/
/**
Write data to VME bus. Uses MVME_BLT_BLT32 for enabling the DMA
or size to transfer larger the 128 bytes (32 DWORD)
@param *mvme VME structure
@param vme_addr source address (VME location).
@param *src source array
@param n_bytes  size of the array in bytes
@return MVME_SUCCESS, MVME_ACCESS_ERROR
*/
int mvme_write(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, void *src, mvme_size_t n_bytes)
{
  /* Perform write */
  /*--------------- DMA --------------*/
  if ((mvme->blt_mode == MVME_BLT_BLT32) ||
      (mvme->blt_mode == MVME_BLT_MBLT64) ||
      (n_bytes > 127)) {

    DMA_INFO  *info = ((DMA_INFO  *)mvme->info);

    if (n_bytes >= vmicvme_max_dma_nbytes)
      {
	fprintf(stderr,"mvme_write: Attempt to DMA %d bytes more than DEFAULT_DMA_NBYTES=%d\n", (int)n_bytes, vmicvme_max_dma_nbytes);
	return(ERROR);
      }

    memcpy(info->dma_ptr, src, n_bytes);
    
    if(0 > vme_dma_write((vme_bus_handle_t )mvme->handle
       , info->dma_handle
       , 0
       , vme_addr
       , mvme->am
       , n_bytes
       , 0)) {
      perror("Error writing data");
      return(MVME_ACCESS_ERROR);
    }
  } else {
    int i;
    mvme_addr_t addr;
  
    /*--------------- Single Access -------------*/
    addr = vmic_mapcheck(mvme, vme_addr, n_bytes);
    
    /* Perform write */
    if (mvme->dmode == MVME_DMODE_D8) {
      char*dst = (char*)addr;
      char*sss = (char*)src;
      for (i=0; i<n_bytes; i+=sizeof(char))
	*dst++ = *sss++;
    } else if (mvme->dmode == MVME_DMODE_D16) {
      WORD*dst = (WORD*)addr;
      WORD*sss = (WORD*)src;
      for (i=0; i<n_bytes; i+=sizeof(WORD))
	*dst++ = *sss++;
    } else if (mvme->dmode == MVME_DMODE_D32) {
      DWORD*dst = (DWORD*)addr;
      DWORD*sss = (DWORD*)src;
      for (i=0; i<n_bytes; i+=sizeof(DWORD))
	*dst++ = *sss++;
    } else {
      fprintf(stderr,"mvme_write: Invalid dmode %d\n",mvme->dmode);
      return(ERROR);
    }
  }
  return MVME_SUCCESS;
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
  
  addr = vmic_mapcheck(mvme, vme_addr, 4);
  
  /* Perform write */
  if (mvme->dmode == MVME_DMODE_D8)
    *((char *)addr)  = (char) (value &  0xFF);
  else if (mvme->dmode == MVME_DMODE_D16)
    *((WORD *)addr)  = (WORD) (value &  0xFFFF);
  else if (mvme->dmode == MVME_DMODE_D32)
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
  mvme->blt_mode = mode;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_get_blt(MVME_INTERFACE *mvme, int *mode)
{
  *mode = mvme->blt_mode;
  return MVME_SUCCESS;
}

/********************************************************************/
/*
static void myisr(int sig, siginfo_t * siginfo, void *extra)
{
  fprintf(stderr, "myisr interrupt received \n");
  fprintf(stderr, "interrupt: level:%d Vector:0x%x\n", int_info.level, siginfo->si_value.sival_int & 0xFF);
}
*/
/********************************************************************/
int mvme_interrupt_generate(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  if (vme_interrupt_generate((vme_bus_handle_t )mvme->handle, level, vector)) {
    perror("vme_interrupt_generate");
    return MVME_ERROR;
  }

  return MVME_SUCCESS;
}

/********************************************************************/
/**

<code>
static void handler(int sig, siginfo_t * siginfo, void *extra)
{
	print_intr_msg(level, siginfo->si_value.sival_int);
	done = 0;
}
</endcode>

*/
int mvme_interrupt_attach(MVME_INTERFACE *mvme, int level, int vector
					, void (*isr)(int, void*, void *), void *info)
{
	struct sigevent event;
	struct sigaction handler_act;

	/* Set up sigevent struct */
	event.sigev_signo = SIGIO;
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_value.sival_int = 0;

	/* Install the signal handler */
	sigemptyset(&handler_act.sa_mask);
	handler_act.sa_sigaction = (void*)isr;
	handler_act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGIO, &handler_act, NULL)) {
		perror("sigaction");
		return -1;
	}

	int_info.flags = *((unsigned int *) info);

  if (vme_interrupt_attach((vme_bus_handle_t )mvme->handle
                      , &int_handle, level, vector
                      , int_info.flags, &event)) {
    perror("vme_interrupt_attach");
    return -1;
  }
  int_info.handle = int_handle;
  int_info.level  = level;
  int_info.vector = vector;

  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_detach(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  if (int_info.handle) {
    
    if (vme_interrupt_release((vme_bus_handle_t )mvme->handle
			      , int_info.handle)) {
      perror("vme_interrupt_release");
      return -1;
    }
  }
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_enable(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  //  vme_interrupt_enable((vme_bus_handle_t )mvme->handle,
  //		       int_info.handle);
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_disable(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  return MVME_SUCCESS;
}

/********************************************************************/
/**
VME Memory map, uses the driver MVME_INTERFACE for storing the map information.
@param *mvme VME structure
@param vme_addr source address (VME location).
@param n_bytes  data to write
@return MVME_SUCCESS, MVME_ACCESS_ERROR
*/
int vmic_mmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes)
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
    table[j].nbytes = n_bytes;
    
    /* Create master window */
    if(0 > vme_master_window_create( (vme_bus_handle_t )mvme->handle
             , &(table[j].wh)
             , table[j].low
             , table[j].am
             , table[j].nbytes
             , VME_CTL_PWEN
             , NULL) ) {
      perror("Error creating the window");
      return(MVME_ACCESS_ERROR);
    }
    
    /* Create the mapped window */
    if(NULL == (table[j].ptr = (DWORD *)vme_master_window_map((vme_bus_handle_t )mvme->handle, table[j].wh, 0) ) ) {
      perror("Error mapping the window");
      vme_master_window_release( (vme_bus_handle_t )mvme->handle
         , table[j].wh );
      
      /* Cleanup slot */
      table[j].wh = 0;
      return(MVME_ACCESS_ERROR);
    }
    
    if (NULL == (phys_addr = vme_master_window_phys_addr ((vme_bus_handle_t )mvme->handle, table[j].wh)))
      {
	perror ("vme_master_window_phys_addr");
      }
    fprintf(stderr, "vmic_mmap: Mapped VME AM 0x%02x addr 0x%08x size 0x%08x at address %p\n", table[j].am, (int)vme_addr, (int)n_bytes, phys_addr);
    table[j].valid = 1;
    table[j].high  = (table[j].low + table[j].nbytes);
  }
  else {
    /* No more slot available */
    return(MVME_ACCESS_ERROR);
  }
  
  return(MVME_SUCCESS);
}

/********************************************************************/
/**
Unmap VME region.
VME Memory map, uses the driver MVME_INTERFACE for storing the map information.
@param *mvme VME structure
@param  nbytes requested transfer size.
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
/**
Retrieve mapped address from mapped table.
Check if the given address belongs to an existing map. If not will
create a new map based on the address, current address modifier and
the given size in bytes.
@param *mvme VME structure
@param  nbytes requested transfer size.
@return MVME_SUCCESS, ERROR
*/
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
  }
  // scan table
  /* Check if handle found or need to create new one */
  if (!table[j].valid) {
    /* Adjust vme_addr start point (split in quarters) */
    if (vme_addr > A32_CHUNK) 
      addr = vme_addr & (~A32_CHUNK);
    else
      addr = 0x00000000;
    /* Create a new window */
    if (vmic_mmap(mvme, addr, FullWsze(mvme->am)) != MVME_SUCCESS) {
      perror("cannot create vme map window");
      abort(); // cannot return anything!
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
  } // scan table
  if (!table[j].valid) {
    perror("Map not found");
    abort(); // cannot return anything!
  }
  addr = (mvme_addr_t) (table[j].ptr) + (mvme_addr_t) (vme_addr - table[j].low);
  return addr;
}

/********************************************************************/
/*-PAA- For test purpose only
  Allows code testing, in this case, the VMEIO board has been used. */

#ifdef MAIN_ENABLE

int main () {

  int status;
  int myinfo = VME_INTERRUPT_SIGEVENT;

  MVME_INTERFACE *myvme;
  int vmeio_status;
  
  /* Open VME channel */
  status = mvme_open(&myvme, 0);
  
  /* Reset VME */
  //  status = mvme_sysreset(myvme);
  
  /* Setup AM */
  status = mvme_set_am(myvme, 0x09);
  
  /* Setup Data size */
  status = mvme_set_dmode(myvme, MVME_DMODE_D32);
  
  /* Read VMEIO status */
  status = mvme_read(myvme, &vmeio_status, 0x4078001C, 4);
  printf("VMEIO status : 0x%x\n", vmeio_status);
  
  /* Write Single value */
  mvme_write_value(myvme, 0x40780010, 0x0);
  
  /* Read Single Value */
  printf("Value : 0x%lx\n", mvme_read_value(myvme, 0x40780018));
  
  /* Write Single value */
  mvme_write_value(myvme, 0x40780010, 0x3);
  
  /* Read Single Value */
  printf("Value : 0x%lx\n", mvme_read_value(myvme, 0x40780018));
  
  /* Write Single value */
  mvme_write_value(myvme, 0x40780008, 0xF);
  
  /* Write to the VMEIO in pulse mode */
//  data = 0xF;
//  for (;;) {
//    status = mvme_write(myvme, 0x78000C, &data, 4);
//    status = mvme_write(myvme, 0x78000C, &data, 4);
//  }
  
//------------ Interrupt section
  // IRQ 7, vector from switch 0x0 -> 0x80 -> ch 0 source
  mvme_interrupt_attach(myvme, 7, 0x10, myisr, &myinfo);
  
  //  for (;;) {    
  //  }
  //  mvme_interrupt_enable(myvme, 1, 0x00, &myinfo);
  
  //  mvme_interrupt_disable(myvme, 1, 0x00, &myinfo);
  
  
  mvme_interrupt_generate(myvme, 7, 0x10, &myinfo);


  //  mvme_interrupt_detach(myvme, 1, 0x00, &myinfo);

  /* Close VME channel */
  status = mvme_close(myvme);
  return 1;
}

#endif

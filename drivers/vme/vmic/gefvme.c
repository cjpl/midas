/*********************************************************************
 * Name:         gefvme.c
 * Created by:   Konstantin Olchanski, Pierre-Andre Amaudruz
 *
 * Contents:     VME interface for the GE FANUC (VMIC) VME board processor
 *               using gefvme Linux driver.
 *
 *  $Id$
 *
*********************************************************************/

//#define _XOPEN_SOURCE 500
#define  _GNU_SOURCE 1

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "gefvme.h"

#include "vmedrv.h"

#define MAX_VME_SLOTS 7

typedef struct {
  int          handle;
  int              am;
  mvme_size_t  nbytes;
  void          *addr;
  mvme_addr_t     low;
  mvme_addr_t    high;
  int           valid;
} VME_TABLE;

/********************************************************************/
/**
VME Memory map, uses the driver MVME_INTERFACE for storing the map information.
@param *mvme VME structure
@param vme_addr source address (VME location).
@param n_bytes  data to write
@return MVME_SUCCESS, MVME_ACCESS_ERROR
*/
static int gefvme_openWindow(MVME_INTERFACE *mvme, int am, mvme_addr_t vme_addr, mvme_size_t n_bytes, int* pwindow, char** paddr)
{
  int j;
  VME_TABLE *table;
  char devname[256];
  int fd;
  struct  vmeOutWindowCfg conf;
  int status;
  char* amode = "(unknown)";
  
  table = (VME_TABLE *) mvme->table;

  if (1)
    {
      /* Find new slot */
      for (j=0; j<MAX_VME_SLOTS; j++)
        if (!table[j].valid)
          {
            /* Create a new window */
            
            sprintf(devname, "/dev/vme_m%d", j);
            fd = open(devname, O_RDWR);
            if (fd < 0) {
              if (errno == ENXIO) {
                fprintf(stderr,"gefvme_openWindow: VME driver not present: Cannot open VME device \'%s\', errno %d (%s)\n", devname, errno, strerror(errno));
                exit(1);
              }
              if (errno != EBUSY)
                fprintf(stderr,"gefvme_openWindow: Cannot open VME device \'%s\', errno %d (%s)\n", devname, errno, strerror(errno));
              continue;
            }
            
            break;
          }
    }
  else
    {
      // always use slot 0

      j = 2;

      fd = table[j].handle;

      if (!table[j].valid)
        {
          /* Create a new window */
          
          sprintf(devname, "/dev/vme_m%d", j);
          fd = open(devname, O_RDWR);
          assert(fd >= 0);
        }
    }

  printf("gefvme_openWindow: Slot %d, VME addr 0x%08x, am 0x%02x, size 0x%08x %d\n", j, vme_addr, table[j].am, n_bytes, n_bytes);
        
  if (j >= MAX_VME_SLOTS) {
    /* No more slot available */
    fprintf(stderr,"gefvme_openWindow: All VME slots are in use!\n");
    assert(!"Cannot continue!\n");
    return MVME_ACCESS_ERROR;
  }

  table[j].am     = ((am == 0) ? MVME_AM_DEFAULT : am);
  table[j].handle = fd;

  memset(&conf, 0, sizeof(conf));

  switch (table[j].am)
    {
    default:
      fprintf(stderr,"gefvme_openWindow: Do not know how to handle VME AM 0x%x\n", table[j].am);
      abort();
      return MVME_ACCESS_ERROR;
    case MVME_AM_A24:
    case MVME_AM_A24_ND:
      vme_addr = 0;
      n_bytes = (1<<24);
      conf.addrSpace = VME_A24;       /*  Address Space */
      conf.userAccessType = VME_USER; /*  User/Supervisor Access Type */
      conf.dataAccessType = VME_DATA; /*  Data/Program Access Type */
      amode = "A24";
      break;
    case MVME_AM_A32_ND:
    case MVME_AM_A32_SD:
      //n_bytes = 0x01000000;
      n_bytes = 0x00800000;
      {
        char* env = getenv("GEFVME_A32_SIZE");
        if (env)
          {
            n_bytes = strtoul(env, NULL, 0);
            fprintf(stderr,"gefvme: A32 window size set to 0x%x\n", n_bytes);
          }
      }
      vme_addr = vme_addr - vme_addr%n_bytes;
      conf.addrSpace = VME_A32;       /*  Address Space */
      conf.userAccessType = VME_USER; /*  User/Supervisor Access Type */
      conf.dataAccessType = VME_DATA; /*  Data/Program Access Type */
      amode = "A32";
      break;
    }

  table[j].low    = vme_addr;
  table[j].nbytes = n_bytes;

  table[j].addr   = NULL;

  conf.windowNbr = j;    /*  Window Number */
  conf.windowEnable = 1; /*  State of Window */
  conf.pciBusAddrU = 0;  /*  Start Address on the PCI Bus */
  conf.pciBusAddrL = 0;  /*  Start Address on the PCI Bus */
  conf.windowSizeU = 0;           /*  Window Size */
  conf.windowSizeL = n_bytes;     /*  Window Size */
  conf.xlatedAddrU = 0;           /*  Starting Address on the VMEbus */
  conf.xlatedAddrL = vme_addr;    /*  Starting Address on the VMEbus */
  conf.bcastSelect2esst = 0;      /*  2eSST Broadcast Select */
  conf.wrPostEnable = 0;          /*  Write Post State */
  conf.prefetchEnable = 0; /*  Prefetch Read Enable State */
  conf.prefetchSize = 0;   /*  Prefetch Read Size (in Cache Lines) */
  conf.xferRate2esst = VME_SSTNONE;  /*  2eSST Transfer Rate */
  conf.maxDataWidth  = VME_D32;      /*  Maximum Data Width */
  conf.xferProtocol  = 0; // VME_SCT;   /*  Transfer Protocol */
  conf.reserved = 0;       /* For future use */

  status = ioctl(fd, VME_IOCTL_SET_OUTBOUND, &conf);
  if (status != 0) {
    fprintf(stderr,"gefvme_openWindow: cannot VME_IOCTL_SET_OUTBOUND, errno %d (%s)\n", errno, strerror(errno));
    abort();
    return MVME_ACCESS_ERROR;
  }

  void* addr = mmap(NULL, n_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  if (addr == (void*)(-1)) {
    fprintf(stderr,"gefvme_openWindow: cannot mmap() VME address space, errno %d (%s)\n", errno, strerror(errno));
    abort();
    return MVME_ACCESS_ERROR;
  }

  table[j].addr = addr;

  fprintf(stderr, "gefvme_openWindow: Configured window %d VME AM 0x%02x %s addr 0x%08x size 0x%08x fd %d\n", j, table[j].am, amode, (int)vme_addr, (int)n_bytes, table[j].handle);

  table[j].valid = 1;
  table[j].high  = (table[j].low + table[j].nbytes);

  if (pwindow)
    *pwindow = j;

  if (paddr)
    *paddr = addr;
  
  return MVME_SUCCESS;
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
static int gefvme_mapcheck(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes, int *pfd, int *poffset, char **paddr)
{
  int status;
  int j;
  VME_TABLE *table;
  table = (VME_TABLE *) mvme->table;

  /* Look for existing mapping */

  for (j=0; j<MAX_VME_SLOTS; j++)
    if (table[j].valid) {    /* Check for the proper am */
      if (mvme->am != table[j].am)
        continue;
      /* Check the vme address range */
      if ((vme_addr >= table[j].low) && ((vme_addr+n_bytes) < table[j].high)) {
        /* valid range */
        break;
      }
    }

  /* Create a new mapping if needed */

  if (j>=MAX_VME_SLOTS) {
    status = gefvme_openWindow(mvme, mvme->am, vme_addr, n_bytes, &j, NULL);
    if (status != MVME_SUCCESS)
      return status;
  }

  if (pfd)
    *pfd = table[j].handle;
  if (poffset)
    *poffset = vme_addr - table[j].low;
  if (paddr)
    *paddr = table[j].addr;

  return MVME_SUCCESS;
}

// gefvme specific functions

char* gefvme_get_a16(MVME_INTERFACE *mvme)
{
  char* addr;
  int status = gefvme_openWindow(mvme, MVME_AM_A16, 0, 0x10000, NULL, &addr);
  if (status != MVME_SUCCESS)
    return NULL;
  return addr;
}

char* gefvme_get_a24(MVME_INTERFACE *mvme)
{
  char* addr;
  int status = gefvme_openWindow(mvme, MVME_AM_A24, 0, 0x1000000, NULL, &addr);
  if (status != MVME_SUCCESS)
    return NULL;
  return addr;
}

char* gefvme_get_a32(MVME_INTERFACE *mvme, uint32_t vmeaddr, int size)
{
  char* addr;
  int status = gefvme_openWindow(mvme, MVME_AM_A32, vmeaddr, size, NULL, &addr);
  if (status != MVME_SUCCESS)
    return NULL;
  return addr;
}

int gefvme_dma_debug   = 0;

static int makeDmaPacket(vmeDmaPacket_t *pkt, int blt_mode, int am, uint32_t vme_addr, char* dst_addr, int nbytes)
{

  memset(pkt, 0, sizeof(*pkt));
  
  pkt->maxPciBlockSize = 16*1024;
  pkt->maxVmeBlockSize = 16*1024;
  pkt->byteCount = nbytes;
  pkt->srcBus = VME_DMA_VME;
  pkt->srcAddr = vme_addr;
  pkt->srcAddrU = 0; // limited to A24 and A32. No A64, please!
  
  switch (blt_mode)
    {
    case MVME_BLT_BLT32:
      assert(vme_addr%4 == 0);
      assert(nbytes%4 == 0);
      pkt->srcVmeAttr.maxDataWidth = VME_D32;
      pkt->srcVmeAttr.xferProtocol = VME_BLT;
      break;
    case MVME_BLT_MBLT64:
    case MVME_AM_A24_SMBLT:
    case MVME_AM_A24_NMBLT:
    case MVME_AM_A32_SMBLT:
    case MVME_AM_A32_NMBLT:
      assert(vme_addr%8 == 0);
      assert(nbytes%8 == 0);
      //pkt->srcVmeAttr.maxDataWidth = VME_D64;
      pkt->srcVmeAttr.maxDataWidth = VME_D32;
      pkt->srcVmeAttr.xferProtocol = VME_MBLT;
      break;
    default:
      fprintf(stderr,"makeDmaPacket: Unknown mvme->blt_mode DMA mode %d\n", blt_mode);
      assert(!"makeDmaPacket: Unknown DMA mode!");
    }
  
  switch (am)
    {
    default:
      fprintf(stderr,"makeDmaPacket: Unknown mvme->am VMA AM 0x%x\n", am);
      assert(!"makeDmaPacket: Unknown VME AM DMA mode!");
      return MVME_ACCESS_ERROR;
    case MVME_AM_A24:
    case MVME_AM_A24_ND:
    case MVME_AM_A24_SMBLT:
    case MVME_AM_A24_NMBLT:
      pkt->srcVmeAttr.addrSpace = VME_A24;
      break;
    case MVME_AM_A32_ND:
    case MVME_AM_A32_SD:
    case MVME_AM_A32_SMBLT:
    case MVME_AM_A32_NMBLT:
      pkt->srcVmeAttr.addrSpace = VME_A32;
      break;
    }
  
  //pkt->srcVmeAttr.userAccessType = VME_SUPER;
  pkt->srcVmeAttr.userAccessType = VME_USER;
  pkt->srcVmeAttr.dataAccessType = VME_DATA;
  pkt->dstBus = VME_DMA_USER;
  pkt->dstAddr  = 0xFFFFFFFF & ((uint64_t)dst_addr);
  pkt->dstAddrU = 0xFFFFFFFF & (((uint64_t)dst_addr)>>32);
  pkt->dstVmeAttr.maxDataWidth = 0;
  pkt->dstVmeAttr.addrSpace = 0;
  pkt->dstVmeAttr.userAccessType = 0;
  pkt->dstVmeAttr.dataAccessType = 0;
  pkt->dstVmeAttr.xferProtocol = 0;
  
  pkt->vmeDmaStatus = gefvme_dma_debug;

  return MVME_SUCCESS;
}

static int runDma(int channel, vmeDmaPacket_t *pkt)
{
  char devnode[256];
  int status, fd, xerrno;

  sprintf(devnode, "/dev/vme_dma%d", channel);
  fd = open(devnode, 0);
  if (fd < 0) {
    fprintf(stderr,"gefvme::runDma: Cannot open VME device \'%s\', errno %d (%s)\n", devnode, errno, strerror(errno));
    return -errno;
  }

  errno  = 0;
  status = ioctl(fd, VME_IOCTL_START_DMA, pkt);
  xerrno = errno;

  close(fd);

  if (status < 0)
    return -xerrno;

  return 0;
}

int gefvme_dma_channel = 0;

void  gefvme_set_dma_debug(int debug)
{
  gefvme_dma_debug = debug;
}

void  gefvme_set_dma_channel(int channel)
{
  assert(channel>=0 && channel<=1);
  gefvme_dma_channel = channel;
}

int gefvme_read_dma_multiple(MVME_INTERFACE *mvme, int nseg, void* dstaddr[], const mvme_addr_t vmeaddr[], int nbytes[])
{
  int i;
  int xerrno;
  vmeDmaPacket_t pkt[nseg];
  for (i=0; i<nseg; i++)
    {
      printf("packet %p+%d, blt %d, am 0x%x, vmeaddr 0x%x, dstaddr %p, nbytes %d\n",  pkt+i, sizeof(vmeDmaPacket_t), mvme->blt_mode, mvme->am, vmeaddr[i], dstaddr[i], nbytes[i]);
      makeDmaPacket(pkt+i, mvme->blt_mode, mvme->am, vmeaddr[i], dstaddr[i], nbytes[i]);
      pkt[i].pNextPacket = pkt+i+1;
    }
  pkt[nseg-1].pNextPacket = NULL;

  xerrno = runDma(gefvme_dma_channel, pkt);

  if (xerrno < 0) {
    fprintf(stderr,"gefvme_read_dma_multiple: ioctl(VME_IOCTL_START_DMA) errno %d (%s)\n", -xerrno, strerror(-xerrno));
    return MVME_ACCESS_ERROR;
  }

  if (pkt[0].vmeDmaStatus != 0x02000000) {
    fprintf(stderr,"mvme_read_dma_multiple: ioctl(VME_IOCTL_START_DMA) returned vmeDmaStatus 0x%08x\n", pkt[0].vmeDmaStatus);
    return MVME_ACCESS_ERROR;
  }
  
  return MVME_SUCCESS;
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
  /* index can be only 0 for VMIC */
  if (index != 0) {
    fprintf(stderr,"mvme_open: invalid index value %d, only 0 is permitted\n", index);
    return MVME_INVALID_PARAM;
  }
  
  /* Allocate MVME_INTERFACE */
  *mvme = (MVME_INTERFACE *) calloc(1, sizeof(MVME_INTERFACE));

  (*mvme)->handle = -1;
  
  /* Allocte VME_TABLE */
  (*mvme)->table = (char *) calloc(MAX_VME_SLOTS, sizeof(VME_TABLE));
  
  /* Set default parameters */
  (*mvme)->am = MVME_AM_DEFAULT;

  /* Set default block transfer mode */
  (*mvme)->blt_mode = 0; // 0 == disabled

#if 0
  /* create a default master window */
  if (vmic_mmap(*mvme, DEFAULT_SRC_ADD, FullWsze((*mvme)->am)) != MVME_SUCCESS) {
    printf("mvme_open: cannot create vme map window");
    return(MVME_ACCESS_ERROR);
  }
#endif
  
  return MVME_SUCCESS;
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
  //DMA_INFO  *info;
  
  table = ((VME_TABLE *)mvme->table);
  
  /*------------------------------------------------------------*/
  /* Close all the window handles */
  for (j=0; j< MAX_VME_SLOTS; j++) {
    if (table[j].valid) {
      close(table[j].handle);
    }
  } // scan
  
  /* Free table pointer */
  free (mvme->table);
  mvme->table = NULL;
  
  /* Free mvme block */
  free (mvme);
  
  return MVME_SUCCESS;
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
  assert(!"Not implemented");
  return MVME_SUCCESS;
}

int gefvme_read_dma(MVME_INTERFACE *mvme, void *dst, mvme_addr_t vme_addr, int n_bytes)
{
  int i;
  vmeDmaPacket_t vmeDma;
  char* ptr;
  int xerrno;
  int bytesRead;

  makeDmaPacket(&vmeDma, mvme->blt_mode, mvme->am, vme_addr, dst, n_bytes);
  xerrno = runDma(gefvme_dma_channel, &vmeDma);

  if (0)
    {
      uint32_t *ptr = (uint32_t*)dst;
      int i;
      printf("dst %p\n", dst);
      for (i=0; i<10; i++)
        printf("dst[%d] = 0x%08x\n", i, ptr[i]);
    }
  
  if (0)
    {
      printf("mvme_read_dma: DMA status:\n");
      printf("  requested %d bytes\n", n_bytes);
      printf("  vmeDmaToken: %d\n", vmeDma.vmeDmaToken);
      printf("  vmeDmaWait:  %d\n", vmeDma.vmeDmaWait);
      printf("  vmeDmaStartTick: %d\n", vmeDma.vmeDmaStartTick);
      printf("  vmeDmaStopTick: %d\n", vmeDma.vmeDmaStopTick);
      printf("  vmeDmaElapsedTime: %d\n", vmeDma.vmeDmaElapsedTime);
      printf("  vmeDmaStatus: %d\n", vmeDma.vmeDmaStatus);
      printf("  byte count: %d\n", vmeDma.byteCount);
      //assert(!"Crash here!");
    }

  bytesRead = vmeDma.srcAddr - vme_addr;

  //printf("mvme_read_dma: DMA %6d of %6d bytes from 0x%08x, status 0x%08x, byteCount %d, srcAddr 0x%08x, dstAddr 0x%08x\n", n_bytes, bytesRead, vme_addr, vmeDma.vmeDmaStatus, vmeDma.byteCount, vmeDma.srcAddr, vmeDma.dstAddr);
  
  if (xerrno < 0) {
    fprintf(stderr,"gefvme_read_dma: ioctl(VME_IOCTL_START_DMA) errno %d (%s)\n", -xerrno, strerror(-xerrno));
    return MVME_ACCESS_ERROR;
  }

  if (vmeDma.vmeDmaStatus != 0x02000000) {
    //*((uint32_t*)dst) = 0xdeadbeef;
    printf("mvme_read_dma: DMA %6d of %6d bytes from 0x%08x, status 0x%08x, srcAddr 0x%08x, dstAddr 0x%08x\n", n_bytes, bytesRead, vme_addr, vmeDma.vmeDmaStatus, vmeDma.srcAddr, vmeDma.dstAddr);
    fprintf(stderr,"mvme_read_dma: ioctl(VME_IOCTL_START_DMA) returned vmeDmaStatus 0x%08x\n", vmeDma.vmeDmaStatus);
    return MVME_ACCESS_ERROR;
  }
  
  if (bytesRead!=0 && bytesRead!=n_bytes) {
    printf("mvme_read_dma: DMA %6d of %6d bytes from 0x%08x, status 0x%08x, srcAddr 0x%08x, dstAddr 0x%08x\n", n_bytes, bytesRead, vme_addr, vmeDma.vmeDmaStatus, vmeDma.srcAddr, vmeDma.dstAddr);
    fprintf(stderr,"mvme_read_dma: ioctl(VME_IOCTL_START_DMA) returned byteCount %d while requested read of %d bytes\n", bytesRead, n_bytes);
    return MVME_ACCESS_ERROR;
  }

  if (0)
    {
      int i;
      uint32_t *p32 = (uint32_t*)dst;
      for (i=0; i<10; i++)
        printf("dma at %d is 0x%08x\n", i, p32[i]);
    }
  
  ptr = (char*)dst;
  for (i=0; i<n_bytes; i+=4) {
    char tmp;
    tmp = ptr[i+0];
    ptr[i+0] = ptr[i+3];
    ptr[i+3] = tmp;
    tmp = ptr[i+1];
    ptr[i+1] = ptr[i+2];
    ptr[i+2] = tmp;
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
  int i, fd;
  int offset;
  int status;
  char* addr;

  /* Perform read */
  /*--------------- DMA --------------*/
  switch (mvme->blt_mode) {
  default:
    fprintf(stderr,"mvme_read: Unknown VME block transfer mode %d\n", mvme->blt_mode);
    return MVME_ACCESS_ERROR;

  case MVME_AM_A24_SMBLT:
  case MVME_AM_A24_NMBLT:
  case MVME_AM_A32_SMBLT:
  case MVME_AM_A32_NMBLT:
  case MVME_BLT_BLT32:
  case MVME_BLT_MBLT64:
    return gefvme_read_dma(mvme, dst, vme_addr, n_bytes);

  case 0: // no block transfers
  
    status = gefvme_mapcheck(mvme, vme_addr, n_bytes, &fd, &offset, &addr);
    if (status != MVME_SUCCESS)
      return status;

    addr += offset;

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
      fprintf(stderr,"mvme_read_pio: Invalid dmode %d\n",mvme->dmode);
      return MVME_ACCESS_ERROR;
    }

#if 0
      for (i=0; i<n_bytes; i+=4) {
        char tmp;
        tmp = ptr[i+0];
        ptr[i+0] = ptr[i+3];
        ptr[i+3] = tmp;
        tmp = ptr[i+1];
        ptr[i+1] = ptr[i+2];
        ptr[i+2] = tmp;
      }
#endif

#if 0
    status = pread(fd, dst, n_bytes, offset);
    if (status != n_bytes)
      return MVME_ACCESS_ERROR;

    // NOTE: gefvme PIO block read uses D32 data accesses, so D8 and D16 would not work.
    // If gefvme ever implements mmap(), then we can do D8/D16/D32 PIO
    // block reads...

    if (mvme->dmode == MVME_DMODE_D8) {
      assert(!"mvme_read: VME D8 PIO block read not implemented");
    } else if (mvme->dmode == MVME_DMODE_D16) {
      assert(!"mvme_read: VME D16 PIO block read not implemented");
    } else if (mvme->dmode == MVME_DMODE_D32) {
      int i;
      char *ptr = (char*)dst;
      
      for (i=0; i<n_bytes; i+=4) {
        char tmp;
        tmp = ptr[i+0];
        ptr[i+0] = ptr[i+3];
        ptr[i+3] = tmp;
        tmp = ptr[i+1];
        ptr[i+1] = ptr[i+2];
        ptr[i+2] = tmp;
      }

    } else
      assert(!"mvme_read: Unknown value of mvme->dmode. Only MVME_DMODE_D32 and MVME_DMODE_D64 permitted");
#endif

    return MVME_SUCCESS;
  }

  assert(!"Cannot be reached!");
  return MVME_ACCESS_ERROR;
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
  int status, fd, offset;
  char *addr;
  DWORD dst = 0;
  unsigned char tmp[4];
  
  status = gefvme_mapcheck(mvme, vme_addr, 4, &fd, &offset, &addr);
  if (status != MVME_SUCCESS)
    return 0xFFFFFFFF;
  
  //printf("mvme_read_value: fd %d, vmeaddr 0x%08x, offset %d, addr %p\n", fd, vme_addr, offset, addr);

  addr += offset;
  
  /* Perform read */
  if (mvme->dmode == MVME_DMODE_D8)
    dst  = *((char *)addr);
  else if (mvme->dmode == MVME_DMODE_D16) {
    *((WORD*)tmp) = *((WORD*)addr);
    dst = tmp[0]<<8 | tmp[1];
  }
  else if (mvme->dmode == MVME_DMODE_D32) {
    *((DWORD*)tmp) = *((DWORD*)addr);
    dst = tmp[0]<<24 | tmp[1]<<16 | tmp[2]<<8 | tmp[3];
  }

#if 0  
  /* Perform read */
  if (mvme->dmode == MVME_DMODE_D8) {
    status = pread(fd, &dst, 1, offset);
    if (status != 1) {
      //fprintf(stderr,"mvme_read_value: read() returned %d, errno %d (%s)\n", status, errno, strerror(errno));
      return 0xFF;
    }
  } else if (mvme->dmode == MVME_DMODE_D16) {
    char buf16[2];
    uint16_t dst16;
    status = pread(fd, buf16, 2, offset);
    dst16 = (buf16[1]&0xFF) | (buf16[0]<<8);
    //printf("read16: addr 0x%x, value 0x%02x 0x02x 0x%04x, status %d\n", vme_addr, buf16[0], buf16[1], dst16, status);
    if (status == 2)
      return dst16;

    //fprintf(stderr,"mvme_read_value: read() returned %d, errno %d (%s)\n", status, errno, strerror(errno));
    return 0xFFFF;
  } else if (mvme->dmode == MVME_DMODE_D32) {
    char buf32[4];
    uint32_t dst32;
    status = pread(fd, buf32, 4, offset);
    dst32 = (buf32[3]&0xFF) | (0xFF00&buf32[2]<<8)| (0xFF0000&buf32[1]<<16) | (0xFF000000&buf32[0]<<24);
    //printf("read32: fd %d, addr 0x%x, offset 0x%x, value 0x%02x 0x%02x 0x%02x 0x%02x 0x%08x, status %d\n", fd, vme_addr, offset, buf32[0], buf32[1], buf32[2], buf32[3], dst32, status);
    //exit(1);
    if (status == 4)
      return dst32;

    //fprintf(stderr,"mvme_read_value: read() returned %d, errno %d (%s)\n", status, errno, strerror(errno));
    return 0xFFFFFFFF;
  }
#endif

  return dst;
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
  assert(!"Not implemented");
#if 0
  /* Perform write */
  /*--------------- DMA --------------*/
  if ((mvme->blt_mode == MVME_BLT_BLT32) ||
      (mvme->blt_mode == MVME_BLT_MBLT64) ||
      (n_bytes > 127)) {

    DMA_INFO  *info = ((DMA_INFO  *)mvme->info);

    if (n_bytes >= DEFAULT_DMA_NBYTES)
      {
	fprintf(stderr,"mvme_write: Attempt to DMA %d bytes more than DEFAULT_DMA_NBYTES=%d\n", (int)n_bytes, DEFAULT_DMA_NBYTES);
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
#endif
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
  int status, fd, offset;
  char* addr;
  
  status = gefvme_mapcheck(mvme, vme_addr, 4, &fd, &offset, &addr);
  if (status != MVME_SUCCESS)
    return status;

#if 0
  addr += offset;
  
  /* Perform write */
  if (mvme->dmode == MVME_DMODE_D8)
    *((char *)addr)  = (char) (value &  0xFF);
  else if (mvme->dmode == MVME_DMODE_D16) {
    unsigned char buf[2];
    buf[1] = value & 0xFF;
    buf[0] = value >> 8;
    *((WORD *)addr)  = *(WORD*) buf;
  }
  else if (mvme->dmode == MVME_DMODE_D32) {
    unsigned char buf[4];
    buf[3] = value;
    buf[2] = value >> 8;
    buf[1] = value >> 16;
    buf[0] = value >> 24;
    *((DWORD *)addr)  = *(DWORD*) buf;
  }
#else
  /* Perform read */
  if (mvme->dmode == MVME_DMODE_D8) {
    status = pwrite(fd, &value, 1, offset);
  } else if (mvme->dmode == MVME_DMODE_D16) {
    char buf[2];
    buf[0] = (value&0xff00) >> 8;
    buf[1] = (value&0xff);
    status = pwrite(fd, buf, 2, offset);
  } else if (mvme->dmode == MVME_DMODE_D32) {
    char buf[4];
    buf[0] = (value&0xff000000) >> 24;
    buf[1] = (value&0xff0000) >> 16;
    buf[2] = (value&0xff00) >> 8;
    buf[3] = (value&0xff);
    status = pwrite(fd, buf, 4, offset);
  }
#endif

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
int mvme_interrupt_generate(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  assert(!"Not implemented");
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
  assert(!"Not implemented");
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_detach(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  assert(!"Not implemented");
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_enable(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  assert(!"Not implemented");
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_disable(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  assert(!"Not implemented");
  return MVME_SUCCESS;
}

//end

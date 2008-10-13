//
// gefvme.h
//
// Driver for the V7865 tsi148 VME interface
//

// Include standard VME functions

#include "mvmestd.h"

// Define gefvme-specific functions

/// return pointer to memory-mapped VME A16 address space
char* gefvme_get_a16(MVME_INTERFACE* mvme);

/// return pointer to memory-mapped VME A24 address space
char* gefvme_get_a24(MVME_INTERFACE* mvme);

/// return pointer to memory-mapped VME A32 address space. Unlike A16 and A24, all of A32 cannot be all memory-mapped at once, so one has to map it one piece at a time. There is a limited number of available mappings
char* gefvme_get_a32(MVME_INTERFACE* mvme, mvme_addr_t vmeaddr, int size);

/// set gefvme DMA debug level: 0=off, >0=write more and more stuff into the system log
void  gefvme_set_dma_debug(int debug);

/// select which one of the 2 tsi148 dma channels to use
void  gefvme_set_dma_channel(int channel);

/// run a single dma read operation. (note: data read through the tsi148 will have the wrong endianness and has to be byte-swapped by the user)
int gefvme_read_dma(MVME_INTERFACE *mvme, void *dst, mvme_addr_t vme_addr, int nbytes);

/// run a chained dma read operation: each dma segment has it's own vme source address, length and memory destination address. (note: data read through the tsi148 will have the wrong endianness and has to be byte-swapped by the user)
int gefvme_read_dma_multiple(MVME_INTERFACE* mvme, int nseg, void* dstaddr[], const mvme_addr_t vmeaddr[], int nbytes[]);

// end

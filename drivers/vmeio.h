#ifndef _VW_MIDAS
typedef long             int_32;
typedef unsigned long   uint_32;
typedef unsigned short  uint_16;
typedef char            uint_08;
#endif


#include "stdio.h"
#include "vme.h"
#include "vxWorks.h"
#include "intLib.h"
#include "sys/fcntlcom.h"
#include "arch/mc68k/ivMc68k.h"
#include "taskLib.h"
/*
#include "semLib.h"
#include "loadLib.h"
#include "moduleLib.h"
#include "symLib.h"
#include "logLib.h"
#include "errnoLib.h"
#include "wdLib.h"
*/

/* from "arch/mc68k/ivMc68k.h" */
#define IVEC_TO_INUM(intVec)    ((int) (intVec) >> 2)
#define INUM_TO_IVEC(intNum)    ((VOIDFUNCPTR *) ((intNum) << 2))

#define ENABLE_INT      0x00
#define SET_INT_SYNC    0x01
#define SET_PULSE       0x02
#define WRITE_PULSE     0x03
#define WRITE_LATCH     0x04
#define READ_SYNC       0x05
#define READ_ASYNC      0x06
#define READ_STATUS     0x07
#define CLEAR_VMEIO     0x07

#define VMEIO_VECT_BASE 0x7f

void vmeio (void);
void set_vmeio_intsync (uint_32 * base_addr, uint_32 pattern);
void set_vmeio_pulse (uint_32 * base_addr, uint_32 pattern);
void read_vmeio_sync (uint_32 * base_addr, uint_32 * data);
void read_vmeio_async (uint_32 * base_addr, uint_32 * data);
void write_vmeio_pulse (uint_32 * base_addr, uint_32 data);
void write_vmeio_latch (uint_32 * base_addr, uint_32 data);
void clear_vmeio (uint_32 * base_addr);
void read_vmeio_status (uint_32 * base_addr, uint_32 * data);
void int_vmeio_clr(uint_32 * base_adr);
void int_vmeio_enable (uint_32 * base_adr, uint_32 intnum);
void int_vmeio_disable (uint_32 * base_adr, uint_32 intnum);
void int_vmeio_book (uint_32 * base_adr, uint_32 base_vect, uint_32 intnum, uint_32 isr_routine );
void int_vmeio_unbook (uint_32 * base_adr, uint_32 base_vect, uint_32 intnum);

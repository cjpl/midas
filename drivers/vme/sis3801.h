/*********************************************************************

  Name:         sis3801.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     Multi-channel scalers

  $Id:$
*********************************************************************/
#ifndef __SIS3801_INCLUDE__
#define __SIS3801_INCLUDE__

#
#include <stdio.h>
#include <string.h>
#include "mvmestd.h"

#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

#ifdef __alpha
typedef unsigned int       DWORD;
#else
typedef unsigned long int  DWORD;
#endif

#endif /* MIDAS_TYPE_DEFINED */


#define HALF_FIFO        16384        /* FIFO 64Kw -> 32Kw 1/2 FIFO
					 2words/Data -> 16KData */
#define MAX_FIFO_SIZE    2*HALF_FIFO  /* FIFO size in words (16 bits) */
#define SIS_FIFO_SIZE    4*HALF_FIFO  /* FIFO size in bytes */
#define CSR_READ         0x0
#define CSR_FULL         0xffffffff  /* CSR Read */
#define CSR_WRITE        0x0

#define IS_LED           0x00000001  /* CSR read */
#define LED_ON           0x00000001  /* CSR write */
#define LED_OFF          0x00000100  /* CSR write */
#define GET_MODE         0x0000000C  /* CSR read */
#define MODE_0           0x00000C00  /* CSR write */
#define MODE_1           0x00000804  /* CSR write */
#define MODE_2           0x00000408  /* CSR write */
#define MODE_3           0x0000000C  /* CSR write */
#define IS_25MHZ         0x00000010  /* CSR read */
#define ENABLE_25MHZ     0x00000010  /* CSR write */
#define DISABLE_25MHZ    0x00001000  /* CSR write */
#define IS_TEST          0x00000020  /* CSR write */
#define ENABLE_TEST      0x00000020  /* CSR write */
#define DISABLE_TEST     0x00002000  /* CSR write */

#define IS_102LNE        0x00000040  /* CSR read */
#define ENABLE_102LNE    0x00000040  /* CSR write */
#define DISABLE_102LNE   0x00000400  /* CSR write */
#define IS_LNE           0x00000080  /* CSR read */
#define ENABLE_LNE       0x00000080  /* CSR write */
#define DISABLE_LNE      0x00000800  /* CSR write */
#define IS_FIFO_EMPTY          0x00000100  /* CSR Read */
#define IS_FIFO_ALMOST_EMPTY   0x00000200  /* CSR Read */
#define IS_FIFO_HALF_FULL      0x00000400  /* CSR Read */
#define IS_FIFO_FULL           0x00001000  /* CSR Read */
#define IS_REF1                0x00002000  /* CSR read */

#define IS_NEXT_LOGIC_ENABLE   0x00008000  /* CSR read */
#define IS_EXTERN_NEXT         0x00010000
#define IS_EXTERN_CLEAR        0x00020000
#define IS_EXTERN_DISABLE      0x00040000
#define IS_SOFT_COUNTING       0x00080000
#define ENABLE_EXTERN_NEXT     0x00010000
#define ENABLE_EXTERN_CLEAR    0x00020000
#define ENABLE_EXTERN_DISABLE  0x00040000
#define SET_SOFT_DISABLE       0x00080000
#define DISABLE_EXTERN_NEXT    0x01000000
#define DISABLE_EXTERN_CLEAR   0x02000000
#define DISABLE_EXTERN_DISABLE 0x04000000
#define CLEAR_SOFT_DISABLE     0x08000000

#define IS_IRQ_EN_CIP          0x00100000  /* CSR read */
#define IS_IRQ_EN_FULL         0x00200000  /* CSR read */
#define IS_IRQ_EN_HFULL        0x00400000  /* CSR read */
#define IS_IRQ_EN_ALFULL       0x00800000  /* CSR read */
#define IS_IRQ_CIP             0x10000000  /* CSR read */
#define IS_IRQ_FULL            0x20000000  /* CSR read */
#define IS_IRQ_HFULL           0x40000000  /* CSR read */
#define IS_IRQ_ALFULL          0x80000000  /* CSR read */
#define ENABLE_IRQ_CIP         0x00100000  /* CSR write */
#define ENABLE_IRQ_FULL        0x00200000  /* CSR write */
#define ENABLE_IRQ_HFULL       0x00400000  /* CSR write */
#define ENABLE_IRQ_ALFULL      0x00800000  /* CSR write */
#define DISABLE_IRQ_CIP        0x10000000  /* CSR write */
#define DISABLE_IRQ_FULL       0x20000000  /* CSR write */
#define DISABLE_IRQ_HFULL      0x40000000  /* CSR write */
#define DISABLE_IRQ_ALFULL     0x80000000  /* CSR write */
#define VME_IRQ_ENABLE         0x00000800  /* CSR write */

#define SIS3801_CSR_RW              0x000
#define SIS3801_MODULE_ID_RO        0x004
#define SIS3801_IRQ_REG_RW          0x004
#define SIS3801_COPY_REG_WO         0x00C
#define SIS3801_FIFO_WRITE_WO       0x010
#define SIS3801_FIFO_CLEAR_WO       0x020
#define SIS3801_VME_NEXT_CLK_WO     0x024
#define SIS3801_ENABLE_NEXT_CLK_WO  0x028
#define SIS3801_DISABLE_NEXT_CLK_WO 0x02C
#define SIS3801_ENABLE_REF_CH1_WO   0x050
#define SIS3801_DISABLE_REF_CH1_WO  0x054
#define SIS3801_MODULE_RESET_WO     0x060
#define SIS3801_SINGLE_TST_PULSE_WO 0x068
#define SIS3801_PRESCALE_REG_RW     0x080
#define SIS3801_FIFO_RO        0x100  // 0x100..0x1FC

#define SOURCE_CIP           0
#define SOURCE_FIFO_FULL     1
#define SOURCE_FIFO_HFULL    2
#define SOURCE_FIFO_ALFULL   3

#define SIS3801_VECT_BASE 0x7f

DWORD sis3801_module_ID(MVME_INTERFACE *myvme, DWORD base);
void  sis3801_module_reset(MVME_INTERFACE *myvme, DWORD base);
DWORD sis3801_IRQ_REG_read(MVME_INTERFACE *myvme, DWORD base);
DWORD sis3801_IRQ_REG_write(MVME_INTERFACE *myvme, DWORD base, DWORD irq);
DWORD sis3801_module_mode(MVME_INTERFACE *myvme, DWORD base, DWORD mode);
DWORD sis3801_dwell_time(MVME_INTERFACE *myvme, DWORD base, DWORD dwell);
int sis3801_ref1(MVME_INTERFACE *myvme, DWORD base, DWORD endis);
int sis3801_next_logic(MVME_INTERFACE *myvme, DWORD base, DWORD endis);
void  sis3801_channel_enable(MVME_INTERFACE *myvme, DWORD base, DWORD nch);
DWORD sis3801_CSR_read(MVME_INTERFACE *myvme, DWORD base, const DWORD what);
DWORD sis3801_CSR_write(MVME_INTERFACE *myvme, DWORD base, const DWORD what);
void  sis3801_FIFO_clear(MVME_INTERFACE *myvme, DWORD base);
int   sis3801_HFIFO_read(MVME_INTERFACE *myvme, DWORD base, DWORD *p);
int   sis3801_FIFO_flush(MVME_INTERFACE *myvme, DWORD base, DWORD *p);
void  sis3801_int_source (MVME_INTERFACE *myvme, DWORD base, DWORD int_source);
void  sis3801_int_source_enable (MVME_INTERFACE *myvme, DWORD base, const int intnum);
void  sis3801_int_source_disable (MVME_INTERFACE *myvme, DWORD base, const int intnum);
void  sis3801_int_clear (MVME_INTERFACE *myvme, DWORD base, const int intnum);
void  sis3801_int_attach (MVME_INTERFACE *myvme, DWORD base, DWORD base_vect, int level, void (*isr)(void));
void  sis3801_int_detach (MVME_INTERFACE *myvme, DWORD baser, DWORD base_vect, int level);
void  sis3801_setup(const MVME_INTERFACE *myvme, DWORD base, int mode, int dsp);

int  sis3801_Setup(MVME_INTERFACE *myvme, DWORD base, int mode);
void sis3801_Status(MVME_INTERFACE *myvme, DWORD base);

#ifdef __cplusplus
}
#endif

#endif  // _INCLUDE

/* emacs                                                                                                             
 * Local Variables:
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */




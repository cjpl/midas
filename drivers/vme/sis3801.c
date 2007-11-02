/*********************************************************************
  @file
  Name:         sis3801.c
  Created by:   Pierre-Andre Amaudruz

  Contents:      Routines for accessing the SIS Multi-channel scalers board

gcc -g -O2 -Wall -g -DMAIN_ENABLE -I/home1/midas/midas/include
    -o sis3801 sis3801.c vmicvme.o -lm -lz -lutil -lnsl -lpthread -L/lib
    -lvme

    $Id:$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#if defined(OS_LINUX)
#include <unistd.h>
#endif


#ifdef MAIN_ENABLE
// For VIMC processor
#include "vmicvme.h"
extern INT_INFO int_info;
int myinfo = VME_INTERRUPT_SIGEVENT;
#endif

#include "sis3801.h"


/*****************************************************************/
/**
   Module ID
   Purpose: return the Module ID number (I) version (V)
   IRQ level (L) IRQ vector# (BB) 0xIIIIVLBB
*/
DWORD sis3801_module_ID(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode;
  DWORD id;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  id = mvme_read_value(mvme, base + SIS3801_MODULE_ID_RO);
  mvme_set_dmode(mvme, cmode);
  return (id);
}

/*****************************************************************/
/**
 */
void sis3801_module_reset(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  mvme_write_value(mvme, base + SIS3801_MODULE_RESET_WO, 0x0);
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/**
   DWORD                : 0xE(1)L(3)V(8)
*/
DWORD sis3801_IRQ_REG_read(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode, id;
  DWORD reg;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  id = mvme_read_value(mvme, base + SIS3801_IRQ_REG_RW);
  mvme_set_dmode(mvme, cmode);
  return (reg & 0xFFF);
}


/*****************************************************************/
/**
   Purpose: write irq (ELV) to the register and read back
   DWORD                : 0xE(1)L(3)V(8)
*/
DWORD sis3801_IRQ_REG_write(MVME_INTERFACE *mvme, DWORD base, DWORD vector)
{
  int   cmode;
  DWORD reg;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  mvme_write_value(mvme, base + SIS3801_IRQ_REG_RW, (vector & 0xFF));
  reg = mvme_read_value(mvme, base + SIS3801_IRQ_REG_RW);
  mvme_set_dmode(mvme, cmode);
  return (reg & 0xFFF);
}

/*****************************************************************/
/**
   Purpose: Set input configuration mode
   DWORD mode          : Mode 0-4
*/
/************ INLINE function for General command ***********/
DWORD sis3801_input_mode(MVME_INTERFACE *mvme, DWORD base, DWORD mode)
{
  int   cmode;
  DWORD rmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if (mode < 4)
    mode <<= 2;
  mvme_write_value(mvme, base + SIS3801_CSR_RW, mode);

  rmode = mvme_read_value(mvme, base + SIS3801_CSR_RW);
  mvme_set_dmode(mvme, cmode);
  return ((rmode & GET_MODE) >> 2);
}

/*****************************************************************/
/**
   Purpose: Set dwell time in us
   DWORD dwell         : dwell time in microseconds
*/
DWORD sis3801_dwell_time(MVME_INTERFACE *mvme, DWORD base, DWORD dwell)
{
  int   cmode;
  DWORD prescale;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  prescale = (10 * dwell ) - 1;
  if ((prescale > 0) && (prescale < 2<<24))
    mvme_write_value(mvme, base + SIS3801_PRESCALE_REG_RW, prescale);

  prescale = mvme_read_value(mvme, base + SIS3801_PRESCALE_REG_RW);
  mvme_set_dmode(mvme, cmode);

  return (prescale);
}

/*****************************************************************/
/**
   Purpose: Enable/Disable Reference on Channel 1
   DWORD endis         : action either ENABLE_REF_CH1
   DISABLE_REF_CH1
*/
int sis3801_ref1(MVME_INTERFACE *mvme, DWORD base, DWORD endis)
{
  int   cmode;
  DWORD csr;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if ((endis == SIS3801_ENABLE_REF_CH1_WO) || (endis == SIS3801_DISABLE_REF_CH1_WO))
    {
      mvme_write_value(mvme, base + endis, 0x0);
    }
  else
    printf("sis3801_ref1: unknown command %d\n",endis);

  /* read back the status */
  csr = mvme_read_value(mvme, base + SIS3801_CSR_RW);
  mvme_set_dmode(mvme, cmode);

  return ((csr & IS_REF1) ? 1 : 0);

}

/*****************************************************************/
/**
 */
int sis3801_next_logic(MVME_INTERFACE *mvme, DWORD base, DWORD endis)
{
  int   cmode;
  DWORD csr;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if ((endis == SIS3801_ENABLE_NEXT_CLK_WO) || (endis == SIS3801_DISABLE_NEXT_CLK_WO))
    {
      mvme_write_value(mvme, base + endis, 0x0);
    }
  else
    printf("sis3801_next_logic: unknown command %d\n",endis);

  /* read back the status */
  csr = mvme_read_value(mvme, base + SIS3801_CSR_RW);
  mvme_set_dmode(mvme, cmode);

  return ((csr & IS_NEXT_LOGIC_ENABLE) ? 1 : 0);
}

/*****************************************************************/
/**
   Purpose: Enable nch channel for acquistion.
   blind command! 1..24 or 32
*/
void sis3801_channel_enable(MVME_INTERFACE *mvme, DWORD base, DWORD nch)
{
  int   cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if (nch > 24) nch = 32;
  mvme_write_value(mvme, base + SIS3801_COPY_REG_WO, (1<<nch));
  mvme_set_dmode(mvme, cmode);

  return;
}

/*****************************************************************/
/**
   Purpose: Read the CSR and return 1/0 based on what.
   except for what == CSR_FULL where it returns current CSR
*/
DWORD sis3801_CSR_read(MVME_INTERFACE *mvme, DWORD base, const DWORD what)
{
  int   cmode;
  DWORD csr;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if (what == CSR_FULL)
    {
      csr = mvme_read_value(mvme, base + SIS3801_CSR_RW);
    } else if (what == GET_MODE) {
      csr = mvme_read_value(mvme, base + SIS3801_CSR_RW);
      csr = ((csr & what) >> 2);
    } else {
      csr = mvme_read_value(mvme, base + SIS3801_CSR_RW);
      csr = ((csr & what) ? 1 : 0);
    }
  mvme_set_dmode(mvme, cmode);
  return (csr);
}

/*****************************************************************/
/**
   Purpose: Write to the CSR and return CSR_FULL.
*/
DWORD sis3801_CSR_write(MVME_INTERFACE *mvme, DWORD base, const DWORD what)
{
  int   cmode;
  int csr;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  mvme_write_value(mvme, base + SIS3801_CSR_RW, what);

  csr =  sis3801_CSR_read(mvme, base, CSR_FULL);

  mvme_set_dmode(mvme, cmode);
  return (csr);
}

/*****************************************************************/
/**
   Purpose: Clear FIFO and logic
*/
void sis3801_FIFO_clear(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  mvme_write_value(mvme, base + SIS3801_FIFO_CLEAR_WO, 0x0);

  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/**
   Purpose: Reads 32KB (8K DWORD) of the FIFO
   Function value:
   int                 : -1 FULL
   0 NOT 1/2 Full
   number of words read
*/
int sis3801_HFIFO_read(MVME_INTERFACE *mvme, DWORD base, DWORD * pfifo)
{
  int   i, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if (sis3801_CSR_read(mvme, base, IS_FIFO_FULL)) {
    mvme_set_dmode(mvme, cmode);
    return -1;
  }
  if (sis3801_CSR_read(mvme, base, IS_FIFO_HALF_FULL) == 0) {
    mvme_set_dmode(mvme, cmode);
    return 0;
  }
  for (i=0;i<HALF_FIFO;i++)
    *pfifo++ = mvme_read_value(mvme, base + SIS3801_FIFO_RO);

  mvme_set_dmode(mvme, cmode);
  return HALF_FIFO;
}

/*****************************************************************/
/**
   Purpose: Test and read FIFO until empty
   This is done using while, if the dwelling time is short
   relative to the readout time, the pfifo can be overrun.
   No test on the max read yet!
   Function value:
   int                 : -1 FULL
   number of words read
*/
int sis3801_FIFO_flush(MVME_INTERFACE *mvme, DWORD base, DWORD * pfifo)
{
  int cmode, counter=0;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if (sis3801_CSR_read(mvme, base, IS_FIFO_FULL)) {
    mvme_set_dmode(mvme, cmode);
    return -1;
  }
  while ((sis3801_CSR_read(mvme, base, IS_FIFO_EMPTY) == 0) && (counter < MAX_FIFO_SIZE))
    {
      counter++;
      *pfifo++ = mvme_read_value(mvme, base + SIS3801_FIFO_RO);
    }

  mvme_set_dmode(mvme, cmode);
  return counter;
}

/*****************************************************************/
/**
   Purpose: Enable the interrupt for the bitwise input intnum (0xff).
   The interrupt vector is then the VECTOR_BASE
*/
void sis3801_int_source_enable (MVME_INTERFACE *mvme, DWORD base, const int intnum)
{
  int cmode;
  DWORD int_source;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  switch (intnum)
    {
    case  0:
      int_source = ENABLE_IRQ_CIP;
      break;
    case  1:
      int_source = ENABLE_IRQ_FULL;
      break;
    case  2:
      int_source = ENABLE_IRQ_HFULL;
      break;
    case  3:
      int_source = ENABLE_IRQ_ALFULL;
      break;
    default:
      printf("Unknown interrupt source (%d)\n",int_source);
    }
  mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);

  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/**
   Purpose: Enable the interrupt for the bitwise input intnum (0xff).
   The interrupt vector is then the VECTOR_BASE
*/
void sis3801_int_source_disable (MVME_INTERFACE *mvme, DWORD base, const int intnum)
{
  int cmode;
  DWORD int_source;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  switch (intnum)
    {
    case  0:
      int_source = DISABLE_IRQ_CIP;
      break;
    case  1:
      int_source = DISABLE_IRQ_FULL;
      break;
    case  2:
      int_source = DISABLE_IRQ_HFULL;
      break;
    case  3:
      int_source = DISABLE_IRQ_ALFULL;
      break;
    default:
      printf("Unknown interrupt source (%d)\n",int_source);
    }
  mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);

  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/**
 Purpose: Enable the one of the 4 interrupt using the
          predefined parameters (see sis3801.h)
  DWORD * intnum          : interrupt number (input 0..3)
*/
void sis3801_int_source (MVME_INTERFACE *mvme, DWORD base, DWORD int_source)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  int_source &= (ENABLE_IRQ_CIP | ENABLE_IRQ_FULL
    | ENABLE_IRQ_HFULL | ENABLE_IRQ_ALFULL
    | DISABLE_IRQ_CIP  | DISABLE_IRQ_FULL
    | DISABLE_IRQ_HFULL| DISABLE_IRQ_ALFULL);
  mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);

  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
 Purpose: Book an ISR for a bitwise set of interrupt input (0xff).
          The interrupt vector is then the VECTOR_BASE+intnum
 Input:
  DWORD * base_addr      : base address of the sis3801
  DWORD base_vect        : base vector of the module
  int   level            : IRQ level (1..7)
  DWORD isr_routine      : interrupt routine pointer
*/
void sis3801_int_attach (MVME_INTERFACE *mvme, DWORD base, DWORD base_vect, int level, void (*isr)(void))
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  /* disable all IRQ sources */
  sis3801_int_source(mvme, base
         , DISABLE_IRQ_CIP | DISABLE_IRQ_FULL
         | DISABLE_IRQ_HFULL | DISABLE_IRQ_ALFULL);
  if ((level < 8) && (level > 0) && (base_vect < 0x100)) {
    mvme_write_value(mvme, base + SIS3801_IRQ_REG_RW, (level << 8) | VME_IRQ_ENABLE | base_vect);
    mvme_interrupt_attach(mvme, level, base_vect, (void *)isr, &myinfo);
  }
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
 Purpose: Unbook an ISR for a bitwise set of interrupt input (0xff).
          The interrupt vector is then the VECTOR_BASE+intnum
 Input:
  DWORD * base_addr       : base address of the sis3801
  DWORD base_vect        : base vector of the module
  int   level            : IRQ level (1..7)
*/
void sis3801_int_detach (MVME_INTERFACE *mvme, DWORD base, DWORD base_vect, int level)
{

  /* disable all IRQ sources */
  sis3801_int_source(mvme, base , DISABLE_IRQ_CIP
        | DISABLE_IRQ_FULL | DISABLE_IRQ_HFULL
  | DISABLE_IRQ_ALFULL);

  return;
}

/*****************************************************************/
/**
 Purpose: Disable the interrupt for the bitwise input intnum (0xff).
          The interrupt vector is then the VECTOR_BASE+intnum
 Input:
  DWORD * base_addr       : base address of the sis3801
  int   * intnum          : interrupt number (input 0..3)
*/
void sis3801_int_clear (MVME_INTERFACE *mvme, DWORD base, const int intnum)
{
  DWORD int_source;

  switch (intnum)
    {
    case  0:
      int_source = DISABLE_IRQ_CIP;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      int_source = ENABLE_IRQ_CIP;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      break;
    case  1:
      int_source = DISABLE_IRQ_FULL;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      int_source = ENABLE_IRQ_FULL;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      break;
    case  2:
      int_source = DISABLE_IRQ_HFULL;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      int_source = ENABLE_IRQ_HFULL;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      break;
    case  3:
      int_source = DISABLE_IRQ_ALFULL;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      int_source = ENABLE_IRQ_ALFULL;
      mvme_write_value(mvme, base + SIS3801_CSR_RW, int_source);
      break;
    default:
      printf("Unknown interrupt source (%d)\n",int_source);
    }

  return;
}


/*****************************************************************/
/**
Sets all the necessary parameters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@param *nentry number of entries requested and returned.
@return MVME_SUCCESS
*/
int  sis3801_Setup(MVME_INTERFACE *mvme, DWORD base, int mode)
{
  int  cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  switch (mode) {
  case 0x1:
    printf("Default setting after power up (mode:%d)\n", mode);
    printf("...\n");
    printf("...\n");
    break;
  case 0x2:
    break;
  default:
    printf("Unknown setup mode\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }

  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
/**
 */
void sis3801_Status(MVME_INTERFACE *mvme, DWORD base)
{
  DWORD csr;

  csr = sis3801_CSR_read(mvme, base, CSR_FULL);

  printf("Module Version   : %d\t", ((sis3801_module_ID (mvme, base) & 0xf000) >> 12));
  printf("Module ID        : %4.4x\t", (sis3801_module_ID (mvme, base) >> 16));
  printf("CSR contents     : 0x%8.8x\n", csr);
  printf("LED              : %s \t",(csr &     IS_LED) ? "Y" : "N");
  printf("FIFO test mode   : %s \t",(csr &        0x2) ? "Y" : "N");
  printf("Input mode       : %d \n",sis3801_input_mode(mvme, base, 2));
  printf("25MHz test pulse : %s \t",(csr &   IS_25MHZ) ? "Y" : "N");
  printf("Input test mode  : %s \t",(csr &    IS_TEST) ? "Y" : "N");
  printf("10MHz to LNE     : %s \t",(csr &  IS_102LNE) ? "Y" : "N");
  printf("LNE prescale     : %s \n",(csr &     IS_LNE) ? "Y" : "N");
  printf("Reference pulse 1: %s \t",(csr &    IS_REF1) ? "Y" : "N");
  printf("Next Logic       : %s \n",(csr & IS_NEXT_LOGIC_ENABLE) ? "Y" : "N");
  printf("FIFO empty       : %s \t",(csr & IS_FIFO_EMPTY ) ? "Y" : "N");
  printf("FIFO almost empty: %s \t",(csr & IS_FIFO_ALMOST_EMPTY) ? "Y" : "N");
  printf("FIFO half full   : %s \t",(csr & IS_FIFO_HALF_FULL) ? "Y" : "N");
  printf("FIFO full        : %s \n",(csr & IS_FIFO_FULL) ? "Y" : "N");
  printf("External next    : %s \t",(csr & IS_EXTERN_NEXT) ? "Y" : "N");
  printf("External clear   : %s \t",(csr & IS_EXTERN_CLEAR) ? "Y" : "N");
  printf("External disable : %s \t",(csr & IS_EXTERN_DISABLE) ? "Y" : "N");
  printf("Software couting : %s \n",(csr & IS_SOFT_COUNTING) ? "N" : "Y");
  printf("IRQ enable CIP   : %s \t",(csr & IS_IRQ_EN_CIP) ? "Y" : "N");
  printf("IRQ enable FULL  : %s \t",(csr & IS_IRQ_EN_FULL) ? "Y" : "N");
  printf("IRQ enable HFULL : %s \t",(csr & IS_IRQ_EN_HFULL) ? "Y" : "N");
  printf("IRQ enable ALFULL: %s \n",(csr & IS_IRQ_EN_ALFULL) ? "Y" : "N");
  printf("IRQ CIP          : %s \t",(csr & IS_IRQ_CIP) ? "Y" : "N");
  printf("IRQ FIFO full    : %s \t",(csr & IS_IRQ_FULL) ? "Y" : "N");
  printf("IRQ FIFO 1/2 full: %s \t",(csr & IS_IRQ_HFULL) ? "Y" : "N");
  printf("IRQ FIFO almost F: %s \n",(csr & IS_IRQ_ALFULL) ? "Y" : "N");
  printf("internal VME IRQ : %s \t",(csr &  0x4000000) ? "Y" : "N");
  printf("VME IRQ          : %s \n",(csr &  0x8000000) ? "Y" : "N");
}

int intflag=0;

static void myisr(int sig, siginfo_t * siginfo, void *extra)
{

  //  fprintf(stderr, "interrupt: level:%d Vector:0x%x ...  "
  //          , int_info.level, siginfo->si_value.sival_int & 0xFF);

  if (intflag == 0) {
    intflag = 1;
    printf("interrupt: level:%d Vector:0x%x ...  "
     , int_info.level, siginfo->si_value.sival_int & 0xFF);
  }
}

/*****************************************************************/
/*-PAA- For test purpose only */

#ifdef MAIN_ENABLE

#define IRQ_VECTOR_CIP        0x70
#define IRQ_LEVEL                5

int main (int argc, char* argv[]) {

  int status, i;
  DWORD SIS3801_BASE = 0x110000;
  DWORD *pfifo=NULL, nwords;

  MVME_INTERFACE *myvme;

  if (argc>1) {
    sscanf(argv[1],"%x",&SIS3801_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);


#if 0  // simple
  printf("ID:%x\n", sis3801_module_ID(myvme, SIS3801_BASE));
  sis3801_Status(myvme, SIS3801_BASE);

  for(i=0;i<4;i++) {
    sis3801_CSR_write(myvme, SIS3801_BASE, LED_ON);
    usleep(500000);
    sis3801_CSR_write(myvme, SIS3801_BASE, LED_OFF);
    usleep(500000);
  }
#endif

#if 1  // full acq with interrupts
  // reset
  sis3801_module_reset(myvme, SIS3801_BASE);
  // 0..3 enabled
  sis3801_channel_enable(myvme, SIS3801_BASE, 4);
  // Use ch 1 as reference
  sis3801_ref1(myvme, SIS3801_BASE, SIS3801_ENABLE_REF_CH1_WO);
  // Set the input lemo to the rght mode (ask SD)
  sis3801_input_mode(myvme, SIS3801_BASE, 0);
  // Set fix dwell time to 1s (check with SD)
  sis3801_dwell_time(myvme, SIS3801_BASE, 1000);
  //
  sis3801_CSR_write(myvme, SIS3801_BASE, DISABLE_EXTERN_NEXT);
  sis3801_CSR_write(myvme, SIS3801_BASE, DISABLE_EXTERN_DISABLE);
  sis3801_CSR_write(myvme, SIS3801_BASE, ENABLE_102LNE);
  sis3801_CSR_write(myvme, SIS3801_BASE, ENABLE_LNE);
  sis3801_CSR_write(myvme, SIS3801_BASE, DISABLE_TEST);  /* disable test bit */
  sis3801_CSR_write(myvme, SIS3801_BASE, DISABLE_25MHZ); /* disable 25MHZ test pulse*/

  // Attach to local isr
  sis3801_int_attach(myvme, SIS3801_BASE, IRQ_VECTOR_CIP, IRQ_LEVEL, (void *)myisr);

  // Interrupt only on Half full
  sis3801_int_source_enable(myvme, SIS3801_BASE, SOURCE_FIFO_HFULL);

  // Start the engine
  sis3801_next_logic(myvme, SIS3801_BASE, SIS3801_ENABLE_NEXT_CLK_WO);

  printf("CSR:0x%x\n", sis3801_CSR_read(myvme, SIS3801_BASE, CSR_FULL));
  sis3801_Status(myvme, SIS3801_BASE);

  pfifo = (DWORD *) malloc(100000);

  for (;;) {
    nwords = sis3801_HFIFO_read(myvme, SIS3801_BASE, pfifo);
    if (nwords) {
      for (i=0;i<32;i++) {
  printf("%d:0x%x ",i, pfifo[i]);
      }
      // Takes some time before the data are readout
      // prevent burst of interrupt.
      intflag = 0;
    }
    printf("CSR:0x%x\n", sis3801_CSR_read(myvme, SIS3801_BASE, CSR_FULL));
   usleep(500000);
  }

#endif

  status = mvme_close(myvme);
  return 1;
}
#endif

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

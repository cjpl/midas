/*****************************************************************************
**
**      Filename:   btwapi.h
**
**      Purpose:    Bit 3 Application Programming Interface header file
**                  for 973 and 983.
**
**      $Revision$
**
******************************************************************************/
/*****************************************************************************
**
**        Copyright (c) 1996-1997 by Bit 3 Computer Corporation.
**                     All Rights Reserved.
**              License governs use and distribution.
**
*****************************************************************************/

#ifndef _BTWAPI_H
#define _BTWAPI_H

#ifndef BTDRIVER
#include <windows.h>
#endif                          /* !BTDRIVER */

#define BT_NBUS

#define FILE_DEVICE_BTBRIDGE ((DEVICE_TYPE)32768)
#define BT_LEN_TR_MSG 80
#define BT_MIN_LEN_TR_Q    50   // minimum trace queue length
#define BT_MAX_LEN_TR_Q    500  // maximum trace queue length
#define BT_MAX_UNITS 32

// Driver trace flags most commonly used
#define BT_TRC_NONE      0x00000000     // no tracing is enabled whatsoever
#define BT_TRC_ERROR     0x00000001     // critical driver errors, shouldn't happen
#define BT_TRC_WARN      0x00000002     // driver errors that may happen due to
                                    // external conditions
#define BT_TRC_USR_ERROR 0x00000004     // errors returned to the user
#define BT_TRC_INIT      0x00000008     // driver initialization
#define BT_TRC_ENTR_EXIT 0x00000010     // function entry and exit
#define BT_TRC_RD_WR     0x00000020     // read/write
#define BT_TRC_ISR       0x00000040     // Interrupt Service Routine internal
#define BT_TRC_INTDPC    0x00000080     // Interrupt Deferred Procedure Call internal
#define BT_TRC_MMAP      0x00000100     // MMAP
#define BT_TRC_STATUS    0x00000200     // status, reset, setup API calls
#define BT_TRC_IO        0x00000400     // I/O register read/write, TAS, CAS API calls
#define BT_TRC_INTR      0x00000800     // BT_xxxInterrupt() routines
#define BT_TRC_PARAM     0x00001000     // BT_Set/GetParam() & BT_GetInfo()
#define BT_TRC_UISR      0x00002000     // user ISR
#define BT_TRC_BIND      0x00004000     // Bind and Unbind
#define BT_TRC_VERBOSE   0x07ff0000     // all detailed information

// Trace flags providing detailed information
#define BT_TRC_MAPREG    0x00010000     // details of each map register load
#define BT_TRC_DMA       0x00020000     // DMA
#define BT_TRC_PIO       0x00040000     // PIO
#define BT_TRC_W32EVT    0x00080000     // Win32 event processing
#define BT_TRC_LOWIO     0x00100000     // Low level I/O functions
#define BT_TRC_LOCK      0x00200000     // all locks (user & driver internal)
#define BT_TRC_USR_VALID 0x07ffffff     // valid user trace flags
#define BT_TRC_T1        0x08000000     // temporary driver internal usage
#define BT_TRC_T2        0x10000000     // temporary driver internal usage
#define BT_TRC_T3        0x20000000     // temporary driver internal usage
#define BT_TRC_PROFILE   0x40000000     // enable H/W timing marks for driver profiling
#define BT_TRC_BREAK     0x80000000     // break into debugger (no effect in user trace)

#define BT_TRC_USR_DEFAULT  (BT_TRC_ERROR|BT_TRC_WARN|BT_TRC_INIT)      // user trace default
#if DBG
#define BT_TRC_DRV_DEFAULT  (BT_TRC_ERROR|BT_TRC_WARN|BT_TRC_INIT|BT_TRC_USR_ERROR)     // driver internal trace default
#else                           // DBG
#define BT_TRC_DRV_DEFAULT BT_TRC_NONE
#endif                          // DBG

#define BT_TRC_INFO      0x00003fff     // Most tracing enables- lot of output!

#define BT_DMA_MIN_PKT_SIZE 4   // minimum our H/W can DMA
#define BT_DMA_MIN_TIMEOUT  500 // minimum DMA timeout in mS
#define BT_DMA_DFLT_TIMEOUT 5000        // default DMA timeout in mS
#define BT_DMA_MAX_TIMEOUT  60000       // maximum DMA timeout in mS

typedef enum {
   BT_DEV_DP,
   BT_DEV_IO,
   BT_DEV_A24,
   BT_DEV_RR,
   BT_DEV_LM,
   BT_DEV_NIO,
   BT_MAX_DEV,

   /*
    ** Aliases
    */
   BT_DEV_A16 = BT_DEV_IO,
   BT_DEV_A32 = BT_DEV_RR,
   BT_DEV_MEM = BT_DEV_RR,
   BT_DEV_DEFAULT = BT_DEV_DP
} bt_dev_t;

struct bt_desc_struct_t {
   HANDLE handle;
   void *dll_data;
   struct bt_desc_struct_t *next_p;
};

typedef struct bt_desc_struct_t *bt_desc_t;

#define BT_MAX_DEV_NAME 20

typedef enum {
   BT_SUCCESS,
   BT_EBUSY,
   BT_EINVAL,
   BT_EIO,
   BT_EDESC,
   BT_ENOMEM,
   BT_ENOPWR,
   BT_ENOSUP,
   BT_EPWRCYC,
   BT_ESTATUS,
   BT_EFAIL,
   BT_EEXCEPT,
   BT_EEVT_NOT_REGISTERED,
   BT_ENOT_FOUND,
   BT_ENOT_LOCKED,
   BT_ENOT_TRANSMITTER,
   BT_EPCI_CONFIG,
   BT_EACCESS,
   BT_ELOCAL,
   BT_EUNKNOWN_REMID,
   BT_ENO_UNIT,
   BT_ENO_MMAP,
   BT_EHANDLE,
   BT_MAX_ERROR
} bt_error_t;

typedef enum {
   BT_INFO_INVALID,
   BT_INFO_BIND_ALIGN,
   BT_INFO_BIND_COUNT,
   BT_INFO_BIND_SIZE,
   BT_INFO_BLOCK,
   BT_INFO_DATAWIDTH,
   BT_INFO_DMA_AMOD,
   BT_INFO_DMA_POLL_CEILING,
   BT_INFO_DMA_THRESHOLD,
   BT_INFO_LM_SIZE,
   BT_INFO_LOC_PN,
   BT_INFO_MMAP_AMOD,
   BT_INFO_PAUSE,
   BT_INFO_PIO_AMOD,
   BT_INFO_REM_PN,
   BT_INFO_SWAP,
   BT_INFO_TRACE,
   BT_INFO_RESET_DELAY,
   BT_INFO_DMA_WATCHDOG,
   BT_INFO_TRANSMITTER,
   BT_INFO_LOG_STAT,
   BT_INFO_BUS_NUM,
   BT_INFO_SLOT_NUM,
   BT_INFO_UNIT_NUM,
   BT_INFO_SIG_TOT,
   BT_INFO_SIG_PRG,
   BT_INFO_INTR_PEND_CNT,
   BT_INFO_INTR_FREE_CNT,
   BT_INFO_INTR_LVL,
   BT_INFO_INTR_NODE_CNT,
   BT_INFO_ICBR_Q_SIZE = BT_INFO_INTR_NODE_CNT,
   BT_INFO_INTR_REG_CNT,
   BT_INFO_INTR_AQR_CNT,
   BT_INFO_SIG_ERR,
   BT_INFO_SIG_IACK,
   BT_INFO_DMA_NOINC,
   BT_MAX_INFO
} bt_info_t;
#define BT_MIN_RESET_DELAY 10   /* Min. value for BT_INFO_REM_RESET_DELAY */
#define BT_MIN_DMA_PKT_SIZE 4   /* Min. value for BT_INFO_DMA_THRESHOLD */
#define BT_MIN_DMA_TIMEOUT 500
#define BT_MAX_DMA_TIMEOUT 60000


typedef enum {
   BT_IRQ_ERROR,
   BT_IRQ_OVERFLOW,
   BT_IRQ_PRG,
   BT_IRQ_IACK
} bt_irq_t;

typedef DWORD bt_devdata_t;
typedef DWORD bt_devaddr_t;
typedef bt_devdata_t bt_amod_t;

typedef int bt_binddesc_t;

typedef enum {
   BT_MIN_CTRL,
   BT_MAX_CTRL
} bt_ctrl_t;


#define BT_AMOD_A16     BT_AMOD_A16_SD
#define BT_AMOD_A32     BT_AMOD_A32_SD
#define BT_AMOD_A24     BT_AMOD_A24_SD
#define BT_AMOD_DEFAULT 0

#define BT_BIND_ANYWHERE (~((bt_devaddr_t) 0))
#define BT_VECTOR_ALL 0

typedef enum {
   BT_SWAP_NONE,
   BT_SWAP_BSNBD,
   BT_SWAP_WS,
   BT_SWAP_WS_BSNBD,
   BT_SWAP_BSBD,
   BT_SWAP_BSBD_BSNBD,
   BT_SWAP_BSBD_WS,
   BT_SWAP_BSBD_WS_BSNBD,
   BT_SWAP_DEFAULT,
   BT_MAX_SWAP
} bt_swap_t;

typedef enum {

   BT_REG_LOC_START,
   BT_REG_LOC_CMD1 = BT_REG_LOC_START,  // Local Command register 1
   BT_REG_LOC_INT_CTRL,         // Local Interrupt Control
   BT_REG_LOC_INT_STATUS,       // Local Interrupt Status
   BT_REG_LOC_STATUS,           // Local Status register
   BT_REG_LOC_HANDSHAKE,        // Local Handshake register
   BT_REG_LOC_CONFIG_CTRL,      // Local Configuration Register
   BT_REG_LOC_RCMD2,            // Local access to remote command 2
   BT_REG_LOC_RPAGE,            // Local access to remote page reg
   BT_REG_LOC_AMOD,             // Local address modifier
   BT_REG_LDMA_CMD,             // Local DMA Command register
   BT_REG_LDMA_RMD_CNT,         // Local DMA Remainder Cnt register
   BT_REG_LDMA_ADDR,            // Local DMA Address register
   BT_REG_LDMA_PKT_CNT,         // Local DMA Packet count register
   BT_REG_LOC_TCSIZE,           // TURBOChannel size register
   BT_REG_LOC_256MB_PAGE,       // Local 256MB window remote page
   BT_REG_LOC_RCMD1,            // Local access to remote command 1

   BT_REG_REM_START,
   BT_REG_REM_CMD1 = BT_REG_REM_START,  // Remote Command register 1
   BT_REG_REM_STATUS,           // Remote Status register
   BT_REG_REM_CMD2,             // Remote Command register 2
   BT_REG_REM_PAGE,             // Remote Page Register
   BT_REG_REM_AMOD,             // Remote Address Modifier
   BT_REG_REM_IACK,             // Remote IACK register
   BT_REG_REM_CARD_ID,          // Remote Card ID Register
   BT_REG_RDMA_ADDR,            // Remote DMA Address
   BT_REG_RDMA_RMD_CNT,         // Remote DMA Remainder Count
   BT_REG_REM_SLAVE_STATUS,     // Remote slave status
   BT_REG_REM_SLAVE_CLRERR,     // Remote slave clear error

   BT_MAX_REG,                  // Maximum number of registers

   /*
    **  PCI Configuration Registers
    **
    **  These are the user-accessable registers in PCI configuration
    **  space.  They are not normally enabled, and programs should
    **  act as if they do not exist.  Which is why they are put
    **  after BT_MAX_REGISTER.
    **
    **  See the Bit 3 Hardware Manual and the PCI hardware specification
    **  for more information.
    **
    */
   BT_REGPCI_START,
   BT_REGPCI_VEN_ID = BT_REGPCI_START,  // PCI Vendor ID register
   BT_REGPCI_DEV_ID,            // PCI Device ID register
   BT_REGPCI_COMMAND,           // PCI Command Register
   BT_REGPCI_STATUS,            // PCI Status Register
   BT_REGPCI_REV_ID,            // PCI Revision ID Register
   BT_REGPCI_PROGIF,            // PCI Programming Interface
   BT_REGPCI_SUBCLASS,          // PCI Subclass
   BT_REGPCI_BASECLASS,         // PCI Base Class
   BT_REGPCI_CACHE_LINE_SIZE,   // PCI Cache Line Size Register
   BT_REGPCI_LATENCY_TIMER,     // PCI Latency Timer Register
   BT_REGPCI_HEADER_TYPE,       // PCI Header Type Register
   BT_REGPCI_BIST,              // PCI BIST Register
   BT_REGPCI_INTR_LINE,         // PCI Interrupt Line Register
   BT_REGPCI_INTR_PIN,          // PCI Interrupt Pin Register
   BT_REGPCI_MIN_GNT,           // PCI Min_Gnt Register
   BT_REGPCI_MAX_LAT,           // PCI Max_Lat Register
   BT_REGPCI_MAX_REGISTER       // Number of PCI Config Registers
} bt_reg_t;

typedef unsigned long int bt_msec_t;
#define BT_NO_WAIT (0ul)
#define BT_FOREVER (~0ul)


/*****************************************************************************
**
**  List all Bit 3 adaptor IDs registered to date.
**  Used in process of determining remote identification.
**
*****************************************************************************/

#define BT_ID_EISA      0x62    /* EISA ID Value                */
#define BT_ID_EISA2     0x41    /* New EISA ID Value            */
#define BT_ID_MCA3_DMA  0x76    /* MCA type 3 w/slave DMA       */
#define BT_ID_SB_DMA    0xDF    /* SBus ID Value                */
#define BT_ID_TC        0x4D    /* TURBOchannel ID Value        */
#define BT_ID_VME_SDMA  0x80    /* VMEbus w/ Slave DMA          */
#define BT_ID_GIO       0x47    /* GIO ID value - ASCII "G"     */
#define BT_ID_PCI_DMA   0xAB    /* PCI card ID value            */
#define BT_ID_PCI       0xAC    /* PCI wo/DMA card ID value     */


#define BT_ALIGN_PTR(buf_p, align) ((((unsigned long)(buf_p))&((align) - 1))?\
                                    ((ptrdiff_t)((align)-(((unsigned long) \
                                     (buf_p))&((align)-1)))):((ptrdiff_t)0))

typedef void bt_icbr_t(void *, bt_irq_t, bt_data32_t);

typedef struct {
   HANDLE hEvent;
   DWORD dwDrvEventId;
} bt_intr_struct_t;

typedef bt_intr_struct_t *bt_intrdesc_t;

// Defines for logical device status (BT_LD.dwStatus)
#define BT_LDSTAT_ONLINE   0x00000001   // Set if logical device is online
#define BT_LDSTAT_OPEN     0x00000002   // Set if logical device is open
#define BT_LDSTAT_READ     0x00000004   // Set if logical device opened for reads
#define BT_LDSTAT_WRITE    0x00000008   // Set if logical device opened for writes
#define BT_LDSTAT_HNDSHK   0x00000010   // Set if logical device using handshake (not implemented)
#define BT_LDSTAT_DMA      0x00000020   // Set if logical device supports DMA
#define BT_LDSTAT_LOCAL    0x00000040   // Set if logical device is local resource
#define BT_LDSTAT_MMAP     0x00000080   // Set if logical device supports mmapping

// Interrupt flags. The first 6 flags are used in interrupt registration.
// ERR, PRG, IACK, and OVRRUN can be reported by BT_AcquireInterrupt().
typedef DWORD BT_INTRFLAG, *PBT_INTRFLAG;
#define BT_INTR_ERR      0x00000001     // Error Interrupt desired
#define BT_INTR_PRG      0x00000002     // Programmed Interrupt desired
#define BT_INTR_IACK     0x00000004     // IACK interrupt desired
#define BT_INTR_PARAM    0x00002000     // Set if selective signal (*_param)
#define BT_INTR_SIGO     0x00004000     // Signal only (don't save on AqrEvtList)
#define BT_INTR_HLD      0x00008000     // Re-registration select
#define BT_INTR_OVRRUN   0x00010000     // Interrupt overrun

// Adapter status flags. These can be reported by BT_Status().
#define BT_INTR_XMITR    0x00020000     // Local node is transmitter
#define BT_INTR_PAGE_EN  0x00040000     // Page register enabled (not supported)
#define BT_INTR_RRESET   0x00080000     // Remote bus was reset
#define BT_INTR_POWER    0x01000000     // Remote bus power off or cable
#define BT_INTR_LRC      0x02000000     // Logitudinal Redundancy Check error
#define BT_INTR_TIMEOUT  0x04000000     // Interface timeout
#define BT_INTR_REMBUS   0x40000000     // Remote bus error
#define BT_INTR_PARITY   0x80000000     // Interface parity error
#define BT_STATUS_MASK   (BT_INTR_POWER|BT_INTR_LRC|BT_INTR_TIMEOUT|BT_INTR_REMBUS|BT_INTR_PARITY)
#define BT_INTR_ERR_SHFT (24)
#define BT_INTR_REGMSK (BT_INTR_ERR | BT_INTR_PRG | BT_INTR_IACK)

#undef BT_EXTERN

#ifndef BTDRIVER

#ifndef BT_APIDLL

#ifdef __cplusplus
#define BT_EXTERN extern "C" __declspec(dllimport)
#else                           /* __cplusplus */
#define BT_EXTERN extern __declspec(dllimport)
#endif                          /* __cplusplus */

#else                           /* !BT_APIDLL */

#ifdef __cplusplus
#define BT_EXTERN extern "C" __declspec(dllexport)
#else                           /* __cplusplus */
#define BT_EXTERN extern __declspec(dllexport)
#endif                          /* __cplusplus */

#endif                          /* !BT_APIDLL */

#else                           /* !BTDRIVER */

#ifdef __cplusplus
#define BT_EXTERN extern "C"
#else                           /* __cplusplus */
#define BT_EXTERN extern
#endif                          /* __cplusplus */

#endif                          /* !BTDRIVER */

/*
** Nanobus extension functions
*/
BT_EXTERN bt_error_t __stdcall bt_cas(bt_desc_t btd, bt_devaddr_t rem_off,
                                      bt_data32_t cmpval, bt_data32_t swapval,
                                      bt_data32_t * prevval_p, size_t data_size);
BT_EXTERN bt_error_t __stdcall bt_tas(bt_desc_t btd, bt_devaddr_t rem_off,
                                      bt_data8_t * prevval_p);
BT_EXTERN bt_error_t __stdcall bt_get_io(bt_desc_t btd, bt_reg_t reg,
                                         bt_data32_t * result_p);
BT_EXTERN bt_error_t __stdcall bt_put_io(bt_desc_t btd, bt_reg_t reg, bt_data32_t value);
BT_EXTERN bt_error_t __stdcall bt_or_io(bt_desc_t btd, bt_reg_t reg, bt_data32_t value);
BT_EXTERN const char *__stdcall bt_reg2str(bt_reg_t reg);
BT_EXTERN bt_error_t __stdcall bt_reset(bt_desc_t btd);
BT_EXTERN bt_error_t __stdcall bt_send_irq(bt_desc_t btd);

/*
** Windows implementation-specific functions
*/
BT_EXTERN bt_error_t __stdcall bt_gettrace(char *lpBuffer, unsigned long *lpNumMessages);
BT_EXTERN bt_error_t __stdcall bt_status(bt_desc_t btd, bt_data32_t * Status_p);
BT_EXTERN bt_error_t __stdcall bt_querydriver(int btUnitNo,
                                              unsigned long *pdwBusNum,
                                              unsigned long *pdwSlotNum,
                                              unsigned long *pdwLocId);
BT_EXTERN bt_error_t __stdcall bt_querydriver2(int btUnitNo,
                                               unsigned long *pdwBusNum,
                                               unsigned long *pdwSlotNum,
                                               unsigned long *pdwLocId,
                                               char *pRegistryName);
#endif                          /* _BTWAPI_H */

/******************************************************************************
**
** File:    btwuser.h
**
** Purpose: Bit 3 983 Windows NT device driver module.
**          Include file for user written NT kernel mode drivers that implement
**          user ISRs.
**
** Author:  Craig Harrison
**
** $Revision$
**
** Warning: Use byte packing on all structures in this file. For Microsoft
**          compilers this is done as follows:
**              #pragma pack(push,1)
**              #include "btwuser.h"
**              #pragma pack(pop)
**
******************************************************************************/
/******************************************************************************
**
** Copyright (c) 1996-1997 by Bit 3 Computer Corporation.
** All Rights Reserved.
** License governs use and distribution.
**
******************************************************************************/

#ifndef BTWUSER_H
#define BTWUSER_H 1


// Regsiter user ISR internal device control code
#define IOCTL_BTBRIDGE_REGISTER_UISR \
    CTL_CODE(FILE_DEVICE_BTBRIDGE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Unregsiter user ISR internal device control code
#define IOCTL_BTBRIDGE_UNREGISTER_UISR \
    CTL_CODE(FILE_DEVICE_BTBRIDGE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Max number of user ISR registrations allowed
#define BT_MAX_UISR_REGISTR 64

// All possible cable interrupts
enum BT_CABLE_INTR
{
    BT_CINT_NONE,   // no cable interrupt
    BT_CINT_1,      // cable interrupt 1
    BT_CINT_2,      // cable interrupt 2
    BT_CINT_3,      // cable interrupt 3
    BT_CINT_4,      // cable interrupt 4
    BT_CINT_5,      // cable interrupt 5
    BT_CINT_6,      // cable interrupt 6
    BT_CINT_7,      // cable interrupt 7
    BT_CINT_MAX     // maximum cable interrupt + 1
};


// Memory mapped adapter register structure. The structure allows adapter
// registers to be read/written using pointer notation.
typedef struct
{
    BYTE byLocCmd1;         // Local node command register - read/write
    BYTE byLocIntrCtrl;     // Local node interrupt control register - read/write
    BYTE byLocStatus;       // Local node status register - read only
    BYTE byLocIntrStatus;   // Local node interrupt status register - read only
    BYTE byLocPciCtrl;      // Local node bus control register - read/write
    BYTE byLocNotUsed05;    // Local node not used register - offset 0x05
    BYTE byLocNotUsed06;    // Local node not used register - offset 0x06
    BYTE byLocNotUsed07;    // Local node not used register - offset 0x07
    union    // union because both at - offset 0x08
    {
        BYTE byRemCmd1;     // Remote node command register 1 - write only
        BYTE byRemStatus;   // Remote node status register - read only
    } unCS;
    BYTE byRemCmd2;         // Remote node command register 2 - read/write - offset 0x09
    BYTE byRemPage;         // Remote node page register - offset 0x0a
    BYTE byRemNotUsed0b;    // Remote node not used register - offset 0x0b
    BYTE byRemAdapterId;    // Remote node adapter ID register - read/write (write has no effect)
    BYTE byRemAMod;         // Remote node remote VME bus address modifier register - read/write
    union   // union because IACK high reg can be accessed with word read only
    {
        BYTE byRemIackLow;  // Remote node remote IACK read low register - read only
        WORD wRemIack;      // Remote node remote IACK read both high & low registers - read only
    } unIack;
    BYTE byLocDmaCmd;       // Local DMA command register (read/write, 8 bits)
    BYTE byLocDmaRmdCnt;    // Local DMA remainder count register (read/write, 8 bits)
    WORD wLocDmaPktCnt;     // Local DMA packet count register (read/write, 16 bits)
    DWORD dwLocDmaAddr;     // Local Node DMA Address Register (read/write, 24 bits)
    BYTE byRemDmaRmdCnt;    // Remote DMA remainder count register (read/write, 8 bits)
    BYTE byRemNotUsed19;    // Remote node not used register - offset 0x19
    DWORD dwRemDmaAddr;     // Remote DMA address register (read/write, 32 bits)
    BYTE byRemSlave;        // Remote Slave Status register (read only, 8 bits)
    BYTE byRemNotUsed1f;    // Remote node not used register - offset 0x1f
} BT_REGMAP, *PBT_REGMAP;

// User ISR function pointer type
typedef DWORD (* PBT_UISR_HANDLER)(ULONG ulUnitNum, PVOID pParam, DWORD btIntrFlag);
typedef DWORD BT_UISR_HANDLER(ULONG ulUnitNum, PVOID pParam, DWORD btIntrFlag);

// User ISR information structure. This holds all the information the user
// ISR will need to access the adapter.
typedef struct
{
    PDWORD pMapReg;         // pointer to mapping register
    PBYTE pWin;             // pointer to window mapped by the mapping reg
    PBT_REGMAP pNodeIo;     // pointer to memory mapped node IO regs
} BT_UISR_INFO, *PBT_UISR_INFO;

// User ISR registration structure.
typedef struct
{
    ULONG ulUnit;           // unit number
    DWORD btIntrFlag;       // type of interrupt (BT_INTR_ERR, BT_INTR_PRG,
                            // BT_INTR_IACK)
    DWORD dwCIntrLevel;     // cable interrupt level (BT_CINT_NONE - BT_CINT7,
                            // only used on BT_INTR_IACK registrations) 
    PBT_UISR_HANDLER pHandler; // ptr to user's interrupt handler
    PVOID pParam;           // parameter passed to user's interrupt handler
} BT_UISR_REGR, *PBT_UISR_REGR;


//////////////////////////////////////////////////////////////////////////////
// Macros to load and read a mapping register
// Parameters are:
//  PMR:  Pointer to the mapping register in the adapter hardware
//  ADDR: Remote VME address
//  AMOD: VME address modifier. Use one of the VME_XXX #defines from btwapi.h
//  FUNC: Function code. Use one of the BT_MR_FUNC_XX #defines below
//  SWAP: Swap bits. Use one of the BT_tagSWAP_BITS_VALUE enum values from
//        btwapi.h
//////////////////////////////////////////////////////////////////////////////
#define BT_MR_FUNC_IO   0x00000010  // function code for remote bus I/O
#define BT_MR_FUNC_RAM  0x00000020  // function code for remote RAM
#define BT_MR_FUNC_DP   0x00000030  // function code for remote dual port RAM
#define BT_MR_FUNC      0x00000030  // all funcion code bits
#define BT_MR_ADRBT     0xfffff000  // remote physical address bits
#define BT_MR_ADMBT     0x00000fc0  // address modifier bits
#define BT_MR_ADMSHFT   6           // bits to shift AMOD into position
#define BT_MR_SWPBT     0x0000000e  // all the byte/word swapping bits
#define BT_MR_SWPSHFT   1           // bits to shift swap into position

#define LOAD_MAPREG(PMR, ADDR, AMOD, FUNC, SWAP) *(PMR) = \
                    (((ADDR) & BT_MR_ADRBT) | \
                     (((AMOD) << BT_MR_ADMSHFT) & BT_MR_ADMBT) | \
                     ((FUNC) & BT_MR_FUNC) | \
                     (((SWAP) << BT_MR_SWPSHFT) & BT_MR_SWPBT))

#define READ_MAPREG(PMR) *(PMR)

#endif // BTWUSER_H

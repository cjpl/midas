/*****************************************************************************
**
**      Filename:   btapi.h
**
**      Purpose:    Bit 3 Application Programming Interface header file.
**
**      $Revision$
**
******************************************************************************/
/*****************************************************************************
**
**        Copyright (c) 1997 by SBS Technologies, Inc.
**        Copyright (c) 1996-1997 by Bit 3 Computer Corporation.
**                     All Rights Reserved.
**              License governs use and distribution.
**
*****************************************************************************/

#ifndef _BTAPI_H
#define _BTAPI_H

#include <stddef.h>             /* gives us size_t and NULL */
#include "btdef.h"

/*
** Define BT_EXTERN
*/
#ifdef __cplusplus
#define BT_EXTERN extern "C"
#else                           /* __cplusplus */
#define BT_EXTERN extern
#endif                          /* __cplusplus */

/*****************************************************************************
**
**  Product specific include files
**
*****************************************************************************/

#if defined(BT21805)
#include "btuapi.h"             /* Universe specific include file */
#endif                          /* BT21805 */

#if defined(BT28801)
#include "btvapi.h"
#endif                          /* BT28801 */

#if defined(BT973) || defined(BT983)
#include "btwapi.h"
#endif                          /* BT973 || BT983 */

#if defined(BT18901)
#include "btvapi.h"
#endif                          /* BT18901 */

#if defined(BT15901)
#include "btmapi.h"             /* Broadcast Memory specific include file */
#endif                          /* BT15901 */

/*****************************************************************************
**
**  Macros and types for the Bit 3 Application Programming Inteface.
**
*****************************************************************************/

/*****************************************************************************
**
**  Common types and defines for Bit 3 Application Programming Interface
**
*****************************************************************************/
/*
**  Flags for bt_open() and bt_mmap()
*/
#if !defined(BT_RD) && !defined(BT_WR) && !defined(BT_RDWR)
    /* any implementation which needs to redefined bt_accessflag_t
       should redefine BT_RD, BT_WR, and BT_RDWR */

typedef unsigned long bt_accessflag_t;
#define BT_RD  (1<<0)
#define BT_WR  (1<<1)
#define BT_RDWR (BT_RD | BT_WR)

#endif                          /* !BT_RD */

/*****************************************************************************
**
**  Prototypes for the Bit 3 Application Programming Inteface.
**
*****************************************************************************/
/*
**  Function prototypes for Bit 3 API
*/
#if !defined(BT973) && !defined(BT983) && !defined(BT15901)

/*
**                    Standard Prototypes
*/

BT_EXTERN bt_dev_t bt_str2dev(const char *name_p);
BT_EXTERN char *bt_gen_name(int unit, bt_dev_t logdev, char *devname_p,
                            size_t max_devname);
BT_EXTERN bt_error_t bt_open(bt_desc_t * btd_p, const char *devname_p,
                             bt_accessflag_t flags);
BT_EXTERN bt_error_t bt_close(bt_desc_t btd);
BT_EXTERN bt_error_t bt_chkerr(bt_desc_t btd);
BT_EXTERN bt_error_t bt_clrerr(bt_desc_t btd);
BT_EXTERN bt_error_t bt_perror(bt_desc_t btd, bt_error_t status, const char *msg_p);
BT_EXTERN char *bt_strerror(bt_desc_t btd, bt_error_t status,
                            const char *msg_p, char *buf_p, size_t buf_len);
BT_EXTERN bt_error_t bt_init(bt_desc_t btd);
BT_EXTERN bt_error_t bt_read(bt_desc_t btd, void *buf_p,
                             bt_devaddr_t xfer_off, size_t xfer_len,
                             size_t * xfer_done_p);
BT_EXTERN bt_error_t bt_write(bt_desc_t btd, void *buf_p,
                              bt_devaddr_t xfer_off, size_t xfer_len,
                              size_t * xfer_done_p);
BT_EXTERN bt_error_t bt_get_info(bt_desc_t btd, bt_info_t param, bt_devdata_t * value_p);
BT_EXTERN bt_error_t bt_set_info(bt_desc_t btd, bt_info_t param, bt_devdata_t value);
BT_EXTERN bt_error_t bt_icbr_install(bt_desc_t btd, bt_irq_t irq_type,
                                     bt_icbr_t * icbr_p, void *param_p,
                                     bt_data32_t vector);
BT_EXTERN bt_error_t bt_icbr_remove(bt_desc_t btd, bt_irq_t irq_type, bt_icbr_t * icbr_p);
BT_EXTERN bt_error_t bt_lock(bt_desc_t btd, bt_msec_t wait_len);
BT_EXTERN bt_error_t bt_unlock(bt_desc_t btd);
BT_EXTERN bt_error_t bt_mmap(bt_desc_t btd, void **map_p,
                             bt_devaddr_t map_off, size_t map_len,
                             bt_accessflag_t flags, bt_swap_t swapping);
BT_EXTERN bt_error_t bt_unmmap(bt_desc_t btd, void *map_p, size_t map_len);
BT_EXTERN bt_error_t bt_bind(bt_desc_t btd, bt_binddesc_t * bind_p,
                             bt_devaddr_t * offset_p, void *buf_p,
                             size_t buf_len, bt_accessflag_t flags, bt_swap_t swapping);
BT_EXTERN bt_error_t bt_unbind(bt_desc_t btd, bt_binddesc_t desc);
BT_EXTERN const char *bt_dev2str(bt_dev_t type);
BT_EXTERN bt_error_t bt_ctrl(bt_desc_t btd, int ctrl, void *param_p);

#else                           /* !BT973 && !BT983 && !defined(BT15901) */

/*
**                    Windows Prototypes
*/

/* Windows requires __stdcall between the return type and the function
   name- so we can't just hide it in BT_EXTERN. */
BT_EXTERN bt_dev_t __stdcall bt_str2dev(const char *name_p);
BT_EXTERN char *__stdcall bt_gen_name(int unit, bt_dev_t logdev,
                                      char *devname_p, size_t max_devname);
BT_EXTERN bt_error_t __stdcall bt_open(bt_desc_t * btd_p,
                                       const char *devname_p, bt_accessflag_t flags);
BT_EXTERN bt_error_t __stdcall bt_close(bt_desc_t btd);
BT_EXTERN bt_error_t __stdcall bt_chkerr(bt_desc_t btd);
BT_EXTERN bt_error_t __stdcall bt_clrerr(bt_desc_t btd);
BT_EXTERN bt_error_t __stdcall bt_perror(bt_desc_t btd, bt_error_t status,
                                         const char *msg_p);
BT_EXTERN char *__stdcall bt_strerror(bt_desc_t btd, bt_error_t status,
                                      const char *msg_p, char *buf_p, size_t buf_len);
BT_EXTERN bt_error_t __stdcall bt_init(bt_desc_t btd);
BT_EXTERN bt_error_t __stdcall bt_read(bt_desc_t btd, void *buf_p,
                                       bt_devaddr_t xfer_off,
                                       size_t xfer_len, size_t * xfer_done_p);
BT_EXTERN bt_error_t __stdcall bt_write(bt_desc_t btd, void *buf_p,
                                        bt_devaddr_t xfer_off,
                                        size_t xfer_len, size_t * xfer_done_p);
BT_EXTERN bt_error_t __stdcall bt_get_info(bt_desc_t btd, bt_info_t param,
                                           bt_devdata_t * value_p);
BT_EXTERN bt_error_t __stdcall bt_set_info(bt_desc_t btd, bt_info_t param,
                                           bt_devdata_t value);
BT_EXTERN bt_error_t __stdcall bt_icbr_install(bt_desc_t btd,
                                               bt_irq_t irq_type,
                                               bt_icbr_t * icbr_p,
                                               void *param_p, bt_data32_t vector);
BT_EXTERN bt_error_t __stdcall bt_icbr_remove(bt_desc_t btd,
                                              bt_irq_t irq_type, bt_icbr_t * icbr_p);
BT_EXTERN bt_error_t __stdcall bt_lock(bt_desc_t btd, bt_msec_t wait_len);
BT_EXTERN bt_error_t __stdcall bt_unlock(bt_desc_t btd);
BT_EXTERN bt_error_t __stdcall bt_mmap(bt_desc_t btd, void **map_p,
                                       bt_devaddr_t map_off, size_t map_len,
                                       bt_accessflag_t flags, bt_swap_t swapping);
BT_EXTERN bt_error_t __stdcall bt_unmmap(bt_desc_t btd, void *map_p, size_t map_len);
BT_EXTERN bt_error_t __stdcall bt_bind(bt_desc_t btd, bt_binddesc_t * bind_p,
                                       bt_devaddr_t * offset_p, void *buf_p,
                                       size_t buf_len, bt_accessflag_t flags,
                                       bt_swap_t swapping);
BT_EXTERN bt_error_t __stdcall bt_unbind(bt_desc_t btd, bt_binddesc_t desc);
BT_EXTERN const char *__stdcall bt_dev2str(bt_dev_t type);
BT_EXTERN bt_error_t __stdcall bt_ctrl(bt_desc_t btd, int ctrl, void *param_p);

#endif                          /* !BT973 && !BT983 && !BT15901 */

#endif                          /* _BTAPI_H */

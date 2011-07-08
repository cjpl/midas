/********************************************************************\

  Name:         mdsupport.h
  Created by:   Pierre Amaudruz, Stefan Ritt

  Contents:     Declarations for mdsupport.c for mdump, logger, lazylogger support

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file mdsupport.h
The mdsupport include file
*/

/** @defgroup mdsupportincludecode The mdsupport.h & mdsupport.c
 */

/** @defgroup mdsupportdefineh Midas dump support Define 
 */

/** @defgroup mdsupportmacroh Midas dump support Macros 
 */

/** @defgroup mdsupporterrorh Midas dump error code 
 */

/**dox***************************************************************/
/** @addtogroup mdsupportincludecode
 *  
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef OS_WINNT
#include <io.h>
#include <time.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef EXPRT
#define EXPRT
#endif

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**dox***************************************************************/
/** @addtogroup mdsupporterrorh
 *  
 *  @{  */
#define MD_SUCCESS               1     /**< Ok */
#define MD_EVENT_NOT_SWAPPED     2     /**< Not swapped */
#define MD_DONE                  2     /**< Operation complete */
#define MD_WRONG_BANK_TYPE    -100     /**< Wrong bank type (see @ref MDSUPPORT_Bank_Types) */
#define MD_BANK_NOT_FOUND     -101     /**< Bank not found */
#define MD_SWAP_ERROR         -102     /**< Error swapping */
#define MD_NOMORE_SLOT        -103     /**< No more space for fragment */
#define MD_UNKNOWN_FORMAT     -104     /**< Unknown format (see @ref MDSUPPORT_format) */

/**dox***************************************************************/
/** @} */ /* end of mdsupporterrorh */

/**dox***************************************************************/
/** @addtogroup mdsupportdefineh
 *  
 *  @{  */
/**
Display parameters */
#define D_RECORD       1     /**<  */
#define D_HEADER       2     /**<  */
#define D_EVTLEN       3     /**<  */

/**
Display mode options */
#define DSP_RAW    1         /**< Display raw data */
#define DSP_RAW_SINGLE    2  /**< Display raw data (no single mode) */
#define DSP_BANK   3         /**< Display data in bank format */
#define DSP_BANK_SINGLE   4  /**< Display only requested data in bank format */

/**
Display format */
#define DSP_UNK    0  /**< Display format unknown */
#define DSP_DEC    1  /**< Display data in decimal format*/
#define DSP_HEX    2  /**< Display data in headecimal format */
#define DSP_ASC    3  /**< Display data in ASCII format */

#define NLINE              8    /* number of elements for display routine */

#define MD_BANKLIST_MAX        32     /**< maximum number of banks to be found 
                                         by the bk_list() */
#define MD_STRING_BANKLIST_MAX          MD_BANKLIST_MAX * 4
                                       /**< to be used for bk_list() */
/**dox***************************************************************/
/** @} */ /* end of mdsupportdefineh */

/**dox***************************************************************/
/** @addtogroup mdsupportmacroh
 *  
 *  @{  */
/** @} */ /* end of mdsupportmacroh */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*---- function declarations ---------------------------------------*/
/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

  INT  mftp_open(char *destination, FTP_CON ** con);
  void EXPRT md_bank_display(void *pmbh, void *pbk, INT fmt, INT dsp_mode, INT dsp_fmt);
  void EXPRT md_event_display(void *pevt, INT data_fmt, INT dsp_mode, INT dsp_fmt, char * bn);
  INT EXPRT md_all_info_display(INT what);
  INT EXPRT md_physrec_display(INT data_fmt);
  INT EXPRT md_physrec_skip(INT data_fmt, INT bl);
  INT EXPRT md_physrec_get(INT data_fmt, void **prec, DWORD * psize);
  INT EXPRT md_file_rclose(INT data_fmt);
  INT EXPRT md_file_ropen(char *infile, INT data_fmt, INT openzip);
  INT EXPRT md_file_wopen(INT type, INT data_fmt, char *filename, INT * hDev);
  INT EXPRT md_file_wclose(INT handle, INT type, INT data_fmt, char *filename);
  INT EXPRT md_log_write(INT handle, INT data_fmt, INT type, void *prec, DWORD nbytes);
  INT EXPRT md_event_swap(INT data_fmt, void *pevent);
  INT EXPRT md_event_get(INT data_fmt, void **pevent, DWORD * psize);
  
#ifdef __cplusplus
}
#endif

/*------------ END --------------------------------------------------------------*/
/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
/** @} */ /* end of mdsupportincludecode */

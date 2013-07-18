/********************************************************************\

  Name:         ybos.h
  Created by:   Pierre Amaudruz, Stefan Ritt

  Contents:     Declarations for ybos data format

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file ybos.h
The YBOS include file
*/

/** @defgroup ybosincludecode The ybos.h & ybos.c
 */
/** @defgroup ybosdefineh YBOS Define 
 */
/** @defgroup ybosmacroh YBOS Macros 
 */
/** @defgroup yboserrorh YBOS error code 
 */

/**dox***************************************************************/
/** @addtogroup ybosincludecode
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

/**dox***************************************************************/
/** @addtogroup ybosdefineh
 *  
 *  @{  */

/********************************************************************/
/**
General parameters
*/
#ifdef YBOS_VERSION_3_3
#define YBOS_PHYREC_SIZE        8190  /**< I*4 */
#else
#define YBOS_PHYREC_SIZE        8192  /**< I*4 */
#endif
#define YBOS_HEADER_LENGTH      4
#define YBOS_BUFFER_SIZE        3*(YBOS_PHYREC_SIZE<<2) + MAX_EVENT_SIZE + 128  /**< in BYTES */

#define YB_BANKLIST_MAX        32     /**< maximum number of banks to be found 
                                         by the ybk_list() or bk_list() */
#define YB_STRING_BANKLIST_MAX          YB_BANKLIST_MAX * 4
                                       /**< to be used for xbk_list() */

/**dox***************************************************************/
/** @} */ /* end of ybosdefineh */

/**dox***************************************************************/
/** @addtogroup yboserrorh
 *  
 *  @{  */
#define YB_SUCCESS               1     /**< Ok */
#define YB_EVENT_NOT_SWAPPED     2     /**< Not swapped */
#define YB_DONE                  2     /**< Operation complete */
#define YB_WRONG_BANK_TYPE    -100     /**< Wrong bank type (see @ref YBOS_Bank_Types) */
#define YB_BANK_NOT_FOUND     -101     /**< Bank not found */
#define YB_SWAP_ERROR         -102     /**< Error swapping */
#define YB_NOMORE_SLOT        -103     /**< No more space for fragment */
#define YB_UNKNOWN_FORMAT     -104     /**< Unknown format (see @ref YBOS_format) */

/**dox***************************************************************/
/** @} */ /* end of yboserrorh */

/**dox***************************************************************/
/** @addtogroup ybosdefineh
 *  
 *  @{  */

/**
record header content */
#define H_BLOCK_SIZE   0     /**< YBOS */
#define H_BLOCK_NUM    1     /**< YBOS */
#define H_HEAD_LEN     2     /**< YBOS */
#define H_START        3     /**< YBOS */

/**
Display parameters */
#define D_RECORD       1     /**< YBOS */
#define D_HEADER       2     /**< YBOS */
#define D_EVTLEN       3     /**< YBOS */

/**
File fragmentation */
#define YB_COMPLETE       1     /**< YBOS */
#define YB_INCOMPLETE     2     /**< YBOS */
#define YB_NO_RECOVER    -1     /**< YBOS */
#define YB_NO_RUN         0     /**< YBOS */
#define YB_ADD_RUN        1     /**< YBOS */

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

/**dox***************************************************************/
/** @} */ /* end of ybosdefineh */

/**dox***************************************************************/
/** @addtogroup ybosmacroh
 *  
 *  @{  */

/*---- Macros for YBOS ------------------------*/
/**
word swap (I4=I21I22 -> I4=I22I21*/
#define SWAP_D2WORD(_d2w) {\
  WORD _tmp2;                                    \
  _tmp2                 = *((WORD *)(_d2w));     \
  *((WORD *)(_d2w))     = *(((WORD *)(_d2w))+1); \
  *(((WORD *)(_d2w))+1) = _tmp2;                 \
}

/* the s for the following macros */
#ifdef CHAOS_EVID_FMT
extern chaos;
#endif

/********************************************************************/
/**
As soon as the Midas header is striped out from the event, the YBOS
remaining  data has lost the event synchonization unless included by the
user. It is therefore necessary to have a YBOS bank duplicating
this information usually done in the FE by creating a
"EVID" bank filled with the Midas info and other user information.

Unfortunately the format of this EVID is flexible and I couldn't
force user to use a default structure. For this reason, I'm
introducing a preprocessor flag for selecting such format.

Omitting the declaration of the pre-processor flag the EVID_TRINAT is taken by
default see @ref AppendixD.

Special macros are avaialbe to retrieve this information
based on the EVID content and the type of EVID structure.

The Macro parameter should point to the first data of the EVID bank.
\code
  // check if EVID is present if so display its content 
  if ((status = ybk_find (pybos, "EVID", &bklen, &bktyp, (void *)&pybk)) == YB_SUCCESS)
  {
    pdata = (DWORD *)((YBOS_BANK_HEADER *)pybk + 1);
    pevent->event_id      = YBOS_EVID_EVENT_ID(pdata);
    pevent->trigger_mask  = YBOS_EVID_TRIGGER_MASK(pdata);
    pevent->serial_number = YBOS_EVID_SERIAL(pdata);
    pevent->time_stamp    = YBOS_EVID_TIME(pdata);
    pevent->data_size     = pybk->length;
  }
\endcode

The current type of EVID bank are:
- [EVID_TRINAT] Specific for Trinat experiment.
\code
  ybk_create((DWORD *)pevent, "EVID", I4_BKTYPE, (DWORD *)(&pbkdat));
  *((WORD *)pbkdat) = EVENT_ID(pevent);     ((WORD *)pbkdat)++;
  *((WORD *)pbkdat) = TRIGGER_MASK(pevent); ((WORD *)pbkdat)++;
  *(pbkdat)++ = SERIAL_NUMBER(pevent);
  *(pbkdat)++ = TIME_STAMP(pevent);
  *(pbkdat)++ = gbl_run_number;                // run number 
\endcode
- [EVID_TWIST] Specific to Twist Experiment (Triumf).
\code
  ybk_create((DWORD *)pevent, "EVID", I4_BKTYPE, &pbkdat);
  *((WORD *)pbkdat) = EVENT_ID(pevent);     ((WORD *)pbkdat)++;
  *((WORD *)pbkdat) = TRIGGER_MASK(pevent); ((WORD *)pbkdat)++;
  *(pbkdat)++ = SERIAL_NUMBER(pevent);
  *(pbkdat)++ = TIME_STAMP(pevent);
  *(pbkdat)++ = gbl_run_number;                // run number 
  *(pbkdat)++ = *((DWORD *)frontend_name);     // frontend name 
  ybk_close((DWORD *)pevent, pbkdat);
\endcode
*/
#if (!defined (EVID_TRINAT) && !defined (EVID_CHAOS) && !defined (EVID_TWIST))
#define EVID_TRINAT
#endif

#if defined(EVID_TRINAT)
#define YBOS_EVID_EVENT_ID(e)      *((WORD *)(e)+1)
#define YBOS_EVID_TRIGGER_MASK(e)  *((WORD *)(e)+0)
#define YBOS_EVID_SERIAL(e)        *((DWORD *)(e)+1)
#define YBOS_EVID_TIME(e)          *((DWORD *)(e)+2)
#define YBOS_EVID_RUN_NUMBER(e)    *((DWORD *)(e)+3)
#define YBOS_EVID_EVENT_NB(e)      *((DWORD *)(e)+1)
#elif defined(EVID_CHAOS)
#define YBOS_EVID_EVENT_ID(e)      *((WORD *)(e)+3)
#define YBOS_EVID_TRIGGER_MASK(e)  *((WORD *)(e)+2)
#define YBOS_EVID_SERIAL(e)        *((DWORD *)(e)+2)
#define YBOS_EVID_TIME(e)          *((DWORD *)(e)+3)
#define YBOS_EVID_RUN_NUMBER(e)    *((DWORD *)(e)+4)
#define YBOS_EVID_EVENT_NB(e)      *((DWORD *)(e)+0)
#elif defined(EVID_TWIST)
#define YBOS_EVID_EVENT_ID(e)      *((WORD *)(e)+1)
#define YBOS_EVID_TRIGGER_MASK(e)  *((WORD *)(e)+0)
#define YBOS_EVID_SERIAL(e)        *((DWORD *)(e)+1)
#define YBOS_EVID_TIME(e)          *((DWORD *)(e)+2)
#define YBOS_EVID_RUN_NUMBER(e)    *((DWORD *)(e)+3)
#define YBOS_EVID_EVENT_NB(e)      *((DWORD *)(e)+1)
/* frontend name ignored */
#endif

/********************************************************************/
/**
pevt Evt# id/msk serial run# */
#define YBOS_EVID_BANK(__a, __b, __c,   __d,   __e) {\
      DWORD * _pbuf;\
      ybk_create(__a, "EVID", I4_BKTYPE, &_pbuf);\
      *(_pbuf)++ = (DWORD)__b;\
      *(_pbuf)++ = (DWORD)__c;\
      *(_pbuf)++ = (DWORD)__d;\
      *(_pbuf)++ = (DWORD)ss_millitime();\
      *(_pbuf)++ = (DWORD)__e;\
      ybk_close(__a, _pbuf);\
        }

/********************************************************************/
/**
pevt Evt# id/msk serial run# */
#define MIDAS_EVID_BANK(__a, __b, __c,   __d,   __e) {\
      DWORD * _pbuf;\
      bk_create(__a, "EVID", TID_DWORD, &_pbuf);\
      *(_pbuf)++ = (DWORD)__b;\
      *(_pbuf)++ = (DWORD)__c;\
      *(_pbuf)++ = (DWORD)__d;\
      *(_pbuf)++ = (DWORD)ss_millitime();\
      *(_pbuf)++ = (DWORD)__e;\
      bk_close(__a, _pbuf);\
        }

/**dox***************************************************************/
/** @} */ /* end of ybosmacroh */

/**dox***************************************************************/
/** @addtogroup ybosdefineh
 *  
 *  @{  */

/*---- data structures for YBOS file format ------------------------*/
/**
YBOS Bank types */
#define I2_BKTYPE       1  /**< Signed Integer 2 bytes */
#define A1_BKTYPE       2  /**< ASCII 1 byte */
#define I4_BKTYPE       3  /**< Signed Interger 4bytes */
#define F4_BKTYPE       4  /**< Float 4 bytes */
#define D8_BKTYPE       5  /**< Double 8 bytes */
#define I1_BKTYPE       8  /**< Signed Integer 1 byte */
#define MAX_BKTYPE      I1_BKTYPE+1 /**< delimiter */

/**dox***************************************************************/
/** @} */ /* end of ybosdefineh */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/**
YBOS logger channel ALL in 4BYTES! */
typedef struct {
   DWORD *ptop;        /**< pointer to top of YBOS buffer */
   DWORD *pbuf;        /**< current data pointer for writing to buffer */
   DWORD *pwrt;        /**< current data pointer for writing to device */
   DWORD *pbot;        /**< bottom of the physical record */
   DWORD *pend;        /**< end of the buffer */
   DWORD reco;         /**< offset to first logical record 4BYTES */
   DWORD recn;         /**< current YBOS physical record number */
} YBOS_INFO;

/**
YBOS Physical record header */
typedef struct {
   DWORD rec_size;          /**< LPR Length of Physical Record (exclusive) */
   DWORD header_length;     /**< LPH Length of Physical Header (inclusive) */
   DWORD rec_num;           /**< NPR Physical Record Number (start 0) */
   DWORD offset;            /**< LRD Offset to 1st Logical Record 
                                   (=LPH for 1st Logical record) */
} YBOS_PHYSREC_HEADER;

/**
Bank header */
typedef struct {
   DWORD name;              /**< bank name 4ASCII */
   DWORD number;            /**< bank number (=1) */
   DWORD index;             /**< index within bank (=0) */
   DWORD length;            /**< bank length (I*4, exclusive ) */
   DWORD type;              /**< bank type refer to above */
} YBOS_BANK_HEADER;

/**
YBOS FILE parameters */
#define MAX_FILE_PATH    128
#define MAX_FRAG_SIZE   2000    /* max event size for fragmented file (bytes) */
#define MAX_YM_FILE        8    /* max concurrent file handling */
#define NLINE              8    /* number of elements for display routine */

/**
YBOS control file header (private structure) */
typedef struct {
   INT file_ID;
   INT size;
   INT fragment_size;
   INT total_fragment;
   INT current_fragment;
   INT current_read_byte;
   INT run_number;
   INT spare;
} YM_CFILE;

/**
YBOS path file header (private structure) */
typedef struct {
   char path[MAX_FILE_PATH];
} YM_PFILE;

/**
YBOS file replay handler (for multiple file entries) */
typedef struct {
   INT fHandle;
   INT file_ID;
   INT current_fragment;
   INT current_read_byte;
   char path[MAX_FILE_PATH];
} R_YM_FILE;

/*---- function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

   INT EXPRT yb_file_recompose(void *pevt, INT fmt, char *svpath, INT file_mode);
   INT EXPRT feodb_file_dump(EQUIPMENT * eqp, char *eqpname, char *pevent,
                             INT run_number, char *path);

   void EXPRT yb_any_bank_display(void *pmbh, void *pbk, INT fmt,
                                  INT dsp_mode, INT dsp_fmt);
   void EXPRT yb_any_event_display(void *pevt, INT data_fmt, INT dsp_mode, INT dsp_fmt, char * bn);
   INT EXPRT yb_any_all_info_display(INT what);
   INT EXPRT yb_any_physrec_display(INT data_fmt);

   INT EXPRT yb_any_physrec_skip(INT data_fmt, INT bl);
   INT EXPRT yb_any_physrec_get(INT data_fmt, void **prec, DWORD * psize);
   INT EXPRT yb_any_file_rclose(INT data_fmt);
   INT EXPRT yb_any_file_ropen(char *infile, INT data_fmt);
   INT EXPRT yb_any_file_wopen(INT type, INT data_fmt, char *filename, INT * hDev);
   INT EXPRT yb_any_file_wclose(INT handle, INT type, INT data_fmt, char *filename);
   INT EXPRT yb_any_log_write(INT handle, INT data_fmt, INT type,
                              void *prec, DWORD nbytes);
   INT EXPRT yb_any_event_swap(INT data_fmt, void *pevent);
   INT EXPRT yb_any_event_get(INT data_fmt, void **pevent, DWORD * psize);

/* Bank manipulation */
   void EXPRT ybk_init(DWORD * pevent);
   void EXPRT ybk_create(DWORD * pevent, char *bkname, DWORD btype, void *pbkdat);
   INT EXPRT ybk_close(DWORD * pevent, void *pbkdat);
   INT EXPRT ybk_size(DWORD * pevent);
   INT EXPRT ybk_list(DWORD * pevent, char *bklist);
   INT EXPRT ybk_locate(DWORD * pevent, char *bkname, void *pdata);
   INT EXPRT ybk_find(DWORD * pevent, char *bkname, DWORD * bklength,
                      DWORD * bktype, void **pbkdata);
   void EXPRT ybk_create_chaos(DWORD * pevent, char *bname, DWORD btype, void *pbkdat);
   INT EXPRT ybk_iterate(DWORD * pevent, YBOS_BANK_HEADER ** pybkh, void **pdata);
   INT EXPRT ybk_close_chaos(DWORD * pevent, DWORD btype, void *pbkdat);

#ifdef HAVE_LOGGING
   INT EXPRT ybos_log_open(LOG_CHN * log_chn, INT run_number);
   INT EXPRT ybos_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size);
   INT EXPRT ybos_log_close(LOG_CHN * log_chn, INT run_number);
#endif

   INT EXPRT ybos_event_get(DWORD ** plrl, DWORD * size);
   INT EXPRT ybos_get_tid_size(INT tid);

#ifdef __cplusplus
}
#endif

/*------------ END --------------------------------------------------------------*/
/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
/** @} */ /* end of ybosincludecode */

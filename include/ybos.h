/********************************************************************\

  Name:         ybos.h
  Created by:   Pierre Amaudruz, Stefan Ritt

  Contents:     Declarations for ybos data format

  Revision history
  ------------------------------------------------------------------
  date        by    modification
  ---------   ---   ------------------------------------------------
*  $Log$
*  Revision 1.11  2000/04/17 17:22:24  pierre
*  - First round of doc++ comments
*
*  Revision 1.10  1999/12/20 08:38:25  midas
*  Defined ybos_event_get
*
*  Revision 1.9  1999/09/30 22:52:16  pierre
*  - arg of yb_any_bank_display
*
*  Revision 1.8  1999/06/23 09:59:22  midas
*  Added D8_BKTYPE
*
*  Revision 1.7  1999/01/19 19:56:59  pierre
*  - Fix prototype
*  - Added YB_UNKNOWN_FORMAT
*
*  Revision 1.6  1999/01/18 17:34:36  pierre
*  - cleanup definitions and structures for ybos
*  - Correct prototype for ybos
*
\********************************************************************/

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

/*---- General parameters -----------------------------*/
#define YBOS_PHYREC_SIZE        8190  /* I*4 */
#define YBOS_HEADER_LENGTH      4
#define YBOS_BUFFER_SIZE        3*(YBOS_PHYREC_SIZE<<2) + MAX_EVENT_SIZE + 128           /* in BYTES */

#define YB_BANKLIST_MAX        32     /* maximum number of banks to be found 
                                         by the xbk_list() */
#define YB_STRING_BANKLIST_MAX          YB_BANKLIST_MAX * 4
                                       /* to be used for xbk_list() */

#define YB_SUCCESS               1
#define YB_EVENT_NOT_SWAPPED     2
#define YB_DONE                  2
#define YB_WRONG_BANK_TYPE    -100
#define YB_BANK_NOT_FOUND     -101
#define YB_SWAP_ERROR         -102
#define YB_NOMORE_SLOT        -103
#define YB_UNKNOWN_FORMAT     -104

/* record header content */
#define H_BLOCK_SIZE   0     /* YBOS */
#define H_BLOCK_NUM    1     /* YBOS */
#define H_HEAD_LEN     2     /* YBOS */
#define H_START        3     /* YBOS */

/* Display parameters */
#define D_RECORD       1
#define D_HEADER       2
#define D_EVTLEN       3

/* File fragmentation */
#define YB_COMPLETE       1
#define YB_INCOMPLETE     2
#define YB_NO_RECOVER    -1
#define YB_NO_RUN         0
#define YB_ADD_RUN        1

/*---- Macros for YBOS ------------------------*/
/* word swap (I4=I21I22 -> I4=I22I21 */
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

/*                                          s=TRUE>CHAOS        s=FALSE>NORMAL */
#define YBOS_EVID_EVENT_ID(e)      *(e-2) == 6 ? *((WORD *)(e)+3)  : *((WORD *)(e)+1)
#define YBOS_EVID_TRIGGER_MASK(e)  *(e-2) == 6 ? *((WORD *)(e)+2)  : *((WORD *)(e)+0)
#define YBOS_EVID_SERIAL(e)        *(e-2) == 6 ? *((DWORD *)(e)+2) : *((DWORD *)(e)+1)
#define YBOS_EVID_TIME(e)          *(e-2) == 6 ? *((DWORD *)(e)+3) : *((DWORD *)(e)+2)
#define YBOS_EVID_RUN_NUMBER(e)    *(e-2) == 6 ? *((DWORD *)(e)+4) : *((DWORD *)(e)+3)
#define YBOS_EVID_EVENT_NB(e)      *(e-2) == 6 ? *((DWORD *)(e)+0) : *((DWORD *)(e)+1)

/*                     pevt Evt# id/msk serial run# */
#define YBOS_EVID_BANK(__a, __b, __c,   __d,   __e) {\
      DWORD * pbuf;\
      ybk_create(__a, "EVID", I4_BKTYPE, &pbuf);\
      *(pbuf)++ = (DWORD)__b;\
      *(pbuf)++ = (DWORD)__c;\
      *(pbuf)++ = (DWORD)__d;\
      *(pbuf)++ = (DWORD)ss_millitime();\
      *(pbuf)++ = (DWORD)__e;\
      ybk_close(__a, pbuf);\
        }

/*                      pevt Evt# id/msk serial run# */
#define MIDAS_EVID_BANK(__a, __b, __c,   __d,   __e) {\
      DWORD * pbuf;\
      bk_create(__a, "EVID", TID_DWORD, &pbuf);\
      *(pbuf)++ = (DWORD)__b;\
      *(pbuf)++ = (DWORD)__c;\
      *(pbuf)++ = (DWORD)__d;\
      *(pbuf)++ = (DWORD)ss_millitime();\
      *(pbuf)++ = (DWORD)__e;\
      bk_close(__a, pbuf);\
        }

/*---- data structures for YBOS file format ------------------------*/
/* YBOS Bank types */
#define I2_BKTYPE       1
#define A1_BKTYPE       2
#define I4_BKTYPE       3
#define F4_BKTYPE       4
#define D8_BKTYPE       5
#define I1_BKTYPE       8
#define MAX_BKTYPE      I1_BKTYPE+1

/* YBOS logger channel ALL in 4BYTES! */
typedef struct {
  DWORD *ptop;         /* pointer to top of YBOS buffer */
  DWORD *pbuf;         /* current data pointer for writing to buffer */
  DWORD *pwrt;         /* current data pointer for writing to device */
  DWORD *pbot;         /* bottom of the physical record */
  DWORD *pend;         /* end of the buffer */
  DWORD  reco;         /* offset to first logical record 4BYTES */
  DWORD  recn;         /* current YBOS physical record number */
} YBOS_INFO;

/* YBOS Physical record header */
typedef struct {
  DWORD  rec_size;          /* LPR Length of Physical Record (exclusive) */
  DWORD  header_length;     /* LPH Length of Physical Header (inclusive) */
  DWORD  rec_num;           /* NPR Physical Record Number (start 0) */
  DWORD  offset;            /* LRD Offset to 1st Logical Record 
                                   (=LPH for 1st Logical record) */
} YBOS_PHYSREC_HEADER;

/* Bank header */
typedef struct {
  DWORD  name;              /* bank name 4ASCII */
  DWORD  number;            /* bank number (=1) */
  DWORD  index;             /* index within bank (=0) */
  DWORD  length;            /* bank length (I*4, exclusive ) */
  DWORD  type;              /* bank type refer to above */
} YBOS_BANK_HEADER;


/* YBOS FILE parameters */
#define MAX_FILE_PATH    128
#define MAX_FRAG_SIZE   2000          /* max event size for fragmented file (bytes) */
#define MAX_YM_FILE        8          /* max concurrent file handling */
#define NLINE              8          /* number of elements for display routine */

/* YBOS control file header (private structure) */
typedef struct{
  INT file_ID;
  INT size;
  INT fragment_size;
  INT total_fragment;
  INT current_fragment;
  INT current_read_byte;
  INT run_number;
  INT spare;
} YM_CFILE;

/* YBOS path file header (private structure) */
typedef struct{
  char path[MAX_FILE_PATH];
} YM_PFILE;

/* YBOS file replay handler (for multiple file entries) */
typedef struct {
  INT fHandle;
  INT file_ID;
  INT current_fragment;
  INT current_read_byte;
  char path[MAX_FILE_PATH];
} R_YM_FILE;

/* Display mode options */
#define DSP_RAW    1
#define DSP_BANK   2

/* Display format */
#define DSP_DEC    1
#define DSP_HEX    2
#define DSP_ASC    3

/*---- function declarations ---------------------------------------*/
INT   EXPRT yb_file_recompose(void * pevt, INT fmt, char * svpath, INT file_mode);
INT   EXPRT feodb_file_dump (EQUIPMENT * eqp, char * eqpname,char * pevent, INT run_number, char *path);

void  EXPRT yb_any_bank_display(void * pmbh, void * pbk, INT fmt, INT dsp_mode, INT dsp_fmt);
void  EXPRT yb_any_event_display(void * pevt, INT data_fmt, INT dsp_mode, INT dsp_fmt);
INT   EXPRT yb_any_all_info_display (INT what);
INT   EXPRT yb_any_physrec_display(INT data_fmt);

INT   EXPRT yb_any_physrec_skip(INT data_fmt, INT bl);
INT   EXPRT yb_any_physrec_get (INT data_fmt, void ** prec, DWORD * psize);
INT   EXPRT yb_any_file_rclose (INT data_fmt);
INT   EXPRT yb_any_file_ropen(char * infile, INT data_fmt);
INT   EXPRT yb_any_file_wopen (INT type, INT data_fmt, char * filename, INT * hDev);
INT   EXPRT yb_any_file_wclose (INT handle, INT type, INT data_fmt);
INT   EXPRT yb_any_log_write (INT handle, INT data_fmt, INT type, void * prec, DWORD nbytes);
INT   EXPRT yb_any_event_swap (INT data_fmt, void * pevent);
INT   EXPRT yb_any_event_get (INT data_fmt, void ** pevent, DWORD * psize);

/* Bank manipulation */
INT   EXPRT bk_list (BANK_HEADER * pmbh, char * bklist);
INT   EXPRT bk_find (BANK_HEADER * pmbh, char * bkname, DWORD * bklen, DWORD * bktype, void **pbk);

/** ybk\_init()
    \begin{description}
    \item[Description:] Initializes an event for YBOS banks structure.
    \item[Remarks:] Before banks can be created in an event, ybk\_init()
    has to be called first.  See \Ref{YBOS bank examples}.
    \end{description}
    @memo Initialize an event.
    @param pevent pointer to the first DWORD of the event area of event 
    @return void
*/
void  EXPRT ybk_init        (DWORD *pevent);

/** ybk\_create()
    \begin{description}
    \item[Description:] Define the following memory area to be a YBOS bank with the
    given attribute.  See \Ref{YBOS bank examples}.
    \item[Remarks:] Before banks can be created in an event, ybk\_init(). 
    has to be called first. YBOS does not support mixed bank type. i.e: all the
    data are expected to be of the same type. YBOS is a 4 bytes bank aligned structure.
    Padding is performed at the closing of the bank (see \Ref{ybk\_close}) with values of
    0x0f or/and 0x0ffb. See \Ref{YBOS bank examples}.
    \end{description}
    @memo Create a YBOS bank.
    @param pevent pointer to the first DWORD of the event area.
    @param bkname name to be assigned to the breated bank (max 4 char)
    @param btype \Ref{YBOS Bank Types} of the values for the entire created bank.
    @param pbkdat return pointer to the first empty data location.
    @return void
*/
void  EXPRT ybk_create      (DWORD *pevent, char *bkname, DWORD btype, void *pbkdat);

/** ybk\_close()
    \begin{description}
    \item[Description:] Close the YBOS bank previously created by \Ref{ybk_create}.
    \item[Remarks:] The data pointer pdata must be obtained by \Ref{ybk_create()} and
    used as an address to fill a bank. It is incremented with every value written
    to the bank and finally points to a location just after the last byte of the
    bank. It is then passed to ybk\_close() to finish the bank creation. YBOS is a
    4 bytes bank aligned structure. Padding is performed at the closing of the bank
    with values of 0x0f or/and 0x0ffb. See \Ref{YBOS bank examples}.
    \end{description}
    @memo Close YBOS bank.
    @param pevent pointer to current composed event.
    @param pbkdata  pointer to the current data.
    @return void
 */
void  EXPRT ybk_close       (DWORD *pevent, void *pbkdat);

/** ybk\_size()
    \begin{description}
    \item[Description:] Returns the size in bytes of the event composed of YBOS bank(s).
    \item[Remarks:]
    \end{description}
    @memo compute event size in bytes.
    @param pevent pointer to the area of event
    @return number of bytes contained in data area of event 
*/
INT   EXPRT ybk_size        (DWORD *pevent);

/** ybk\_list()
    \begin{description}
    \item[Description:] Returns the size in bytes of the event composed of YBOS bank(s).
    \item[Remarks:] The bklist has to be a predefined string of max size of
    YB\_STRING\_BANKLIST\_MAX.
    \end{description}
    @memo Extract the bank list of an event composed of YBOs banks.
    @param pevent pointer to the area of event
    @param bklist Filled character string of the YBOS bank names found in the event.
    @return number of banks found in this event.
*/
INT   EXPRT ybk_list        (DWORD *pevent, char *bklist);

/** ybk\_locate()
    \begin{description}
    \item[Description:] Locate the requested bank and return the pointer to the
    top of the data section.
    \item[Remarks:]
    \end{description}
    @memo Locate bank in event.
    @param pevent pointer to the area of event
    @param bkname name of the bank to be located.
    @param pdata pointer to the first data of the located bank.
    @return  YB\_SUCCESS, YB\_BANK\_NOT\_FOUND, YB\_WRONG\_BANK\_TYPE
*/
INT   EXPRT ybk_locate      (DWORD *pevent, char * bkname, void *pdata);

/** ybk\_find()
    \begin{description}
    \item[Description:] Find the requested bank and return the infirmation if the bank as well
    as the pointer to the top of the data section.
    \item[Remarks:]
    \end{description}
    @memo Find bank in event.
    @param pevent pointer to the area of event.
    @param bkname name of the bank to be located.
    @param bklength returned length in 4bytes unit of the bank.
    @param bktype returned bank type.
    @param pbkdata pointer to the first data of the found bank.
    @return  YB\_SUCCESS, YB\_BANK\_NOT\_FOUND, YB\_WRONG\_BANK\_TYPE
*/
INT   EXPRT ybk_find        (DWORD *pevent, char *bkname, DWORD *bklength, DWORD *bktype
                           , void **pbkdata);
void  EXPRT ybk_create_chaos(DWORD *pevent, char *bname, DWORD btype, void *pbkdat);

/** ybk\_iterate()
    \begin{description}
    \item[Description:] Returns the bank header pointer and data pointer of the given
    bank name.
    \item[Remarks:]
    \end{description}
    @memo Find bank in event.
    @param pevent pointer to the area of event.
    @param bkname name of the bank to be located.
    @param pybkh pointer to the YBOS bank header.
    @param pdata pointer to the first data of the current bank.
    @return  data length in 4 bytes unit. return 0 (zero) if no more bank found.
*/
INT   EXPRT ybk_iterate     (DWORD *pevent, YBOS_BANK_HEADER ** pybkh, void ** pdata);
void  EXPRT ybk_close_chaos (DWORD *pevent, DWORD btype, void *pbkdat);

#ifdef INCLUDE_LOGGING
INT   EXPRT ybos_log_open(LOG_CHN * log_chn, INT run_number);
INT   EXPRT ybos_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size);
INT   EXPRT ybos_log_close(LOG_CHN * log_chn, INT run_number);
#endif

INT   EXPRT ybos_event_get (DWORD ** plrl, DWORD * size);

/*------------ END --------------------------------------------------------------*/

/********************************************************************\

  Name:         ybos.h
  Created by:   Pierre Amaudruz, Stefan Ritt

  Contents:     Declarations for ybos data format


  Revision history
  ------------------------------------------------------------------
  date        by    modification
  ---------   ---   ------------------------------------------------
  15-JAN-97   PAA   created
  12-MAY-97   SR    changed RING_SIZE from NET_BUFFER_SIZE to
                    MAX_EVENT_SIZE

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
/* All parameters given in 4bytes word. */

#define YBOS_BLK_SIZE        8190
#define YBOS_NB_BLK             4
#define YBOS_HEADER_LENGTH      4
#define RING_BLK_SIZE        YBOS_BLK_SIZE + (YBOS_NB_BLK-1)*(YBOS_BLK_SIZE-YBOS_HEADER_LENGTH)
#define RING_SIZE            RING_BLK_SIZE*4 + MAX_EVENT_SIZE   /* + max event size (midas)*/
#define YBOS_SUCCESS            1
#define YBOS_FAIL_MALLOC_RING  -1
#define YBOS_FAIL_OPEN         -2
#define YBOS_FAIL_FLUSH        -3
#define YBOS_WRONG_BANK_TYPE   -4
#define YBOS_BANK_NOT_FOUND    -5

/* rdlog global definitions */
#define NLINE                    8    /* number of elements for display routine */
#define RDLOG_SUCCESS            1
#define RDLOG_DONE               2
#define RDLOG_EOR                3
#define RDLOG_COMPLETE           4
#define RDLOG_INCOMPLETE         5

/* Global error */
#define RDLOG_FAIL_OPEN         -1
#define RDLOG_FAIL_READING      -2
#define RDLOG_EOF_REACHED       -3
#define RDLOG_DEV_CLOSED        -4
#define RDLOG_BLOCK_TOO_BIG     -5
#define RDLOG_ALREADY_OPEN      -6
#define RDLOG_UNKNOWN_FMT       -7
#define RDLOG_FAIL_CLOSE        -8
#define RDLOG_NOMORE_SLOT       -9

/* YBOS swapping flags */
#define YBOS_EVENT_SWAPPED       1
#define YBOS_EVENT_NOT_SWAPPED   0
#define YBOS_SWAP_ERROR         -1

/* YBOS Bank types */
#define I2_BKTYPE       1
#define A1_BKTYPE       2
#define I4_BKTYPE       3
#define F4_BKTYPE       4
#define I1_BKTYPE       8

/* record header content */
#define H_BLOCK_SIZE   0     /* YBOS */
#define H_BLOCK_NUM    1     /* YBOS */
#define H_HEAD_LEN     2     /* YBOS */
#define H_START        3     /* YBOS */

/* rdlog global definitions */
#define NLINE                    8    /* number of elements for display routine */
#define RDLOG_SUCCESS            1
#define RDLOG_DONE               2
#define RDLOG_EOR                3

/* Display mode options */
#define DSP_RAW    1
#define DSP_BANK   2
#define DSP_DEC    1
#define DSP_HEX    2
#define DSP_ASC    3

/* file option */
#define NO_RECOVER  -1
#define NO_RUN       0
#define ADD_RUN      1

/* Display parameters */
#define D_RECORD       1
#define D_HEADER       2
#define D_EVENT        3
#define D_EVTLEN       4

/* rlog global definitions */
#define MAX_DLOG_UNIT            8    /* max data logger channels */
#define MAX_DLOG_FMT             1    /* max data logger data format type */
#define MAX_EVT_SIZE         0xFFFF   /* max event size */
#define MAX_BLOCK_SIZE       0xFFFF   /* max block size */
#define MAX_DLOG_NAME_SIZE     128    /* max data logger name size */
#define MAX_DLOG_LABEL_SIZE     32    /* max data logger label size */
#define MAX_FILE_PATH          128
#define YBOS_FMT                 0    /* known data logger data format */
#define DLOG_TAPE                1    /* type of device (tape file) */
#define DLOG_DISK                2    /* type of device (disk file) */
#define VT                       1    /* type of device for error log */

/*---- Macros for YBOS ------------------------*/

/* word swap (I4=I21I22 -> I4=I22I21 */
#define SWAP_D2WORD(_d2w) {\
  WORD _tmp2;                                    \
  _tmp2                 = *((WORD *)(_d2w));     \
  *((WORD *)(_d2w))     = *(((WORD *)(_d2w))+1); \
  *(((WORD *)(_d2w))+1) = _tmp2;                 \
}

/*---- data structures for YBOS file format ------------------------*/

/* YBOS FILE parameters */
#define  MAX_FRAG_SIZE   2000          /* max event size for fragmented file (bytes) */
#define  MAX_YBOS_FILE      8          /* max concurrent file handling */

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
} YBOS_CFILE;

/* YBOS path file header (private structure) */
typedef struct{
  char path[128];
} YBOS_PFILE;

/* YBOS file replay handler (for multiple file entries) */
typedef struct {
  INT fHandle;
  INT file_ID;
  INT current_fragment;
  INT current_read_byte;
  char path[MAX_FILE_PATH];
} R_YBOS_FILE;

/*---- data structures for YBOS format -----------------------------*/

/* Midas YBOS logger channel */
typedef struct {
  DWORD *top_ptr;         /* top ring_buffer data pointer */
  DWORD *bot_ptr;         /* bottom ring_buffer data pointer */
  DWORD *rd_cur_ptr;      /* current read data pointer (from buffer to tape) */
  DWORD *wr_cur_ptr;      /* current write (from event to buffer) */
  DWORD *blk_end_ptr;     /* end of Ybos data block pointer */
  DWORD  event_offset;     /* offset to first logical record in bytes */
  DWORD  cur_block;        /* current YBOS block number */
} YBOS_INFO;

/* YBOS Physical record header */
typedef struct {
  DWORD  block_size;        /* LPR Length of Physical Record (exclusive) */
  DWORD  header_length;     /* LPH Length of Physical Header (inclusive) */
  DWORD  block_num;         /* NPR Physical Record Number (start 0) */
  DWORD  offset;            /* LRD Offset to 1st Logical Record (=LPH for 1st logical record */
                           /*     in first physical record (== LPH)*/
} YBOS_PHYSREC_HEADER;

/* YBOS BANK record
   The MONO-TYPE YBOS bank should have the following structure
   I*4 Bank name            ... (4 ascii)
   I*4 Bank number             0
   I*4 Index in Bank           1
   I*4 Length of Bank       ... (exclusive from this point in I*4)
   I*4 Bank type            ... (see #define)
   */
typedef struct {
  DWORD  name;              /* bank name 4ASCII */
  DWORD  number;            /* bank number (=1) */
  DWORD  index;             /* index within bank (=0) */
  DWORD  length;            /* bank length (I*4, exclusive ) */
  DWORD  type;              /* bank type refer to above */
} YBOS_BANK_HEADER;

/*---- function declarations ---------------------------------------*/

void  EXPRT display_any_bank(void * pbk, INT data_fmt, INT dsp_mode, INT dsp_fmt);
void  EXPRT display_any_event(void * pevt, INT data_fmt, INT dsp_mode, INT dsp_fmt);
void  EXPRT display_any_physrec(INT data_fmt, INT dsp_fmt);
void  EXPRT display_any_physhead(INT data_fmt, INT dsp_fmt);
void  EXPRT display_any_evtlen(INT data_fmt, INT dsp_fmt);

INT   EXPRT get_any_physrec(INT data_fmt);
INT   EXPRT get_any_event(INT data_fmt, INT bl, void *prevent);
INT   EXPRT skip_any_physrec(INT data_fmt, INT bl);
INT   EXPRT swap_any_event(INT data_fmt, void * prevent);

INT   EXPRT open_any_datafile (INT data_fmt, char * datafile);
INT   EXPRT close_any_datafile (INT data_fmt);

INT   EXPRT open_any_logfile (INT log_type, INT data_fmt, char * device_name, HNDLE * hDev);
INT   EXPRT close_any_logfile (INT log_type, HNDLE hDev);

INT   EXPRT ybos_odb_file_dump(INT hBuf, INT ev_ID, INT run_number, char * path);
INT   EXPRT ybos_feodb_file_dump(EQUIPMENT * eqp, DWORD* pevent, INT run_number, char * path);
INT   EXPRT ybos_file_compose(char * pevent, char * path, INT file_mode);

void  EXPRT ybk_init(DWORD *pevent);
void  EXPRT ybk_create(DWORD *pevent, char *bname, DWORD bt, DWORD *pbkdat);
void  EXPRT ybk_close(DWORD *pevent, void *pbkdat);
INT   EXPRT ybk_size (DWORD *pevent);
void  EXPRT ybk_create_chaos(DWORD *pevent, char *bname, DWORD bt, void *pbkdat);
void  EXPRT ybk_close_chaos(DWORD *pevent, DWORD bt, void *pbkdat);

INT   EXPRT ybk_list(DWORD *pevt , char *bklist);
INT   EXPRT ybk_find (DWORD *pevt , char * bkname, DWORD * bklength, DWORD * bktype, DWORD **pbkdata);
INT   EXPRT ybk_iterate (DWORD * pevt , YBOS_BANK_HEADER ** pybkh, void ** pbkdata);

#ifdef INCLUDE_LOGGING
INT   EXPRT ybos_log_open(LOG_CHN * log_chn, INT run_number);
INT   EXPRT ybos_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size);
INT   EXPRT ybos_flush_buffer(LOG_CHN * log_chn);
INT   EXPRT ybos_log_close(LOG_CHN * log_chn, INT run_number);
#endif


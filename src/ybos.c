/*  Copyright (c) 1993      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 *
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca         Tel: (604) 222-1047    Fax: (604) 222-1074
 *         amaudruz@triumf.ca                            Local:           6234
 * -----------------------------------------------------------------------------
 *
 *  Description	: ybos.c : contains functions for YBOS bank manipulation
 *
 *  Requires 	: ybos.h
 *  Author:  Pierre-Andre Amaudruz Data Acquisition Group
 *
 *  Revision 1.0  1994/Oct Pierre	 Initial revision
 *  Revision History: Oct/96  :
 *    1 April 97: S. Ritt, minor modification to supress compiler warnings
 *  $Log$
 *  Revision 1.5  1998/10/22 12:40:34  midas
 *  Added "oflag" to ss_tape_open()
 *
 *  Revision 1.4  1998/10/19 17:45:42  pierre
 *  - new ybos_magta_write
 *
 *  Revision 1.3  1998/10/12 12:19:03  midas
 *  Added Log tag in header
 *
 *  Revision 1.2  1998/10/12 11:59:11  midas
 *  Added Log tag in header
/*---------------------------------------------------------------------------*/
/* include files */
#include "midas.h"
#include "msystem.h"
#ifdef INCLUDE_ZLIB
#include "zlib.h"
#endif

#define INCLUDE_LOGGING
#include "ybos.h"

/* hidden prototypes */
INT    ybos_extract_info(INT fmt, INT element);
void   ybos_display_all_info(INT what);

void  display_raw_event(void * pevt, INT dsp_fmt);
void  display_ybos_event(DWORD * pevt, INT dsp_fmt);
void  display_midas_event(EVENT_HEADER * pevt, INT dsp_fmt);

void  ybos_display_raw_bank(DWORD * pbk, INT dsp_fmt);
void  ybos_display_ybos_bank( DWORD *pbk, INT dsp_fmt);
void  midas_display_raw_bank(void  * pbk, INT dsp_fmt);
void  midas_display_midas_bank(void *pbk, INT dsp_fmt);

INT   ybos_event_swap (DWORD * pevt);
INT   ybos_open_datafile (char * filename);
INT   ybos_close_datafile (void);
INT   ybos_skip_physrec (INT bl);
INT   ybos_physhead_get (void);
INT   ybos_physrec_get (void);
INT   ybos_physhead_display (void);
INT   ybos_physrec_display (void);
INT   ybos_evtlen_display (void);
INT   ybos_event_get (INT bl, DWORD * prevent);

INT   ybos_file_open(int *slot, char *pevt, char * svpath, INT file_mode);
INT   ybos_file_update(int slot, char * pevent);
INT   ybos_fefile_dump(EQUIPMENT * eqp, EVENT_HEADER * pevent, INT run_number, char * path);
INT   ybos_file_dump(INT hBuf, INT ev_ID, INT run_number, char * path);
void  ybos_log_odb_dump(LOG_CHN *log_chn, short int event_id, INT run_number);

INT   ybos_log_open(LOG_CHN * log_chn, INT run_number);
INT   ybos_write(LOG_CHN * log_chn, EVENT_HEADER * pevent, INT evt_size);
INT   ybos_flush_buffer(LOG_CHN * log_chn);
INT   ybos_log_close(LOG_CHN * log_chn, INT run_number);

/* MAGTA parameters for YBOS disk file
   When the disk file has a *BOT record at the BOF then,
   VMS can read nicely the file. YBOS package knows how to
   deal with this too. The format in I*4 is then:
   0x00000004 (record length in bytes)
   0x544f422a (the record content "*BOT")
   0x7ff8 (record length in bytes)
   0x1ffd x 0x00000000 (empty record)
   0x7ff8 (record length in bytes)
   0x1ffd x user data
   0x7ff8 (record length in bytes)
   0x1ffd x user data
   :
   :
   */

#if !defined (FE_YBOS_SUPPORT)
DWORD magta[3]={0x00000004, 0x544f422a, 0x00007ff8};
INT magta_flag = 0;
DWORD  *pbot;

/* rdlog general info */
struct {
  char     name[MAX_DLOG_NAME_SIZE];    /* Device name (/dev/nrmt0h) */
  INT      handle;                      /* file handle */
  DWORD    block_size;                  /* current block size */
  DWORD    block_size_32;               /* current block size */
  DWORD   *header_ptr;                  /* pointer to block header */
  char    *begrec_ptr;                  /* backup pointer to begin of record */
  char    *rd_cur_ptr;                  /* current read point in record */
  DWORD    cur_evt;                     /* current event number */
  DWORD    cur_evt_len;                 /* current event length (-1 if not available) */
  DWORD    cur_block;                   /* current block number */
  DWORD    fmt;                         /* contains FORMAT type */
  DWORD    dev_type;                    /* Device type (tape, disk, ...) */
} rdlog;

R_YBOS_FILE  yfile[MAX_YBOS_FILE];

char * plazy;
INT    szlazy;
BOOL   zipfile = FALSE;
#ifdef INCLUDE_ZLIB
gzFile  filegz;
#endif
DWORD *prevent;
YBOS_PHYSREC_HEADER *ybosbl;

struct stat *filestat;

/*---- LOGGER YBOS format routines ----------------------------------------*/
INT open_any_logfile(INT type, INT format, char * path, HNDLE *handle)
/********************************************************************\
  Routine: open_any_logfile
  Purpose: Open a logging channel for either type (Disk/Tape/FTP) in
           either format (MIDAS/YBOS)
  Input:
  INT type       : Disk, Tape, FTP
  INT format     : MIDAS, YBOS
  char * path    : Device name

  Output:
  HNDLE * handle ; returned handle of the open device
    none
 Function value:
    error, success
\********************************************************************/
{
INT         status, written;

  /* Create device channel */
  if (type == LOG_TYPE_TAPE)
  {
    return ss_tape_open(path, O_WRONLY | O_CREAT | O_TRUNC, handle);
  }
  else if ((type == LOG_TYPE_DISK) && (format == FORMAT_YBOS))
  {
#ifdef OS_WINNT       
    *handle = (int) CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | 
                      FILE_FLAG_SEQUENTIAL_SCAN, 0);
#else
    *handle = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
#endif
    if (*handle < 0)
	     return SS_FILE_ERROR;
      /* specific to YBOS Disk structure */
      /* write MAGTA header in bytes 0x4, "*BOT" */
#ifdef OS_WINNT
    WriteFile((HANDLE *) *handle, (char *) magta, 8, &written, NULL);
    status = written == 8 ? SS_SUCCESS : SS_FILE_ERROR;
#else
    status = write(*handle, (char *) magta, 8) == 8 ? SS_SUCCESS : SS_FILE_ERROR;
#endif
    /* allocate temporary emtpy record */
    pbot = realloc (pbot, magta[2]-4);
    memset ((char *)pbot,0,magta[2]-4);
    /* write BOT empty record for MAGTA */
#ifdef OS_WINNT
    WriteFile((HANDLE) *handle, (char *) pbot, magta[2]-4, &written, NULL);
    status = written == (INT) magta[2]-4 ? SS_SUCCESS : SS_FILE_ERROR;
#else
    status = write(*handle, (char *) pbot, magta[2]-4) == (INT) magta[2]-4 ? SS_SUCCESS : SS_FILE_ERROR;
#endif
  }
  else if ((type == LOG_TYPE_FTP) && (format == FORMAT_MIDAS))
  {
  }
    return 1;
}

/*---- LOGGER YBOS format routines ----------------------------------------*/
INT close_any_logfile(INT type, HNDLE handle)
/********************************************************************\
  Routine: close_any_logfile
  Purpose: close a logging channel for either type (Disk/Tape/FTP) in
  Input:
  INT type       : Disk, Tape, FTP
  HNDLE * handle ; returned handle of the open device

  Output:
    none
 Function value:
    error, success
\********************************************************************/
{
  /* Write EOF if Tape */
  if (type == LOG_TYPE_TAPE)
  {
    /* writing EOF mark on tape only */
    ss_tape_write_eof(handle);
    ss_tape_write_eof(handle);
    ss_tape_fskip(handle, -1);
    ss_tape_close(handle);
  }
  else if (type == LOG_TYPE_DISK)
  {
#ifdef OS_WINNT
    CloseHandle((HANDLE) handle);
#else
    close(handle);
#endif
  }
  else if (type == LOG_TYPE_FTP)
  {
    
  }
  return 1;
}

/*---- LOGGER YBOS format routines ----------------------------------------*/
INT ybos_log_open(LOG_CHN *log_chn, INT run_number)
/********************************************************************\
  Routine: ybos_log_open
  Purpose: Open a logger channel in YBOS fmt
  Input:
    LOG_CHN * log_chn      Concern log channel
    INT   run_number       run number
  Output:
    none
 Function value:
    error, success
\********************************************************************/
{
  YBOS_INFO * ybos;
  INT status;

  /* allocate YBOS buffer info */
  log_chn->format_info = malloc(sizeof(YBOS_INFO));

  ybos = (YBOS_INFO *) log_chn->format_info;

  /* reset memory */
  memset (ybos,0,sizeof(YBOS_INFO));

  if (ybos == NULL)
    {
    log_chn->handle = 0;
    return SS_NO_MEMORY;
    }

  /* allocate full ring buffer for that channel */
  if ( (ybos->top_ptr = (DWORD *) malloc(RING_SIZE)) == NULL)
    {
    free(ybos);
    log_chn->handle = 0;
    return SS_NO_MEMORY;
    }

  memset ((char *) ybos->top_ptr, 0, RING_SIZE);
  /* Setup YBOS pointers */
  ybos->event_offset = YBOS_HEADER_LENGTH;
  ybos->rd_cur_ptr   = ybos->top_ptr + YBOS_HEADER_LENGTH;
  ybos->wr_cur_ptr   = ybos->rd_cur_ptr;
  ybos->blk_end_ptr  = ybos->top_ptr + YBOS_BLK_SIZE;
  ybos->bot_ptr      = ybos->top_ptr + RING_BLK_SIZE;

  status = open_any_logfile(log_chn->type, log_chn->format, log_chn->path, &log_chn->handle); 
  if (status != SS_SUCCESS)
  {
    free(ybos->top_ptr);
    free(ybos);
    log_chn->handle = 0;
    return status;
  }
  
  /* write ODB dump */
  if (log_chn->settings.odb_dump)
    ybos_log_odb_dump(log_chn, EVENTID_BOR, run_number);
  
  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ybos_magta_write( INT logH, char *pwt, INT nbytes)
/********************************************************************\
  Routine: ybos_magta_write
  Purpose: Special data write using magta format for YBOS
  Input:
    HANDLE logH        Devive handle
    char * pwt         pointer to data
    INT    nbytes      data size in bytes
  Output:
    none
 Function value:
    error, success
\********************************************************************/
  {
    INT status, written;
#ifdef OS_WINNT
        WriteFile((HANDLE) logH, (char *) ((DWORD *)(magta+2)), 4, &written, NULL);
        status = written == 4 ? SS_SUCCESS : SS_FILE_ERROR;
#else
        status = write(logH, (char *)((DWORD *)(magta+2)), 4) == 4 ? SS_SUCCESS : SS_FILE_ERROR;
#endif
#ifdef OS_WINNT
        WriteFile((HANDLE) logH, (char *) pwt, nbytes, &written, NULL);
        status = written == nbytes ? SS_SUCCESS : SS_FILE_ERROR;
#else
        status = write(logH, (char *)pwt, nbytes) == nbytes ? SS_SUCCESS : SS_FILE_ERROR;
#endif
        return status;
  }

/*------------------------------------------------------------------*/
INT ybos_flush_buffer(LOG_CHN *log_chn)
/********************************************************************\
  Routine: ybos_flush_buffer
  Purpose: Empty the internal buffer to logger channel for YBOS fmt
  Input:
    LOG_CHN * log_chn      Concern log channel
  Output:
    none
 Function value:
    error, success
\********************************************************************/
{
INT         status;
YBOS_INFO   *ybos;

  ybos = (YBOS_INFO *) log_chn->format_info;

  /* adjust read pointer to beg of record */
  ybos->rd_cur_ptr -= YBOS_HEADER_LENGTH;

  ybosbl = (YBOS_PHYSREC_HEADER *)ybos->rd_cur_ptr;

  ybosbl->block_size    = YBOS_BLK_SIZE-1;       /* exclusive */
  ybosbl->header_length = YBOS_HEADER_LENGTH;    /* inclusive */
  ybosbl->block_num     = ybos->cur_block;
  ybosbl->offset        = ybos->event_offset;    /* exclusive from block_size */

/* YBOS known only about fix record size. The way to find out
   it there is no more valid event is to look at the LRL for -1
   put some extra -1 in the current physical record */
  memset((DWORD *)ybos->wr_cur_ptr,-1,64);

  /* write record to device */
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    status = ss_tape_write(log_chn->handle, (char *)ybos->rd_cur_ptr, YBOS_BLK_SIZE*4);
    log_chn->statistics.bytes_written += YBOS_BLK_SIZE*4;
    log_chn->statistics.bytes_written_total += YBOS_BLK_SIZE*4;
    }
  else
    {
    /* write MAGTA header (4bytes)=0x7ff8 */
    status = ybos_magta_write(log_chn->handle, (char *) ybos->rd_cur_ptr, YBOS_BLK_SIZE*4);
    log_chn->statistics.bytes_written += YBOS_BLK_SIZE*4 + 4;
    log_chn->statistics.bytes_written_total += YBOS_BLK_SIZE*4 + 4;
    }
  return status;
}

  /*------------------------------------------------------------------*/
INT ybos_write(LOG_CHN *log_chn, EVENT_HEADER *pevent, INT evt_size)
/********************************************************************\
  Routine: ybos_write
  Purpose: Write an event to the logger channel in YBOS fmt
  Input:
    LOG_CHN *      log_chn      Concern log channel
    EVENT_HEADER * pevent       event pointer to Midas header
    INT            evt_size     event size in bytes seen by Midas
  Output:
    none
 Function value:
    error, success
\********************************************************************/
{
  short int   evid, evmsk;
  INT         status, left_over_length, datasize, event_size;
  YBOS_INFO   *ybos;
  DWORD       *pbkdat,*pbktop;
  DWORD        bfsize;
  
  /* Check the Event ID for :
     Midas BOR/EOR which include the ODB dump : 0x8000/0x8001
     Msg dump from MUSER flag from odb mainly : 0x8002
     */
  
  evid = pevent->event_id;
  evmsk= pevent->trigger_mask;

  /* shortcut to ybos struct */
  ybos = (YBOS_INFO *) log_chn->format_info;
  
  /* detect if event is message oriented (ASCII) */
  if ((evid >= EVENTID_BOR) && (evid <= EVENTID_MESSAGE))
    {
      /* skip ASCII dump if not MUSER */
      if (!(evmsk & MT_USER))
	return SS_SUCCESS;

      /* skip if MUSER  but not Log message enabled */
      if (!(MT_USER & log_chn->settings.log_messages))
	return SS_SUCCESS;

      /* ASCII event has to be recasted YBOS */
      /* skip if event too long (>32Kbytes) */
      if (pevent->data_size > 32000)
	      {
	        cm_msg(MINFO,"ybos_write","MMSG or MODB event too large");
	        return SS_SUCCESS;
	      }

      /* align to DWORD boundary in bytes */
      datasize = 4 * (pevent->data_size + 3) / 4;

      /* overall buffer size in bytes */
      bfsize = datasize + sizeof(YBOS_BANK_HEADER) + 4;   /* +LRL */

      /* allocate space */
      pbktop = (DWORD *) malloc (bfsize);
      memset (pbktop, 0, bfsize);
      ybk_init((DWORD *) pbktop);

      printf("pbktop:%p (%x)\n",pbktop, *pbktop);
      /* open bank depending on event type */
      if (evid == EVENTID_MESSAGE)
      	ybk_create(pbktop, "MMSG", A1_BKTYPE, (DWORD *)&pbkdat);
      else
      	ybk_create(pbktop, "MODB", A1_BKTYPE, (DWORD *)&pbkdat);

      printf("pbktop:%p (%x)\n",pbktop, *pbktop);
      memcpy ((char *)pbkdat, (char *)(pevent+1), pevent->data_size);
      (char *) pbkdat += datasize;
      ybk_close(pbktop, pbkdat);
      
      printf("pbktop:%p (%x)\n",pbktop, *pbktop);
      printf("%x - %x-%x-%x - %x-%x-%x-%x\n",
	     *pbktop,*(pbktop+1),*(pbktop+2),*(pbktop+3),
	     *(pbktop+4),*(pbktop+5),*(pbktop+6),*(pbktop+7));

      /* event size in bytes for Midas */
      event_size = *pbktop * 4;
      
      /* swap bytes if necessary based on the ybos.bank_type */
      ybos_event_swap((DWORD *)pbktop);
      printf("pbktop:%p (%x) [%x]\n",pbktop, *pbktop, event_size);
      
      /* Event with MIDAS header striped out */
      memcpy( (char *)ybos->wr_cur_ptr, (char *) pbktop, event_size);
      
      /* move write pointer to next free location (DWORD) */
      ybos->wr_cur_ptr += (event_size>>2);
      
      free (pbktop);

      status = SS_SUCCESS;
    }
  else
    {
      /* skip the Midas EVENT_HEADER */
      pevent++;

      /* swap bytes if necessary based on the ybos.bank_type */
      ybos_event_swap((DWORD *)pevent);
      
      /* event size include the Midas EVENT_HEADER... don't need for ybos
	     I do this in order to keep the log_write from mlogger intact */
      
      evt_size -= sizeof(EVENT_HEADER);
      
      /* Event with MIDAS header striped out */
      memcpy( (char *)ybos->wr_cur_ptr, (char *) pevent, evt_size);
      
/*    if (evid == 2)
      display_any_bank( (void *)((DWORD *)pevent+1), FORMAT_YBOS, DSP_BANK, DSP_HEX);
      */

      /* move write pointer to next free location (DWORD) */
      ybos->wr_cur_ptr += (evt_size>>2);
      
      status = SS_SUCCESS;
    }

  /* wr_cur beyond bot => send block to device and reset point to top "ring" */
  if (ybos->wr_cur_ptr     >= ybos->bot_ptr)
    {
    ybos->rd_cur_ptr     -= YBOS_HEADER_LENGTH;
    ybosbl                = (YBOS_PHYSREC_HEADER *)(ybos->rd_cur_ptr);
    ybosbl->block_size    = YBOS_BLK_SIZE-1;     /* exclusive */
    ybosbl->header_length = YBOS_HEADER_LENGTH;
    ybosbl->block_num     = ybos->cur_block;
    ybosbl->offset        = ybos->event_offset;

    if (log_chn->type == LOG_TYPE_TAPE)
      {
      status = ss_tape_write(log_chn->handle, (char *)ybos->rd_cur_ptr, YBOS_BLK_SIZE*4);
      log_chn->statistics.bytes_written += YBOS_BLK_SIZE*4;
      log_chn->statistics.bytes_written_total += YBOS_BLK_SIZE*4;
      }
    else
      {
      /* write MAGTA header (4bytes)=0x7ff8 */
        status = ybos_magta_write(log_chn->handle, (char *) ybos->rd_cur_ptr, YBOS_BLK_SIZE*4);
        log_chn->statistics.bytes_written += YBOS_BLK_SIZE*4 + 4;
        log_chn->statistics.bytes_written_total += YBOS_BLK_SIZE*4 + 4;
      }
    if (status != SS_SUCCESS)
      return status;

    ybos->rd_cur_ptr      = ybos->top_ptr + YBOS_HEADER_LENGTH;
    left_over_length      = ybos->wr_cur_ptr - ybos->bot_ptr;
    memcpy(ybos->rd_cur_ptr, ybos->bot_ptr, left_over_length*4);
    ybos->cur_block++;
    ybos->wr_cur_ptr      = ybos->rd_cur_ptr + left_over_length;
    ybos->blk_end_ptr     = ybos->top_ptr    + YBOS_BLK_SIZE;
    ybos->event_offset    = ybos->wr_cur_ptr - ybos->rd_cur_ptr + 4; /* YBOS header */
    }

  /*  wr_cur beyond blk_end => block full send to device */
  while (ybos->wr_cur_ptr  >= ybos->blk_end_ptr)
    {
      ybos->rd_cur_ptr     -= YBOS_HEADER_LENGTH;
      ybosbl                = (YBOS_PHYSREC_HEADER *)(ybos->rd_cur_ptr);
      ybosbl->block_size    = YBOS_BLK_SIZE-1;     /* exclusive */
      ybosbl->header_length = YBOS_HEADER_LENGTH;
      ybosbl->block_num     = ybos->cur_block;
      ybosbl->offset        = ybos->event_offset;

      /* write record to device */
      if (log_chn->type == LOG_TYPE_TAPE)
        {
        status = ss_tape_write(log_chn->handle, (char *)ybos->rd_cur_ptr, YBOS_BLK_SIZE*4);
        log_chn->statistics.bytes_written += YBOS_BLK_SIZE*4;
        log_chn->statistics.bytes_written_total += YBOS_BLK_SIZE*4;
        }
      else
        {
        /* write MAGTA header (4bytes)=0x7ff8 */
          status = ybos_magta_write(log_chn->handle, (char *)ybos->rd_cur_ptr, YBOS_BLK_SIZE*4);
          log_chn->statistics.bytes_written += YBOS_BLK_SIZE*4 + 4;
          log_chn->statistics.bytes_written_total += YBOS_BLK_SIZE*4 + 4;
        }
      if (status != SS_SUCCESS)
        return status;

      /* count physical records (blocks) */
      ybos->cur_block++;
      ybos->rd_cur_ptr  += YBOS_BLK_SIZE;
      ybos->event_offset = ybos->wr_cur_ptr - ybos->rd_cur_ptr + YBOS_HEADER_LENGTH;
      ybos->blk_end_ptr  = ybos->rd_cur_ptr + YBOS_BLK_SIZE - YBOS_HEADER_LENGTH;
    } /* while */

/* update statistics */
  log_chn->statistics.events_written++;

  return status;
}

/*------------------------------------------------------------------*/
INT ybos_log_close(LOG_CHN *log_chn, INT run_number)
/********************************************************************\
  Routine: ybos_log_close
  Purpose: Close a logger channel in YBOS fmt
  Input:
    LOG_CHN * log_chn      Concern log channel
    INT   run_number       run number
  Output:
    none
 Function value:
    error, success
\********************************************************************/
{
INT status;
YBOS_INFO   *ybos;

  ybos = (YBOS_INFO *) log_chn->format_info;

  /* Write EOF mark and close the device */
  /* flush buffer before closing */
  status = ybos_flush_buffer(log_chn);

  if (status != SS_SUCCESS)
    return status;

  status = close_any_logfile(log_chn->type, log_chn->handle);

  free(ybos->top_ptr);
  free(ybos);

  return SS_SUCCESS;
}

/*---- ODB dump routines --------------------------------------------*/
void ybos_log_odb_dump(LOG_CHN *log_chn, short int event_id, INT run_number)
/********************************************************************\
  Routine: ybos_log_odb_dump
  Purpose: Serves the logger flag /logger/ODB dump
           Extract the ODB in ASCII format and send it to the logger channel
           In ybos format the file will be fragmented in the form of:
           YBOSevt[(YBOS_CFILE)(YBOS_PFILE)(YBOS_DFILE)]
  Input:
    LOG_CHN * log_chn      Concern log channel
    short in  event_id     event ID
    INT   run_number       run number
  Output:
    none
 Function value:
    none
\********************************************************************/
{
INT          status, buffer_size, size;
EVENT_HEADER *pevent;
HNDLE         hDB;

  cm_get_experiment_database (&hDB, NULL);
  /* write ODB dump */
  buffer_size = 10000;
  do
    {
    pevent = malloc(buffer_size);
    if (pevent == NULL)
      {
        cm_msg(MERROR, "ybos_log_odb_dump", "Cannot allocate ODB dump buffer");
        break;
      }

    size = buffer_size - sizeof(EVENT_HEADER);
    status = db_copy(hDB, 0, (char *) (pevent+1), &size, "");
    if (status != DB_TRUNCATED)
      {
        bm_compose_event(pevent, event_id, MIDAS_MAGIC,
            buffer_size-sizeof(EVENT_HEADER)-size+1, run_number);
        ybos_write(log_chn, pevent, pevent->data_size+sizeof(EVENT_HEADER));
        break;
      }

    /* increase buffer size if truncated */
    free(pevent);
    buffer_size *= 2;
    } while (1);
 }

/*------------------------------------------------------------------*/
INT   ybos_file_dump(INT hBuf, INT ev_ID, INT run_number, char * path)
/********************************************************************\
  Routine: ybos_file_dump
  Purpose: Fragment file in order to send it through Midas.
           This version compose an event of the form of:
           Midas_header[(YBOS_CFILE)(YBOS_PFILE)(YBOS_DFILE)]
  Input:
    INT    hBuf            Handle of the Midas system buffer
    INT   ev_ID            event ID for all file oriented YBOS bank
    INT   run_number       currrent run_number
    char * path            full file name specification
  Output:
    none
  Function value:
    RDLOG_SUCCESS          Successful completion
    RDLOG_OPEN_FAILED      file doesn't exist
    -2                     file not closed
\********************************************************************/
  {
  INT dmpf, status, remaining;
  INT nread, filesize, nfrag;
  INT allheader_size;
  DWORD *pbuf, *pcfile, *pybos;
  YBOS_CFILE ybos_cfileh;
  YBOS_PFILE ybos_pfileh;
  EVENT_HEADER  *pheader;

  /* check if file exists */
  /* Open for read (will fail if file does not exist) */
  if( (dmpf = open(path, O_RDONLY | O_BINARY, 0644 )) == -1 )
  	{
	  cm_msg(MINFO,"ybos_file_dump","File dump -Failure- on open file %s",path);
	  return RDLOG_FAIL_OPEN;
	  }

  /* get file size */
  filestat = malloc( sizeof(struct stat) );
  stat(path,filestat);
  filesize = filestat->st_size;
  free(filestat);
  cm_msg(MINFO,"ybos_file_dump","Accessing File %s (%i)",path, filesize);

  /* compute fragmentation & initialize*/
  nfrag = filesize / MAX_FRAG_SIZE;

  /* Generate a unique FILE ID */
  srand( (unsigned)time( NULL ) );
  srand( (unsigned)time( NULL ) );

  /* Fill file YBOS_CFILE header */
  ybos_cfileh.file_ID = rand();
  ybos_cfileh.size = filesize;
  ybos_cfileh.total_fragment = nfrag + (((filesize % MAX_FRAG_SIZE)==0) ? 0 : 1);
  ybos_cfileh.current_fragment = 0;
  ybos_cfileh.current_read_byte = 0;
  ybos_cfileh.run_number= run_number;
  ybos_cfileh.spare= 0xabcd;

  /* Fill file YBOS_PFILE header */
  memset (ybos_pfileh.path,0,sizeof(YBOS_PFILE));
  /* first remove path if present */
  if (strrchr(path,'/') != NULL)
    {
      strncpy(ybos_pfileh.path
	      ,strrchr(path,'/')+1
	      ,strlen(strrchr(path,'/')));
    }
  else
      strcpy(ybos_pfileh.path, path);

  /* allocate space */
  allheader_size = sizeof(EVENT_HEADER)
    + sizeof(YBOS_BANK_HEADER)           /* EVID bank header */
    + 5*sizeof(DWORD)                    /* EVID data size */
    + sizeof(YBOS_CFILE)
    + sizeof(YBOS_PFILE) + 64;

  /* Midas header and top of event */
  pheader = malloc(allheader_size + MAX_FRAG_SIZE);

  /* init the memory */
  memset (pheader, 0, allheader_size + MAX_FRAG_SIZE);
  
  /* YBOS bank header */
  pybos = (DWORD *) ((EVENT_HEADER *) pheader+1);

  /* read file */
  while (ybos_cfileh.current_fragment <= nfrag)
    {
      /*---- YBOS Event bank ----*/
      ybk_init(pybos);
      ybk_create(pybos, "EVID", I4_BKTYPE, (DWORD *)&pbuf);
      *(pbuf)++ = ybos_cfileh.current_fragment;          /* Event number */
      *(pbuf)++ = (DWORD)(ev_ID<<16);                    /* Event_ID + Mask */
      *(pbuf)++ = ybos_cfileh.current_fragment;          /* Serial number */
      *(pbuf)++ = ss_millitime();                        /* Time Stamp */
      *(pbuf)++ = run_number;                            /* run number */
      ybk_close(pybos, pbuf);

      /* Create YBOS bank */
      ybk_create(pybos, "CFIL", I4_BKTYPE, (DWORD *)&pbuf);
      /* save pointer for later */
      pcfile = pbuf;
      (char *)pbuf += sizeof(YBOS_CFILE);
      ybk_close(pybos, pbuf);

      ybk_create(pybos, "PFIL", A1_BKTYPE, (DWORD *)&pbuf);
      memcpy((char *)pbuf,(char *)&ybos_pfileh,sizeof(YBOS_PFILE));
      (char *)pbuf += sizeof(YBOS_PFILE);
      ybk_close(pybos, pbuf);

      ybk_create(pybos, "DFIL", A1_BKTYPE, (DWORD *)&pbuf);
      /* compute data length */
      remaining = filesize-ybos_cfileh.current_read_byte;
      nread = read(dmpf,(char *)pbuf,(remaining > MAX_FRAG_SIZE) ? MAX_FRAG_SIZE : remaining);

      /* adjust target pointer */
      (char *) pbuf += nread;
      /* keep track of statistic */
      ybos_cfileh.current_fragment++;
      ybos_cfileh.fragment_size = nread;
      ybos_cfileh.current_read_byte += nread;
      memcpy((char *)pcfile,(char *)&ybos_cfileh,sizeof(YBOS_CFILE));

      /* close YBOS bank */
      ybk_close(pybos, pbuf);

      /* Fill the midas header */
      bm_compose_event(pheader, (short) ev_ID, (short) 0, ybk_size(pybos), 0);

      /* get event size in bytes for midas */
      status = bm_send_event(hBuf,(char *)pheader
                            ,pheader->data_size + sizeof(EVENT_HEADER),SYNC);
    } /* while */

  /* cleanup memory */
  free (pheader);

  /* close file */
  if( close( dmpf) )
    {
      printf("File was not closed\n");
      return RDLOG_FAIL_CLOSE;
    }
  return RDLOG_SUCCESS;
}

/*--Dump tool routines ---------------------------------------------*/
INT   swap_any_event (INT data_fmt, void * prevent)
/********************************************************************\
  Routine: swap_any_event
  Purpose: Swap an event from the given data format.
  Input:
    INT data_fmt :  YBOS or MIDAS 
    void * prevent : pointer to the event
  Output:
    none
  Function value:
    status :  from the lower function
\********************************************************************/
{
  INT status;
  if (data_fmt == FORMAT_MIDAS)
     status = bk_swap(prevent, FALSE);
  else if (data_fmt == FORMAT_YBOS)
     status = ybos_event_swap ((DWORD *)prevent);
  return(status);
}

/*------------------------------------------------------------------*/
INT ybos_event_swap (DWORD * pevt)
/********************************************************************\
  Routine: ybos_event_swap
  Purpose: display on screen the YBOS data in YBOS bank mode.
  Input:
    DWORD * pevt           pointer to the YBOS event pointing to lrl
  Output:
    none
  Function value:
    YBOS_EVENT_SWAPPED                 Event has been swapped
    YBOS_EVENT_NOT_SWAPPED             Event has been not been swapped
    YBOS_SWAP_ERROR                    swapping error
\********************************************************************/
{
  DWORD    *pnextb, *pendevt, lrl;
  DWORD     bank_length, bank_type;

  /* check if bank_type < 9 then swapped otherwise return unswapped */
  if (*(pevt+5) < 0x9)
    return (YBOS_EVENT_NOT_SWAPPED);

  /* swap LRL */
  DWORD_SWAP (pevt);
  lrl = *pevt++;

  /* end of event pointer */
  pendevt = pevt + lrl;

  /* scan event */
  while (pevt < pendevt)
    {
      /* swap YBOS bank header for sure */
      /* bank name doesn't have to be swapped as it's a ASCII */
      pevt++;                                /* bank name */

      DWORD_SWAP(pevt);                      /* bank number */
      pevt++;

      DWORD_SWAP(pevt);                      /* bank index */
      pevt++;

      DWORD_SWAP(pevt);                      /* bank length */
      bank_length = *pevt;
      pevt++;

      DWORD_SWAP(pevt);                      /* bank type */
      bank_type   = *pevt;
      pevt++;

      /* pevt left pointing at first data in bank */

      /* pointer to next bank */
      pnextb = pevt + bank_length - 1;

      switch (bank_type)
	{
	case I4_BKTYPE :
	case F4_BKTYPE :
	  while ((BYTE *) pevt < (BYTE *) pnextb)
	    {
	      DWORD_SWAP(pevt);
	      pevt++;
	    }
	  break;
	case I2_BKTYPE :
	  while ((BYTE *) pevt < (BYTE *) pnextb)
	    {
	      WORD_SWAP(pevt);
	      ((WORD *)pevt)++;
	    }
	  break;
	case I1_BKTYPE :
	case A1_BKTYPE :
	  pevt = pnextb;
	  break;
	default :
	  printf("ybos_swap_event-E- Unknown bank type %i\n",bank_type);
    return (YBOS_SWAP_ERROR);
	  break;
	}
    }
  return (YBOS_EVENT_SWAPPED);
}

/*------------------------------------------------------------------*/
void display_any_event(void * pevt, INT data_fmt, INT dsp_mode, INT dsp_fmt)
/********************************************************************\
  Routine: midas_display_any_event
  Purpose: display on screen the YBOS event in either RAW or YBOS mode
           and in either Decimal or Hexadecimal.
  Input:
    void *  pevt         pointer to the Midas event header pmbk (name,type,data_size)
                      or pointer to the YBOS event header (LRL).
    INT     data_fmt     uses the YBOS or MIDAS event structure
    INT     dsp_mode     display in RAW or Bank mode
    INT     dsp_fmt      display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  if (data_fmt == FORMAT_MIDAS)
    {
      if (dsp_mode == DSP_RAW)
        display_raw_event((EVENT_HEADER *)pevt, dsp_fmt);
      if (dsp_mode == DSP_BANK)
        display_midas_event((EVENT_HEADER *)pevt, dsp_fmt);
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      if (dsp_mode == DSP_RAW)
        display_raw_event((DWORD *)pevt, dsp_fmt);
      if (dsp_mode == DSP_BANK)
        display_ybos_event(pevt, dsp_fmt);
    }
  else
    display_raw_event((EVENT_HEADER *)pevt, dsp_fmt);
  return;
}

/*------------------------------------------------------------------*/
void display_raw_event(void * pheader, INT dsp_fmt)
/********************************************************************\
  Routine: display_raw_event
  Purpose: display on screen the RAW data of an YBOS event.
  Input:
    void *  pevt           pointer to the LRL YBOS event
    INT     dsp_fmt        display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  DWORD *pevt, lrl, j, i, total=0;

  lrl = *((DWORD *)pheader);
  
  pevt = (DWORD *) (((DWORD *)pheader)+1);
  for (i=0;i<lrl; i+=NLINE)
    {
    printf ("%6.0d->: ",total);
      for (j=0;j<NLINE;j++)
     	{
	     if ((i+j) < lrl)
	      {
          if (dsp_fmt == DSP_DEC)
              printf ("%8.i ",*pevt);
          if (dsp_fmt == DSP_HEX)
              printf ("%8.8x ",*pevt);
	      pevt++;
	      }
	    }
      total += NLINE;
      printf ("\n");
    }
}

/*------------------------------------------------------------------*/
void display_midas_event(EVENT_HEADER * pheader, INT dsp_fmt)
/********************************************************************\
  Routine: display_midas_event
  Purpose: display on screen the MIDAS data in Midas bank mode.
  Input:
    EVENT_HEADER * pheader  pointer to Midas event 
    INT     dsp_fmt         display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  BANK_HEADER * pevt;
  BANK * pmbk;
  char * pdata;
  INT    size;

  pevt = (BANK_HEADER *) (pheader+1);
  pmbk = NULL;
  do
    {
      size = bk_iterate(pevt, &pmbk, &pdata);
      if (pmbk == NULL)
      break;
      midas_display_midas_bank(pmbk,dsp_fmt);
    }
  while (1);
  return;
}

/*------------------------------------------------------------------*/
void display_ybos_event( DWORD *pevt, INT dsp_fmt)
/********************************************************************\
  Routine: display_ybos_event
  Purpose: display on screen the YBOS data in YBOS bank mode.
  Input:
    DWORD * pevt           pointer to the event at lrl
    INT     dsp_fmt        display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  YBOS_BANK_HEADER   ybos;
  char bank_name[5], strbktype[32];
  DWORD *pnextb, *pendevt, lrl, length_type;
  INT i,j;

  lrl = *pevt;
  pevt++;
  pendevt = pevt + lrl;
  while (pevt < pendevt)
    {
      /* extract ybos bank header */
      memcpy (&ybos, (char *) pevt, sizeof(YBOS_BANK_HEADER));

      /* extract ybos bank name */
      memcpy (&bank_name[0],(char *) &ybos.name,4);
      bank_name[4]=0;

      /* pointer to first data */
      pevt += (sizeof(YBOS_BANK_HEADER) >> 2);

      /* pointer to next bank */
      pnextb = pevt + ybos.length - 1;

      j=8;         /* elements within line */
      i=1;         /* data counter */

      if (ybos.type == F4_BKTYPE)
	{
	  length_type = ybos.length-1;
	  strcpy (strbktype,"Real*4 (FMT machine dependent)");
	}
      if (ybos.type == I4_BKTYPE)
	{
	  length_type = ybos.length-1;
	  strcpy (strbktype,"Integer*4");
	}
      if (ybos.type == I2_BKTYPE)
	{
	  length_type = (ybos.length-1 << 1);
	  strcpy (strbktype,"Integer*2");
	}
      if (ybos.type == I1_BKTYPE)
	{
	  length_type = (ybos.length-1 << 2);
	  strcpy (strbktype,"8 bit Bytes");
	}
      if (ybos.type == A1_BKTYPE)
	{
	  length_type = (ybos.length-1 << 2);
	  strcpy (strbktype,"8 bit ASCII");
	}
      printf("\nBank:%s Length: %i(I*1)/%i(I*4)/%i(Type) Type:%s",
	     bank_name,(ybos.length << 2), ybos.length, length_type, strbktype);
      j = 16;

      while ((BYTE *) pevt < (BYTE *) pnextb)
	{
	  switch (ybos.type)
	    {
	    case F4_BKTYPE :
	      if (j>7)
		{
		  printf("\n%4i-> ",i);
		  j = 0;
		  i += 8;
		}
	      if (dsp_fmt == DSP_DEC)
		printf("%8.3e ",*((float *)pevt));
	      if (dsp_fmt == DSP_HEX)
		printf("0x%8.8x ",*((DWORD *)pevt));

	      pevt++;
	      j++;
	      break;
	    case I4_BKTYPE :
	      if (j>7)
		{
		  printf("\n%4i-> ",i);
		  j = 0;
		  i += 8;
		}
	      if (dsp_fmt == DSP_DEC)
		printf("%8.1i ",*((DWORD *)pevt));
	      if (dsp_fmt == DSP_HEX)
		printf("0x%8.8x ",*((DWORD *)pevt));

	      pevt++;
	      j++;
	      break;
	    case I2_BKTYPE :
	      if (j>7)
		    {
		      printf("\n%4i-> ",i);
		      j = 0;
		      i += 8;
		    }
	      if (dsp_fmt == DSP_DEC)
		    printf("%5.1i ",*((WORD *)pevt));
	      if (dsp_fmt == DSP_HEX)
		    printf("0x%4.4x ",*((WORD *)pevt));
	      ((WORD *)pevt)++;
	      j++;
	      break;
	    case A1_BKTYPE :
	      if (j>15)
		    {
		      printf("\n%4i-> ",i);
		      j = 0;
		      i += 16;
		    }
	      if (dsp_fmt == DSP_DEC)
		printf("%2.i ",*((BYTE *)pevt));
	      if (dsp_fmt == DSP_ASC)
		printf("%1.1s ",(char *)pevt);
	      if (dsp_fmt == DSP_HEX)
		printf("0x%2.2x ",*((BYTE *)pevt));

	      ((BYTE *)pevt)++;
	      j++;
	      break;
	    case I1_BKTYPE :
	      if (j>7)
		{
		  printf("\n%4i-> ",i);
		  j = 0;
		  i += 8;
		}
	      if (dsp_fmt == DSP_DEC)
		printf("%4.i ",*((BYTE *)pevt));
	      if (dsp_fmt == DSP_HEX)
		printf("0x%2.2x ",*((BYTE *)pevt));

	      ((BYTE *)pevt)++;
	      j++;
	      break;
	    }
	} /* next bank */
      printf ("\n");
    }
  return ;
}

/*------------------------------------------------------------------*/
void  display_any_bank(void * pbh, INT data_fmt, INT dsp_mode, INT dsp_fmt)
/********************************************************************\
  Routine: display_any_bank
  Purpose: display on screen the requested bank in either RAW or YBOS mode
           and in either Decimal, Hexadecimal or ASCII format.
  Input:
    BANK_HEADER * pbk            pointer to the event
    INT     data_fmt       display Midas or YBOS format
    INT     dsp_mode       display in RAW or YBOS mode
    INT     dsp_fmt        display format (DSP_DEC/HEX/ASCII)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  if (data_fmt == FORMAT_MIDAS)
    {
      if (dsp_mode == DSP_RAW)
        midas_display_raw_bank(pbh, dsp_fmt);
      if (dsp_mode == DSP_BANK)
        midas_display_midas_bank(pbh, dsp_fmt);
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      if (dsp_mode == DSP_RAW)
        ybos_display_raw_bank(pbh, dsp_fmt);
      if (dsp_mode == DSP_BANK)
        ybos_display_ybos_bank(pbh, dsp_fmt);
    }
  else
    printf("Unknown data format \n");
  return;
}

/*------------------------------------------------------------------*/
void ybos_display_raw_bank(DWORD * pbk, INT dsp_fmt)
/********************************************************************\
  Routine: ybos_display_raw_bank
  Purpose: display on screen the RAW data of a given YBOS bank.
  Input:
    DWORD * pbk           pointer to the bank name
    INT     dsp_fmt        display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  YBOS_BANK_HEADER *pbkh;
  DWORD *pdata, j, i;

  /* YBOS header */
  pbkh = (YBOS_BANK_HEADER *)pbk;

  /* skip YBOS header */
  pdata = (DWORD *) (pbkh+1);

  for (i=0;i<pbkh->length;i +=NLINE)
  {
    j = 0;
    printf("\n%4i-> ",i+j+1);
    for (j=0;j<NLINE;j++)
    {
      if ((i+j) < pbkh->length)
      {
        switch (dsp_fmt)
        {
        case DSP_DEC:
          printf ("%8.i ",*pdata);
          break;
        case DSP_ASC:
        case DSP_HEX:
          printf ("%8.8x ",*pdata);
          break;
        }
        pdata++;
      }
    }
  }
}

/*------------------------------------------------------------------*/
void midas_display_raw_bank(void * pbh, INT dsp_fmt)
/********************************************************************\
  Routine: midas_display_raw_bank
  Purpose: display on screen the RAW data of a given MIDAS bank.
  Input:
    BANK *  pbk            pointer to the bank name
    INT     dsp_fmt        display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  DWORD *pdata, size, lrl, j, i, type;
  BANK  *pmbk;

    /* scan bank for data */
  pmbk = NULL;
  do
    {
      size = bk_iterate(pbh, &pmbk, &pdata);
      if (pmbk == NULL)
        break;
      lrl= ((BANK *)pmbk)->data_size;   /* in bytes */
      lrl >>= 2;
      type = ((BANK *)pmbk)->type;
      (BANK *)pmbk += 1;

      /* skip MIDAS header */
      pdata = (DWORD *)pmbk;

      for (i=0;i<lrl;i +=NLINE)
        {
            j = 0;
            printf("\n%4i-> ",i+j+1);
            for (j=0;j<NLINE;j++)
     	      {
	             if ((i+j) < lrl)
	             { 
                  switch (dsp_fmt)
		              {
		                case DSP_DEC:
		                  printf ("%8.i ",*((DWORD *)pdata));
		                  break;
		                case DSP_ASC:
		                case DSP_HEX:
		                  printf ("%8.8x ",*((DWORD *)pdata));
		                  break;
    		          }
	                ((DWORD *)pdata)++;
               }
   	        } 
        }
    }
  while (1);
}

/*------------------------------------------------------------------*/
void ybos_display_ybos_bank( DWORD *pbk, INT dsp_fmt)
/********************************************************************\
  Routine: ybos_display_ybos_bank
  Purpose: display on screen the pointed YBOS bank data in YBOS bank mode.
  Input:
    DWORD * pbk            pointer to the bank.name
    INT     dsp_fmt        display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  YBOS_BANK_HEADER   *pbkh;

  char bank_name[5], strbktype[32];
  DWORD *pdata, *pendbk, length_type;
  INT i,j;

  pbkh = (YBOS_BANK_HEADER *)pbk;
  pdata = (DWORD *) (pbkh+1);
  memcpy (&bank_name[0],(char *) &pbkh->name,4);
  bank_name[4]=0;

  j=64;         /* elements within line */
  i=1;         /* data counter */

  if (pbkh->type == F4_BKTYPE)
    {
      length_type = pbkh->length-1;
      strcpy (strbktype,"Real*4 (FMT machine dependent)");
    }
  if (pbkh->type == I4_BKTYPE)
    {
      length_type = pbkh->length-1;
      strcpy (strbktype,"Integer*4");
    }
  if (pbkh->type == I2_BKTYPE)
    {
      length_type = (pbkh->length-1 << 1);
      strcpy (strbktype,"Integer*2");
    }
  if (pbkh->type == I1_BKTYPE)
    {
      length_type = (pbkh->length-1 << 2);
      strcpy (strbktype,"8 bit Bytes");
    }
  if (pbkh->type == A1_BKTYPE)
    {
      length_type = (pbkh->length-1 << 2);
      strcpy (strbktype,"8 bit ASCII");
    }
    printf("\nBank:%s Length: %i(I*1)/%i(I*4)/%i(Type) Type:%s",
	   bank_name,(pbkh->length << 2), pbkh->length, length_type, strbktype);
    j = 16;
    
  pendbk = pdata + pbkh->length - 1;
  while ((BYTE *) pdata < (BYTE *) pendbk)
    {
      switch (pbkh->type)
	    {
	    case F4_BKTYPE :
	      if (j>7)
	        {
	          printf("\n%4i-> ",i);
	          j = 0;
	          i += 8;
	        }
	      switch (dsp_fmt)
	        {
	        case DSP_DEC:
	          printf("%8.3e ",*((float *)pdata));
	          break;
	        case DSP_ASC:
	        case DSP_HEX:
	          printf("0x%8.8x ",*((DWORD *)pdata));
	          break;
	        }
	      pdata++;
	      j++;
	      break;
	    case I4_BKTYPE :
	      if (j>7)
	        {
	          printf("\n%4i-> ",i);
	          j = 0;
	          i += 8;
	        }
	      switch (dsp_fmt)
	        {
	        case DSP_DEC:
	          printf("%8.1i ",*((DWORD *)pdata));
	          break;
	        case DSP_ASC:
	        case DSP_HEX:
	          printf("0x%8.8x ",*((DWORD *)pdata));
	          break;
	        }
	      pdata++;
	      j++;
	      break;
	    case I2_BKTYPE :
	      if (j>7)
	        {
	          printf("\n%4i-> ",i);
	          j = 0;
	          i += 8;
	        }
	      switch (dsp_fmt)
	        {
	        case DSP_DEC:
	          printf("%5.1i ",*((WORD *)pdata));
	          break;
	        case DSP_ASC:
	        case DSP_HEX:
	          printf("0x%4.4x ",*((WORD *)pdata));
	          break;
	        }
	      ((WORD *)pdata)++;
	      j++;
	      break;
	    case A1_BKTYPE :
	      if (j>15)
	        {
	          printf("\n%4i-> ",i);
	          j = 0;
	          i += 16;
	        }
	      switch (dsp_fmt)
	        {
	        case DSP_DEC:
	          printf("%3.i ",*((BYTE *)pdata));
	          break;
	        case DSP_ASC:
	          printf("%1.1s ",(char *)pdata);
	          break;
	        case DSP_HEX:
	          printf("0x%2.2x ",*((BYTE *)pdata));
	          break;
	        }
	      ((char *)pdata)++;
	      j++;
	      break;
	    case I1_BKTYPE :
	      if (j>15)
	        {
	          printf("\n%4i-> ",i);
	          j = 0;
	          i += 16;
	        }
	      switch (dsp_fmt)
	        {
	        case DSP_DEC:
	          printf("%3.i ",*((BYTE *)pdata));
	          break;
	        case DSP_ASC:
	        case DSP_HEX:
	          printf("0x%2.2x ",*((BYTE *)pdata));
	          break;
	        }
	      ((BYTE *)pdata)++;
	      j++;
	      break;
	    }
    } /* end of bank */
  printf ("\n");
  return ;
}

/*------------------------------------------------------------------*/
void midas_display_midas_bank( void *pbk, INT dsp_fmt)
/********************************************************************\
  Routine: midas_display_midas_bank
  Purpose: display on screen the pointed MIDAS bank data using MIDAS Bank structure.
  Input:
    BANK_HEADER *  pbk            pointer to the BANK
    INT     dsp_fmt        display format (DSP_DEC/HEX)
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  char  bank_name[5], strbktype[32];
  char  *pdata, *pendbk;
  DWORD length_type, lrl;
  INT type, i, j;

  lrl= ((BANK *)pbk)->data_size;   /* in bytes */
  type = ((BANK *)pbk)->type;
  bank_name[4] = 0;
  memcpy (bank_name,(char *)(((BANK *)pbk)->name),4);
  (BANK *)pbk += 1;
  pdata = (char *) pbk;

  j=64;         /* elements within line */
  i=1;         /* data counter */

  if (type == TID_FLOAT)
    {
      length_type = sizeof (float);
      strcpy (strbktype,"Real*4 (FMT machine dependent)");
    }
  if (type == TID_DWORD)
    {
      length_type = sizeof (DWORD);
      strcpy (strbktype,"Integer*4");
    }
  if (type == TID_WORD)
    {
      length_type = sizeof (WORD);
      strcpy (strbktype,"Integer*2");
    }
  if (type == TID_BYTE)
    {
      length_type = sizeof (BYTE);
      strcpy (strbktype,"8 bit Bytes");
    }
  if (type == TID_CHAR)
    {
      length_type = sizeof(char);
      strcpy (strbktype,"8 bit ASCII");
    }
  printf("\nBank:%s Length: %i(I*1)/%i(I*4)/%i(Type) Type:%s",
	 bank_name,lrl, lrl>>2, lrl, strbktype);

  pendbk = pdata + lrl;
  while (pdata < pendbk)
    {
      switch (type)
	        {
	        case TID_FLOAT :
	          if (j>7)
	            {
	              printf("\n%4i-> ",i);
	              j = 0;
	              i += 8;
	            }
	          switch (dsp_fmt)
	            {
	            case DSP_DEC:
	              printf("%8.3e ",*((float *)pdata));
	              break;
	            case DSP_ASC:
	            case DSP_HEX:
	              printf("0x%8.8x ",*((DWORD *)pdata));
	              break;
	            }
	          ((DWORD *)pdata)++;
	          j++;
	          break;
	        case TID_DWORD :
	          if (j>7)
	            {
	              printf("\n%4i-> ",i);
	              j = 0;
	              i += 8;
	            }
	          switch (dsp_fmt)
	            {
	            case DSP_DEC:
	              printf("%8.1i ",*((DWORD *)pdata));
	              break;
	            case DSP_ASC:
	            case DSP_HEX:
	              printf("0x%8.8x ",*((DWORD *)pdata));
	              break;
	            }
	          ((DWORD *)pdata)++;
	          j++;
	          break;
	        case TID_WORD :
	          if (j>7)
	            {
	              printf("\n%4i-> ",i);
	              j = 0;
	              i += 8;
	            }
	          switch (dsp_fmt)
	            {
	            case DSP_DEC:
	              printf("%5.1i ",*((WORD *)pdata));
	              break;
	            case DSP_ASC:
	            case DSP_HEX:
	              printf("0x%4.4x ",*((WORD *)pdata));
	              break;
	            }
	          ((WORD *)pdata)++;
	          j++;
	          break;
	        case TID_BYTE :
	          if (j>15)
	            {
	              printf("\n%4i-> ",i);
	              j = 0;
	              i += 16;
	            }
	          switch (dsp_fmt)
	            {
	            case DSP_DEC:
	              printf("%3.i ",*((BYTE *)pdata));
	              break;
	            case DSP_ASC:
	              printf("%1.1s ",(char *)pdata);
	              break;
	            case DSP_HEX:
	              printf("0x%2.2x ",*((BYTE *)pdata));
	              break;
	            }
	          pdata++;
	          j++;
	          break;
	        case TID_CHAR :
	          if (j>15)
	            {
	              printf("\n%4i-> ",i);
	              j = 0;
	              i += 16;
	            }
	          switch (dsp_fmt)
	            {
	            case DSP_DEC:
	              printf("%3.i ",*((BYTE *)pdata));
	              break;
	            case DSP_ASC:
	            case DSP_HEX:
	              printf("0x%2.2x ",*((BYTE *)pdata));
	              break;
	            }
        	  pdata++;
	          j++;
	          break;
	        }
    } /* end of bank */
  printf ("\n");
  return;
}

/*------------------------------------------------------------------*/
INT ybos_extract_info (INT fmt, INT element)
/********************************************************************\
  Routine: ybos_extract_info
  Purpose: extract the physical record header info.
  Input:
    INT   fmt                 Record type
    INT   element             element descriptor
  Output:
    none
  Function value:             element value
\********************************************************************/
{
  switch (rdlog.fmt)
    {
    case YBOS_FMT:
      switch (element)
	    {
	    case H_BLOCK_SIZE:
	      return( (YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->block_size;
	    case H_HEAD_LEN:
	      return( (YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->header_length;
	    case H_BLOCK_NUM:
	      return( (YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->block_num;
	    case H_START:
	      return( (YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->offset;
	    }
      break;
    }
  return -1;
}

/*------------------------------------------------------------------*/
void ybos_display_all_info ( INT what)
/********************************************************************\
  Routine: ybos_display_all_info
  Purpose: display on screen all the info about a YBOS record.
  Input:
    INT     what              type of display.
  Output:
    none
  Function value:
    INT extracted value
\********************************************************************/
{
  DWORD bz,hyl,ybn,of;
  switch (rdlog.fmt)
    {
    case YBOS_FMT:
      bz  = ybos_extract_info (rdlog.fmt,H_BLOCK_SIZE);
      hyl = ybos_extract_info (rdlog.fmt,H_HEAD_LEN);
      ybn = ybos_extract_info (rdlog.fmt,H_BLOCK_NUM);
      of  = ybos_extract_info (rdlog.fmt,H_START);
      switch (what)
	{
	case D_RECORD:
	  printf ("rec#%d- ",rdlog.cur_block);
	  printf ("%5dbz %5dhyl %5dybn %5dof\n",bz,hyl,ybn,of);
	  break;
	case D_HEADER:
	  printf ("rec#%d- ",rdlog.cur_block);
	  printf ("%5dbz %5dhyl %5dybn %5dof\n",bz,hyl,ybn,of);
	  break;
	case D_EVTLEN:
	  printf ("rec#%d- ",rdlog.cur_block);
	  printf ("%5dbz %5dhyl %5dybn %5dof ",bz,hyl,ybn,of);
	  printf ("%5del/x%x %5dev\n",rdlog.cur_evt_len/4,rdlog.cur_evt_len/4,rdlog.cur_evt);
	  break;
	case D_EVENT:
	  printf ("rec#%d- ",rdlog.cur_block);
	  printf ("%5dbz %5dhyl %5dybn %5dof\n",bz,hyl,ybn,of);
	  break;
	}
      break;
    }
  return;
}

/*------------------------------------------------------------------*/
INT   open_any_datafile (INT data_fmt, char * datafile)
/********************************************************************\
  Routine: open_any_datafile
  Purpose: Open data file for replay for the given data format
  Input:
    INT data_fmt :  YBOS or MIDAS 
    char * datafile : Data file name
  Output:
    none
  Function value:
    status : from lower function
\********************************************************************/
{
  INT status;
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      status = ybos_open_datafile(datafile);
    }
  return(status);
}

/*------------------------------------------------------------------*/
INT   ybos_open_datafile (char * filename)
/********************************************************************\
  Routine: ybos_open_datafile
  Purpose: open a YBOS data file for replay
  Input:
    char *  filename       File to open.
  Output:
    none
  Function value:
    RDLOG_FAIL_OPEN        Failed to open file
    RDLOG_FAIL_READING     Faile to read
    RDLOG_BLOCK_TOO_BIG    Block size read too big for expected
    RDLOG_UNKNOWN_FMT      Unknown data format on file
    RDLOG_SUCCESS          File opened
\********************************************************************/
{
  /* fill up record with file name */
  strcpy (rdlog.name,filename);

  /* find out what dev it is ? : check on /dev */
  zipfile = FALSE;
  if (strncmp(rdlog.name,"/dev",4) == 0)
    {
      /* tape device */
      rdlog.dev_type = DLOG_TAPE;
    }
  else
    {
      /* disk device */
      rdlog.dev_type = DLOG_DISK;
      if (strncmp(filename+strlen(filename)-3,".gz",3) == 0)
        zipfile = TRUE;
    }

  /* open file */
  if(!zipfile)
  {
    if ((rdlog.handle = open(rdlog.name, O_RDONLY | O_BINARY, 0644 )) == -1 )
    {
      printf("dev name :%s Handle:%d \n",rdlog.name, rdlog.handle);
      return(SS_FILE_ERROR);
    }
  }
  else 
  {
#ifdef INCLUDE_ZLIB
    filegz = gzopen(rdlog.name, "rb");
    rdlog.handle=0;
    if (filegz == NULL)
    {
      printf("dev name :%s gzopen error:%d \n",rdlog.name, rdlog.handle);
      return(SS_FILE_ERROR);
    }
#else
    cm_msg(MERROR,"ybos.c","Zlib not included ... gz file not supported");
    return (SS_FILE_ERROR);
#endif
  }
  rdlog.fmt           = YBOS_FMT;
  rdlog.block_size    = 4 * 8190;
  rdlog.header_ptr    = (DWORD *) malloc( sizeof( YBOS_PHYSREC_HEADER));
  ((YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->block_size    = 8190-1;
  ((YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->header_length = 4;
  ((YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->block_num     = 0;
  ((YBOS_PHYSREC_HEADER *) rdlog.header_ptr)->offset        = 0;

  /* allocate memory for physrec  rdlog structure */
  rdlog.begrec_ptr  = (char *) malloc (rdlog.block_size);
  memset ((char *) rdlog.begrec_ptr, -1, rdlog.block_size);

  /* reset first path */
  magta_flag = 0;

  /* book space for event */
  prevent   = malloc(MAX_EVT_SIZE);
  *prevent  = (DWORD) -1;

  /* initialize pertinent variables */
  rdlog.cur_block  = (DWORD) -1;
  rdlog.cur_evt    = 0;
  rdlog.rd_cur_ptr = rdlog.begrec_ptr;

  /* the device pointer is left at the top of the record!
     BUT YET NO RECORD IS READ */

  return ( RDLOG_SUCCESS);
}

/*------------------------------------------------------------------*/
INT   close_any_datafile (INT data_fmt)
/********************************************************************\
  Routine: close_any_datafile
  Purpose: Close data file for the given data format
  Input:
  INT data_fmt :  YBOS or MIDAS 
  Output:
    none
  Function value:
  status : from lower function
 /*******************************************************************/
{
  INT status;
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      status = ybos_close_datafile();
    }
  return(status);
}

/*------------------------------------------------------------------*/
INT   ybos_close_datafile (void)
/********************************************************************\
  Routine: ybos_close_datafile
  Purpose: Close YBOS data file for read.
  Input:
    none
  Output:
    none
  Function value:
  status : from lower function
\********************************************************************/
{
  switch (rdlog.dev_type)
    {
    case DLOG_TAPE:
    case DLOG_DISK:
      /* close file */
/*
      if (zipfile)
#ifdef INCLUDE_ZLIB
        gzclose(rdlog.name);
#endif      
*/
      if (rdlog.handle != 0)
        close (rdlog.handle);
      break;
    }
  /* free allocated memory, remove entry from list */
/*
  if ((DWORD *)rdlog.header_ptr != NULL)
    free (rdlog.header_ptr);
  if (rdlog.begrec_ptr != NULL)
    free(rdlog.begrec_ptr);
  if (prevent != NULL)
    free(prevent);
    */
  return(RDLOG_SUCCESS);
}

/*------------------------------------------------------------------*/
INT   skip_any_physrec (INT data_fmt, INT bl)
/********************************************************************\
  Routine: skip_any_physrec
  Purpose: Skip physical record until block = bl for the given data
           format
  Input:
    INT data_fmt :  YBOS or MIDAS 
    INT bl:         block number (-1==all, 0 = first block)
  Output:
    none
  Function value:
    status : from lower function
\********************************************************************/
{
  INT status;
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      status = ybos_skip_physrec(bl);
    }
  return(status);
}

/*------------------------------------------------------------------*/

INT   ybos_skip_physrec (INT bl)
/********************************************************************\
  Routine: ybos_skip_physrec
  Purpose: skip physical record on a YBOS file.
  Input:
    INT     bl            physical record number. (start at 0)
                          if bl = -1 : skip skiping
  Output:
    none
  Function value:
    RDLOG_SUCCESS        Ok
\********************************************************************/
{
  if (bl == -1)
  {
    if(ybos_physrec_get() == RDLOG_SUCCESS)
    return RDLOG_SUCCESS;
  }
  while (ybos_physrec_get() == RDLOG_SUCCESS)
  {
    if(ybos_extract_info( rdlog.fmt, H_BLOCK_NUM) != bl)
      {
	      printf("Skipping record_# ... ");
	      printf("%d \r",ybos_extract_info( rdlog.fmt,H_BLOCK_NUM));
	      fflush (stdout);
	    }
     else
     {
	      printf ("\n");
        return RDLOG_SUCCESS;
     }
  }
  return RDLOG_DONE;
}

/*------------------------------------------------------------------*/
INT   get_any_physrec (INT data_fmt)
/********************************************************************\
  Routine: get_any_physrec
  Purpose: Retrieve a physical record for the given data format
  Input:
  INT data_fmt :  YBOS or MIDAS 
  Output:
    none
  Function value:
    status : from lower function
\********************************************************************/
{
  INT status;
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      status = ybos_physrec_get();
    }
  return(status);
}

/*------------------------------------------------------------------*/
INT   ybos_physrec_get (void)
/********************************************************************\
  Routine: ybos_physrec_get
  Purpose: read one physical YBOS record.
  Input:
    none
  Output:
    none
  Function value:
    RDLOG_DONE            No more record to read
    RDLOG_SUCCESS         Ok
\********************************************************************/
{
  INT physrec_size;

  /* extract physical record length from previous record */
  physrec_size = ybos_extract_info(rdlog.fmt,H_BLOCK_SIZE) + 1;

  if (magta_flag == 1)
  {
      if (!zipfile)
      {
        /* skip 4 bytes from MAGTA header */
        if (read(rdlog.handle, (char *)rdlog.begrec_ptr, 4) <= 0)
	        return (RDLOG_DONE);
      }
      else
      {
#ifdef INCLUDE_ZLIB
        status = gzread(filegz, (char *)rdlog.begrec_ptr,4);
        if (status <= 0)
          return (RDLOG_DONE);
#endif
      }
  }
  /* read full YBOS physical record */
  if (!zipfile)
  {
    if (read(rdlog.handle, (char *)rdlog.begrec_ptr, (physrec_size<<2)) <= 0)
    return (RDLOG_DONE);
  }
  else
  {
#ifdef INCLUDE_ZLIB
    status = gzread(filegz, (char *)rdlog.begrec_ptr,(physrec_size<<2));
    if (status <= 0)
      return (RDLOG_DONE);
#endif
  }

  /* check if header make sense
     for MAGTA there is extra stuff to get rid off */
  if ((magta_flag == 0) && (*((DWORD *)rdlog.begrec_ptr) == 0x00000004))
  {
      /* set MAGTA flag */
      magta_flag  = 1;
      /* BOT of MAGTA skip record */
      if (!zipfile)
      {
        if (read(rdlog.handle, (char *)rdlog.begrec_ptr, 8 ) <= 0)
	         return (RDLOG_DONE);
      }
      else
      {
#ifdef INCLUDE_ZLIB
      status = gzread(filegz, (char *)rdlog.begrec_ptr, 8);
        if (status <= 0)
          return (RDLOG_DONE);
#endif
      }

      /* read full YBOS physical record */
      if (!zipfile)
      {
        if (read(rdlog.handle, (char *)rdlog.begrec_ptr, (physrec_size<<2)) <= 0)
	        return (RDLOG_DONE);
      }
      else
      {
#ifdef INCLUDE_ZLIB
        status = gzread(filegz, (char *)rdlog.begrec_ptr, (physrec_size<<2));
        if (status <= 0)
          return (RDLOG_DONE);
#endif
      }
  }
  /* copy for lazy */
  plazy = rdlog.begrec_ptr;
  szlazy = physrec_size<<2;
  /* local save of the header */
  memcpy (rdlog.header_ptr, rdlog.begrec_ptr, sizeof(YBOS_PHYSREC_HEADER));

  /* set rd_cur to next valid data in rec */
  rdlog.rd_cur_ptr = rdlog.begrec_ptr + sizeof(YBOS_PHYSREC_HEADER);

  /* count blocks */
  rdlog.cur_block++;

  /* rd_cur left after the YBOS header in any case. */
  return(RDLOG_SUCCESS);
}

/*------------------------------------------------------------------*/
void display_any_physhead (INT data_fmt, INT dsp_fmt)
/********************************************************************\
  Routine: display_any_physhead
  Purpose: Display the physical header info for the current record 
           for the given data format
  Input:
    INT data_fmt :  YBOS or MIDAS 
    INT dsp_fmt:    not used for now
  Output:
    none
  Function value:
    none 
\********************************************************************/
{
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      ybos_display_all_info (D_HEADER);
    }
  return;
}

/*------------------------------------------------------------------*/
void display_any_physrec (INT data_fmt, INT dsp_fmt)
/********************************************************************\
  Routine: display_any_physrec
  Purpose: Display the physical record of the current record 
           for the given data format
  Input:
    INT data_fmt :  YBOS or MIDAS 
    INT dsp_fmt:    not used for now
  Output:
    none
  Function value:
    none 
\********************************************************************/
{
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      ybos_physrec_display();
    }
  return;
}

/*------------------------------------------------------------------*/
INT   ybos_physrec_display (void)
/********************************************************************\
  Routine: ybos_physrec_display
  Purpose: read one physical YBOS record.
  Input:
    none
  Output:
    none
  Function value:
    RDLOG_DONE            No more record to read
    RDLOG_SUCCESS         Ok
\********************************************************************/
{
  INT bz, j, i, k;
  DWORD *rec;

  ybos_display_all_info (D_RECORD);
  bz  = ybos_extract_info (rdlog.fmt,H_BLOCK_SIZE)+1;
  /* adjust local pointer to top of record to include record header */
  rec =(DWORD *) (rdlog.begrec_ptr);
  k = ybos_extract_info( rdlog.fmt, H_BLOCK_NUM);
  for (i=0;i<bz; i+=NLINE)
    {
      printf ("R(%d)[%d] = ",k,i);
      for (j=0;j<NLINE;j++)
      {
        if (i+j < bz)
        {
          printf ("%8.8x ",*rec);
          rec++;
        }
      }
      printf ("\n");
    }
    return(RDLOG_SUCCESS);
}

/*------------------------------------------------------------------*/
void display_any_evtlen (INT data_fmt, INT dsp_fmt)
/********************************************************************\
  Routine: display_any_evtlen
  Purpose: Display the event length for within the current record 
           for the given data format
  Input:
    INT data_fmt :  YBOS or MIDAS 
    INT dsp_fmt:    not used for now
  Output:
    none
  Function value:
    none 
/********************************************************************/
{
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      ybos_display_all_info (D_EVTLEN);
    }
  return;
}

/*------------------------------------------------------------------*/
INT   get_any_event (INT data_fmt, INT bl, void * prevent)
/********************************************************************\
  Routine: get_any_event
  Purpose: Retreive an event from the given data format.
  Input:
    INT data_fmt :  YBOS or MIDAS 
    INT dsp_fmt:    not used for now
  Output:
    none
  Function value:
    status : from lower function
\********************************************************************/
{
  INT status;
  if (data_fmt == FORMAT_MIDAS)
    {
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      status=ybos_event_get (bl, (DWORD *)prevent);
    }
  return(status);
}

/*------------------------------------------------------------------*/
INT   ybos_event_get (INT bl, DWORD * prevent)
/********************************************************************\
  Routine: ybos_event_get
  Purpose: read one YBOS event.
  Input:
    INT    bl            physical record number
                         != -1 stop at the end of the current physical record
  Output:
    DWORD * prevent      points to LRL valid full YBOS event
  Function value:
    RDLOG_DONE           No more record to read
    RDLOG_SUCCESS        Ok
\********************************************************************/
{
  INT     ret, fpart, lpart;
  char   *tmp_ptr;
  DWORD   evt_length;

  if (*prevent == -1)
    {
      /* event stuff never been set */
      /* read one physrec (rd_cur left after YBOS header) */
      /* 	if ((ret=ybos_physrec_get()) != RDLOG_SUCCESS) */
      /*     return(ret); */
      /* move rc_cur to next valid event in physrec */
      rdlog.rd_cur_ptr = rdlog.begrec_ptr + (ybos_extract_info(rdlog.fmt,H_START) << 2);
    }

  /* extract lrl */
  evt_length = *((DWORD *) rdlog.rd_cur_ptr) + 1;

  /* stop if LRL  = -1 */
  if (evt_length-1 == -1)
    return (RDLOG_DONE);

  /* inclusive length convert it in bytes */
  evt_length *= sizeof(DWORD);

  /* check if event cross block boundary */
  if ( (rdlog.rd_cur_ptr + evt_length) >=
       (rdlog.begrec_ptr + rdlog.block_size)    )
    {
      /* check if request of event should be terminate due to bl != -1 */
      if (bl != -1)
	return (RDLOG_DONE);
      /* upcomming event crosses block, then first copy first part of event */
      /* compute max copy for first part of event */
      fpart = rdlog.begrec_ptr + rdlog.block_size - rdlog.rd_cur_ptr;
      lpart = evt_length - fpart;

      memcpy ((char *)prevent, rdlog.rd_cur_ptr, fpart);

      /* adjust temporary evt pointer */
      tmp_ptr = (char *) prevent + fpart;

      /* get next physical record */
      if ((ret=ybos_physrec_get ()) != RDLOG_SUCCESS)
	return (ret);

      /* now copy remaining from temporary pointer */
      memcpy (tmp_ptr, rdlog.rd_cur_ptr, lpart);

      /* adjust pointer to next valid data (LRL) */
      rdlog.rd_cur_ptr += lpart;
    }
  else
    {
      /* up comming event fully in block, copy event */
      memcpy ((char *)prevent, rdlog.rd_cur_ptr, evt_length);

      /* adjust pointer to next valid data (LRL) */
      rdlog.rd_cur_ptr += evt_length;
    }
  /* count event */
   rdlog.cur_evt++;

  /* set current event length in bytes */
   rdlog.cur_evt_len = evt_length-4;

   return(RDLOG_SUCCESS);
}

/*------------------------------------------------------------------*/
INT   ybos_file_compose(char * pevt, char * svpath, INT file_mode)
/********************************************************************\
  Routine: ybos_file_compose
  Purpose: Receive event which are expected to be file oriented with
           YBOS_xFILE header.
  Input:
    char * pevt           pointer to a YBOS event (->LRL).
    char * svpath         path where to save file
    INT    file_mode      NO_RUN : save file under original name
                          ADD_RUN: cat run number at end of file name
  Output:
    none
  Function value:
    RDLOG_SUCCESS         OP successfull
    RDLOG_INCOMPLETE      file compose channels still open
    RDLOG_COMPLETE        All file compose channels closed or complete
    status                 -x error of inner call
\********************************************************************/
{
  YBOS_CFILE *pyfch;
  YBOS_BANK_HEADER *pbkh;
  DWORD *pbbkh;
  int slot, status;
  DWORD bklen, bktyp;

  if (file_mode == NO_RECOVER)
    return RDLOG_SUCCESS;

  if ((status = ybk_find ((DWORD *)pevt,"CFIL",&bklen,&bktyp , &pbbkh)) != YBOS_SUCCESS)
    return (status);
  pbkh = (YBOS_BANK_HEADER *)pbbkh;
  pyfch = (YBOS_CFILE *)(pbkh+1);

  /*
  printf("%i - %i - %i - %i - %i -%i -%i \n"
	 ,pyfch->file_ID, pyfch->size
	 ,pyfch->fragment_size, pyfch->total_fragment
	 ,pyfch->current_fragment, pyfch->current_read_byte
	 , pyfch->run_number);
	 */

  /* check if file is in progress */
  for (slot=0;slot<MAX_YBOS_FILE;slot++)
    {
      if ((yfile[slot].fHandle != 0) && (pyfch->file_ID == yfile[slot].file_ID))
        {
	  /* Yep file in progress for that file_ID */
          if ((status = ybos_file_update(slot, pevt)) != RDLOG_SUCCESS)
            {
              printf("ybos_file_update() failed\n");
              return status;
            }
          goto check;
        }
      /* next slot */
    }
  /* current fragment not registered => new file */
  /* open file, get slot back */
  if((status = ybos_file_open(&slot, pevt, svpath, file_mode)) != RDLOG_SUCCESS)
    {
      printf("ybos_file_open() failed\n");
      return status;
    }
  /* update file */
  if ((status = ybos_file_update(slot,pevt)) != RDLOG_SUCCESS)
    {
      printf("ybos_file_update() failed\n");
      return status;
    }

check:
  /* for completion of recovery on ALL files */
  for (slot=0;slot<MAX_YBOS_FILE;slot++)
    {
      if (yfile[slot].fHandle != 0)
	{
	  /* Yes still some file composition in progress */
          return RDLOG_INCOMPLETE;
        }
      /* next slot */
    }
  return RDLOG_COMPLETE;
}

/*------------------------------------------------------------------*/
INT   ybos_file_open(int *slot, char *pevt, char * svpath, INT file_mode)
/********************************************************************\
  Routine: ybos_file_open
  Purpose: Prepare channel for receiving event of YBOS_FILE type.
  Input:
    char * pevt           pointer to the data portion of the event
    char * svpath         path where to save file
    INT    file_mode      NO_RUN : save file under original name
                          ADD_RUN: cat run number at end of file name
  Output:
    INT  * slot           index of the opened channel
  Function value:
    RDLOG_SUCCESS          Successful completion
    RDLOG_FAIL_OPEN        cannot create output file
    RDLOG_NOMORE_SLOT      no more slot for starting dump
\********************************************************************/
{
  YBOS_BANK_HEADER *pbkh;
  DWORD bklen, bktyp;
  YBOS_CFILE *pyfch;
  YBOS_PFILE *pyfph;
  DWORD *pbbkh;
  char * fpoint;
  char srun[16];
  int i, status;

  /* initialize invalid slot */
  *slot = -1;

  if ((status = ybk_find ((DWORD *)pevt,"CFIL",&bklen,&bktyp, &pbbkh)) != YBOS_SUCCESS)
    return (status);
  pbkh = (YBOS_BANK_HEADER *)pbbkh;
  pyfch = (YBOS_CFILE *)(pbkh+1);

  if ((status = ybk_find ((DWORD *)pevt,"PFIL",&bklen,&bktyp, &pbbkh)) != YBOS_SUCCESS)
    return (status);
  pbkh = (YBOS_BANK_HEADER *)pbbkh;
  pyfph = (YBOS_PFILE *)(pbkh+1);

  /* find free slot */
  for (i=0;i<MAX_YBOS_FILE; i++)
    if (yfile[i].fHandle == 0)
      break;
  if (i<MAX_YBOS_FILE)
    {
      /* copy necessary file header info */
      yfile[i].file_ID = pyfch->file_ID;
      strcpy(yfile[i].path,pyfph->path);

      /* extract file name */
      fpoint = pyfph->path;
      if (strrchr(pyfph->path,'/') > fpoint)
        fpoint = strrchr(pyfph->path,'/');
      if (strrchr(pyfph->path,'\\') > fpoint)
        fpoint = strrchr(pyfph->path,'\\');
      if (strrchr(pyfph->path,':') > fpoint)
        fpoint = strrchr(pyfph->path,':');

      /* add path name */
      if (svpath[0] != 0)
	    { 
	      yfile[i].path[0]=0;
	      strncat(yfile[i].path,svpath,strlen(svpath));
	      strncat(yfile[i].path,fpoint+1,strlen(fpoint)-1);
	      /*
	        printf("output file:%s-\n",yfile[i].path);
	        */
	    }
      if (file_mode == ADD_RUN)
	    {  /* append run number */
	      strcat (yfile[i].path,".");
	      sprintf(srun,"Run%4.4i",pyfch->run_number);
	      strncat (yfile[i].path,srun,strlen(srun));
	      /*
	      printf("output with run file:%s-\n",yfile[i].path);
	      */
	    }

      /* open device */
      if( (yfile[i].fHandle = open(yfile[i].path ,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644)) == -1 )
	    {
	      yfile[i].fHandle = 0;
	      printf("File %s cannot be created\n",yfile[i].path);
	      return (RDLOG_FAIL_OPEN);
	    }
    }
  else
    {
      /* no more slot */
      printf("No more slot for file %s\n",pyfph->path);
      return RDLOG_NOMORE_SLOT;
    }

  yfile[i].current_read_byte = 0;
  yfile[i].current_fragment = 0;
  *slot = i;
  return RDLOG_SUCCESS;
}

/*------------------------------------------------------------------*/
INT   ybos_file_update(int slot, char * pevt)
/********************************************************************\
  Routine: ybos_file_update
  Purpose: dump Midas/Ybos event to file for YBOS file oriented event type.
  Input:
    char * pevt           pointer to the data portion of the event.
  Output:
    INT  slot             valid index of the opened channel.
  Function value:
    RDLOG_SUCCESS         Successful completion
    -1                     error
\********************************************************************/
{
  DWORD *pbbkh;
  YBOS_BANK_HEADER *pbkh;
  DWORD bklen, bktyp;
  YBOS_CFILE *pyfch;
  char * pyfd;
  int status;
  int nwrite;

  if ((status = ybk_find ((DWORD *)pevt,"CFIL",&bklen,&bktyp, &pbbkh)) != YBOS_SUCCESS)
    return (status);
  pbkh = (YBOS_BANK_HEADER *)pbbkh;
  pyfch = (YBOS_CFILE *)(pbkh+1);

  if ((status = ybk_find ((DWORD *)pevt,"DFIL",&bklen,&bktyp, &pbbkh)) != YBOS_SUCCESS)
    return (status);
  pbkh = (YBOS_BANK_HEADER *)pbbkh;
  pyfd = (char *) (pbkh+1);

  /* check sequence order */
  if (yfile[slot].current_fragment + 1 != pyfch->current_fragment)
    {
      printf("Out of sequence %i / %i\n"
	     ,yfile[slot].current_fragment,pyfch->current_fragment);
    }
  /* dump fragment to file */
  nwrite = write (yfile[slot].fHandle, pyfd, pyfch->fragment_size);

  /* update current file record */
  yfile[slot].current_read_byte += nwrite;
  yfile[slot].current_fragment++;
  /* check if file has to be closed */
  if ( yfile[slot].current_fragment == pyfch->total_fragment)
    {
      /* file complete */
      close (yfile[slot].fHandle);
      printf("File %s (%i) completed\n",yfile[slot].path,yfile[slot].current_read_byte);
      /* cleanup slot */
      yfile[slot].fHandle = 0;
      return RDLOG_SUCCESS;
    } /* close file */
  else
    {
      /* fragment retrieved wait next one */
      return RDLOG_SUCCESS;
    }
}
#endif /* !FE_YBOS_SUPPORT */
/*------------------------------------------------------------------*/
INT   ybos_feodb_file_dump (EQUIPMENT * eqp, DWORD* pevent, INT run_number, char *path)
/********************************************************************\
  Routine: ybos_feodb_file_dump
  Purpose: Access ODB for the /Equipment/<equip_name>/Dump.
           in order to scan for file name and dump the files content to the
           Midas buffer channel.
  Input:
    EQUIPMENT * eqp        Current equipment
    INT    run_number      current run_number
    char * path            full file name specification
  Output:
    none
  Function value:
    0                      Successful completion
\********************************************************************/
{
  INT      index, size, status;
  HNDLE    hDB, hKey, hKeydump;
  char     strpath[MAX_FILE_PATH], Dumpfile[MAX_FILE_PATH];
  char     odb_entry[MAX_FILE_PATH];

  cm_get_experiment_database(&hDB, &hKey);

  /* loop over all channels */
  sprintf (odb_entry,"/Equipment/%s/Dump",path);
  status = db_find_key(hDB, 0, odb_entry, &hKey);
  if (status != DB_SUCCESS)
    {
      cm_msg(MINFO, "ybos_odb_file_dump", "odb_access_file -I- %s not found", odb_entry);
      return RDLOG_SUCCESS;
    }
  index = 0;
  while ((status = db_enum_key(hDB, hKey, index, &hKeydump))!= DB_NO_MORE_SUBKEYS)
    {
      if (status == DB_SUCCESS)
	{
	  size = sizeof(strpath);
	  db_get_path(hDB, hKeydump, strpath, size);
	  db_get_value(hDB, 0, strpath, Dumpfile, &size, TID_STRING);
	  ybos_fefile_dump(eqp, (EVENT_HEADER *)pevent, run_number, Dumpfile);
	}
      index++;
    }
  return (RDLOG_SUCCESS);
}

/*------------------------------------------------------------------*/
INT  ybos_fefile_dump(EQUIPMENT * eqp, EVENT_HEADER *pevent, INT run_number, char * path)
/********************************************************************\
  Routine: ybos_fefile_dump
  Purpose: Fragment file in order to send it through Midas.
           This version compose an event of the form of:
           Midas_header[(YBOS_CFILE)(YBOS_PFILE)(YBOS_DFILE)]
      	   Specific for the fe.
  Input:
    EQUIPMENT * eqp        Current equipment
    INT   run_number       currrent run_number
    char * path            full file name specification
  Output:
    none
  Function value:
    RDLOG_SUCCESS          Successful completion
    RDLOG_OPEN_FAILED      file doesn't exist
    -2                     file not closed
\********************************************************************/
  {
    INT dmpf, remaining;
    INT nread, filesize, nfrag;
    INT allheader_size;
    DWORD *pbuf, *pcfile, *pybos;
    YBOS_CFILE ybos_cfileh;
    YBOS_PFILE ybos_pfileh;
    int  send_sock, flag;

    /* check if file exists */
    /* Open for read (will fail if file does not exist) */
    if( (dmpf = open(path, O_RDONLY | O_BINARY, 0644 )) == -1 )
  	  {
	      cm_msg(MINFO,"ybos_file_dump","File dump -Failure- on open file %s",path);
	      return RDLOG_FAIL_OPEN;
	    }

    /* get file size */
    filestat = malloc( sizeof(struct stat) );
    stat(path,filestat);
    filesize = filestat->st_size;
    free(filestat);
    cm_msg(MINFO,"ybos_file_dump","Accessing File %s (%i)",path, filesize);

    /*-PAA-Oct06/97 added for ring buffer option */
    send_sock = rpc_get_send_sock();
  
    /* compute fragmentation & initialize*/
    nfrag = filesize / MAX_FRAG_SIZE;

    /* Generate a unique FILE ID */
    srand( (unsigned)time( NULL ) );
    srand( (unsigned)time( NULL ) );

    /* Fill file YBOS_CFILE header */
    ybos_cfileh.file_ID = rand();
    ybos_cfileh.size = filesize;
    ybos_cfileh.total_fragment = nfrag + (((filesize % MAX_FRAG_SIZE)==0) ? 0 : 1);
    ybos_cfileh.current_fragment = 0;
    ybos_cfileh.current_read_byte = 0;
    ybos_cfileh.run_number= run_number;
    ybos_cfileh.spare= 0xabcd;

    /* Fill file YBOS_PFILE header */
    memset (ybos_pfileh.path,0,sizeof(YBOS_PFILE));
    /* first remove path if present */
    if (strrchr(path,'/') != NULL)
      {
        strncpy(ybos_pfileh.path
	        ,strrchr(path,'/')+1
	        ,strlen(strrchr(path,'/')));
      }
    else
        strcpy(ybos_pfileh.path, path);

    /* allocate space */
    allheader_size = sizeof(EVENT_HEADER)
      + sizeof(YBOS_BANK_HEADER)           /* EVID bank header */
      + 5*sizeof(DWORD)                    /* EVID data size */
      + sizeof(YBOS_CFILE)
      + sizeof(YBOS_PFILE) + 64;

    flag = 0;
    pevent -= 1;

    /* read file */
    while (ybos_cfileh.current_fragment <= nfrag)
    {
      /* pevent passed by fe for first event only */
      if (flag)
        pevent = eb_get_pointer();
      flag = 1;

      /* YBOS bank header */
      pybos = (DWORD *) ((EVENT_HEADER *) pevent+1);

      /*-PAA-Oct06/97 for ring buffer reset the LRL */
      ybk_init((DWORD *) pybos);

      /*---- YBOS Event bank ----*/
      ybk_create(pybos, "EVID", I4_BKTYPE, (DWORD *)&pbuf);
      *(pbuf)++ = ybos_cfileh.current_fragment;          /* Event number */
      *(pbuf)++ = (eqp->info.event_id <<16) | (eqp->info.trigger_mask);  /* Event_ID + Mask */
      *(pbuf)++ = eqp->serial_number;                    /* Serial number */
      *(pbuf)++ = ss_millitime();                        /* Time Stamp */
      *(pbuf)++ = run_number;                            /* run number */
      ybk_close(pybos, pbuf);

      /* Create YBOS bank */
      ybk_create(pybos, "CFIL", I4_BKTYPE, (DWORD *)&pbuf);
      /* save pointer for later */
      pcfile = pbuf;
      (char *)pbuf += sizeof(YBOS_CFILE);
      ybk_close(pybos, pbuf);

      ybk_create(pybos, "PFIL", A1_BKTYPE, (DWORD *)&pbuf);
      memcpy((char *)pbuf,(char *)&ybos_pfileh,sizeof(YBOS_PFILE));
      (char *)pbuf += sizeof(YBOS_PFILE);
      ybk_close(pybos, pbuf);

      ybk_create(pybos, "DFIL", A1_BKTYPE, (DWORD *)&pbuf);
      /* compute data length */
      remaining = filesize-ybos_cfileh.current_read_byte;
      nread = read(dmpf,(char *)pbuf,(remaining > MAX_FRAG_SIZE) ? MAX_FRAG_SIZE : remaining);

      /* adjust target pointer */
      (char *) pbuf += nread;
      /* keep track of statistic */
      ybos_cfileh.current_fragment++;
      ybos_cfileh.fragment_size = nread;
      ybos_cfileh.current_read_byte += nread;
      memcpy((char *)pcfile,(char *)&ybos_cfileh,sizeof(YBOS_CFILE));

      /* close YBOS bank */
      ybk_close(pybos, pbuf);

      /* Fill the Midas header */
      bm_compose_event(pevent, eqp->info.event_id, eqp->info.trigger_mask
		       , ybk_size(pybos), eqp->serial_number++);

/*-PAA-Oct06/97 Added the ring buffer option for FE event send */
      eqp->bytes_sent += pevent->data_size+sizeof(EVENT_HEADER);
      eqp->events_sent++;
      if (eqp->buffer_handle)
        {
/*-PAA- Jun98 These events should be sent directly as they come before the run
        started. If the event channel has to be used, then care should be taken
        if interrupt are being used too. May requires buffer checks like in
        scheduler (mfe.c) */
/* #undef USE_EVENT_CHANNEL */
#ifdef USE_EVENT_CHANNEL
        eb_increment_pointer(eqp->buffer_handle, 
                             pevent->data_size + sizeof(EVENT_HEADER));
#else
        rpc_flush_event();
        bm_send_event(eqp->buffer_handle, pevent,
                       pevent->data_size + sizeof(EVENT_HEADER), SYNC);
#endif
        }
    }
  /* close file */
  if( close( dmpf) )
    {
      printf("File was not closed\n");
      return RDLOG_FAIL_CLOSE;
    }
  return RDLOG_SUCCESS;
}

/*------------------------------------------------------------------*/
void  ybk_init(DWORD *pevent)
/********************************************************************\
  Routine: ybk_init
  Purpose: Prepare a YBOS event for a new YBOS bank structure
           reserve the Logical Record length (LRL)
           The pevent should ALWAYS remain to the same location.
           In this case it will point to the LRL.
  Input:
    DWORD * pevent    pointer to the top of the YBOS event.
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  *pevent = 0;
  return;
}

/*------------------------------------------------------------------*/
static YBOS_BANK_HEADER *__pbkh;
void ybk_create(DWORD *pevent, char *bname, DWORD bt, DWORD *pbkdat)
/********************************************************************\
  Routine: ybk_create
  Purpose: fills up the bank header and return the pointer to the 
           first DWORD of the user space.
  Input:
    DWORD * pevent        pointer to the top of the YBOS event (LRL)
    char  * bname         Bank name should be char*4
    DWORD   bt            Bank type can by either
                          I2_BKTYPE, I1_BKTYPE, I4_BKTYPE, F4_BKTYPE
  Output:
    DWORD   *pbkdat       pointer to first valid data of the created bank
  Function value:
    none
\********************************************************************/
{
  __pbkh         = (YBOS_BANK_HEADER *) ( ((DWORD *)(pevent+1)) + (*(DWORD *)pevent) );
  __pbkh->name   = *((DWORD *)bname);
  __pbkh->number = 1;
  __pbkh->index  = 0;
  __pbkh->length = 0;
  __pbkh->type   = bt;
  *((DWORD **)pbkdat) = (DWORD *)(__pbkh + 1);
  return;
}

static DWORD *__pchaosi4;
void ybk_create_chaos(DWORD *pevent, char *bname, DWORD bt, void *pbkdat)
/********************************************************************\
  Routine: ybk_create
  Purpose: fills up the bank header,
           reserve the first 4bytes for the size of the bank in bt unit
           and return the pointer to the next DWORD of the user space.
  Input:
    DWORD * pevent        pointer to the top of the YBOS event (LRL)
    char  * bname         Bank name should be char*4
    DWORD   bt            Bank type can by either
                          I2_BKTYPE, I1_BKTYPE, I4_BKTYPE, F4_BKTYPE
  Output:
    DWORD   *pbkdat       pointer to first valid data of the created bank
  Function value:
    none
\********************************************************************/
{
  __pbkh         = (YBOS_BANK_HEADER *) ( (pevent+1) + (*pevent) );
  __pbkh->name   = *((DWORD *)bname);
  __pbkh->number = 1;
  __pbkh->index  = 0;
  __pbkh->length = 0;
  __pbkh->type   = bt;

  *((DWORD **)pbkdat) = (DWORD *)(__pbkh + 1);
  __pchaosi4 = (DWORD *)(*(DWORD *)pbkdat);
  *((DWORD **)pbkdat) += 1;
  return;
}

/*------------------------------------------------------------------*/
void  ybk_close(DWORD *pevent, void *pbkdat)
/********************************************************************\
  Routine: ybk_close
  Purpose: patch the end of the event to the next 4 byte boundary,
           fills up the bank header,
           update the LRL (pevent)
  Input:
    DWORD * pevent        pointer to the top of the YBOS event (LRL).
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  DWORD tdlen;
  /* align pbkdat to I*4 */
  if (((DWORD) pbkdat & 0x1) != 0)
    {         
      *((BYTE *)pbkdat) = 0x0f;
      ((BYTE *)pbkdat)++;
    }
  if (((DWORD) pbkdat & 0x2) != 0)
    {
      *((WORD *)pbkdat) = 0x0ffb;
      ((WORD *)pbkdat)++;
    }

  /* length in byte */
  tdlen = (DWORD)((char *) pbkdat - (char *)__pbkh - sizeof(YBOS_BANK_HEADER));

  /* YBOS bank length in I4 */
  __pbkh->length = (tdlen + 4) / 4;       /* (+Bank Type &@#$!) YBOS bank length */

  /* adjust Logical Record Length (entry point from the system) */
  *pevent += __pbkh->length + (sizeof(YBOS_BANK_HEADER)/4) - 1;
  return;
}

/*------------------------------------------------------------------*/
void  ybk_close_chaos(DWORD *pevent, DWORD bt, void *pbkdat)
/********************************************************************\
  Routine: ybk_close_chaos
  Purpose: patch the end of the event to the next 4 byte boundary,
           fills up the bank header,
           compute the data size in bt unit.
           update the LRL (pevent)
  Input:
    DWORD * pevent        pointer to the top of the YBOS event (LRL).
    DWORD   bt            bank type
                          I2_BKTYPE, I1_BKTYPE, I4_BKTYPE, F4_BKTYPE
    void  * pbkdat        pointer to the user area
  Output:
    none
  Function value:
    none
\********************************************************************/
{
	switch (bt)
	  {
	  case I4_BKTYPE:
	  case F4_BKTYPE:
	   *__pchaosi4 = (DWORD) ((DWORD *)pbkdat - __pchaosi4 - 1);
	   break;
	 case I2_BKTYPE:
	   *__pchaosi4 = (DWORD) (  (WORD *)pbkdat - (WORD *)__pchaosi4 - 2);
	   SWAP_D2WORD (__pchaosi4);
     break;
	 case I1_BKTYPE:
	 case A1_BKTYPE:
	   *__pchaosi4 = (DWORD) ( (BYTE *)pbkdat - (BYTE *)__pchaosi4 - 4);
	   break;
	   }

  ybk_close(pevent, pbkdat);
}

/*------------------------------------------------------------------*/
INT ybk_size(DWORD *pevent)
/********************************************************************\
  Routine: ybk_init
  Purpose: Compute the overall byte size of the last closed YBOS event
  Input:
    DWORD * pevent    pointer to the top of the YBOS event.
  Output:
    none
  Function value:
    INT size in byte including the LRL
\********************************************************************/
{
  return (*((DWORD *)pevent) * 4 + 4);
}

/*------------------------------------------------------------------*/
INT   ybk_list (DWORD *pevt , char *bklist)
/********************************************************************\
  Routine: ybk_list
  Purpose: extract the YBOS bank name listing of a pointed event (->LRL)
  Input:
    DWORD * pevt             pointer to the YBOS event pointing to lrl
  Output:
    char *bklist             returned ASCII string
  Function value:
    YBOS_WRONG_BANK_TYPE     bank type unknown
    number of bank found in this event
\********************************************************************/
{
  DWORD    *pendevt, bank_length, nbk;

  /* check if bank_type < 9 then swapped otherwise return unswapped */
  if (*(pevt+5) > 0x9)
    return (YBOS_WRONG_BANK_TYPE);

  /* end of event pointer */
  pendevt = pevt + *pevt++;

  /*init bank counter and returned string*/
  nbk = 0;
  bklist[0]=0;

  /* scan event */
  while (pevt < pendevt)
    {
      /* extract ybos bank name */
      strncat (bklist,(char *) pevt,4);

      /* update the number of bank counter */
      nbk++;

      /* skip to bank length */
      pevt +=3;

      /* bank length */
      bank_length = *pevt;

      /* skip to next bank */
      pevt += (bank_length+1);
    }
  return(nbk);
}

/*------------------------------------------------------------------*/
INT ybk_find (DWORD *pevt, char * bkname, DWORD * bklen, DWORD * bktype, DWORD **pbkdata)
/********************************************************************\
  Routine: ybk_find
  Purpose: find the requested bank and return the pointer to the
           top of the data section
  Input:
    DWORD * pevt           pointer to the YBOS event pointing to lrl
    char  * bkname         bank name
  Output:
    DWORD * pbklen         data section length in I*4
    DWORD * pbktype        bank type
    DWORD ** pbkdata       pointer to the bank name
  Function value:
    YBOS_SUCCESS
    YBOS_BANK_NOT_FOUND
    YBOS_WRONG_BANK_TYPE

\********************************************************************/
{
  DWORD    *pendevt;

  /* init returned variables */
  *bklen  = 0;
  *bktype = 0;

  /* check if bank_type < 9 then swapped otherwise return unswapped */
  if (*(pevt+5) > 0x9)
    return (YBOS_WRONG_BANK_TYPE);

  /* end of event pointer */
  pendevt = pevt + *pevt++;

  /* scan event */
  while (pevt < pendevt)
  {
    /* check bank name */
    if (strncmp((char *)pevt, bkname, 4) == 0)
    {
      /* bank name match */
      /* extract bank length */
      *bklen = (*(pevt+3)) - 1;         /* exclude bank type */

      /* extract bank type */
      *bktype = *(pevt+4);

      /* return point to bank name */
      *pbkdata = pevt;

      return (YBOS_SUCCESS);
    }
    else
  	{
	    /* skip bank */
	    pevt += *(pevt+3) + 4;
	  }
  }
  return (YBOS_BANK_NOT_FOUND);
}

/*------------------------------------------------------------------*/
INT   ybk_iterate (DWORD *pyevt, YBOS_BANK_HEADER **pybkh , void **pybkdata)
/********************************************************************\
  Routine: ybk_iterate
  Purpose: return the pointer to the bank name and data bank section
           of the current pointed event.
  Input:
  DWORD * pyevt                     pointer to the YBOS event (after the lrl)
  Output:
  YBOS_BANK_HEADER ** pybkh         pointer to the top of the current bank
                                   if (pybkh == NULL) no more bank in event
  void ** pbkdata                   pointer to the data bank section

  Function value:
                                    data length in I*4
\********************************************************************/
{
  static int      len;
  static DWORD  * pendevt;
  static DWORD  * pybk;
  char bname[5];

  /* the event may have several bank
     check if we have been already in here */
  if (*pybkh == NULL)
    {
      /* first time in (skip lrl) */
      *pybkh = (YBOS_BANK_HEADER *) (pyevt+1);

      if ((*pybkh)->type > 0x9)
	{
	  *pybkh = *pybkdata = NULL;
	  return (0);
	}

      /* end of event pointer (+ lrl) */
      pendevt = pyevt + *pyevt;

      /* skip the EVID bank if present */
      *((DWORD *)bname) = (*pybkh)->name;
      if (strncmp (bname,"EVID",4) == 0)
	{
	  len = (*pybkh)->length;
	  (YBOS_BANK_HEADER *)(*pybkh)++;
	  pybk = (DWORD *) *pybkh;
	  pybk += len - 1;
	  *pybkh = (YBOS_BANK_HEADER *) pybk;
	}
    }
  else
    {
      /* already been in iterate*/
      /* skip current pointed bank ( + bank_length + header) */
      len = (*pybkh)->length;
      (YBOS_BANK_HEADER *)(*pybkh)++;
      pybk = (DWORD *) *pybkh;
      pybk += len - 1;
      *pybkh = (YBOS_BANK_HEADER *) pybk;
    }

  /* check for end of event */
  if ((DWORD *)(*pybkh) < pendevt)
    {
      /* points to the data section */
      *pybkdata = (void *) ((YBOS_BANK_HEADER *)(*pybkh) + 1);

      /* length always in I*4 due to YBOS -1 because type included in length !@# */
      return ((*pybkh)->length - 1);
    }
  else
    {
      /* no more bank in this event */
      *pybkh = *pybkdata = NULL;
      return (0);
    }
}

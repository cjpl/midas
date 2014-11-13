/*  Copyright (c) 1993      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 *
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca         Tel: (604) 222-1047    Fax: (604) 222-1074
 *         amaudruz@triumf.ca                            Local:           6234
 * ---------------------------------------------------------------------------
 * $Id$
 *
 *  Description : mdsupport.c :
 *  contains support for the mdump and lazylogger data support.
 *                   related to event writing, reading, display
 */

/**dox***************************************************************/
/** @file mdsupport.c
The Midas Dump support file
*/

/**dox***************************************************************/
/** @addtogroup mdsupportincludecode
 *  
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS


/* include files */
/* moved #define HAVE_FTPLIB into makefile (!vxWorks) */

#define TRACE
#include "midas.h"
#include "msystem.h"

#ifdef HAVE_FTPLIB
#include "ftplib.h"
FTP_CON *ftp_con;
#endif

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif

#include "mdsupport.h"

INT  md_dev_os_read(INT handle, INT type, void *prec, DWORD nbytes, DWORD * nread);
INT  md_dev_os_write(INT handle, INT type, void *prec, DWORD nbytes, DWORD * written);
void md_bank_event_display(void *pevent, INT data_fmt, INT dsp_fmt, INT dsp_mode, char *bn);
void md_raw_event_display(void *pevent, INT data_fmt, INT dsp_fmt);
void md_raw_bank_display(void *pbank, INT data_fmt, INT dsp_fmt);

INT  midas_event_get(void **pevent, DWORD * size);
INT  midas_physrec_get(void *prec, DWORD * readn);
INT  midas_event_skip(INT evtn);
void midas_bank_display(BANK * pbk, INT dsp_fmt);
void midas_bank_display32(BANK32 * pbk, INT dsp_fmt);

struct stat *filestat;
char *ptopmrd;

#ifdef HAVE_ZLIB
gzFile filegz;
#endif

/* General MIDAS struct for util */
typedef struct {
   INT handle;                  /* file handle */
   char name[128];    /* Device name (/dev/nrmt0h) */

   char *pmp;                   /* ptr to a physical TAPE_BUFFER_SIZE block */
   EVENT_HEADER *pmh;           /* ptr to Midas event (midas bank_header) */
   EVENT_HEADER *pme;           /* ptr to Midas content (event+1) (midas bank_header) */
   char *pmrd;                  /* current point in the phyical record */

   DWORD evtn;                  /* current event number */
   DWORD serial;                /* serial event number */
   DWORD evtlen;                /* current event length (-1 if not available) */
   DWORD size;                  /* midas max_evt_size */
   DWORD recn;                  /* current physical record number */
   INT fmt;                     /* contains FORMAT type */
   INT type;                    /* Device type (tape, disk, ...) */
   DWORD runn;                  /* run number */
   BOOL zipfile;
} MY;

MY my;
/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef HAVE_FTPLIB
/*------------------------------------------------------------------*/
INT mftp_open(char *destination, FTP_CON ** con)
{
   INT status;
   short port = 0;
   char *token, host_name[HOST_NAME_LENGTH],
       user[256], pass[256], directory[256], file_name[256], file_mode[256];

   /* 
      destination should have the form:
      host, port, user, password, directory, run%05d.mid, file_mode, command, ...
    */

   /* break destination in components */
   token = strtok(destination, ",");
   if (token)
      strcpy(host_name, token);

   token = strtok(NULL, ",");
   if (token)
      port = atoi(token);

   token = strtok(NULL, ",");
   if (token)
      strcpy(user, token);

   token = strtok(NULL, ",");
   if (token)
      strcpy(pass, token);

   token = strtok(NULL, ",");
   if (token)
      strcpy(directory, token);

   token = strtok(NULL, ",");
   if (token)
      strcpy(file_name, token);

   token = strtok(NULL, ",");
   file_mode[0] = 0;
   if (token)
      strcpy(file_mode, token);

   status = ftp_login(con, host_name, port, user, pass, "");
   if (status >= 0)
      return status;

   status = ftp_chdir(*con, directory);
   if (status >= 0) {
      /* directory does not exist -> create it */
      ftp_mkdir(*con, directory);
      status = ftp_chdir(*con, directory);
   }
   if (status >= 0)
      return status;
   status = ftp_binary(*con);
   if (status >= 0)
      return status;

   if (file_mode[0]) {
      status = ftp_command(*con, "umask %s", file_mode, 200, 250, EOF);
      if (status >= 0)
         return status;
   }

   while (token) {
      token = strtok(NULL, ",");
      if (token) {
         status = ftp_command(*con, token, NULL, 200, 250, EOF);
         if (status >= 0)
            return status;
      }
   }

   if (ftp_open_write(*con, file_name) >= 0)
      return (*con)->err_no;

   return SS_SUCCESS;
}
#endif // HAVE_FTPLIB

/*------------------------------------------------------------------*/
INT md_file_ropen(char *infile, INT data_fmt, INT openzip)
/********************************************************************\
Routine: external md_any_file_ropen
Purpose: Open data file for replay for the given data format.
It uses the local "my" structure.
Input:
INT data_fmt :  YBOS or MIDAS 
char * infile : Data file name
Output:
none
Function value:
status : from lower function
\********************************************************************/
{
   INT status;

   /* fill up record with file name */
   strcpy(my.name, infile);

   /* find out what dev it is ? : check on /dev */
   my.zipfile = FALSE;
   if ((strncmp(my.name, "/dev", 4) == 0) || (strncmp(my.name, "\\\\.\\", 4) == 0)) {
      /* tape device */
      my.type = LOG_TYPE_TAPE;
   } else {
      /* disk device */
      my.type = LOG_TYPE_DISK;
      if (strncmp(infile + strlen(infile) - 3, ".gz", 3) == 0) {
        // FALSE will for now prevent the mdump to see inside the .gz
	// But lazylogger will NOT unzip during copy!
	if (openzip == 0) my.zipfile = FALSE; // ignore zip, copy blindly blocks
	else my.zipfile = TRUE; // Open Zip file
      }
   }

   /* open file */
   if (!my.zipfile) {
      if (my.type == LOG_TYPE_TAPE) {
         status = ss_tape_open(my.name, O_RDONLY | O_BINARY, &my.handle);
      } else if ((my.handle = open(my.name, O_RDONLY | O_BINARY | O_LARGEFILE, 0644)) == -1) {
         printf("dev name :%s Handle:%d \n", my.name, my.handle);
         return (SS_FILE_ERROR);
      }
   } else {
#ifdef HAVE_ZLIB
      if (my.type == LOG_TYPE_TAPE) {
         printf(" Zip on tape not yet supported \n");
         return (SS_FILE_ERROR);
      }
      filegz = gzopen(my.name, "rb");
      my.handle = 0;
      if (filegz == NULL) {
         printf("dev name :%s gzopen error:%d \n", my.name, my.handle);
         return (SS_FILE_ERROR);
      }
#else
      cm_msg(MERROR, "mdsupport", "Zlib not included ... gz file not supported");
      return (SS_FILE_ERROR);
#endif
   }

   if (data_fmt == FORMAT_YBOS) {
      assert(!"YBOS support not supported anymore");
   } else if (data_fmt == FORMAT_MIDAS) {
      my.fmt = FORMAT_MIDAS;
      my.size = TAPE_BUFFER_SIZE;
      my.pmp = (char *) malloc(my.size);
      if (my.pmp == NULL)
         return SS_NO_MEMORY;
      my.pme = (EVENT_HEADER *) my.pmp;

      /* allocate memory for one full event */
      if (my.pmrd != NULL)
         free(my.pmrd);
      my.pmrd = (char *) malloc(5 * MAX_EVENT_SIZE);    /* in bytes */
      ptopmrd = my.pmrd;
      if (my.pmrd == NULL)
         return SS_NO_MEMORY;
      memset((char *) my.pmrd, -1, 5 * MAX_EVENT_SIZE);
      my.pmh = (EVENT_HEADER *) my.pmrd;
   }

   /* initialize pertinent variables */
   my.recn = -1;
   my.evtn = 0;
   return (MD_SUCCESS);
}

/*------------------------------------------------------------------*/
INT md_file_rclose(INT data_fmt)
/********************************************************************
Routine: external md_any_file_rclose
Purpose: Close a data file used for replay for the given data format
Input:
INT data_fmt :  YBOS or MIDAS 
Output:
none
Function value:
status : from lower function
*******************************************************************/
{
   int i;

   i = data_fmt;                /* avoid compiler warning */

   switch (my.type) {
   case LOG_TYPE_TAPE:
   case LOG_TYPE_DISK:
      /* close file */
      if (my.zipfile) {
#ifdef HAVE_ZLIB
         gzclose(filegz);
#endif
      } else {
         if (my.handle != 0)
            close(my.handle);
      }
      break;
   }
   if (ptopmrd != NULL)
      free(ptopmrd);
   if (my.pmp != NULL)
      free(my.pmp);
   my.pmp = NULL;
   my.pmh = NULL;
   ptopmrd = NULL;
   my.pmrd = NULL;              /* ptopmrd and my.pmrd point to the same place. K.O. 25-FEB-2005 */
   return (MD_SUCCESS);
}

/*------------------------------------------------------------------*/
INT md_file_wopen(INT type, INT data_fmt, char *filename, INT * hDev)
/********************************************************************
Routine: external md_file_wopen
Purpose: Open a data file for the given data format
Input:
INT type     :  Tape or Disk
INT data_fmt :  YBOS or MIDAS
char * filename : file to open
Output:
INT * hDev      : file handle
Function value:
status : from lower function
*******************************************************************/
{
   INT status = 0;

   if (type == LOG_TYPE_DISK)
      /* takes care of TapeLX/NT under ss_tape_open , DiskLX/NT here */
   {
      if (data_fmt == FORMAT_YBOS) {
	 assert(!"YBOS not supported anymore");
      } else if (data_fmt == FORMAT_MIDAS) {
#ifdef OS_WINNT
         *hDev =
	   (int) CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ,
			    NULL, CREATE_ALWAYS,
			    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN, 0);
#else
         *hDev = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | O_LARGEFILE, 0644);
#endif
         status = *hDev < 0 ? SS_FILE_ERROR : SS_SUCCESS;
      }
   } else if (type == LOG_TYPE_TAPE) {
     if (data_fmt == FORMAT_YBOS) {
	 assert(!"YBOS not supported anymore");
      } else if (data_fmt == FORMAT_MIDAS)
         status = ss_tape_open(filename, O_WRONLY | O_CREAT | O_TRUNC, hDev);
   } else if (type == LOG_TYPE_FTP) {
#ifdef HAVE_FTPLIB
      status = mftp_open(filename, (FTP_CON **) & ftp_con);
      if (status != SS_SUCCESS) {
         *hDev = 0;
         return status;
      } else
         *hDev = 1;
#else
      cm_msg(MERROR, "mdsupport", "FTP support not included");
      return SS_FILE_ERROR;
#endif
   }

   return status;
}

/*------------------------------------------------------------------*/
INT md_file_wclose(INT handle, INT type, INT data_fmt, char *destination)
/********************************************************************
Routine: external md_file_wclose
Purpose: Close a data file used for replay for the given data format
Input:
INT data_fmt :  YBOS or MIDAS 
Output:
none
Function value:
status : from lower function
*******************************************************************/
{
   INT status;

   status = SS_SUCCESS;
   switch (type) {
   case LOG_TYPE_TAPE:
      /* writing EOF mark on tape Fonly */
      status = ss_tape_write_eof(handle);
      ss_tape_close(handle);
      break;
   case LOG_TYPE_DISK:
      /* close file */
      if (handle != 0)
#ifdef OS_WINNT
         CloseHandle((HANDLE) handle);
#else
         close(handle);
#endif
      break;
   case LOG_TYPE_FTP:
#ifdef HAVE_FTPLIB
      {
      char *p, filename[256];
      int  i;

      ftp_close(ftp_con);

      /* 
         destination should have the form:
         host, port, user, password, directory, run%05d.mid, file_mode, command, ...
      */
      p = destination;
      for (i=0 ; i<5 && p != NULL ; i++) {
         p = strchr(p, ',');
         if (*p == ',')
            p++;
      }
      if (p != NULL) {
         strlcpy(filename, p, sizeof(filename));
         if (strchr(filename, ','))
            *strchr(filename, ',') = 0;
      } else
         strlcpy(filename, destination, sizeof(filename));

      /* if filename starts with a '.' rename it */
      if (filename[0] == '.')
         ftp_move(ftp_con, filename, filename+1);

      ftp_bye(ftp_con);
      }
#endif
      break;
   }
   if (status != SS_SUCCESS)
      return status;
   return (MD_SUCCESS);
}

/*------------------------------------------------------------------*/
INT md_dev_os_read(INT handle, INT type, void *prec, DWORD nbytes, DWORD * readn)
/********************************************************************\
Routine: md_dev_os_read
Purpose: read nbytes from the type device.
Input:
INT  handle        file handler
INT  type          Type of device (TAPE or DISK)
void * prec        pointer to the record
DWORD nbytes       # of bytes to read
Output:
DWORD *readn       # of bytes read
Function value:
MD_DONE            No more record to read
MD_SUCCESS         Ok
\********************************************************************/
{
   INT status;
   if (type == LOG_TYPE_DISK)
      /* --------- DISK ---------- */
   {
      *readn = read(handle, prec, nbytes);
      if (*readn <= 0)
         status = SS_FILE_ERROR;
      else
         status = SS_SUCCESS;
      return status;
   }
   /* --------- TAPE ---------- */
#ifdef OS_UNIX
   else if (type == LOG_TYPE_TAPE) {
      *readn = read(handle, prec, nbytes);
      if (*readn <= 0)
         status = SS_FILE_ERROR;
      else
         status = SS_SUCCESS;
      return status;
   }
#endif

#ifdef OS_WINNT
   else if (type == LOG_TYPE_TAPE) {
      if (!ReadFile((HANDLE) handle, prec, nbytes, readn, NULL))
         status = GetLastError();
      else
         status = SS_SUCCESS;
      if (status == ERROR_NO_DATA_DETECTED)
         status = SS_END_OF_TAPE;

      return status;
   }
#endif                          /* OS_WINNT */
   else
      return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT md_dev_os_write(INT handle, INT type, void *prec, DWORD nbytes, DWORD * written)
/********************************************************************\
Routine: md_dev_os_write
Purpose: write nbytes to the device.
Input:
INT  handle        file handler
INT  type          Type of device (TAPE or DISK)
void * prec        pointer to the record
DWORD   nbytes     record length to be written
Output:
DWORD *written     # of written bytes
Function value:
INT status           # of written bytes or ERROR
SS_FILE_ERROR      write error
SS_SUCCESS         Ok
\********************************************************************/
{
   INT status;
   if (type == LOG_TYPE_DISK)
#ifdef OS_WINNT
   {                            /* --------- DISK ---------- */
      WriteFile((HANDLE) handle, (char *) prec, nbytes, written, NULL);
      status = *written == nbytes ? SS_SUCCESS : SS_FILE_ERROR;
      return status;            /* return for DISK */
   }
#else
   {                            /* --------- DISK ---------- */
     status = *written = write(handle, (char *) prec, nbytes) == (INT) nbytes ? SS_SUCCESS : SS_FILE_ERROR;
      return status;            /* return for DISK */
   }
#endif
   else if (type == LOG_TYPE_TAPE) {    /* --------- TAPE ---------- */
#ifdef OS_UNIX
      do {
         status = write(handle, (char *) prec, nbytes);
      } while (status == -1 && errno == EINTR);
      *written = status;
      if (*written != nbytes) {
         cm_msg(MERROR, "any_dev_os_write", strerror(errno));
         if (errno == EIO)
            return SS_IO_ERROR;
         if (errno == ENOSPC)
            return SS_NO_SPACE;
         else
            return SS_TAPE_ERROR;
      }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
      WriteFile((HANDLE) handle, (char *) prec, nbytes, written, NULL);
      if (*written != nbytes) {
         status = GetLastError();
         cm_msg(MERROR, "any_dev_os_write", "error %d", status);
         return SS_IO_ERROR;
      }
      return SS_SUCCESS;        /* return for TAPE */
#endif                          /* OS_WINNT */
   } else if (type == LOG_TYPE_FTP)
#ifdef HAVE_FTPLIB
   {
      *written = status = ftp_send(ftp_con->data, (char *) prec,
                                   (int) nbytes) == (int) nbytes ? SS_SUCCESS : SS_FILE_ERROR;
      return status;
   }
#else
   {
      cm_msg(MERROR, "mdsupport", "FTP support not included");
      return SS_IO_ERROR;
   }
#endif
   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT md_physrec_get(INT data_fmt, void **precord, DWORD * readn)
/********************************************************************\
Routine: external md_physrec_get
Purpose: Retrieve a physical record for the given data format
Input:
INT data_fmt :  YBOS or MIDAS 
Output:
void ** precord     pointer to the record
DWORD * readn       record length in bytes
Function value:
status : from lower function
\********************************************************************/
{
   *precord = my.pmp;
   if (data_fmt == FORMAT_MIDAS) {
      return midas_physrec_get(*precord, readn);
   } else
     return MD_UNKNOWN_FORMAT;
}

/*------------------------------------------------------------------*/
INT md_log_write(INT handle, INT data_fmt, INT type, void *prec, DWORD nbytes)
/********************************************************************\
Routine: external md_log_write
Purpose: Write a physical record to the out device.

Input:
void handle   : file handle
INT data_fmt  : YBOS or MIDAS 
INT type      : Tape or disk 
void *prec    : record pointer
DWORD nbytes  : record length to be written
Output:
none
Function value:
status : from lower function  SS_SUCCESS, SS_FILE_ERROR
\********************************************************************/
{
   INT status;
   DWORD written;

   status = data_fmt;           /* avoid compiler warning */
   /* write record */
   status = md_dev_os_write(handle, type, prec, nbytes, &written);
   return status;
}

/*------------------------------------------------------------------*/
INT md_physrec_skip(INT data_fmt, INT bl)
/********************************************************************\
Routine: external md_physrec_skip
Purpose: Skip physical record until block = bl for the given data
format, Under midas the skip is an event as no physical 
record is present under that format,
Input:
INT data_fmt :  YBOS or MIDAS 
INT bl:         block number (-1==all, 0 = first block)
in case of MIDAS the bl represent an event
Output:
none
Function value:
status : from lower function
\********************************************************************/
{
   INT status;

   if (data_fmt == FORMAT_MIDAS) {
      status = midas_event_skip(bl);
      return MD_SUCCESS;
   } else
      return MD_UNKNOWN_FORMAT;
}

/*------------------------------------------------------------------*/
INT midas_event_skip(INT evtn)
/********************************************************************\
Routine: midas_event_skip
Purpose: skip event on a MIDAS file.
Input:
INT     evtn          event record number. (start at 0)
if evt = -1 : skip skiping
Output:
none
Function value:
MD_SUCCESS        Ok
\********************************************************************/
{
   void *pevent;
   DWORD size;

   size = MAX_EVENT_SIZE;
   if (evtn == -1) {
      /*    if(midas_event_get(&pevent, &size) == MD_SUCCESS) */
      return MD_SUCCESS;
   }
   while (midas_event_get(&pevent, &size) == MD_SUCCESS) {
      if ((INT) my.evtn < evtn) {
         printf("Skipping event_# ... ");
         printf("%d \r", my.evtn);
         fflush(stdout);
      } else {
         printf("\n");
         return MD_SUCCESS;
      }
   }
   return MD_DONE;
}

/*------------------------------------------------------------------*/
INT md_physrec_display(INT data_fmt)
/********************************************************************\
Routine: external md_physrec_display
Purpose: Display the physical record of the current record 
for the given data format.
Not possible for MIDAS as no physical record structure
Input:
INT data_fmt :  YBOS or MIDAS 
Output:
none
Function value:
status          Lower function
\********************************************************************/
{

   if (data_fmt == FORMAT_MIDAS) {
      printf(">>> No physical record structure for Midas format <<<\n");
      return MD_DONE;
   } else if (data_fmt == FORMAT_YBOS) {
     assert(!"YBOS not supported anymore");
     return (MD_SUCCESS);
   } else
     return MD_UNKNOWN_FORMAT;
}

/*------------------------------------------------------------------*/
INT md_all_info_display(INT what)
/********************************************************************\
Routine: md_all_info_display
Purpose: display on screen all the info about "what".
Input:
INT     what              type of display.
Output:
none
Function value:
INT                 MD_SUCCESS
MD_DONE
\********************************************************************/
{
   if (my.fmt == FORMAT_YBOS) {
     assert(!"YBOS not supported anymore");
   } else if (my.fmt == FORMAT_MIDAS) {
     DWORD mbn, run, ser;
     WORD id, msk;
     mbn = my.evtn;
     run = my.runn;
     id = my.pmh->event_id;
     msk = my.pmh->trigger_mask;
     ser = my.pmh->serial_number;
     switch (what) {
     case D_RECORD:
     case D_HEADER:
       printf(">>> No physical record structure for Midas format <<<\n");
       return MD_DONE;
       break;
     case D_EVTLEN:
       printf("Evt#%d- ", my.evtn);
       printf("%irun 0x%4.4uxid 0x%4.4uxmsk %5dmevt#", run, id, msk, mbn);
       printf("%5del/x%x %5dserial\n", my.evtlen, my.evtlen, ser);
       break;
     }
   }
   return MD_SUCCESS;
}

/*------------------------------------------------------------------*/
INT md_event_swap(INT data_fmt, void *pevent)
/********************************************************************\
Routine: external md_event_swap
Purpose: Swap an event from the given data format.
Input:
INT data_fmt  : YBOS or MIDAS 
void * pevent : pointer to either plrl or pheader
Output:
none
Function value:
status :  from the lower function
\********************************************************************/
{
   INT status;
   BANK_HEADER *pbh;

   if (data_fmt == FORMAT_MIDAS) {
      if ((((EVENT_HEADER *) pevent)->event_id == EVENTID_BOR) ||
          (((EVENT_HEADER *) pevent)->event_id == EVENTID_EOR) ||
          (((EVENT_HEADER *) pevent)->event_id == EVENTID_MESSAGE))
         return SS_SUCCESS;
      pbh = (BANK_HEADER *) (((EVENT_HEADER *) pevent) + 1);
      status = bk_swap(pbh, FALSE);
      return status == CM_SUCCESS ? MD_EVENT_NOT_SWAPPED : MD_SUCCESS;
   } else if (data_fmt == FORMAT_YBOS) {
     assert(!"YBOS not supported anymore");
   }

   return MD_UNKNOWN_FORMAT;
}

/*------------------------------------------------------------------*/
INT md_event_get(INT data_fmt, void **pevent, DWORD * readn)
/********************************************************************\
Routine: external md_event_get
Purpose: Retrieve an event from the given data format.
Input:
INT data_fmt :  YBOS or MIDAS 
void ** pevent : either plrl or pheader
Output:
DWORD * readn : number of bytes read
Function value:
status : from lower function
\********************************************************************/
{
   INT status = 0;

   *pevent = NULL;
   if (data_fmt == FORMAT_MIDAS)
     status = midas_event_get(pevent, readn);
   else if (data_fmt == FORMAT_YBOS)
     assert(!"YBOS not supported anymore");
   return (status);
}

/*------------------------------------------------------------------*/
void md_event_display(void *pevent, INT data_fmt, INT dsp_mode, INT dsp_fmt, char *bn)
/********************************************************************\
Routine: external md_event_display
Purpose: display on screen the event data in either Decimal or Hexadecimal.
Input:
void *  pevent     pointer to either plrl or pheader.
INT     data_fmt   uses the YBOS or MIDAS event structure
INT     dsp_mode   display in RAW or Bank mode
INT     dsp_fmt    display format (DSP_DEC/HEX)
Output:
none
Function value:
none
\********************************************************************/
{
   if (dsp_mode == DSP_RAW)
      md_raw_event_display(pevent, data_fmt, dsp_fmt);
   else if ((dsp_mode == DSP_BANK) || (dsp_mode == DSP_BANK_SINGLE))
      md_bank_event_display(pevent, data_fmt, dsp_fmt, dsp_mode, bn);
   else
      printf("mdsupport- Unknown format:%i\n", dsp_fmt);
   return;
}

/*------------------------------------------------------------------*/
void md_raw_event_display(void *pevent, INT data_fmt, INT dsp_fmt)
/********************************************************************\
Routine: md_raw_event_display
Purpose: display on screen the RAW data of MIDAS format.
Input:
DWORD *  pevent         points to either plrl or pheader
INT     data_fmt        uses the YBOS or MIDAS event structure
INT      dsp_fmt        display format (DSP_DEC/HEX)
Output:
none
Function value:
none
\********************************************************************/
{
   DWORD lrl = 0, *pevt = NULL, j, i, total = 0;

   if (data_fmt == FORMAT_YBOS) {
      assert(!"YBOS not supported anymore");
   } else if (data_fmt == FORMAT_MIDAS) {
      lrl = ((((EVENT_HEADER *) pevent)->data_size) + sizeof(EVENT_HEADER)) / sizeof(DWORD);    /* in I*4 for raw including the header */
      pevt = (DWORD *) pevent;  /* local copy starting from the pheader */
   }

   for (i = 0; i < lrl; i += NLINE) {
      printf("%6.0d->: ", total);
      for (j = 0; j < NLINE; j++) {
         if ((i + j) < lrl) {
            if (dsp_fmt == DSP_DEC)
               printf("%8.i ", *pevt);
            else
               printf("%8.8x ", *pevt);
            pevt++;
         }
      }
      total += NLINE;
      printf("\n");
   }
}

/*------------------------------------------------------------------*/
void md_bank_event_display(void *pevent, INT data_fmt, INT dsp_fmt, INT dsp_mode, char *bn)
/********************************************************************\
Routine: md_bank_event_display
Purpose: display on screen the event header, bank list and bank content
for midas format.)
Input:
void * pevent          points to either plrl or pheader
INT     data_fmt       uses the YBOS or MIDAS event structure
INT     dsp_fmt        display format (DSP_DEC/HEX)
Output:
none
Function value:
none
\********************************************************************/
{
   char banklist[MD_STRING_BANKLIST_MAX + STRING_BANKLIST_MAX];
   DWORD *pdata, *pdata1;
   BANK_HEADER *pbh = NULL;
   BANK *pmbk;
   BANK32 *pmbk32;
   EVENT_HEADER *pheader;
   INT status, single = 0;

   if (data_fmt == FORMAT_YBOS) {
     assert(!"YBOS not supported anymore");
   } else if (data_fmt == FORMAT_MIDAS) {
     /* skip these special events (NO bank structure) */
     pheader = (EVENT_HEADER *) pevent;
     if (pheader->event_id == EVENTID_BOR ||
	 pheader->event_id == EVENTID_EOR || pheader->event_id == EVENTID_MESSAGE)
       return;
     
     /* check if format is MIDAS or FIXED */
     pbh = (BANK_HEADER *) (pheader + 1);
     
     /* Check for single bank display request */
     if (dsp_mode == DSP_BANK_SINGLE) {
       bk_locate(pbh, bn, &pdata1);
       single = 1;
     }
     /* event header (skip it if in single bank display) */
     if (!single)
       printf
	 ("Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Dsize:%i/0x%x",
	  (WORD) pheader->event_id, (WORD) pheader->trigger_mask,
	  pheader->serial_number, pheader->time_stamp, pheader->data_size, pheader->data_size);
     
     if ((pbh->data_size + 8) == pheader->data_size) {
       /* bank list */
       if (!single) {
	 /* Skip list if in single bank display */
	 status = bk_list((BANK_HEADER *) (pheader + 1), banklist);
	 printf("\n#banks:%i - Bank list:-%s-\n", status, banklist);
       }
       
       /* display bank content */
       if (bk_is32(pbh)) {
	 pmbk32 = NULL;
	 do {
	   bk_iterate32(pbh, &pmbk32, &pdata);
	   if (pmbk32 != NULL)
	     if (single && (pdata == pdata1))
	       midas_bank_display32(pmbk32, dsp_fmt);
	   if (!single)
	     if (pmbk32 != NULL)
	       midas_bank_display32(pmbk32, dsp_fmt);
	 } while (pmbk32 != NULL);
       } else {
	 pmbk = NULL;
	 do {
	   bk_iterate(pbh, &pmbk, &pdata);
	   if (pmbk != NULL)
	     if (single && (pdata == pdata1))
	       midas_bank_display(pmbk, dsp_fmt);
	   if (!single)
	     if (pmbk != NULL)
	       midas_bank_display(pmbk, dsp_fmt);
	 } while (pmbk != NULL);
       }
     } else {
       printf("\nFIXED event with Midas Header\n");
       md_raw_event_display(pevent, data_fmt, dsp_fmt);
     }
   }
   
   return;
}

/*------------------------------------------------------------------*/
void md_bank_display(void *pmbh, void *pbk, INT data_fmt, INT dsp_mode, INT dsp_fmt)
/********************************************************************\
Routine: external md_bank_display
Purpose: display on screen the given bank.
Input:
void * pbk          pointer to the bank
INT  data_fmt       YBOS or MIDAS
INT  dsp_mode       display mode (RAW/BANK)
INT  dsp_fmt        display format (DSP_DEC/HEX)
Output:             
none
Function value:
none
\********************************************************************/
{
   if (dsp_mode == DSP_RAW)
      md_raw_bank_display(pbk, data_fmt, dsp_fmt);
   else {
      if (data_fmt == FORMAT_MIDAS) {
         if (bk_is32(pmbh))
            midas_bank_display32((BANK32 *) pbk, dsp_fmt);
         else
            midas_bank_display((BANK *) pbk, dsp_fmt);
      } else if (data_fmt == FORMAT_YBOS)
      assert(!"YBOS not supported anymore");
   }
   return;
}

/*------------------------------------------------------------------*/
void md_raw_bank_display(void *pbank, INT data_fmt, INT dsp_fmt)
/********************************************************************\
Routine: md__raw_bank_display
Purpose: display on screen the RAW data of a given YBOS/MIDAS bank.
Input:
void  * pbank          pointer to the bank name
INT     data_fmt       uses the YBOS or MIDAS event structure
INT     dsp_fmt        display format (DSP_DEC/HEX)
Output:
none
Function value:
none
\********************************************************************/
{
   DWORD *pdata = NULL, lrl = 0, j, i;

   if (data_fmt == FORMAT_YBOS) {
     assert(!"YBOS not supported any ore");
   } else if (data_fmt == FORMAT_MIDAS) {
      lrl = ((BANK *) pbank)->data_size >> 2;   /* in DWORD */
      pdata = (DWORD *) ((BANK *) (pbank) + 1);
   }

   for (i = 0; i < lrl; i += NLINE) {
      j = 0;
      printf("\n%4i-> ", i + j + 1);
      for (j = 0; j < NLINE; j++) {
         if ((i + j) < lrl) {
            if (dsp_fmt == DSP_DEC)
               printf("%8.i ", *((DWORD *) pdata));
            if (dsp_fmt == DSP_ASC)
               printf("%8.8x ", *((DWORD *) pdata));
            if (dsp_fmt == DSP_HEX)
               printf("%8.8x ", *((DWORD *) pdata));
            pdata++;
         }
      }
   }
}

/*------------------------------------------------------------------*/
INT midas_event_get(void **pevent, DWORD * readn)
/********************************************************************\
Routine: midas_event_get
Purpose: read one MIDAS event.
Will detect:
The first pass in getting the record number being -1
The last pass in checking the midas_physrec_get() readn
being different then the my.size then flushing the current
buffer until pointer goes beyond last event.
Input:
void ** pevent     points to MIDAS HEADER 
Output:
DWORD * readn      event size in bytes (MIDAS)
Function value:
MD_DONE           No more record to read
MD_SUCCESS        Ok
\********************************************************************/
{
   INT status, leftover;
   DWORD fpart;
   static DWORD size = 0;

   /* save pointer */
   *pevent = (char *) my.pmh;
   if (size == 0)
      size = my.size;

   /* first time in get physrec once */
   if ((int) my.recn == -1) {
      status = midas_physrec_get((void *) my.pmp, &size);
      if (status != MD_SUCCESS)
         return (MD_DONE);
   }

  /*-PAA- Jul 12/2002
    if (((my.pmp+size) - (char *)my.pme) == 0)
        return (MD_DONE);
  */

   /* copy header only */
   if (((my.pmp + size) - (char *) my.pme) < (int) sizeof(EVENT_HEADER)) {
      fpart = (my.pmp + my.size) - (char *) my.pme;
      memcpy(my.pmh, my.pme, fpart);
      my.pmh = (EVENT_HEADER *) (((char *) my.pmh) + fpart);
      leftover = sizeof(EVENT_HEADER) - fpart;
      status = midas_physrec_get((void *) my.pmp, &size);
      if (status != MD_SUCCESS)
         return (MD_DONE);
      memset(my.pmp + size, -1, my.size - size);
      my.pme = (EVENT_HEADER *) my.pmp;
      memcpy(my.pmh, my.pme, leftover);
      my.pme = (EVENT_HEADER *) (((char *) my.pme) + leftover);
      my.pmh = (EVENT_HEADER *) * pevent;
   } else {
      memcpy(my.pmh, my.pme, sizeof(EVENT_HEADER));
      my.pme = (EVENT_HEADER *) (((char *) my.pme) + sizeof(EVENT_HEADER));
   }
   /* leave with pmh  to destination header
      pmrd to destination event (pmh+1)
      pme  to source event
    */
   my.pmrd = (char *) (my.pmh + 1);

   /* check for end of file */
   if (my.pmh->event_id == -1)
      return MD_DONE;

   /* copy event (without header) */
   leftover = my.pmh->data_size;

   /* check for block crossing */
   while (((my.pmp + size) - (char *) my.pme) < leftover) {
      fpart = (my.pmp + my.size) - (char *) my.pme;
      memcpy(my.pmrd, my.pme, fpart);
      my.pmrd += fpart;
      leftover -= fpart;
      status = midas_physrec_get((void *) my.pmp, &size);
      if (status != MD_SUCCESS)
         return (MD_DONE);
      memset(my.pmp + size, -1, my.size - size);
      my.pme = (EVENT_HEADER *) my.pmp;
   }

   /* copy left over or full event if no Xing */
   *readn = my.evtlen = my.pmh->data_size + sizeof(EVENT_HEADER);
   memcpy(my.pmrd, my.pme, leftover);
   my.pme = (EVENT_HEADER *) (((char *) my.pme) + leftover);
   my.evtn++;
   return MD_SUCCESS;
}

/*------------------------------------------------------------------*/
INT midas_physrec_get(void *prec, DWORD * readn)
/********************************************************************\
Routine: midas_physrec_get
Purpose: read one physical record.from a MIDAS run
This is a "fake" physical record as Midas is
not block structured. This function is used for
reading a my.size record size. The return readn if different
then my.size, will indicate a end of run. An extra read will
indicate an eof.

Input:
void * prec        pointer to the record
Output:
DWORD *readn       retrieve number of bytes
Function value:
MD_DONE            No more record to read
MD_SUCCESS         Ok
\********************************************************************/
{
   INT status = 0;

   /* read one block of data */
   if (!my.zipfile) {
      status = md_dev_os_read(my.handle, my.type, prec, my.size, readn);
   } else {
#ifdef HAVE_ZLIB
      *readn = gzread(filegz, (char *) prec, my.size);
      if (*readn <= 0)
         status = SS_FILE_ERROR;
      else
         status = SS_SUCCESS;
#endif
   }

   if (status != SS_SUCCESS) {
      return (MD_DONE);
   } else {
      /* count blocks */
      my.recn++;
      return (MD_SUCCESS);
   }
}

/*------------------------------------------------------------------*/
void midas_bank_display(BANK * pbk, INT dsp_fmt)
/******************************* *************************************\
Routine: midas_bank_display
Purpose: display on screen the pointed MIDAS bank data using MIDAS Bank structure.
Input:
BANK *  pbk            pointer to the BANK
INT     dsp_fmt        display format (DSP_DEC/HEX)
Output:
none
Function value:
none
\********************************************************************/
{
   char bank_name[5], strbktype[32];
   char *pdata, *pendbk;
   DWORD length_type = 0, lrl;
   INT type, i, j;

   lrl = pbk->data_size;        /* in bytes */
   type = pbk->type & 0xff;
   bank_name[4] = 0;
   memcpy(bank_name, (char *) (pbk->name), 4);
   pdata = (char *) (pbk + 1);

   j = 64;                      /* elements within line */
   i = 1;                       /* data counter */
   strcpy(strbktype, "Unknown format");
   if (type == TID_DOUBLE) {
      length_type = sizeof(double);
      strcpy(strbktype, "double*8");
   }
   if (type == TID_FLOAT) {
      length_type = sizeof(float);
      strcpy(strbktype, "Real*4 (FMT machine dependent)");
   }
   if (type == TID_DWORD) {
      length_type = sizeof(DWORD);
      strcpy(strbktype, "Unsigned Integer*4");
   }
   if (type == TID_INT) {
      length_type = sizeof(INT);
      strcpy(strbktype, "Signed Integer*4");
   }
   if (type == TID_WORD) {
      length_type = sizeof(WORD);
      strcpy(strbktype, "Unsigned Integer*2");
   }
   if (type == TID_SHORT) {
      length_type = sizeof(short);
      strcpy(strbktype, "Signed Integer*2");
   }
   if (type == TID_BYTE) {
      length_type = sizeof(BYTE);
      strcpy(strbktype, "Unsigned Bytes");
   }
   if (type == TID_SBYTE) {
      length_type = sizeof(BYTE);
      strcpy(strbktype, "Signed Bytes");
   }
   if (type == TID_BOOL) {
      length_type = sizeof(DWORD);
      strcpy(strbktype, "Boolean");
   }
   if (type == TID_CHAR) {
      length_type = sizeof(char);
      strcpy(strbktype, "8 bit ASCII");
   }
   if (type == TID_STRUCT) {
      length_type = sizeof(char);
      strcpy(strbktype, "STRUCT (not supported->8 bits)");
   }
   if (type == TID_STRING) {
      length_type = sizeof(char);
      strcpy(strbktype, "String 8bit ASCII");
   }

   printf("\nBank:%s Length: %i(I*1)/%i(I*4)/%i(Type) Type:%s",
          bank_name, lrl, lrl >> 2, lrl / (length_type == 0 ? 1 : length_type), strbktype);

   pendbk = pdata + lrl;
   while (pdata < pendbk) {
      switch (type) {
      case TID_DOUBLE:
         if (j > 3) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 4;
         }
         printf("%15.5le    ", *((double *) pdata));
         pdata = (char *) (((double *) pdata) + 1);
         j++;
         break;
      case TID_FLOAT:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%8.3e ", *((float *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%8.8x ", *((DWORD *) pdata));
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_DWORD:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%8.1i ", *((DWORD *) pdata));
         if ((dsp_fmt == DSP_HEX) || (dsp_fmt == DSP_UNK))
            printf("0x%8.8x ", *((DWORD *) pdata));
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_INT:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%8.1i ", *((DWORD *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%8.8x ", *((DWORD *) pdata));
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_WORD:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%5.1i ", *((WORD *) pdata));
         if ((dsp_fmt == DSP_HEX) || (dsp_fmt == DSP_UNK))
            printf("0x%4.4x ", *((WORD *) pdata));
         pdata = (char *) (((WORD *) pdata) + 1);
         j++;
         break;
      case TID_SHORT:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%5.1i ", *((short *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%4.4x ", *((short *) pdata));
         pdata = (char *) (((short *) pdata) + 1);
         j++;
         break;
      case TID_BYTE:
      case TID_STRUCT:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%4.i ", *((BYTE *) pdata));
         if ((dsp_fmt == DSP_HEX) || (dsp_fmt == DSP_UNK))
            printf("0x%2.2x ", *((BYTE *) pdata));
         pdata++;
         j++;
         break;
      case TID_SBYTE:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%4.i ", *((BYTE *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%2.2x ", *((BYTE *) pdata));
         pdata++;
         j++;
         break;
      case TID_BOOL:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         (*((BOOL *) pdata) != 0) ? printf("Y ") : printf("N ");
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_CHAR:
      case TID_STRING:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%3.i ", *((BYTE *) pdata));
         if ((dsp_fmt == DSP_ASC) || (dsp_fmt == DSP_UNK))
            printf("%1.1s ", (char *) pdata);
         if (dsp_fmt == DSP_HEX)
            printf("0x%2.2x ", *((BYTE *) pdata));
         pdata++;
         j++;
         break;
      default:
         printf("bank type not supported (%d)\n", type);
         return;
         break;
      }
   }                            /* end of bank */
   printf("\n");
   return;
}

/*------------------------------------------------------------------*/
void midas_bank_display32(BANK32 * pbk, INT dsp_fmt)
/********************************************************************\
Routine: midas_bank_display32
Purpose: display on screen the pointed MIDAS bank data using MIDAS Bank structure.
for 32bit length banks
Input:
BANK32 *  pbk            pointer to the BANK
INT     dsp_fmt        display format (DSP_DEC/HEX)
Output:
none
Function value:
none
\********************************************************************/
{
   char bank_name[5], strbktype[32];
   char *pdata, *pendbk;
   DWORD length_type = 0, lrl;
   INT type, i, j;

   lrl = pbk->data_size;        /* in bytes */
   type = pbk->type & 0xff;
   bank_name[4] = 0;
   memcpy(bank_name, (char *) (pbk->name), 4);
   pdata = (char *) (pbk + 1);

   j = 64;                      /* elements within line */
   i = 1;                       /* data counter */
   strcpy(strbktype, "Unknown format");
   if (type == TID_DOUBLE) {
      length_type = sizeof(double);
      strcpy(strbktype, "double*8");
   }
   if (type == TID_FLOAT) {
      length_type = sizeof(float);
      strcpy(strbktype, "Real*4 (FMT machine dependent)");
   }
   if (type == TID_DWORD) {
      length_type = sizeof(DWORD);
      strcpy(strbktype, "Unsigned Integer*4");
   }
   if (type == TID_INT) {
      length_type = sizeof(INT);
      strcpy(strbktype, "Signed Integer*4");
   }
   if (type == TID_WORD) {
      length_type = sizeof(WORD);
      strcpy(strbktype, "Unsigned Integer*2");
   }
   if (type == TID_SHORT) {
      length_type = sizeof(short);
      strcpy(strbktype, "Signed Integer*2");
   }
   if (type == TID_BYTE) {
      length_type = sizeof(BYTE);
      strcpy(strbktype, "Unsigned Bytes");
   }
   if (type == TID_SBYTE) {
      length_type = sizeof(BYTE);
      strcpy(strbktype, "Signed Bytes");
   }
   if (type == TID_BOOL) {
      length_type = sizeof(DWORD);
      strcpy(strbktype, "Boolean");
   }
   if (type == TID_CHAR) {
      length_type = sizeof(char);
      strcpy(strbktype, "8 bit ASCII");
   }
   if (type == TID_STRUCT) {
      length_type = sizeof(char);
      strcpy(strbktype, "STRUCT (not supported->8 bits)");
   }
   if (type == TID_STRING) {
      length_type = sizeof(char);
      strcpy(strbktype, "String 8bit ASCI");
   }

   printf("\nBank:%s Length: %i(I*1)/%i(I*4)/%i(Type) Type:%s",
          bank_name, lrl, lrl >> 2, lrl / (length_type == 0 ? 1 : length_type), strbktype);

   pendbk = pdata + lrl;
   while (pdata < pendbk) {
      switch (type) {
      case TID_DOUBLE:
         if (j > 3) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 4;
         }
         printf("%15.5e    ", *((double *) pdata));
         pdata = (char *) (((double *) pdata) + 1);
         j++;
         break;
      case TID_FLOAT:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%8.3e ", *((float *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%8.8x ", *((DWORD *) pdata));
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_DWORD:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%8.1i ", *((DWORD *) pdata));
         if ((dsp_fmt == DSP_HEX) || (dsp_fmt == DSP_UNK))
            printf("0x%8.8x ", *((DWORD *) pdata));
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_INT:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%8.1i ", *((DWORD *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%8.8x ", *((DWORD *) pdata));
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_WORD:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%5.1i ", *((WORD *) pdata));
         if ((dsp_fmt == DSP_HEX) || (dsp_fmt == DSP_UNK))
            printf("0x%4.4x ", *((WORD *) pdata));
         pdata = (char *) (((WORD *) pdata) + 1);
         j++;
         break;
      case TID_SHORT:
         if (j > 7) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 8;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%5.1i ", *((short *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%4.4x ", *((short *) pdata));
         pdata = (char *) (((short *) pdata) + 1);
         j++;
         break;
      case TID_BYTE:
      case TID_STRUCT:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%4.i ", *((BYTE *) pdata));
         if ((dsp_fmt == DSP_HEX) || (dsp_fmt == DSP_UNK))
            printf("0x%2.2x ", *((BYTE *) pdata));
         pdata++;
         j++;
         break;
      case TID_SBYTE:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         if ((dsp_fmt == DSP_DEC) || (dsp_fmt == DSP_UNK))
            printf("%4.i ", *((BYTE *) pdata));
         if (dsp_fmt == DSP_HEX)
            printf("0x%2.2x ", *((BYTE *) pdata));
         pdata++;
         j++;
         break;
      case TID_BOOL:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         (*((BOOL *) pdata) != 0) ? printf("Y ") : printf("N ");
         pdata = (char *) (((DWORD *) pdata) + 1);
         j++;
         break;
      case TID_CHAR:
      case TID_STRING:
         if (j > 15) {
            printf("\n%4i-> ", i);
            j = 0;
            i += 16;
         }
         if (dsp_fmt == DSP_DEC)
            printf("%3.i ", *((BYTE *) pdata));
         if (dsp_fmt == DSP_ASC || (dsp_fmt == DSP_UNK))
            printf("%1.1s ", (char *) pdata);
         if (dsp_fmt == DSP_HEX)
            printf("0x%2.2x ", *((BYTE *) pdata));
         pdata++;
         j++;
         break;
      default:
         printf("bank type not supported (%d)\n", type);
         return;
         break;
      }
   }                            /* end of bank */
   printf("\n");
   return;
}

/*------------------------------------------------------------------*/
/*--END of MDSUPPORT.C----------------------------------------------*/
/*------------------------------------------------------------------*/

/**dox***************************************************************/
                   /** @} *//* end of mdsupportincludecode */

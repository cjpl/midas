/*  Copyright (c) 1993      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 *
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca         Tel: (604) 222-1047    Fax: (604) 222-1074
 *         amaudruz@triumf.ca                            Local:           6234
 * -----------------------------------------------------------------------------
   $Log$
   Revision 1.9  1999/01/18 17:42:59  pierre
   - Added lost cvs log flag

 *
 *  Description	: ybos.c : contains support for the YBOS structure.
 *              : YBOS is a 4bytes aligned structure. The FIRST element
 *                of a YBOS event is always the LRL (Logical Record Length)
 *                This element represent the event size in 4 bytes count!
 *            
 *                The event structure is the following 
 *                        Midas event        Ybos event
 *         pheader ->     EVENT_HEADER      EVENT_HEADER      
 *         pmbkh   ->     BANK_HEADER           LRL             <- plrl
 *                        BANK              YBOS_BANK_HEADER    <- pybk
 *
 *                pevent is used for yb_any_....() pointing to pheader for MIDAS
 *                                                 pointing to plrl    for YBOS
 *                All ybk_...() requires plrl as input pointer
 *                All  bk_...() requires pmbkh as input pointer
 *
 *                While replaying data, the EVENT_HEADER has been striped out
 *                from the event in the YBOS format. In this case the plrl is the
 *                first data of the event. NO MORE EVENT_HEADER is present. In order
 *                to provide anyway some evnt info, The FE could produce a EVID bank
 *                containing a "copy" of the EVENT_HEADER (see ybos_simfe.c)
 *                If the EVID is present in the YBOS event, mdump will try to recover 
 *                this info and display it.
 *
 *                function marked with * are externaly accessible
 *
 *   Section a)*: bank manipulation.
 *                ybk_init
 *                ybk_create, ybk_create_chaos
 *                ybk_close, ybk_close_chaos
 *                ybk_size, ybk_list, ybk_find, ybk_locate, ybk_iterate
 *   Section b) : mlogger functions.
 *                *ybos_log_open,      *ybos_write,      *ybos_log_close
 *                ybos_log_dump,       ybos_buffer_flush 
 *                ybos_logfile_close,  ybos_logfile_open, 
 *   Section c)   utilities (mdump, lazylogger, etc...)
 *                *yb_any_file_ropen,   *yb_any_file_rclose (uses my struct)
 *                *yb_any_file_wopen    *yb_any_file_wclose
 *                *yb_any_physrec_get:   ybos_physrec_get
 *                                       midas_physrec_get
 *                yb_any_dev_os_read,  yb_any_dev_os_write
 *                *yb_any_log_write
 *                *yb_any_physrec_skip:  ybos_physrec_skip
 *                *yb_any_physrec_display
 *                *yb_any_all_info_display
 *                *yb_any_event_swap:    ybos_event_swap
 *                *yb_any_event_get:     ybos_event_get
 *                                       midas_event_get
 *                *yb_any_event_display: yb_any_raw_event_display
 *                                       yb_any_bank_event_display
 *                *yb_any_bank_display:  yb_any_raw_bank_display
 *                                       ybos_bank_display
 *                                       midas_bank_display
 *   Section d)   File fragmentation and recovery
 *                *feodb_file_dump:    yb_file_fragment
 *                *yb_file_recompose : yb_ymfile_open
 *                                     yb_ymfile_update
 *
 *                gz not tested
 *                ftp channel not tested
 *
 *          online replay MIDAS YBOS NT UNIX TAPE DISK FTP largeEVT frag/recomp
 *                
/*---------------------------------------------------------------------------*/
/* include files */
#include "midas.h"
#include "msystem.h"

#ifdef INCLUDE_FTPLIB
#include "ftplib.h"
#endif

#ifdef INCLUDE_ZLIB
#include "zlib.h"
#endif

#define INCLUDE_LOGGING
#include "ybos.h"

/*---- Hidden prototypes ---------------------------------------------------*/
/* File fragmentation and recovery */
INT  yb_any_dev_os_read (INT handle, INT type, void * prec, DWORD nbytes, DWORD * nread);
INT  yb_any_dev_os_write(INT handle, INT type, void * prec, DWORD nbytes, DWORD * written);
INT  yb_ymfile_update(int slot, int fmt, void * pevt);
INT  yb_ymfile_open(int *slot, int fmt, void *pevt, char * svpath, INT file_mode);
INT  yb_file_fragment(EQUIPMENT * eqp, EVENT_HEADER *pevent, INT run_number, char * path);

INT  midas_event_skip (INT evtn);
INT  ybos_physrec_skip (INT bl);

INT  ybos_physrec_get (DWORD ** prec, DWORD * readn);
INT  midas_physrec_get (void ** prec, DWORD * readn);

void yb_any_bank_event_display(void * pevent, INT data_fmt, INT dsp_mode, INT dsp_fmt);
void yb_any_raw_event_display(void * pevent, INT data_fmt, INT dsp_fmt);

void yb_any_raw_bank_display(void * pbank, INT data_fmt, INT dsp_fmt);
void ybos_bank_display(YBOS_BANK_HEADER * pybk, INT dsp_fmt);
void midas_bank_display( BANK * pbk, INT dsp_fmt);

INT  ybos_event_get (INT bl, DWORD ** plrl, DWORD * size);
INT  midas_event_get (void ** pevent, DWORD * size);
INT  ybos_event_swap   (DWORD * pevt);

INT  ybos_buffer_flush(LOG_CHN *log_chn, INT run_number);
INT  ybos_logfile_open (INT type, char * path, HNDLE *handle);
INT  ybos_logfile_close(INT type, HNDLE handle);
void ybos_log_dump(LOG_CHN *log_chn, short int event_id, INT run_number);

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

#ifdef INCLUDE_FTPLIB
FTP_CON ftp_con;
#endif

/* magta stuff */
DWORD *pbot, *pbktop=NULL;
DWORD magta[3]={0x00000004, 0x544f422a, 0x00007ff8};

/* For Fragmentation */
R_YM_FILE      ymfile[MAX_YM_FILE];
struct stat    *filestat;

#ifdef INCLUDE_ZLIB
gzFile  filegz;
#endif

/* General YBOS/MIDAS struct for util */
struct {
  INT      handle;                      /* file handle */
  char     name[MAX_FILE_PATH];         /* Device name (/dev/nrmt0h) */

  char    *pmp;                         /* ptr to a physical TAPE_BUFFER_SIZE block */
  EVENT_HEADER *pmh;                     /* ptr to Midas event (midas bank_header)*/
  char    *pme;                         /* ptr to Midas content (event+1) (midas bank_header)*/
  char    *pmrd;                        /* current point in the phyical recoord */

  char    *pmagta;                      /* dummy zone for magta stuff */
  YBOS_PHYSREC_HEADER *pyh;             /* ptr to ybos physical block header */
  DWORD   *pylrl;                       /* ptr to ybos logical record */
  DWORD   *pyrd;                        /* ptr to current loc in physical record */

  DWORD    evtn;                        /* current event number */
  DWORD    serial;                      /* serial event number */
  DWORD    evtlen;                      /* current event length (-1 if not available) */
  DWORD    size;                        /* ybos block size or midas max_evt_size */
  DWORD    recn;                        /* ybos current physical record number */
  INT      fmt;                         /* contains FORMAT type */
  INT      type;                        /* Device type (tape, disk, ...) */
  DWORD    runn;                        /* run number */
  BOOL     zipfile;
  BOOL     magtafl;
  } my;

/*--BANK MANIPULATION-----Section a)--------------------------------*/
/*--BANK MANIPULATION-----------------------------------------------*/
/*--BANK MANIPULATION-----------------------------------------------*/
/*--BANK MANIPULATION-----------------------------------------------*/
/*--BANK MANIPULATION-----------------------------------------------*/
/*--BANK MANIPULATION-----------------------------------------------*/
/*------------------------------------------------------------------*/
INT bk_list (BANK_HEADER * pmbh, char * bklist)
/********************************************************************\
  Routine: bk_list
  Purpose: extract the MIDAS bank name listing of an event (pheader+1)
  Input:
    BANK_HEADER * pmbh      pointer to the bank header (pheader+1)
  Output:
    char *bklist            returned ASCII string
                            has to be booked with YB_STRING_BANKLIST_MAX
  Function value:
    INT nbk                 number of bank found in this event
\********************************************************************/
{ /* Full event */
  DWORD nbk, size;
  BANK * pmbk;
  char * pdata;
  pmbk = NULL;

  /* compose bank list */
  bklist[0] = 0;
  nbk=0;
  do
  {
    /* scan all banks for bank name only */
    size = bk_iterate(pmbh, &pmbk, &pdata);
    if (pmbk == NULL)
      break;
    nbk++;

    if (nbk> YB_BANKLIST_MAX)
    {
      cm_msg(MINFO,"bk_list","over %i banks -> truncated",YB_BANKLIST_MAX);
      return (nbk);
    }
    strncat (bklist,(char *) pmbk->name,4);
  }
  while (1);
  return (nbk);
}

/*------------------------------------------------------------------*/
INT bk_find (BANK_HEADER * pmbh, char * bkname, DWORD * bklen, DWORD * bktype, void **pbk)
/********************************************************************\
  Routine: bk_find
  Purpose: find the requested bank and return bank info
  
  Input:
    BANK_HEADER * pmbh     pointer to the Midas bank header (pheader+1)
    char  * bkname         bank name
  Output:
    DWORD * pbklen         data section length in bytes
    DWORD * pbktype        bank type
    void  ** pbk           pointer to the bank header (bank name)
  Function value:
    SS_SUCCESS
    SS_INVALID_NAME        bank not found
\********************************************************************/
{
  BANK * pmbk;
  char * pbkdata;

  pmbk = NULL;
  /* search for given bank */
  do
  {
    bk_iterate(pmbh, &pmbk, &pbkdata);
    if (pmbk == NULL)
      break;
    if (strncmp(bkname,(char *)(pmbk->name) ,4) == 0)
    {
      *bklen  = pmbk->data_size;
      *bktype = pmbk->type;
      *pbk    = pmbk;
      return SS_SUCCESS;
    }
  } while(1);
  return (SS_INVALID_NAME);
}
            
/*------------------------------------------------------------------*/
void  ybk_init(DWORD *plrl)
/********************************************************************\
  Routine: ybk_init
  Purpose: Prepare a YBOS event for a new YBOS bank structure
           reserve the Logical Record length (LRL)
           The plrl should ALWAYS remain to the same location.
           In this case it will point to the LRL.
  Input:
    DWORD * plrl    pointer to the top of the YBOS event.
  Output:
    none
  Function value:
    none
\********************************************************************/
{
  *plrl = 0;
  return;
}

/*------------------------------------------------------------------*/
static YBOS_BANK_HEADER *__pbkh;
void ybk_create(DWORD *pevt, char *bn, DWORD bt, void *pbkdat)
/********************************************************************\
  Routine: ybk_create
  Purpose: fills up the bank header and return the pointer to the 
           first user space data pointer.
  Input:
    DWORD * pevt          pointer to the top of the YBOS event (LRL)
    char  * bname         Bank name should be char*4
    DWORD   bt            Bank type can by either
                          I2_BKTYPE, I1_BKTYPE, I4_BKTYPE, F4_BKTYPE
  Output:
    void  *pbkdat         pointer to first valid data of the created bank
  Function value:
    none
\********************************************************************/
{
  __pbkh         = (YBOS_BANK_HEADER *) ( ((DWORD *)(pevt+1)) + (*(DWORD *)pevt) );
  __pbkh->name   = *((DWORD *)bn);
  __pbkh->number = 1;
  __pbkh->index  = 0;
  __pbkh->length = 0;
  __pbkh->type   = bt;
  *((DWORD **)pbkdat) = (DWORD *)(__pbkh + 1);
  return;
}

/*------------------------------------------------------------------*/
static DWORD *__pchaosi4;
void ybk_create_chaos(DWORD *pevt, char *bn, DWORD bt, void *pbkdat)
/********************************************************************\
  Routine: ybk_create
  Purpose: fills up the bank header,
           reserve the first 4bytes for the size of the bank in bt unit
           and return the pointer to the user space.
  Input:
    DWORD * pevt          pointer to the top of the YBOS event (LRL)
    char  * bname         Bank name should be char*4
    DWORD   bt            Bank type can by either
                          I2_BKTYPE, I1_BKTYPE, I4_BKTYPE, F4_BKTYPE
  Output:
    void    *pbkdat       pointer to first valid data of the created bank
  Function value:
    none
\********************************************************************/
{
  __pbkh         = (YBOS_BANK_HEADER *) ( (pevt+1) + (*pevt) );
  __pbkh->name   = *((DWORD *)bn);
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
void  ybk_close(DWORD *pevt, void *pbkdat)
/********************************************************************\
  Routine: ybk_close
  Purpose: patch the end of the event to the next 4 byte boundary,
           fills up the bank header, update the LRL (pevt)
  Input:
    DWORD * pevt          pointer to the top of the YBOS event (LRL).
    void    *pbkdat       pointer to first valid data of the created bank
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
  *pevt += __pbkh->length + (sizeof(YBOS_BANK_HEADER)/4) - 1;
  return;
}

/*------------------------------------------------------------------*/
void  ybk_close_chaos(DWORD *pevt, DWORD bt, void *pbkdat)
/********************************************************************\
  Routine: ybk_close_chaos
  Purpose: patch the end of the event to the next 4 byte boundary,
           fills up the bank header,
           compute the data size in bt unit.
           update the LRL (pevt)
  Input:
    DWORD * pevt          pointer to the top of the YBOS event (LRL).
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

  ybk_close(pevt, pbkdat);
}

/*------------------------------------------------------------------*/
INT ybk_size(DWORD *pevt)
/********************************************************************\
  Routine: ybk_init
  Purpose: Compute the overall byte size of the last closed YBOS event
  Input:
    DWORD * pevt    pointer to the top of the YBOS event.
  Output:
    none
  Function value:
    INT size in byte including the LRL
\********************************************************************/
{
  return (*((DWORD *)pevt) * 4 + 4);
}

/*------------------------------------------------------------------*/
INT   ybk_list (DWORD *plrl , char *bklist)
/********************************************************************\
  Routine: ybk_list
  Purpose: extract the YBOS bank name listing of a pointed event (->LRL)
  Input:
    DWORD * plrl            pointer to the YBOS event pointing to lrl
                            has to be booked with YB_STRING_BANKLIST_MAX
  Output:
    char *bklist            returned ASCII string
  Function value:
    YBOS_WRONG_BANK_TYPE    bank type unknown
    INT nbk                 number of bank found in this event
\********************************************************************/
{

  YBOS_BANK_HEADER  *pbk;
  DWORD    *pendevt, nbk;

  pbk = (YBOS_BANK_HEADER *) (plrl+1);
  
  /* end of event pointer skip LRL, point to YBOS_BANK_HEADER */
  pendevt = (DWORD *)pbk + *plrl;

  /* check if bank_type in range*/
  if (pbk->type >= MAX_BKTYPE)
    return (YB_WRONG_BANK_TYPE);

  /*init bank counter and returned string*/
  nbk = 0;
  bklist[0]=0;

  /* scan event */
  while ((DWORD *)pbk < pendevt)
    {
      /* update the number of bank counter */
      nbk++;

      if (nbk> YB_BANKLIST_MAX)
        {
          cm_msg(MINFO,"ybk_list","over %i banks -> truncated",YB_BANKLIST_MAX);
          return (nbk);
        }

      /* append ybos bank name to list */
      strncat (bklist,(char *) &(pbk->name),4);

      /* skip to next bank */
      pbk = (YBOS_BANK_HEADER *)(((DWORD *) pbk) + pbk->length + 4);
    }
  return(nbk);
}

/*------------------------------------------------------------------*/
INT ybk_find (DWORD *plrl, char * bkname, DWORD * bklen, DWORD * bktype, void **pbk)
/********************************************************************\
  Routine: ybk_find
  Purpose: find the requested bank and return the pointer to the
           top of the data section
  Input:
    DWORD * plrl           pointer to the YBOS event pointing to lrl
    char  * bkname         bank name
  Output:
    DWORD * pbklen         data section length in I*4
    DWORD * pbktype        bank type
    void  ** pbk           pointer to the bank name
  Function value:
    YB_SUCCESS
    YB_BANK_NOT_FOUND
    YB_WRONG_BANK_TYPE

\********************************************************************/
{
  YBOS_BANK_HEADER * pevt;
  DWORD    *pendevt;

  pevt = (YBOS_BANK_HEADER *) (plrl+1);
  
  /* end of event pointer skip LRL, point to YBOS_BANK_HEADER */
  pendevt = (DWORD *)pevt + *plrl;

  /* check if bank_type in range*/
  if (pevt->type >= MAX_BKTYPE)
    return (YB_WRONG_BANK_TYPE);

  /* init returned variables */
  *bklen  = 0;
  *bktype = 0;

  /* scan event */
  while ((DWORD *)pevt < pendevt)
  {
    /* check bank name */
    if (strncmp((char *) &(pevt->name), bkname, 4) == 0)
    { /* bank name match */
      /* extract bank length */
      *bklen = pevt->length - 1;  /* exclude bank type */

      /* extract bank type */
      *bktype = pevt->type;

      /* return point to bank name */
      *pbk = &pevt->name;
      return (YB_SUCCESS);
    }
    else
  	{
      /* skip to next bank */
      pevt = (YBOS_BANK_HEADER *)(((DWORD *) pevt) + pevt->length + 4);
	  }
  }
  return (YB_BANK_NOT_FOUND);
}

/*------------------------------------------------------------------*/
INT ybk_locate (DWORD *plrl, char * bkname, void *pdata)
/********************************************************************\
  Routine: ybk_locate
  Purpose: find the requested bank and return the pointer to the
           top of the data section
  Input:
    DWORD * plrl           pointer to the YBOS event pointing to lrl
    char  * bkname         bank name
  Output:
    DWORD * pbklen         data section length in I*4
    DWORD * pbktype        bank type
    void  * pdata          pointer to the data section
  Function value:
    YB_SUCCESS
    YB_BANK_NOT_FOUND
    YB_WRONG_BANK_TYPE

\********************************************************************/
{
  YBOS_BANK_HEADER * pybk;
  DWORD    *pendevt;

  pybk = (YBOS_BANK_HEADER *) (plrl+1);
  
  /* end of event pointer skip LRL, point to YBOS_BANK_HEADER */
  pendevt = (DWORD *)pybk + *plrl;

  /* check if bank_type in range*/
  if (pybk->type >= MAX_BKTYPE)
    return (YB_WRONG_BANK_TYPE);

  /* scan event */
  while ((DWORD *)pybk < pendevt)
  {
    /* check bank name */
    if (strncmp((char *) &(pybk->name), bkname, 4) == 0)
    { /* bank name match */
      /* extract bank length */

      /* return pointer to data section */
      *((void **)pdata) = pybk+1;
      return (YB_SUCCESS);
    }
    else
  	{
      /* skip to next bank */
      pybk = (YBOS_BANK_HEADER *)(((DWORD *) pybk) + pybk->length + 4);
	  }
  }
  return (YB_BANK_NOT_FOUND);
}

/*------------------------------------------------------------------*/
INT   ybk_iterate (DWORD *plrl, YBOS_BANK_HEADER ** pybkh , void ** pdata)
/********************************************************************\
  Routine: ybk_iterate
  Purpose: return the pointer to the bank name and data bank section
           of the current pointed event.
  Input:
  DWORD * plrl                   pointer to the YBOS event (at the LRL)
  Output:
  YBOS_BANK_HEADER ** pybkh      pointer to the top of the current bank
                                 if (pybkh == NULL) no more bank in event
  void ** pdata                pointer to the data bank section

  Function value:                data length in I*4
\********************************************************************/
{
  static int      len;
  static DWORD  * pendevt;
  static DWORD  * pybk;
  /*PAA char bname[5]; */
		
  /* the event may have several bank
     check if we have been already in here */
  if (*pybkh == NULL)
    {
      /* first time in (skip lrl) */
      *pybkh = (YBOS_BANK_HEADER *) (plrl+1);
						
      if ((*pybkh)->type > I1_BKTYPE)
								{
										*pybkh = *pdata = NULL;
										return (YB_WRONG_BANK_TYPE);
								}
						
      /* end of event pointer (+ lrl) */
      pendevt = plrl + *plrl;
						
      /* skip the EVID bank if present */
/*-PAA- kee it in for a little while Dec 17/98
      *((DWORD *)bname) = (*pybkh)->name;
      if (strncmp (bname,"EVID",4) == 0)
				{
					len = (*pybkh)->length;
					(YBOS_BANK_HEADER *)(*pybkh)++;
					pybk = (DWORD *) *pybkh;
					pybk += len - 1;
					*pybkh = (YBOS_BANK_HEADER *) pybk;
				}
*/
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
      *pdata = (void *) (*pybkh + 1);
						
      /* length always in I*4 due to YBOS -1 because type included in length !@# */
      return ((*pybkh)->length - 1);
    }
  else
    {
      /* no more bank in this event */
      *pybkh = *pdata = NULL;
      return (0);
    }
}

/*-- GENERAL file fragmentation and recovery -----Section d)--------*/
/*-- GENERAL file fragmentation and recovery -----------------------*/
/*-- GENERAL file fragmentation and recovery -----------------------*/
/*-- GENERAL file fragmentation and recovery -----------------------*/
/*------------------------------------------------------------------*/
INT feodb_file_dump (EQUIPMENT * eqp, char * eqpname, 
                     char * pevent, INT run_number, char *path)
/********************************************************************\
  Routine: feodb_file_dump
  Purpose: Access ODB for the /Equipment/<equip_name>/Dump.
           in order to scan for file name and dump the files content to the
           Midas buffer channel.
  Input:
    EQUIPMENT * eqp           Current equipment
    INT       run_number      current run_number
    char      * path          full file name specification
  Output:
    none
  Function value:
    0                      Successful completion
\********************************************************************/
{
  EQUIPMENT *peqp;
  INT      index, size, status;
  HNDLE    hDB, hKey, hKeydump;
  char     strpath[MAX_FILE_PATH], Dumpfile[MAX_FILE_PATH];
  char     odb_entry[MAX_FILE_PATH];
  BOOL     eqpfound = FALSE;

  cm_get_experiment_database(&hDB, &hKey);
  peqp = eqp;

  /* find the equipment info for this job */
  while (*((peqp->name), eqpname) != 0)
  {
    if (equal_ustring((peqp->name), eqpname))
    {
      eqpfound = TRUE;
      break;
    }
    peqp++;
  }
  if (!eqpfound)
   return -1;

  /* loop over all channels */
  sprintf (odb_entry,"/Equipment/%s/Dump",path);
  status = db_find_key(hDB, 0, odb_entry, &hKey);
  if (status != DB_SUCCESS)
    {
      cm_msg(MINFO, "ybos_odb_file_dump", "odb_access_file -I- %s not found", odb_entry);
      return YB_SUCCESS;
    }
  index = 0;
  while ((status = db_enum_key(hDB, hKey, index, &hKeydump))!= DB_NO_MORE_SUBKEYS)
    {
      if (status == DB_SUCCESS)
	    {
	      size = sizeof(strpath);
	      db_get_path(hDB, hKeydump, strpath, size);
	      db_get_value(hDB, 0, strpath, Dumpfile, &size, TID_STRING);
        yb_file_fragment(peqp, (EVENT_HEADER *)pevent, run_number, Dumpfile);
	    }
      index++;
    }
  return (YB_SUCCESS);
}

/*------------------------------------------------------------------*/
INT  yb_file_fragment(EQUIPMENT * eqp, EVENT_HEADER *pevent, INT run_number, char * path)
/********************************************************************\
  Routine: yb_file_fragment
  Purpose: Fragment file in order to send it through Midas.
           Compose an event of the form of:
           Midas_header[(YM_CFILE)(YM_PFILE)(YM_DFILE)]
      	   Specific for the fe for either format YBOS/MIDAS
  Input:
    EQUIPMENT * eqp        Current equipment
    INT   run_number       currrent run_number
    char * path            full file name specification
  Output:
    none
  Function value:
    YB_SUCCESS          Successful completion
    SS_FILE_ERROR       file access error
\********************************************************************/
  {
    INT dmpf, remaining;
    INT nread, filesize, nfrag;
    INT allheader_size;
    DWORD *pbuf, *pcfile, *pmy;
    YM_CFILE myc_fileh;
    YM_PFILE myp_fileh;
    int  send_sock, flag;

    /* check if file exists */
    /* Open for read (will fail if file does not exist) */
    if( (dmpf = open(path, O_RDONLY | O_BINARY, 0644 )) == -1 )
  	  {
	      cm_msg(MINFO,"ybos_file_fragment","File dump -Failure- on open file %s",path);
	      return SS_FILE_ERROR;
	    }

    /* get file size */
    filestat = malloc( sizeof(struct stat) );
    stat(path,filestat);
    filesize = filestat->st_size;
    free(filestat);
    cm_msg(MINFO,"ybos_file_fragment","Accessing File %s (%i)",path, filesize);

    /*-PAA-Oct06/97 added for ring buffer option */
    send_sock = rpc_get_send_sock();
  
    /* compute fragmentation & initialize*/
    nfrag = filesize / MAX_FRAG_SIZE;

    /* Generate a unique FILE ID */
    srand( (unsigned)time( NULL ) );
    srand( (unsigned)time( NULL ) );

    /* Fill file YM_CFILE header */
    myc_fileh.file_ID = rand();
    myc_fileh.size = filesize;
    myc_fileh.total_fragment = nfrag + (((filesize % MAX_FRAG_SIZE)==0) ? 0 : 1);
    myc_fileh.current_fragment = 0;
    myc_fileh.current_read_byte = 0;
    myc_fileh.run_number= run_number;
    myc_fileh.spare= 0x1234abcd;

    /* Fill file YM_PFILE header */
    memset (myp_fileh.path,0,sizeof(YM_PFILE));
    /* first remove path if present */
    if (strrchr(path,'/') != NULL)
      {
        strncpy(myp_fileh.path
	        ,strrchr(path,'/')+1
	        ,strlen(strrchr(path,'/')));
      }
    else
        strcpy(myp_fileh.path, path);

    /* allocate space */
    allheader_size = sizeof(EVENT_HEADER)
      + sizeof(YBOS_BANK_HEADER)           /* EVID bank header */
      + 5*sizeof(DWORD)                    /* EVID data size */
      + sizeof(YM_CFILE)
      + sizeof(YM_PFILE) + 64;

    flag = 0;
    pevent -= 1;

    /* read file */
    while (myc_fileh.current_fragment <= nfrag)
    {
      /* pevent passed by fe for first event only */
      if (flag)
        pevent = eb_get_pointer();
      flag = 1;

      /* bank header */
      pmy = (DWORD *) (pevent+1);                          
      
      /*-PAA-Oct06/97 for ring buffer reset the LRL */
      if (eqp->format == FORMAT_YBOS)
         ybk_init((DWORD *) pmy);                            
      else if (eqp->format == FORMAT_MIDAS)
         bk_init(pmy);

      /*---- EVID bank ----*/
      if (eqp->format == FORMAT_YBOS)
        { YBOS_EVID_BANK(pmy
              ,myc_fileh.current_fragment
              ,(eqp->info.event_id <<16) | (eqp->info.trigger_mask)
              ,eqp->serial_number
              ,run_number);}
      else if (eqp->format == FORMAT_MIDAS)
        { MIDAS_EVID_BANK(pmy
              ,myc_fileh.current_fragment
              ,(eqp->info.event_id <<16) | (eqp->info.trigger_mask)
              ,eqp->serial_number
              ,run_number);}
      
      /* Create Control file bank */
      if (eqp->format == FORMAT_YBOS)
        ybk_create(pmy, "CFIL", I4_BKTYPE, (DWORD *)&pbuf);        
      else if (eqp->format == FORMAT_MIDAS)
        bk_create(pmy, "CFIL", TID_DWORD, &pbuf);

      /* save pointer for later */
      pcfile = pbuf;
      (char *)pbuf += sizeof(YM_CFILE);                             
      if (eqp->format == FORMAT_YBOS)
        ybk_close(pmy, pbuf);
      else if (eqp->format == FORMAT_MIDAS)
        bk_close(pmy, pbuf);

      /* Create Path file name bank */
      if (eqp->format == FORMAT_YBOS)
        ybk_create(pmy, "PFIL", A1_BKTYPE, (DWORD *)&pbuf);       
      else if (eqp->format == FORMAT_MIDAS)
        bk_create(pmy, "PFIL", TID_CHAR, &pbuf);
      memcpy((char *)pbuf,(char *)&myp_fileh,sizeof(YM_PFILE));
      (char *)pbuf += sizeof(YM_PFILE);
      if (eqp->format == FORMAT_YBOS)
        ybk_close(pmy, pbuf);
      else if (eqp->format == FORMAT_MIDAS)
        bk_close(pmy, pbuf);

      /* file content */
      if (eqp->format == FORMAT_YBOS)
        ybk_create(pmy, "DFIL", A1_BKTYPE, (DWORD *)&pbuf);   
      else if (eqp->format == FORMAT_MIDAS)
        bk_create(pmy, "DFIL", TID_CHAR, (DWORD *)&pbuf);
      /* compute data length */
      remaining = filesize-myc_fileh.current_read_byte;
      nread = read(dmpf,(char *)pbuf,(remaining > MAX_FRAG_SIZE) ? MAX_FRAG_SIZE : remaining);
      /* adjust target pointer */
      (char *) pbuf += nread;
      /* keep track of statistic */
      myc_fileh.current_fragment++;
      myc_fileh.fragment_size = nread;
      myc_fileh.current_read_byte += nread;
      memcpy((char *)pcfile,(char *)&myc_fileh,sizeof(YM_CFILE));

      /* close YBOS bank */
      if (eqp->format == FORMAT_YBOS)
        ybk_close(pmy, pbuf);
      else if (eqp->format == FORMAT_MIDAS)
        bk_close(pmy, pbuf);

      /* Fill the Midas header */
      if (eqp->format == FORMAT_YBOS)
        bm_compose_event(pevent, eqp->info.event_id, eqp->info.trigger_mask
		       , ybk_size(pmy), eqp->serial_number++);
      else if (eqp->format == FORMAT_MIDAS)
        bm_compose_event(pevent, eqp->info.event_id, eqp->info.trigger_mask
		       , bk_size(pmy), eqp->serial_number++); 

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
        eqp->odb_out++; 
        }
    }
  /* close file */
  if( close( dmpf) )
    {
      cm_msg(MERROR, "fe_file_dump","cannot close file: %s", path);
      return SS_FILE_ERROR;
    }
  return YB_SUCCESS;
}

#if !defined (FE_YBOS_SUPPORT)
/*---- LOGGER YBOS format routines ----Section b)--------------------------*/
/*---- LOGGER YBOS format routines ----------------------------------------*/
/*---- LOGGER YBOS format routines ----------------------------------------*/
/*---- LOGGER YBOS format routines ----------------------------------------*/
/*---- LOGGER YBOS format routines ----------------------------------------*/

INT ybos_log_open(LOG_CHN *log_chn, INT run_number)
/********************************************************************\
  Routine: ybos_log_open, Should be used only by mlogger.
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
  if ( (ybos->ptop = (DWORD *) malloc(YBOS_BUFFER_SIZE)) == NULL)
    {
      log_chn->handle = 0;
      return SS_NO_MEMORY;
    }

  memset ((char *) ybos->ptop, 0, YBOS_BUFFER_SIZE);
  /* Setup YBOS pointers */
  ybos->reco  = YBOS_HEADER_LENGTH;
  ybos->pbuf  = ybos->ptop + YBOS_HEADER_LENGTH;
  ybos->pwrt  = ybos->pbuf;
  ybos->pbot  = ybos->ptop + YBOS_PHYREC_SIZE;
  ybos->pend  = ybos->ptop + YBOS_BUFFER_SIZE;
  ybos->recn  = 0;
  /* open logging device */
  status = ybos_logfile_open(log_chn->type, log_chn->path, &log_chn->handle); 
  if (status != SS_SUCCESS)
  {
    free(ybos->ptop);
    free(ybos);
    log_chn->handle = 0;
    return status;
  }
  
  /* write ODB dump */
  if (log_chn->settings.odb_dump)
    ybos_log_dump(log_chn, EVENTID_BOR, run_number);
 
  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ybos_logfile_open(INT type, char * path, HNDLE *handle)
/********************************************************************\
  Routine: ybos_logfile_open
  Purpose: Open a YBOS logging channel for either type (Disk/Tape)
           The device open is taken care here. But the writting is done
           through yb_any_dev_os_write for ybos magta.

  Input:
  INT type       : Disk, Tape
  char * path    : Device name

  Output:
  HNDLE * handle ; returned handle of the open device
    none
 Function value:
    error, success
\********************************************************************/
{
  INT   status;
  DWORD written;

  /* Create device channel */
  if (type == LOG_TYPE_TAPE)
  { 
    /*-PAA- Should check for the TAPE_BUFFER_SIZE set in ss_tape_open() */
    return ss_tape_open(path, O_WRONLY | O_CREAT | O_TRUNC, handle);
  }
  else if (type == LOG_TYPE_DISK)
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
    status = yb_any_dev_os_write(*handle, type, (char *) magta, 8, &written);
    if (status != SS_SUCCESS)
      return status;

    /* allocate temporary emtpy record */
    pbot = realloc (pbot, magta[2]-4);
    memset ((char *)pbot,0,magta[2]-4);
    /* write BOT empty record for MAGTA */
    status = yb_any_dev_os_write(*handle, type, (char *) pbot, magta[2]-4, &written);
    if (status != SS_SUCCESS)
      return status;
  }
  return YB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ybos_write(LOG_CHN *log_chn, EVENT_HEADER *pevent, INT evt_size)
/********************************************************************\
  Routine: ybos_write
  Purpose: Write a YBOS event to the logger channel. Should be used only by 
           mlogger.
           Takes care of the EVENT_BOR and EVENT_MESSAGE which are 
           shiped as YBOS bank in A1_BKTYPE bank. named respectively
           MODB, MMSG
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
  BOOL        large_evt;
  INT         status, left_over_length, datasize;
  YBOS_INFO   *ybos;
  DWORD       *pbkdat;
  DWORD        bfsize;
  YBOS_PHYSREC_HEADER *yb_phrh;

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
    { /* skip ASCII dump if not MUSER */
      if (!(evmsk & MT_USER))
      	return SS_SUCCESS;

      /* skip if MUSER  but not Log message enabled */
      if (MT_USER & !log_chn->settings.log_messages)
	      return SS_SUCCESS;

      /* ASCII event has to be recasted YBOS */
      /* Inform if event too long (>32Kbytes) */
      if (pevent->data_size > MAX_EVENT_SIZE)
	        cm_msg(MINFO,"ybos_write","MMSG or MODB event too large");

      /* align to DWORD boundary in bytes */
      datasize = 4 * (pevent->data_size + 3) / 4;

      /* overall buffer size in bytes */
      bfsize = datasize + sizeof(YBOS_BANK_HEADER) + 4;   /* +LRL */

      /* allocate space */
						if (pbktop == NULL)
								pbktop = malloc(bfsize);
      if (pbktop == NULL)
        {
          cm_msg(MERROR,"ybos_write","malloc error for ASCII dump");
          return SS_NO_MEMORY;
        }
      memset (pbktop, 0, bfsize);
      ybk_init(pbktop);

      /* open bank depending on event type */
      if (evid == EVENTID_MESSAGE)
      	ybk_create(pbktop, "MMSG", A1_BKTYPE, &pbkdat);
      else
      	ybk_create(pbktop, "MODB", A1_BKTYPE, &pbkdat);

      printf("pbktop:%p (%x)\n",pbktop, *pbktop);
      memcpy ((char *)pbkdat, (char *)(pevent+1), pevent->data_size);
      (char *) pbkdat += datasize;
      ybk_close(pbktop, pbkdat);
      
      printf("pbktop:%p (%x)\n",pbktop, *pbktop);
      printf("%x - %x-%x-%x - %x-%x-%x-%x\n",
	     *pbktop,*(pbktop+1),*(pbktop+2),*(pbktop+3),
	     *(pbktop+4),*(pbktop+5),*(pbktop+6),*(pbktop+7));

      /* event size in bytes for Midas */
      evt_size = ybk_size(pbktop);
      
      /* swap bytes if necessary based on the ybos.bank_type */
      ybos_event_swap((DWORD *)pbktop);
      printf("pbktop:%p (%x) [%x]\n",pbktop, *pbktop, evt_size);
      
      /* Event with MIDAS header striped out */
      memcpy( (char *)ybos->pbuf, (char *) pbktop, evt_size);

      if (pbktop != NULL)
								free (pbktop);
						pbktop = NULL;
      status = SS_SUCCESS;
    }
  else
    { /* normal event */
      /* Strip the event from the Midas EVENT_HEADER */
      /* event size include the Midas EVENT_HEADER... don't need for ybos
	     I do this in order to keep the log_write from mlogger intact */
      
      /* correct the event length. Now it is a pure YBOS event */
      pevent++;

      /* correct the event length in bytes */
      evt_size -= sizeof(EVENT_HEADER);

      /* swap bytes if necessary based on the ybos.bank_type */
      ybos_event_swap((DWORD *)pevent);
      
      /* I have ALWAYS enough space for the event <MAX_EVENT_SIZE */
      memcpy( (char *)ybos->pbuf, (char *) pevent, evt_size);

      status = YB_SUCCESS;
    }

  /* move write pointer to next free location (DWORD) */
  ybos->pbuf += (4 * (evt_size+3) / 4)>>2;

  /* default not a large event */
  large_evt = FALSE;

  /* Loop over buffer until this condition 
     The event offset in the phys rec is ==0 if event larger than PHYREC_SIZE */
  while (ybos->pbuf >= ybos->pbot)
    {
      ybos->pwrt             -= YBOS_HEADER_LENGTH;
      yb_phrh                = (YBOS_PHYSREC_HEADER *)(ybos->pwrt);
      yb_phrh->rec_size      = YBOS_PHYREC_SIZE-1;     /* exclusive */
      yb_phrh->header_length = YBOS_HEADER_LENGTH;
      yb_phrh->rec_num       = ybos->recn;
      yb_phrh->offset        = large_evt ? 0 : ybos->reco;

      /* Write physical record to device */
      status = yb_any_log_write(log_chn->handle, log_chn->format, log_chn->type
                    , ybos->pwrt, YBOS_PHYREC_SIZE<<2);
      if (status != SS_SUCCESS)
        return status;

      /* update statistics */
      if (log_chn->type == LOG_TYPE_TAPE)
        { /* statistics in bytes */
        log_chn->statistics.bytes_written += YBOS_PHYREC_SIZE<<2;
        log_chn->statistics.bytes_written_total += YBOS_PHYREC_SIZE<<2;
        }
      else
        { /* statistics in bytes + the extra magta */
          log_chn->statistics.bytes_written += YBOS_PHYREC_SIZE<<2 + 4;
          log_chn->statistics.bytes_written_total += YBOS_PHYREC_SIZE<<2 + 4;
        }

      /* Update statistics */
       ybos->recn++;
               
      /* check if event is larger than YBOS_PHYREC_SIZE */
      if (ybos->pbuf >= ybos->pbot+YBOS_PHYREC_SIZE)
        {
          large_evt = TRUE;
          /* shift record window by one YBOS_PHYSREC */
          ybos->pwrt  = ybos->pbot;
          ybos->pbot += YBOS_PHYREC_SIZE;
        }
      else
        {
          large_evt = FALSE;
          /* adjust pointers */
          ybos->pwrt            = ybos->ptop + YBOS_HEADER_LENGTH;
          left_over_length      = ybos->pbuf - ybos->pbot;
          memcpy(ybos->pwrt, ybos->pbot, left_over_length<<2);   /* in bytes */
          ybos->pbuf            = ybos->pwrt + left_over_length;
          ybos->pbot            = ybos->ptop + YBOS_PHYREC_SIZE;
          ybos->reco            = ybos->pbuf - ybos->pwrt + 4; /* YBOS header */
        }
    }

  /* update statistics */
  log_chn->statistics.events_written++;

  return status;
}

/*------------------------------------------------------------------*/
INT ybos_buffer_flush(LOG_CHN *log_chn, INT run_number)
/********************************************************************\
  Routine: ybos_buffer_flush
  Purpose: Empty the internal buffer to logger channel for YBOS fmt
           YBOS end of run marks (End of file) is -1 in the *plrl
           I'm writting an extra FULL YBOS_PHYSREC_SIZE of -1
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
YBOS_PHYSREC_HEADER *yb_phrh;

  ybos = (YBOS_INFO *) log_chn->format_info;

 /* dump the ODB if necessary */
  if (log_chn->settings.odb_dump)
    ybos_log_dump(log_chn, EVENTID_EOR, run_number);

/* adjust read pointer to beg of record */
  ybos->pwrt             -= YBOS_HEADER_LENGTH;
  yb_phrh                = (YBOS_PHYSREC_HEADER *)ybos->pwrt;
                      
  yb_phrh->rec_size      = YBOS_PHYREC_SIZE-1;       /* exclusive */
  yb_phrh->header_length = YBOS_HEADER_LENGTH;    /* inclusive */
  yb_phrh->rec_num       = ybos->recn;
  yb_phrh->offset        = ybos->reco;    /* exclusive from block_size */

/* YBOS known only about fix record size. The way to find out
   it there is no more valid event is to look at the LRL for -1
   put some extra -1 in the current physical record */
  memset((DWORD *)ybos->pbuf,-1,YBOS_PHYREC_SIZE<<2);

  /* write record to device */
  status = yb_any_log_write(log_chn->handle, log_chn->format, log_chn->type,
                        ybos->pwrt, YBOS_PHYREC_SIZE<<2);
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    log_chn->statistics.bytes_written += YBOS_PHYREC_SIZE<<2;
    log_chn->statistics.bytes_written_total += YBOS_PHYREC_SIZE<<2;
    }
  else
    {
    /* write MAGTA header (4bytes)=0x7ff8 */
    log_chn->statistics.bytes_written += YBOS_PHYREC_SIZE<<2 + 4;
    log_chn->statistics.bytes_written_total += YBOS_PHYREC_SIZE<<2 + 4;
    }
  return status;
}

/*------------------------------------------------------------------*/
INT ybos_logfile_close(INT type, HNDLE handle)
/********************************************************************\
  Routine: ybos_logfile_close
  Purpose: close a logging channel for either type (Disk/Tape)
           For the tape I'm writting just a EOF and expect the rewind command to 
           write another one if necessary. This way the run restart is faster.
  Input:
  INT type       : Disk, Tape
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

/*-PAA- Done by odb>rewind if enable with "extra eof before rewind"
    ss_tape_write_eof(handle);
    ss_tape_fskip(handle, -1);
*/
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
  return YB_SUCCESS;
}


/*------------------------------------------------------------------*/
INT ybos_log_close(LOG_CHN *log_chn, INT run_number)
/********************************************************************\
  Routine: ybos_log_close
  Purpose: Close a YBOS logger channel, Should be used only by mlogger.
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
  status = ybos_buffer_flush(log_chn, run_number);

  if (status != SS_SUCCESS)
    return status;

  status = ybos_logfile_close(log_chn->type, log_chn->handle);

  free(ybos->ptop);
  free(ybos);

  return SS_SUCCESS;
}

/*---- ODB   manipulation   ----------------------------------------*/
void ybos_log_dump(LOG_CHN *log_chn, short int event_id, INT run_number)
/********************************************************************\
  Routine: ybos_log_dump, used by mlogger, ybos_log_open
  Purpose: Serves the logger flag /logger/settings/ODB dump
           Extract the ODB in ASCII format and send it to the logger channel
           Compose a ybos bank in A1_BKTYPE regardless of the odb size.
           It uses ybos_write to compose the actual event. From here it looks
           like a MIDAS event.
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
        cm_msg(MERROR, "ybos_odb_log_dump", "Cannot allocate ODB dump buffer");
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
    free(pevent);
 }

/*-- GENERAL mdump functions for MIDAS / YBOS -----Section c)-------*/
/*-- GENERAL mdump functions for MIDAS / YBOS ----------------------*/
/*-- GENERAL mdump functions for MIDAS / YBOS ----------------------*/
/*-- GENERAL mdump functions for MIDAS / YBOS ----------------------*/
/*-- GENERAL mdump functions for MIDAS / YBOS ----------------------*/
/*------------------------------------------------------------------*/
INT   yb_any_file_ropen(char * infile, INT data_fmt)
/********************************************************************\
  Routine: external yb_any_file_ropen
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
  strcpy (my.name,infile);

  /* find out what dev it is ? : check on /dev */
  my.zipfile = FALSE;
  if ((strncmp(my.name,"/dev",4) == 0 )||
      (strncmp(my.name,"\\\\.\\",4) == 0 ))
    {
      /* tape device */
      my.type = LOG_TYPE_TAPE;
    }
  else
    {
      /* disk device */
      my.type = LOG_TYPE_DISK;
      if (strncmp(infile+strlen(infile)-3,".gz",3) == 0)
        my.zipfile = TRUE;
    }

  /* open file */
  if(!my.zipfile)
  {
    if (my.type == LOG_TYPE_TAPE)
    {
      status = ss_tape_open(my.name, O_RDONLY | O_BINARY, &my.handle);
    }
    else if ((my.handle = open(my.name, O_RDONLY | O_BINARY, 0644 )) == -1 )
    {
      printf("dev name :%s Handle:%d \n",my.name, my.handle);
      return(SS_FILE_ERROR);
    }
  }
  else 
  {
#ifdef INCLUDE_ZLIB
    if (my.type == LOG_TYPE_TAPE)
      {
        printf(" Zip on tape not yet supported \n");
        return (SS_FIEL_ERROR);
      }
    filegz    = gzopen(my.name, "rb");
    my.handle = 0;
    if (filegz == NULL)
    {
      printf("dev name :%s gzopen error:%d \n",my.name, my.handle);
      return(SS_FILE_ERROR);
    }
#else
    cm_msg(MERROR,"ybos.c","Zlib not included ... gz file not supported");
    return (SS_FILE_ERROR);
#endif
  }

  if (data_fmt == FORMAT_YBOS)
    {
      my.fmt           = FORMAT_YBOS;
      my.size          = YBOS_PHYREC_SIZE;    /* in DWORD  */
      if (my.pmagta == NULL)
								my.pmagta        = malloc(32);
						if (my.pmagta == NULL)
								return SS_NO_MEMORY;
      if (my.pyh == NULL)
								my.pyh         = (YBOS_PHYSREC_HEADER *) malloc(my.size * 4);
						if (my.pyh == NULL)
								return SS_NO_MEMORY;
      (my.pyh)->rec_size    = my.size - 1;
      (my.pyh)->header_length = YBOS_HEADER_LENGTH;
      (my.pyh)->rec_num       = 0;
      (my.pyh)->offset        = 0;
      /* current ptr in the physical record */
      my.pyrd               = (DWORD *)((DWORD *)my.pyh + (my.pyh)->offset);

      /* allocate memory for one full event */
						if (my.pylrl == NULL)
								my.pylrl  = (DWORD *) malloc (MAX_EVENT_SIZE);    /* in bytes */
						if (my.pylrl == NULL)
								return SS_NO_MEMORY;
      memset ((char *)my.pylrl, -1, MAX_EVENT_SIZE);

      /* reset first path */
      my.magtafl = FALSE;
    }
  else if (data_fmt == FORMAT_MIDAS)
    {
      my.fmt  = FORMAT_MIDAS;
      my.size = TAPE_BUFFER_SIZE;
						if (my.pmp == NULL)
								my.pmp = malloc( my.size);
						if (my.pmp == NULL)
								return SS_NO_MEMORY;
      my.pmrd = my.pmp;

      /* allocate memory for one full event */
						if (my.pmh == NULL)
								my.pmh = malloc(MAX_EVENT_SIZE);    /* in bytes */
						if (my.pmh == NULL)
								return SS_NO_MEMORY;
      memset ((char *)my.pmh, -1, MAX_EVENT_SIZE);
      my.pme  = (char *)(my.pmh + 1);
    }

  /* initialize pertinent variables */
  my.recn = (DWORD) -1;        /* physical record number */
  my.evtn    = 0;
  return (YB_SUCCESS);
}

/*------------------------------------------------------------------*/
INT   yb_any_file_rclose (INT data_fmt)
/********************************************************************\
  Routine: external yb_any_file_rclose
  Purpose: Close a data file used for replay for the given data format
  Input:
  INT data_fmt :  YBOS or MIDAS 
  Output:
    none
  Function value:
  status : from lower function
 /*******************************************************************/
{
  switch (my.type)
    {
    case LOG_TYPE_TAPE:
    case LOG_TYPE_DISK:
      /* close file */
      if (my.zipfile)
        {
#ifdef INCLUDE_ZLIB
        gzclose(my.name);
#endif   
        }
      else
        {
          if (my.handle != 0)
            close (my.handle);
        }
      break;
    }
		if (my.pmagta != NULL)
				free (my.pmagta);
		if (my.pyh != NULL)
				free (my.pyh);
		if (my.pylrl != NULL)
				free (my.pylrl);
		if (my.pmh != NULL)
				free (my.pmh);
		if (my.pmp != NULL)
				free (my.pmp);
  (void *)my.pyh = (void *)my.pmagta = (void *)my.pylrl = NULL;
		(void *)my.pmp = (void *)my.pmh = NULL;
  return(YB_SUCCESS);
}

#ifdef INCLUDE_FTPLIB
/* @ NOT TESTED @ */
INT yb_ftp_open(char *destination, FTP_CON **con)
{
INT   status;
short port;
char  *token, host_name[HOST_NAME_LENGTH], 
      user[32], pass[32], directory[256], file_name[256];

  /* 
  destination should have the form:
  host, port, user, password, directory, run%05d.mid
  */

  /* break destination in components */
  token = strtok(destination, ",");
  if (token)
    strcpy(host_name, token);

  token = strtok(NULL, ", ");
  if (token)
    port = atoi(token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(user, token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(pass, token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(directory, token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(file_name, token);

  status = ftp_login(con, host_name, port, user, pass, "");
  if (status >= 0)
    return status;

  status = ftp_chdir(*con, directory);
  if (status >= 0)
    return status;

  status = ftp_binary(*con);
  if (status >= 0)
    return status;

  if (ftp_open_write(*con, file_name) >= 0)
    return (*con)->err_no;

  return SS_SUCCESS;
}
/* @ NOT TESTED @ */
#endif

/*------------------------------------------------------------------*/
INT   yb_any_file_wopen (INT type, INT data_fmt, char * filename, INT * hDev)
/********************************************************************\
  Routine: external yb_any_file_wopen
  Purpose: Open a data file for the given data format
  Input:
  INT type     :  Tape or Disk
  INT data_fmt :  YBOS or MIDAS
  char * filename : file to open
  Output:
  INT * hDev      : file handle
  Function value:
  status : from lower function
 /*******************************************************************/
{
  INT status;

  if (data_fmt == FORMAT_YBOS)   
    /* takes care of TapeLX/NT under ss_tape_open , DiskLX/NT there */
    status = ybos_logfile_open(type, filename, hDev); 
  else if (data_fmt == FORMAT_MIDAS)
    {
      if (type == LOG_TYPE_DISK)
        /* takes care of TapeLX/NT under ss_tape_open , DiskLX/NT here */
        {
#ifdef OS_WINNT       
         *hDev = (int) CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | 
                     FILE_FLAG_SEQUENTIAL_SCAN, 0);
#else
         *hDev = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
#endif
        }
      else if (type == LOG_TYPE_TAPE)
        status = ss_tape_open(filename, O_WRONLY | O_CREAT | O_TRUNC, hDev);
      else if (type == LOG_TYPE_FTP)
        {

#ifdef INCLUDE_FTPLIB
          status = yb_ftp_open(filename,(FTP_CON **)&ftp_con);
          if (status != SS_SUCCESS)
            {
            *hDev = 0;
            return status;
            }
          else
            *hDev = 1;
#else
      cm_msg(MERROR,"ybos","FTP support not included");
      return SS_FILE_ERROR;
#endif
       }
    }
  if (*hDev < 0)
  {
    *hDev = 0;
    return SS_FILE_ERROR;
  }
  return status;
}

/*------------------------------------------------------------------*/
INT   yb_any_file_wclose (INT handle, INT type, INT data_fmt)
/********************************************************************\
  Routine: external yb_any_file_wclose
  Purpose: Close a data file used for replay for the given data format
  Input:
  INT data_fmt :  YBOS or MIDAS 
  Output:
    none
  Function value:
  status : from lower function
 /*******************************************************************/
{
  switch (type)
    {
    case LOG_TYPE_TAPE:
      /* writing EOF mark on tape Fonly */
      ss_tape_write_eof(handle);
      ss_tape_close(handle);
      break;
    case LOG_TYPE_DISK:
      /* close file */
      if (handle != 0)
#ifdef OS_WINNT
        CloseHandle((HANDLE)handle);
#else
        close(handle);
#endif
      break;
    case LOG_TYPE_FTP:
#ifdef INCLUDE_FTPLIB
      ftp_close(&ftp_con);
      ftp_bye(&ftp_con);
#endif
      break;
    }
  return(YB_SUCCESS);
}

/*------------------------------------------------------------------*/
INT  yb_any_dev_os_read(INT handle, INT type, void * prec, DWORD nbytes, DWORD *readn)
/********************************************************************\
  Routine: yb_any_dev_os_read
  Purpose: read nbytes from the type device.
  Input:
    INT  handle        file handler
    INT  type          Type of device (TAPE or DISK)
    void * prec        pointer to the record
    DWORD nbytes       # of bytes to read
  Output:
    DWORD *readn       # of bytes read
  Function value:
    YB_DONE            No more record to read
    YB_SUCCESS         Ok
\********************************************************************/
{
  INT   status;
  if (type == LOG_TYPE_DISK)
    /* --------- DISK ----------*/
    {
      *readn = read(handle, prec, nbytes);
      if (*readn <= 0)
	status = SS_FILE_ERROR;
      else 
	status = SS_SUCCESS;
      return status;
    }
  else if (type == LOG_TYPE_TAPE)
    /* --------- TAPE ----------*/
    {
#ifdef OS_UNIX
      *readn = read(handle, prec, nbytes);
      if (*readn <= 0)
	status = SS_FILE_ERROR;
      else 
	status = SS_SUCCESS;
      return status;
#endif
      
#ifdef OS_WINNT
      if (!ReadFile((HANDLE)handle, prec, nbytes, readn, NULL))
      	status = GetLastError();
      else
      	status = SS_SUCCESS;
      
      if (status == ERROR_NO_DATA_DETECTED)
	      status = SS_END_OF_TAPE;
      
      return status;
#endif /* OS_WINNT */
    }
}

/*------------------------------------------------------------------*/
INT  yb_any_dev_os_write(INT handle, INT type, void * prec, DWORD nbytes, DWORD *written)
/********************************************************************\
  Routine: yb_any_dev_os_write
  Purpose: write nbytes to the device. This function is YBOS independent
           (NO magta stuff for disk)
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
    { /* --------- DISK ----------*/
#ifdef OS_WINNT
      WriteFile((HANDLE) handle, (char *) prec, nbytes, written, NULL);
      status = *written == nbytes ? SS_SUCCESS : SS_FILE_ERROR;
#else
      status = *written = 
        write(handle, (char *)prec, nbytes) == nbytes ? SS_SUCCESS : SS_FILE_ERROR;
#endif
      return status;       /* return for DISK */
    }
  else if (type == LOG_TYPE_TAPE)
    { /* --------- TAPE ----------*/
#ifdef OS_UNIX
      do
      {
        status = write(handle, (char *)prec, nbytes);
      } while (status == -1 && errno == EINTR);
      *written = status;
      if (*written != nbytes)
      {
        cm_msg(MERROR, "any_dev_os_write", strerror(errno));
        if (errno == EIO)
          return SS_IO_ERROR;
        else
          return SS_TAPE_ERROR;
      }
#endif /* OS_UNIX */

#ifdef OS_WINNT
    WriteFile((HANDLE) handle, (char *)prec, nbytes, written, NULL);
    if (*written != nbytes)
      {
      status = GetLastError();
      cm_msg(MERROR, "any_dev_os_write", "error %d", status);
      return SS_IO_ERROR;
      }
#endif /* OS_WINNT */
      return SS_SUCCESS;      /* return for TAPE */
    }
  else if (type == LOG_TYPE_FTP)
    {
#ifdef INCLUDE_FTPLIB
      (int)written = (int)status = ftp_send(ftp_con.sock, (char *)prec, (int)nbytes) == (int)nbytes ?
                        SS_SUCCESS : SS_FILE_ERROR;
      return status;
#else
      cm_msg(MERROR,"ybos","FTP support not included");
      return SS_IO_ERROR;
#endif
    }
}

/*------------------------------------------------------------------*/
INT  yb_any_physrec_get (INT data_fmt, void ** precord, DWORD * readn)
/********************************************************************\
  Routine: external yb_any_physrec_get
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
  *precord = NULL;
  if (data_fmt == FORMAT_MIDAS)
    return midas_physrec_get(precord, readn);
  else if (data_fmt == FORMAT_YBOS)
    return ybos_physrec_get((DWORD **) precord, readn);
}
/*------------------------------------------------------------------*/
INT   ybos_physrec_get (DWORD ** precord, DWORD * readn)
/********************************************************************\
  Routine: ybos_physrec_get
  Purpose: read one physical YBOS record.
           The number of bytes to be read is fixed under YBOS.
           It is defined by my.size, In case of Disk file the magta
           stuff has to be read/rejected first.
  Input:
    void ** precord     pointer to the record
    DWORD * readn       record length in bytes
  Output:
    none
  Function value:
    YB_DONE            No more record to read
    YB_SUCCESS         Ok
\********************************************************************/
{
  INT   status;

  if (my.magtafl)
  {
    /* skip 4 bytes from MAGTA header */
    if (!my.zipfile)
    {
      status = yb_any_dev_os_read(my.handle, my.type, my.pmagta, 4, readn);
      if (status != SS_SUCCESS)
	      return (YB_DONE);
    }
    else
    { /* --------- GZIP ----------*/
#ifdef INCLUDE_ZLIB
      status = gzread(filegz, (char *)my.pmagta,4);
      if (status <= 0)
        return (YB_DONE);
#endif
    }
  }

  /* read full YBOS physical record */
  if (!my.zipfile)
  {
    status = yb_any_dev_os_read(my.handle, my.type, my.pyh, my.size<<2, readn);
    if (status != SS_SUCCESS)
     return (YB_DONE);
  }
  else
  {
#ifdef INCLUDE_ZLIB
    status = gzread(filegz, (char *)my.pyh, my.size<<2);
    if (status <= 0)
      return (YB_DONE);
#endif
  }

  /* check if header make sense for MAGTA there is extra stuff to get rid off */
  if ((!my.magtafl) && (*((DWORD *)my.pyh) == 0x00000004))
  {
    /* set MAGTA flag */
    my.magtafl = TRUE;
    /* BOT of MAGTA skip record */
    if (!my.zipfile)
    {
      status = yb_any_dev_os_read(my.handle, my.type, my.pmagta, 8, readn);
      if (status != SS_SUCCESS)
       return (YB_DONE);
    }
    else
    {
#ifdef INCLUDE_ZLIB
    status = gzread(filegz, (char *)my.pmagta, 8);
      if (status <= 0)
        return (YB_DONE);
#endif
    }

    /* read full YBOS physical record */
    if (!my.zipfile)
    {
      status = yb_any_dev_os_read(my.handle, my.type, my.pyh, my.size<<2, readn);
      if (status != SS_SUCCESS)
       return (YB_DONE);
    }
    else
    {
#ifdef INCLUDE_ZLIB
      status = gzread(filegz, (char *)my.pyh, my.size<<2);
      if (status <= 0)
        return (YB_DONE);
#endif
    }
  }

  /* move current ptr of newly phys rec to first event */
  if ( (my.pyh)->offset == 0)
    {
     /* no new event ==> full phys rec is continuation of the previous event
        leave pointer to continuation */
      my.pyrd = (DWORD *)my.pyh + (my.pyh)->offset;
    }
  else
    {
     /* new event in physical record 
        leave pointer to plrl */
      my.pyrd = (DWORD *)my.pyh + (my.pyh)->offset;
    }
  /* count blocks */
  my.recn++;

  *precord = (DWORD *)(my.pyh);

  return(YB_SUCCESS);
}

/*------------------------------------------------------------------*/
INT   midas_physrec_get (void ** precord, DWORD *readn)
/********************************************************************\
  Routine: midas_physrec_get
  Purpose: read one physical record.from a MIDAS run
           This is a "fake" physical record as Midas is
           not block structured. This function is used for
           reading a my.size record size. The return readn if different
           then my.size, will indicate a end of run. An extra read will
           indicate an eof.

  Input:
    void ** precord     pointer to the record
  Output:
    DWORD *readn        retrieve number of bytes
  Function value:
    YB_DONE            No more record to read
    YB_SUCCESS         Ok
\********************************************************************/
{
  INT   status;

  /* read one block of data */
  if (!my.zipfile)
  {
    status = yb_any_dev_os_read(my.handle, my.type, my.pmp, my.size, readn);
  }
  else
  {
#ifdef INCLUDE_ZLIB
    readn = gzread(filegz, (char *)my.pmp, my.size);
#endif
  }

  *precord = my.pmp;
  if (status != SS_SUCCESS)
    {
      return(YB_DONE);
    }
  else
    {  
      /* count blocks */
      my.recn++;
      return(YB_SUCCESS);
    }
}

/*------------------------------------------------------------------*/
INT   yb_any_log_write (INT handle, INT data_fmt, INT type, void * prec, DWORD nbytes)
/********************************************************************\
  Routine: external yb_any_log_write
  Purpose: Write a physical record to the out device, takes care of the
           magta under YBOS.
           
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

  if ((type == LOG_TYPE_DISK) && (data_fmt == FORMAT_YBOS))
    { /* add the magta record if going to disk */
      status = yb_any_dev_os_write(handle, type, (char *) ((DWORD *)(magta+2)), 4, &written);
      if (status != SS_SUCCESS)
        return status;
    }
  /* write record */
  status = yb_any_dev_os_write(handle, type, prec, nbytes, &written);
  if (status != SS_SUCCESS)
    return status;
}

/*------------------------------------------------------------------*/
INT   yb_any_physrec_skip (INT data_fmt, INT bl)
/********************************************************************\
  Routine: external yb_any_physrec_skip
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

  if (data_fmt == FORMAT_MIDAS)
    {
      status = midas_event_skip(bl);
      return YB_SUCCESS;
    }
  else if (data_fmt == FORMAT_YBOS)
    return ybos_physrec_skip(bl);
}

/*------------------------------------------------------------------*/
INT   ybos_physrec_skip (INT bl)
/********************************************************************\
  Routine: ybos_physrec_skip
  Purpose: skip physical record on a YBOS file.
           The physical record size is fixed (see ybos.h)
  Input:
    INT     bl            physical record number. (start at 0)
                          if bl = -1 : skip skiping
  Output:
    none
  Function value:
    YB_SUCCESS        Ok
\********************************************************************/
{
  INT status;
  DWORD *prec, size;

  if (bl == -1)
  {
    if((status = ybos_physrec_get(&prec, &size)) == YB_SUCCESS)
      return status;
  }
  while (ybos_physrec_get(&prec, &size) == YB_SUCCESS)
  {
    if((INT) (my.pyh)->rec_num != bl)
      {
	      printf("Skipping physical record_# ... ");
	      printf("%d \r",(my.pyh)->rec_num);
	      fflush (stdout);
	    }
     else
     {
	      printf ("\n");
        return YB_SUCCESS;
     }
  }
  return YB_DONE;
}

/*------------------------------------------------------------------*/
INT   midas_event_skip (INT evtn)
/********************************************************************\
  Routine: midas_event_skip
  Purpose: skip event on a MIDAS file.
  Input:
    INT     evtn          event record number. (start at 0)
                          if evt = -1 : skip skiping
  Output:
    none
  Function value:
    YB_SUCCESS        Ok
\********************************************************************/
{
  void * pevent;
  DWORD size;

  size = MAX_EVENT_SIZE;
  if (evtn == -1)
  {
    if(midas_event_get(&pevent, &size) == YB_SUCCESS)
    return YB_SUCCESS;
  }
  while (midas_event_get(&pevent, &size) == YB_SUCCESS)
  {
    if((INT) my.evtn < evtn)
      {
	      printf("Skipping event_# ... ");
	      printf("%d \r",my.evtn);
	      fflush (stdout);
	    }
     else
     {
	      printf ("\n");
        return YB_SUCCESS;
     }
  }
  return YB_DONE;
}


/*------------------------------------------------------------------*/
INT yb_any_physrec_display (INT data_fmt, INT dsp_fmt)
/********************************************************************\
  Routine: external yb_any_physrec_display
  Purpose: Display the physical record of the current record 
           for the given data format.
           Not possible for MIDAS as no physical record structure
  Input:
    INT data_fmt :  YBOS or MIDAS 
    INT dsp_fmt:    not used for now
  Output:
    none
  Function value:
    status          Lower function
\********************************************************************/
{
  INT bz, j, i, k;
  DWORD *prec;

  if (data_fmt == FORMAT_MIDAS)
    {
      printf(">>> No physical record structure for Midas format <<<\n");
      return YB_DONE;
    }
  else if (data_fmt == FORMAT_YBOS)
    {
      yb_any_all_info_display (D_RECORD);
      bz  = (my.pyh)->rec_size + 1;
      /* adjust local pointer to top of record to include record header */
      prec =(DWORD *) (my.pyh);
      k = (my.pyh)->rec_num;
      for (i=0;i<bz; i+=NLINE)
        {
          printf ("R(%d)[%d] = ",k,i);
          for (j=0;j<NLINE;j++)
          {
            if (i+j < bz)
            {
              printf ("%8.8x ",*prec);
              prec++;
            }
          }
          printf ("\n");
        }
        return(YB_SUCCESS);
    }
}

/*------------------------------------------------------------------*/
INT yb_any_all_info_display (INT what)
/********************************************************************\
  Routine: yb_any_all_info_display
  Purpose: display on screen all the info about "what".
  Input:
    INT     what              type of display.
  Output:
    none
  Function value:
    INT                 YB_SUCCESS
                        YB_DONE
\********************************************************************/
{
  if (my.fmt == FORMAT_YBOS)
    {
      DWORD bz,hyl,ybn,of;

      bz  = (my.pyh)->rec_size;
      hyl = (my.pyh)->header_length;
      ybn = (my.pyh)->rec_num;
      of  = (my.pyh)->offset;
      switch (what)
	    {
	      case D_RECORD:
	      case D_HEADER:
	        printf ("rec#%d- ",my.recn);
	        printf ("%5dbz %5dhyl %5dybn %5dof\n",bz,hyl,ybn,of);
	        break;
	      case D_EVTLEN:
	        printf ("rec#%d- ",my.recn);
	        printf ("%5dbz %5dhyl %5dybn %5dof ",bz,hyl,ybn,of);
	        printf ("%5del/x%x %5dev\n",my.evtlen,my.evtlen,my.evtn);
	        break;
	    }
    }
  else if (my.fmt == FORMAT_MIDAS)
  {
    DWORD mbn,run;
    WORD  id, msk;
    mbn = my.evtn;
    run = my.runn;
    id  = my.pmh->event_id;
    msk = my.pmh->trigger_mask;
    switch (what)
	  {
	  case D_RECORD:
    case D_HEADER:
      printf(">>> No physical record structure for Midas format <<<\n");
      return YB_DONE;
      break;
	  case D_EVTLEN:
	    printf("Evt#%d- ",my.evtn);
      printf("%irun 0x%4.4xid 0x%4.4xmsk %5dmevt#",run, id, msk,mbn);
	    printf("%5del/x%x %5dserial\n",my.evtlen,my.evtlen,my.serial);
	    break;
	  }
  }
  return YB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT   yb_any_event_swap (INT data_fmt, void * pevent)
/********************************************************************\
  Routine: external yb_any_event_swap
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
  if (data_fmt == FORMAT_MIDAS)
  {
    if ((((EVENT_HEADER *)pevent)->event_id == EVENTID_BOR) ||
       (((EVENT_HEADER *)pevent)->event_id == EVENTID_EOR) ||
       (((EVENT_HEADER *)pevent)->event_id == EVENTID_MESSAGE))
       return SS_SUCCESS;
    /* bk_function needc the (BANK_HEADER *)pointer */
    return status=bk_swap(((EVENT_HEADER *)pevent)+1, FALSE) == 0 ? YB_SUCCESS : status;
  }
  else if (data_fmt == FORMAT_YBOS)
   return status=ybos_event_swap ((DWORD *)pevent) == YB_EVENT_NOT_SWAPPED ? YB_SUCCESS : status;
}

/*------------------------------------------------------------------*/
INT ybos_event_swap (DWORD * plrl)
/********************************************************************\
  Routine: ybos_event_swap
  Purpose: byte swap the entire YBOS event if necessary.
           chekc necessity of swapping by looking at the 
           bank type being < MAX_BKTYPE 
  Input:
    DWORD * plrl           pointer to the YBOS event
  Output:
    none
  Function value:
    YB_SUCCESS             Event has been swapped
    YB_EVENT_NOT_SWAPPED   Event has been not been swapped
    YB_SWAP_ERROR          swapping error
\********************************************************************/
{
  DWORD    *pevt, *pnextb, *pendevt;
  DWORD     bank_length, bank_type;

  /* check if event has to be swapped */
  if ((((YBOS_BANK_HEADER *)(plrl+1))->type) < MAX_BKTYPE)
    return (YB_EVENT_NOT_SWAPPED);

  /* swap LRL */
  DWORD_SWAP (plrl);
  pevt = plrl+1;

  /* end of event pointer */
  pendevt = pevt + *plrl;

  /* scan event */
  while (pevt < pendevt)
    {
      /* swap YBOS bank header for sure */
      /* bank name doesn't have to be swapped as it's an ASCII coded */
      pevt++;                                /* bank name */

      DWORD_SWAP(pevt);                      /* bank number */
      pevt++;

      DWORD_SWAP(pevt);                      /* bank index */
      pevt++;

      DWORD_SWAP(pevt);                      /* bank length */
      bank_length = *pevt++;

      DWORD_SWAP(pevt);                      /* bank type */
      bank_type   = *pevt++;

      /* pevt left pointing at first data in bank */

      /* pointer to next bank (-1 due to type inclided in length #$%@ */
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
          return (YB_SWAP_ERROR);
	        break;
	    }
  }
  return (YB_SUCCESS);
}

/*------------------------------------------------------------------*/
INT   yb_any_event_get (INT data_fmt, INT bl, void ** pevent, DWORD * readn)
/********************************************************************\
  Routine: external yb_any_event_get
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
  INT status;

  *pevent = NULL;
  if (data_fmt == FORMAT_MIDAS)
    status = midas_event_get(pevent, readn);
  else if (data_fmt == FORMAT_YBOS)
    status = ybos_event_get (bl, (DWORD **)pevent, readn);
  return(status);
}

/*------------------------------------------------------------------*/
INT   ybos_event_get (INT bl, DWORD ** plrl, DWORD * readn)
/********************************************************************\
  Routine: ybos_event_get
  Purpose: read one YBOS event.
           detect the end of run by checking the *plrl content (-1)
  Input:
    INT    bl            physical record number
                         != -1 stop at the end of the current physical record
  Output:
    DWORD ** plrl      points to LRL valid full YBOS event
    DWORD * readn      event size in Bytes 
  Function value:
    YB_DONE           No more record to read
    YB_SUCCESS        Ok
\********************************************************************/
{
  DWORD   size, fpart, lpart, evt_length;
  DWORD  *ptmp, *prec;
  INT     status;
  
  
  /* detect end of run (no more events) 
     by checking the *pyrd == -1 */
  if ((INT)(*my.pyrd) == -1)
    return YB_DONE;
  /* extract event to local event area
     event may not be complete if larger then physical record size
     will be taken care below, ADD the lrl  */
  evt_length = *(my.pyrd) + 1;
  memcpy((char *)my.pylrl, (char *)my.pyrd, evt_length<<2);

  /* extract lrl in I*4 and include itself (lrl) */

  /* stop if LRL  = -1 ... I don't think it is necessary but I leave it in for now
     or forever... */
  if (evt_length - 1 == -1)
    return (YB_DONE);

  /* check if event cross physical record boundary */
  if ((my.pyrd + evt_length) >= (DWORD *)my.pyh + my.size)
    {
      /* check if request of event should be terminate due to bl != -1 */
/*-PAA- It's better to skip and go til end of run instead of display only 
        one physical record and leave 
      if (bl != -1)
	      return (YB_DONE);
*/
      /* upcomming event crosses block, then first copy first part of event */
      /* compute max copy for first part of event */
      fpart = (DWORD *)my.pyh + my.size - my.pyrd;
      lpart = evt_length - fpart;

      memcpy ((char *)my.pylrl, (char *)my.pyrd, fpart<<2);

      /* adjust temporary evt pointer all in I*4 */
      ptmp = my.pylrl + fpart;

      /* get next physical record */
      if ((status=ybos_physrec_get (&prec, &size)) != YB_SUCCESS)
	      return (status);

      /* pyrd is left at the next lrl but here we comming from
         a cross boundary request so read just the pyrd to 
         pyh+header_length */
      my.pyrd = (DWORD *)my.pyh + my.pyh->header_length;
      /* now copy remaining from temporary pointer */
      memcpy ((char *)ptmp, (char *)my.pyrd, lpart<<2);

      /* adjust pointer to next valid data (LRL) 
         should be equivalent to pyh+pyh->offset */
      my.pyrd += lpart;
      if ( my.pyrd !=  (DWORD *)my.pyh + my.pyh->offset)
        printf(" event misalignment !!\n");
    }
  else
    {
      /* adjust pointer to next valid data (LRL) */
      my.pyrd += evt_length;
    }
  /* count event */
   my.evtn++;

  /* adjust event size in I*4 */
   my.evtlen = evt_length-4;
   /* in bytes for the world */
   *readn = my.evtlen<<2;
   *plrl = (DWORD *)my.pylrl;
   return(YB_SUCCESS);
}

/*------------------------------------------------------------------*/
INT   midas_event_get (void ** pevent, DWORD * readn)
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
    YB_DONE           No more record to read
    YB_SUCCESS        Ok
\********************************************************************/
{
  static char *pbeyond;
  static BOOL last_physrec = FALSE;
  BOOL        last_evt = FALSE;
  INT x_boundary;
  INT status;
  DWORD full_evt_length, fpart, lpart, size;
  
  if (my.recn == -1)
    {
      /* first time in get physrec once */
      status = midas_physrec_get((void *)&my.pmp, &size);
      if (status != YB_SUCCESS)
        return (YB_DONE);
    }

  /* Check if enough space until the end of the phys record */
  fpart = my.pmp + my.size - my.pmrd;

  if (fpart < sizeof(EVENT_HEADER))
    {
      /* no space even for a header */
      x_boundary = 2;
    }
  else
    {
      /* enough for at least a header, check for full event */
      full_evt_length = sizeof(EVENT_HEADER) + ((EVENT_HEADER *)my.pmrd)->data_size;

      /* check if last event: should always come here on the last event */
      if (last_physrec && (pbeyond <= my.pmrd + full_evt_length))
         last_evt = TRUE;
      if ((my.pmrd + full_evt_length) > (my.pmp + my.size))
        {
          /* event cross boundary */
          x_boundary = 0;
        }
      else
        {
          /* full event in the current phys rec */
          x_boundary = 1;  
         }
     }
  
  /* recompose the event */
  if (x_boundary == 1)
    {
      /* full event in phys rec */
      memcpy (my.pmh, my.pmrd, full_evt_length);
      my.pmrd += full_evt_length;
    }
  else      
    { /* event cross boundary */
      /* copy first part of the event */
      memcpy ((char *)my.pmh, my.pmrd, fpart);
      /* move pointer to end of first part */
      my.pme = (char *)my.pmh + fpart;
      
      /* retrieve the next physical record */
      if (!last_physrec)
        {
          status = midas_physrec_get((void *)&my.pmp, &size);
          if ((status != YB_SUCCESS) || (size != my.size))
            {
              last_physrec = TRUE;
              pbeyond = my.pmp + size; 
            }
        }
      else
        {
          /* it over no more events */
        }
      /* reset current source pointer */
      my.pmrd = my.pmp;

      /* copy left over part of th event */
      if (x_boundary == 2)
        {
          /* complete at left over of the header */
          fpart =  sizeof(EVENT_HEADER) - fpart;
          memcpy (my.pme, my.pmrd, fpart);
          /* move destination pointer */
          my.pme  += fpart;
          /* move source pointer */
          my.pmrd += fpart;

          full_evt_length = my.pmh->data_size + sizeof(EVENT_HEADER);

          /* correct first part length */
          lpart = full_evt_length - sizeof(EVENT_HEADER);
          memcpy (my.pme, my.pmrd, lpart);

          /* move source pointer */
          my.pmrd += lpart;
        }
      else
        {
          full_evt_length = my.pmh->data_size + sizeof(EVENT_HEADER);

          lpart = full_evt_length - fpart;
          memcpy (my.pme, my.pmrd, lpart);

          /* move source pointer */
          my.pmrd += lpart;
        }
    }
  
 *pevent = (char *)(my.pmh);
 *readn = my.evtlen = full_evt_length;

 /* count event */
 my.evtn++;

 /* count events or run number */
 if (my.pmh->event_id == EVENTID_BOR)
   my.runn = my.pmh->serial_number;
 else if (my.pmh->event_id == EVENTID_EOR)
   my.runn = my.pmh->serial_number;
 else
   my.serial = my.pmh->serial_number;

if (last_evt)
 return (YB_DONE);
return(YB_SUCCESS);
}

/*------------------------------------------------------------------*/
void yb_any_event_display(void * pevent, INT data_fmt, INT dsp_mode, INT dsp_fmt)
/********************************************************************\
  Routine: external yb_any_event_display
  Purpose: display on screen the YBOS event in either RAW or YBOS mode
           and in either Decimal or Hexadecimal.
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
     yb_any_raw_event_display(pevent, data_fmt, dsp_fmt);
  else if (dsp_mode == DSP_BANK)
     yb_any_bank_event_display(pevent, data_fmt, dsp_mode, dsp_fmt);
  else
    printf("yb_any_event_display- Unknown format:%i\n",dsp_fmt);
  return;
}

/*------------------------------------------------------------------*/
void yb_any_raw_event_display(void * pevent, INT data_fmt, INT dsp_fmt)
/********************************************************************\
  Routine: ybos_raw_event_display
  Purpose: display on screen the RAW data of either YBOS or MIDAS format.
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
  DWORD lrl, *pevt, j, i, total=0;

  if (data_fmt == FORMAT_YBOS)
  {
    lrl = *((DWORD *) (pevent)) + 1;    /* include itself */
    pevt = (DWORD *)pevent;             /* local copy starting from the plrl */
  }
  else if (data_fmt == FORMAT_MIDAS)
  {
    lrl = (((EVENT_HEADER *)pevent)->data_size)/sizeof(DWORD) 
          + sizeof(EVENT_HEADER);  /* in I*4 for raw including the header */
    pevt = (DWORD *)pevent;              /* local copy starting from the pheader */
  }

  for (i=0; i<lrl; i+=NLINE)
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
void yb_any_bank_event_display( void * pevent, INT data_fmt, INT dsp_mode, INT dsp_fmt)
/********************************************************************\
  Routine: ybos_bank_event_display
  Purpose: display on screen the event header, bank list and bank content
           for either ybos or midas format. In case of ybos check is EVID is 
           present if so extract its content (see macro ybos.h)
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
  char  banklist[YB_STRING_BANKLIST_MAX];
  YBOS_BANK_HEADER * pybk;
  DWORD * pdata;
  DWORD bklen, bktyp;
  BANK_HEADER * pbh;
  BANK * pmbk;
  EVENT_HEADER * pheader;
  INT status;

  if (data_fmt == FORMAT_YBOS)
  {
    /* event header --> No event header in YBOS */

    /* bank list */
    status = ybk_list (pevent, banklist);
    printf("#banks:%i - Bank list:-%s-\n",status,banklist);

    /* check if EVID is present if so display its content */
    if ((status = ybk_find ((DWORD *)pevent, "EVID", &bklen, &bktyp, (void *)&pybk)) == YB_SUCCESS)
    {
      pdata = (DWORD *)((YBOS_BANK_HEADER *)pybk + 1);
      printf("--------- EVID --------- Event# %i ------Run#:%i--------\n"
					,YBOS_EVID_EVENT_NB(pdata), YBOS_EVID_RUN_NUMBER(pdata));
			printf("Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Dsize:%i/0x%x"
					,YBOS_EVID_EVENT_ID(pdata), YBOS_EVID_TRIGGER_MASK(pdata)
          ,YBOS_EVID_SERIAL(pdata), YBOS_EVID_TIME(pdata)
          ,((YBOS_BANK_HEADER *)pybk)->length
					,((YBOS_BANK_HEADER *)pybk)->length);
    }

    /* display bank content */
    pybk = NULL;
    while (ybk_iterate((DWORD *)pevent, &pybk, (void *)&pdata) && (pybk != NULL))
       ybos_bank_display(pybk, dsp_fmt);
  }
  else if (data_fmt == FORMAT_MIDAS)
  {
    /* skip these special events (NO bank structure) */
    pheader = (EVENT_HEADER *)pevent;
    if(pheader->event_id == EVENTID_BOR ||
       pheader->event_id == EVENTID_EOR ||
       pheader->event_id == EVENTID_MESSAGE)
      return;

    /* event header */
    printf("Evid:%4.4x- Mask:%4.4x- Serial:%i- Time:0x%x- Dsize:%i/0x%x"
  	,pheader->event_id, pheader->trigger_mask ,pheader->serial_number
	  ,pheader->time_stamp, pheader->data_size, pheader->data_size);

     /* bank list */
    status = bk_list ((BANK_HEADER *)(pheader+1), banklist);
    printf("\n#banks:%i - Bank list:-%s-\n",status,banklist);

    /* display bank content */
    pbh = (BANK_HEADER *) (pheader+1);
    pmbk = NULL;
    while (bk_iterate(pbh, &pmbk, &pdata) && (pmbk != NULL))
      midas_bank_display(pmbk, dsp_fmt);
    return;
  }
  return;
}
  
/*------------------------------------------------------------------*/
void  yb_any_bank_display( void * pbk, INT data_fmt, INT dsp_mode, INT dsp_fmt)
/********************************************************************\
  Routine: external yb_any_bank_display
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
    yb_any_raw_bank_display(pbk, data_fmt, dsp_fmt);
  else
    {
      if (data_fmt == FORMAT_MIDAS)
        midas_bank_display (pbk, dsp_fmt);
      else if (data_fmt == FORMAT_YBOS)
        ybos_bank_display (pbk, dsp_fmt);
    }
  return;
}

/*------------------------------------------------------------------*/
void yb_any_raw_bank_display(void * pbank, INT data_fmt, INT dsp_fmt)
/********************************************************************\
  Routine: yb_any_raw_bank_display
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
  DWORD *pdata, lrl, j, i;

  if (data_fmt == FORMAT_YBOS)
  {
    lrl = (((YBOS_BANK_HEADER *)pbank)->length) - 1 ;
    pdata = (DWORD *) (((YBOS_BANK_HEADER *)pbank) + 1);
  }
  else if (data_fmt == FORMAT_MIDAS)
  {
    lrl= ((BANK *)pbank)->data_size >> 2;   /* in DWORD */
    pdata = (DWORD *)((BANK *)(pbank)+1);
  }

  for (i=0;i<lrl;i +=NLINE)
  {
    j = 0;
    printf("\n%4i-> ",i+j+1);
    for (j=0;j<NLINE;j++)
    {
	     if ((i+j) < lrl)
	     { 
          if (dsp_fmt == DSP_DEC) printf ("%8.i ",*((DWORD *)pdata));
          if (dsp_fmt == DSP_ASC) printf ("%8.8x ",*((DWORD *)pdata));
          if (dsp_fmt == DSP_HEX) printf ("%8.8x ",*((DWORD *)pdata));
	        ((DWORD *)pdata)++;
       }
   	} 
  }
}

/*------------------------------------------------------------------*/
void ybos_bank_display(YBOS_BANK_HEADER * pybk, INT dsp_fmt)
/********************************************************************\
  Routine: ybos_event_display
  Purpose: display on screen the YBOS data in YBOS bank mode.
  Input:
    YBOS_BANK_HEADER * pybk     pointer to the bank header
    INT     dsp_fmt             display format (DSP_DEC/HEX)
  Output:             
    none
  Function value:
    none
\********************************************************************/
{
  char bank_name[5], strbktype[32];
  DWORD length_type;
  DWORD *pdata, *pendbk;
  INT i,j;

  j=8;         /* elements within line */
  i=1;         /* data counter */

  pdata = (DWORD *) (pybk+1);
  memcpy (&bank_name[0],(char *) &pybk->name,4);
  bank_name[4]=0;

  if (pybk->type == F4_BKTYPE)
	  {
	    length_type = pybk->length-1;
	    strcpy (strbktype,"Real*4 (FMT machine dependent)");
	  }
  if (pybk->type == I4_BKTYPE)
    {
      length_type = pybk->length-1;
      strcpy (strbktype,"Integer*4");
    }
  if (pybk->type == I2_BKTYPE)
    {
      length_type = ((pybk->length-1) << 1);
      strcpy (strbktype,"Integer*2");
    }
  if (pybk->type == I1_BKTYPE)
    {
      length_type = ((pybk->length-1) << 2);
      strcpy (strbktype,"8 bit Bytes");
    }
  if (pybk->type == A1_BKTYPE)
	  {
	    length_type = ((pybk->length-1) << 2);
	    strcpy (strbktype,"8 bit ASCII");
	  }
  printf("\nBank:%s Length: %i(I*1)/%i(I*4)/%i(Type) Type:%s",
  bank_name,((pybk->length-1) << 2), pybk->length-1, length_type, strbktype);
  j = 16;

  pendbk = pdata + pybk->length - 1;
  while ((BYTE *) pdata < (BYTE *) pendbk)
    {
	    switch (pybk->type)
	      {
	      case F4_BKTYPE :
	        if (j>7)
		      {
		        printf("\n%4i-> ",i);
		        j = 0;
		        i += 8;
		      }
	        if (dsp_fmt == DSP_DEC) printf("%8.3e ",*((float *)pdata));
	        if (dsp_fmt == DSP_HEX) printf("0x%8.8x ",*((DWORD *)pdata));
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
	        if (dsp_fmt == DSP_DEC) printf("%8.1i ",*((DWORD *)pdata));
	        if (dsp_fmt == DSP_HEX) printf("0x%8.8x ",*((DWORD *)pdata));
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
	        if (dsp_fmt == DSP_DEC) printf("%5.1i ",*((WORD *)pdata));
	        if (dsp_fmt == DSP_HEX) printf("0x%4.4x ",*((WORD *)pdata));
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
	        if (dsp_fmt == DSP_DEC) printf("%2.i ",*((BYTE *)pdata));
	        if (dsp_fmt == DSP_ASC) printf("%1.1s ",(char *)pdata);
	        if (dsp_fmt == DSP_HEX) printf("0x%2.2x ",*((BYTE *)pdata));
	        ((BYTE *)pdata)++;
	        j++;
	        break;
	      case I1_BKTYPE :
	        if (j>7)
		      {
		        printf("\n%4i-> ",i);
		        j = 0;
		        i += 8;
		      }
	        if (dsp_fmt == DSP_DEC) printf("%4.i ",*((BYTE *)pdata));
	        if (dsp_fmt == DSP_HEX) printf("0x%2.2x ",*((BYTE *)pdata));
	        ((BYTE *)pdata)++;
	        j++;
	        break;
	      } /* switch */
	  } /* while next bank */
    printf ("\n");
  return;
}

/*------------------------------------------------------------------*/
void midas_bank_display( BANK * pbk, INT dsp_fmt)
/********************************************************************\
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
  char  bank_name[5], strbktype[32];
  char  *pdata, *pendbk;
  DWORD length_type, lrl;
  INT type, i, j;
  
  lrl= pbk->data_size;   /* in bytes */
  type = pbk->type;
  bank_name[4] = 0;
  memcpy (bank_name,(char *)(pbk->name),4);
  pdata = (char *) (pbk+1);

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
	          if (dsp_fmt == DSP_DEC) printf("%8.3e ",*((float *)pdata));
	          if (dsp_fmt == DSP_HEX) printf("0x%8.8x ",*((DWORD *)pdata));
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
	          if (dsp_fmt == DSP_DEC) printf("%8.1i ",*((DWORD *)pdata));
	          if (dsp_fmt == DSP_HEX) printf("0x%8.8x ",*((DWORD *)pdata));
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
	          if (dsp_fmt == DSP_DEC) printf("%5.1i ",*((WORD *)pdata));
	          if (dsp_fmt == DSP_HEX) printf("0x%4.4x ",*((WORD *)pdata));
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
	          if (dsp_fmt == DSP_DEC) printf("%4.i ",*((BYTE *)pdata));
	          if (dsp_fmt == DSP_HEX) printf("0x%2.2x ",*((BYTE *)pdata));
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
	          if (dsp_fmt == DSP_DEC) printf("%3.i ",*((BYTE *)pdata));
	          if (dsp_fmt == DSP_ASC) printf("%1.1s ",(char *)pdata);
	          if (dsp_fmt == DSP_HEX) printf("0x%2.2x ",*((BYTE *)pdata));
        	  pdata++;
	          j++;
	          break;
	        }
    } /* end of bank */
  printf ("\n");
  return;
}

/*-- GENERAL file fragmentation and recovery -----Section d)--------*/
/*-- GENERAL file fragmentation and recovery -----------------------*/
/*-- GENERAL file fragmentation and recovery -----------------------*/
/*-- GENERAL file fragmentation and recovery -----------------------*/
/*------------------------------------------------------------------*/
INT   yb_file_recompose(void * pevt, INT format, char * svpath, INT file_mode)
/********************************************************************\
  Routine: external file_recompose
  Purpose: Receive event which are expected to be file oriented with
           YM_xFILE header.
  Input:
    char * pevt           pointer to a YBOS event (->LRL).
    char * svpath         path where to save file
    INT    file_mode      NO_RUN : save file under original name
                          ADD_RUN: cat run number at end of file name
  Output:
    none
  Function value:
    YB_SUCCESS         OP successfull
    YB_INCOMPLETE      file compose channels still open
    YB_COMPLETE        All file compose channels closed or complete
    status             -x error of inner call
\********************************************************************/
{
  YM_CFILE *pmyfch;
  int slot, status;

  if (file_mode == YB_NO_RECOVER)
    return YB_SUCCESS;

  if (format == FORMAT_YBOS)
    {
      if ((status = ybk_locate((DWORD *)pevt,"CFIL", &pmyfch)) != YB_SUCCESS)
        return (status);
    }
  else if (format == FORMAT_MIDAS)
    {
      if ((((EVENT_HEADER *)pevt)->event_id == EVENTID_BOR) ||
          (((EVENT_HEADER *)pevt)->event_id == EVENTID_EOR) ||
          (((EVENT_HEADER *)pevt)->event_id == EVENTID_MESSAGE))
          return YB_BANK_NOT_FOUND;
    
      pevt = (EVENT_HEADER *)pevt + 1;
      if ((status = bk_locate (pevt ,"CFIL", &pmyfch)) <= 0)
        return (status);
    }

  printf("%i - %i - %i - %i - %i -%i -%i \n"
	     ,pmyfch->file_ID, pmyfch->size
	     ,pmyfch->fragment_size, pmyfch->total_fragment
	     ,pmyfch->current_fragment, pmyfch->current_read_byte
	     ,pmyfch->run_number);

  /* check if file is in progress */
  for (slot=0;slot<MAX_YM_FILE;slot++)
    {
      if ((ymfile[slot].fHandle != 0) && (pmyfch->file_ID == ymfile[slot].file_ID))
        {
	  /* Yep file in progress for that file_ID */
          if ((status = yb_ymfile_update(slot, format, pevt)) != YB_SUCCESS)
            {
              printf("yb_ymfile_update() failed\n");
              return status;
            }
          goto check;
        }
      /* next slot */
    }
  /* current fragment not registered => new file */
  /* open file, get slot back */
  if((status = yb_ymfile_open(&slot, format, pevt, svpath, file_mode)) != YB_SUCCESS)
    {
      printf("yb_ymfile_open() failed\n");
      return status;
    }
  /* update file */
  if ((status = yb_ymfile_update(slot, format, pevt)) != YB_SUCCESS)
    {
      printf("yb_ymfile_update() failed\n");
      return status;
    }

check:
  /* for completion of recovery on ALL files */
  for (slot=0;slot<MAX_YM_FILE;slot++)
    {
      if (ymfile[slot].fHandle != 0)
	    {
	      /* Yes still some file composition in progress */
          return YB_INCOMPLETE;
      } 
      /* next slot */
    }
  return YB_COMPLETE;
}

/*------------------------------------------------------------------*/
INT   yb_ymfile_open(int *slot, int fmt, void *pevt, char * svpath, INT file_mode)
/********************************************************************\
  Routine: yb_ymfile_open
  Purpose: Prepare channel for receiving event of YM_FILE type.
  Input:
    void * pevt           pointer to the data portion of the event
    char * svpath         path where to save file
    INT    file_mode      NO_RUN : save file under original name
                          ADD_RUN: cat run number at end of file name
  Output:
    INT  * slot           index of the opened channel
  Function value:
    YB_SUCCESS          Successful completion
    YB_FAIL_OPEN        cannot create output file
    YB_NOMORE_SLOT      no more slot for starting dump
\********************************************************************/
{
  YM_CFILE *pmyfch;
  YM_PFILE *pmyfph;
  char * pfilename;
  char srun[16], sslot[3];
  int i, status;

  /* initialize invalid slot */
  *slot = -1;

  if (fmt == FORMAT_YBOS)
    {
      if ((status = ybk_locate((DWORD *)pevt,"CFIL", &pmyfch)) != YB_SUCCESS)
        return (status);
      if ((status = ybk_locate((DWORD *)pevt,"PFIL", &pmyfph)) != YB_SUCCESS)
        return (status);
    }
  else if (fmt == FORMAT_MIDAS)
    {
      if ((status = bk_locate(pevt,"CFIL", &pmyfch)) <= 0)
        return (status);
      if ((status = bk_locate(pevt,"PFIL", &pmyfph)) <= 0)
        return (status);
    }
  else 
    return -2;
  /* find free slot */
  for (i=0;i<MAX_YM_FILE; i++)
    if (ymfile[i].fHandle == 0)
      break;
  if (i<MAX_YM_FILE)
    {
      /* copy necessary file header info */
      ymfile[i].file_ID = pmyfch->file_ID;
      strcpy(ymfile[i].path,pmyfph->path);

      /* extract file name */
      pfilename = pmyfph->path;
      if (strrchr(pmyfph->path,'/') > pfilename)
        pfilename = strrchr(pmyfph->path,'/');
      if (strrchr(pmyfph->path,'\\') > pfilename)
        pfilename = strrchr(pmyfph->path,'\\');
      if (strrchr(pmyfph->path,':') > pfilename)
        pfilename = strrchr(pmyfph->path,':');
      if (*pfilename != pmyfph->path[0])
        pfilename++;

      /* add path name */
      if (svpath[0] != 0)
	    { 
	      ymfile[i].path[0]=0;
	      strncat(ymfile[i].path,svpath,strlen(svpath));
        if (ymfile[i].path[strlen(ymfile[i].path)-1] != DIR_SEPARATOR)
          strcat(ymfile[i].path, DIR_SEPARATOR_STR);
        /* append the file name */
        strcat(ymfile[i].path, pfilename);
	    }
      if (file_mode == YB_ADD_RUN)
	    {  /* append run number */
	      strcat (ymfile[i].path,".");
	      sprintf(srun,"Run%4.4i",pmyfch->run_number);
	      strncat (ymfile[i].path,srun,strlen(srun));
	    }
      /* differentiate the possible file dumps 
         as the path is unique */
      if (i > 0)
        {
          sprintf(sslot, ".%03i",i);
          strcat (ymfile[i].path, sslot);
        }

      /* open device */
      if( (ymfile[i].fHandle = open(ymfile[i].path ,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644)) == -1 )
	    {
	      ymfile[i].fHandle = 0;
	      printf("File %s cannot be created\n",ymfile[i].path);
	      return (SS_FILE_ERROR);
	    }
    }
  else
    {
      /* no more slot */
      printf("No more slot for file %s\n",pmyfph->path);
      return YB_NOMORE_SLOT;
    }

  ymfile[i].current_read_byte = 0;
  ymfile[i].current_fragment = 0;
  *slot = i;
  return YB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT   yb_ymfile_update(int slot, int fmt, void * pevt)
/********************************************************************\
  Routine: yb_ymfile_update
  Purpose: dump Midas/Ybos event to file for YBOS file oriented event type.
  Input:
    char * pevt           pointer to the data portion of the event.
  Output:
    INT  slot             valid index of the opened channel.
  Function value:
    YB_SUCCESS         Successful completion
    -1                     error
\********************************************************************/
{
  YM_CFILE *pmyfch;
  char * pmyfd;
  int status;
  int nwrite;

  if (fmt == FORMAT_YBOS)
    {
    if ((status = ybk_locate((DWORD *)pevt,"CFIL", &pmyfch)) != YB_SUCCESS)
      return (status);
    if ((status = ybk_locate((DWORD *)pevt,"DFIL", &pmyfd)) != YB_SUCCESS)
      return (status);

    /* check sequence order */
    if (ymfile[slot].current_fragment + 1 != pmyfch->current_fragment)
      {
        printf("Out of sequence %i / %i\n"
	       ,ymfile[slot].current_fragment,pmyfch->current_fragment);
      }
    /* dump fragment to file */
    nwrite = write (ymfile[slot].fHandle, pmyfd, pmyfch->fragment_size);

    /* update current file record */
    ymfile[slot].current_read_byte += nwrite;
    ymfile[slot].current_fragment++;
    /* check if file has to be closed */
    if ( ymfile[slot].current_fragment == pmyfch->total_fragment)
      {
        /* file complete */
        close (ymfile[slot].fHandle);
        printf("File %s (%i) completed\n",ymfile[slot].path,ymfile[slot].current_read_byte);
        /* cleanup slot */
        ymfile[slot].fHandle = 0;
        return YB_SUCCESS;
      } /* close file */
    else
      {
        /* fragment retrieved wait next one */
        return YB_SUCCESS;
      }
  }
  else if (fmt == FORMAT_MIDAS)
    {
      if ((status = bk_locate(pevt,"CFIL", &pmyfch)) <= 0)
        return (status);
      if ((status = bk_locate(pevt,"DFIL", &pmyfd)) <= 0)
        return (status);

      /* check sequence order */
      if (ymfile[slot].current_fragment + 1 != pmyfch->current_fragment)
        {
          printf("Out of sequence %i / %i\n"
	         ,ymfile[slot].current_fragment,pmyfch->current_fragment);
        }
      /* dump fragment to file */
      nwrite = write (ymfile[slot].fHandle, pmyfd, pmyfch->fragment_size);

      /* update current file record */
      ymfile[slot].current_read_byte += nwrite;
      ymfile[slot].current_fragment++;
      /* check if file has to be closed */
      if ( ymfile[slot].current_fragment == pmyfch->total_fragment)
        {
          /* file complete */
          close (ymfile[slot].fHandle);
          printf("File %s (%i) completed\n",ymfile[slot].path,ymfile[slot].current_read_byte);
          /* cleanup slot */
          ymfile[slot].fHandle = 0;
          return YB_SUCCESS;
        } /* close file */
      else
        {
          /* fragment retrieved wait next one */
          return YB_SUCCESS;
        }
      }
}
#endif /* !FE_YBOS_SUPPORT */
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*--END of YBOS.C---------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

/********************************************************************\

  Name:         ebuser.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     User section for the Event builder

  $Log$
  Revision 1.2  2002/01/17 23:34:29  pierre
  doc++ format

  Revision 1.1.1.1  2002/01/17 19:49:54  pierre
  Initial Version

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "mevb.h"
#include "ybos.h"

INT eb_begin_of_run(INT, char *);
INT eb_end_of_run(INT, char *);
INT ebuser(INT, EBUILDER_CHANNEL *, EVENT_HEADER *, void *, INT *);

/*--------------------------------------------------------------------*/
/** eb_begin_of_run()
Hook to the event builder task at PreStart transition.
\begin{verbatim}
\end{verbatim}

@param rn Run Number
@param error error string to be passed back to the system.
@return EB_SUCCESS
*/
INT eb_begin_of_run(INT rn, char * error)
{
  printf("In eb_begin_of_run\n");
  return EB_SUCCESS;
}

/*--------------------------------------------------------------------*/
/** eb_end_of_run()
Hook to the event builder task at completion of event collection after
receiving the Stop transition.
\begin{verbatim}
\end{verbatim}

@param rn Run Number
@param error error string to be passed back to the system.
@return EB_SUCCESS
*/
INT eb_end_of_run(INT rn, char * error)
{
  printf("In eb_end_of_run\n");
  return EB_SUCCESS;
}

/*--------------------------------------------------------------------*/
/** @name eb_user()
Hook to the event builder task after the reception of
all fragments of the same serial number. The destination
event has already the final EVENT_HEADER setup with
the data size set to 0. It is than possible to
add private data at this point using the proper
bank calls.

The ebch[] array structure points to nfragment channel structure
with the following content:
\begin{verbatim}
typedef struct {
    char  name[32];         // Fragment name (Buffer name).
    DWORD serial;           // Serial fragment number.
    char *pfragment;        // Pointer to fragment (EVENT_HEADER *)
    ...
} EBUILDER_CHANNEL;
\end{verbatim}

The correct code for including your own bank is shown below where
{\bf TID_xxx} is one of the valid Bank type starting with {\bf TID_} for
midas format or {\bf xxx_BKTYPE} for Ybos data format.
{\bf bank_name} is a 4 character descriptor.
{\bf pdata} has to be declared accordingly with the bank type.
Refers to the ebuser.c source code for further description.

{\Large It is not possible to mix within the same destination event different
   event format!}

\begin{verbatim}
  // Event is empty, fill it with BANK_HEADER
  // If you need to add your own bank at this stage 
  
  bk_init(pevent);
  bk_create(pevent, bank_name, TID_xxxx, &pdata);
  *pdata++ = ...;
  *dest_size = bk_close(pevent, pdata);
  pheader->data_size = *dest_size + sizeof(EVENT_HEADER);

\end{verbatim}

@param nfrag Number of fragment.
@param ebch  Structure to all the fragments.
@param pheader Destination pointer to the header.
@param pevent Destination pointer to the bank header.
@param dest_size Destination event size in bytes.
@memo  event builder user code for private data filtering.
@return
*/
INT eb_user(INT nfrag
	    , EBUILDER_CHANNEL * ebch, EVENT_HEADER *pheader
	    , void *pevent, INT * dest_size)
{
  INT    i, dest_serial, frag_size, serial;
  DWORD  *pdata;

  dest_serial = pheader->serial_number;  
  printf("Event Serial%d ", dest_serial);
  for (i=0;i<nfrag;i++) {
    frag_size  = ((EVENT_HEADER *) ebch[i].pfragment)->data_size;
    serial =  ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;
    printf("Fragment#:%d Data size:%d Serial%d ", i+1, frag_size, serial);
  }
  
  /* Event is empty, fill it with BANK_HEADER
     If you need to add your own bank at this stage */
  bk_init(pevent);
  
  bk_create(pevent, "EVTB", TID_DWORD, &pdata);
  *pdata++ = 0xAA0000AA;
  *pdata++ = serial;
  *pdata++ = 0x77000077;
  *pdata++ = 0xFF0000FF;
  *dest_size = bk_close(pevent, pdata);
  
  /* The destination data_size has to be adjust for:
     the new data + the header length */
  pheader->data_size = *dest_size + sizeof(EVENT_HEADER);

  /* To slow it down */
  cm_yield(20);
  printf("\n");
  return EB_SUCCESS;
}


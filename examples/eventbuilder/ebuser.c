/********************************************************************\

  Name:         ebuser.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     User section for the Event builder

  $Id$

\********************************************************************/
/** @file ebuser.c
The Event builder user file
*/

#include <stdio.h>
#include "midas.h"
#include "mevb.h"
#include "ybos.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "Ebuilder";

/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL ebuilder_call_loop = FALSE;

/* A frontend status page is displayed with this frequency in ms */
INT display_period = 3000;

/* maximum event size produced by this frontend */
INT max_event_size = 10000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * 10000;

/**
Globals */
INT lModulo = 100;              ///< Global var for testing passed at BOR

/*-- Function declarations -----------------------------------------*/
INT ebuilder_init();
INT ebuilder_exit();
INT eb_begin_of_run(INT, char *, char *);
INT eb_end_of_run(INT, char *);
INT ebuilder_loop();
INT ebuser(INT, BOOL mismatch, EBUILDER_CHANNEL *, EVENT_HEADER *, void *, INT *);
INT read_scaler_event(char *pevent, INT off);
extern EBUILDER_SETTINGS ebset;
extern BOOL debug;

/*-- Equipment list ------------------------------------------------*/
EQUIPMENT equipment[] = {
   {"EB",                /* equipment name */
    {1, 1,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     0,                      /* equipment type */
     0,                      /* event source */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     },
    },

  {""}
};

#ifdef __cplusplus
}
#endif
/********************************************************************/
/********************************************************************/

/********************************************************************/
INT ebuilder_init()
{
  return EB_SUCCESS;
}

/********************************************************************/
INT ebuilder_exit()
{
  return EB_SUCCESS;
}

/********************************************************************/
INT ebuilder_loop()
{
  return EB_SUCCESS;
}

/********************************************************************/
/**
Hook to the event builder task at PreStart transition.
@param rn run number
@param UserField argument from /Ebuilder/Settings
@param error error string to be passed back to the system.
@return EB_SUCCESS
*/
INT eb_begin_of_run(INT rn, char *UserField, char *error)
{
  printf("In eb_begin_of_run for run:%d User_field:%s \n", rn, UserField);
   lModulo = atoi(UserField);
   return EB_SUCCESS;
}

/********************************************************************/
/**
Hook to the event builder task at completion of event collection after
receiving the Stop transition.
@param rn run number
@param error error string to be passed back to the system.
@return EB_SUCCESS
*/
INT eb_end_of_run(INT rn, char *error)
{
   printf("In eb_end_of_run\n");
   return EB_SUCCESS;
}

/********************************************************************/
/**
Hook to the event builder task after the reception of
all fragments of the same serial number. The destination
event has already the final EVENT_HEADER setup with
the data size set to 0. It is than possible to
add private data at this point using the proper
bank calls.

The ebch[] array structure points to nfragment channel structure
with the following content:
\code
typedef struct {
    char  name[32];         // Fragment name (Buffer name).
    DWORD serial;           // Serial fragment number.
    char *pfragment;        // Pointer to fragment (EVENT_HEADER *)
    ...
} EBUILDER_CHANNEL;
\endcode

The correct code for including your own MIDAS bank is shown below where
\b TID_xxx is one of the valid Bank type starting with \b TID_ for
midas format or \b xxx_BKTYPE for Ybos data format.
\b bank_name is a 4 character descriptor.
\b pdata has to be declared accordingly with the bank type.
Refers to the ebuser.c source code for further description.

<strong>
It is not possible to mix within the same destination event different event format!
</strong>

\code
  // Event is empty, fill it with BANK_HEADER
  // If you need to add your own bank at this stage

  bk_init(pevent);
  bk_create(pevent, bank_name, TID_xxxx, &pdata);
  *pdata++ = ...;
  *dest_size = bk_close(pevent, pdata);
  pheader->data_size = *dest_size + sizeof(EVENT_HEADER);
\endcode

For YBOS format, use the following example.

\code
  ybk_init(pevent);
  ybk_create(pevent, "EBBK", I4_BKTYPE, &pdata);
  *pdata++ = 0x12345678;
  *pdata++ = 0x87654321;
  *dest_size = ybk_close(pevent, pdata);
  *dest_size *= 4;
  pheader->data_size = *dest_size + sizeof(YBOS_BANK_HEADER);
\endcode
@param nfrag Number of fragment.
@param mismatch Midas Serial number mismatch flag.
@param ebch  Structure to all the fragments.
@param pheader Destination pointer to the header.
@param pevent Destination pointer to the bank header.
@param dest_size Destination event size in bytes.
@return EB_SUCCESS
*/
INT eb_user(INT nfrag, BOOL mismatch, EBUILDER_CHANNEL * ebch
            , EVENT_HEADER * pheader, void *pevent, INT * dest_size)
{
  INT i, frag_size, serial;
  DWORD *psrcData;
//  DWORD  *pdata;

  //
  // Do some extra fragment consistency check
  if (mismatch){
    printf("Serial number do not match across fragments\n");
    for (i = 0; i < nfrag; i++) {
      serial = ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;
      printf("Ser[%i]:%d ", i + 1, serial);
    }
    printf("\n");
    return EB_USER_ERROR;
  }

  //
  // Include my own bank
  //bk_init(pevent);
  //bk_create(pevent, "MYOW", TID_DWORD, &pdata);
  //for (i = 0; i < nfrag; i++) {
  //  *pdata++ = ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;
  //  *pdata++ = ((EVENT_HEADER *) ebch[i].pfragment)->time_stamp;
  //}
  //*dest_size = bk_close(pevent, pdata);
  //pheader->data_size = *dest_size + sizeof(EVENT_HEADER);


  //
  // Destination access
  // dest_serial = pheader->serial_number;
  // printf("DSer#:%d ", dest_serial);

  // Stop run if condition requires
  // if (dest_serial == 505) return EB_USER_ERROR;

  // Skip event if condition requires
  // if (dest_serial == 505) return EB_SKIP;

  //
  // Loop over fragments.
  if (debug) {
    for (i = 0; i < nfrag; i++) {
      if (ebset.preqfrag[i]) { // printf if channel enable
        frag_size = ((EVENT_HEADER *) ebch[i].pfragment)->data_size;
        serial = ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;
        printf("Frg#:%d Dsz:%d Ser:%d ", i + 1, frag_size, serial);
        // For Data fragment Access.
        psrcData = (DWORD *) (((EVENT_HEADER *) ebch[i].pfragment) + 1);
      }
    }
    printf("\n");
  }
  return EB_SUCCESS;
}

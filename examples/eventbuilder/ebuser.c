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
#include <string.h>
#include "midas.h"
#include "mevb.h"

/* data type sizes */
extern INT tid_size[];
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
INT max_event_size = 500000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 20 * 50000;

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
    {1, 0,                   /* event ID, trigger mask */
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
INT ebuilder_init()
{
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
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
bank calls. Therefore any new banks created within eb_user will be appearing
before the collected banks from the fragments.
When using the eb_user with the ODB flag "user build=y" 
(equipments/EB/settings) the automatic event builder is skipped.
This allow the user to extract selectively from the different fragments the
appropriate banks and compose the final destination event. In order to
do so, the function "bk_copy(pevent, ebch[i].pfragment, bankname)" will copy
a particular bank from a given fragment.

<b>Note:</b> It exists two Midas event format to address bank size less than 32KB and 
larger bank size <4GB. This distinction is done by the call bk_init(pevent) for the small
bank size and bk_init32(pevent) for large bank size. Within an experiment, this
declaration has to be consistant. Therefore the bk_init in the eb_user should follow
as well the type of the frontends.

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
midas format \b bank_name is a 4 character descriptor.
\b pdata has to be declared accordingly with the bank type.
Refers to the ebuser.c source code for further description.

<strong>
It is not possible to mix within the same destination event different event format!
No bk_swap performed when user build is requested.
</strong>

\code
  // Event is empty, fill it with BANK_HEADER
  // If you need to add your own bank at this stage

  // Need to match the decalration in the Frontends.
  bk_init(pevent);  
//  bk_init32(pevent);
  bk_create(pevent, bank_name, TID_xxxx, &pdata);
  *pdata++ = ...;
  *dest_size = bk_close(pevent, pdata);
  pheader->data_size = *dest_size + sizeof(EVENT_HEADER);
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
  INT i, frag_size, serial, status;
  DWORD  *pdata, *psrcData;


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
  bk_init(pevent);
  bk_create(pevent, "MYOW", TID_DWORD, &pdata);
  for (i = 0; i < nfrag; i++) {
    *pdata++ = ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;
    *pdata++ = ((EVENT_HEADER *) ebch[i].pfragment)->time_stamp;
  }
  *dest_size = bk_close(pevent, pdata);
  pheader->data_size = *dest_size + sizeof(EVENT_HEADER);

  // Copy the bank TC01 if found from fragment0 to the destination
  status = bk_copy(pevent, ebch[0].pfragment, "TC01");
  if (status == EB_BANK_NOT_FOUND) {
    printf("bank TC01 not found\n");
  }

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
      if (1) {
	if (ebset.preqfrag[i]) { // printf if channel enable
	  frag_size = ((EVENT_HEADER *) ebch[i].pfragment)->data_size;
	  serial = ((EVENT_HEADER *) ebch[i].pfragment)->serial_number;
	  printf("Frg#:%d Dsz:%d Ser:%d ", i, frag_size, serial);
	  // For Data fragment Access.
	  psrcData = (DWORD *) (((EVENT_HEADER *) ebch[i].pfragment) + 1);
	}
      }
    }
    printf("\n");
  }
  return EB_SUCCESS;
}

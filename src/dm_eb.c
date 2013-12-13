/********************************************************************\

  Name:         dm_eb.c
  Created by:   Stefan Ritt

  Contents:     dm_xxx() and eb_xxx() functions separated from midas.c

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include <assert.h>
#include <signal.h>

#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

static INT _send_sock;

/**dox***************************************************************/
/** @file dm_eb.c
The main core C-code for Midas.
*/

/** @defgroup dmfunctionc Midas Dual Buffer Memory Functions (dm_xxx)
 */

/**dox***************************************************************/
/** @addtogroup dmfunctionc
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************\
*                                                                    *
*                 Dual memory buffer functions                       *
*                                                                    *
* Provide a dual memory buffer scheme for handling front-end         *
* event. This code as been requested for allowing contemporary       *
* task handling a)acquisition, b)network transfer if possible.       *
* The pre-compiler switch will determine the mode of operation.      *
* if DM_DUAL_THREAD is defined in mfe.c, it is expected to have      *
* a seperate task taking care of the dm_area_send                    *
*                                                                    *
* "*" : visible functions                                            *
* dm_buffer_create():     *Setup the dual memory buffer              *
*                          Setup semaphore                           *
*                          Spawn second thread                       *
* dm_buffer_release():    *Release memory allocation for dm          *
*                          Force a kill of 2nd thread                *
*                          Remove semaphore                          *
* dm_area_full():         *Check for both area being full            *
*                          None blocking, may be used for interrupt  *
*                          disable.                                  *
* dm_pointer_get()     :  *Check memory space and return pointer     *
*                          Blocking function with timeout if no more *
*                          space for next event. If error will abort.*
* dm_pointer_increment(): *Move pointer to next free location        *
*                          None blocking. performs bm_send_event if  *
*                          local connection.                         *
* dm_area_send():         *Transfer FULL buffer(s)                   *
*                          None blocking function.                   *
*                          if DUAL_THREAD: Give sem_send semaphore   *
*                          else transfer FULL buffer                 *
* dm_area_flush():        *Transfer all remaining events from dm     *
*                          Blocking function with timeout            *
*                          if DUAL_THREAD: Give sem_flush semaphore. *
* dm_task():               Secondary thread handling DUAL_THREAD     *
*                          mechanism. Serves 2 requests:             *
*                          dm_send:  Transfer FULL buffer only.      *
*                          dm_flush: Transfer ALL buffers.           *
* dm_area_switch():        internal, used by dm_pointer_get()        *
* dm_active_full():        internal: check space in current buffer   *
* dm_buffer_send():        internal: send data for given area        *
* dm_buffer_time_get():    interal: return the time stamp of the     *
*                          last switch                               *
\********************************************************************/

#define DM_FLUSH       10       /* flush request for DUAL_THREAD */
#define DM_SEND        11       /* FULL send request for DUAL_THREAD */
#define DM_KILL        12       /* Kill request for 2nd thread */
#define DM_TIMEOUT     13       /* "timeout" return state in flush request for DUAL_THREAD */
#define DM_ACTIVE_NULL 14       /* "both buffer were/are FULL with no valid area" return state */

typedef struct {
   char *pt;                    /* top pointer    memory buffer          */
   char *pw;                    /* write pointer  memory buffer          */
   char *pe;                    /* end   pointer  memory buffer          */
   char *pb;                    /* bottom pointer memory buffer          */
   BOOL full;                   /* TRUE if memory buffer is full         */
   DWORD serial;                /* full buffer serial# for evt order     */
} DMEM_AREA;

typedef struct {
   DMEM_AREA *pa;               /* active memory buffer */
   DMEM_AREA area1;             /* mem buffer area 1 */
   DMEM_AREA area2;             /* mem buffer area 2 */
   DWORD serial;                /* overall buffer serial# for evt order     */
   INT action;                  /* for multi thread configuration */
   DWORD last_active;           /* switch time stamp */
   HNDLE sem_send;              /* semaphore for dm_task */
   HNDLE sem_flush;             /* semaphore for dm_task */
} DMEM_BUFFER;

DMEM_BUFFER dm;
INT dm_user_max_event_size;

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Setup a dual memory buffer. Has to be called initially before
           any other dm_xxx function
@param size             Size in bytes
@param user_max_event_size max event size
@return CM_SUCCESS, BM_NO_MEMORY, BM_MEMSIZE_MISMATCH
*/
INT dm_buffer_create(INT size, INT user_max_event_size)
{

   dm.area1.pt = (char *) M_MALLOC(size);
   if (dm.area1.pt == NULL)
      return (BM_NO_MEMORY);
   dm.area2.pt = (char *) M_MALLOC(size);
   if (dm.area2.pt == NULL)
      return (BM_NO_MEMORY);

   /* check user event size against the system MAX_EVENT_SIZE */
   if (user_max_event_size > MAX_EVENT_SIZE) {
      cm_msg(MERROR, "dm_buffer_create", "user max event size too large");
      return BM_MEMSIZE_MISMATCH;
   }
   dm_user_max_event_size = user_max_event_size;

   memset(dm.area1.pt, 0, size);
   memset(dm.area2.pt, 0, size);

   /* initialize pointers */
   dm.area1.pb = dm.area1.pt + size - 1024;
   dm.area1.pw = dm.area1.pe = dm.area1.pt;
   dm.area2.pb = dm.area2.pt + size - 1024;
   dm.area2.pw = dm.area2.pe = dm.area2.pt;

  /*-PAA-*/
#ifdef DM_DEBUG
   printf(" in dm_buffer_create ---------------------------------\n");
   printf(" %i %p %p %p %p\n", size, dm.area1.pt, dm.area1.pw, dm.area1.pe, dm.area1.pb);
   printf(" %i %p %p %p %p\n", size, dm.area2.pt, dm.area2.pw, dm.area2.pe, dm.area2.pb);
#endif

   /* activate first area */
   dm.pa = &dm.area1;

   /* Default not full */
   dm.area1.full = dm.area2.full = FALSE;

   /* Reset serial buffer number with proper starting sequence */
   dm.area1.serial = dm.area2.serial = 0;
   /* ensure proper serial on next increment */
   dm.serial = 1;

   /* set active buffer time stamp */
   dm.last_active = ss_millitime();

   /* get socket for event sending */
   _send_sock = rpc_get_event_sock();

#ifdef DM_DUAL_THREAD
   {
      INT status;
      VX_TASK_SPAWN starg;

      /* create semaphore */
      status = ss_semaphore_create("send", &dm.sem_send);
      if (status != SS_CREATED && status != SS_SUCCESS) {
         cm_msg(MERROR, "dm_buffer_create", "error in ss_semaphore_create send");
         return status;
      }
      status = ss_semaphore_create("flush", &dm.sem_flush);
      if (status != SS_CREATED && status != SS_SUCCESS) {
         cm_msg(MERROR, "dm_buffer_create", "error in ss_semaphore_create flush");
         return status;
      }
      /* spawn dm_task */
      memset(&starg, 0, sizeof(VX_TASK_SPAWN));

#ifdef OS_VXWORKS
      /* Fill up the necessary arguments */
      strcpy(starg.name, "areaSend");
      starg.priority = 120;
      starg.stackSize = 20000;
#endif

      if ((status = ss_thread_create(dm_task, (void *) &starg))
          != SS_SUCCESS) {
         cm_msg(MERROR, "dm_buffer_create", "error in ss_thread_create");
         return status;
      }
#ifdef OS_WINNT
      /* necessary for true MUTEX (NT) */
      ss_semaphore_wait_for(dm.sem_send, 0);
#endif
   }
#endif                          /* DM_DUAL_THREAD */

   return CM_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT dm_buffer_release(void)
/********************************************************************\
  Routine: dm_buffer_release

  Purpose: Release dual memory buffers
  Input:
    none
  Output:
    none
  Function value:
    CM_SUCCESS              Successful completion
\********************************************************************/
{
   if (dm.area1.pt) {
      free(dm.area1.pt);
      dm.area1.pt = NULL;
   }
   if (dm.area2.pt) {
      free(dm.area2.pt);
      dm.area2.pt = NULL;
   }
   dm.serial = 0;
   dm.area1.full = dm.area2.full = TRUE;
   dm.area1.serial = dm.area2.serial = 0;

#ifdef DM_DUAL_THREAD
   /* kill spawned dm_task */
   dm.action = DM_KILL;
   ss_semaphore_release(dm.sem_send);
   ss_semaphore_release(dm.sem_flush);

   /* release semaphore */
   ss_semaphore_delete(dm.sem_send, 0);
   ss_semaphore_delete(dm.sem_flush, 0);
#endif

   return CM_SUCCESS;
}

/********************************************************************/
INLINE DMEM_AREA *dm_area_switch(void)
/********************************************************************\
  Routine: dm_area_switch

  Purpose: set active area to the other empty area or NULL if both
           area are full. May have to check the serial consistancy...
  Input:
    none
  Output:
    none
  Function value:
    DMEM_AREA *            Pointer to active area or both full
\********************************************************************/
{
   volatile BOOL full1, full2;

   full1 = dm.area1.full;
   full2 = dm.area2.full;

   if (!full1 && !full2) {
      if (dm.area1.serial <= dm.area2.serial)
         return (&(dm.area1));
      else
         return (&(dm.area2));
   }

   if (!full1) {
      return (&(dm.area1));
   } else if (!full2) {
      return (&(dm.area2));
   }
   return (NULL);
}

/********************************************************************/
INLINE BOOL dm_area_full(void)
/********************************************************************\
  Routine: dm_area_full

  Purpose: Test if both area are full in order to block interrupt
  Input:
    none
  Output:
    none
  Function value:
    BOOL         TRUE if not enough space for another event
\********************************************************************/
{
   if (dm.pa == NULL || (dm.area1.full && dm.area2.full))
      return TRUE;
   return FALSE;
}

/********************************************************************/
INLINE BOOL dm_active_full(void)
/********************************************************************\
  Routine: dm_active_full

  Purpose: Test if there is sufficient space in either event buffer
           for another event.
  Input:
    none
  Output:
    none
  Function value:
    BOOL         TRUE if not enough space for another event
\********************************************************************/
{
   /* catch both full areas, waiting for transfer */
   if (dm.pa == NULL)
      return TRUE;
   /* Check the space in the active buffer only
      as I don't switch buffer here */
   if (dm.pa->full)
      return TRUE;
   return (((POINTER_T) dm.pa->pb - (POINTER_T) dm.pa->pw) < (INT)
           (dm_user_max_event_size + sizeof(EVENT_HEADER) + sizeof(INT)));
}

/********************************************************************/
DWORD dm_buffer_time_get(void)
/********************************************************************\
  Routine: dm_buffer_time_get

  Purpose: return the time from the last buffer switch.

  Input:
    none
  Output:
    none
  Function value:
    DWORD        time stamp

\********************************************************************/
{
   return (dm.last_active);
}


/********************************************************************/
EVENT_HEADER *dm_pointer_get(void)
/********************************************************************\
  Routine: dm_pointer_get

  Purpose: Get pointer to next free location in event buffer.
           after 10sec tries, it times out return NULL indicating a
           serious problem, i.e. abort.
  REMARK : Cannot be called twice in a raw due to +sizeof(INT)
  Input:
    none
  Output:
    DM_BUFFER * dm    local valid dm to work on
  Function value:
    EVENT_HEADER *    Pointer to free location
    NULL              cannot after several attempt get free space => abort
\********************************************************************/
{
   int timeout, status;

   /* Is there still space in the active area ? */
   if (!dm_active_full())
      return (EVENT_HEADER *) (dm.pa->pw + sizeof(INT));

   /* no more space => switch area */

   /* Tag current area with global dm.serial for order consistency */
   dm.pa->serial = dm.serial++;

   /* set active buffer time stamp */
   dm.last_active = ss_millitime();

   /* mark current area full */
   dm.pa->full = TRUE;

   /* Trigger/do data transfer (Now/don't wait) */
   if ((status = dm_area_send()) == RPC_NET_ERROR) {
      cm_msg(MERROR, "dm_pointer_get()", "Net error or timeout %i", status);
      return NULL;
   }

   /* wait switch completion (max 10 sec) */
   timeout = ss_millitime();    /* with timeout */
   while ((ss_millitime() - timeout) < 10000) {
      dm.pa = dm_area_switch();
      if (dm.pa != NULL)
         return (EVENT_HEADER *) (dm.pa->pw + sizeof(INT));
      ss_sleep(200);
#ifdef DM_DEBUG
      printf(" waiting for space ... %i  dm_buffer  %i %i %i %i %i \n",
             ss_millitime() - timeout, dm.area1.full, dm.area2.full, dm.area1.serial, dm.area2.serial,
             dm.serial);
#endif
   }

   /* Time running out abort */
   cm_msg(MERROR, "dm_pointer_get", "Timeout due to buffer full");
   return NULL;
}


/********************************************************************/
int dm_pointer_increment(INT buffer_handle, INT event_size)
/********************************************************************\
  Routine: dm_pointer_increment

  Purpose: Increment write pointer of event buffer after an event
           has been copied into the buffer (at an address previously
           obtained via dm_pointer_get)
  Input:
    INT buffer_handle         Buffer handle event should be sent to
    INT event_size            Event size in bytes including header
  Output:
    none
  Function value:
    CM_SUCCESS                Successful completion
    status                    from bm_send_event for local connection
\********************************************************************/
{
   INT aligned_event_size;

   /* if not connected remotely, use bm_send_event */
   if (_send_sock == 0) {
      *((INT *) dm.pa->pw) = buffer_handle;
      return bm_send_event(buffer_handle, dm.pa->pw + sizeof(INT), event_size, BM_WAIT);
   }
   aligned_event_size = ALIGN8(event_size);

   *((INT *) dm.pa->pw) = buffer_handle;

   /* adjust write pointer */
   dm.pa->pw += sizeof(INT) + aligned_event_size;

   /* adjust end pointer */
   dm.pa->pe = dm.pa->pw;

   return CM_SUCCESS;
}

/********************************************************************/
INLINE INT dm_buffer_send(DMEM_AREA * larea)
/********************************************************************\
  Routine: dm_buffer_send

  Purpose: Ship data to the cache in fact!
           Basically the same do loop is done in the send_tcp.
           but _opt_tcp_size preveal if <= NET_TCP_SIZE.
           Introduced for bringing tcp option to user code.
  Input:
    DMEM_AREA * larea   The area to work with.
  Output:
    none
  Function value:
    CM_SUCCESS       Successful completion
    DM_ACTIVE_NULL   Both area were/are full
    RPC_NET_ERROR    send error
\********************************************************************/
{
   INT tot_size, nwrite;
   char *lpt;

   /* if not connected remotely, use bm_send_event */
   if (_send_sock == 0)
      return bm_flush_cache(*((INT *) dm.pa->pw), BM_NO_WAIT);

   /* alias */
   lpt = larea->pt;

   /* Get overall buffer size */
   tot_size = (POINTER_T) larea->pe - (POINTER_T) lpt;

   /* shortcut for speed */
   if (tot_size == 0)
      return CM_SUCCESS;

#ifdef DM_DEBUG
   printf("lpt:%p size:%i ", lpt, tot_size);
#endif
   nwrite = send_tcp(_send_sock, lpt, tot_size, 0);
#ifdef DM_DEBUG
   printf("nwrite:%i  errno:%i\n", nwrite, errno);
#endif
   if (nwrite < 0)
      return RPC_NET_ERROR;

   /* reset area */
   larea->pw = larea->pe = larea->pt;
   larea->full = FALSE;
   return CM_SUCCESS;
}

/********************************************************************/
INT dm_area_send(void)
/********************************************************************\
  Routine: dm_area_send

  Purpose: Empty the FULL area only in proper event order
           Meant to be use either in mfe.c scheduler on every event

  Dual memory scheme:
   DM_DUAL_THREAD : Trigger sem_send
   !DM_DUAL_THREAD: empty full buffer in order, return active to area1
                    if dm.pa is NULL (were both full) and now both are empty

  Input:
    none
  Output:
    none
  Function value:
    CM_SUCCESS                Successful completion
    RPC_NET_ERROR             send error
\********************************************************************/
{
#ifdef DM_DUAL_THREAD
   INT status;

   /* force a DM_SEND if possible. Don't wait for completion */
   dm.action = DM_SEND;
   ss_semaphore_release(dm.sem_send);
#ifdef OS_WINNT
   /* necessary for true MUTEX (NT) */
   status = ss_semaphore_wait_for(dm.sem_send, 1);
   if (status == SS_NO_SEMAPHORE) {
      printf(" timeout while waiting for sem_send\n");
      return RPC_NET_ERROR;
   }
#endif

   return CM_SUCCESS;
#else
   /* ---------- NOT IN DUAL THREAD ----------- */
   INT status = 0;

   /* if no DUAL thread everything is local then */
   /* select the full area */
   if (dm.area1.full && dm.area2.full)
      if (dm.area1.serial <= dm.area2.serial)
         status = dm_buffer_send(&dm.area1);
      else
         status = dm_buffer_send(&dm.area2);
   else if (dm.area1.full)
      status = dm_buffer_send(&dm.area1);
   else if (dm.area2.full)
      status = dm_buffer_send(&dm.area2);
   if (status != CM_SUCCESS)
      return status;            /* catch transfer error too */

   if (dm.pa == NULL) {
      printf(" sync send dm.pa:%p full 1%d 2%d\n", dm.pa, dm.area1.full, dm.area2.full);
      dm.pa = &dm.area1;
   }
   return CM_SUCCESS;
#endif
}

/********************************************************************/
INT dm_task(void *pointer)
/********************************************************************\
  Routine: dm_task

  Purpose: async send events doing a double purpose:
  a) send full buffer if found (DM_SEND) set by dm_active_full
  b) flush full areas (DM_FLUSH) set by dm_area_flush
  Input:
  none
  Output:
  none
  Function value:
  none
  \********************************************************************/
{
#ifdef DM_DUAL_THREAD
   INT status, timeout;

   printf("Semaphores initialization ... in areaSend ");
   /* Check or Wait for semaphore to be setup */
   timeout = ss_millitime();
   while ((ss_millitime() - timeout < 3000) && (dm.sem_send == 0))
      ss_sleep(200);
   if (dm.sem_send == 0)
      goto kill;

#ifdef OS_WINNT
   /* necessary for true MUTEX (NT) get semaphore */
   ss_semaphore_wait_for(dm.sem_flush, 0);
#endif

   /* Main FOREVER LOOP */
   printf("task areaSend ready...\n");
   while (1) {
      if (!dm_area_full()) {
         /* wait semaphore here ........ 0 == forever */
         ss_semaphore_wait_for(dm.sem_send, 0);
#ifdef OS_WINNT
         /* necessary for true MUTEX (NT) give semaphore */
         ss_semaphore_release(dm.sem_send);
#endif
      }
      if (dm.action == DM_SEND) {
#ifdef DM_DEBUG
         printf("Send %i %i ", dm.area1.full, dm.area2.full);
#endif
         /* DM_SEND : Empty the oldest buffer only. */
         if (dm.area1.full && dm.area2.full) {
            if (dm.area1.serial <= dm.area2.serial)
               status = dm_buffer_send(&dm.area1);
            else
               status = dm_buffer_send(&dm.area2);
         } else if (dm.area1.full)
            status = dm_buffer_send(&dm.area1);
         else if (dm.area2.full)
            status = dm_buffer_send(&dm.area2);

         if (status != CM_SUCCESS) {
            cm_msg(MERROR, "dm_task", "network error %i", status);
            goto kill;
         }
      } /* if DM_SEND */
      else if (dm.action == DM_FLUSH) {
         /* DM_FLUSH: User is waiting for completion (i.e. No more incomming
            events) Empty both area in order independently of being full or not */
         if (dm.area1.serial <= dm.area2.serial) {
            status = dm_buffer_send(&dm.area1);
            if (status != CM_SUCCESS)
               goto error;
            status = dm_buffer_send(&dm.area2);
            if (status != CM_SUCCESS)
               goto error;
         } else {
            status = dm_buffer_send(&dm.area2);
            if (status != CM_SUCCESS)
               goto error;
            status = dm_buffer_send(&dm.area1);
            if (status != CM_SUCCESS)
               goto error;
         }
         /* reset counter */
         dm.area1.serial = 0;
         dm.area2.serial = dm.serial = 1;
#ifdef DM_DEBUG
         printf("dm.action: Flushing ...\n");
#endif
         /* reset area to #1 */
         dm.pa = &dm.area1;

         /* release user */
         ss_semaphore_release(dm.sem_flush);
#ifdef OS_WINNT
         /* necessary for true MUTEX (NT) get semaphore back */
         ss_semaphore_wait_for(dm.sem_flush, 0);
#endif
      }
      /* if FLUSH */
      if (dm.action == DM_KILL)
         goto kill;

   }                            /* FOREVER (go back wainting for semaphore) */

   /* kill spawn now */
 error:
   cm_msg(MERROR, "dm_area_flush", "aSync Net error");
 kill:
   ss_semaphore_release(dm.sem_flush);
#ifdef OS_WINNT
   ss_semaphore_wait_for(dm.sem_flush, 1);
#endif
   cm_msg(MERROR, "areaSend", "task areaSend exiting now");
   exit;
   return 1;
#else
   printf("DM_DUAL_THREAD not defined %p\n", pointer);
   return 0;
#endif
}

/********************************************************************/
INT dm_area_flush(void)
/********************************************************************\
  Routine: dm_area_flush

  Purpose: Flush all the events in the areas.
           Used in mfe for BOR events, periodic events and
           if rate to low in main loop once a second. The standard
           data transfer should be done/triggered by dm_area_send (sync/async)
           in dm_pointer_get().
  Input:
    none
  Output:
    none
  Function value:
    CM_SUCCESS       Successful completion
    RPC_NET_ERROR    send error
\********************************************************************/
{
   INT status;
#ifdef DM_DUAL_THREAD
   /* request FULL flush */
   dm.action = DM_FLUSH;
   ss_semaphore_release(dm.sem_send);
#ifdef OS_WINNT
   /* necessary for true MUTEX (NT) get semaphore back */
   ss_semaphore_wait_for(dm.sem_send, 0);
#endif

   /* important to wait for completion before continue with timeout
      timeout specified milliseconds */
   status = ss_semaphore_wait_for(dm.sem_flush, 10000);
#ifdef DM_DEBUG
   printf("dm_area_flush after waiting %i\n", status);
#endif
#ifdef OS_WINNT
   ss_semaphore_release(dm.sem_flush);      /* give it back now */
#endif

   return status;
#else
   /* full flush done here */
   /* select in order both area independently of being full or not */
   if (dm.area1.serial <= dm.area2.serial) {
      status = dm_buffer_send(&dm.area1);
      if (status != CM_SUCCESS)
         return status;
      status = dm_buffer_send(&dm.area2);
      if (status != CM_SUCCESS)
         return status;
   } else {
      status = dm_buffer_send(&dm.area2);
      if (status != CM_SUCCESS)
         return status;
      status = dm_buffer_send(&dm.area1);
      if (status != CM_SUCCESS)
         return status;
   }
   /* reset serial counter */
   dm.area1.serial = dm.area2.serial = 0;
   dm.last_active = ss_millitime();
   return CM_SUCCESS;
#endif
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
                  /** @} *//* end of dmfunctionc */

/***** sKIP eb_xxx **************************************************/
/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS
/***** sKIP eb_xxx **************************************************/

#if !defined(OS_VXWORKS)
/********************************************************************\
*                                                                    *
*                 Event buffer functions                             *
*                                                                    *
\********************************************************************/

/* PAA several modification in the eb_xxx()
   also new function eb_buffer_full()
*/
static char *_event_ring_buffer = NULL;
static INT _eb_size;
static char *_eb_read_pointer, *_eb_write_pointer, *_eb_end_pointer;

/********************************************************************/
INT eb_create_buffer(INT size)
/********************************************************************\

  Routine: eb_create_buffer

  Purpose: Create an event buffer. Has to be called initially before
           any other eb_xxx function

  Input:
    INT    size             Size in bytes

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion
    BM_NO_MEMEORY           Out of memory

\********************************************************************/
{
   _event_ring_buffer = (char *) M_MALLOC(size);
   if (_event_ring_buffer == NULL)
      return BM_NO_MEMORY;

   memset(_event_ring_buffer, 0, size);
   _eb_size = size;

   _eb_write_pointer = _eb_read_pointer = _eb_end_pointer = _event_ring_buffer;

   _send_sock = rpc_get_event_sock();

   return CM_SUCCESS;
}

/********************************************************************/
INT eb_free_buffer()
/********************************************************************\

  Routine: eb_free_buffer

  Purpose: Free memory allocated voa eb_create_buffer

  Input:
    none

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
   if (_event_ring_buffer)
      M_FREE(_event_ring_buffer);

   _eb_size = 0;
   return CM_SUCCESS;
}


/********************************************************************/
INT eb_free_space(void)
/********************************************************************\

  Routine: eb_free_space

  Purpose: Compute and return usable free space in the event buffer

  Input:
    none

  Output:
    none

  Function value:
    INT    Number of usable free bytes in the event buffer

\********************************************************************/
{
   INT free_space;

   if (_event_ring_buffer == NULL) {
      cm_msg(MERROR, "eb_get_pointer", "please call eb_create_buffer first");
      return -1;
   }

   if (_eb_write_pointer >= _eb_read_pointer) {
      free_space = _eb_size - ((POINTER_T) _eb_write_pointer - (POINTER_T) _event_ring_buffer);
   } else if (_eb_write_pointer >= _event_ring_buffer) {
      free_space = (POINTER_T) _eb_read_pointer - (POINTER_T) _eb_write_pointer;
   } else if (_eb_end_pointer == _event_ring_buffer) {
      _eb_write_pointer = _event_ring_buffer;
      free_space = _eb_size;
   } else if (_eb_read_pointer == _event_ring_buffer) {
      free_space = 0;
   } else {
      _eb_write_pointer = _event_ring_buffer;
      free_space = (POINTER_T) _eb_read_pointer - (POINTER_T) _eb_write_pointer;
   }

   return free_space;
}


/********************************************************************/
DWORD eb_get_level()
/********************************************************************\

  Routine: eb_get_level

  Purpose: Return filling level of event buffer in percent

  Input:
    none

  Output:
    none

  Function value:
    DWORD level              0..99

\********************************************************************/
{
   INT size;

   size = _eb_size - eb_free_space();

   return (100 * size) / _eb_size;
}


/********************************************************************/
BOOL eb_buffer_full(void)
/********************************************************************\

  Routine: eb_buffer_full

  Purpose: Test if there is sufficient space in the event buffer
    for another event

  Input:
    none

  Output:
    none

  Function value:
    BOOL  Is there enough space for another event in the event buffer

\********************************************************************/
{
   DWORD free_space;

   free_space = eb_free_space();

   /* if max. event won't fit, return zero */
   return (free_space < MAX_EVENT_SIZE + sizeof(EVENT_HEADER) + sizeof(INT));
}


/********************************************************************/
EVENT_HEADER *eb_get_pointer()
/********************************************************************\

  Routine: eb_get_pointer

  Purpose: Get pointer to next free location in event buffer

  Input:
    none

  Output:
    none

  Function value:
    EVENT_HEADER *            Pointer to free location

\********************************************************************/
{
   /* if max. event won't fit, return zero */
   if (eb_buffer_full()) {
#ifdef OS_VXWORKS
      logMsg("eb_get_pointer(): Event won't fit: read=%d, write=%d, end=%d\n",
             _eb_read_pointer - _event_ring_buffer,
             _eb_write_pointer - _event_ring_buffer, _eb_end_pointer - _event_ring_buffer, 0, 0, 0);
#endif
      return NULL;
   }

   /* leave space for buffer handle */
   return (EVENT_HEADER *) (_eb_write_pointer + sizeof(INT));
}


/********************************************************************/
INT eb_increment_pointer(INT buffer_handle, INT event_size)
/********************************************************************\

  Routine: eb_increment_pointer

  Purpose: Increment write pointer of event buffer after an event
           has been copied into the buffer (at an address previously
           obtained via eb_get_pointer)

  Input:
    INT buffer_handle         Buffer handle event should be sent to
    INT event_size            Event size in bytes including header

  Output:
    none

  Function value:
    CM_SUCCESS                Successful completion

\********************************************************************/
{
   INT aligned_event_size;

   /* if not connected remotely, use bm_send_event */
   if (_send_sock == 0)
      return bm_send_event(buffer_handle, _eb_write_pointer + sizeof(INT), event_size, BM_WAIT);

   aligned_event_size = ALIGN8(event_size);

   /* copy buffer handle */
   *((INT *) _eb_write_pointer) = buffer_handle;
   _eb_write_pointer += sizeof(INT) + aligned_event_size;

   if (_eb_write_pointer > _eb_end_pointer)
      _eb_end_pointer = _eb_write_pointer;

   if (_eb_write_pointer > _event_ring_buffer + _eb_size)
      cm_msg(MERROR, "eb_increment_pointer",
             "event size (%d) exceeds maximum event size (%d)", event_size, MAX_EVENT_SIZE);

   if (_eb_size - ((POINTER_T) _eb_write_pointer - (POINTER_T) _event_ring_buffer) <
       (int) (MAX_EVENT_SIZE + sizeof(EVENT_HEADER) + sizeof(INT))) {
      _eb_write_pointer = _event_ring_buffer;

      /* avoid rp==wp */
      if (_eb_read_pointer == _event_ring_buffer)
         _eb_write_pointer--;
   }

   return CM_SUCCESS;
}


/********************************************************************/
INT eb_send_events(BOOL send_all)
/********************************************************************\

  Routine: eb_send_events

  Purpose: Send events from the event buffer to the server

  Input:
    BOOL send_all             If FALSE, only send events if buffer
                              contains more than _opt_tcp_size bytes

  Output:
    none

  Function value:
    CM_SUCCESS                Successful completion

\********************************************************************/
{
   char *eb_wp, *eb_ep;
   INT size, i;
   INT opt_tcp_size = rpc_get_opt_tcp_size();

   /* write pointers are volatile, so make copy */
   eb_ep = _eb_end_pointer;
   eb_wp = _eb_write_pointer;

   if (eb_wp == _eb_read_pointer)
      return CM_SUCCESS;
   if (eb_wp > _eb_read_pointer) {
      size = (POINTER_T) eb_wp - (POINTER_T) _eb_read_pointer;

      /* don't send if less than optimal TCP buffer size available */
      if (size < opt_tcp_size && !send_all)
         return CM_SUCCESS;
   } else {
      /* send last piece of event buffer */
      size = (POINTER_T) eb_ep - (POINTER_T) _eb_read_pointer;
   }

   while (size > opt_tcp_size) {
      /* send buffer */
      i = send_tcp(_send_sock, _eb_read_pointer, opt_tcp_size, 0);
      if (i < 0) {
         printf("send_tcp() returned %d\n", i);
         cm_msg(MERROR, "eb_send_events", "send_tcp() failed");
         return RPC_NET_ERROR;
      }

      _eb_read_pointer += opt_tcp_size;
      if (_eb_read_pointer == eb_ep && eb_wp < eb_ep)
         _eb_read_pointer = _eb_end_pointer = _event_ring_buffer;

      size -= opt_tcp_size;
   }

   if (send_all || eb_wp < _eb_read_pointer) {
      /* send buffer */
      i = send_tcp(_send_sock, _eb_read_pointer, size, 0);
      if (i < 0) {
         printf("send_tcp() returned %d\n", i);
         cm_msg(MERROR, "eb_send_events", "send_tcp() failed");
         return RPC_NET_ERROR;
      }

      _eb_read_pointer += size;
      if (_eb_read_pointer == eb_ep && eb_wp < eb_ep)
         _eb_read_pointer = _eb_end_pointer = _event_ring_buffer;
   }

   /* Check for case where eb_wp = eb_ring_buffer - 1 */
   if (eb_wp < _event_ring_buffer && _eb_end_pointer == _event_ring_buffer) {
      return CM_SUCCESS;
   }

   if (eb_wp != _eb_read_pointer)
      return BM_MORE_EVENTS;

   return CM_SUCCESS;
}

#endif                          /* OS_VXWORKS  eb section */

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
                  /** @} *//* end of midasincludecode */

/********************************************************************\

  Name:         cnaf_callback.c
  Created by:   Stefan Ritt
  Created by:   moved here from mfe.c by Konstantin Olchanski

  Contents:     The system part of the MIDAS frontend. Has to be
                linked with user code to form a complete frontend

  $Id$

\********************************************************************/

#include <stdio.h>
#include <assert.h>
#include "midas.h"
#include "msystem.h"
#include "mcstd.h"

/*------------------------------------------------------------------*/

static int cnaf_debug = 0;

INT cnaf_callback(INT index, void *prpc_param[])
{
   DWORD cmd, b, c, n, a, f, *pdword, *size, *x, *q, dtemp;
   WORD *pword, *pdata, temp;
   INT i, count;

   /* Decode parameters */
   cmd = CDWORD(0);
   b = CDWORD(1);
   c = CDWORD(2);
   n = CDWORD(3);
   a = CDWORD(4);
   f = CDWORD(5);
   pdword = CPDWORD(6);
   pword = CPWORD(6);
   pdata = CPWORD(6);
   size = CPDWORD(7);
   x = CPDWORD(8);
   q = CPDWORD(9);

   /* determine repeat count */
   if (index == RPC_CNAF16)
      count = *size / sizeof(WORD);     /* 16 bit */
   else
      count = *size / sizeof(DWORD);    /* 24 bit */

   switch (cmd) {
    /*---- special commands ----*/

   case CNAF_INHIBIT_SET:
      cam_inhibit_set(c);
      break;
   case CNAF_INHIBIT_CLEAR:
      cam_inhibit_clear(c);
      break;
   case CNAF_CRATE_CLEAR:
      cam_crate_clear(c);
      break;
   case CNAF_CRATE_ZINIT:
      cam_crate_zinit(c);
      break;

   case CNAF_TEST:
      break;

   case CNAF:
      if (index == RPC_CNAF16) {
         for (i = 0; i < count; i++)
            if (f < 16)
               cam16i_q(c, n, a, f, pword++, (int *) x, (int *) q);
            else if (f < 24)
               cam16o_q(c, n, a, f, pword[i], (int *) x, (int *) q);
            else
               cam16i_q(c, n, a, f, &temp, (int *) x, (int *) q);
      } else {
         for (i = 0; i < count; i++)
            if (f < 16)
               cam24i_q(c, n, a, f, pdword++, (int *) x, (int *) q);
            else if (f < 24)
               cam24o_q(c, n, a, f, pdword[i], (int *) x, (int *) q);
            else
               cam24i_q(c, n, a, f, &dtemp, (int *) x, (int *) q);
      }

      break;

   case CNAF_nQ:
     if (index == RPC_CNAF16) {
       if (f < 16) {
	 cam16i_rq(c, n, a, f, &pword, count);
	 *size = (POINTER_T) pword - (POINTER_T) pdata;
       }
     } else {
       if (f < 16) {
	 cam24i_rq(c, n, a, f, &pdword, count);
	 *size = (POINTER_T) pdword - (POINTER_T) pdata;
       }
     }
     
     /* return reduced return size */
     break;
     
   default:
     printf("cnaf: Unknown command 0x%X\n", (unsigned int) cmd);
   }

   if (cnaf_debug) {
      if (index == RPC_CNAF16)
         printf("cmd=%d r=%d c=%d n=%d a=%d f=%d d=%X x=%d q=%d\n",
                (int) cmd, (int) count, (int) c, (int) n, (int) a, (int) f,
                (int) pword[0], (int) *x, (int) *q);
      else if (index == RPC_CNAF24)
         printf("cmd=%d r=%d c=%d n=%d a=%d f=%d d=%X x=%d q=%d\n",
                (int) cmd, (int) count, (int) c, (int) n, (int) a, (int) f,
                (int) pdword[0], (int) *x, (int) *q);
   }

   return RPC_SUCCESS;
}

void register_cnaf_callback(int debug)
{
   cnaf_debug = debug;
   /* register CNAF callback */
   cm_register_function(RPC_CNAF16, cnaf_callback);
   cm_register_function(RPC_CNAF24, cnaf_callback);
}

/* end file */

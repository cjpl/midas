/*********************************************************************

  Name:         lrs2373.c
  Created by:   Stefan Ritt

  Cotents:      Routines for LeCroy 2365 Octal Logic Matrix
                
  $Log$
  Revision 1.2  1998/10/12 12:18:57  midas
  Added Log tag in header


*********************************************************************/

#include <stdio.h>
#include <dos.h>
#include <string.h>

#include "midas.h"
#include "mcstd.h"

#include "lrs2365.h"

/*------------------------------------------------------------------*/

int lrs2365_set(int crate, int slot, WORD coeff[18])
/**********************************************************************\
   
   Description:

   Load the logic matrix of the LRS2365 with 18 coefficients according
   to the specifications in the manual. After loading, the matrix
   is verified.

\**********************************************************************/
{
int  i;
WORD data;

  /* Initialize 2365 */
  camc(crate, slot, 0, 9);

  /* Set mode control to zero (front-panel input) */
  camo(crate, slot, 3, 16, 0);

  /* Write coefficients */
  for (i=0 ; i<18 ; i++)
    camo(crate, slot, 0, 16, coeff[i]);

  /* validate coefficients */
  for (i=0 ; i<18 ; i++)
    {
    cami(crate, slot, 0, 0, &data);
    if (data != coeff[i])
      {
      cm_msg(MERROR, "lrs2365_set", "Error verifying coefficients: should be %d, read %d", coeff[i], data);
      return 0;
      }
    }

  return SUCCESS;
}

/********************************************************************\

  Name:         mcnaf.h
  Created by:   Stefan Ritt

  Contents:     Contains cnaf() callback function. If include by the
                user front-end and installed with cnaf_register(cnaf)
                in the frontend_init() routine, CAMAC functions can
                be called through RPC.

                The proper CAMAC driver has to be included prior to
                this file.

  Revision history
  ------------------------------------------------------------------
  date         by    modification
  ---------    ---   ------------------------------------------------
  12-MAR-98    SR    created


\********************************************************************/

/*-- CNAF callback -------------------------------------------------*/

void cnaf(DWORD branch, DWORD crate, DWORD n, DWORD a, DWORD f,
          DWORD *pdata, DWORD *q, DWORD *x)
{
  *q = *x = 0;

  if (f == CNAF_SET_INHIBIT)
    CAMAC_SET_INHIBIT();
  else if (f == CNAF_CLEAR_INHIBIT)
    CAMAC_CLEAR_INHIBIT();
  else if (f == CNAF_C_CYCLE)
    CAMAC_C_CYCLE();
  else if (f == CNAF_Z_CYCLE)
    CAMAC_Z_CYCLE();
  else if (f > 15)
    CAMO(n, a, f, *pdata);
  else
    CAMIQ32(n, a, f, pdata, x, q);
}

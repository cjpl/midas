/********************************************************************\

  Name:         lrs2365.h
  Created by:   Stefan Ritt

  Cotents:      Header file for LeCroy 2365 Octal Logic Matrix

  $Log$
  Revision 1.1  1999/12/20 10:18:23  midas
  Reorganized driver directory structure

  Revision 1.2  1998/10/12 12:18:57  midas
  Added Log tag in header


\********************************************************************/

int lrs2365_set(int crate, int slot, WORD coeff[18]);

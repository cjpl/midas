/********************************************************************\

  Name:         das1600.h
  Created by:   Stefan Ritt

  Contents:     DAS1600 device driver function declarations

  $Log$
  Revision 1.1  1999/12/20 10:18:19  midas
  Reorganized driver directory structure

  Revision 1.2  1998/10/12 12:18:56  midas
  Added Log tag in header


\********************************************************************/

INT das1600_ai(INT cmd, ...);
INT das1600_ao(INT cmd, ...);
INT das1600_di(INT cmd, ...);
INT das1600_do(INT cmd, ...);



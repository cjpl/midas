/********************************************************************\

  Name:         generic.h
  Created by:   Stefan Ritt

  Contents:     Generic Class Driver header file

  $Log$
  Revision 1.2  2000/03/02 21:54:02  midas
  Added offset in readout routines, added cmd_set_label

  Revision 1.1  1999/12/20 10:18:16  midas
  Reorganized driver directory structure

  Revision 1.2  1998/10/12 12:18:55  midas
  Added Log tag in header


\********************************************************************/

/* class driver routines */
INT cd_gen(INT cmd, PEQUIPMENT pequipment);
INT cd_gen_read(char *pevent, int);

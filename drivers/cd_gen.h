/********************************************************************\

  Name:         cd_gen.h
  Created by:   Stefan Ritt

  Contents:     Generic Class Driver header file

  $Log$
  Revision 1.2  1998/10/12 12:18:55  midas
  Added Log tag in header


\********************************************************************/

/* class driver routines */
INT cd_gen(INT cmd, PEQUIPMENT pequipment);
INT cd_gen_read(char *pevent);

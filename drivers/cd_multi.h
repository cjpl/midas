/********************************************************************\

  Name:         cd_multi.h
  Created by:   Stefan Ritt

  Contents:     Multimeter Class Driver Header File

  $Log$
  Revision 1.2  1998/10/12 12:18:56  midas
  Added Log tag in header


\********************************************************************/

/* class driver routines */
INT cd_multi(INT cmd, PEQUIPMENT pequipment);
INT cd_multi_read(char *pevent);

/********************************************************************\

  Name:         cd_hv.h
  Created by:   Stefan Ritt

  Contents:     High Voltage Class Driver

  $Log$
  Revision 1.2  1998/10/12 12:18:55  midas
  Added Log tag in header


\********************************************************************/

/* class driver routines */
INT cd_hv(INT cmd, PEQUIPMENT pequipment);
INT cd_hv_read(char *pevent);

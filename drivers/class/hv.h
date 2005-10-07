/********************************************************************\

  Name:         hv.h
  Created by:   Stefan Ritt

  Contents:     High Voltage Class Driver

  $Id:$

\********************************************************************/

/* class driver routines */
INT cd_hv(INT cmd, PEQUIPMENT pequipment);
INT cd_hv_read(char *pevent, int);

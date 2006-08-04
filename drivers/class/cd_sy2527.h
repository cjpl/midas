/********************************************************************\

  Name:         cd_sys2527.h
  Created by:   Stefan Ritt

  Contents:     Generic Class Driver header file

  $Id: generic.h 2753 2005-10-07 14:55:31Z ritt $

\********************************************************************/

/* class driver routines */
INT cd_sy2527(INT cmd, PEQUIPMENT pequipment);
INT cd_sy2527_read(char *pevent, int);

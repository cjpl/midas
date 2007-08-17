/********************************************************************\

  Name:         cd_rf.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     FGD / T2K Class Driver header file

  $Id: cd_rf.h 3390 2006-10-21 06:29:55Z amaudruz $

\********************************************************************/

/* class driver routines */
INT cd_rf(INT cmd, PEQUIPMENT pequipment);
INT cd_rf_read(char *pevent, int);

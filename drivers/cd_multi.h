/********************************************************************\

  Name:         cd_multi.h
  Created by:   Stefan Ritt

  Contents:     Multimeter Class Driver Header File


  Revision history
  ------------------------------------------------------------------
  date         by    modification
  ---------    ---   ------------------------------------------------
  26-MAR-97    SR    created


\********************************************************************/

/* class driver routines */
INT cd_multi(INT cmd, PEQUIPMENT pequipment);
INT cd_multi_read(char *pevent);

/********************************************************************\

  Name:         cd_hv.h
  Created by:   Stefan Ritt

  Contents:     High Voltage Class Driver


  Revision history
  ------------------------------------------------------------------
  date         by    modification
  ---------    ---   ------------------------------------------------
  26-MAR-97    SR    created


\********************************************************************/

/* class driver routines */
INT cd_hv(INT cmd, PEQUIPMENT pequipment);
INT cd_hv_read(char *pevent);

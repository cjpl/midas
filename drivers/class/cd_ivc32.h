/********************************************************************\

  Name:         cd_ivc32.h
  Created by:   Pierre-Andr Amaudruz

  Contents:     MSCB IVC32 for the LiXe Intermediate HV control
                Refer for documentation to:
                http://daq-plone.triumf.ca/HR/PERIPHERALS/IVC32.pdf 
  $Id: cd_ivc32.h$
\********************************************************************/

/* class driver routines */
INT cd_ivc32(INT cmd, PEQUIPMENT pequipment);
INT cd_ivc32_read(char *pevent, int);

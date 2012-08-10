/********************************************************************\

  Name:         psi_epics_beamline.h
  Created by:   Stefan Ritt

  Contents:     Channel Access device driver function declarations

  $Id: psi_epics_beamline.h 4021 2007-11-01 18:03:02Z amaudruz $

\********************************************************************/

#define DT_MAGNET      1
#define DT_SLIT        2
#define DT_BEAMBLOCKER 3
#define DT_PSA         4
#define DT_ACCEL       5

#define DT_READONLY  128

INT psi_epics_beamline(INT cmd, ...);

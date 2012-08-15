/********************************************************************\

  Name:         psi_epics_beamline.h
  Created by:   Stefan Ritt

  Contents:     Channel Access device driver function declarations

  $Id: psi_epics_beamline.h 4021 2007-11-01 18:03:02Z amaudruz $

\********************************************************************/

#define DT_DEVICE      1
#define DT_BEAMBLOCKER 2
#define DT_PSA         3
#define DT_SEPARATOR   4

INT psi_epics(INT cmd, ...);

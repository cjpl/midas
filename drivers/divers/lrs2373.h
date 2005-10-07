/********************************************************************\

  Name:         lrs2373.h
  Created by:   Stefan Ritt

  Cotents:      Header file for accessing LeCroy 2372 MLU

  $Id:$

\********************************************************************/

#define WRITE		16
#define READ		0

#define DATA		0
#define CAR		1
#define CCR		2

#define SET_INHIBIT	(1<<4)  /* R/W 5 */
#define SET_2373MODE	(1<<6)  /* R/W 7 */
#define SET_TRANSPARENT	(1<<3)  /* R/W 4 */

int lrs2373_set(int camac_slot, char *filename);

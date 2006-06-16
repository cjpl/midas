/********************************************************************\
  Name:         ces8210.h
  Created by:   P.-A. Amaudruz

  Contents:     CES CBD 8210 CAMAC Branch Driver 
                Following mvmestd for mcstd
                Based on the original CES8210 for VxWorks
                Initialy ported to VMIC by Chris Pearson
                Modified to follow the latest mvmestd by PAA
  $Id$
\********************************************************************/
#ifndef _CES8210_H_
#define _CES8210_H_

#define CBD8210_BASE       0x800000    /* camac base address */
#define MAX_CRATE                 8    /* Full branch */
#define CBD8210_CRATE_SIZE  0x10000    /* crate range */

#define CBDWL_D16      0x000002
#define CBDWL_D24      0x000000

/* Accessing these registers with the CAMAC address equivalent */
#define CSR_OFFSET     0x0000e800       /* camac control and status reg */
#define CAR_OFFSET     0x0000e820       /* Crate Address Register */
#define BTB_OFFSET     0x0000e824       /* on-line status (R) */
#define INT_OFFSET     0x0000e810
#define GL             0x0000e828       /* GL register */

/* Control and Status Register */
#define IT4	0x0001          /* front panel int IT4 status */
#define IT2	0x0002          /* front panel int IT2 status */
#define MIT4	0x0004          /* IT4 mask 0 = enabled */
#define MIT2	0x0008          /* IT2 mask 0 = enabled */

#endif
/*-----------------------------------------------------------------*/

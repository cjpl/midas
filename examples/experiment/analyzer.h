/********************************************************************\

  Name:         analyzer.h
  Created by:   Stefan Ritt

  Contents:     Analyzer global include file

  $Log$
  Revision 1.4  2004/01/08 08:40:08  midas
  Implemented standard indentation

  Revision 1.3  2003/04/23 15:08:57  midas
  Decreased N_TDC to 4

  Revision 1.2  1998/10/12 12:18:58  midas
  Added Log tag in header


\********************************************************************/

/*-- Parameters ----------------------------------------------------*/

/* number of channels */
#define N_ADC               4
#define N_TDC               4
#define N_SCLR              4

/*-- Histo ID bases ------------------------------------------------*/

#define ADCCALIB_ID_BASE 1000
#define ADCSUM_ID_BASE   2000

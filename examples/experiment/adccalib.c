/********************************************************************\

  Name:         adccalib.c
  Created by:   Stefan Ritt

  Contents:     Example analyzer module for ADC calibration. Looks
                for ADC0 bank, subtracts pedestals and applies gain
                calibration. The resulting values are appended to 
                the event as an CADC bank ("calibrated ADC"). The
                pedestal values and software gains are stored in
                adccalib_param structure which was defined in the ODB
                and transferred to experim.h.

  $Log$
  Revision 1.4  2003/04/21 03:51:41  olchansk
  kludge misunderstanding of the ADC0 bank size.

  Revision 1.3  2002/05/10 05:22:47  pierre
  add MANA_LITE #ifdef

  Revision 1.2  1998/10/12 12:18:58  midas
  Added Log tag in header


\********************************************************************/
                                                    
/*-- Include files -------------------------------------------------*/

/* standard includes */
#include <stdio.h>
#include <time.h>

/* midas includes */
#include "midas.h"
#include "experim.h"
#include "analyzer.h"

/* cernlib includes */
#ifdef OS_WINNT
#define VISUAL_CPLUSPLUS
#endif

#ifdef __linux__
#define f2cFortran
#endif

#ifndef MANA_LITE
#include <cfortran.h>
#include <hbook.h>
#endif

/*-- Parameters ----------------------------------------------------*/

ADC_CALIBRATION_PARAM adccalib_param;
extern EXP_PARAM      exp_param;
extern RUNINFO        runinfo;

/*-- Module declaration --------------------------------------------*/

INT adc_calib(EVENT_HEADER*,void*);
INT adc_calib_init(void);
INT adc_calib_bor(INT run_number);
INT adc_calib_eor(INT run_number);

ADC_CALIBRATION_PARAM_STR(adc_calibration_param_str);

ANA_MODULE adc_calib_module = {
  "ADC calibration",             /* module name           */  
  "Stefan Ritt",                 /* author                */
  adc_calib,                     /* event routine         */
  adc_calib_bor,                 /* BOR routine           */
  adc_calib_eor,                 /* EOR routine           */
  adc_calib_init,                /* init routine          */
  NULL,                          /* exit routine          */
  &adccalib_param,               /* parameter structure   */
  sizeof(adccalib_param),        /* structure size        */
  adc_calibration_param_str,     /* initial parameters    */
};

/*-- init routine --------------------------------------------------*/

INT adc_calib_init(void)
{
  /* book histos */
  adc_calib_bor(0);
  return SUCCESS;
}

/*-- BOR routine ---------------------------------------------------*/

#define ADC_N_BINS   500
#define ADC_X_LOW      0
#define ADC_X_HIGH  4000

INT adc_calib_bor(INT run_number)
{
int    i;
char   str[80];

#ifdef MANA_LITE
 printf("manalite: adc_calib_bor: HBOOK disable\n");
#else
  /* book CADC histos */
  for (i=0; i<N_ADC; i++)
    {
    if (HEXIST(ADCCALIB_ID_BASE+i))
      HDELET(ADCCALIB_ID_BASE+i);
    sprintf(str, "CADC%02d", i);
    HBOOK1(ADCCALIB_ID_BASE+i, str, ADC_N_BINS, 
           (float)ADC_X_LOW, (float)ADC_X_HIGH, 0.f); 
    }
#endif

  return SUCCESS;
}

/*-- eor routine ---------------------------------------------------*/

INT adc_calib_eor(INT run_number)
{
  return SUCCESS;
}

/*-- event routine -------------------------------------------------*/

INT adc_calib(EVENT_HEADER *pheader, void *pevent)
{
INT    i, n_adc;
WORD   *pdata;
float  *cadc;

  /* look for ADC0 bank, return if not present */
  n_adc = bk_locate(pevent, "ADC0", &pdata);
  if (n_adc == 0)
    return 1;

  /* if ADC0 is a structured bank, bk_locate()
     returns sizeof(structure) rather than
     the number of ADCs. Hence this kludge */
  n_adc = n_adc/sizeof(WORD);

  /* create calibrated ADC bank */
  bk_create(pevent, "CADC", TID_FLOAT, &cadc);

  /* zero cadc bank */
  for (i=0 ; i<N_ADC ; i++)
    cadc[i] = 0.f;

  /* subtract pedestal */
  for (i=0 ; i<n_adc ; i++)
    cadc[i] = (float) ((double)pdata[i] - adccalib_param.pedestal[i] + 0.5);

  /* apply software gain calibration */
  for (i=0 ; i<n_adc ; i++)
    cadc[i] *= adccalib_param.software_gain[i];

#ifdef MANA_LITE
 printf("manalite: adc_calib: HBOOK disable\n");
#else
  /* fill ADC histos if above threshold */
  for (i=0 ; i<n_adc ; i++)
    if (cadc[i] > (float) adccalib_param.histo_threshold)
      HF1(ADCCALIB_ID_BASE+i, cadc[i], 1.f);
#endif

  /* close calculated bank */
  bk_close(pevent, cadc+n_adc);

  return SUCCESS;
}

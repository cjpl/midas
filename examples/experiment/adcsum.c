/********************************************************************\

  Name:         adcsum.c
  Created by:   Stefan Ritt

  Contents:     Example analyzer module for ADC summing. This module
                looks for a bank named CADC and produces an ASUM
                bank which contains the sum of all ADC values. The
                ASUM bank is a "structured" bank. It has been defined
                in the ODB and transferred to experim.h.

  $Log$
  Revision 1.7  2003/04/25 14:49:46  midas
  Removed HBOOK code

  Revision 1.6  2003/04/23 15:09:29  midas
  Added 'average' in ASUM bank

  Revision 1.5  2003/04/22 15:00:27  midas
  Worked on ROOT histo booking

  Revision 1.4  2003/04/21 04:02:13  olchansk
  replace MANA_LITE with HAVE_HBOOK and HAVE_ROOT
  implement ROOT-equivalents of the sample HBOOK code
  implement example of adding custom branches to the analyzer root output file

  Revision 1.3  2002/05/10 05:22:59  pierre
  add MANA_LITE #ifdef

  Revision 1.2  1998/10/12 12:18:58  midas
  Added Log tag in header


\********************************************************************/
                                                        
/*-- Include files -------------------------------------------------*/

/* standard includes */
#include <stdio.h>
#include <math.h>

/* midas includes */
#include "midas.h"
#include "experim.h"
#include "analyzer.h"

/* foot includes */
#include <TH1F.h>
#include <TDirectory.h>

#ifndef PI
#define PI 3.14159265359
#endif

/*-- Parameters ----------------------------------------------------*/

ADC_SUMMING_PARAM adc_summing_param;

/*-- Module declaration --------------------------------------------*/

INT adc_summing(EVENT_HEADER*,void*);
INT adc_summing_init(void);
INT adc_summing_bor(INT run_number);

ADC_SUMMING_PARAM_STR(adc_summing_param_str);

ANA_MODULE adc_summing_module = {
  "ADC summing",                 /* module name           */  
  "Stefan Ritt",                 /* author                */
  adc_summing,                   /* event routine         */
  NULL,                          /* BOR routine           */
  NULL,                          /* EOR routine           */
  adc_summing_init,              /* init routine          */
  NULL,                          /* exit routine          */
  &adc_summing_param,            /* parameter structure   */
  sizeof(adc_summing_param),     /* structure size        */
  adc_summing_param_str,         /* initial parameters    */
};

/*-- Module-local variables-----------------------------------------*/

#ifdef HAVE_ROOT
extern TDirectory *gManaHistsDir;

static TH1F *gAdcSumHist;
#endif

/*-- init routine --------------------------------------------------*/

INT adc_summing_init(void)
{
  /* book sum histo */
  gAdcSumHist = (TH1F*)gManaHistsDir->GetList()->FindObject("ADCSUM");
    
  if (gAdcSumHist == NULL)
    gAdcSumHist = new TH1F("ADCSUM", "ADC sum", 500, 0, 10000);

  return SUCCESS;
}

/*-- event routine -------------------------------------------------*/

INT adc_summing(EVENT_HEADER *pheader, void *pevent)
{
INT          i, j, n_adc;
float        *cadc;
ASUM_BANK    *asum;

  /* look for CADC bank, return if not present */
  n_adc = bk_locate(pevent, "CADC", &cadc);
  if (n_adc == 0)
    return 1;

  /* create ADC sum bank */
  bk_create(pevent, "ASUM", TID_STRUCT, &asum);

  /* sum all channels above threashold */
  asum->sum = 0.f;
  for (i=j=0 ; i<n_adc ; i++)
    if (cadc[i] > adc_summing_param.adc_threshold)
      {
      asum->sum += cadc[i];
      j++;
      }

  /* calculate ADC average */
    asum->average = j > 0 ? asum->sum / j : 0;

  /* fill sum histo */
  gAdcSumHist->Fill(asum->sum,1);

  /* close calculated bank */
  bk_close(pevent, asum+1);
  
  return SUCCESS;
}

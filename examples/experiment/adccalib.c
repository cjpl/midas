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
  Revision 1.5  2003/04/21 04:02:13  olchansk
  replace MANA_LITE with HAVE_HBOOK and HAVE_ROOT
  implement ROOT-equivalents of the sample HBOOK code
  implement example of adding custom branches to the analyzer root output file

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

#ifdef HAVE_HBOOK
#include <cfortran.h>
#include <hbook.h>
#endif

#ifdef HAVE_ROOT
#include <TH1F.h>
#include <TTree.h>
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

/*-- module-local variables ----------------------------------------*/

#ifdef HAVE_ROOT
static TH1F* gAdcHists[N_ADC];

static char* gMyBranchString = "run_number/I:n_adc/I:adc_sum/F";
static struct
{
  int   run_number;       // run number
  int   n_adc;            // number of ADCs
  float adc_sum;          // sum of calibrated ADCs

} gMyBranchData;

static int   gAdcRawData[N_ADC];
static float gAdcCalibData[N_ADC];

#endif

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

  /* book CADC histos */
#ifdef HAVE_HBOOK
  for (i=0; i<N_ADC; i++)
    {
    char str[80];
    if (HEXIST(ADCCALIB_ID_BASE+i))
      HDELET(ADCCALIB_ID_BASE+i);
    sprintf(str, "CADC%02d", i);
    HBOOK1(ADCCALIB_ID_BASE+i, str, ADC_N_BINS, 
           (float)ADC_X_LOW, (float)ADC_X_HIGH, 0.f); 
    }
#endif /* HAVE_HBOOK */
#ifdef HAVE_ROOT
  for (i=0; i<N_ADC; i++)
    {
    char name[256];
    char title[256];
    if (gAdcHists[i] != NULL)
      delete gAdcHists[i];
    sprintf(name,  "CADC%02d", i);
    sprintf(title, "ADC %d", i);
    gAdcHists[i] = new TH1F(name, title, ADC_N_BINS, ADC_X_LOW, ADC_X_HIGH);
    }
#endif /* HAVE_ROOT */

#ifdef HAVE_ROOT
  // create a custom branch in the output file
  extern TTree* gManaOutputTree;

  // create the branches only if there is an output file and
  // if the output tree exists.
  //
  // also only create branches is they do not exist: in append
  // mode the output file is not reopened and the output
  // tree with it's branches persists across calls to bor() and eor().
  
  if (gManaOutputTree != NULL)
    {
    if (gManaOutputTree->GetBranch("MyBranch")==NULL)
      gManaOutputTree->Branch("MyBranch",&gMyBranchData,gMyBranchString);
    gMyBranchData.run_number = run_number;

    if (gManaOutputTree->GetBranch("AdcRaw")==NULL)
      gManaOutputTree->Branch("AdcRaw",  &gAdcRawData,  "adc_raw[n_adc]/I");
    if (gManaOutputTree->GetBranch("AdcCalib")==NULL)
      gManaOutputTree->Branch("AdcCalib",&gAdcCalibData,"adc_calib[n_adc]/F");
    }
#endif /* HAVE_ROOT */

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

  /* fill ADC histos if above threshold */
#ifdef HAVE_HBOOK
  for (i=0 ; i<n_adc ; i++)
    if (cadc[i] > (float) adccalib_param.histo_threshold)
      HF1(ADCCALIB_ID_BASE+i, cadc[i], 1.f);
#endif /* HAVE_HBOOK */
#ifdef HAVE_ROOT
  for (i=0 ; i<n_adc ; i++)
    if (cadc[i] > (float) adccalib_param.histo_threshold)
      gAdcHists[i]->Fill(cadc[i], 1);
#endif /* HAVE_ROOT */

#ifdef HAVE_ROOT
  // Fill the custom branch
  gMyBranchData.n_adc = n_adc;
  gMyBranchData.adc_sum = 0;
  for (i=0 ; i<n_adc ; i++)
    {
    gAdcRawData[i]   = pdata[i];
    gAdcCalibData[i] = cadc[i];
    gMyBranchData.adc_sum += cadc[i];
    }
#endif /* HAVE_ROOT */

  /* close calculated bank */
  bk_close(pevent, cadc+n_adc);

  return SUCCESS;
}

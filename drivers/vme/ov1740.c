/*********************************************************************

  Name:         ov1740.c
  Created by:   Pierre-A. Amaudruz / K.Olchanski
                    implementation of the CAENCommLib functions
  Contents:     V1740 64ch. 12bit 50Msps for Optical link
  MAIN_ENABLE Build : ov1740.c, v1740.h, ov1740drv.h
   > gcc -g -O2 -Wall -DDO_TIMING -DMAIN_ENABLE -o ov1740.exe ov1740.c -lCAENComm -lrt 
  Operation:
  > ./ov1740 -l 100 -l 0 -b 0
  > ./ov1740 -l 10000 -m 100 -l 1 -b 0

  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "ov1740drv.h"

#define LARGE_NUMBER 10000000
// Buffer organization map for number of samples
uint32_t V1740_NSAMPLES_MODE[11] = { (1<<20), (1<<19), (1<<18), (1<<17), (1<<16), (1<<15)
              ,(1<<14), (1<<13), (1<<12), (1<<11), (1<<10)};

/*****************************************************************/
CAENComm_ErrorCode ov1740_GroupSet(int handle, uint32_t channel, uint32_t what, uint32_t that)
{
  uint32_t reg;

  reg = what | (channel << 8);
  return CAENComm_Write32(handle, reg, (that & 0xFFF));
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_GroupGet(int handle, uint32_t channel, uint32_t what, uint32_t *data)
{
  uint32_t reg;

  reg = what | (channel << 8);
  return CAENComm_Read32(handle, reg, data);
}


/*****************************************************************/
CAENComm_ErrorCode ov1740_GroupThresholdSet(int handle, uint32_t channel, uint32_t threshold)
{
  uint32_t reg;
  reg = V1740_GROUP_THRESHOLD | (channel << 8);
  printf("reg:0x%x, threshold:%x\n", reg, threshold);
  return CAENComm_Write32(handle, reg,(threshold & 0xFFF));
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_GroupDACSet(int handle, uint32_t channel, uint32_t dac)
{
  uint32_t reg, status, ncount;
  
  if ((channel >= 0) && (channel < 8)) {
    reg = V1740_GROUP_STATUS | (channel << 8);
    ncount = 10000;
    do {
      CAENComm_Read32(handle, reg, &status);
    } while ((status & 0x04) && (ncount--));
    if (ncount == 0) return -1;
    reg = V1740_GROUP_DAC | (channel << 8);
    printf("reg:0x%x, DAC:%x\n", reg, dac);
    return CAENComm_Write32(handle, reg, (dac & 0xFFFF));
  }
  return -1;
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_GroupDACGet(int handle, uint32_t channel, uint32_t *dac)
{
  uint32_t reg;
  CAENComm_ErrorCode sCAEN = -1;

  if ((channel >= 0) && (channel < 8)) {
    reg = V1740_GROUP_DAC | (channel << 8);
    sCAEN = CAENComm_Read32(handle, reg, dac);
  }
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_AcqCtl(int handle, uint32_t operation)
{
  uint32_t reg;
  CAENComm_ErrorCode sCAEN;
  
  sCAEN = CAENComm_Read32(handle, V1740_ACQUISITION_CONTROL, &reg);
  //  printf("sCAEN:%d ACQ Acq Control:0x%x\n", sCAEN, reg);

  switch (operation) {
  case V1740_RUN_START:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, (reg | 0x4));
    break;
  case V1740_RUN_STOP:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, (reg & ~( 0x4)));
    break;
  case V1740_REGISTER_RUN_MODE:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, 0x0);
    break;
  case V1740_SIN_RUN_MODE:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, 0x1);
    break;
  case V1740_SIN_GATE_RUN_MODE:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, 0x2);
    break;
  case V1740_MULTI_BOARD_SYNC_MODE:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, 0x3);
    break;
  case V1740_COUNT_ACCEPTED_TRIGGER:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, (reg & ~( 0x8)));
    break;
  case V1740_COUNT_ALL_TRIGGER:
    sCAEN = CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL, (reg | 0x8));
    break;
  default:
    printf("operation not defined\n");
    break;
  }
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_GroupConfig(int handle, uint32_t operation)
{
  CAENComm_ErrorCode sCAEN;
  uint32_t reg, cfg;
  
  sCAEN = CAENComm_Read32(handle, V1740_GROUP_CONFIG, &reg);  
  sCAEN = CAENComm_Read32(handle, V1740_GROUP_CONFIG, &cfg);  
  //  printf("Group_config1: 0x%x\n", cfg);  

  switch (operation) {
  case V1740_TRIGGER_UNDERTH:
    sCAEN = CAENComm_Write32(handle, V1740_GROUP_CFG_BIT_SET, 0x40);
    break;
  case V1740_TRIGGER_OVERTH:
    sCAEN = CAENComm_Write32(handle, V1740_GROUP_CFG_BIT_CLR, 0x40);
    break;
  default:
    break;
  }
  sCAEN = CAENComm_Read32(handle, V1740_GROUP_CONFIG, &cfg);  
  //  printf("Group_config2: 0x%x\n", cfg);
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_info(int handle, int *nchannels, uint32_t *data)
{
  CAENComm_ErrorCode sCAEN;
  int i, chanmask;
  uint32_t reg;

  // Evaluate the event size
  // Number of samples per channels
  sCAEN = CAENComm_Read32(handle, V1740_BUFFER_ORGANIZATION, &reg);  
  *data = V1740_NSAMPLES_MODE[reg];

  // times the number of active channels
  sCAEN = CAENComm_Read32(handle, V1740_GROUP_EN_MASK, &reg);  
  chanmask = 0xff & reg; 
  *nchannels = 0;
  for (i=0;i<8;i++) {
    if (chanmask & (1<<i))
      *nchannels += 1;
  }

  *data *= *nchannels;
  *data /= 2;   // 2 samples per 32bit word
  *data += 4;   // Headers
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_BufferOccupancy(int handle, uint32_t channel, uint32_t *data)
{
  uint32_t reg;

  reg = V1740_GROUP_BUFFER_OCCUPANCY + (channel<<16);
  return  CAENComm_Read32(handle, reg, data);  
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_BufferFree(int handle, int nbuffer, uint32_t *mode)
{
  CAENComm_ErrorCode sCAEN;

  sCAEN = CAENComm_Read32(handle, V1740_BUFFER_ORGANIZATION, mode);  
  if (nbuffer <= (1 << *mode) ) {
    sCAEN = CAENComm_Write32(handle,V1740_BUFFER_FREE, nbuffer);
    return sCAEN;
  } else
    return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1740_Status(int handle)
{
  CAENComm_ErrorCode sCAEN;
  uint32_t reg;

  printf("================================================\n");
  sCAEN = CAENComm_Read32(handle, V1740_BOARD_ID, &reg);  
  printf("Board ID             : 0x%x\n", reg);
  sCAEN = CAENComm_Read32(handle, V1740_BOARD_INFO, &reg);  
  printf("Board Info           : 0x%x\n", reg);
  sCAEN = CAENComm_Read32(handle, V1740_ACQUISITION_CONTROL, &reg);  
  printf("Acquisition control  : 0x%8.8x\n", reg);
  sCAEN = CAENComm_Read32(handle, V1740_ACQUISITION_STATUS, &reg);  
  printf("Acquisition status   : 0x%8.8x\n", reg);
  printf("================================================\n");
  return sCAEN;
}

/*****************************************************************/
/**
Sets all the necessary paramters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
- <b>Mode 1</b> : 

@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@return 0: OK. -1: Bad
*/
CAENComm_ErrorCode  ov1740_Setup(int handle, int mode)
{
  switch (mode) {
  case 0x0:
    printf("--------------------------------------------\n");
    printf("Setup Skip\n");
    printf("--------------------------------------------\n");
  case 0x1:
    printf("--------------------------------------------\n");
    printf("Trigger from FP, 8ch, 1Ks, postTrigger 800\n");
    printf("--------------------------------------------\n");
    CAENComm_Write32(handle, V1740_BUFFER_ORGANIZATION,   0x0A);    // 1K buffer
    CAENComm_Write32(handle, V1740_TRIG_SRCE_EN_MASK,     0x4000);  // External Trigger
    CAENComm_Write32(handle, V1740_GROUP_EN_MASK,       0xFF);    // 8ch enable
    CAENComm_Write32(handle, V1740_POST_TRIGGER_SETTING,  800);     // PreTrigger (1K-800)
    CAENComm_Write32(handle, V1740_ACQUISITION_CONTROL,   0x00);    // Reset Acq Control
    printf("\n");
    break;
  case 0x2:
    printf("--------------------------------------------\n");
    printf("Trigger from LEMO\n");
    printf("--------------------------------------------\n");
    CAENComm_Write32(handle, V1740_BUFFER_ORGANIZATION, 1);
    printf("\n");
    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  return ov1740_Status(handle);
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  CAENComm_ErrorCode sCAEN;
  int handle[2];
  int nw, l=0, d=0, h=0, Nh;
  uint32_t i, lcount, data[50000], temp, lam;
  int Nmodulo=10;
  int tcount=0, eloop=0;
  uint32_t   *pdata, eStored, eSize;
  int loop, Nloop=10;
  int bshowData=0;
  int debug = 0;
  uint32_t pct=0, ct;
  struct timeval t1;
  int   dt1, savelcount=0;

   /* get parameters */
   /* parse command line parameters */
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = 1;
    else if (strncmp(argv[i], "-s", 2) == 0)
      bshowData = 1;
    else if (argv[i][0] == '-') {
      if (i + 1 >= argc || argv[i + 1][0] == '-')
	goto usage;
      if (strncmp(argv[i], "-l", 2) == 0)
	Nloop =  (atoi(argv[++i]));
      else if (strncmp(argv[i], "-o", 2) == 0)
	l =  (atoi(argv[++i]));
      else if (strncmp(argv[i], "-b", 2) == 0)
	d =  (atoi(argv[++i]));
      else if (strncmp(argv[i], "-m", 2) == 0)
	Nmodulo =  (atoi(argv[++i]));
      else if (strncmp(argv[i], "-d", 2) == 0)
	d =  (atoi(argv[++i]));
    } else {
    usage:
      printf("usage: ov1740 -l (loop count) \n");
      printf("              -o link#\n");
      printf("              -b board#\n");
      printf("              -d daisy#\n");
      printf("              -m modulo display\n");
      printf("              -s show data\n\n");
      return 0;
         }
  }
  
  printf("in ov1740\n");
  
#if 1
  //
  // Open devices
  sCAEN = CAENComm_OpenDevice(CAENComm_PCIE_OpticalLink, l, d, 0, &(handle[h])); 
  if (sCAEN != CAENComm_Success) {
    handle[h] = -1;
    printf("CAENComm_OpenDevice [l:%d, d:%d]: Error %d\n", l, d, sCAEN);
  } else {
    printf("Device found : Link:%d  Daisy:%d Handle[%d]:%d\n", l, d, h, handle[h]);
    sCAEN = ov1740_Status(handle[h]);
    h++;
  }
  Nh = h;
  printf("Handles opened (%d)\n", Nh);
#endif
#if 0
  //
  // Open devices
  for (h=0, l=0;l<1;l++) {
    for (d=0;d<2;d++) {
      // Open VME interface   
      sCAEN = CAENComm_OpenDevice(CAENComm_PCIE_OpticalLink, l, d, 0, &(handle[h])); 
      if (sCAEN != CAENComm_Success) {
	handle[h] = -1;
	printf("CAENComm_OpenDevice [l:%d, d:%d]: Error %d\n", l, d, sCAEN);
      } else {
	printf("Device found : Link:%d  Daisy:%d Handle[%d]:%d\n", l, d, h, handle[h]);
	sCAEN = ov1740_Status(handle[h]);
	h++;
      }
    }
  }
  Nh = h;
  printf("Handles opened (%d)\n", Nh);
#endif
#if 1
  for (h=0;h<Nh;h++) {
    sCAEN = CAENComm_Write32(handle[h], V1740_SW_RESET              , 0);
    
    sCAEN = ov1740_AcqCtl(handle[h], 0x3);
    sCAEN = CAENComm_Write32(handle[h], V1740_GROUP_CONFIG        , 0x10);
    sCAEN = CAENComm_Write32(handle[h], V1740_BUFFER_ORGANIZATION   , 0xa);
    sCAEN = CAENComm_Write32(handle[h], V1740_GROUP_EN_MASK       , 0x3);
    sCAEN = CAENComm_Write32(handle[h], V1740_TRIG_SRCE_EN_MASK     , 0x400000FF);
    sCAEN = CAENComm_Write32(handle[h], V1740_MONITOR_MODE          , 0x3);  // buffer occupancy
    // 
    // Set Group threshold
    for (i=0;i<8;i++) {
      ov1740_GroupSet(handle[h], i, V1740_GROUP_THRESHOLD, 0x820);
      sCAEN = ov1740_GroupGet(handle[h], i, V1740_GROUP_THRESHOLD, &temp);
      printf("Board: %d Threshold[%i] = %d \n", h, i, temp);
    }

    //
    // Set DAC value
    for (i=0;i<8;i++) {
      sCAEN = ov1740_GroupDACSet(handle[h], i, 0x88b8);
      sCAEN = ov1740_GroupDACGet(handle[h], i, &temp);
      printf("Board :%d DAC[%i] = %d \n", h, i, temp);
    }
    
  }
  printf("Modules configured\n");


  // Start boards
  for (h=0;h<Nh;h++) {
    if (handle[h] < 0) continue;   // Skip unconnected board
    // Start run then wait for trigger
    sCAEN = CAENComm_Write32(handle[h], V1740_SW_CLEAR, 0);
    sleep(1);
    ov1740_AcqCtl(handle[h], V1740_RUN_START);
  }  
  
  printf("Modules started\n");
 
  for (loop=0;loop<Nloop;loop++) {
    do {
      sCAEN = CAENComm_Read32(handle[0], V1740_ACQUISITION_STATUS, &lam);
      printf("lam:%x, sCAEN:%d\n",lam, sCAEN);
      lam &= 0x4;
    } while (lam == 0);

    // Read all modules
    for (h=0; h<Nh; h++) {
      // Skip unconnected devices
      if (handle[h] < 0) continue;   // Skip unconnected board
      
      // Check if data ready
      lcount=LARGE_NUMBER;
      do {
	sCAEN = CAENComm_Read32(handle[h], V1740_VME_STATUS, &lam);
	lam &= 0x1;
      } while ((lam==0) && (lcount--));
      if (h==1) savelcount = LARGE_NUMBER - lcount;
      if (lcount == 0) {
	printf("timeout on LAM for module %d\n", h);
	goto out;
      }
      // buffer info
      sCAEN = CAENComm_Read32(handle[h], V1740_EVENT_STORED, &eStored);
      sCAEN = CAENComm_Read32(handle[h], V1740_EVENT_SIZE, &eSize);
      
      // Some info display
      if ((h==0) && ((loop % Nmodulo) == 0)) {
	gettimeofday(&t1, NULL);
	ct = t1.tv_sec * 1e6 + t1.tv_usec;
	dt1 = ct-pct;
	pct = ct;
	printf("B:%02d Hndle:%d sCAEN:%d Evt#:%d Event Stored:0x%x Event Size:0x%x try:%d KB/s:%6.2f BLTl:%d\n"
	       , h, handle[h], sCAEN, loop, eStored, eSize, savelcount, (float) 1e3*tcount/dt1, eloop);
	tcount = 0;
      }
      
      // Read data
      pdata = &data[0];
      eloop = 0;
      do {
	sCAEN = CAENComm_BLTRead(handle[h], V1740_EVENT_READOUT_BUFFER, pdata, eSize < 1028 ? eSize : 1028, &nw);
	eSize -= nw;
	pdata += nw;
	tcount += nw;  // debugging
	eloop++;       // debugging
      } while (eSize);
      
      if (bshowData) printf("Module:%d nw:%d data: 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n"
			    ,h , nw, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    } // loop over Nh
    
    //    printf("Modules all readout\n");
  }

#endif

#if 0
  Address = V1740_EVENT_READOUT_BUFFER;
  CAENComm_MultiRead32(handle, &Address, status/4, &(data[0]), &sCAEN);
  printf("sCAEN:%d MultiRead32 length:0x%x\n", sCAEN, status/4);
#endif

#if 0
  // SCAEN returns -13 when no more data, nw on the -13 returns the remain
  Address = V1740_EVENT_READOUT_BUFFER;
  for (i=0;i<32;i++) {
    sCAEN = CAENComm_BLTRead(handle, Address, &(data[0]), 512, &nw);
    printf("sCAEN:%d BLTRead length:0x%x\n", sCAEN, nw);
    if (nw == 0) break;
  }
#endif
  
#if 0
  Address = V1740_EVENT_READOUT_BUFFER;
  for (i=0;i<32;i++) {
    sCAEN = CAENComm_MBLTRead(handle, Address, &(data[0]), 128, &nw);
    printf("sCAEN:%d MBLTRead length:0x%x\n", sCAEN, nw);
    if (nw == 0) break;
  }
#endif
  
#if 0
  for (i=0;i<nw;i++) {
    printf(" Data[%d] = 0x%8.8x\n", i, data[i]);
  }
#endif

  // Exit port
 out:
  for (h=0;h<Nh;h++) {
    sCAEN = ov1740_AcqCtl(handle[h], V1740_RUN_STOP);
  }
  printf("Modules stopped\n");
#if 0
  sleep(5);
  printf("Sleep sCAEN:%d\n", sCAEN);
  sCAEN = CAENComm_Read32(handle[h], V1740_EVENT_SIZE, &eSize);
  sCAEN = CAENComm_BLTRead(handle[h], V1740_EVENT_READOUT_BUFFER, pdata, eSize, &nw);
  printf("read sCAEN:%d\n", sCAEN);
#endif

  for (h=0;h<Nh;h++) {
    sCAEN = CAENComm_CloseDevice(handle[h]); 
  }
  printf("Handles released\n");

  return 1;

}
#endif

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

//end

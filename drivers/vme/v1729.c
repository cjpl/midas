/*********************************************************************

  Name:         v1792.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     V1729 4channel /12 bit sampling ADC (1,2Gsps)

  $Log: v1729.c,v $
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "v1729.h"

int debug = 1;
static float  Corr_vernier[4];
static int    trig_rec, end_cell, ncol, get_ref=1;
static WORD   post_trig;
static float  pedestals[V1729_RAM_SIZE], ped[4][V1729_MAX_CHANNEL_SIZE+3];
static float  pedestalsRms[V1729_RAM_SIZE];
static int    ped_ok = 0;
static int    lMINVER, lMAXVER;
static int    vernier_ok = 0;

/*****************************************************************/
/*****************************************************************/
void v1729_AcqStart(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_INTERRUPT_REG, 0);
  mvme_write_value(mvme, base+V1729_ACQ_START_W, 0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1729_Reset(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_RESET_W, 0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
Sets all the necessary paramters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@param *nentry number of entries requested and returned.
@return MVME_SUCCESS
*/
int  v1729_Setup(MVME_INTERFACE *mvme, DWORD base, int mode)
{
  int      cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  switch (mode) {
  case 0x1:
    printf("4 channels 1Gsps External Trigger(mode:%d)\n", mode);
    mvme_write_value(mvme, base+V1729_INTERRUPT_ENABLE, 0);
    v1729_PreTrigSet(mvme, base, 0x4000);
    v1729_PostTrigSet(mvme, base, 64);
    v1729_NColsSet(mvme, base, 64);
    v1729_TriggerTypeSet(mvme, base,
       V1729_EXTERNAL_TRIGGER | V1729_RISING_EDGE
       |V1729_INHIBIT_RANDOM | V1729_NORMAL_TRIGGER);
    //    v1729_TriggerChannelSelect(mvme, base, 0x3); // V1729_ALL_FOUR);
    v1729_ChannelSelect(mvme, base, V1729_ALL_FOUR);
    v1729_FrqSamplingSet(mvme, base, V1729_1GSPS);
    break;
  case 0x2:
    printf("4 channels 2Gsps External Trigger(mode:%d)\n", mode);
    mvme_write_value(mvme, base+V1729_INTERRUPT_ENABLE, 0);
    v1729_PreTrigSet(mvme, base, 0x4000);
    v1729_PostTrigSet(mvme, base, 64);
    v1729_NColsSet(mvme, base, 64);
    v1729_TriggerTypeSet(mvme, base,
       V1729_EXTERNAL_TRIGGER | V1729_RISING_EDGE
       |V1729_INHIBIT_RANDOM | V1729_NORMAL_TRIGGER);
    v1729_ChannelSelect(mvme, base, V1729_ALL_FOUR);
    v1729_FrqSamplingSet(mvme, base, V1729_2GSPS);
    break;
  case 0x3:
    printf("4 channels 1Gsps External Trigger(mode:%d)\n", mode);
    mvme_write_value(mvme, base+V1729_INTERRUPT_ENABLE, 0);
    v1729_PreTrigSet(mvme, base, 0x4000);
    v1729_PostTrigSet(mvme, base, 32);//64
    v1729_NColsSet(mvme, base, 128);
    v1729_TriggerTypeSet(mvme, base,
       V1729_EXTERNAL_TRIGGER | V1729_RISING_EDGE
       |V1729_INHIBIT_RANDOM | V1729_NORMAL_TRIGGER);
    //    v1729_TriggerChannelSelect(mvme, base, 0x3); // V1729_ALL_FOUR);
    v1729_ChannelSelect(mvme, base, V1729_ALL_FOUR);
    v1729_FrqSamplingSet(mvme, base, V1729_1GSPS);
    break;
  case 0x7:
    printf("4 channels 1Gsps Soft Trigger for Pedestals(mode:%d)\n", mode);
    v1729_PreTrigSet(mvme, base, 0x4000);
    v1729_PostTrigSet(mvme, base, 1 );
    v1729_NColsSet(mvme, base, 128);
    v1729_TriggerTypeSet(mvme, base,
       V1729_SOFT_TRIGGER | V1729_RISING_EDGE
       | V1729_INHIBIT_RANDOM | V1729_NORMAL_TRIGGER);
    v1729_ChannelSelect(mvme, base, V1729_ALL_FOUR);
    v1729_FrqSamplingSet(mvme, base, V1729_1GSPS);
    break;
  case 0x8:
    printf("4 channels 1Gsps Random Trigger for Vernier time extraction(mode:%d)\n", mode);
    v1729_PreTrigSet(mvme, base, 1);
    v1729_PostTrigSet(mvme, base, 1);
    v1729_NColsSet(mvme, base, 0);
    v1729_TriggerTypeSet(mvme, base, V1729_AUTHORIZE_RANDOM);
    v1729_ChannelSelect(mvme, base, V1729_ALL_FOUR);
    v1729_FrqSamplingSet(mvme, base, V1729_1GSPS);
    break;
  default:
    printf("Unknown setup mode\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }
  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
int  v1729_Status(MVME_INTERFACE *mvme, DWORD base)
{
  WORD value, value1;
  int      cmode, ncol;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  printf("--V1729---------------------------------\n");
  value = mvme_read_value(mvme, base+V1729_VERSION_R);
  printf("Version                       : 0x%02x\n", value);
  value = mvme_read_value(mvme, base+V1729_FRQ_SAMPLING);
  printf("Frequency Sampling            : %d[Gsps]\n", value==1 ? 2 : 1);
  value = mvme_read_value(mvme, base+V1729_CHANMASK);
  printf("Channel Mask                  : 0x%01x\n", value&0xf);
  value = mvme_read_value(mvme, base+V1729_TRIGCHAN);
  printf("Trigger Channel source(enable): 0x%01x\n", value&0xF);
  value = mvme_read_value(mvme, base+V1729_N_COL);
  ncol = v1729_NColsGet(mvme, base);
  printf("Number of Columns             : %d\n", ncol); // value&0xFF);
  value = mvme_read_value(mvme, base+V1729_POSTTRIG_LSB);
  value1 = mvme_read_value(mvme, base+V1729_POSTTRIG_MSB);
  printf("PostTrigger delay             : %d\n", ((value1&0xFF)<<8)|(value&0xFF));
  value = mvme_read_value(mvme, base+V1729_PRETRIG_LSB);
  value1 = mvme_read_value(mvme, base+V1729_PRETRIG_MSB);
  printf("PreTrigger delay              : %d\n", ((value1&0xFF)<<8)|(value&0xFF));
  value = mvme_read_value(mvme, base+V1729_FAST_READ_MODES);
  printf("Fast read modes               : %d\n", value & 3);
  value = mvme_read_value(mvme, base+V1729_TRIGTYPE);
  value &= 0x1F;
  if ((value&0x3)==0) printf("Software Trigger, ");
  if ((value&0x3)==1) printf("Trigger on Discr (fixed by DAC), ");
  if ((value&0x3)==2) printf("External Trigger, ");
  if ((value&0x3)==3) printf("Software Trigger OR Trigger on Discr, ");
  if (((value>>2)&0x1)==0) printf("on Rising Edge.\n");
  if (((value>>2)&0x1)==1) printf("on Fallling Edge.\n");
  if (((value>>3)&0x1)==0) printf("Inhibit random internal Trigger, ");
  if (((value>>3)&0x1)==1) printf("Authorize random internal Trigger, ");
  if (((value>>4)&0x1)==0) printf("Normal Trigger.\n");
  if (((value>>4)&0x1)==1) printf("External Trigger without masking.\n");
  mvme_set_dmode(mvme, cmode);
  return V1729_SUCCESS;
}

/*****************************************************************/
void v1729_DataRead(MVME_INTERFACE *mvme, DWORD base, WORD *pdest, int nch, int npt)
{
  int cmode, i, rdlen;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  if (nch>V1729_MAX_CHANNELS) nch = V1729_MAX_CHANNELS;
  if (npt>V1729_MAX_CHANNEL_SIZE) npt = V1729_MAX_CHANNEL_SIZE;
  rdlen =  nch * (npt+3);


  for (i=0 ; i < rdlen ; i++) {
    pdest[i] = (int) mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
  }


  //  mvme_read(mvme, pdest, base+V1729_DATA_FIFO_R, rdlen*2);

  get_ref = 1;
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int v1729_isTrigger(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, done;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  done =  mvme_read_value(mvme, base+V1729_INTERRUPT_REG);
  mvme_set_dmode(mvme, cmode);
  return (done & 0xFF);
}

/*****************************************************************/
void v1729_PreTrigSet(MVME_INTERFACE *mvme, DWORD base, int value)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_PRETRIG_LSB, value&0xFF);
  mvme_write_value(mvme, base+V1729_PRETRIG_MSB, (value>>8)&0xFF);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1729_PostTrigSet(MVME_INTERFACE *mvme, DWORD base, int value)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_POSTTRIG_LSB, value&0xFF);
  mvme_write_value(mvme, base+V1729_POSTTRIG_MSB, (value>>8)&0xFF);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1729_TriggerTypeSet(MVME_INTERFACE *mvme, DWORD base, int value)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_TRIGTYPE, value);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1729_NColsSet(MVME_INTERFACE *mvme, DWORD base, int value)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_N_COL, value & 0xFF);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int v1729_NColsGet(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  value = mvme_read_value(mvme, base+V1729_N_COL);
  mvme_set_dmode(mvme, cmode);
  return (value & 0xFF);
}

/*****************************************************************/
void v1729_ChannelSelect(MVME_INTERFACE *mvme, DWORD base, int value)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_TRIGCHAN, value&0xF);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1729_SoftTrigger(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V1729_SOFT_TRIGGER_W, 0x1);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v1729_FrqSamplingSet(MVME_INTERFACE *mvme, DWORD base, int value)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  if (value < 0) value = 1;
  if (value > 2) value = 2;
  mvme_write_value(mvme, base+V1729_FRQ_SAMPLING, value);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
Does Time calibration of all 4 channels. Requires the module to be in
random Vernier mode (setup 8). Trigger internally generated for the whole
memory.

! Code not yet guarantee to be correct.
*/
int v1729_TimeCalibrationRun(MVME_INTERFACE *mvme, DWORD base, int flag)
{
  int      cmode;
  int vernier[4][16384];
  int i_bar=0;
  int h[5000];
  char bars[] = "|\\-/";
  int i, k=0, j, toggle, save[4];
  FILE *fH=NULL;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  memset(vernier, 0, sizeof(vernier));
  memset(h, 0, sizeof(h));

  // Set Vernier calibration
  v1729_Setup(mvme, base, 8);

  // Start ACQ
  v1729_AcqStart(mvme, base);

  usleep(10000);

  if (flag) {
    fH = fopen("timecalibration.txt", "w");
    if (fH == NULL) {
      printf("File not open\n");
    }
    printf("\ntimecalibration.txt file opened\n");
  }


  // Wait for trigger
  while(!v1729_isTrigger(mvme, base)) {

     printf("...%c Waiting trigger\r", bars[i_bar++ % 4]);
     fflush(stdout);

     usleep(10);
  };

  for (i=0;i<16384;i++) {
    vernier[3][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
    vernier[2][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
    vernier[1][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
    vernier[0][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
  }
  /*
  k = 1;
  for (l=0;l<16384;l++) {
    if (vernier[k][i] < 5000)
      h[vernier[k][l]] += 1;
    else
      printf("vernier:%d\n", vernier[k][i]);
  }
  */
  // Vernier evaluation done
  vernier_ok = 1;

  // Debugging code below... to display only the edges of the
  // time window. I expect to have only 1 edge above 2000.
  // Looking at the whole array, I've seen multiple group!

  if (flag)
    for (i=0;i<5000;i++) {
      fprintf(fH, "Vernier[%i][%i] = %6d\n", k, i, vernier[k][i]);
    }

  toggle=1;
  j=save[0]=save[1]=save[2]=save[3]=0;
  for (i=2000 ; i<4000 ; i++) {
    if (toggle && (h[i] != 0)) {
      save[j++] = i;
      toggle = 0;
    }
    if (!toggle && h[i] == 0) {
      save[j++] = i;
      toggle = 1;
    }

    if (flag) 
      fprintf(fH, "%i - h[%i] = %6d\n", k, i, h[ i]);
  }
  if (flag) fprintf(fH, "index lMINVERnier:%d lMAXVERnier:%d %d %d DiffMaxMin:%d\n"
		    , save[0], save[1], save[2], save[3], save[1]-save[0]);

  lMINVER = save[0];
  lMAXVER = save[1];

  if (flag) {
    fclose(fH);
    printf("File timecalibration.txt closed\n");
  }

  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
/**
   Pedestal extraction. Requires to module to be in setup mode 7
   (soft  trigger).
   Show major improvment in the WF reconstruction, but still has
   20 bin periodic spikes.

   Code not yet guarantee to be fully correct...
*/
int v1729_PedestalRun(MVME_INTERFACE *mvme, DWORD base, int loop, int flag)
{
  int cmode;
  //  int i_bar=0;
  //  char bars[] = "|\\-/";
  WORD data[V1729_RAM_SIZE];
  int i, l;
  int xloop;
  int stuck_trig_rec = -1;
  FILE *fH=NULL;

  memset(pedestals, 0, sizeof(pedestals));

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  // Set Trigger type Soft trigger
  v1729_Setup(mvme, base, 7);

  // give it time to settle...
  sleep(2);

  if (flag) {
    fH = fopen("pedestals.txt", "w");
    if (fH == NULL) {
      printf("File not open\n");
    }
    printf("\npedestals.txt file opened\n");
  }

  for (xloop=0; xloop<2; xloop++) {
    // loop twice: first pass: accept all data, second pass:
    // reject data too far away from the mean
    int countWide = 0;

    static double sum0[V1729_RAM_SIZE];
    static double sum1[V1729_RAM_SIZE];
    static double sum2[V1729_RAM_SIZE];

    memset(sum0, 0, sizeof(sum0));
    memset(sum1, 0, sizeof(sum1));
    memset(sum2, 0, sizeof(sum2));

  for (l=0;l<loop;l++) {
    // Start ACQ
    v1729_AcqStart(mvme, base);

    usleep(20000);

    // Generate soft Trigger
    v1729_SoftTrigger(mvme, base);

    // Wait for trigger
    while(1) {

      int t = v1729_isTrigger(mvme, base);
      //printf("trigger %d\n", t);
      if (t == 255)
	break;

      usleep(10);
    };

    v1729_DataRead(mvme, base, data, 4, V1729_MAX_CHANNEL_SIZE);

    {
      int oldi;
      int post_trig = (mvme_read_value(mvme, base+V1729_POSTTRIG_LSB) & 0xFF)
	| ((mvme_read_value(mvme, base+V1729_POSTTRIG_MSB) & 0xFF) << 8);
      int trig_rec = mvme_read_value(mvme, base+V1729_TRIGREC_R) & 0xFF;

      int end_cell = 20*(post_trig - trig_rec) % 128;

      if (stuck_trig_rec >= 0)
        if (trig_rec == stuck_trig_rec)
          fprintf(stderr,"v1729 at 0x%x: Warning: TRIG_REC is stuck at 0x%x\n", base, trig_rec);
      stuck_trig_rec = trig_rec;

      if (flag) fprintf(fH, "Loop:%d posttrig %d, trigrec %d, endcell %d\n", l, post_trig, trig_rec, end_cell);

      for (oldi=0; oldi<V1729_RAM_SIZE; oldi++)
	{
	  int newi = (2560 + oldi - end_cell) % 2560;

	  //i = newi;
	  i = oldi;
	  newi = i;
	  
	  if (flag) fprintf(fH, "Loop:%d data[%i] = %d\n", l, i, data[i]);
	  
	  if ((xloop==0) || (fabs(data[i] - pedestals[newi]) < 2+5*pedestalsRms[newi]))
	    {
	      sum0[newi] += 1;
	      sum1[newi] += data[i];
	      sum2[newi] += data[i]*data[i];
	    }
	}
    }
  }

  for (i=0;i<V1729_RAM_SIZE;i++) {
    double mean = sum1[i]/sum0[i];
    double var  = sum2[i]/sum0[i] - mean*mean;
    double rms  = 0;

    if (sum0[i] < 3) {
      printf("Pedestal shifted, memory address %5d\n", i);
      mean = 0;
      var  = 0;
    }

    if (var > 0)
      rms = sqrt(var);
    pedestals[i] = mean;
    pedestalsRms[i] = rms;

    if (rms > 20) {
      countWide ++;
      printf("Wide pedestals, memory address %5d, mean: %8.1f, rms: %6.1f\n", i, mean, rms);
    }

  }

  printf("Found %d wide pedestals\n", countWide);

  if (flag)
    for (i=0;i<V1729_RAM_SIZE;i++) {
      fprintf(fH, "Loop:%d Pedestals[%i] = %f, rms: %f\n", l, i, pedestals[i], pedestalsRms[i]);
    }

  //break;
  }

  l = i = 0;
  while (i<V1729_RAM_SIZE ) {
    ped[3][l] = pedestals[i++];
    ped[2][l] = pedestals[i++];
    ped[1][l] = pedestals[i++];
    ped[0][l] = pedestals[i++];
    l++;
  }

  // Pedestal evaluation done
  ped_ok = 1;

  if (flag)
    for (i=0;i<2563;i++) {
      fprintf(fH, "Ped0,1,2,3[%i] = %f %f %f %f\n", i, ped[0][i], ped[1][i], ped[2][i], ped[3][i]);
    }
  if (flag) {
    fclose(fH);
    printf("File pedestals.txt closed\n");
  }

  mvme_set_dmode(mvme, cmode);
  return loop;
}

/*****************************************************************/
/**
    Re-order given channel from srce to dest
    len is for now frozen to V1729_MAX_CHANNEL_SIZE
    Does pedestal subtraction if enabled (ped_ok)
    Doesn't correct for timing.

    NOTE:
    destination cell index[k] computed as 2560+j+end_cell instead
    of manual formula [2] page 13 (2560+j-end_cell).
*/
int v1729_OrderData(MVME_INTERFACE *mvme, DWORD base, WORD *srce, int *dest, int nch, int chan, int npt)
{
  int i, j, k;
  WORD value, value1;
  int cmode, temp, rdlen;

  if (get_ref) {
    // Set dmode to D16
    mvme_get_dmode(mvme, &cmode);
    mvme_set_dmode(mvme, MVME_DMODE_D16);
    
    value = mvme_read_value(mvme, base+V1729_POSTTRIG_LSB);
    value1 = mvme_read_value(mvme, base+V1729_POSTTRIG_MSB);
    post_trig = ((value1 & 0xFF)<<8) | (value & 0xFF);
    ncol = v1729_NColsGet(mvme, base);
    Corr_vernier[3] = (float) (srce[4] - lMINVER) / (float) (lMAXVER - lMINVER);
    Corr_vernier[2] = (float) (srce[5] - lMINVER) / (float) (lMAXVER - lMINVER);
    Corr_vernier[1] = (float) (srce[6] - lMINVER) / (float) (lMAXVER - lMINVER);
    Corr_vernier[0] = (float) (srce[7] - lMINVER) / (float) (lMAXVER - lMINVER);
    trig_rec = mvme_read_value(mvme, base+V1729_TRIGREC_R);
    trig_rec &= 0xFF;
    //end_cell = (20 * ((post_trig + trig_rec) % ncol));
    end_cell = (20 * ((post_trig + trig_rec) % 2560));
    get_ref = 0;

    //printf("posttrig %d, trigrec %d, endcell %d\n", post_trig, trig_rec, end_cell);
  }

  if (nch>V1729_MAX_CHANNELS) nch = V1729_MAX_CHANNELS;
  if (npt>V1729_MAX_CHANNEL_SIZE) npt = V1729_MAX_CHANNEL_SIZE;
  rdlen = nch * (npt+3);

  if (nch == 4) {    // 0-1-2/3-4-5/6-7-8 start=9>2, 10>1, 11>0
    for (i=12+3-chan, j=0; i < rdlen; i+=nch, j++) {
      temp =  ((int) srce[i] - pedestals[i]);
      k =  (20*ncol + j + end_cell) % (20*ncol);
      dest[k] = temp;
    }
  } else if (nch == 3) { // 0-1-2/3-4-5/6-7-8 start=9>2, 10>1, 11>0
    for (i=9+2-chan, j=0; i < rdlen; i+=nch, j++) {
      temp =  ((int) srce[i] - pedestals[i]);
      k =  ((20*ncol) + j + end_cell) % (20*ncol);
      dest[k] = temp;
    }
  } else if (nch == 2) { // 0-1/2-3/4-5  start=6>1, 7>0
    for (i=6+1-chan, j=0; i < rdlen; i+=nch, j++) {
      temp =  ((int) srce[i] - pedestals[i]);
      k =  ((20*ncol) + j + end_cell) % (20*ncol);
      dest[k] = temp;
    }
  } else if (nch == 1) { // 0/1/2        start=3>0
    for (i=3-chan, j=0; i < rdlen; i+=nch, j++) {
      //temp =  ((int) srce[i] - (ped_ok ? (int) ped[chan][3+j] : 0));
      temp =  ((int) srce[i] - pedestals[i]);
      k =  ((20*ncol) + j + end_cell) % (20*ncol);
      dest[k] = temp;
    }
  }
  
  /*
    if (debug) {
    for (i=0;i<V1729_MAX_CHANNEL_SIZE;i++) {
    printf("dest[%i] = %d\n", i, dest[i]);
    }
    }
    printf("First sample   %d %d %d %d\n", srce[0], srce[1], srce[2], srce[3]);
    printf("Vernier        %d %d %d %d\n", srce[4], srce[5], srce[6], srce[7]);
    printf("Reset Baseline %d %d %d %d\n", srce[8], srce[9], srce[10], srce[11]);
    printf("Value:%d Value1:%d\n", value, value1);
    printf("Post trig:%d - Trig rec:%d(C:%d) - End_cell:%d(%d)\n"
    , post_trig, trig_rec, 128-trig_rec, end_cell, end_cell/20);
    printf("Cvernier       %f %f %f %f\n", Corr_vernier[0], Corr_vernier[1]
    , Corr_vernier[2], Corr_vernier[3]);
  */
  
  mvme_set_dmode(mvme, cmode);
  return 0;
}

/********************************************************************/
/********************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main () {

  MVME_INTERFACE *myvme;
  DWORD V1729_BASE = 0x600000;
  char bars[] = "|\\-/";
  int i_bar, wheel = 0, status, csr, i, j, len;
  WORD    data[20000];
  WORD    odata[3000];

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D16
  mvme_set_dmode(myvme, MVME_DMODE_D16);

  // Reset board
  v1729_Reset(myvme, V1729_BASE);

  // Get Firmware revision
  csr = mvme_read_value(myvme, V1729_BASE+V1729_VERSION_R);
  printf("Firmware revision: 0x%x\n", csr);

  // Collect Pedestals
  v1729_PedestalRun(myvme, V1729_BASE, 10);

  // Do vernier calibration
  v1729_TimeCalibrationRun(myvme, V1729_BASE);

  // Set mode 1
  v1729_Setup(myvme, V1729_BASE, 1);

  // Print Current status
  v1729_Status(myvme, V1729_BASE);

  // Start acquisition
  v1729_AcqStart(myvme, V1729_BASE);

  printf("ACQ started\n");
  // Wait for trigger
 while(!v1729_isTrigger(myvme, V1729_BASE)) {
   if (wheel) {
     printf("...%c Waiting trigger\r", bars[i_bar++ % 4]);
     fflush(stdout);
   };
 };
 printf("Going to read event (trig_rec:%ld)\n"
  , (0xFF & mvme_read_value(myvme, V1729_BASE+V1729_TRIGREC_R)));

 v1729_DataRead(myvme, V1729_BASE, data, 4, V1729_MAX_CHANNEL_SIZE);
 i = 0;
 printf("First Sample  = %d-%d-%d-%d\n", data[i], data[i+1], data[i+2], data[i+3]);
 i = 4;
 printf("Vernier       = %d-%d-%d-%d\n", data[i], data[i+1], data[i+2], data[i+3]);
 i = 8;
 printf("Reset Baseline= %d-%d-%d-%d\n", data[i], data[i+1], data[i+2], data[i+3]);

 for (i=12 , j=0; i<V1729_RAM_SIZE ; i+=4, j++) {
   printf("Data[%5i...] (%6d) = %4d-%4d-%4d-%4d\n", i, j, data[i], data[i+1], data[i+2], data[i+3]);
 }

 v1729_OrderData(myvme, V1729_BASE, data, odata, 4, 0, V1729_MAX_CHANNEL_SIZE);

 /*
 for (i=0 ; i<len; i+=4) {
   printf("%6d - Data[%5i...] = %4d-%4d-%4d-%4d\n", len, i, odata[i], odata[i+1], odata[i+2], odata[i+3]);
 }
 */
 // exit:
 status = mvme_close(myvme);
 return 0;
}

#endif


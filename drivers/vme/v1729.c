/*********************************************************************

  Name:         v1792.c
  Created by:   Pierre-Andre Amaudruz

  Contents:     V1729 4channel /12 bit sampling ADC (1,2Gsps)

  $Log: v1729.c,v $
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include "unistd.h"
#include "v1729.h"
int debug = 1;

/*****************************************************************/
/*****************************************************************/
void v1729_AcqStart(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
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
    v1729_PreTrigSet(mvme, base, 0x4000);
    v1729_PostTrigSet(mvme, base, 64);
    v1729_NColsSet(mvme, base, 128);
    v1729_TriggerTypeSet(mvme, base,
       V1729_EXTERNAL_TRIGGER | V1729_RISING_EDGE
       |V1729_INHIBIT_RANDOM | V1729_NORMAL_TRIGGER);
    v1729_ChannelSelect(mvme, base, V1729_ALL_FOUR);
    v1729_FrqSamplingSet(mvme, base, V1729_1GSPS);
    break;
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
  v1729_Status(mvme, base);
  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
int  v1729_Status(MVME_INTERFACE *mvme, DWORD base)
{
  WORD value, value1;
  int      cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  value = mvme_read_value(mvme, base+V1729_VERSION_R);
  printf("Version                       : 0x%02x\n", value);
  value = mvme_read_value(mvme, base+V1729_FRQ_SAMPLING);
  printf("Frequency Sampling            : %d[Gsps]\n", value==1 ? 2 : 1);
  value = mvme_read_value(mvme, base+V1729_CHANMASK);
  printf("Channel Mask                  : 0x%01x\n", value&0xf);
  value = mvme_read_value(mvme, base+V1729_TRIGCHAN);
  printf("Trigger Channel source(enable): 0x%01x\n", value&0xF);
  value = mvme_read_value(mvme, base+V1729_N_COL);
  printf("Number of Columns             : %d\n", value&0xFF);
  value = mvme_read_value(mvme, base+V1729_POSTTRIG_LSB);
  value1 = mvme_read_value(mvme, base+V1729_POSTTRIG_MSB);
  printf("PostTrigger delay             : %d\n", ((value1&0xFF)<<8)|(value&0xFF));
  value = mvme_read_value(mvme, base+V1729_PRETRIG_LSB);
  value1 = mvme_read_value(mvme, base+V1729_PRETRIG_MSB);
  printf("PreTrigger delay              : %d\n", ((value1&0xFF)<<8)|(value&0xFF));
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
  return V1729_SUCCESS;
}

/*****************************************************************/
void v1729_DataRead(MVME_INTERFACE *mvme, DWORD base, WORD *pdest, int *evtcnt)
{
  int cmode, i;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  *evtcnt = V1729_RAM_SIZE;
  for (i=0;i<*evtcnt;i++) {
    pdest[i] = (int) mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
  }
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int v1729_isTrigger(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, done;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  done =  mvme_read_value(mvme, base+V1729_INTERRUPT_REG);
  usleep(1000);
  mvme_set_dmode(mvme, cmode);
  return (done&0x1);
}

/*****************************************************************/
void v1729_ChannelRead(MVME_INTERFACE *mvme, DWORD base, WORD *pdest, int ch, int *evtcnt)
{
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
static int lMINVER, lMAXVER;
static int   vernier_ok = 0;
int v1729_TimeCalibrationRun(MVME_INTERFACE *mvme, DWORD base)
{
  int vernier[4][16384];
  int i_bar=0;
  int h[5000];
  char bars[] = "|\\-/";
  int i, l, j, k, toggle, save[4];

  memset(vernier, 0, sizeof(vernier));
  memset(h, 0, sizeof(h));

  // Set Vernier calibration
  v1729_Setup(mvme, base, 8);

  // Start ACQ
  v1729_AcqStart(mvme, base);

  usleep(10000);

  // Wait for trigger
  while(!v1729_isTrigger(mvme, base)) {
     printf("...%c Waiting trigger\r", bars[i_bar++ % 4]);
     fflush(stdout);
  };

  for (i=0;i<16384;i++) {
    vernier[3][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
    vernier[2][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
    vernier[1][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
    vernier[0][i] =  mvme_read_value(mvme, base+V1729_DATA_FIFO_R);
  }

  k = 1;
  for (l=0;l<16384;l++) {
    if (vernier[k][i] < 5000)
      h[vernier[k][l]] += 1;
    else
      printf("vernier:%d\n", vernier[k][i]);
  }

  // Vernier evaluation done
  vernier_ok = 1;

  // Debugging code below... to display only the edges of the
  // time window. I expect to have only 1 edge above 2000.
  // Looking at the whole array, I've seen multiple group!

  /*
  for (i=0;i<5000;i++) {
    printf("Vernier[%i][%i] = %6d\n", k, i, vernier[k][i]);
  }
  */

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

    printf("%i - h[%i] = %6d\n", k, i, h[ i]);
  }
  printf("index %d %d %d %d D:%d\n", save[0],save[1], save[2], save[3], save[1]-save[0]);

  lMINVER = save[0];
  lMAXVER = save[1];

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
static float pedestals[V1729_RAM_SIZE];
static int   ped_ok = 0;
int v1729_PedestalRun(MVME_INTERFACE *mvme, DWORD base, int loop)
{
  int i_bar=0;
  char bars[] = "|\\-/";
  WORD data[V1729_RAM_SIZE];
  int i, l, len;

  memset(pedestals, 0, sizeof(pedestals));

  // Set Trigger type Soft trigger
  v1729_Setup(mvme, base, 7);

  for (l=0;l<loop;l++) {
    // Start ACQ
    v1729_AcqStart(mvme, base);

    usleep(20000);

    // Generate soft Trigger
    v1729_SoftTrigger(mvme, base);


    // Wait for trigger
    while(!v1729_isTrigger(mvme, base)) {
      printf("%i ...%c Waiting trigger for pedestal extraction\r", l, bars[i_bar++ % 4]);
      fflush(stdout);
    };

    len = V1729_RAM_SIZE;
    v1729_DataRead(mvme, base, data, &len);
    for (i=0;i<len;i++) {
      pedestals[i] += (float) data[i];
    }
  }

  for (i=0;i<len;i++) {
    pedestals[i] /= (float) loop;
  }

  // Pedestal evaluation done
  ped_ok = 1;

  /*
  for (i=0;i<len;i++) {
    printf("Loop:%d Pedestals[%i] = %f\n", loop, i, pedestals[i]);
  }
  */

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
int v1729_OrderData(MVME_INTERFACE *mvme, DWORD base, WORD *srce, int *dest, int channel, int *len)
{

  WORD value, value1, post_trig;
  float  Corr_vernier[4];
  int i, j, k, trig_rec, end_cell;
  int temp;

  value = mvme_read_value(mvme, base+V1729_POSTTRIG_LSB);
  value1 = mvme_read_value(mvme, base+V1729_POSTTRIG_MSB);
  post_trig = ((value1 & 0xFF)<<8) | (value & 0xFF);
  Corr_vernier[0] = (float) (srce[4] - lMINVER) / (float) (lMAXVER - lMINVER);
  Corr_vernier[1] = (float) (srce[5] - lMINVER) / (float) (lMAXVER - lMINVER);
  Corr_vernier[2] = (float) (srce[6] - lMINVER) / (float) (lMAXVER - lMINVER);
  Corr_vernier[3] = (float) (srce[7] - lMINVER) / (float) (lMAXVER - lMINVER);
  trig_rec = mvme_read_value(mvme, base+V1729_TRIGREC_R);
  trig_rec &= 0xFF;
  end_cell = (20 * ((post_trig + trig_rec) % 128));

  for (i=12+3-channel, j=0; i<V1729_RAM_SIZE; i+=4, j++) {
    temp =  ((int) srce[i] - (ped_ok ? (int) pedestals[i] : 0));
    k =  (2560 + j + end_cell) % 2560;
    dest[k] = temp;
  }

  *len = V1729_MAX_CHANNEL_SIZE;

  if (debug) {
    //  for (i=0;i<V1729_MAX_CHANNEL_SIZE;i++) {
    //    printf("dest[%i] = %d\n", i, dest[i]);
    //  }
    printf("First sample   %d %d %d %d\n", srce[0], srce[1], srce[2], srce[3]);
    printf("Vernier        %d %d %d %d\n", srce[4], srce[5], srce[6], srce[7]);
    printf("Reset Baseline %d %d %d %d\n", srce[8], srce[9], srce[10], srce[11]);
    printf("Post trig:%d - Trig rec:%d(C:%d) - End_cell:%d(%d)\n"
     , post_trig, trig_rec, 128-trig_rec, end_cell, end_cell/20);
    printf("Cvernier       %f %f %f %f\n", Corr_vernier[0], Corr_vernier[1], Corr_vernier[2], Corr_vernier[3]);
  }
  return 0;
}

/********************************************************************/
/********************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main () {

  MVME_INTERFACE *myvme;
  DWORD V1729_BASE = 0x400000;
  char bars[] = "|\\-/";
  int i_bar, wheel = 0, status, csr, i, j, len;
  WORD    data[20000];
  int     odata[3000];

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

 len = V1729_RAM_SIZE;
 v1729_DataRead(myvme, V1729_BASE, data, &len);
 i = 0;
 printf("First Sample  = %d-%d-%d-%d\n", data[i], data[i+1], data[i+2], data[i+3]);
 i = 4;
 printf("Vernier       = %d-%d-%d-%d\n", data[i], data[i+1], data[i+2], data[i+3]);
 i = 8;
 printf("Reset Baseline= %d-%d-%d-%d\n", data[i], data[i+1], data[i+2], data[i+3]);

 for (i=12 , j=0; i<len ; i+=4, j++) {
   printf("Data[%5i...] (%6d) = %4d-%4d-%4d-%4d\n", i, j, data[i], data[i+1], data[i+2], data[i+3]);
 }

 v1729_OrderData(myvme, V1729_BASE, data, odata, 0, &len);

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


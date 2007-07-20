/*********************************************************************
  @file
  Name:         lrs1190.c
  Created by:   Pierre-Andre Amaudruz
  Content:      LeCroy 32bit Dual port memory
  $Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#if defined(OS_LINUX)
#include <unistd.h>
#endif
#include "lrs1190.h"

/*****************************************************************/
/*
Reset the LRS1190
*/
void lrs1190_Reset(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode, status;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  status = mvme_read_value(mvme, base+LRS1190_RESET_WO);
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/*
Enable the LRS1190 FPP
*/
void lrs1190_Enable(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  mvme_write_value(mvme, base+LRS1190_ENABLE_RW, 0x1);
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/*
Disable the LRS1190 FPP
*/
void lrs1190_Disable(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  mvme_write_value(mvme, base+LRS1190_ENABLE_RW, 0x0);
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/*
Read the counter
*/
int lrs1190_CountRead(MVME_INTERFACE *mvme, DWORD base)
{
  int   cmode, count;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  count = mvme_read_value(mvme, base+LRS1190_COUNT_RO);
  mvme_set_dmode(mvme, cmode);
  return count;
}

/*****************************************************************/
/*
Read the I4 data
*/
int lrs1190_I4Read(MVME_INTERFACE *mvme, DWORD base, DWORD * data, int r)
{
  int  cmode, count;
  DWORD add;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  count = mvme_read_value(mvme, base+LRS1190_COUNT_RO);
  mvme_write_value(mvme, base+LRS1190_ENABLE_RW, 0x0);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  printf("internal count:%d\n", count);
  if (r > count) r = count;
  add = base+LRS1190_DATA_RO;
  while (r > 0) {
    *data = mvme_read_value(mvme, add);
    add += 4;
    printf("data:0x%x\n", *data);
    data++;
    r--;
  }
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+LRS1190_RESET_WO, 0);
  mvme_write_value(mvme, base+LRS1190_ENABLE_RW, 0x1);
  mvme_set_dmode(mvme, cmode);
  return count;
}

/*****************************************************************/
/*
Read the LowI2 data
*/
int lrs1190_L2Read(MVME_INTERFACE *mvme, DWORD base, WORD * data, int r)
{
  int  cmode, count;
  DWORD local;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  count = mvme_read_value(mvme, base+LRS1190_COUNT_RO);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if (r > count) r = count;
  while (r > 0) {
    local = mvme_read_value(mvme, base+LRS1190_DATA_RO);
    *data = *((WORD *)(&local)+0);
    data++;
    r--;
  }
  mvme_set_dmode(mvme, cmode);
  return count;
}

/*****************************************************************/
/*
Read the HighI2 data
*/
int lrs1190_H2Read(MVME_INTERFACE *mvme, DWORD base, WORD * data, int r)
{
  int  cmode, count;
  DWORD local;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  count = mvme_read_value(mvme, base+LRS1190_COUNT_RO);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if (r > count) r = count;
  while (r > 0) {
    local = mvme_read_value(mvme, base+LRS1190_DATA_RO);
    *data = *((WORD *)(&local)+1);
    data++;
    r--;
  }
  mvme_set_dmode(mvme, cmode);
  return count;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {
  DWORD localbuf[1000], data[1000];
  int status, count, newcount, i;
  DWORD LRS1190_BASE = 0x780000;

  MVME_INTERFACE *myvme;

  if (argc>1) {
    sscanf(argv[1],"%lx",&LRS1190_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  printf("\nReset \n");
  
  while(1) {
    //  lrs1190_Reset(myvme, LRS1190_BASE);
  // Test Enable/Disable
    usleep(100000);
    printf("Enable \n");
    //    lrs1190_Enable(myvme, LRS1190_BASE);
    usleep(1000000);
    printf("Disable \n");
    //    lrs1190_Disable(myvme, LRS1190_BASE);
    usleep(100000);
    count = lrs1190_CountRead(myvme, LRS1190_BASE);
    printf("Count : 0x%x\n", count);

#if 0
    if (count > 1000) count = 1000;
    if (count > 0) {
      if (count == 1) {
	lrs1190_I4Read(myvme, LRS1190_BASE, &(data[0]), count);
      } else {
	lrs1190_I4Read(myvme, LRS1190_BASE, &(localbuf[0]), count);
	data[0] = localbuf[0];
	newcount = 1;
	for (i=1;i<count;i++) {
	  if (localbuf[i] != localbuf[i-1]) {
	    data[i] = localbuf[i];
	    newcount += 1;
	  }
	}
      }
      for (i=0;i<newcount;i++) {
	printf("Data[%i]=0x%x\n", i, data[i]);
      }
    }
#endif
#if 1
    if (count > 1000) count = 1000;
    lrs1190_I4Read(myvme, LRS1190_BASE, data, count);
    for (i=0;i<count;i++) {
      printf("Data[%i]=0x%x\n", i, data[i]);
    }
#endif

    }
  status = mvme_close(myvme);
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


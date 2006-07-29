/*********************************************************************
  @file
  Name:         lrs1190.c
  Created by:   Pierre-Andre Amaudruz
  Content:      LeCroy 32bit Dual port memory
  $Log: lrs1190.c,v $
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
 
  mvme_write_value(mvme, base+LRS1190_EDABLE_RW, 0x1);
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
 
  mvme_write_value(mvme, base+LRS1190_EDABLE_RW, 0x0);
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
  DWORD * local;
  
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
 
  count = mvme_read_value(mvme, base+LRS1190_COUNT_RO);
 
  if (count <= r)
    r = count;
  local = (DWORD *)(base+LRS1190_DATA_RO);
  while (r > 0) {
    *data = mvme_read_value(mvme, (DWORD) local);
    data++;
    local++;
    r--;
  }

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
  DWORD * local;
  
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
 
  count = mvme_read_value(mvme, base+LRS1190_COUNT_RO);
 
  if (count <= r)
    r = count;
  local = (DWORD *)(base+LRS1190_DATA_RO);
  while (r > 0) {
    *data = mvme_read_value(mvme, (DWORD) local);
    ((WORD *)data)++;
    local++;
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
  DWORD * local;
  
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
 
  count = mvme_read_value(mvme, base+LRS1190_COUNT_RO);
 
  if (count <= r)
    r = count;
  local = (DWORD *)(base+LRS1190_DATA_RO);
  while (r > 0) {
    ((WORD *)local)++;    
    *data = mvme_read_value(mvme, (DWORD) local);
    ((WORD *)data)++;
    ((WORD *)local)++;    
    r--;
  }
  mvme_set_dmode(mvme, cmode);
  return count;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  int status, count;
  DWORD LRS1190_BASE = 0x600000;
  
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

  while(1) {
  // Test Enable/Disable
    printf("\nReset \n");
    lrs1190_Reset(myvme, LRS1190_BASE);
    usleep(1000000);  
    printf("Enable \n");
    lrs1190_Enable(myvme, LRS1190_BASE);
    usleep(1000000);  
    printf("Disable \n");
    lrs1190_Disable(myvme, LRS1190_BASE);
    usleep(1000000);  
    count = lrs1190_CountRead(myvme, LRS1190_BASE);
    printf("Count : 0x%x\n", count);
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


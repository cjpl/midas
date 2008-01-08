/*********************************************************************
  @file
  Name:         lrs1151.c
  Created by:   Pierre-Andre Amaudruz
  Content:      LeCroy 16 channel scalers
  $Id:$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#if defined(OS_LINUX)
#include <unistd.h>
#endif
#include "lrs1151.h"

/*---------------------------------------------------------------*/
void lrs1151_Read(MVME_INTERFACE *mvme, DWORD base, DWORD * data)
{
  int   r, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  for (r = 0; r < 16; r++) {
    *data++ = mvme_read_value(mvme, base + LRS1151_DATA_RO + r);
  }
  mvme_set_dmode(mvme, cmode);
  return;
}

/*---------------------------------------------------------------*/
void lrs1151_Clear (MVME_INTERFACE *mvme, DWORD base)
{
  int   r, cmode, status;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  for (r = 0; r < 16; r++) {
    status = mvme_write_value(mvme, base+LRS1151_CLEAR_WO + r, 0x0);
  }
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {
  DWORD data[20];
  int status, count = 16, i;
  DWORD LRS1151_BASE = 0xF00000;
  
  MVME_INTERFACE *myvme;
  
  if (argc>1) {
    sscanf(argv[1],"%lx",&LRS1151_BASE);
  }
  
  // Test under vmic
  status = mvme_open(&myvme, 0);
  
  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);
  
  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  lrs1151_Clear(myvme, LRS1151_BASE);
  printf("\nClear Scalers \n");  
  
#if 1
  lrs1151_Read(myvme, LRS1151_BASE, data);
  for (i=0;i<count;i++) {
    printf("Data[%i]=0x%lx\n", i, data[i]);
  }
#endif
  
  status = mvme_close(myvme);
  
  return 0;
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

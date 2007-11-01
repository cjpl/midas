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
void lrs1151_read(MVME_INTERFACE *mvme, DWORD base, DWORD * data)
{
  int   r, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  for (r = 0; r < 16; r++) {
    *data++ = mvme_read_value(mvme, base + LRS1151_DATA_RO + (r<<2));
  }
  mvme_set_dmode(mvme, cmode);
  return;
}

/*---------------------------------------------------------------*/
void lrs1151_clear (MVME_INTERFACE *mvme, DWORD base, int ch)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  if (ch == LRS1151_CLEAR_ALL)
    mvme_write_value(mvme, base+LRS1151_CLEAR_ALL, LRS1151_CLEAR_ALL);
  else {
    mvme_write_value(mvme, base+LRS1151_CLEAR_WO + (ch<<2), 0x0);
  }
 
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {
  DWORD data[16];
  int status, count = 16, i, clear = 0;
  DWORD LRS1151_BASE = 0x7A00000;
  
  MVME_INTERFACE *myvme;
  
  if (argc>1) {
    sscanf(argv[1],"%lx",&LRS1151_BASE);
  }

  if (argc>2) {
    clear = 1;
  }
  // Test under vmic
  status = mvme_open(&myvme, 0);
  
  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);
  
  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D16);


  if (clear) {
    lrs1151_clear(myvme, LRS1151_BASE, LRS1151_CLEAR_ALL);
    printf("\nClear Scalers \n");  
  }

  printf("ID:0x%x \n", mvme_read_value(myvme, LRS1151_BASE + 0xfa));
  printf("MA:0x%x \n", mvme_read_value(myvme, LRS1151_BASE + 0xfc));
  printf("RV:0x%x \n", mvme_read_value(myvme, LRS1151_BASE + 0xfe));
#if 1
  lrs1151_read(myvme, LRS1151_BASE, data);
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

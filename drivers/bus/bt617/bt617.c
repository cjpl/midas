/*********************************************************************

  Name:         bt617.c
  Created by:   Stefan Ritt

  Cotents:      Routines for accessing VME over SBS Bit3 Model 617
                Interface under Windows NT using the NT device driver 
                Model 983 and under Linux using the vmehb device
                driver.

  $LOG$
*********************************************************************/

#include    <stdio.h>

#ifdef _MSC_VER
#define BT_NTDRIVER
#define BT_WINNT
#define BT973
#include    "btapi.h"
#endif

#include    "mvmestd.h"

/*------------------------------------------------------------------*/

#define MAX_BT617_DEVICES  4
#define MAX_BT617_MODES   20

typedef struct {
   int amod;
   char devname[BT_MAX_DEV_NAME];
   bt_desc_t btd;
} BT617_MODE;

typedef struct {
   int i_mode;
   int dmode;
   int dma;
   int fifo_mode;
   BT617_MODE bt617_mode[MAX_BT617_MODES];
} BT617_DEVICE;

BT617_DEVICE bt617_device[MAX_BT617_DEVICES];

static int i_bt617;

/*------------------------------------------------------------------*/

int mvme_init(void)
{
   BT617_MODE *pmode;
   bt_devdata_t flag;
   int status;

   i_bt617 = 0; /* use first device by default */
   bt617_device[0].i_mode = 0;
   bt617_device[0].dmode = MVME_DMODE_D16;
   pmode = bt617_device[i_bt617].bt617_mode;

   /* open in A16D16 by default */
   pmode->amod = MVME_AMOD_A16;
   bt_gen_name(i_bt617, BT_DEV_A16, pmode->devname, BT_MAX_DEV_NAME);
   status = bt_open(&pmode->btd, pmode->devname, BT_RDWR);
   if (status != BT_SUCCESS) {
      bt_perror(pmode->btd, status, "bt_open error");
      return MVME_NO_INTERFACE;
   }
   bt_init(pmode->btd);
   bt_clrerr(pmode->btd);

   /* select swapping mode */
   flag = BT_SWAP_NONE;
   bt_set_info(pmode->btd, BT_INFO_SWAP, flag);

   /* set data mode */
   flag = BT_WIDTH_D16;
   bt_set_info(pmode->btd, BT_INFO_DATAWIDTH, flag);

   /* switch off block transfer */
   flag = FALSE;
   bt_set_info(pmode->btd, BT_INFO_BLOCK, flag);

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_exit()
{
   int i, j;
   bt_error_t status;
   BT617_MODE *pmode;

   for (i=0 ; i<MAX_BT617_DEVICES ; i++)
      for (j=0 ; j<MAX_BT617_MODES ; j++) {
         pmode = &(bt617_device[i].bt617_mode[j]);
         status = bt_close(pmode->btd);
         if (status != BT_SUCCESS)
            bt_perror(pmode->btd, status, "bt_close error");
      }

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_write(mvme_addr_t vme_addr, void *src, mvme_size_t n_bytes)
{
   mvme_size_t n;
   bt_error_t status;
   BT617_MODE *pmode;

   pmode = &bt617_device[i_bt617].bt617_mode[bt617_device[i_bt617].i_mode];

   status = bt_write(pmode->btd, src, vme_addr, n_bytes, &n);
   if (status != BT_SUCCESS)
      bt_perror(pmode->btd, status, "bt_write error");

   return n;
}

/*------------------------------------------------------------------*/

int mvme_read(void *dst, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
   mvme_size_t n;
   bt_error_t status;
   BT617_MODE *pmode;

   pmode = &bt617_device[i_bt617].bt617_mode[bt617_device[i_bt617].i_mode];

   status = bt_read(pmode->btd, dst, vme_addr, n_bytes, &n);
   if (status != BT_SUCCESS)
      bt_perror(pmode->btd, status, "bt_read error");

   return n;
}

/*------------------------------------------------------------------*/

int bt617_set_amod(int amod)
{
   int i;
   BT617_MODE *pmode;
   bt_error_t status;
   bt_devdata_t flag;

   for (i=0 ; i<MAX_BT617_MODES ; i++)
      if (bt617_device[i_bt617].bt617_mode[i].amod == amod)
         break;

   if (bt617_device[i_bt617].bt617_mode[i].amod == amod) {

      bt617_device[i_bt617].i_mode = i;
      pmode = &bt617_device[i_bt617].bt617_mode[i];
      bt_set_info(pmode->btd, BT_INFO_DMA_AMOD, amod);
      bt_set_info(pmode->btd, BT_INFO_PIO_AMOD, amod);
      bt_set_info(pmode->btd, BT_INFO_MMAP_AMOD, amod);
      return MVME_SUCCESS;
   }

   /* search free slot */
   for (i=0 ; i<MAX_BT617_MODES ; i++)
      if (bt617_device[i_bt617].bt617_mode[i].amod == 0)
         break;
   if (i==MAX_BT617_MODES)
      return MVME_UNSUPPORTED;

   /* mode not yet open, open it */
   bt617_device[0].i_mode = i;
   pmode = &bt617_device[i_bt617].bt617_mode[i];

   /* open in A16D16 by default */
   pmode->amod = amod;
   if (amod == MVME_AMOD_A16_SD || amod == MVME_AMOD_A16_ND)
      bt_gen_name(i_bt617, BT_DEV_A16, pmode->devname, BT_MAX_DEV_NAME);
   else if (amod == MVME_AMOD_A24_SB     ||
            amod == MVME_AMOD_A24_SP     ||
            amod == MVME_AMOD_A24_SD     ||
            amod == MVME_AMOD_A24_NB     ||
            amod == MVME_AMOD_A24_NP     ||
            amod == MVME_AMOD_A24_ND     ||
            amod == MVME_AMOD_A24_SMBLT  ||
            amod == MVME_AMOD_A24_NMBLT  ||
            amod == MVME_AMOD_A24        ||
            amod == MVME_AMOD_A24_D64)
      bt_gen_name(i_bt617, BT_DEV_A24, pmode->devname, BT_MAX_DEV_NAME);
   else
      bt_gen_name(i_bt617, BT_DEV_A32, pmode->devname, BT_MAX_DEV_NAME);

   status = bt_open(&pmode->btd, pmode->devname, BT_RDWR);
   if (status != BT_SUCCESS) {
      bt_perror(pmode->btd, status, "bt_open error");
      return MVME_NO_INTERFACE;
   }
   bt_init(pmode->btd);
   bt_clrerr(pmode->btd);

   /* select swapping mode */
   flag = BT_SWAP_NONE;
   bt_set_info(pmode->btd, BT_INFO_SWAP, flag);

   /* set data mode */
   flag = BT_WIDTH_D16;
   bt_set_info(pmode->btd, BT_INFO_DATAWIDTH, flag);

   /* switch off block transfer */
   flag = FALSE;
   bt_set_info(pmode->btd, BT_INFO_BLOCK, flag);

   /* set amod */
   bt_set_info(pmode->btd, BT_INFO_DMA_AMOD, amod);
   bt_set_info(pmode->btd, BT_INFO_PIO_AMOD, amod);
   bt_set_info(pmode->btd, BT_INFO_MMAP_AMOD, amod);

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int bt617_set_dmode(int dmode)
{
   BT617_MODE *pmode;
   bt_devdata_t flag;

   bt617_device[i_bt617].dmode = dmode;
   pmode = &bt617_device[i_bt617].bt617_mode[bt617_device[i_bt617].i_mode];

   if (dmode == MVME_DMODE_D8)
      flag = BT_WIDTH_D8;
   else if (dmode == MVME_DMODE_D16)
      flag = BT_WIDTH_D16;
   else if (dmode == MVME_DMODE_D32)
      flag = BT_WIDTH_D16;
   else if (dmode == MVME_DMODE_D64)
      flag = BT_WIDTH_D64;
   else
      return MVME_UNSUPPORTED;
   
   bt_set_info(pmode->btd, BT_INFO_DATAWIDTH, flag);
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int bt617_set_dma(int dma)
{
   BT617_MODE *pmode;
   bt_devdata_t flag;

   pmode = &bt617_device[i_bt617].bt617_mode[bt617_device[i_bt617].i_mode];

   if (dma) {
      /* switch on block transfer */
      flag = TRUE;
      bt_set_info(pmode->btd, BT_INFO_BLOCK, flag);

      flag = 4;
      bt_set_info(pmode->btd, BT_INFO_DMA_POLL_CEILING, flag);

      flag = 4;
      bt_set_info(pmode->btd, BT_INFO_DMA_THRESHOLD, flag);
   } else {
      /* switch on block transfer */
      flag = FALSE;
      bt_set_info(pmode->btd, BT_INFO_BLOCK, flag);

      flag = 100000000;
      bt_set_info(pmode->btd, BT_INFO_DMA_POLL_CEILING, flag);

      flag = 100000000;
      bt_set_info(pmode->btd, BT_INFO_DMA_THRESHOLD, flag);
   }

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_ioctl(int req, int *param)
{
   switch (req) {
      case MVME_IOCTL_CRATE_SET:
         *param = i_bt617;
         break;

      case MVME_IOCTL_CRATE_GET:
         if (*param >= MAX_BT617_DEVICES)
            return MVME_NO_CRATE;
         i_bt617 = *param;
         break;

      case MVME_IOCTL_DMODE_SET:
         bt617_device[i_bt617].dmode = *param;
         return bt617_set_dmode(*param);
         break;

      case MVME_IOCTL_DMODE_GET:
         *param = bt617_device[i_bt617].dmode;
         break;

      case MVME_IOCTL_AMOD_SET:
         return bt617_set_amod(*param);
         break;

      case MVME_IOCTL_AMOD_GET:
         *param = bt617_device[i_bt617].bt617_mode[bt617_device[i_bt617].i_mode].amod;
         break;

      case MVME_IOCTL_FIFO_SET:
         bt617_device[i_bt617].fifo_mode = *param;
         break;

      case MVME_IOCTL_FIFO_GET:
         *param = bt617_device[i_bt617].fifo_mode;
         break;

      case MVME_IOCTL_DMA_SET:
         bt617_device[i_bt617].dma = *param;
         return bt617_set_dma(*param);
         break;

      case MVME_IOCTL_DMA_GET:
         *param = bt617_device[i_bt617].dma;
         break;
   }

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int bt_vme_mmap(int vh, void **ptr, int vme_addr, int size)
{
   bt_error_t status;

   status = bt_mmap((bt_desc_t) vh, ptr, vme_addr, size, BT_RDWR, BT_SWAP_DEFAULT);
   if (status != BT_SUCCESS)
      bt_perror((bt_desc_t) vh, status, "vme_mmap error");

   return status == BT_SUCCESS ? 1 : (int) status;
}

/*------------------------------------------------------------------*/

int bt_vme_unmap(int vh, void *ptr, int size)
{
   bt_error_t status;

   status = bt_unmmap((bt_desc_t) vh, ptr, size);
   if (status != BT_SUCCESS)
      bt_perror((bt_desc_t) vh, status, "vme_unmap error");

   return status == BT_SUCCESS ? 1 : (int) status;
}

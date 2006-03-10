/*********************************************************************

  Name:         bt617.c
  Created by:   Stefan Ritt

  Cotents:      Routines for accessing VME over SBS Bit3 Model 617
                Interface under Windows NT using the NT device driver 
                Model 983 and under Linux using the vmehb device
                driver.

  $Id$

*********************************************************************/

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#ifdef _MSC_VER
#define BT_NTDRIVER
#define BT_WINNT
#define BT973
#include    "btapi.h"
#endif

#ifdef __linux__
#define BT1003
#include    "btapi.h"
#endif

#include    "mvmestd.h"

/*------------------------------------------------------------------*/

#define MAX_BT617_TABLES 32 /* one for each address mode */

typedef struct {
   int am;
   char devname[BT_MAX_DEV_NAME];
   bt_desc_t btd;
} BT617_TABLE;

/*------------------------------------------------------------------*/

int mvme_open(MVME_INTERFACE **vme, int index)
{
   BT617_TABLE *ptab;
   bt_devdata_t flag;
   int status;

   *vme = (MVME_INTERFACE *)malloc(sizeof(MVME_INTERFACE));
   if (*vme == NULL)
      return MVME_NO_MEM;

   (*vme)->am       = MVME_AM_DEFAULT;
   (*vme)->dmode    = MVME_DMODE_DEFAULT;
   (*vme)->blt_mode = MVME_BLT_NONE;

   (*vme)->handle = 0; /* use first entry in BT617_TABLE by default */

   (*vme)->table = (void *)malloc(sizeof(BT617_TABLE)*MAX_BT617_TABLES);
   if ((*vme)->table == NULL)
      return MVME_NO_MEM;

   memset((*vme)->table, 0, sizeof(BT617_TABLE)*MAX_BT617_TABLES);   
   ptab = (BT617_TABLE *) (*vme)->table;

   ptab->am = MVME_AM_DEFAULT;
   
   bt_gen_name(index, BT_DEV_A32, ptab->devname, BT_MAX_DEV_NAME);
   status = bt_open(&ptab->btd, ptab->devname, BT_RDWR);
   if (status != BT_SUCCESS) {
      bt_perror(ptab->btd, status, "bt_open error");
      return MVME_NO_INTERFACE;
   }
   bt_init(ptab->btd);
   bt_clrerr(ptab->btd);

   /* select swapping mode */
   flag = BT_SWAP_NONE;
   bt_set_info(ptab->btd, BT_INFO_SWAP, flag);

   /* set data mode */
   flag = BT_WIDTH_D32;
   bt_set_info(ptab->btd, BT_INFO_DATAWIDTH, flag);

   /* switch off block transfer */
   flag = FALSE;
   bt_set_info(ptab->btd, BT_INFO_BLOCK, flag);

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_exit(MVME_INTERFACE *vme)
{
   int i;
   bt_error_t status;
   BT617_TABLE *ptab;

   for (i=0 ; i<MAX_BT617_TABLES ; i++) {
      ptab = ((BT617_TABLE *)vme->table)+i;
      if (ptab->btd != NULL) {
         status = bt_close(ptab->btd);
         if (status != BT_SUCCESS)
            bt_perror(ptab->btd, status, "bt_close error");
         }
      }

   return MVME_SUCCESS;
}


/*------------------------------------------------------------------*/

int mvme_read(MVME_INTERFACE *vme, void *dst, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
   mvme_size_t n;
   bt_error_t status;
   BT617_TABLE *ptab;

   ptab = ((BT617_TABLE *)vme->table)+vme->handle;

   status = bt_read(ptab->btd, dst, vme_addr, n_bytes, (size_t *)&n);
   if (status != BT_SUCCESS)
      bt_perror(ptab->btd, status, "bt_read error");

   return n;
}

/*------------------------------------------------------------------*/

DWORD mvme_read_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr)
{
   mvme_size_t n;
   DWORD data;
   bt_error_t status;
   BT617_TABLE *ptab;

   ptab = ((BT617_TABLE *)vme->table)+vme->handle;

   if (vme->dmode == MVME_DMODE_D8)
      n = 1;
   else if (vme->dmode == MVME_DMODE_D16)
      n = 2;
   else
      n = 4;

   data = 0;
   status = bt_read(ptab->btd, &data, vme_addr, n, (size_t *)&n);
   if (status != BT_SUCCESS)
      bt_perror(ptab->btd, status, "bt_read error");

   return data;
}

/*------------------------------------------------------------------*/

int mvme_write(MVME_INTERFACE *vme, mvme_addr_t vme_addr, void *src, mvme_size_t n_bytes)
{
   mvme_size_t n;
   bt_error_t status;
   BT617_TABLE *ptab;

   ptab = ((BT617_TABLE *)vme->table)+vme->handle;

   status = bt_write(ptab->btd, src, vme_addr, n_bytes, (size_t *)&n);
   if (status != BT_SUCCESS)
      bt_perror(ptab->btd, status, "bt_write error");

   return n;
}
 
/*------------------------------------------------------------------*/

int mvme_write_value(MVME_INTERFACE *vme, mvme_addr_t vme_addr, DWORD value)
{
   mvme_size_t n;
   bt_error_t status;
   BT617_TABLE *ptab;
 
   ptab = ((BT617_TABLE *)vme->table)+vme->handle;

   if (vme->dmode == MVME_DMODE_D8)
      n = 1;
   else if (vme->dmode == MVME_DMODE_D16)
      n = 2;
   else
      n = 4;

   status = bt_write(ptab->btd, &value, vme_addr, n, (size_t *)&n);
   if (status != BT_SUCCESS)
      bt_perror(ptab->btd, status, "bt_write error");

   return n;
}
 
/*------------------------------------------------------------------*/

int bt617_set_am(MVME_INTERFACE *vme, int am)
{
   int i;
   BT617_TABLE *ptab;
   bt_error_t status;
   bt_devdata_t flag;

   for (i=0 ; i<MAX_BT617_TABLES ; i++)
      if (((BT617_TABLE *)vme->table)[i].am == am)
         break;

   if (((BT617_TABLE *)vme->table)[i].am == am) {

      vme->handle = i;
      ptab = ((BT617_TABLE *)vme->table)+i;
      bt_set_info(ptab->btd, BT_INFO_DMA_AMOD, am);
      bt_set_info(ptab->btd, BT_INFO_PIO_AMOD, am);
      bt_set_info(ptab->btd, BT_INFO_MMAP_AMOD, am);
      return MVME_SUCCESS;
   }

   /* search free slot */
   for (i=0 ; i<MAX_BT617_TABLES ; i++)
      if (((BT617_TABLE *)vme->table)[i].am == 0)
         break;
   if (i==MAX_BT617_TABLES)
      return MVME_NO_MEM;

   /* mode not yet open, open it */
   vme->handle = i;
   ptab = ((BT617_TABLE *)vme->table)+i;

   ptab->am = am;
   if (am == MVME_AM_A16_SD || am == MVME_AM_A16_ND)
      bt_gen_name(vme->index, BT_DEV_A16, ptab->devname, BT_MAX_DEV_NAME);
   else if (am == MVME_AM_A24_SB     ||
            am == MVME_AM_A24_SP     ||
            am == MVME_AM_A24_SD     ||
            am == MVME_AM_A24_NB     ||
            am == MVME_AM_A24_NP     ||
            am == MVME_AM_A24_ND     ||
            am == MVME_AM_A24_SMBLT  ||
            am == MVME_AM_A24_NMBLT  ||
            am == MVME_AM_A24        ||
            am == MVME_AM_A24_D64)
      bt_gen_name(vme->index, BT_DEV_A24, ptab->devname, BT_MAX_DEV_NAME);
   else
      bt_gen_name(vme->index, BT_DEV_A32, ptab->devname, BT_MAX_DEV_NAME);

   status = bt_open(&ptab->btd, ptab->devname, BT_RDWR);
   if (status != BT_SUCCESS) {
      bt_perror(ptab->btd, status, "bt_open error");
      return MVME_NO_INTERFACE;
   }
   bt_init(ptab->btd);
   bt_clrerr(ptab->btd);

   /* select swapping mode */
   flag = BT_SWAP_NONE;
   bt_set_info(ptab->btd, BT_INFO_SWAP, flag);

   /* set data mode */
   flag = BT_WIDTH_D16;
   bt_set_info(ptab->btd, BT_INFO_DATAWIDTH, flag);

   /* switch off block transfer */
   flag = FALSE;
   bt_set_info(ptab->btd, BT_INFO_BLOCK, flag);

   /* set amod */
   bt_set_info(ptab->btd, BT_INFO_DMA_AMOD, am);
   bt_set_info(ptab->btd, BT_INFO_PIO_AMOD, am);
   bt_set_info(ptab->btd, BT_INFO_MMAP_AMOD, am);

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int bt617_set_dmode(MVME_INTERFACE *vme, int dmode)
{
   BT617_TABLE *ptab;
   bt_devdata_t flag;

   vme->dmode = dmode;
   ptab = ((BT617_TABLE *)vme->table)+vme->handle;

   if (dmode == MVME_DMODE_D8)
      flag = BT_WIDTH_D8;
   else if (dmode == MVME_DMODE_D16)
      flag = BT_WIDTH_D16;
   else if (dmode == MVME_DMODE_D32)
      flag = BT_WIDTH_D32;
   else if (dmode == MVME_DMODE_D64)
      flag = BT_WIDTH_D64;
   else
      return MVME_UNSUPPORTED;
   
   bt_set_info(ptab->btd, BT_INFO_DATAWIDTH, flag);
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int bt617_set_blt(MVME_INTERFACE *vme, int blt)
{
   BT617_TABLE *ptab;
   bt_devdata_t flag;

   ptab = ((BT617_TABLE *)vme->table)+vme->handle;

   if (blt == MVME_BLT_BLT32 ||
       blt == MVME_BLT_MBLT64) {
      /* switch on block transfer */
      flag = TRUE;
      bt_set_info(ptab->btd, BT_INFO_BLOCK, flag);

      flag = 4;
      bt_set_info(ptab->btd, BT_INFO_DMA_POLL_CEILING, flag);

      flag = 4;
      bt_set_info(ptab->btd, BT_INFO_DMA_THRESHOLD, flag);
   } else {
      /* switch on block transfer */
      flag = FALSE;
      bt_set_info(ptab->btd, BT_INFO_BLOCK, flag);

      flag = 100000000;
      bt_set_info(ptab->btd, BT_INFO_DMA_POLL_CEILING, flag);

      flag = 100000000;
      bt_set_info(ptab->btd, BT_INFO_DMA_THRESHOLD, flag);
   }

   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_set_am(MVME_INTERFACE *vme, int am)
{
   vme->am = am;
   bt617_set_am(vme, am);
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_get_am(MVME_INTERFACE *vme, int *am)
{
   *am = vme->am;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_set_dmode(MVME_INTERFACE *vme, int dmode)
{
   vme->dmode = dmode;
   bt617_set_dmode(vme, dmode);
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_get_dmode(MVME_INTERFACE *vme, int *dmode)
{
   *dmode = vme->dmode;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_set_blt(MVME_INTERFACE *vme, int mode)
{
   vme->blt_mode = mode;
   bt617_set_blt(vme, mode);
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

int mvme_get_blt(MVME_INTERFACE *vme, int *mode)
{
   *mode = vme->blt_mode;
   return MVME_SUCCESS;
}

/*------------------------------------------------------------------*/

/* Small test program, comment back for testing...

main()
{
   int i;
   MVME_INTERFACE *vme;
   mvme_open(&vme, 0);

   mvme_set_am(vme, MVME_AM_A32_ND);
   mvme_set_dmode(vme, MVME_DMODE_D32);

   i = 0x0F;
   mvme_read(vme, &i, 0x00000000, 4);
   i = 0x00;
   mvme_read(vme, &i, 0xee000000, 4);

   do {
   i = 0x0F;
   mvme_write(vme, 0x780010, &i, 4);
   i = 0x00;
   mvme_write(vme, 0x780010, &i, 4);
   } while (1);

}
*/

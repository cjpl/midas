/********************************************************************\

  Name:         mscbbus.c
  Created by:   Stefan Ritt

  Contents:     MSCB bus driver to transport ASCII commands to scs_210 

  $Id$

\********************************************************************/

#include <stdarg.h>
#include <midas.h>
#include "../mscb/mscb.h"

static int debug_flag = 0;

typedef struct {
   char mscb_device[32];
   char pwd[32];
   unsigned short address;
} MSCBBUS_SETTINGS;

#define MSCBBUS_SETTINGS_STR "\
MSCB Device = STRING : [32] usb0\n\
Pwd = STRING : [32] \n\
Address = WORD : 0\n\
"

typedef struct {
   MSCBBUS_SETTINGS settings;
   int fd;                      /* device handle for MSCB device */
} MSCBBUS_INFO;

/*------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>

#define SUCCESS 1

/*----------------------------------------------------------------------------*/

int mscbbus_exit(MSCBBUS_INFO * info)
{
   mscb_exit(info->fd);

   return SUCCESS;
}

/*----------------------------------------------------------------------------*/

int mscbbus_write(MSCBBUS_INFO * info, char *data, int size)
{
   int i;

   if (debug_flag) {
      FILE *f;

      f = fopen("mscbbus.log", "a");
      fprintf(f, "write: ");
      for (i = 0; (int) i < size; i++)
         fprintf(f, "%X ", data[i]);
      fprintf(f, "\n");
      fclose(f);
   }

   i = mscb_write(info->fd, info->settings.address, 0, data, size);

   return i;
}

/*----------------------------------------------------------------------------*/

int mscbbus_read(MSCBBUS_INFO * info, char *str, int size, int timeout)
{
   int i, l, status;
   struct timeb start, now;
   double fstart, fnow;

   ftime(&start);
   fstart = start.time + start.millitm / 1000.0;

   memset(str, 0, size);
   
   for (l = 0; l < size ;) {

      i = size-l;
      status = mscb_read_no_retries(info->fd, info->settings.address, 0, str + l, &i);
      if (status != MSCB_SUCCESS)
         perror("mscbbus_read");

      if (i > 0)
         l += i;

      ftime(&now);
      fnow = now.time + now.millitm / 1000.0;

      if (fnow - fstart >= timeout / 1000.0)
         break;
   }

   if (debug_flag && l > 0) {
      FILE *f;

      f = fopen("mscbbus.log", "a");
      fprintf(f, "read: ");
      for (i = 0; i < size; i++)
         fprintf(f, "%X ", str[i]);
      fprintf(f, "\n");
      fclose(f);
   }

   return l;
}

/*----------------------------------------------------------------------------*/

int mscbbus_puts(MSCBBUS_INFO * info, char *str)
{
   if (debug_flag) {
      FILE *f;

      f = fopen("mscbbus.log", "a");
      fprintf(f, "puts: %s\n", str);
      fclose(f);
   }

   mscb_write(info->fd, info->settings.address, 0, str, strlen(str));

   return strlen(str);
}

/*----------------------------------------------------------------------------*/

int mscbbus_gets(MSCBBUS_INFO * info, char *str, int size, char *pattern, int timeout)
{
   int i, l, status;
   struct timeb start, now;
   double fstart, fnow;

   ftime(&start);
   fstart = start.time + start.millitm / 1000.0;

   memset(str, 0, size);
   for (l = 0; l < size - 1;) {
      
      i = size-l;
      status = mscb_read_no_retries(info->fd, info->settings.address, 0, str + l, &i);
      if (status != MSCB_SUCCESS)
         perror("mscbbus_read");

      if (i > 0)
         l += i;

      if (pattern && pattern[0])
         if (strstr(str, pattern) != NULL)
            break;

      ftime(&now);
      fnow = now.time + now.millitm / 1000.0;

      if (fnow - fstart >= timeout / 1000.0) {
         if (pattern && pattern[0])
            return 0;
         break;
      }
   }

   if (debug_flag && l > 0) {
      FILE *f;

      f = fopen("mscbbus.log", "a");
      fprintf(f, "getstr %s: %s\n", pattern, str);
      fclose(f);
   }

   return l;
}

/*----------------------------------------------------------------------------*/

int mscbbus_init(HNDLE hkey, void **pinfo)
{
   HNDLE hDB, hkeybd;
   INT size, status;
   MSCBBUS_INFO *info;

   /* allocate info structure */
   info = calloc(1, sizeof(MSCBBUS_INFO));
   *pinfo = info;

   cm_get_experiment_database(&hDB, NULL);

   /* create MSCBBUS settings record */
   status = db_create_record(hDB, hkey, "BD", MSCBBUS_SETTINGS_STR);
   if (status != DB_SUCCESS)
      return FE_ERR_ODB;

   db_find_key(hDB, hkey, "BD", &hkeybd);
   size = sizeof(info->settings);
   db_get_record(hDB, hkeybd, &info->settings, &size, 0);

   /* open port */
   // check if ethernet submaster
   info->fd = mscb_init(info->settings.mscb_device, sizeof(info->settings.mscb_device), info->settings.pwd, FALSE);

   if (info->fd < 0)
      return FE_ERR_HW;

   /* check if scs_210 is alive */
   status = mscb_addr(info->fd, MCMD_PING16, info->settings.address, 1);
   if (status != MSCB_SUCCESS) {
      if (status == MSCB_SEMAPHORE)
         printf("MSCB used by other process\n");
      else if (status == MSCB_SUBM_ERROR)
         printf("Error: MSCB Submaster not responding\n");
      else
         printf("MSCB Node %d does not respond\n", info->settings.address);

      return FE_ERR_HW;
   }

   return SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT mscbbus(INT cmd, ...)
{
   va_list argptr;
   HNDLE hkey;
   INT status, size, timeout;
   void *info;
   char *str, *pattern;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hkey = va_arg(argptr, HNDLE);
      info = va_arg(argptr, void *);
      status = mscbbus_init(hkey, info);
      break;

   case CMD_EXIT:
      info = va_arg(argptr, void *);
      status = mscbbus_exit(info);
      break;

   case CMD_NAME:
      info = va_arg(argptr, void *);
      str = va_arg(argptr, char *);
      strcpy(str, "mscbbus");
      break;

   case CMD_WRITE:
      info = va_arg(argptr, void *);
      str = va_arg(argptr, char *);
      size = va_arg(argptr, int);
      status = mscbbus_write(info, str, size);
      break;

   case CMD_READ:
      info = va_arg(argptr, void *);
      str = va_arg(argptr, char *);
      size = va_arg(argptr, INT);
      timeout = va_arg(argptr, INT);
      status = mscbbus_read(info, str, size, timeout);
      break;

   case CMD_PUTS:
      info = va_arg(argptr, void *);
      str = va_arg(argptr, char *);
      status = mscbbus_puts(info, str);
      break;

   case CMD_GETS:
      info = va_arg(argptr, void *);
      str = va_arg(argptr, char *);
      size = va_arg(argptr, INT);
      pattern = va_arg(argptr, char *);
      timeout = va_arg(argptr, INT);
      status = mscbbus_gets(info, str, size, pattern, timeout);
      break;

   case CMD_DEBUG:
      status = va_arg(argptr, INT);
      debug_flag = status;
      break;
   }

   va_end(argptr);

   return status;
}

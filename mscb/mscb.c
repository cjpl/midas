/********************************************************************\

  Name:         mscb.c
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus communication functions

  $Id$

\********************************************************************/

#define MSCB_LIBRARY_VERSION   "2.3.3"
#define MSCB_PROTOCOL_VERSION  "4"
#define MSCB_SUBM_VERSION      4

#ifdef _MSC_VER                 // Windows includes

#include <windows.h>
#include <conio.h>
#include <sys/timeb.h>

#elif defined(OS_LINUX)        // Linux includes

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

#ifdef HAVE_LIBUSB
#include <usb.h>
#endif

#elif defined(OS_DARWIN)

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <assert.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#endif

#include <stdio.h>
#include <musbstd.h>
#include <strlcpy.h>
#include "mscb.h"
#include "mscbrpc.h"

/*------------------------------------------------------------------*/

MSCB_FD mscb_fd[MSCB_MAX_FD];

/* pin definitions parallel port */

/*

  Ofs Inv   PC     DB25     SM300   DIR   MSCB Name

   0  No    D0 ----- 2 ----- P1.0   <->    |
   0  No    D1 ----- 3 ----- P1.1   <->    |
   0  No    D2 ----- 4 ----- P1.2   <->    |  D
   0  No    D3 ----- 5 ----- P1.3   <->     \ a
   0  No    D4 ----- 6 ----- P1.4   <->     / t
   0  No    D5 ----- 7 ----- P1.5   <->    |  a
   0  No    D6 ----- 8 ----- P1.6   <->    |
   0  No    D7 ----- 9 ----- P1.7   <->    |

   2  Yes   !STR --- 1  ---- P2.3    ->    /STROBE
   1  Yes   !BSY --- 11 ---- P2.7   <-     BUSY

   1  Yes   !ACK --- 10 ---- P2.6   <-     /DATAREADY
   2  Yes   !DSL --- 17 ---- P2.5    ->    /ACK

   1  No    PAP ---- 12 ---- P2.   <-     STATUS

   2  Yes   !ALF --- 14 ---- RESET   ->    /HW RESET
   2  No    INI ---- 16 ---- P2.4    ->    BIT9

*/


/* enumeration of control lines */

#define LPT_STROBE     1
#define LPT_ACK        2
#define LPT_RESET      3
#define LPT_BIT9       4
#define LPT_READMODE   5

#define LPT_BUSY       6
#define LPT_DATAREADY  7

/*
#define LPT_NSTROBE     (1<<0)
#define LPT_NSTROBE_OFS     2

#define LPT_NBUSY       (1<<7)
#define LPT_NBUSY_OFS       1

#define LPT_NDATAREADY  (1<<6)
#define LPT_NDATAREADY_OFS  1

#define LPT_NACK        (1<<3)
#define LPT_NACK_OFS        2

#define LPT_NRESET      (1<<1)
#define LPT_NRESET_OFS      2

#define LPT_BIT9        (1<<2)
#define LPT_BIT9_OFS        2

#define LPT_DIRECTION   (1<<5)
#define LPT_DIRECTION_OFS   2
*/

/* other constants */

#define TO_LPT           1000   /* ~1ms   */

#define TO_SHORT          100   /* 100 ms */
#define TO_LONG          5000   /* 5 s    */

extern int _debug_flag;         /* global debug flag */
extern void debug_log(char *format, int start, ...);

/* RS485 flags for USB submaster */
#define RS485_FLAG_BIT9      (1<<0)
#define RS485_FLAG_NO_ACK    (1<<1)
#define RS485_FLAG_SHORT_TO  (1<<2)
#define RS485_FLAG_LONG_TO   (1<<3)
#define RS485_FLAG_CMD       (1<<4)
#define RS485_FLAG_ADR_CYCLE (1<<5)

#define MUSB_TIMEOUT 3000 /* 3 sec */

/*------------------------------------------------------------------*/

/* cache definitions for mscb_link */

typedef struct {
   int fd;
   unsigned short adr;
   unsigned char index;
   void *data;
   unsigned char size;
   unsigned long last;
} CACHE_ENTRY;

CACHE_ENTRY *cache;
int n_cache;

#define CACHE_PERIOD 500        // update cache every 500 ms

/*------------------------------------------------------------------*/

/* cache definitions for mscb_info_var */

typedef struct {
   int            fd;
   unsigned short adr;
   unsigned char  index;
   MSCB_INFO_VAR  info;
} CACHE_INFO_VAR;

CACHE_INFO_VAR *cache_info_var;
int n_cache_info_var;

void mscb_clear_info_cache(int fd);

/*------------------------------------------------------------------*/

/* missing linux functions */

#if defined(OS_LINUX)

int kbhit()
{
   int n;

   ioctl(0, FIONREAD, &n);
   return (n > 0);
}

#endif

/*------------------------------------------------------------------*/

unsigned char crc8_data[] = {
   0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
   0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
   0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
   0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
   0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
   0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
   0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
   0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
   0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
   0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
   0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
   0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
   0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
   0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
   0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
   0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
   0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
   0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
   0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
   0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
   0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
   0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
   0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
   0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
   0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
   0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
   0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
   0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
   0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
   0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
   0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
   0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
};

unsigned char crc8(unsigned char *data, int len)
/********************************************************************\

  Routine: crc8

  Purpose: Calculate 8-bit cyclic redundancy checksum

  Input:
    unsigned char *data     data buffer
    int len                 data length in bytes


  Function value:
    unsighend char          CRC-8 code

\********************************************************************/
{
   int i;
   unsigned char crc8_code, index;

   crc8_code = 0;
   for (i = 0; i < len; i++) {
      index = data[i] ^ crc8_code;
      crc8_code = crc8_data[index];
   }

   return crc8_code;
}

/*------------------------------------------------------------------*/

int strieq(const char *str1, const char *str2)
{
   if (str1 == NULL && str2 != NULL)
      return 0;
   if (str1 != NULL && str2 == NULL)
      return 0;
   if (str1 == NULL && str2 == NULL)
      return 1;

   while (*str1)
      if (toupper(*str1++) != toupper(*str2++))
         return 0;

   if (*str2)
      return 0;

   return 1;
}

/*---- mutex functions ---------------------------------------------*/

int mscb_mutex_create(char *device)
{
#ifdef _MSC_VER
   HANDLE mutex_handle;

   mutex_handle = CreateMutex(NULL, FALSE, device);
   if (mutex_handle == 0)
      return 0;

   return (int)mutex_handle;

#elif defined(OS_LINUX)

#if (!defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
   union semun arg;
#else
   union semun {
      int val;
      struct semid_ds *buf;
      ushort *array;
   } arg;
#endif

   struct semid_ds buf;
   int key, status, fh, mutex_handle;
   char str[256];

   status = 1;

   /* create a unique key from the file name */
   sprintf(str, "/tmp/%s.SEM", device);
   key = ftok(str, 'M');
   if (key < 0) {
      fh = open(str, O_CREAT, 0644);
      close(fh);
      key = ftok(str, 'M');
      if (key < 0)
         return 0;
   }

   /* create or get semaphore */
   mutex_handle = semget(key, 1, 0);
   if (mutex_handle < 0) {
      mutex_handle = semget(key, 1, IPC_CREAT);
      status = 2;
   }

   if (mutex_handle < 0) {
      printf("semget() failed, errno = %d", errno);
      return 0;
   }

   buf.sem_perm.uid = getuid();
   buf.sem_perm.gid = getgid();
   buf.sem_perm.mode = 0666;
   arg.buf = &buf;

   semctl(mutex_handle, 0, IPC_SET, arg);

   /* if semaphore was created, set value to one */
   if (status == 2) {
      arg.val = 1;
      if (semctl(mutex_handle, 0, SETVAL, arg) < 0)
         return 0;
   }

   return mutex_handle;
#endif // OS_LINUX
}

/*------------------------------------------------------------------*/

int mscb_lock(int fd)
{
#ifdef _MSC_VER
   int status;

   if (fd);

   /* wait with a timeout of 10 seconds */
   status = WaitForSingleObject((HANDLE)mscb_fd[fd - 1].mutex_handle, 10000);

   if (status == WAIT_FAILED)
      return 0;
   if (status == WAIT_TIMEOUT)
      return 0;

#elif defined(OS_LINUX)

   if (mscb_fd[fd - 1].type == MSCB_TYPE_LPT) {
      if (ioctl(mscb_fd[fd - 1].fd, PPCLAIM))
         return 0;
   } 
#ifdef HAVE_LIBUSB
      else if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {
      if (usb_claim_interface((usb_dev_handle *) mscb_fd[fd - 1].ui->dev, 0) < 0)
         return 0;
   }
#endif // HAVE_LIBUSB

   else if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {

#if (!defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
      union semun arg;
#else
      union semun {
         int val;
         struct semid_ds *buf;
         ushort *array;
      } arg;
#endif
      time_t start_time, now;
      struct sembuf sb;
      int status;

      /* claim semaphore */
      sb.sem_num = 0;
      sb.sem_op = -1;           /* decrement semaphore */
      sb.sem_flg = SEM_UNDO;

      memset(&arg, 0, sizeof(arg));
      time(&start_time);

      do {
         status = semop(mscb_fd[fd - 1].mutex_handle, &sb, 1);

         /* return on success */
         if (status == 0)
            break;

         /* retry if interrupted by an alarm signal */
         if (errno == EINTR) {
            /* return if timeout expired */
            time(&now);
            if (now - start_time > 10)
               return 0;

            continue;
         }

         return 0;
      } while (1);
   }

#endif // OS_LINUX
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_release(int fd)
{
#ifdef _MSC_VER
   int status;

   if (fd);

   status = ReleaseMutex((HANDLE)mscb_fd[fd - 1].mutex_handle);
   if (status == FALSE)
      return 0;

#elif defined(OS_LINUX)

   if (mscb_fd[fd - 1].type == MSCB_TYPE_LPT) {
      if (ioctl(mscb_fd[fd - 1].fd, PPRELEASE))
         return 0;
   } 

#ifdef HAVE_LIBUSB
     else if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {
      if (usb_release_interface((usb_dev_handle *) mscb_fd[fd - 1].ui->dev, 0) < 0)
         return 0;
   }
#endif // HAVE_LIBUSB
     
   else if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {
 
      /* release semaphore */
      struct sembuf sb;
      int status;

      sb.sem_num = 0;
      sb.sem_op = 1;            /* increment semaphore */
      sb.sem_flg = SEM_UNDO;

      do {
         status = semop(mscb_fd[fd - 1].mutex_handle, &sb, 1);

         /* return on success */
         if (status == 0)
            break;

         /* retry if interrupted by a ss_wake signal */
         if (errno == EINTR)
            continue;

         return 0;
      } while (1);
   }

#endif // OS_LINUX

   return MSCB_SUCCESS;
}

/*---- low level functions for direct port access ------------------*/

void pp_wcontrol(int fd, int signal, int flag)
/* write control signal */
{
   static unsigned int mask = 0;

   switch (signal) {
   case LPT_STROBE:            // negative port, negative MSCB usage
      mask = flag ? mask | (1 << 0) : mask & ~(1 << 0);
      break;
   case LPT_RESET:             // negative port, negative MSCB usage
      mask = flag ? mask | (1 << 1) : mask & ~(1 << 1);
      break;
   case LPT_BIT9:              // positive
      mask = flag ? mask | (1 << 2) : mask & ~(1 << 2);
      break;
   case LPT_ACK:               // negative port, negative MSCB usage
      mask = flag ? mask | (1 << 3) : mask & ~(1 << 3);
      break;
   case LPT_READMODE:          // positive
      mask = flag ? mask | (1 << 5) : mask & ~(1 << 5);
      break;
   }

#if defined(_MSC_VER)
   _outp((unsigned short) (mscb_fd[fd - 1].fd + 2), mask);
#elif defined(OS_LINUX)
   ioctl(mscb_fd[fd - 1].fd, PPWCONTROL, &mask);
#else
   /* no-op unsupported operation */
#endif
}

/*------------------------------------------------------------------*/

int pp_rstatus(int fd, int signal)
{
/* read status signal */
   unsigned int mask = 0;

#if defined(_MSC_VER)
   mask = _inp((unsigned short) (mscb_fd[fd - 1].fd + 1));
#elif defined(OS_LINUX)
   ioctl(mscb_fd[fd - 1].fd, PPRSTATUS, &mask);
#else
   /* no-op unsupported operation */
#endif

   switch (signal) {
   case LPT_BUSY:
      return (mask & (1 << 7)) == 0;
   case LPT_DATAREADY:
      return (mask & (1 << 6)) == 0;
   }

   return 0;
}

/*------------------------------------------------------------------*/

void pp_setdir(int fd, unsigned int r)
{
/* set port direction: r==0:write mode;  r==1:read mode */
#if defined(_MSC_VER)
   pp_wcontrol(fd, LPT_READMODE, r);
#elif defined(OS_LINUX)
   if (ioctl(mscb_fd[fd - 1].fd, PPDATADIR, &r))
      perror("PPDATADIR");
#else
   /* no-op unsupported operation */
#endif
}

/*------------------------------------------------------------------*/

void pp_wdata(int fd, unsigned int data)
/* output data byte */
{
#if defined(_MSC_VER)
   _outp((unsigned short) mscb_fd[fd - 1].fd, data);
#elif defined(OS_LINUX)
   ioctl(mscb_fd[fd - 1].fd, PPWDATA, &data);
#else
   /* no-op unsupported operation */
#endif
}

/*------------------------------------------------------------------*/

unsigned char pp_rdata(int fd)
/* intput data byte */
{
#if defined(_MSC_VER)
   return (unsigned char)_inp((unsigned short) mscb_fd[fd - 1].fd);
#elif defined(OS_LINUX)
   unsigned char data;
   if (ioctl(mscb_fd[fd - 1].fd, PPRDATA, &data))
      perror("PPRDATA");
   return data;
#else
   /* no-op unsupported operation */
#endif
}

/*------------------------------------------------------------------*/

int subm250_open(MUSB_INTERFACE **ui, int usb_index)
{
   int  i, status, found;
   char buf[80];

   for (i = found = 0 ; i<127 ; i++) {
      status = musb_open(ui, 0x10C4, 0x1175, i, 1, 0);
      if (status != MUSB_SUCCESS) {
         printf("musb_open returned %d\n", status);
         return EMSCB_NOT_FOUND;
      }

      /* check if it's a subm_250 */
      buf[0] = 0;
      musb_write(*ui, 0, buf, 1, MUSB_TIMEOUT);

      status = musb_read(*ui, 0, buf, sizeof(buf), MUSB_TIMEOUT);
      if (strcmp(buf, "SUBM_250") == 0)
         if (found++ == usb_index)
            return MSCB_SUCCESS;
      printf("buf: %s\n", buf);
   }

   return EMSCB_NOT_FOUND;
}

/*------------------------------------------------------------------*/

int mscb_out(int index, unsigned char *buffer, int len, int flags)
/********************************************************************\

  Routine: mscb_out

  Purpose: Write number of bytes to SM via parallel port or USB

  Input:
    int  index              index to file descriptor table
    char *buffer            data buffer
    int  len                number of bytes in buffer
    int  flags              bit combination of:
                              RS485_FLAG_BIT9      node arressing
                              RS485_FLAG_NO_ACK    no acknowledge
                              RS485_FLAG_SHORT_TO  short/
                              RS485_FLAG_LONG_TO     long timeout
                              RS485_FLAG_CMD       direct subm_250 command

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout

\********************************************************************/
{
   int i, timeout;
   unsigned char usb_buf[65], eth_buf[65];

   if (index > MSCB_MAX_FD || index < 1 || !mscb_fd[index - 1].type)
      return MSCB_INVAL_PARAM;

   /*---- USB code ----*/

   if (mscb_fd[index - 1].type == MSCB_TYPE_USB) {

      if (len >= 64 || len < 1)
         return MSCB_INVAL_PARAM;

      /* add flags in first byte of USB buffer */
      usb_buf[0] = (unsigned char)flags;
      memcpy(usb_buf + 1, buffer, len);

      /* send on OUT pipe */
      i = musb_write(mscb_fd[index - 1].ui, 0, usb_buf, len + 1, MUSB_TIMEOUT);

      if (i == 0) {
         /* USB submaster might have been dis- and reconnected, so reinit */
         musb_close(mscb_fd[index - 1].ui);

         i = subm250_open(&mscb_fd[index - 1].ui, atoi(mscb_fd[index - 1].device+3));
         if (i < 0)
            return MSCB_TIMEOUT;

         i = musb_write(mscb_fd[index - 1].ui, 0, usb_buf, len + 1, MUSB_TIMEOUT);
      }

      if (i != len + 1)
         return MSCB_TIMEOUT;
   }

   /*---- LPT code ----*/

   if (mscb_fd[index - 1].type == MSCB_TYPE_LPT) {

      /* remove accidental strobe */
      pp_wcontrol(index, LPT_STROBE, 0);

      for (i = 0; i < len; i++) {
         /* wait for SM ready */
         for (timeout = 0; timeout < TO_LPT; timeout++) {
            if (!pp_rstatus(index, LPT_BUSY))
               break;
         }
         if (timeout == TO_LPT) {
#ifndef _USRDLL
            printf("Automatic submaster reset.\n");
#endif

            mscb_release(index);
            mscb_reset(index);
            mscb_lock(index);

            Sleep(100);

            /* wait for SM ready */
            for (timeout = 0; timeout < TO_LPT; timeout++) {
               if (!pp_rstatus(index, LPT_BUSY))
                  break;
            }

            if (timeout == TO_LPT)
               return MSCB_TIMEOUT;
         }

         /* output data byte */
         pp_wdata(index, buffer[i]);

         /* set bit 9 */
         if (flags & RS485_FLAG_BIT9)
            pp_wcontrol(index, LPT_BIT9, 1);
         else
            pp_wcontrol(index, LPT_BIT9, 0);

         /* set strobe */
         pp_wcontrol(index, LPT_STROBE, 1);

         /* wait for busy to become active */
         for (timeout = 0; timeout < TO_LPT; timeout++) {
            if (pp_rstatus(index, LPT_BUSY))
               break;
         }

         if (timeout == TO_LPT) {
#ifndef _USRDLL
            printf("Automatic submaster reset.\n");
#endif

            mscb_release(index);
            mscb_reset(index);
            mscb_lock(index);

            Sleep(100);

            /* try again */
            return mscb_out(index, buffer, len, flags);
         }

         if (timeout == TO_LPT) {
            pp_wdata(index, 0xFF);
            pp_wcontrol(index, LPT_STROBE, 0);
            return MSCB_TIMEOUT;
         }

         /* remove data, make port available for input */
         pp_wdata(index, 0xFF);

         /* remove strobe */
         pp_wcontrol(index, LPT_STROBE, 0);
      }
   }

   /*---- Ethernet code ----*/

   if (mscb_fd[index - 1].type == MSCB_TYPE_ETH) {
      if (len >= 64 || len < 1)
         return MSCB_INVAL_PARAM;

      /* write buffer size in first byte */
      eth_buf[0] = (unsigned char)(len + 1);

      /* add flags in second byte of Ethernet buffer */
      eth_buf[1] = (unsigned char)flags;
      memcpy(eth_buf + 2, buffer, len);

      /* send over TCP link */
      i = msend_tcp(mscb_fd[index - 1].fd, eth_buf, len + 2);

      if (i <= 0) {
         /* Ethernet submaster might have been dis- and reconnected, so reinit */
         if (mscb_fd[index - 1].fd > 0)
            mrpc_disconnect(mscb_fd[index - 1].fd);

         mscb_fd[index - 1].fd = mrpc_connect(mscb_fd[index - 1].device, MSCB_NET_PORT);

         if (mscb_fd[index - 1].fd < 0)
            return MSCB_SUBM_ERROR;

         i = msend_tcp(mscb_fd[index - 1].fd, eth_buf, len + 2);
      }

      if (i != len + 2)
         return MSCB_TIMEOUT;
   }

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_in1(int fd, unsigned char *c, int timeout)
/********************************************************************\

  Routine: mscb_in1

  Purpose: Read one byte from SM via parallel port

  Input:
    int  fd                 file descriptor for connection
    int  timeout            timeout in milliseconds

  Output:
    char *c                 data bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout

\********************************************************************/
{
   int i;

   /* wait for DATAREADY, each port access takes roughly 1us */
   timeout = (int) (timeout*1000 / 1.3);

   for (i = 0; i < timeout; i++) {
      if (pp_rstatus(fd, LPT_DATAREADY))
         break;
   }
   if (i == timeout)
      return MSCB_TIMEOUT;

   /* switch to input mode */
   pp_setdir(fd, 1);

   /* signal switch to input */
   pp_wcontrol(fd, LPT_ACK, 1);

   /* wait for data ready */
   for (i = 0; i < 1000; i++) {
      if (!pp_rstatus(fd, LPT_DATAREADY))
         break;
   }

   /* read data */
   *c = pp_rdata(fd);

   /* signal data read */
   pp_wcontrol(fd, LPT_ACK, 0);

   /* wait for mode switch */
   for (i = 0; i < 1000; i++) {
      if (pp_rstatus(fd, LPT_DATAREADY))
         break;
   }

   /* switch to output mode */
   pp_setdir(fd, 0);

   /* indicate mode switch */
   pp_wcontrol(fd, LPT_ACK, 1);

   /* wait for end of cycle */
   for (i = 0; i < 1000; i++) {
      if (!pp_rstatus(fd, LPT_DATAREADY))
         break;
   }

   /* remove acknowlege */
   pp_wcontrol(fd, LPT_ACK, 0);

   if (i == 1000)
      return MSCB_TIMEOUT;

   for (i = 0; i < 50; i++)
      pp_rdata(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int recv_eth(int sock, char *buf, int buffer_size, int millisec)
{
   int n_received, n, status;
   unsigned char buffer[65];
   fd_set readfds;
   struct timeval timeout;

   /* check for valid socket */
   if (sock < 0)
      return 0;

   if (buffer_size > sizeof(buffer))
      buffer_size = sizeof(buffer);

   /* at least 3 sec timeout for slow network connections */
   if (millisec < 3000)
      millisec = 3000;

   /* receive buffer in TCP mode, first byte contains remaining bytes */
   n_received = 0;
   do {
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec = millisec / 1000;
      timeout.tv_usec = (millisec % 1000) * 1000;

      do {
         status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);
      } while (status == -1);        /* dont return if an alarm signal was cought */

      if (!FD_ISSET(sock, &readfds))
         return n_received;

      n = recv(sock, buffer + n_received, sizeof(buffer) - n_received, 0);

      if (n <= 0)
         return n;

      n_received += n;

   } while (n_received < 1 && n_received < buffer[0]+1);

   if (buffer[0] > buffer_size) {
      memcpy(buf, buffer+1, buffer_size);
      return buffer_size;
   } else
      memcpy(buf, buffer+1, buffer[0]);

   return buffer[0];
}

/*------------------------------------------------------------------*/

int mscb_in(int index, char *buffer, int size, int timeout)
/********************************************************************\

  Routine: mscb_in

  Purpose: Read number of bytes from SM via parallel port

  Input:
    int  index              index to file descriptor table
    int  size               size of data buffer
    int  timeout            timeout in milliseconds

  Output:
    char *buffer            data buffer

  Function value:
    int                     number of received bytes, -1 for timeout

\********************************************************************/
{
   int i, n, len, status;

   n = 0;
   len = -1;

   if (index > MSCB_MAX_FD || index < 1 || !mscb_fd[index - 1].type)
      return MSCB_INVAL_PARAM;

   /*---- USB code ----*/
   if (mscb_fd[index - 1].type == MSCB_TYPE_USB) {

      /* receive result on IN pipe */
      n = musb_read(mscb_fd[index - 1].ui, 0, buffer, size, timeout);

   }

   /*---- Ethernet code ----*/
   if (mscb_fd[index - 1].type == MSCB_TYPE_ETH) {

      /* receive result on IN pipe */
      n = recv_eth(mscb_fd[index - 1].fd, buffer, size, timeout);
   }

   /*---- LPT code ----*/
   if (mscb_fd[index - 1].type == MSCB_TYPE_LPT) {

      /* wait for MCMD_ACK command */
      do {
         status = mscb_in1(index, buffer, timeout);

         /* only read single byte */
         if (size == 1)
            return status;

         /* return in case of timeout */
         if (status != MSCB_SUCCESS)
            return -1;

         /* check for Acknowledge */
         if ((buffer[0] & 0xF8) == MCMD_ACK) {
            len = buffer[n++] & 0x7;

            /* check for variable length data block */
            if (len == 7) {
               status = mscb_in1(index, buffer + n, timeout);

               /* return in case of timeout */
               if (status != MSCB_SUCCESS)
                  return -1;

               len = buffer[n++];
            }
         }

      } while (len == -1);

      /* receive remaining bytes and CRC code */
      for (i = 0; i < len + 1; i++) {
         status = mscb_in1(index, buffer + n + i, timeout);

         /* return in case of timeout */
         if (status != MSCB_SUCCESS)
            return -1;
      }

      n += i;
   }

   return n;
}

/*------------------------------------------------------------------*/

int lpt_init(char *device, int index)
/********************************************************************\

  Routine: lpt_init

  Purpose: Initialize MSCB submaster on parallel port

  Input:
    char *device            Device name "lptx"/"/dev/parportx"
    int  index              Index to mscb_fd[] array

  Output:
    int  mscb_fd[index].fd  File descriptor for device or
                            DirectIO address
  Function value:
    0                       Successful completion
    EMSCB_INVAL_PARAM       Invalid "device" parameter
    EMSCB_COMM_ERROR        Submaster does not respond
    EMSCB_NO_DIRECTIO       No DirectIO driver
    EMSCB_LOCKED            MSCB system locked by other user
    EMSCB_NO_ACCESS         No access to submaster

\********************************************************************/
{
   int status;
   char c;

#ifdef _MSC_VER

   /* under NT, user directio driver */

   OSVERSIONINFO vi;
   DWORD buffer[4];
   DWORD size;
   HANDLE hdio;

   /* derive base address from device name */
   if (atoi(device + 3) == 1)
      mscb_fd[index].fd = 0x378;
   else if (atoi(device + 3) == 2)
      mscb_fd[index].fd = 0x278;
   else
      return EMSCB_INVAL_PARAM;

   buffer[0] = 6;               /* give IO */
   buffer[1] = mscb_fd[index].fd;
   buffer[2] = buffer[1] + 4;
   buffer[3] = 0;

   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   /* use DirectIO driver under NT to gain port access */
   if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hdio == INVALID_HANDLE_VALUE) {
         //printf("mscb.c: Cannot access parallel port (No DirectIO driver installed)\n");
         return EMSCB_NO_DIRECTIO;
      }

      if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), NULL, 0, &size, NULL))
         return EMSCB_NO_DIRECTIO;
   }
#elif defined(OS_LINUX)

   int i;

   mscb_fd[index].fd = open(device, O_RDWR);
   if (mscb_fd[index].fd < 0) {
#ifndef _USRDLL
      perror("open");
      printf("Please make sure that device \"%s\" is world readable/writable\n", device);
#endif
      return EMSCB_NO_ACCESS;
   }

   if (ioctl(mscb_fd[index].fd, PPCLAIM)) {
#ifndef _USRDLL
      perror("PPCLAIM");
      printf("Please load driver via \"modprobe parport_pc\" as root\n");
#endif
      return EMSCB_NO_ACCESS;
   }

   if (ioctl(mscb_fd[index].fd, PPRELEASE)) {
#ifndef _USRDLL
      perror("PPRELEASE");
#endif
      return EMSCB_NO_ACCESS;
   }

   i = IEEE1284_MODE_BYTE;
   if (ioctl(mscb_fd[index].fd, PPSETMODE, &i)) {
#ifndef _USRDLL
      perror("PPSETMODE");
#endif
      return EMSCB_NO_ACCESS;
   }
#endif

   status = mscb_lock(index + 1);
   if (status != MSCB_SUCCESS)
      return EMSCB_LOCKED;

   /* set initial state of handshake lines */
   pp_wcontrol(index + 1, LPT_RESET, 0);
   pp_wcontrol(index + 1, LPT_STROBE, 0);

   /* switch to output mode */
   pp_setdir(index + 1, 0);

   /* wait for submaster irritated by a stuck LPT_STROBE */
   Sleep(100);

   /* check if SM alive */
   if (pp_rstatus(index + 1, LPT_BUSY)) {
      //printf("mscb.c: No SM present on parallel port\n");
      mscb_release(index + 1);
      return EMSCB_LPT_ERROR;
   }

   /* empty RBuffer of SM */
   do {
      status = mscb_in1(index + 1, &c, 1000);
      if (status == MSCB_SUCCESS)
         printf("%02X ", c);
   } while (status == MSCB_SUCCESS);

   mscb_release(index + 1);

   return 0;
}

/*------------------------------------------------------------------*/

int lpt_close(int fd)
/********************************************************************\

  Routine: lpt_close

  Purpose: Close handle to MSCB submaster

  Input:
    int  index              Index to mscb_fd[] array

  Function value:
    MSCB_SUCCES             Successful completion

\********************************************************************/
{
#ifdef _MSC_VER

   /* under NT, user directio driver */

   OSVERSIONINFO vi;
   DWORD buffer[4];
   DWORD size;
   HANDLE hdio;

   buffer[0] = 7;               /* lock port */
   buffer[1] = fd;
   buffer[2] = buffer[1] + 4;
   buffer[3] = 0;

   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   /* use DirectIO driver under NT to gain port access */
   if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hdio == INVALID_HANDLE_VALUE) {
#ifndef _USRDLL
         printf("mscb.c: Cannot access parallel port (No DirectIO driver installed)\n");
#endif
         return -1;
      }

      if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), NULL, 0, &size, NULL))
         return -1;
   }
#elif defined(OS_LINUX)

   close(fd);

#endif

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

void mscb_get_version(char *lib_version, char *prot_version)
{
   strcpy(lib_version, MSCB_LIBRARY_VERSION);
   strcpy(prot_version, MSCB_PROTOCOL_VERSION);
}

/*------------------------------------------------------------------*/

int mrpc_connected(int fd)
{
   return mscb_fd[fd - 1].type == MSCB_TYPE_RPC;
}

/*------------------------------------------------------------------*/

int mscb_init(char *device, int bufsize, char *password, int debug)
/********************************************************************\

  Routine: mscb_init

  Purpose: Initialize and open MSCB

  Input:
    char *device            Under NT: lpt1 or lpt2
                            Under Linux: /dev/parport0 or /dev/parport1
                            "<host>:device" for RPC connection
                            usbx for USB connection
                            mscbxxx for Ethernet connection

                            If device equals "", the function
                            mscb_select_device is called which selects
                            the first available device or asks the user
                            which one to use.
    int bufsize             Size of "device" string, in case no device
                            is specified from the caller and this function
                            returns the chosen device
    int password            Optional password, used for ethernet submasters
    int debug               Debug flag. If "1" debugging information is
                            written to "mscb_debug.log" in the current
                            directory. If "-1", do not ask for device
                            (useb by LabView DLL)

  Function value:
    int fd                  device descriptor for connection, negative
                            number in case of error:

    EMSCB_NO_MEM            Out of memory for file descriptors
    EMSCB_RPC_ERROR         Cannot talk to RPC server
    EMSCB_COMM_ERROR        Submaster does not reply on echo
    EMSCB_NO_DIRECTIO       No DirectIO driver for LPT installed
    EMSCB_LOCKED            MSCB system locked by other user
    EMSCB_NO_ACCESS         No access to submaster
    EMSCB_INVAL_PARAM       Invalid parameter
    EMSCB_NOT_FOUND         USB submaster not found
    EMSCB_NO_WRITE_ACCESS   No write access under linux
    EMSCB_WRONG_PASSWORD    Wrong password
    EMSCB_LPT_ERROR         Error talking to LPT submaster
    EMSCB_SUMB_VERSION      Submaster has wrong protocol version

\********************************************************************/
{
   int index, i, n;
   int status;
   char host[256], port[256], dev3[256], remote_device[256];
   unsigned char buf[64];

   if (!device[0]) {
      if (debug == -1)
         mscb_select_device(device, bufsize, 0); /* for LabView */
      else
         mscb_select_device(device, bufsize, 1); /* interactively ask for device */
   }

   /* search for open file descriptor for same device, used by LabView */
   for (index = 0; index < MSCB_MAX_FD; index++)
      if (mscb_fd[index].fd != 0 && strcmp(mscb_fd[index].device, device) == 0)
         return index+1;

   /* search for new file descriptor */
   for (index = 0; index < MSCB_MAX_FD; index++)
      if (mscb_fd[index].fd == 0)
         break;

   if (index == MSCB_MAX_FD)
      return EMSCB_NO_MEM;

   /* set global debug flag */
   _debug_flag = (debug == 1);

   debug_log("mscb_init(device=\"%s\",bufsize=%d,password=\"%s\",debug=%d) ", 1, device, bufsize, password, debug);

   /* clear caches */
   for (i = 0; i < n_cache; i++)
      free(cache[i].data);
   free(cache);
   n_cache = 0;

   free(cache_info_var);
   n_cache_info_var = 0;

   /* check for RPC connection */
   if (strchr(device, ':')) {
      strlcpy(mscb_fd[index].device, device, sizeof(mscb_fd[index].device));
      mscb_fd[index].type = MSCB_TYPE_RPC;

      strcpy(remote_device, strchr(device, ':') + 1);
      strcpy(host, device);
      *strchr(host, ':') = 0;

      mscb_fd[index].fd = mrpc_connect(host, MSCB_RPC_PORT);

      if (mscb_fd[index].fd < 0) {
         mscb_fd[index].fd = 0;
         return EMSCB_RPC_ERROR;
      }

      mscb_fd[index].remote_fd = mrpc_call(mscb_fd[index].fd, RPC_MSCB_INIT, remote_device, bufsize, debug);
      if (mscb_fd[index].remote_fd < 0) {
         mrpc_disconnect(mscb_fd[index].fd);
         mscb_fd[index].fd = 0;
         return EMSCB_RPC_ERROR;
      }

      sprintf(device, "%s:%s", host, remote_device);

      debug_log("return %d\n", 0, index + 1);
      return index + 1;
   }

   /* check which device type */
   strcpy(dev3, device);
   dev3[3] = 0;
   strcpy(mscb_fd[index].device, device);
   mscb_fd[index].type = 0;

   /* LPT port with direct address */
   if (device[1] == 'x') {
      mscb_fd[index].type = MSCB_TYPE_LPT;
      sscanf(port + 2, "%x", &mscb_fd[index].fd);
   }

   /* LPTx */
   if (strieq(dev3, "LPT"))
      mscb_fd[index].type = MSCB_TYPE_LPT;

   /* /dev/parportx */
   if (strstr(device, "parport"))
      mscb_fd[index].type = MSCB_TYPE_LPT;

   /* USBx */
   if (strieq(dev3, "usb"))
      mscb_fd[index].type = MSCB_TYPE_USB;

   /* MSCBxxx */
   if (strieq(dev3, "msc") || (atoi(device) > 0 && strchr(device, '.')))
      mscb_fd[index].type = MSCB_TYPE_ETH;

   if (mscb_fd[index].type == 0)
      return EMSCB_INVAL_PARAM;

   /*---- initialize submaster ----*/

   if (mscb_fd[index].mutex_handle == 0)
      mscb_fd[index].mutex_handle = mscb_mutex_create(device);

   if (mscb_fd[index].type == MSCB_TYPE_LPT) {

      status = lpt_init(device, index);
      if (status < 0)
         return status;
   }

   if (mscb_fd[index].type == MSCB_TYPE_USB) {

      if (!mscb_lock(index + 1)) {
         debug_log("return EMSCB_LOCKED\n", 0);
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         return EMSCB_LOCKED;
      }

      status = subm250_open(&mscb_fd[index].ui, atoi(device+3));
      if (status != MSCB_SUCCESS)
         return EMSCB_NOT_FOUND;

      /* mark device descriptor used */
      mscb_fd[index].fd = 1;

      n = 0;

      /* linux needs some time to start-up ...??? */
      for (i = 0; i < 10; i++) {
         /* check if submaster alive */
         buf[0] = MCMD_ECHO;
         mscb_out(index + 1, buf, 1, RS485_FLAG_CMD);

         n = mscb_in(index + 1, buf, 2, TO_SHORT);

         if (n == 2 && buf[0] == MCMD_ACK) {

            /* check version */
            if (buf[1] != MSCB_SUBM_VERSION) {
               debug_log("return EMSCB_SUBM_VERSION\n", 0);
               memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
               mscb_release(index + 1);
               return EMSCB_SUBM_VERSION;
            }

            break;
         }
      }

      mscb_release(index + 1);

      if (n != 2 || buf[0] != MCMD_ACK) {
         debug_log("return EMSCB_COMM_ERROR\n", 0);
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         return EMSCB_COMM_ERROR;
      }
   }

   if (mscb_fd[index].type == MSCB_TYPE_ETH) {

      mscb_fd[index].fd = mrpc_connect(mscb_fd[index].device, MSCB_NET_PORT);

      if (mscb_fd[index].fd < 0) {
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         debug_log("return EMSCB_RPC_ERROR\n", 0);
         return EMSCB_RPC_ERROR;
      }

      if (!mscb_lock(index + 1)) {
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         debug_log("return EMSCB_LOCKED\n", 0);
         return EMSCB_LOCKED;
      }

      /* check if submaster alive */
      buf[0] = MCMD_ECHO;
      mscb_out(index + 1, buf, 1, RS485_FLAG_CMD);

      n = mscb_in(index + 1, buf, 2, TO_SHORT);
      mscb_release(index + 1);

      if (n == 2 && buf[0] == MCMD_ACK) {
         /* check version */
         if (buf[1] != MSCB_SUBM_VERSION) {
            debug_log("return EMSCB_SUBM_VERSION\n", 0);
            memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
            return EMSCB_SUBM_VERSION;
         }
      }

      /* authenticate */
      memset(buf, 0, sizeof(buf));
      buf[0] = MCMD_TOKEN;
      if (password)
         strcpy(buf+1, password);
      else
         buf[1] = 0;

      if (!mscb_lock(index + 1)) {
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         debug_log("return EMSCB_LOCKED\n", 0);
         return EMSCB_LOCKED;
      }

      mscb_out(index + 1, buf, 21, RS485_FLAG_CMD);
      n = mscb_in(index + 1, buf, 2, TO_SHORT);
      mscb_release(index + 1);

      if (n != 1 || (buf[0] != MCMD_ACK && buf[0] != 0xFF)) {
         mscb_exit(index + 1);
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         debug_log("return EMSCB_COMM_ERROR\n", 0);
         return EMSCB_COMM_ERROR;
      }

      if (buf[0] == 0xFF) {
         mscb_exit(index + 1);
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         debug_log("return EMSCB_WRONG_PASSWORD\n", 0);
         return EMSCB_WRONG_PASSWORD;
      }

      debug_log("return %d\n", 0, index + 1);
      return index + 1;
   }

   debug_log("return %d\n", 0, index + 1);
   return index + 1;
}

/*------------------------------------------------------------------*/

int mscb_exit(int fd)
/********************************************************************\

  Routine: mscb_exit

  Purpose: Close a MSCB interface

  Input:
    int  fd                 file descriptor for connection

  Function value:
    MSCB_SUCCESS            Successful completion

\********************************************************************/
{
   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd)) {
      mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_EXIT, mscb_fd[fd - 1].remote_fd);
      mrpc_disconnect(mscb_fd[fd - 1].fd);
   }

   if (mscb_fd[fd - 1].type == MSCB_TYPE_USB)
      musb_close(mscb_fd[fd - 1].ui);

   if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH)
      mrpc_disconnect(mscb_fd[fd - 1].fd);

   /* outcommented, other client (like Labview) could still use LPT ...
   if (mscb_fd[fd - 1].type == MSCB_TYPE_LPT)
      lpt_close(mscb_fd[fd - 1].fd);
   */

   memset(&mscb_fd[fd - 1], 0, sizeof(MSCB_FD));

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

void mscb_cleanup(int sock)
/* Called by mrpc_server_loop to remove stale fd's on broken connection */
{
   int i;

   for (i = 0; i < MSCB_MAX_FD; i++)
      if (mscb_fd[i].fd && mscb_fd[i].remote_fd == sock)
         memset(&mscb_fd[i], 0, sizeof(MSCB_FD));
}

/*------------------------------------------------------------------*/

void mscb_get_device(int fd, char *device, int bufsize)
/********************************************************************\

  Routine: mscb_get_device

  Purpose: Return device name for fd

  Input:
    int fd                  File descriptor obtained wiv mscb_init()
    int bufsize             Size of device string
    char *device            device name, "" if invalid fd

\********************************************************************/
{
   char str[256];

   if (!device)
      return;

   *device = 0;
   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return;

   if (mrpc_connected(fd)) {
      mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_GET_DEVICE,
                mscb_fd[fd - 1].remote_fd, str, sizeof(str));
   }

   strlcpy(device, mscb_fd[fd-1].device, bufsize);
}

/*------------------------------------------------------------------*/

void mscb_check(char *device, int size)
/********************************************************************\

  Routine: mscb_check

  Purpose: Check IO pins of port

  Input:
    char *device            Under NT: lpt1 or lpt2
                            Under Linux: /dev/parport0 or /dev/parport1

  Function value:
    int fd                  device descriptor for connection, -1 if
                            error

\********************************************************************/
{
   int i, fd, d;

   mscb_init(device, size, "", 0);
   fd = 1;

   mscb_lock(fd);

   printf("Toggling %s output pins, hit ENTER to stop.\n", device);
   printf("GND = 19-25, toggling 2-9, 1, 14, 16 and 17\n\n");
   do {
      printf("\r11111111 1111");
      fflush(stdout);
      pp_wdata(fd, 0xFF);
      pp_wcontrol(fd, LPT_STROBE, 1);
      pp_wcontrol(fd, LPT_ACK, 1);
      pp_wcontrol(fd, LPT_RESET, 1);
      pp_wcontrol(fd, LPT_BIT9, 1);

      Sleep(1000);

      printf("\r00000000 0000");
      fflush(stdout);
      pp_wdata(fd, 0);
      pp_wcontrol(fd, LPT_STROBE, 0);
      pp_wcontrol(fd, LPT_ACK, 0);
      pp_wcontrol(fd, LPT_RESET, 0);
      pp_wcontrol(fd, LPT_BIT9, 0);

      Sleep(1000);

   } while (!kbhit());

   while (kbhit())
      getch();

   printf("\n\n\nInput display, hit ENTER to stop.\n");
   printf("Pins 2-9, 10 and 11\n\n");

   do {
      d = pp_rdata(fd);
      for (i = 0; i < 8; i++) {
         printf("%d", (d & 1) > 0);
         d >>= 1;
      }

      printf(" %d%d\r", !pp_rstatus(fd, LPT_DATAREADY), pp_rstatus(fd, LPT_BUSY));
      fflush(stdout);

      Sleep(100);
   } while (!kbhit());

   while (kbhit())
      getch();

   mscb_release(fd);
}

/*------------------------------------------------------------------*/

int mscb_addr(int fd, int cmd, unsigned short adr, int retry, int lock)
/********************************************************************\

  Routine: mscb_addr

  Purpose: Address node or nodes, only used internall from read and
           write routines. A MSCB lock has to be obtained outside
           of this routine!

  Input:
    int fd                  File descriptor for connection
    int cmd                 Addressing mode, one of
                              MCMD_ADDR_BC
                              MCMD_ADDR_NODE8
                              MCMD_ADDR_GRP8
                              MCMD_PING8
                              MCMD_ADDR_NODE16
                              MCMD_ADDR_GRP16
                              MCMD_PING16

    unsigned short adr      Node or group address
    int retry               Number of retries
    int lock                Lock MSCB if TRUE

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving ping acknowledge
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   unsigned char buf[64];
   int i, n, status;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_ADDR, mscb_fd[fd - 1].remote_fd, cmd, adr, retry, lock);

   if (lock) {
      if (mscb_lock(fd) != MSCB_SUCCESS)
         return MSCB_MUTEX;
   }

   for (n = 0; n < retry; n++) {
      buf[0] = (unsigned char)cmd;

      if (cmd == MCMD_ADDR_NODE8 || cmd == MCMD_ADDR_GRP8) {
         buf[1] = (unsigned char) adr;
         buf[2] = crc8(buf, 2);
         status = mscb_out(fd, buf, 3, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO | RS485_FLAG_NO_ACK);
      } else if (cmd == MCMD_PING8) {
         buf[1] = (unsigned char) adr;
         buf[2] = crc8(buf, 2);
         status = mscb_out(fd, buf, 3, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO);
      } else if (cmd == MCMD_ADDR_NODE16 || cmd == MCMD_ADDR_GRP16) {
         buf[1] = (unsigned char) (adr >> 8);
         buf[2] = (unsigned char) (adr & 0xFF);
         buf[3] = crc8(buf, 3);
         status = mscb_out(fd, buf, 4, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO | RS485_FLAG_NO_ACK);
      } else if (cmd == MCMD_PING16) {
         buf[1] = (unsigned char) (adr >> 8);
         buf[2] = (unsigned char) (adr & 0xFF);
         buf[3] = crc8(buf, 3);
         status = mscb_out(fd, buf, 4, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO);
      } else {
         buf[1] = crc8(buf, 1);
         status = mscb_out(fd, buf, 2, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO | RS485_FLAG_NO_ACK);
      }

      if (status != MSCB_SUCCESS) {
         if (lock)
            mscb_release(fd);
         return MSCB_SUBM_ERROR;
      }

      if (cmd == MCMD_PING8 || cmd == MCMD_PING16) {
         /* read back ping reply, 4ms timeout (for USB!) */
         i = mscb_in(fd, buf, 1, TO_SHORT);

         if (i == MSCB_SUCCESS && buf[0] == MCMD_ACK) {
            if (lock)
               mscb_release(fd);
            return MSCB_SUCCESS;
         }

         if (retry > 1) {
            /* send 0's to overflow partially filled node receive buffer */
            memset(buf, 0, sizeof(buf));
            mscb_out(fd, buf, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);

            /* wait some time */
            Sleep(10);
         }

         /* try again.... */
      } else {
         if (lock)
            mscb_release(fd);
         return MSCB_SUCCESS;
      }
   }

   if (lock)
      mscb_release(fd);

   return MSCB_TIMEOUT;
}

/*------------------------------------------------------------------*/

int mscb_reboot(int fd, int addr, int gaddr, int broadcast)
/********************************************************************\

  Routine: mscb_reboot

  Purpose: Reboot node by sending MCMD_INIT

  Input:
    int fd                  File descriptor for connection
    int addr                Node address
    int gaddr               Group address
    int broadcast           Broadcast flag

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_TIMEOUT            Timeout receiving ping acknowledge

\********************************************************************/
{
   unsigned char buf[64];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_REBOOT, mscb_fd[fd - 1].remote_fd, addr, gaddr, broadcast);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   if (addr >= 0) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (addr >> 8);
      buf[2] = (unsigned char) (addr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_INIT;
      buf[5] = crc8(buf+4, 1);
      mscb_out(fd, buf, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (gaddr >= 0) {
      buf[0] = MCMD_ADDR_GRP16;
      buf[1] = (unsigned char) (gaddr >> 8);
      buf[2] = (unsigned char) (gaddr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_INIT;
      buf[5] = crc8(buf+4, 1);
      mscb_out(fd, buf, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (broadcast) {
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_INIT;
      buf[3] = crc8(buf+2, 1);
      mscb_out(fd, buf, 4, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   }

   mscb_release(fd);
   mscb_clear_info_cache(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_reset(int fd)
/********************************************************************\

  Routine: mscb_reset

  Purpose: Reset submaster via hardware reset

  Input:
    int fd                  File descriptor for connection

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   char buf[10];

   debug_log("mscb_reset(fd=%d) ", 1, fd);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_RESET, mscb_fd[fd - 1].remote_fd);

   if (mscb_lock(fd) != MSCB_SUCCESS) {
      debug_log("return MSCB_MUTEX\n", 0);
      return MSCB_MUTEX;
   }

   if (mscb_fd[fd - 1].type == MSCB_TYPE_LPT) {
      /* toggle reset */
      pp_wcontrol(fd, LPT_RESET, 1);
      Sleep(100);               // for elko
      pp_wcontrol(fd, LPT_RESET, 0);
   } else if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {

      buf[0] = MCMD_INIT;
      mscb_out(fd, buf, 1, RS485_FLAG_CMD);

      mrpc_disconnect(fd);
      Sleep(5000);  // let submaster obtain IP address
      mscb_fd[fd - 1].fd = mrpc_connect(mscb_fd[fd - 1].device, MSCB_NET_PORT);
      if (mscb_fd[fd - 1].fd < 0) {
         mscb_release(fd);
         debug_log("return MSCB_SUBM_ERROR (mrpc_connect)\n", 0);
         return MSCB_SUBM_ERROR;
      }

   } else if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {

      buf[0] = MCMD_INIT;
      mscb_out(fd, buf, 1, RS485_FLAG_CMD);
      musb_close(mscb_fd[fd - 1].ui);
      Sleep(1000);
      subm250_open(&mscb_fd[fd - 1].ui, atoi(mscb_fd[fd - 1].device+3));
   }

   /* send 0's to overflow partially filled node receive buffer */
   memset(buf, 0, sizeof(buf));
   mscb_out(fd, buf, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
   Sleep(10);

   mscb_release(fd);

   debug_log("return MSCB_SUCCESS\n", 0);
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_ping(int fd, unsigned short adr)
/********************************************************************\

  Routine: mscb_ping

  Purpose: Ping node to see if it's alive

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address

  Output:
    none

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int status;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_PING, mscb_fd[fd - 1].remote_fd, adr);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   status = mscb_addr(fd, MCMD_PING16, adr, 1, 0);

   mscb_release(fd);
   return status;
}

/*------------------------------------------------------------------*/

void mscb_clear_info_cache(int fd)
/* called internally when a node gets upgraded or address changed */
{
   if (n_cache_info_var)
      free(cache_info_var);
   n_cache_info_var = 0;
}

/*------------------------------------------------------------------*/

int mscb_info(int fd, unsigned short adr, MSCB_INFO * info)
/********************************************************************\

  Routine: mscb_info

  Purpose: Retrieve info on addressd node

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address

  Output:
    MSCB_INFO *info         Info structure defined in mscb.h

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i, retry;
   unsigned char buf[256];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_INFO, mscb_fd[fd - 1].remote_fd, adr, info);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   for (retry = 0 ; retry < 2 ; retry++) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_GET_INFO;
      buf[5] = crc8(buf+4, 1);
      mscb_out(fd, buf, 6, RS485_FLAG_LONG_TO | RS485_FLAG_ADR_CYCLE);

      i = mscb_in(fd, buf, sizeof(buf), TO_SHORT);

      if (i == (int) sizeof(MSCB_INFO) + 3)
         break;
   }

   mscb_release(fd);
   if (i < (int) sizeof(MSCB_INFO) + 3)
      return MSCB_TIMEOUT;

   memcpy(info, buf + 2, sizeof(MSCB_INFO));

   /* do CRC check */
   if (crc8(buf, i-1) != buf[i-1])
      return MSCB_CRC_ERROR;

   WORD_SWAP(&info->node_address);
   WORD_SWAP(&info->group_address);
   WORD_SWAP(&info->watchdog_resets);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_uptime(int fd, unsigned short adr, unsigned int *uptime)
/********************************************************************\

  Routine: mscb_uptime

  Purpose: Retrieve uptime of node in seconds

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address

  Output:
    unsigned long *uptime   Uptime in seconds

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i;
   unsigned char buf[256];
   unsigned long u;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_UPTIME, mscb_fd[fd - 1].remote_fd, adr, uptime);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = MCMD_GET_UPTIME;
   buf[5] = crc8(buf+4, 1);
   mscb_out(fd, buf, 6, RS485_FLAG_ADR_CYCLE);

   i = mscb_in(fd, buf, sizeof(buf), TO_SHORT);
   mscb_release(fd);

   if (i < 6)
      return MSCB_TIMEOUT;

   /* do CRC check */
   if (crc8(buf, i-1) != buf[i-1])
      return MSCB_CRC_ERROR;

   u = (buf[4]<<0) + (buf[3]<<8) + (buf[2]<<16) + (buf[1]<<24);

   *uptime = u;

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_info_variable(int fd, unsigned short adr, unsigned char index, MSCB_INFO_VAR * info)
/********************************************************************\

  Routine: mscb_info_variable

  Purpose: Retrieve info on a specific node variable

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    int index               Variable index 0..255

  Output:
    MSCB_INFO_VAR *info     Info structure defined in mscb.h

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error

    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i, retry;
   unsigned char buf[80];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   /* check if info in cache */
   for (i = 0; i < n_cache_info_var; i++)
      if (cache_info_var[i].fd == fd && cache_info_var[i].adr == adr &&
          cache_info_var[i].index == index)
         break;

   if (i < n_cache_info_var) {
      /* copy from cache */
      if (cache_info_var[i].info.name[0] == 0)
         return MSCB_NO_VAR;

      memcpy(info, &cache_info_var[i].info, sizeof(MSCB_INFO_VAR));
   } else {
      /* add new entry in cache */
      if (n_cache_info_var == 0)
         cache_info_var = (CACHE_INFO_VAR *) malloc(sizeof(CACHE_INFO_VAR));
      else
         cache_info_var = (CACHE_INFO_VAR *) realloc(cache_info_var, sizeof(CACHE_INFO_VAR) * (n_cache_info_var + 1));

      if (cache_info_var == NULL)
         return MSCB_NO_MEM;

      cache_info_var[n_cache_info_var].fd = fd;
      cache_info_var[n_cache_info_var].adr = adr;
      cache_info_var[n_cache_info_var].index = index;

      if (mrpc_connected(fd))
         return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_INFO_VARIABLE,
                        mscb_fd[fd - 1].remote_fd, adr, index, info);

      if (mscb_lock(fd) != MSCB_SUCCESS)
         return MSCB_MUTEX;

      for (retry = 0 ; retry < 2 ; retry++) {
         buf[0] = MCMD_ADDR_NODE16;
         buf[1] = (unsigned char) (adr >> 8);
         buf[2] = (unsigned char) (adr & 0xFF);
         buf[3] = crc8(buf, 3);

         buf[4] = MCMD_GET_INFO + 1;
         buf[5] = index;
         buf[6] = crc8(buf+4, 2);
         mscb_out(fd, buf, 7, RS485_FLAG_ADR_CYCLE);

         i = mscb_in(fd, buf, sizeof(buf), TO_SHORT);

         if (i == (int) sizeof(MSCB_INFO_VAR) + 3 || i == 2)
            break;
      }

      mscb_release(fd);
      if (i < (int) sizeof(MSCB_INFO_VAR) + 3) {
         if (i == 2) // negative acknowledge
            cache_info_var[n_cache_info_var].info.name[0] = 0;
         return MSCB_NO_VAR;
      }

      /* do CRC check */
      if (crc8(buf, sizeof(MSCB_INFO_VAR) + 2) != buf[sizeof(MSCB_INFO_VAR) + 2])
         return MSCB_CRC_ERROR;

      memcpy(info, buf + 2, sizeof(MSCB_INFO_VAR));
      memcpy(&cache_info_var[n_cache_info_var++].info, info, sizeof(MSCB_INFO_VAR));
   }

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_node_addr(int fd, int addr, int gaddr, int broadcast, 
                       unsigned short new_addr)
/********************************************************************\

  Routine: mscb_set_node_addr

  Purpose: Set node address of an node. Only set high byte if addressed
           in group or broadcast mode

  Input:
    int fd                  File descriptor for connection
    int addr                Node address
    int gaddr               Group address
    int broadcast           Broadcast flag
    unsigned short new_addr New node address

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   unsigned char buf[64];
   int status;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_NODE_ADDR, mscb_fd[fd - 1].remote_fd, 
                       addr, gaddr, broadcast, new_addr);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   if (addr >= 0) { // individual node

      /* check if destination address is alive */
      if (addr >= 0) {
         status = mscb_addr(fd, MCMD_PING16, new_addr, 10, 0);
         if (status == MSCB_SUCCESS) {
            mscb_release(fd);
            return MSCB_ADDR_EXISTS;
         }
      }

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (addr >> 8);
      buf[2] = (unsigned char) (addr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_SET_ADDR;
      buf[5] = ADDR_SET_NODE;
      buf[6] = (unsigned char) (new_addr >> 8);
      buf[7] = (unsigned char) (new_addr & 0xFF);
      buf[8] = crc8(buf+4, 4);
      mscb_out(fd, buf, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   } else if (gaddr >= 0) {
      buf[0] = MCMD_ADDR_GRP16;
      buf[1] = (unsigned char) (gaddr >> 8);
      buf[2] = (unsigned char) (gaddr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_SET_ADDR;
      buf[5] = ADDR_SET_HIGH;
      buf[6] = (unsigned char) (new_addr >> 8);
      buf[7] = (unsigned char) (new_addr & 0xFF);
      buf[8] = crc8(buf+4, 4);
      mscb_out(fd, buf, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   } else if (broadcast) {
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_SET_ADDR;
      buf[3] = ADDR_SET_HIGH;
      buf[4] = (unsigned char) (new_addr >> 8);
      buf[5] = (unsigned char) (new_addr & 0xFF);
      buf[6] = crc8(buf+2, 4);
      mscb_out(fd, buf, 7, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   }

   mscb_release(fd);
   mscb_clear_info_cache(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_group_addr(int fd, int addr, int gaddr, int broadcast, unsigned short new_addr)
/********************************************************************\

  Routine: mscb_set_gaddr

  Purpose: Set group address of addressed node(s)

  Input:
    int fd                  File descriptor for connection
    int addr                Node address
    int gaddr               Group address
    int broadcast           Broadcast flag
    unsigned short new_addr New group address

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   unsigned char buf[64];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_GROUP_ADDR, mscb_fd[fd - 1].remote_fd, 
                       gaddr, new_addr);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   if (broadcast > 0) {         
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_SET_ADDR;
      buf[3] = ADDR_SET_GROUP;
      buf[4] = (unsigned char) (new_addr >> 8);
      buf[5] = (unsigned char) (new_addr & 0xFF);
      buf[6] = crc8(buf+2, 4);
      mscb_out(fd, buf, 7, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   } else if (gaddr >= 0) {
      buf[0] = MCMD_ADDR_GRP16;
      buf[1] = (unsigned char) (gaddr >> 8);
      buf[2] = (unsigned char) (gaddr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_SET_ADDR;
      buf[5] = ADDR_SET_GROUP;
      buf[6] = (unsigned char) (new_addr >> 8);
      buf[7] = (unsigned char) (new_addr & 0xFF);
      buf[8] = crc8(buf+4, 4);
      mscb_out(fd, buf, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   } else if (addr >= 0) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (addr >> 8);
      buf[2] = (unsigned char) (addr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_SET_ADDR;
      buf[5] = ADDR_SET_GROUP;
      buf[6] = (unsigned char) (new_addr >> 8);
      buf[7] = (unsigned char) (new_addr & 0xFF);
      buf[8] = crc8(buf+4, 4);
      mscb_out(fd, buf, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   }

   mscb_release(fd);
   mscb_clear_info_cache(fd);

   return MSCB_SUCCESS;
}
/*------------------------------------------------------------------*/

int mscb_set_name(int fd, unsigned short adr, char *name)
/********************************************************************\

  Routine: mscb_set_name

  Purpose: Set node name

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    char *name              New node name, up to 16 characters

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   unsigned char buf[256];
   int i;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_NAME, mscb_fd[fd - 1].remote_fd, adr, name);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = MCMD_SET_NAME;
   for (i = 0; i < 16 && name[i]; i++)
      buf[6 + i] = name[i];

   if (i < 16)
      buf[6 + (i++)] = 0;

   buf[5] = (unsigned char)i; /* varibale buffer length */

   buf[6 + i] = crc8(buf+4, 2 + i);
   mscb_out(fd, buf, 7 + i, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   mscb_release(fd);
   mscb_clear_info_cache(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_group(int fd, unsigned short adr, unsigned char index, void *data, int size)
/********************************************************************\

  Routine: mscb_write_group

  Purpose: Write data to channels on group of nodes

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      group address
    unsigned char index     Variable index 0..255
    unsigned int  data      Data to send
    int size                Data size in bytes 1..4 for byte, word,
                            and dword

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i;
   unsigned char *d;
   unsigned char buf[256];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE_GROUP,
                       mscb_fd[fd - 1].remote_fd, adr, index, data, size);

   if (size > 4 || size < 1)
      return MSCB_INVAL_PARAM;

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   buf[0] = MCMD_ADDR_GRP16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = (unsigned char)(MCMD_WRITE_NA + size + 1);
   buf[5] = index;

   for (i = 0, d = data; i < size; i++)
      buf[6 + size - 1 - i] = *d++;

   buf[6 + i] = crc8(buf, 6 + i);
   mscb_out(fd, buf, 7 + i, RS485_FLAG_ADR_CYCLE | RS485_FLAG_NO_ACK);

   mscb_release(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write(int fd, unsigned short adr, unsigned char index, void *data, int size)
/********************************************************************\

  Routine: mscb_write

  Purpose: Write data to variable on single node

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    unsigned char index     Variable index 0..255
    void *data              Data to send
    int size                Data size in bytes 1..4 for byte, word,
                            and dword

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i, retry, status;
   unsigned char buf[256], crc, ack[2];
   unsigned char *d;
   unsigned int dw;
   float f;

   dw = 0;
   f = 0;
   if (size == 1)
      dw = *((unsigned char *)data);
   else if (size == 2)
      dw = *((unsigned short *)data);
   else if (size == 4) {
      dw = *((unsigned int *)data);
      f = *((float *)data);
   }

   if (size == 4)
      debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=%lf/0x%X,size=%d) ", 1, fd, adr, index, f, dw, size);
   else if (size < 4)
      debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=0x%X,size=%d) ", 1, fd, adr, index, dw, size);
   else
      debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=\"%s\",size=%d) ", 1, fd, adr, index, (char *)data, size);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE, mscb_fd[fd - 1].remote_fd, adr, index, data, size);

   if (size < 1) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mscb_lock(fd) != MSCB_SUCCESS) {
      debug_log("return MSCB_MUTEX\n", 0);
      return MSCB_MUTEX;
   }

   /* try 10 times */
   for (retry=0 ; retry<10 ; retry++) {

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      if (size < 6) {
         buf[4] = (unsigned char)(MCMD_WRITE_ACK + size + 1);
         buf[5] = index;

         /* reverse order for WORD & DWORD */
         if (size < 5)
            for (i = 0, d = data; i < size; i++)
               buf[6 + size - 1 - i] = *d++;
         else
            for (i = 0, d = data; i < size; i++)
               buf[6 + i] = *d++;

         crc = crc8(buf+4, 2 + i);
         buf[6 + i] = crc;
         mscb_out(fd, buf, 7 + i, RS485_FLAG_ADR_CYCLE);
      } else {
         buf[4] = MCMD_WRITE_ACK + 7;
         buf[5] = (unsigned char)(size + 1);
         buf[6] = index;

         for (i = 0, d = data; i < size; i++)
            buf[7 + i] = *d++;

         crc = crc8(buf+4, 3 + i);
         buf[7 + i] = crc;
         mscb_out(fd, buf, 8 + i, RS485_FLAG_ADR_CYCLE);
      }

      /* read acknowledge */
      i = mscb_in(fd, ack, 2, TO_SHORT);
      mscb_release(fd);
      
      if (i == 1) {
#ifndef _USRDLL
         printf("Timeout from RS485 bus\n");
#endif
         debug_log("Timeout from RS485 bus\n", 1);
         status = MSCB_TIMEOUT;

#ifndef _USRDLL
         printf("Flush node communication\n");
#endif
         debug_log("Flush node communication\n", 1);
         memset(buf, 0, sizeof(buf));
         mscb_out(fd, buf, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
         Sleep(100);

         continue;
      }

      if (i < 2) {
         debug_log("return MSCB_TIMEOUT\n", 0);
         status = MSCB_TIMEOUT;
         continue;
      }

      if (ack[0] != MCMD_ACK || ack[1] != crc) {
         debug_log("return MSCB_CRC_ERROR\n", 0);
         status = MSCB_CRC_ERROR;
         continue;
      }

      status = MSCB_SUCCESS;
      debug_log("return MSCB_SUCCESS\n", 0);
      break;
   }

   return status;
}

/*------------------------------------------------------------------*/

int mscb_write_no_retries(int fd, unsigned short adr, unsigned char index, void *data, int size)
/********************************************************************\

  Routine: mscb_write

  Purpose: Same as mscb_write, but without retries

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    unsigned char index     Variable index 0..255
    void *data              Data to send
    int size                Data size in bytes 1..4 for byte, word,
                            and dword

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i;
   unsigned char buf[256], crc, ack[2];
   unsigned char *d;
   unsigned int dw;

   dw = 0;
   if (size == 1)
      dw = *((unsigned char *)data);
   else if (size == 2)
      dw = *((unsigned short *)data);
   else if (size == 4)
      dw = *((unsigned int *)data);

   if (size <= 4)
      debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=0x%X,size=%d) ", 1, fd, adr, index, dw, size);
   else
      debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=\"%s\",size=%d) ", 1, fd, adr, index, (char *)data, size);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE_NO_RETRIES, mscb_fd[fd - 1].remote_fd, adr, index, data, size);

   if (size < 1) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mscb_lock(fd) != MSCB_SUCCESS) {
      debug_log("return MSCB_MUTEX\n", 0);
      return MSCB_MUTEX;
   }

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   if (size < 6) {
      buf[4] = (unsigned char)(MCMD_WRITE_ACK + size + 1);
      buf[5] = index;

      /* reverse order for WORD & DWORD */
      if (size < 5)
         for (i = 0, d = data; i < size; i++)
            buf[6 + size - 1 - i] = *d++;
      else
         for (i = 0, d = data; i < size; i++)
            buf[6 + i] = *d++;

      crc = crc8(buf+4, 2 + i);
      buf[6 + i] = crc;
      mscb_out(fd, buf, 7 + i, RS485_FLAG_ADR_CYCLE);
   } else {
      buf[4] = MCMD_WRITE_ACK + 7;
      buf[5] = (unsigned char)(size + 1);
      buf[6] = index;

      for (i = 0, d = data; i < size; i++)
         buf[7 + i] = *d++;

      crc = crc8(buf+4, 3 + i);
      buf[7 + i] = crc;
      mscb_out(fd, buf, 8 + i, RS485_FLAG_ADR_CYCLE);
   }

   /* read acknowledge */
   i = mscb_in(fd, ack, 2, TO_SHORT);
   mscb_release(fd);
   if (i < 2) {
      debug_log("return MSCB_TIMEOUT\n", 0);
      return MSCB_TIMEOUT;
   }

   if (ack[0] != MCMD_ACK || ack[1] != crc) {
      debug_log("return MSCB_CRC_ERROR\n", 0);
      return MSCB_CRC_ERROR;
   }

   debug_log("return MSCB_SUCCESS\n", 0);
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_block(int fd, unsigned short adr, unsigned char index, void *data, int size)
/********************************************************************\

  Routine: mscb_write_block

  Purpose: Write block of data to single node

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    unsigned char index     Variable index 0..255
    void *data              Data to send
    int size                Data size in bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i, n;
   unsigned char buf[256], crc, ack[2];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE_BLOCK, mscb_fd[fd - 1].remote_fd, adr, index, data, size);

   if (size < 1)
      return MSCB_INVAL_PARAM;

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   /*
   Bulk test gave 128kb/sec, 10.3.04, SR

   for (i=0 ; i<1024 ; i++)
      buf[i] = RS485_FLAG_CMD;

   i = msend_usb(mscb_fd[fd - 1].hw, buf, 1024);
   mscb_release(fd);
   return MSCB_SUCCESS;
   */

   /* slice data in 58 byte blocks */
   for (i=0 ; i<=size/58 ; i++) {
      n = 58;
      if (size < (i+1)*58)
         n = size - i*58;

      if (n == 0)
         break;

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_WRITE_ACK + 7;
      buf[5] = (unsigned char)(n + 1);
      buf[6] = index;

      memcpy(buf+7, (char *)data+i*58, n);
      crc = crc8(buf+4, 3 + n);
      buf[7 + n] = crc;

      mscb_out(fd, buf, n+8, RS485_FLAG_ADR_CYCLE);

      /* read acknowledge */
      n = mscb_in(fd, ack, 2, TO_SHORT);
      if (n < 2) {
         mscb_release(fd);
         return MSCB_TIMEOUT;
      }

      if (ack[0] != MCMD_ACK || ack[1] != crc) {
         mscb_release(fd);
         return MSCB_CRC_ERROR;
      }
   }

   mscb_release(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_flash(int fd, int addr, int gaddr, int broadcast)
/********************************************************************\

  Routine: mscb_flash

  Purpose: Flash node variables to EEPROM


  Input:
    int fd                  File descriptor for connection
    int addr                Node address
    int gaddr               Group address
    int broadcast           Broadcast flag

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   unsigned char buf[64];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_FLASH, mscb_fd[fd - 1].remote_fd, addr, gaddr, broadcast);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   if (addr >= 0) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (addr >> 8);
      buf[2] = (unsigned char) (addr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_FLASH;
      buf[5] = crc8(buf+4, 1);
      mscb_out(fd, buf, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (gaddr >= 0) {
      buf[0] = MCMD_ADDR_GRP16;
      buf[1] = (unsigned char) (gaddr >> 8);
      buf[2] = (unsigned char) (gaddr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_FLASH;
      buf[5] = crc8(buf+4, 1);
      mscb_out(fd, buf, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (broadcast) {
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_FLASH;
      buf[3] = crc8(buf+2, 1);
      mscb_out(fd, buf, 4, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   }

   mscb_release(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_upload(int fd, unsigned short adr, char *buffer, int size, int debug)
/********************************************************************\

  Routine: mscb_upload

  Purpose: Upload new firmware to node


  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    char *buffer            Buffer with Intel HEX file
    int size                Size of buffer
    int debug               If TRUE, produce detailed debugging output

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_FORMAT_ERROR       Error in HEX file format

\********************************************************************/
{
   unsigned char buf[512], crc, ack[2], image[0x10000], *line;
   unsigned int len, ofh, ofl, type, d;
   int i, status, page, subpage, flash_size, n_page, retry, sretry, protected_page,
      n_page_disp;
   unsigned short ofs;
   int page_cont[128];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_UPLOAD, mscb_fd[fd - 1].remote_fd, adr, buffer, size);

   /* check if node alive */
   status = mscb_ping(fd, adr);
   if (status != MSCB_SUCCESS) {
      printf("Error: cannot ping node #%d\n", adr);
      return status;
   }

   /* interprete HEX file */
   memset(image, 0xFF, sizeof(image));
   line = buffer;
   flash_size = 0;
   do {
      if (line[0] == ':') {
         sscanf(line + 1, "%02x%02x%02x%02x", &len, &ofh, &ofl, &type);
         ofs = (unsigned short)((ofh << 8) | ofl);

         for (i = 0; i < (int) len; i++) {
            sscanf(line + 9 + i * 2, "%02x", &d);
            image[ofs + i] = (unsigned char)d;
         }

         flash_size += len;
         line = strchr(line, '\n');
         if (line == NULL)
            break;

         /* skip CR/LF */
         while (*line == '\r' || *line == '\n')
            line++;
      } else
         return MSCB_FORMAT_ERROR;

   } while (*line);

   /* count pages and bytes */
   for (page = 0; page < sizeof(page_cont)/sizeof(int) ; page++)
      page_cont[page] = FALSE;

   for (page = n_page = 0; page < sizeof(page_cont)/sizeof(int) ; page++) {
      /* check if page contains data */
      for (i = 0; i < 512; i++) {
         if (image[page * 512 + i] != 0xFF) {
            page_cont[page] = TRUE;
            n_page++;
            break;
         }
      }
   }

   /* reduce number of "="'s if too many */
   n_page_disp = (n_page / 70) + 1;

   protected_page = 128;

   if (debug)
      printf("Found %d valid pages (%d bytes) in HEX file\n", n_page, flash_size);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   /* enter exclusive mode */
   buf[0] = MCMD_FREEZE;
   buf[1] = 1;
   mscb_out(fd, buf, 2, RS485_FLAG_CMD);

   /* wait for acknowledge */
   if (mscb_in(fd, buf, 1, TO_SHORT) != 1) {
      printf("Error: cannot request exlusive access to submaster\n");
      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   /* address node */
   status = mscb_addr(fd, MCMD_PING16, adr, 10, 0);
   if (status != MSCB_SUCCESS) {
      mscb_release(fd);
      return status;
   }

   /* send upgrade command */
   buf[0] = MCMD_UPGRADE;
   crc = crc8(buf, 1);
   buf[1] = crc;
   mscb_out(fd, buf, 2, RS485_FLAG_LONG_TO);

   /* wait for acknowledge */
   if (mscb_in(fd, buf, 3, TO_LONG) != 3) {
      printf("Error: timeout receiving acknowledge from remote node\n");
      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   if (buf[1] == 2) {
      mscb_release(fd);
      return MSCB_SUBADDR;
   }

   if (buf[1] == 3) {
      mscb_release(fd);
      return MSCB_NOTREADY;
   }

   /* let main routine enter upgrade() */
   Sleep(500);

   /* send echo command */
   buf[0] = UCMD_ECHO;
   mscb_out(fd, buf, 1, RS485_FLAG_LONG_TO);

   /* wait for ready */
   if (mscb_in(fd, ack, 2, TO_LONG) != 2) {
      printf("Error: timeout receiving upgrade echo test from remote node\n");

      /* send exit upgrade command, in case node gets to upgrade routine later */
      buf[0] = UCMD_RETURN;
      mscb_out(fd, buf, 1, RS485_FLAG_NO_ACK);

      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   if (debug)
      printf("Received acknowledge for upgrade command\n");

   if (!debug) {
      printf("\n[");
      for (i=0 ; i<n_page/n_page_disp+1 ; i++)
         printf(" ");
      printf("]\r[");
   }

   /* erase pages up to 64k */
   for (page = 0; page < 128; page++) {

      /* check if page contains data */
      if (!page_cont[page])
         continue;

      for (retry = 0 ; retry < 10 ; retry++) {

         if (debug)
            printf("Erase page   0x%04X - ", page * 512);
         fflush(stdout);

         /* erase page */
         buf[0] = UCMD_ERASE;
         buf[1] = (unsigned char)page;
         mscb_out(fd, buf, 2, RS485_FLAG_LONG_TO);

         if (mscb_in(fd, ack, 2, TO_LONG) != 2) {
            printf("\nError: timeout from remote node for erase page 0x%04X\n", page * 512);
            continue;
         }

         if (ack[1] == 0xFF) {
            /* protected page, so finish */
            if (debug)
               printf("found protected page, exit\n");
            else
               printf("]      ");
            fflush(stdout);
            protected_page = page;
            goto prog_pages;
         }

         if (ack[0] != MCMD_ACK) {
            printf("\nError: received wrong acknowledge for erase page 0x%04X\n", page * 512);
            continue;
         }

         if (debug)
            printf("ok\n");

         break;
      }

      /* check retries */
      if (retry == 10) {
         printf("\nToo many retries, aborting\n");
         goto prog_error;
         break;
      }

      if (!debug)
         if (page % n_page_disp == 0)
            printf("-");
      fflush(stdout);
   }

prog_pages:

   if (!debug)
      printf("\r[");

   /* program pages up to 64k */
   for (page = 0; page < protected_page; page++) {

      /* check if page contains data */
      if (!page_cont[page])
         continue;

      /* build CRC of page */
      for (i = crc = 0; i < 512; i++)
         crc += image[page * 512 + i];

      for (retry = 0 ; retry < 10 ; retry++) {

         if (debug)
            printf("Program page 0x%04X - ", page * 512);
         fflush(stdout);

         /* chop down page in 32 byte segments (->USB) */
         for (subpage = 0; subpage < 16; subpage++) {

            for (sretry = 0 ; sretry < 10 ; sretry++) {
               /* program page */
               buf[0] = UCMD_PROGRAM;
               buf[1] = (unsigned char)page;
               buf[2] = (unsigned char)subpage;

               /* extract page from image */
               for (i = 0; i < 32; i++)
                  buf[i+3] = image[page * 512 + subpage * 32 + i];

               mscb_out(fd, buf, 32+3, RS485_FLAG_LONG_TO);

               /* read acknowledge */
               ack[0] = 0;
               if (mscb_in(fd, ack, 2, TO_LONG) != 2 || ack[0] != MCMD_ACK) {
                  printf("\nError: timeout from remote node for program page 0x%04X, chunk %d\n", page * 512, i);
                  goto prog_error;
               } else
                  break; // successful
            }

            /* check retries */
            if (sretry == 10) {
               printf("\nToo many retries, aborting\n");
               goto prog_error;
               break;
            }
         }

         /* test if successful completion */
         if (ack[0] != MCMD_ACK)
            continue;

         if (debug)
            printf("ok\n");

         /* verify page */
         if (debug)
            printf("Verify page  0x%04X - ", page * 512);
         fflush(stdout);

         /* verify page */
         buf[0] = UCMD_VERIFY;
         buf[1] = (unsigned char)page;
         mscb_out(fd, buf, 2, RS485_FLAG_LONG_TO);

         ack[1] = 0;
         if (mscb_in(fd, ack, 2, TO_LONG) != 2) {
            printf("\nError: timeout from remote node for verify page 0x%04X\n", page * 512);
            goto prog_error;
         }

         /* compare CRCs */
         if (ack[1] == crc) {
            if (debug)
               printf("ok (CRC = 0x%02X)\n", crc);
            break;
         }

         if (debug)
            printf("CRC error (0x%02X != 0x%02X)\n", crc, ack[1]);

      }

      /* check retries */
      if (retry == 10) {
         printf("\nToo many retries, aborting\n");
         goto prog_error;
         break;
      }

      if (!debug)
         if (page % n_page_disp == 0)
            printf("=");
      fflush(stdout);
   }

   printf("\n");

   /* reboot node */
   buf[0] = UCMD_REBOOT;
   mscb_out(fd, buf, 1, RS485_FLAG_NO_ACK);

   if (debug)
      printf("Reboot node\n");

   mscb_clear_info_cache(fd);

prog_error:

   /* exit exclusive mode */
   buf[0] = MCMD_FREEZE;
   buf[1] = 0;
   mscb_out(fd, buf, 2, RS485_FLAG_CMD);

   /* wait for acknowledge */
   if (mscb_in(fd, buf, 1, TO_SHORT) != 1)
      printf("Error: cannot exit exlusive access at submaster\n");

   mscb_release(fd);
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_verify(int fd, unsigned short adr, char *buffer, int size)
/********************************************************************\

  Routine: mscb_verify

  Purpose: Compare remote firmware with buffer contents


  Input:
    int fd                  File descriptor for connection
    unsigend short adr      Node address
    char *buffer            Buffer with Intel HEX file
    int size                Size of buffer

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_FORMAT_ERROR       Error in HEX file format

\********************************************************************/
{
   unsigned char buf[512], crc, ack[2], image[0x10000], *line;
   unsigned int len, ofh, ofl, type, d;
   int i, j, n_error, status, page, subpage, flash_size;
   unsigned short ofs;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_UPLOAD, mscb_fd[fd - 1].remote_fd, adr, buffer, size);

   /* interprete HEX file */
   memset(image, 0xFF, sizeof(image));
   line = buffer;
   flash_size = 0;
   do {
      if (line[0] == ':') {
         sscanf(line + 1, "%02x%02x%02x%02x", &len, &ofh, &ofl, &type);
         ofs = (unsigned short)((ofh << 8) | ofl);

         for (i = 0; i < (int) len; i++) {
            sscanf(line + 9 + i * 2, "%02x", &d);
            image[ofs + i] = (unsigned char)d;
         }

         flash_size += len;
         line = strchr(line, '\r') + 1;
         if (line && *line == '\n')
            line++;
      } else
         return MSCB_FORMAT_ERROR;

   } while (*line);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   status = mscb_addr(fd, MCMD_PING16, adr, 10, 0);
   if (status != MSCB_SUCCESS) {
      mscb_release(fd);
      return status;
   }

   /* send upgrade command */
   buf[0] = MCMD_UPGRADE;
   crc = crc8(buf, 1);
   buf[1] = crc;
   mscb_out(fd, buf, 2, RS485_FLAG_NO_ACK);

   /* let main routine enter upgrade() */
   Sleep(500);

   /* send echo command */
   buf[0] = UCMD_ECHO;
   mscb_out(fd, buf, 1, RS485_FLAG_LONG_TO);

   /* wait for ready, 1 sec timeout */
   if (mscb_in(fd, ack, 2, TO_LONG) != 2) {
      printf("Error: timeout receiving upgrade acknowledge from remote node\n");

      /* send exit upgrade command, in case node gets to upgrade routine later */
      buf[0] = UCMD_RETURN;
      mscb_out(fd, buf, 1, RS485_FLAG_NO_ACK);

      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   /* compare pages up to 64k */
   for (page = 0; page < 128; page++) {

      /* check if page contains data */
      for (i = 0; i < 512; i++)
         if (image[page * 512 + i] != 0xFF)
            break;
      if (i == 512)
         continue;

      /* verify page */
      printf("Verify page  0x%04X - ", page * 512);
      fflush(stdout);
      n_error = 0;

      /* compare page in 32-byte blocks */
      for (subpage=0 ; subpage<16 ; subpage++) {

         /* build CRC of page */
         for (i = crc = 0; i < 512; i++)
            crc += image[page * 512 + i];

         /* read page */
         buf[0] = UCMD_READ;
         buf[1] = (unsigned char)page;
         buf[2] = (unsigned char)subpage;
         mscb_out(fd, buf, 3, RS485_FLAG_LONG_TO);

         memset(buf, 0, sizeof(buf));
         status = mscb_in(fd, buf, 32+3, TO_LONG);
         if (status != 32+3) {
            printf("\nError: timeout from remote node for verify page 0x%04X\n", page * 512);
            goto ver_error;
          }

         /* compare data */
         for (j=0 ; j<32 ; j++) {
            if (buf[j+2] != image[page*512+subpage*32+j]) {
               n_error++;
               printf("\nError at 0x%04X: file=0x%02X != remote=0x%02X", page*512+subpage*32+j,
                  image[page*512+subpage*32+j], buf[j]);
            }
         }
      }

      if (n_error == 0)
         printf("OK\n");
      else
         printf("\n - %d errors\n", n_error);
   }

ver_error:

   /* send exit code */
   buf[0] = UCMD_RETURN;
   mscb_out(fd, buf, 1, RS485_FLAG_NO_ACK);

   printf("Verify finished\n");

   mscb_release(fd);
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read(int fd, unsigned short adr, unsigned char index, void *data, int *size)
/********************************************************************\

  Routine: mscb_read

  Purpose: Read data from variable on node

  Input:
    int fd                  File descriptor for connection
    unsigend short adr                 Node address
    unsigned char index     Variable index 0..255
    int size                Buffer size for data

  Output:
    void *data              Received data
    int  *size              Number of received bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_INVALID_INDEX      index parameter too large

\********************************************************************/
{
   int i, j, n, status;
   unsigned char buf[256], str[1000], crc;

   debug_log("mscb_read(fd=%d,adr=%d,index=%d,size=%d) ", 1, fd, adr, index, *size);

   if (*size > 256)
      return MSCB_INVAL_PARAM;

   memset(data, 0, *size);
   status = 0;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_READ, mscb_fd[fd - 1].remote_fd, adr, index, data, size);

   if (mscb_lock(fd) != MSCB_SUCCESS) {
      debug_log("return MSCB_MUTEX\n", 0);
      return MSCB_MUTEX;
   }

   /* try five times */
   for (n = i = 0; n < 5 ; n++) {

      /* after three times, reset submaster */
      if (n == 3) {
#ifndef _USRDLL
         printf("Automatic submaster reset\n");
#endif
         debug_log("Automatic submaster reset\n", 1);
         mscb_release(fd);
         status = mscb_reset(fd);
         if (status == MSCB_SUBM_ERROR) {
            sprintf(str, "Cannot reconnect to submaster %s\n", mscb_fd[fd - 1].device);
#ifndef _USRDLL
            printf(str);
#endif
            debug_log(str, 1);
            mscb_lock(fd);
            break;
         }
         mscb_lock(fd);
         Sleep(100);
      }

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_READ + 1;
      buf[5] = index;
      buf[6] = crc8(buf+4, 2);
      status = mscb_out(fd, buf, 7, RS485_FLAG_ADR_CYCLE);
      if (status == MSCB_TIMEOUT) {
#ifndef _USRDLL
         /* show error, but repeat 10 times */
         printf("Timeout writing to sumbster\n");
#endif
         debug_log("Timeout writing to sumbster\n", 1);
         continue;
      }
      if (status == MSCB_SUBM_ERROR) {
#ifndef _USRDLL
         /* show error, but repeat 10 times */
         printf("Connection to submaster broken\n");
#endif
         debug_log("Connection to submaster broken\n", 1);
         break;
      }

      /* read data */
      i = mscb_in(fd, buf, sizeof(buf), TO_SHORT);

      if (i == 1 && buf[0] == MCMD_ACK) {
         /* variable has been deleted on node, so refresh cache */ 
         mscb_release(fd);
         mscb_clear_info_cache(fd);
         return MSCB_INVALID_INDEX;
      }

      if (i == 1) {
#ifndef _USRDLL
         printf("Timeout from RS485 bus\n");
#endif
         debug_log("Timeout from RS485 bus\n", 1);
         status = MSCB_TIMEOUT;

#ifndef _USRDLL
         printf("Flush node communication\n");
#endif
         debug_log("Flush node communication\n", 1);
         memset(buf, 0, sizeof(buf));
         mscb_out(fd, buf, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
         Sleep(100);

         continue;
      }

      if (i < 2) {
#ifndef _USRDLL
         /* show error, but repeat request */
         printf("Timeout reading from submaster\n");
#endif
         status = MSCB_TIMEOUT;
         continue;
      }

      crc = crc8(buf, i - 1);

      if ((buf[0] != MCMD_ACK + i - 2 && buf[0] != MCMD_ACK + 7)
         || buf[i - 1] != crc) {
         status = MSCB_CRC_ERROR;
#ifndef _USRDLL
         /* show error, but repeat 10 times */
         printf("CRC error on RS485 bus\n");
#endif
         continue;
      }

      if (buf[0] == MCMD_ACK + 7) {
         if (i - 3 > *size) {
            mscb_release(fd);
            *size = 0;
            debug_log("return MSCB_NO_MEM, i=%d, *size=%d\n", 0, i, *size);
            return MSCB_NO_MEM;
         }

         memcpy(data, buf + 2, i - 3);  // variable length
         *size = i - 3;
      } else {
         if (i - 2 > *size) {
            mscb_release(fd);
            *size = 0;
            debug_log("return MSCB_NO_MEM, i=%d, *size=%d\n", 0, i, *size);
            return MSCB_NO_MEM;
         }

         memcpy(data, buf + 1, i - 2);
         *size = i - 2;
      }

      if (i - 2 == 2)
         WORD_SWAP(data);
      if (i - 2 == 4)
         DWORD_SWAP(data);

      mscb_release(fd);
      sprintf(str, "return %d bytes: ", *size);
      for (j=0 ; j<*size ; j++) {
         sprintf(str+strlen(str), "0x%02X ", 
            *(((unsigned char *)data)+j));
         if (isalnum(*(((unsigned char *)data)+j)))
            sprintf(str+strlen(str), "('%c') ", 
               *(((unsigned char *)data)+j));
      }
      strlcat(str, "\n", sizeof(str)); 
      debug_log(str, 0);

      return MSCB_SUCCESS;
   }

   mscb_release(fd);

   debug_log("\n", 0);
   if (status == MSCB_TIMEOUT)
     debug_log("return MSCB_TIMEOUT\n", 1);
   if (status == MSCB_CRC_ERROR)
      debug_log("return MSCB_CRC_ERROR\n", 1);
   if (status == MSCB_SUBM_ERROR)
      debug_log("return MSCB_SUBM_ERROR\n", 1);

   return status;
}

/*------------------------------------------------------------------*/

int mscb_read_no_retries(int fd, unsigned short adr, unsigned char index, void *data, int *size)
/********************************************************************\

  Routine: mscb_read_no_retries

  Purpose: Same as mscb_read, but without retries

  Input:
    int fd                  File descriptor for connection
    unsigend short adr                 Node address
    unsigned char index     Variable index 0..255
    int size                Buffer size for data

  Output:
    void *data              Received data
    int  *size              Number of received bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i, j, status;
   unsigned char buf[256], str[1000], crc;

   debug_log("mscb_read_no_retries(fd=%d,adr=%d,index=%d,size=%d) ", 1, fd, adr, index, *size);

   if (*size > 256)
      return MSCB_INVAL_PARAM;

   memset(data, 0, *size);
   status = 0;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_READ_NO_RETRIES, mscb_fd[fd - 1].remote_fd, adr, index, data, size);

   if (mscb_lock(fd) != MSCB_SUCCESS) {
      debug_log("return MSCB_MUTEX\n", 0);
      return MSCB_MUTEX;
   }

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = MCMD_READ + 1;
   buf[5] = index;
   buf[6] = crc8(buf+4, 2);
   status = mscb_out(fd, buf, 7, RS485_FLAG_ADR_CYCLE);
   if (status == MSCB_TIMEOUT) {
#ifndef _USRDLL
      /* show error, but repeat 10 times */
      printf("Timeout writing to sumbster\n");
#endif
      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   /* read data */
   i = mscb_in(fd, buf, sizeof(buf), TO_SHORT);

   if (i == 1) {
#ifndef _USRDLL
      /* show error, but repeat request */
      printf("Timeout from RS485 bus\n");
#endif
      memset(buf, 0, sizeof(buf));
      mscb_out(fd, buf, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   if (i < 2) {
#ifndef _USRDLL
      /* show error, but repeat request */
      printf("Timeout reading from submaster\n");
#endif
      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   crc = crc8(buf, i - 1);

   if ((buf[0] != MCMD_ACK + i - 2 && buf[0] != MCMD_ACK + 7)
      || buf[i - 1] != crc) {
#ifndef _USRDLL
      /* show error, but repeat 10 times */
      printf("CRC error on RS485 bus\n");
#endif
      mscb_release(fd);
      return MSCB_CRC_ERROR;
   }

   if (buf[0] == MCMD_ACK + 7) {
      if (i - 3 > *size) {
         mscb_release(fd);
         *size = 0;
         debug_log("return MSCB_NO_MEM, i=%d, *size=%d\n", 0, i, *size);
         return MSCB_NO_MEM;
      }

      memcpy(data, buf + 2, i - 3);  // variable length
      *size = i - 3;
   } else {
      if (i - 2 > *size) {
         mscb_release(fd);
         *size = 0;
         debug_log("return MSCB_NO_MEM, i=%d, *size=%d\n", 0, i, *size);
         return MSCB_NO_MEM;
      }

      memcpy(data, buf + 1, i - 2);
      *size = i - 2;
   }

   if (i - 2 == 2)
      WORD_SWAP(data);
   if (i - 2 == 4)
      DWORD_SWAP(data);

   mscb_release(fd);
   sprintf(str, "return %d bytes: ", *size);
   for (j=0 ; j<*size ; j++) {
      sprintf(str+strlen(str), "0x%02X ", 
         *(((unsigned char *)data)+j));
      if (isalnum(*(((unsigned char *)data)+j)))
         sprintf(str+strlen(str), "('%c') ", 
            *(((unsigned char *)data)+j));
   }
   strlcat(str, "\n", sizeof(str)); 
   debug_log(str, 0);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read_range(int fd, unsigned short adr, unsigned char index1, unsigned char index2, void *data, int *size)
/********************************************************************\

  Routine: mscb_read

  Purpose: Read data from channel on node

  Input:
    int fd                  File descriptor for connection
    unsigend short adr      Node address
    unsigned char index1    First index to read
    unsigned char index2    Last index to read
    int size                Buffer size for data

  Output:
    void *data              Received data
    int  *size              Number of received bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i, n;
   unsigned char buf[256], crc;

   memset(data, 0, *size);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_READ_RANGE,
                       mscb_fd[fd - 1].remote_fd, adr, index1, index2, data, size);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   /* try ten times */
   for (n = i = 0; n < 10; n++) {
      /* after five times, reset submaster */
      if (n == 5) {
#ifndef _USRDLL
         printf("Automatic submaster reset.\n");
#endif

         mscb_release(fd);
         mscb_reset(fd);
         mscb_lock(fd);
         Sleep(100);
      }

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_READ + 2;
      buf[5] = index1;
      buf[6] = index2;
      buf[7] = crc8(buf+4, 3);
      mscb_out(fd, buf, 8, RS485_FLAG_ADR_CYCLE);

      /* read data */
      i = mscb_in(fd, buf, 256, TO_SHORT);

      if (i < 2)
         continue;

      crc = crc8(buf, i - 1);

      if (buf[0] != MCMD_ACK + 7 || buf[i - 1] != crc)
         continue;

      memcpy(data, buf + 2, i - 3);
      *size = i - 3;

      mscb_release(fd);
      return MSCB_SUCCESS;
   }

   mscb_release(fd);

   if (i < 2)
      return MSCB_TIMEOUT;

   return MSCB_CRC_ERROR;
}

/*------------------------------------------------------------------*/

int mscb_read_block(int fd, unsigned short adr, unsigned char index, void *data, int *size)
/********************************************************************\

  Routine: mscb_read_block

  Purpose: Read data from variable on node

  Input:
    int fd                  File descriptor for connection
    unsigend short adr      Node address
    unsigned char index     Variable index 0..255
    int size                Buffer size for data

  Output:
    void *data              Received data
    int  *size              Number of received bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
   int i, n, error_count;
   unsigned char buf[256], crc;

   debug_log("mscb_read_block(fd=%d,adr=%d,index=%d,size=%d) ", 1, fd, adr, index, *size);

   if (*size > 256)
      return MSCB_INVAL_PARAM;

   memset(data, 0, *size);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_READ_BLOCK, mscb_fd[fd - 1].remote_fd, adr, index, data, size);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   n = error_count = 0;
   do {

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_READ + 1;
      buf[5] = index;
      buf[6] = crc8(buf+4, 2);
      mscb_out(fd, buf, 7, RS485_FLAG_ADR_CYCLE);

      /* read data */
      i = mscb_in(fd, buf, sizeof(buf), TO_SHORT);

      if (i == 1) {
#ifndef _USRDLL
         printf("Timeout from RS485 bus.\n");
#endif
         memset(buf, 0, sizeof(buf));
         mscb_out(fd, buf, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
         mscb_release(fd);
         *size = 0;
         return MSCB_TIMEOUT;
      }
      
      if (i<0) {
         error_count++;
         if (error_count > 10) { // too many errors
#ifndef _USRDLL
            printf("Too many errors reading from RS485 bus.\n");
#endif
            mscb_release(fd);
            *size = 0;
            return MSCB_TIMEOUT;
         }
         
         continue; // try again
      }

      crc = crc8(buf, i - 1);

      if ((buf[0] != MCMD_ACK + i - 2 && buf[0] != MCMD_ACK + 7)
         || buf[i - 1] != crc) {
         mscb_release(fd);
         *size = 0;
         return MSCB_CRC_ERROR;
      }

      if (buf[0] == MCMD_ACK + 7) {
         if (n + i - 3 > *size) {
            mscb_release(fd);
            *size = 0;
            return MSCB_NO_MEM;
         }

         /* end of data on empty buffer */
         if (i == 3)
            break;

         memcpy((char *)data+n, buf + 2, i - 3);  // variable length
         n += i - 3;
      } else {
         if (n + i - 2 > *size) {
            mscb_release(fd);
            *size = 0;
            return MSCB_NO_MEM;
         }

         /* end of data on empty buffer */
         if (i == 2)
            break;

         memcpy((char *)data+n, buf + 1, i - 2);
         n += i - 2;
      }
   } while (TRUE);

   mscb_release(fd);
   *size = n;

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_user(int fd, unsigned short adr, void *param, int size, void *result, int *rsize)
/********************************************************************\

  Routine: mscb_user

  Purpose: Call user function on node

  Input:
    int  fd                 File descriptor for connection
    unsigned short adr      Node address
    char *param             Parameters passed to user function, no CRC code
    int  size               Size of parameters in bytes
    int  *rsize             Size of result buffer

  Output:
    char *result            Optional return parameters
    int  *rsize             Number of returned size

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_FORMAT_ERROR       "size" parameter too large

\********************************************************************/
{
   int i, n, status;
   unsigned char buf[80];

   memset(result, 0, *rsize);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (size > 4 || size < 0) {
      return MSCB_FORMAT_ERROR;
   }

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_USER,
                       mscb_fd[fd - 1].remote_fd, adr, param, size, result, rsize);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = (unsigned char)(MCMD_USER + size);

   for (i = 0; i < size; i++)
      buf[5 + i] = ((char *) param)[i];

   /* add CRC code and send data */
   buf[5 + i] = crc8(buf+4, 1 + i);
   status = mscb_out(fd, buf, 6 + i, RS485_FLAG_ADR_CYCLE);
   if (status != MSCB_SUCCESS) {
      mscb_release(fd);
      return status;
   }

   if (result == NULL) {
      mscb_release(fd);
      return MSCB_SUCCESS;
   }

   /* read result */
   n = mscb_in(fd, buf, sizeof(buf), TO_SHORT);
   mscb_release(fd);

   if (n < 2)
      return MSCB_TIMEOUT;

   if (rsize)
      *rsize = n - 2;
   for (i = 0; i < n - 2; i++)
      ((char *) result)[i] = buf[1 + i];

   if (buf[n - 1] != crc8(buf, n - 1))
      return MSCB_CRC_ERROR;

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_echo(int fd, unsigned short adr, unsigned char d1, unsigned char *d2)
/********************************************************************\

  Routine: mscb_echo

  Purpose: Send byte and receive echo, useful for testing

  Input:
    int  fd                 File descriptor for connection
    int  adr                Node address
    unsigned char d1        Byte to send

  Output:
    unsigned char *d2       Received byte

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_FORMAT_ERROR       "size" parameter too large

\********************************************************************/
{
   int n, status;
   unsigned char buf[64];

   *d2 = 0xFF;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_ECHO, mscb_fd[fd - 1].remote_fd, adr, d1, d2);

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_MUTEX;

   if (adr) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_ECHO;
      buf[5] = d1;

      /* add CRC code and send data */
      buf[6] = crc8(buf+4, 2);
      status = mscb_out(fd, buf, 7, RS485_FLAG_ADR_CYCLE);
      if (status != MSCB_SUCCESS) {
         mscb_release(fd);
         return status;
      }
   } else {

      buf[0] = MCMD_ECHO;
      buf[1] = d1;

      /* add CRC code and send data */
      buf[2] = crc8(buf, 2);
      status = mscb_out(fd, buf, 3, 0);
      if (status != MSCB_SUCCESS) {
         mscb_release(fd);
         return status;
      }
   }

   /* read result */
   n = mscb_in(fd, buf, sizeof(buf), TO_SHORT);
   mscb_release(fd);

   if (n < 0)
      return MSCB_TIMEOUT;

   *d2 = buf[1];

   if (buf[n - 1] != crc8(buf, n - 1))
      return MSCB_CRC_ERROR;

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

unsigned long millitime()
{
#ifdef _MSC_VER
   return (int) GetTickCount();
#else
   {
      struct timeval tv;

      gettimeofday(&tv, NULL);

      return tv.tv_sec * 1000 + tv.tv_usec / 1000;
   }
#endif
}

/*------------------------------------------------------------------*/

int mscb_link(int fd, unsigned short adr, unsigned char index, void *data, int size)
/********************************************************************\

  Routine: mscb_link

  Purpose: Used by LabView to link controls to MSCB variables

  Input:
    int  fd                 File descriptor for connection
    int  adr                Node address
    unsigned char index     Variable index
    void *data              Pointer to data
    int size                Size of data

  Output:
    void *data              Readback data

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_FORMAT_ERROR       "size" parameter too large
    MSCB_NO_MEM             Out of memory

\********************************************************************/
{
   int i, s, status;
   MSCB_INFO_VAR info;

   debug_log("mscb_link( %d %d %d * %d)\n", 1, fd, adr, index, size);

   /* check if variable in cache */
   for (i = 0; i < n_cache; i++)
      if (cache[i].fd == fd && cache[i].adr == adr && cache[i].index == index)
         break;

   if (i < n_cache) {
      if (memcmp(data, cache[i].data, cache[i].size) != 0) {
         /* data has changed, send update */
         memcpy(cache[i].data, data, cache[i].size);
         mscb_write(fd, adr, index, data, cache[i].size);
      } else {
         /* retrieve data from node */
         if (millitime() > cache[i].last + CACHE_PERIOD) {
            cache[i].last = millitime();

            s = cache[i].size;
            status = mscb_read(fd, adr, index, cache[i].data, &s);
            if (status != MSCB_SUCCESS)
               return status;

            memcpy(data, cache[i].data, size);
         }
      }
   } else {
      /* add new entry in cache */
      if (n_cache == 0)
         cache = (CACHE_ENTRY *) malloc(sizeof(CACHE_ENTRY));
      else
         cache = (CACHE_ENTRY *) realloc(cache, sizeof(CACHE_ENTRY) * (n_cache + 1));

      if (cache == NULL)
         return MSCB_NO_MEM;

      /* get variable size from node */
      status = mscb_info_variable(fd, adr, index, &info);
      if (status != MSCB_SUCCESS)
         return status;

      /* setup cache entry */
      i = n_cache;
      n_cache++;

      cache[i].fd = fd;
      cache[i].adr = adr;
      cache[i].index = index;
      cache[i].last = 0;
      cache[i].size = info.width;

      /* allocate at least 4 bytes */
      s = info.width;
      if (s < 4)
         s = 4;
      cache[i].data = malloc(s);
      if (cache[i].data == NULL)
         return MSCB_NO_MEM;
      memset(cache[i].data, 0, s);

      /* read initial value */
      s = cache[i].size;
      status = mscb_read(fd, adr, index, cache[i].data, &s);
      if (status != MSCB_SUCCESS)
         return status;

      memcpy(data, cache[i].data, size);
   }

   debug_log("mscb_link() return MSCB_SUCCESS\n", 1);

   return MSCB_SUCCESS;
}


/*------------------------------------------------------------------*/

int mscb_select_device(char *device, int size, int select)
/********************************************************************\

  Routine: mscb_select_device

  Purpose: Select MSCB submaster device. Show dialog if sevral
           possibilities

  Input:
    none

  Output:
    char *device            Device name
    int  size               Size of device string buffer in bytes
    int  select             If 1, ask user which device to select,
                            if zero, use first device (for Labview)

  Function value:
    MSCB_SUCCESS            Successful completion
    0                       No submaster found

\********************************************************************/
{
   char list[10][256], str[256], buf[64];
   int status, usb_index, found, i, n, index, error_code;
   MUSB_INTERFACE *ui;

   n = 0;
   *device = 0;
   error_code = 0;

   /* search for temporary file descriptor */
   for (index = 0; index < MSCB_MAX_FD; index++)
      if (mscb_fd[index].fd == 0)
         break;

   if (index == MSCB_MAX_FD)
      return -1;

   /* check USB devices */
   for (usb_index = found = 0 ; usb_index < 127 ; usb_index ++) {

      status = musb_open(&ui, 0x10C4, 0x1175, usb_index, 1, 0);
      if (status != MUSB_SUCCESS)
         break;

      sprintf(str, "usb%d", found);
      mscb_fd[index].mutex_handle = mscb_mutex_create(str);

      if (!mscb_lock(index + 1)) {
         printf("lock failed\n");
         debug_log("return EMSCB_LOCKED\n", 0);
         return EMSCB_LOCKED;
      }

      /* check if it's a subm_250 */
      buf[0] = 0;
      musb_write(ui, 0, buf, 1, 1000);

      i = musb_read(ui, 0, buf, sizeof(buf), MUSB_TIMEOUT);
      if (strcmp(buf, "SUBM_250") == 0)
         sprintf(list[n++], "usb%d", found++);

      mscb_release(index + 1);

      musb_close(ui);
   }

   /* check LPT devices */
   for (i = 0; i < 1; i++) {
#ifdef _MSC_VER
      sprintf(str, "lpt%d", i + 1);
#else
      sprintf(str, "/dev/parport%d", i);
#endif

      mscb_fd[index].type = MSCB_TYPE_LPT;

      status = lpt_init(str, index);
      if (status < 0)
         error_code = status;
      else
         lpt_close(mscb_fd[index].fd);
      memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
      if (status == 0)
         sprintf(list[n++], str);
      else
         break;
   }

   if (n == 0) {
      if (error_code == -3)
         printf("mscb.c: Cannot access parallel port (No DirectIO driver installed)\n");
      if (error_code == -4)
         printf("mscb.c: Cannot access parallel port, locked by other application\n");
      return 0;
   }

   /* if only one device, return it */
   if (n == 1 || select == 0) {
      strlcpy(device, list[0], size);
      return MSCB_SUCCESS;
   }

   do {
      printf("Found several submasters, please select one:\n\n");
      for (i = 0; i < n; i++)
         printf("%d: %s\n", i + 1, list[i]);

      printf("\n");
      fgets(str, sizeof(str), stdin);
      i = atoi(str);
      if (i > 0 && i <= n) {
         strlcpy(device, list[i - 1], size);
         return MSCB_SUCCESS;
      }

      printf("Invalid selection, please enter again\n");
   } while (1);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

typedef struct {
   char           host_name[20];
   char           password[20];
   unsigned char  eth_mac_addr[6];
   unsigned short magic;
} SUBM_CFG;

int set_mac_address(int fd)
/********************************************************************\

  Routine: set_mac_address

  Purpose: Ask for hostname/password and set MAC configuration of
           subm_260 interface over ethernet

  Input:
    int  fd                 File descriptor for connection
    <parameters are asked interactively>

  Output:
    None

  Function value:
    MSCB_SUCCESS            Successful completion
    0                       Error

\********************************************************************/
{
   char str[256];
   unsigned char buf[64];
   SUBM_CFG cfg;
   int n;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return 0;

   if (mscb_fd[fd - 1].type != MSCB_TYPE_ETH) {
      printf("This command only works on ethernet submasters.\n");
      return 0;
   }

   printf("Hostname (should be \"MSCBxxx\") : MSCB");
   fgets(str, sizeof(str), stdin);
   n = atoi(str);
   if (n < 0 || n > 999) {
      printf("Hostname must be in the range MSCB000 to MSCB999\n");
      return 0;
   }
   sprintf(cfg.host_name, "MSCB%03d", n);

   cfg.eth_mac_addr[0] = 0x00;
   cfg.eth_mac_addr[1] = 0x50;
   cfg.eth_mac_addr[2] = 0xC2;
   cfg.eth_mac_addr[3] = 0x46;
   cfg.eth_mac_addr[4] = (unsigned char)(0xD0 | (n >> 8));
   cfg.eth_mac_addr[5] = (unsigned char)(n & 0xFF);

   printf("MAC Address is 00-50-C2-46-%02X-%02X\n",
           cfg.eth_mac_addr[4], cfg.eth_mac_addr[5]);

   printf("Enter optional password        : ");
   fgets(str, sizeof(str), stdin);
   if (strlen(str) > sizeof(cfg.password)-1) {
      printf("Password too long\n");
      return 0;
   }
   while (strlen(str)>0 &&
           (str[strlen(str)-1] == '\r' || str[strlen(str)-1] == '\n'))
      str[strlen(str)-1] = 0;
   strcpy(cfg.password, str);

   cfg.magic = 0x3412;

   buf[0] = MCMD_FLASH;
   memcpy(buf+1, &cfg, sizeof(cfg));
   mscb_out(fd, buf, 1+sizeof(cfg), RS485_FLAG_CMD);

   n = mscb_in(fd, buf, 2, TO_SHORT);
   if (n == 2 && buf[0] == MCMD_ACK) {
      printf("\nConfiguration successfully downloaded.\n");
      return MSCB_SUCCESS;
   }

   printf("Error downloading configuration.\n");

   return 0;
}

/*------------------------------------------------------------------*/

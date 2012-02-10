/********************************************************************\

  Name:         mscb.c
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus communication functions

  $Id: mscb.c 4982 2011-02-18 13:54:26Z ritt $

\********************************************************************/

#define MSCB_LIBRARY_VERSION   "2.6.0"
#define MSCB_PROTOCOL_VERSION  "5"
#define MSCB_SUBM_VERSION      5

#ifdef _MSC_VER                 // Windows includes

#pragma warning(disable:4996)

#include <windows.h>
#include <conio.h>
#include <sys/timeb.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>

#elif defined(OS_DARWIN)

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <assert.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#elif defined(OS_LINUX)        // Linux includes

#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_USB
#include <usb.h>
#endif

#endif

#include <stdio.h>
#include <assert.h>

#include "strlcpy.h"

#ifdef HAVE_USB
#include "musbstd.h"
#endif

#include "mscb.h"
#include "mscbrpc.h"

/*------------------------------------------------------------------*/

MSCB_FD mscb_fd[MSCB_MAX_FD];

#ifdef HAVE_USB

/* usb endpoints */

#if defined(OS_DARWIN)

#define EP_READ  1
#define EP_WRITE 2

#else

#define EP_READ  0x81
#define EP_WRITE 0x02

#endif

#endif // HAVE_USB

/* constants */

#define TO_SHORT          100   /* 100 ms */
#define TO_LONG          5000   /* 5 s    */

int _debug_flag;         /* global debug flag */
void debug_log(const char *format, int start, ...);

/* RS485 flags for USB submaster */
#define RS485_FLAG_BIT9      (1<<0)
#define RS485_FLAG_NO_ACK    (1<<1)
#define RS485_FLAG_SHORT_TO  (1<<2)  /* 400us for ping */
#define RS485_FLAG_LONG_TO   (1<<3)  /* 5s for upgrade */
#define RS485_FLAG_CMD       (1<<4)
#define RS485_FLAG_ADR_CYCLE (1<<5)
#define RS485_FLAG_NO_RETRY  (1<<6)  /* for mscb_scan_udp */

/* header for UDP communication */
typedef struct {
   unsigned short size;
   unsigned short seq_num;
   unsigned char  flags;
   unsigned char  version;
} UDP_HEADER;

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

CACHE_INFO_VAR *cache_info_var = NULL;
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

void debug_log(const char *format, int start, ...)
{
   int fh;
   char str[2000], line[2000];
   va_list argptr;

   if (!_debug_flag)
      return;

   line[0] = 0;
   if (start) {
#ifdef OS_DARWIN
      struct timeval tv;
      gettimeofday(&tv, NULL);
      strcpy(line, ctime(&tv.tv_sec) + 11);
      sprintf(line + 8, ".%03d ", tv.tv_usec/1000);
#else
      struct timeb tb;
      ftime(&tb);
      strcpy(line, ctime(&tb.time) + 11);
      sprintf(line + 8, ".%03d ", tb.millitm);
#endif
   }

   va_start(argptr, start);
   vsprintf(str, (char *) format, argptr);
   va_end(argptr);

   strcat(line, str);

   /* if debug flag equals 2, write to stdout */
   if (_debug_flag == 2)
      write(fileno(stdout), line, strlen(line));
   else {
      /* else write to file */
      fh = open("mscb_debug.log", O_CREAT | O_WRONLY | O_APPEND, 0644);
      if (fh < 0)
         return;

      write(fh, line, strlen(line));
      close(fh);
   }
}

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

/*----retries and timeouts------------------------------------------*/

static int mscb_max_retry = 10;
static int mscb_usb_timeout = 6000; /* 6 sec, must be longer than TO_LONG */

int EXPRT mscb_get_usb_timeout()
/********************************************************************\

  Routine: mscb_get_usb_timeout

  Purpose: Get the current setting of the MSCB USB timeout

  Return value:
    current value of the MSCB USB timeout

\********************************************************************/
{
   return mscb_usb_timeout;
}

int EXPRT mscb_set_usb_timeout(int timeout)
/********************************************************************\

  Routine: mscb_set_usb_timeout

  Purpose: Set the timeout value for MSCB USB operations

  Input:
    int timeout         USB timeout in ms

  Function value:
    old value of the USB timeout

\********************************************************************/
{
   int old_timeout = mscb_usb_timeout;
   mscb_usb_timeout = timeout;
   return old_timeout;
}

int mscb_get_max_retry()
/********************************************************************\

  Routine: mscb_get_max_retry

  Purpose: Get the current setting of the MSCB maximum retry limit

  Return value:
    current value of the MSCB maximum retry limit

\********************************************************************/
{
   return mscb_max_retry;
}

int EXPRT mscb_set_max_retry(int max_retry)
/********************************************************************\

  Routine: mscb_set_max_retry

  Purpose: Set the limit for retrying MSCB operations

  Input:
    int max_retry           new value of the mscb maximum retry limit

  Function value:
    old value of the maximum retry limit

\********************************************************************/
{
   int old_retry = mscb_max_retry;
   mscb_max_retry = max_retry;
   return old_retry;
}

int mscb_get_eth_max_retry(int fd)
/********************************************************************\

  Routine: mscb_get_eth_max_retry

  Purpose: Get the current setting of the MSCB ethernet retry limit

  Input:
    inf fd                   file descriptor for mscb connection

  Return value:
    current value of the MSCB ethernet retry limit

\********************************************************************/
{
   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   return mscb_fd[fd - 1].eth_max_retry;
}

int EXPRT mscb_set_eth_max_retry(int fd, int eth_max_retry)
/********************************************************************\

  Routine: mscb_set_eth_max_retry

  Purpose: Set retry limit for MSCB ethernet communications

  Input:
    inf fd                   file descriptor for mscb connection
    int eth_max_retry        new value for ethernet retry limit

  Function value:
    old value of the ethernet maximum retry limit

\********************************************************************/
{
   int old_retry;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;
   
   old_retry = mscb_fd[fd - 1].eth_max_retry;
   mscb_fd[fd - 1].eth_max_retry = eth_max_retry;
   return old_retry;
}

/*---- semaphore functions ------------------------------------------*/

int mscb_semaphore_create(const char *device)
{
#ifdef _MSC_VER
   HANDLE semaphore_handle;

   semaphore_handle = CreateMutex(NULL, FALSE, device);
   if (semaphore_handle == 0)
      return 0;

   return (int)semaphore_handle;

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
   int key, status, fh, semaphore_handle;
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
   semaphore_handle = semget(key, 1, 0);
   if (semaphore_handle < 0) {
      semaphore_handle = semget(key, 1, IPC_CREAT);
      status = 2;
   }

   if (semaphore_handle < 0) {
      printf("semget() failed, errno = %d", errno);
      return 0;
   }

   buf.sem_perm.uid = getuid();
   buf.sem_perm.gid = getgid();
   buf.sem_perm.mode = 0666;
   arg.buf = &buf;

   semctl(semaphore_handle, 0, IPC_SET, arg);

   /* if semaphore was created, set value to one */
   if (status == 2) {
      arg.val = 1;
      if (semctl(semaphore_handle, 0, SETVAL, arg) < 0)
         return 0;
   }

   return semaphore_handle;
#endif // OS_LINUX
}

/*------------------------------------------------------------------*/

int mscb_lock(int fd)
{
#ifdef _MSC_VER
   int status;

   /* wait with a timeout of 10 seconds */
   status = WaitForSingleObject((HANDLE)mscb_fd[fd - 1].semaphore_handle, 10000);

   if (status == WAIT_FAILED)
      return 0;
   if (status == WAIT_TIMEOUT)
      return 0;

#elif defined(OS_LINUX)

#ifdef HAVE_USB
   //if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {
   //   if (usb_claim_interface((usb_dev_handle *) mscb_fd[fd - 1].ui->dev, 0) < 0)
   //     return 0;
   //}
#endif // HAVE_USB

   if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {

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
         status = semop(mscb_fd[fd - 1].semaphore_handle, &sb, 1);

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

   status = ReleaseMutex((HANDLE)mscb_fd[fd - 1].semaphore_handle);
   if (status == FALSE)
      return 0;

#elif defined(OS_LINUX)

   if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {
#ifdef HAVE_USB
     //      if (usb_release_interface((usb_dev_handle *) mscb_fd[fd - 1].ui->dev, 0) < 0)
     //    return 0;
#endif // HAVE_USB
   }
     
   else if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {
 
      /* release semaphore */
      struct sembuf sb;
      int status;

      sb.sem_num = 0;
      sb.sem_op = 1;            /* increment semaphore */
      sb.sem_flg = SEM_UNDO;

      do {
         status = semop(mscb_fd[fd - 1].semaphore_handle, &sb, 1);

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

/*---- Low level routines for USB communication --------------------*/

#ifdef HAVE_USB

int subm250_check(MUSB_INTERFACE *ui)
{
   int  j, status, nwrite;
   char buf[80];

   for (j=0; j<2; j++) {
      /* check if it's a subm_250 */
      nwrite = 1;
      buf[0] = 0;
      status = musb_write(ui, EP_WRITE, buf, nwrite, mscb_usb_timeout);
      if (status != nwrite) {
         fprintf(stderr,"mscb.c::subm250_open(): musb_write() error %d\n", status);
         return EMSCB_NOT_FOUND;
      }

      status = musb_read(ui, EP_READ, buf, sizeof(buf), mscb_usb_timeout);
      if (strcmp(buf, "SUBM_250") == 0)
         return MSCB_SUCCESS;

      fprintf(stderr,"mscb.c::subm250_open(): USB device is present, but not responding, trying USB reset\n");
      musb_reset(ui);
   }

   return EMSCB_NOT_FOUND;
}

int subm250_open(MUSB_INTERFACE **ui, int usb_index)
{
   int  i, status, found;

   for (i = found = 0 ; i<127 ; i++) {
      status = musb_open(ui, 0x10C4, 0x1175, i, 1, 0);
      if (status != MUSB_SUCCESS) {
         //printf("musb_open returned %d\n", status);
         return EMSCB_NOT_FOUND;
      }

      if (subm250_check(*ui) == MSCB_SUCCESS)
         if (found++ == usb_index)
            return MSCB_SUCCESS;
   }

   return EMSCB_NOT_FOUND;
}

#endif // HAVE_USB

/*---- Low level routines for UDP communication --------------------*/

int msend_udp(int index, unsigned char *buffer, int size)
{
   int count;

   count = sendto(mscb_fd[index-1].fd, (const char *)buffer, size, 0, 
                  (struct sockaddr *) &mscb_fd[index-1].eth_addr, 
                  sizeof(struct sockaddr));

   return count;
}

/*------------------------------------------------------------------*/

int mrecv_udp(int index, unsigned char *buf, int *size, int millisec)
{
   int n, status;
   unsigned char buffer[1024+6];
   fd_set readfds;
   struct timeval timeout;
   UDP_HEADER *pudp;

   /* receive buffer in UDP mode */

   memset(buf, 0, *size);

   FD_ZERO(&readfds);
   FD_SET(mscb_fd[index-1].fd, &readfds);

   timeout.tv_sec = millisec / 1000;
   timeout.tv_usec = (millisec % 1000) * 1000;

   do {
      status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
   } while (status == -1);        /* dont return if an alarm signal was cought */

   if (!FD_ISSET(mscb_fd[index-1].fd, &readfds)) {
      *size = 0;
      return MSCB_TIMEOUT;
   }

   n = recv(mscb_fd[index-1].fd, (char *)buffer, sizeof(buffer), 0);

   pudp = (UDP_HEADER *)buffer;

   /* check version */
   if (pudp->version != MSCB_SUBM_VERSION) {
      *size = 0;
      return MSCB_SUBM_ERROR;
   }

   /* check size */
   if (ntohs(pudp->size) + (int)sizeof(UDP_HEADER) != n) {
      *size = 0;
      return MSCB_FORMAT_ERROR;
   }

   n = ntohs(pudp->size);

   /* check sequence number */
   if (ntohs(pudp->seq_num) != mscb_fd[index-1].seq_nr) {
#ifndef _USRDLL
      /*
      printf("mrecv_udp: received wrong sequence number %d instead %d, fd=%d\n", 
         ntohs(pudp->seq_num), mscb_fd[index-1].seq_nr, index);
      */
#endif
      /* try again */
      return mrecv_udp(index, buffer, size, millisec);
   }

   /* check size */
   if (n > *size) {
      *size = 0;
      return MSCB_INVAL_PARAM;
   }

   memcpy(buf, pudp+1, n);
   *size = n;

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_exchg(int fd, unsigned char *buffer, int *size, int len, int flags)
/********************************************************************\

  Routine: mscb_exchg

  Purpose: Exchange one data backe with MSCB submaster through
           USB, UDP or RPC

  Input:
    int  fd                 file descriptor
    char *buffer            data buffer
    int  len                number of bytes in buffer
    int  *size              total size of buffer
    int  flags              bit combination of:
                              RS485_FLAG_BIT9      node arressing
                              RS485_FLAG_NO_ACK    no acknowledge
                              RS485_FLAG_SHORT_TO  short/
                              RS485_FLAG_LONG_TO     long timeout
                              RS485_FLAG_CMD       direct submaster command

  Output:
    int *size               number of bytes in buffer returned

  Function value:
    MSCB_INVAL_PARAM        Invalid parameter
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout

\********************************************************************/
{
   int i, n, retry, timeout, status;
   unsigned char eth_buf[1024], ret_buf[1024];
   UDP_HEADER *pudp;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (mscb_lock(fd) != MSCB_SUCCESS)
      return MSCB_SEMAPHORE;

   /*---- USB code ----*/

   if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {

#ifdef HAVE_USB
      unsigned char usb_buf[65];
      
      if (len >= 64 || len < 1) {
         mscb_release(fd);
         return MSCB_INVAL_PARAM;
      }

      /* add flags in first byte of USB buffer */
      usb_buf[0] = (unsigned char)flags;
      memcpy(usb_buf + 1, buffer, len);

      /* send on OUT pipe */
      i = musb_write(mscb_fd[fd - 1].ui, EP_WRITE, usb_buf, len + 1, mscb_usb_timeout);

      if (i == 0) {
         /* USB submaster might have been dis- and reconnected, so reinit */
         musb_close(mscb_fd[fd - 1].ui);

         i = subm250_open(&mscb_fd[fd - 1].ui, atoi(mscb_fd[fd - 1].device+3));
         if (i < 0) {
            if (size && *size)
               memset(buffer, 0, *size);
            mscb_release(fd);
            return MSCB_TIMEOUT;
         }

         i = musb_write(mscb_fd[fd - 1].ui, EP_WRITE, usb_buf, len + 1, mscb_usb_timeout);
      }

      if (i != len + 1) {
         if (size && *size)
            memset(buffer, 0, *size);
         mscb_release(fd);
         return MSCB_TIMEOUT;
      }

      if (flags & RS485_FLAG_NO_ACK) {
         if (size && *size)
            memset(buffer, 0, *size);
         mscb_release(fd);
         return MSCB_SUCCESS;
      }

      if (size == NULL) {
         if (size && *size)
            memset(buffer, 0, *size);
         mscb_release(fd);
         return MSCB_INVAL_PARAM;
      }

      /* receive result on IN pipe */
      n = 0;
      do {
         i = musb_read(mscb_fd[fd - 1].ui, EP_READ, buffer+n, *size-n > 61 ? 61 : *size - n, mscb_usb_timeout);
         if (i == 61) {
            n += 60;
            break;
         }
         n += i;
      } while (i == 60);
      *size = n;

#endif // HAVE_USB
   }

   /*---- Ethernet code ----*/

   if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {
      if (len >= 256 || len < 1) {
         mscb_release(fd);
         return MSCB_INVAL_PARAM;
      }

      status = 0;

      /* try several times according to eth_max_retry, in case packets got lost */
      for (retry=0 ; retry<mscb_fd[fd - 1].eth_max_retry ; retry++) {
         /* increment and write sequence number */
         mscb_fd[fd - 1].seq_nr = (mscb_fd[fd - 1].seq_nr + 1) % 0x10000;

         /* fill UDP header */
         pudp = (UDP_HEADER *)eth_buf;
         pudp->size = htons((short) len);
         pudp->seq_num = htons((short) mscb_fd[fd - 1].seq_nr);
         pudp->flags = flags;
         pudp->version = MSCB_SUBM_VERSION;

         memcpy(pudp+1, buffer, len);

         /* send over UDP link */
         i = msend_udp(fd, eth_buf, len + sizeof(UDP_HEADER));

         if (i != len + (int)sizeof(UDP_HEADER)) {
            if (size && *size)
               memset(buffer, 0, *size);
            mscb_release(fd);
            return MSCB_TIMEOUT;
         }

         if (flags & RS485_FLAG_NO_ACK) {
            if (size && *size)
               memset(buffer, 0, *size);
            mscb_release(fd);
            return MSCB_SUCCESS;
         }

         if (size == NULL) {
            if (size && *size)
               memset(buffer, 0, *size);
            mscb_release(fd);
            return MSCB_INVAL_PARAM;
         }

         memset(ret_buf, 0, sizeof(ret_buf));

         /* increase timeout with each retry 0.1s, 0.2s, ... */
         timeout = 100 * (retry+1);

         if (flags & RS485_FLAG_LONG_TO)
            timeout = 5000;

#ifndef _USRDLL
         /* few retries are common, so only print warning starting from 5th retry */
         if (retry > 4 && status == MSCB_TIMEOUT)
            printf("mscb_exchg: retry %d out of %d with %d ms timeout, fd = %d\n", 
                    retry, mscb_fd[fd - 1].eth_max_retry, timeout, fd);
#endif

         /* receive result on IN pipe */
         n = sizeof(ret_buf);
         status = mrecv_udp(fd, ret_buf, &n, timeout);

         /* return if invalid version */
         if (status == MSCB_SUBM_ERROR) {
            if (size && *size)
               memset(buffer, 0, *size);
            mscb_release(fd);
            *size = 0;
            return n;
         }

         if (n > 0) {
            /* check if return data fits into buffer */
            if (n > *size) {
               memcpy(buffer, ret_buf, *size);
            } else {
               memset(buffer, 0, *size); 
               memcpy(buffer, ret_buf, n);
               *size = n;
            }

            mscb_release(fd);
            return MSCB_SUCCESS;
         }

         /*
         if (retry > 0)
            debug_log("RETRY %d: dev=%s, status=%d, n=%d\n", retry, mscb_fd[fd-1].device, status, n);
         */

         if (flags & RS485_FLAG_NO_RETRY) {
            *size = 0;
            break;
         }
      }

      /* no reply after all the retries, so return error */
      if (size && *size)
         memset(buffer, 0, *size);
      if (size)
         *size = 0;
      mscb_release(fd);
      return MSCB_TIMEOUT;
   }

   mscb_release(fd);

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

int mscb_init(char *device, int bufsize, const char *password, int debug)
/********************************************************************\

  Routine: mscb_init

  Purpose: Initialize and open MSCB

  Input:
    char *device            "<host>:device" for RPC connection
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
    EMSCB_LOCKED            MSCB system locked by other user
    EMSCB_NO_ACCESS         No access to submaster
    EMSCB_INVAL_PARAM       Invalid parameter
    EMSCB_NOT_FOUND         USB submaster not found
    EMSCB_NO_WRITE_ACCESS   No write access under linux
    EMSCB_WRONG_PASSWORD    Wrong password
    EMSCB_SUMB_VERSION      Submaster has wrong protocol version

\********************************************************************/
{
   int index, i, n;
   int status;
   char dev3[256];
   unsigned char buf[64];
   struct hostent *phe;
   struct sockaddr_in *psa_in;

   status = 0;
   if (!device[0]) {
      if (debug == -1)
         status = mscb_select_device(device, bufsize, 0); /* for LabView */
      else
         status = mscb_select_device(device, bufsize, 1); /* interactively ask for device */

      if (status == 0)
         return EMSCB_NOT_FOUND;
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
   cache_info_var = NULL;
   n_cache_info_var = 0;

   /* check for RPC connection */
#ifdef HAVE_MRPC
   if (strchr(device, ':')) {
      char remote_device[256], host[256];

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

      mscb_fd[index].remote_fd = mrpc_call(mscb_fd[index].fd, RPC_MSCB_INIT, remote_device, bufsize, password, debug);
      if (mscb_fd[index].remote_fd < 0) {
         mrpc_disconnect(mscb_fd[index].fd);
         mscb_fd[index].fd = 0;
         return EMSCB_RPC_ERROR;
      }

      sprintf(device, "%s:%s", host, remote_device);

      debug_log("return %d\n", 0, index + 1);
      return index + 1;
   }
#endif

   /* check which device type */
   strcpy(dev3, device);
   dev3[3] = 0;
   strcpy(mscb_fd[index].device, device);
   mscb_fd[index].type = 0;

   /* USBx */
   if (strieq(dev3, "usb"))
      mscb_fd[index].type = MSCB_TYPE_USB;

   /* MSCBxxx */
   if (strieq(dev3, "msc") || (atoi(device) > 0 && strchr(device, '.')))
      mscb_fd[index].type = MSCB_TYPE_ETH;

   if (mscb_fd[index].type == 0)
      return EMSCB_INVAL_PARAM;

   /*---- initialize submaster ----*/

   if (mscb_fd[index].semaphore_handle == 0)
      mscb_fd[index].semaphore_handle = mscb_semaphore_create(device);

   if (mscb_fd[index].type == MSCB_TYPE_USB) {

#ifdef HAVE_USB
      if (!mscb_lock(index + 1)) {
         debug_log("return EMSCB_LOCKED\n", 0);
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         return EMSCB_LOCKED;
      }

      status = subm250_open(&mscb_fd[index].ui, atoi(device+3));
      mscb_release(index + 1);

      if (status != MSCB_SUCCESS)
         return EMSCB_NOT_FOUND;

      /* mark device descriptor used */
      mscb_fd[index].fd = 1;

      n = 0;

      /* linux needs some time to start-up ...??? */
      for (i = 0; i < mscb_max_retry; i++) {
         /* check if submaster alive */
         buf[0] = MCMD_ECHO;
         n = sizeof(buf);
         mscb_exchg(index + 1, buf, &n, 1, RS485_FLAG_CMD);

         if (n > 0 && n < 2) {
            debug_log("return EMSCB_RPC_ERROR\n", 0);
            memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
            return EMSCB_RPC_ERROR;
         }

         if (n > 0 && (n < 4 || buf[1] != MSCB_SUBM_VERSION)) {
            /* invalid version */
            debug_log("return EMSCB_SUBM_VERSION\n", 0);
            memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
            return EMSCB_SUBM_VERSION;
         }

         if (n == 4)
            break;
      }
#endif // HAVE_USB   
   }

   if (mscb_fd[index].type == MSCB_TYPE_ETH) {

#ifdef _MSC_VER
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return -1;
   }
#endif

      /* retrieve destination address */
      phe = gethostbyname(mscb_fd[index].device);
      if (phe == NULL) {
         debug_log("return EMSCB_RPC_ERROR\n", 0);
         return EMSCB_RPC_ERROR;
      }
      memset(&mscb_fd[index].eth_addr, 0, sizeof(mscb_fd[index].eth_addr));
      psa_in = (struct sockaddr_in *)mscb_fd[index].eth_addr;
      memcpy((char *) &(psa_in->sin_addr), phe->h_addr, phe->h_length);
      psa_in->sin_port = htons((short) MSCB_NET_PORT);
      psa_in->sin_family = AF_INET;

      mscb_fd[index].fd = socket(AF_INET, SOCK_DGRAM, 0);
      if (mscb_fd[index].fd == -1) {
         debug_log("return EMSCB_RPC_ERROR\n", 0);
         return EMSCB_RPC_ERROR;
      }

      if (!mscb_lock(index + 1)) {
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         debug_log("return EMSCB_LOCKED\n", 0);
         return EMSCB_LOCKED;
      }
      mscb_release(index + 1);

      /* set default number of retries */
      mscb_fd[index].eth_max_retry = 10;

      /* check if submaster alive */
      buf[0] = MCMD_ECHO;
      n = sizeof(buf);
      status = mscb_exchg(index + 1, buf, &n, 1, RS485_FLAG_CMD);
      
      if (status == MSCB_TIMEOUT || n < 2) {
         debug_log("return EMSCB_RPC_ERROR\n", 0);
         return EMSCB_RPC_ERROR;
      }

      if (n < 4 || buf[1] != MSCB_SUBM_VERSION) {
         /* invalid version */
         debug_log("return EMSCB_SUBM_VERSION\n", 0);
         memset(&mscb_fd[index], 0, sizeof(MSCB_FD));
         return EMSCB_SUBM_VERSION;
      }

      /* authenticate */
      memset(buf, 0, sizeof(buf));
      buf[0] = MCMD_TOKEN;
      if (password)
         strcpy((char *)(buf+1), password);
      else
         buf[1] = 0;

      n = sizeof(buf);
      mscb_exchg(index + 1, buf, &n, 21, RS485_FLAG_CMD);

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

#ifdef HAVE_MRPC
   if (mrpc_connected(fd)) {
      mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_EXIT, mscb_fd[fd - 1].remote_fd);
      mrpc_disconnect(mscb_fd[fd - 1].fd);
   }
#endif

#ifdef HAVE_USB
   if (mscb_fd[fd - 1].type == MSCB_TYPE_USB)
      musb_close(mscb_fd[fd - 1].ui);
#endif

#ifdef HAVE_MRPC
   if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH)
      mrpc_disconnect(mscb_fd[fd - 1].fd);
#endif

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
    int fd                  File descriptor obtained via mscb_init()
    int bufsize             Size of device string
    char *device            device name, "" if invalid fd

\********************************************************************/
{
   if (!device)
      return;

   *device = 0;
   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd)) {
      mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_GET_DEVICE,
                mscb_fd[fd - 1].remote_fd, device, bufsize);
   }
#endif

   strlcpy(device, mscb_fd[fd-1].device, bufsize);
}

/*------------------------------------------------------------------*/

int mscb_addr(int fd, int cmd, unsigned short adr, int retry)
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

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving ping acknowledge
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   unsigned char buf[64];
   int n, size, status;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_ADDR, mscb_fd[fd - 1].remote_fd, cmd, adr, retry);
#endif

   for (n = 0; n < retry; n++) {
      buf[0] = (unsigned char)cmd;
      size = sizeof(buf);
      if (cmd == MCMD_ADDR_NODE8 || cmd == MCMD_ADDR_GRP8) {
         buf[1] = (unsigned char) adr;
         buf[2] = crc8(buf, 2);
         status = mscb_exchg(fd, buf, &size, 3, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO | RS485_FLAG_NO_ACK); 
      } else if (cmd == MCMD_PING8) {
         buf[1] = (unsigned char) adr;
         buf[2] = crc8(buf, 2);
         status = mscb_exchg(fd, buf, &size, 3, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO); 
      } else if (cmd == MCMD_ADDR_NODE16 || cmd == MCMD_ADDR_GRP16) {
         buf[1] = (unsigned char) (adr >> 8);
         buf[2] = (unsigned char) (adr & 0xFF);
         buf[3] = crc8(buf, 3);
         status = mscb_exchg(fd, buf, &size, 4, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO | RS485_FLAG_NO_ACK); 
      } else if (cmd == MCMD_PING16) {
         buf[1] = (unsigned char) (adr >> 8);
         buf[2] = (unsigned char) (adr & 0xFF);
         buf[3] = crc8(buf, 3);
         status = mscb_exchg(fd, buf, &size, 4, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO); 
      } else {
         buf[1] = crc8(buf, 1);
         status = mscb_exchg(fd, buf, &size, 2, RS485_FLAG_BIT9 | RS485_FLAG_SHORT_TO | RS485_FLAG_NO_ACK); 
      }

      if (status != MSCB_SUCCESS)
         return MSCB_SUBM_ERROR;

      if (cmd == MCMD_PING8 || cmd == MCMD_PING16) {
         if (buf[0] == MCMD_ACK)
            return MSCB_SUCCESS;

         if (retry > 1) {
            /* send 0's to overflow partially filled node receive buffer */
            memset(buf, 0, sizeof(buf));
            mscb_exchg(fd, buf, NULL, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);

            /* wait some time */
            Sleep(10);
         }

         /* try again.... */
      } else
         return MSCB_SUCCESS;
   }

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
    MSCB_TIMEOUT            Timeout receiving ping acknowledge

\********************************************************************/
{
   int size;
   unsigned char buf[64];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_REBOOT, mscb_fd[fd - 1].remote_fd, addr, gaddr, broadcast);
#endif

   size = sizeof(buf);
   if (addr >= 0) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (addr >> 8);
      buf[2] = (unsigned char) (addr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_INIT;
      buf[5] = crc8(buf+4, 1);
      mscb_exchg(fd, buf, &size, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (gaddr >= 0) {
      buf[0] = MCMD_ADDR_GRP16;
      buf[1] = (unsigned char) (gaddr >> 8);
      buf[2] = (unsigned char) (gaddr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_INIT;
      buf[5] = crc8(buf+4, 1);
      mscb_exchg(fd, buf, &size, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (broadcast) {
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_INIT;
      buf[3] = crc8(buf+2, 1);
      mscb_exchg(fd, buf, &size, 4, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   }

   mscb_clear_info_cache(fd);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_subm_reset(int fd)
/********************************************************************\

  Routine: mscb_subm_reset

  Purpose: Reset submaster via hardware reset

  Input:
    int fd                  File descriptor for connection

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_SEMPHORE           Cannot obtain semaphore for mscb

\********************************************************************/
{
   unsigned char buf[10];
   int size;

   debug_log("mscb_subm_reset(fd=%d) ", 1, fd);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SUBM_RESET, mscb_fd[fd - 1].remote_fd);
#endif

   if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {

      buf[0] = MCMD_INIT;
      mscb_exchg(fd, buf, NULL, 1, RS485_FLAG_CMD | RS485_FLAG_NO_ACK);

      Sleep(5000);  // let submaster obtain IP address

      /* check if submaster alive */
      size = sizeof(buf);
      buf[0] = MCMD_ECHO;
      mscb_exchg(fd, buf, &size, 1, RS485_FLAG_CMD);
      if (size < 2) {
         debug_log("return MSCB_SUBM_ERROR (mrpc_connect)\n", 0);
         return MSCB_SUBM_ERROR;
      }

   } else if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {
#ifdef HAVE_USB
      buf[0] = MCMD_INIT;
      mscb_exchg(fd, buf, NULL, 1, RS485_FLAG_CMD | RS485_FLAG_NO_ACK);
      musb_close(mscb_fd[fd - 1].ui);
      Sleep(1000);
      subm250_open(&mscb_fd[fd - 1].ui, atoi(mscb_fd[fd - 1].device+3));
#endif
   }

   /* send 0's to overflow partially filled node receive buffer */
   memset(buf, 0, sizeof(buf));
   mscb_exchg(fd, buf, NULL, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
   Sleep(10);

   debug_log("return MSCB_SUCCESS\n", 0);
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_ping(int fd, unsigned short adr, int quick)
/********************************************************************\

  Routine: mscb_ping

  Purpose: Ping node to see if it's alive

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    int quick               If 1, do a quick ping without retries

  Output:
    none

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int status;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_PING, mscb_fd[fd - 1].remote_fd, adr);
#endif

   /* call mscb_addr with/without retries */
   status = mscb_addr(fd, MCMD_PING16, adr, quick ? 1 : mscb_fd[fd - 1].eth_max_retry);

   return status;
}

/*------------------------------------------------------------------*/

void mscb_clear_info_cache(int fd)
/* called internally when a node gets upgraded or address changed */
{
   free(cache_info_var);
   cache_info_var = NULL;
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int size, retry;
   unsigned char buf[256];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_INFO, mscb_fd[fd - 1].remote_fd, adr, info);
#endif

   for (retry = 0 ; retry < mscb_max_retry ; retry++) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_GET_INFO;
      buf[5] = crc8(buf+4, 1);
      
      size = sizeof(buf);
      mscb_exchg(fd, buf, &size, 6, RS485_FLAG_LONG_TO | RS485_FLAG_ADR_CYCLE);

      if (size == (int) sizeof(MSCB_INFO) + 3)
         break;

      if (size == (int) sizeof(MSCB_INFO) + 3 - 6) // old format without RTC
         break;
   }

   if (size < (int) sizeof(MSCB_INFO) + 3 - 6)
      return MSCB_TIMEOUT;

   memcpy(info, buf + 2, sizeof(MSCB_INFO));
   if (size == (int) sizeof(MSCB_INFO) + 3 - 6) // old format without RTC
      memset(info->rtc, 0, 6);

   /* do CRC check */
   if (crc8(buf, size-1) != buf[size-1])
      return MSCB_CRC_ERROR;

   WORD_SWAP(&info->node_address);
   WORD_SWAP(&info->group_address);
   WORD_SWAP(&info->svn_revision);

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int size;
   unsigned char buf[256];
   unsigned long u;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_UPTIME, mscb_fd[fd - 1].remote_fd, adr, uptime);
#endif

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = MCMD_GET_UPTIME;
   buf[5] = crc8(buf+4, 1);
   size = sizeof(buf);
   mscb_exchg(fd, buf, &size, 6, RS485_FLAG_ADR_CYCLE);

   if (size < 6)
      return MSCB_TIMEOUT;

   /* do CRC check */
   if (crc8(buf, size-1) != buf[size-1])
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

    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int i, size, retry;
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

#ifdef HAVE_MRPC
      if (mrpc_connected(fd))
         return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_INFO_VARIABLE,
                        mscb_fd[fd - 1].remote_fd, adr, index, info);
#endif

      for (retry = 0 ; retry < 2 ; retry++) {
         buf[0] = MCMD_ADDR_NODE16;
         buf[1] = (unsigned char) (adr >> 8);
         buf[2] = (unsigned char) (adr & 0xFF);
         buf[3] = crc8(buf, 3);

         buf[4] = MCMD_GET_INFO + 1;
         buf[5] = index;
         buf[6] = crc8(buf+4, 2);
         size = sizeof(buf);
         mscb_exchg(fd, buf, &size, 7, RS485_FLAG_ADR_CYCLE);

         if (size == (int) sizeof(MSCB_INFO_VAR) + 3 || size == 2)
            break;
      }

      if (size < (int) sizeof(MSCB_INFO_VAR) + 3) {
         if (size == 2) // negative acknowledge
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   unsigned char buf[64];
   int status;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_NODE_ADDR, mscb_fd[fd - 1].remote_fd, 
                       addr, gaddr, broadcast, new_addr);
#endif

   if (addr >= 0) { // individual node

      /* check if destination address is alive */
      if (addr >= 0) {
         status = mscb_ping(fd, new_addr, 0);
         if (status == MSCB_SUCCESS)
            return MSCB_ADDR_EXISTS;
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
      mscb_exchg(fd, buf, NULL, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

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
      mscb_exchg(fd, buf, NULL, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   } else if (broadcast) {
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_SET_ADDR;
      buf[3] = ADDR_SET_HIGH;
      buf[4] = (unsigned char) (new_addr >> 8);
      buf[5] = (unsigned char) (new_addr & 0xFF);
      buf[6] = crc8(buf+2, 4);
      mscb_exchg(fd, buf, NULL, 7, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   }

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   unsigned char buf[64];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_GROUP_ADDR, mscb_fd[fd - 1].remote_fd, 
                       gaddr, new_addr);
#endif

   if (broadcast > 0) {         
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_SET_ADDR;
      buf[3] = ADDR_SET_GROUP;
      buf[4] = (unsigned char) (new_addr >> 8);
      buf[5] = (unsigned char) (new_addr & 0xFF);
      buf[6] = crc8(buf+2, 4);
      mscb_exchg(fd, buf, NULL, 7, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

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
      mscb_exchg(fd, buf, NULL, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

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
      mscb_exchg(fd, buf, NULL, 9, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   }

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   unsigned char buf[256];
   int i;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_NAME, mscb_fd[fd - 1].remote_fd, adr, name);
#endif

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
   mscb_exchg(fd, buf, NULL, 7 + i, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int i;
   unsigned char *d;
   unsigned char buf[256];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE_GROUP,
                       mscb_fd[fd - 1].remote_fd, adr, index, data, size);
#endif

   if (size > 4 || size < 1)
      return MSCB_INVAL_PARAM;

   buf[0] = MCMD_ADDR_GRP16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = (unsigned char)(MCMD_WRITE_NA + size + 1);
   buf[5] = index;

   for (i = 0, d = (unsigned char *)data; i < size; i++)
      buf[6 + size - 1 - i] = *d++;

   buf[6 + i] = crc8(buf, 6 + i);
   mscb_exchg(fd, buf, NULL, 7 + i, RS485_FLAG_ADR_CYCLE | RS485_FLAG_NO_ACK);

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int i, len, retry, status;
   unsigned char buf[256], crc;
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

   if (_debug_flag) {
      if (size == 4)
         debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=%lf/0x%X,size=%d) ", 1, fd, adr, index, f, dw, size);
      else if (size < 4)
         debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=0x%X,size=%d) ", 1, fd, adr, index, dw, size);
      else
         debug_log("mscb_write(fd=%d,adr=%d,index=%d,data=\"%s\",size=%d) ", 1, fd, adr, index, (char *)data, size);
   }

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE, mscb_fd[fd - 1].remote_fd, adr, index, data, size);
#endif

   if (size < 1) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   /* retry several times */
   status = 0;
   for (retry=0 ; retry<mscb_max_retry ; retry++) {

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      if (size < 6) {
         buf[4] = (unsigned char)(MCMD_WRITE_ACK + size + 1);
         buf[5] = index;

         /* reverse order for WORD & DWORD */
         if (size < 5)
            for (i = 0, d = (unsigned char *)data; i < size; i++)
               buf[6 + size - 1 - i] = *d++;
         else
            for (i = 0, d = (unsigned char *)data; i < size; i++)
               buf[6 + i] = *d++;

         crc = crc8(buf+4, 2 + i);
         buf[6 + i] = crc;
         len = sizeof(buf);
         status = mscb_exchg(fd, buf, &len, 7 + i, RS485_FLAG_ADR_CYCLE);
      } else {
         buf[4] = MCMD_WRITE_ACK + 7;
         buf[5] = (unsigned char)(size + 1);
         buf[6] = index;

         for (i = 0, d = (unsigned char *)data; i < size; i++)
            buf[7 + i] = *d++;

         crc = crc8(buf+4, 3 + i);
         buf[7 + i] = crc;
         len = sizeof(buf);
         status = mscb_exchg(fd, buf, &len, 8 + i, RS485_FLAG_ADR_CYCLE);
      }

      if (len == 1) {
#ifndef _USRDLL
         printf("mscb_write: Timeout from RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Timeout from RS485 bus at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         status = MSCB_TIMEOUT;

#ifndef _USRDLL
         printf("mscb_write: Flush node communication at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Flush node communication at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         memset(buf, 0, sizeof(buf));
         mscb_exchg(fd, buf, NULL, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
         Sleep(100);

         continue;
      }

      if (status == MSCB_TIMEOUT) {
         debug_log("return MSCB_TIMEOUT\n", 0);
         status = MSCB_TIMEOUT;
         continue;
      }

      if (buf[0] != MCMD_ACK || buf[1] != crc) {
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int i, len;
   unsigned char buf[256], crc;
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

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE_NO_RETRIES, mscb_fd[fd - 1].remote_fd, adr, index, data, size);
#endif

   if (size < 1) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
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
         for (i = 0, d = (unsigned char *)data; i < size; i++)
            buf[6 + size - 1 - i] = *d++;
      else
         for (i = 0, d = (unsigned char *)data; i < size; i++)
            buf[6 + i] = *d++;

      crc = crc8(buf+4, 2 + i);
      buf[6 + i] = crc;
      len = sizeof(buf);
      mscb_exchg(fd, buf, &len, 7 + i, RS485_FLAG_ADR_CYCLE);
   } else {
      buf[4] = MCMD_WRITE_ACK + 7;
      buf[5] = (unsigned char)(size + 1);
      buf[6] = index;

      for (i = 0, d = (unsigned char *)data; i < size; i++)
         buf[7 + i] = *d++;

      crc = crc8(buf+4, 3 + i);
      buf[7 + i] = crc;
      len = sizeof(buf);
      mscb_exchg(fd, buf, &len, 8 + i, RS485_FLAG_ADR_CYCLE);
   }

   if (len < 2) {
      debug_log("return MSCB_TIMEOUT\n", 0);
      return MSCB_TIMEOUT;
   }

   if (buf[0] != MCMD_ACK || buf[1] != crc) {
      debug_log("return MSCB_CRC_ERROR\n", 0);
      return MSCB_CRC_ERROR;
   }

   debug_log("return MSCB_SUCCESS\n", 0);
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_range(int fd, unsigned short adr, unsigned char index1, unsigned char index2, void *data, int size)
/********************************************************************\

  Routine: mscb_write_range

  Purpose: Write data to multiple indices on node

  Input:
    int fd                  File descriptor for connection
    unsigned short adr      Node address
    unsigned char index1    First variable index 0..255 to write
    unsigned char index2    Last variable index 0..255 to write
    void *data              Data to send
    int size                Data size in bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int i, len, status, retry;
   unsigned char buf[256], crc;

   if (_debug_flag)
      debug_log("mscb_write_range(fd=%d,adr=%d,index1=%d,index2=%d,size=%d) ", 
         1, fd, adr, index1, index2, size);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_WRITE_RANGE, 
                       mscb_fd[fd - 1].remote_fd, adr, index1, index2, data, size);
#endif

   if (size < 1 || size + 10 > (int)sizeof(buf)) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   /* retry several times */
   status = 0;
   for (retry=0 ; retry<mscb_max_retry ; retry++) {

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_WRITE_RANGE | 0x07;

      if (size < 128) {
         buf[5] = size + 2;
         buf[6] = index1;
         buf[7] = index2;

         for (i = 0; i < size; i++)
            buf[8 + i] = ((char *)data)[i];

         crc = crc8(buf+4, 4 + i);
         buf[8 + i] = crc;
         len = sizeof(buf);
         status = mscb_exchg(fd, buf, &len, 9 + i, RS485_FLAG_ADR_CYCLE);
      } else {
         buf[5] = 0x80 | ((size + 2) >> 8);
         buf[6] = (size + 2) & 0xFF;
         buf[7] = index1;
         buf[8] = index2;

         for (i = 0; i < size; i++)
            buf[9 + i] = ((char *)data)[i];

         crc = crc8(buf+4, 5 + i);
         buf[9 + i] = crc;
         len = sizeof(buf);
         status = mscb_exchg(fd, buf, &len, 10 + i, RS485_FLAG_ADR_CYCLE);
      }

      if (len == 1) {
#ifndef _USRDLL
         printf("mscb_write: Timeout from RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Timeout from RS485 bus at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         status = MSCB_TIMEOUT;

#ifndef _USRDLL
         printf("mscb_write: Flush node communication at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Flush node communication at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         memset(buf, 0, sizeof(buf));
         mscb_exchg(fd, buf, NULL, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
         Sleep(100);

         continue;
      }

      if (status == MSCB_TIMEOUT) {
         debug_log("return MSCB_TIMEOUT\n", 0);
         status = MSCB_TIMEOUT;
         continue;
      }

      if (buf[0] != MCMD_ACK || buf[1] != crc) {
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   unsigned char buf[64];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_FLASH, mscb_fd[fd - 1].remote_fd, addr, gaddr, broadcast);
#endif

   if (addr >= 0) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (addr >> 8);
      buf[2] = (unsigned char) (addr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_FLASH;
      buf[5] = crc8(buf+4, 1);
      mscb_exchg(fd, buf, NULL, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (gaddr >= 0) {
      buf[0] = MCMD_ADDR_GRP16;
      buf[1] = (unsigned char) (gaddr >> 8);
      buf[2] = (unsigned char) (gaddr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_FLASH;
      buf[5] = crc8(buf+4, 1);
      mscb_exchg(fd, buf, NULL, 6, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (broadcast) {
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_FLASH;
      buf[3] = crc8(buf+2, 1);
      mscb_exchg(fd, buf, NULL, 4, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   }

   /* wait until flash has finished, otherwise nodes cannot continue
      to receive further commands */
   Sleep(500);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_baud(int fd, int baud)
/********************************************************************\

  Routine: mscb_set_baud

  Purpose: Set baud rate of a whole MSCB bus


  Input:
    int fd                  File descriptor for connection
    int baud                Baud rate:
                              1:    2400
                              2:    4800
                              3:    9600
                              4:   19200
                              5:   28800
                              6:   38400
                              7:   57600
                              8:  115200
                              9:  172800
                             10:  345600

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   unsigned char buf[64];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_BAUD, mscb_fd[fd - 1].remote_fd, baud);
#endif

   buf[0] = MCMD_ADDR_BC;
   buf[1] = crc8(buf, 1);

   buf[2] = MCMD_SET_BAUD;
   buf[3] = baud;
   buf[4] = crc8(buf+2, 2);
   mscb_exchg(fd, buf, NULL, 5, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_upload(int fd, unsigned short adr, unsigned char *buffer, int size, int debug)
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
    MSCB_FORMAT_ERROR       Error in HEX file format

\********************************************************************/
{
   unsigned char buf[256], crc, image[0x10000], *line;
   unsigned int len, ofh, ofl, type, d, buflen;
   int i, status, page, subpage, flash_size, n_page, retry, sretry, protected_page,
      n_page_disp;
   unsigned short ofs;
   int page_cont[128];

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_UPLOAD, mscb_fd[fd - 1].remote_fd, adr, buffer, size);
#endif

   /* check if node alive */
   status = mscb_ping(fd, adr, 0);
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
         sscanf((const char *)line + 1, "%02x%02x%02x%02x", &len, &ofh, &ofl, &type);
         ofs = (unsigned short)((ofh << 8) | ofl);

         for (i = 0; i < (int) len; i++) {
            sscanf((const char *)line + 9 + i * 2, "%02x", &d);
            image[ofs + i] = (unsigned char)d;
         }

         flash_size += len;
         line = (unsigned char *)strchr((char *)line, '\n');
         if (line == NULL)
            break;

         /* skip CR/LF */
         while (*line == '\r' || *line == '\n')
            line++;
      } else
         return MSCB_FORMAT_ERROR;

   } while (*line);

   /* count pages and bytes */
   for (page = 0; page < (int)(sizeof(page_cont)/sizeof(int)) ; page++)
      page_cont[page] = FALSE;

   for (page = n_page = 0; page < (int)(sizeof(page_cont)/sizeof(int)) ; page++) {
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

   /* enter exclusive mode */
   buf[0] = MCMD_FREEZE;
   buf[1] = 1;
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 2, RS485_FLAG_CMD);

   /* wait for acknowledge */
   if (buflen != 1) {
      printf("Error: cannot request exlusive access to submaster\n");
      return MSCB_TIMEOUT;
   }

   if (debug)
      printf("Send upgrade command to remote node\n");

   /* send upgrade command */
   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);
   buf[4] = MCMD_UPGRADE;
   buf[5] = crc8(buf+4, 1);
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 6, RS485_FLAG_LONG_TO | RS485_FLAG_ADR_CYCLE);

   /* wait for acknowledge */
   if (buflen != 3) {
      printf("Error: timeout receiving acknowledge from remote node\n");
      return MSCB_TIMEOUT;
   }

   if (debug)
      printf("Received upgrade acknowlege \"%d\" from remote node\n", buf[1]);

   if (buf[1] == 2)
      return MSCB_SUBADDR;

   if (buf[1] == 3)
      return MSCB_NOTREADY;

   /* let main routine enter upgrade() */
   Sleep(500);

   if (debug)
      printf("Sending echo test to remote node\n");

   /* send echo command */
   buf[0] = UCMD_ECHO;
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 1, RS485_FLAG_LONG_TO);

   /* wait for ready */
   if (buflen != 2) {
      printf("Error: timeout receiving upgrade echo test from remote node\n");

      /* send exit upgrade command, in case node gets to upgrade routine later */
      buf[0] = UCMD_RETURN;
      mscb_exchg(fd, buf, NULL, 1, RS485_FLAG_NO_ACK);

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

      for (retry = 0 ; retry < mscb_max_retry ; retry++) {

         if (debug)
            printf("Erase page   0x%04X - ", page * 512);
         fflush(stdout);

         /* erase page */
         buf[0] = UCMD_ERASE;
         buf[1] = (unsigned char)page;
         buflen = sizeof(buf);
         mscb_exchg(fd, buf, (int *)&buflen, 2, RS485_FLAG_LONG_TO);

         if (buflen != 2) {
            printf("\nError: timeout from remote node for erase page 0x%04X\n", page * 512);
            continue;
         }

         if (buf[1] == 0xFF) {
            /* protected page, so finish */
            if (debug)
               printf("found protected page, exit\n");
            else
               printf("]      ");
            fflush(stdout);
            protected_page = page;
            goto prog_pages;
         }

         if (buf[0] != MCMD_ACK) {
            printf("\nError: received wrong acknowledge for erase page 0x%04X\n", page * 512);
            continue;
         }

         if (debug)
            printf("ok\n");

         break;
      }

      /* check retries */
      if (retry == mscb_max_retry) {
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

      for (retry = 0 ; retry < mscb_max_retry ; retry++) {

         if (debug)
            printf("Program page 0x%04X - ", page * 512);
         fflush(stdout);

         /* chop down page in 32 byte segments (->USB) */
         for (subpage = 0; subpage < 16; subpage++) {

            for (sretry = 0 ; sretry < mscb_max_retry ; sretry++) {
               /* program page */
               buf[0] = UCMD_PROGRAM;
               buf[1] = (unsigned char)page;
               buf[2] = (unsigned char)subpage;

               /* extract page from image */
               for (i = 0; i < 32; i++)
                  buf[i+3] = image[page * 512 + subpage * 32 + i];

               buflen = sizeof(buf);
               mscb_exchg(fd, buf, (int *)&buflen, 32+3, RS485_FLAG_LONG_TO);

               /* read acknowledge */
               if (buflen != 2 || buf[0] != MCMD_ACK) {
                  printf("\nError: timeout from remote node for program page 0x%04X, chunk %d\n", page * 512, i);
                  goto prog_error;
               } else
                  break; // successful
            }

            /* check retries */
            if (sretry == mscb_max_retry) {
               printf("\nToo many retries, aborting\n");
               goto prog_error;
               break;
            }
         }

         /* test if successful completion */
         if (buf[0] != MCMD_ACK)
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
         buflen = sizeof(buf);
         mscb_exchg(fd, buf, (int *)&buflen, 2, RS485_FLAG_LONG_TO);

         if (buflen != 2) {
            printf("\nError: timeout from remote node for verify page 0x%04X\n", page * 512);
            goto prog_error;
         }

         /* compare CRCs */
         if (buf[1] == crc) {
            if (debug)
               printf("ok (CRC = 0x%02X)\n", crc);
            break;
         }

         if (debug)
            printf("CRC error (0x%02X != 0x%02X)\n", crc, buf[1]);

      }

      /* check retries */
      if (retry == mscb_max_retry) {
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
   mscb_exchg(fd, buf, NULL, 1, RS485_FLAG_NO_ACK);

   if (debug)
      printf("Reboot node\n");

   mscb_clear_info_cache(fd);

prog_error:

   /* exit exclusive mode */
   buf[0] = MCMD_FREEZE;
   buf[1] = 0;
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 2, RS485_FLAG_CMD);

   /* wait for acknowledge */
   if (buflen != 1)
      printf("Error: cannot exit exlusive access at submaster\n");

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_verify(int fd, unsigned short adr, unsigned char *buffer, int size)
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
    MSCB_FORMAT_ERROR       Error in HEX file format

\********************************************************************/
{
   unsigned char buf[64], crc, image[0x10000], *line;
   unsigned int len, ofh, ofl, type, d, buflen;
   int i, j, retry, n_error, status, page, subpage, flash_size;
   unsigned short ofs;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_UPLOAD, mscb_fd[fd - 1].remote_fd, adr, buffer, size);
#endif

   /* check if node alive */
   status = mscb_ping(fd, adr, 0);
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
         sscanf((char *)line + 1, "%02x%02x%02x%02x", &len, &ofh, &ofl, &type);
         ofs = (unsigned short)((ofh << 8) | ofl);

         for (i = 0; i < (int) len; i++) {
            sscanf((char *)line + 9 + i * 2, "%02x", &d);
            image[ofs + i] = (unsigned char)d;
         }

         flash_size += len;
         line = (unsigned char *)strchr((char *)line, '\r') + 1;
         if (line && *line == '\n')
            line++;
      } else
         return MSCB_FORMAT_ERROR;

   } while (*line);

   /* enter exclusive mode */
   buf[0] = MCMD_FREEZE;
   buf[1] = 1;
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 2, RS485_FLAG_CMD);

   /* wait for acknowledge */
   if (buflen != 1) {
      printf("Error: cannot request exlusive access to submaster\n");
      return MSCB_TIMEOUT;
   }

   /* send upgrade command */
   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);
   buf[4] = MCMD_UPGRADE;
   buf[5] = crc8(buf+4, 1);
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 6, RS485_FLAG_LONG_TO | RS485_FLAG_ADR_CYCLE);

   /* wait for acknowledge */
   if (buflen != 3) {
      printf("Error: timeout receiving acknowledge from remote node\n");
      return MSCB_TIMEOUT;
   }

   /* let main routine enter upgrade() */
   Sleep(500);

   /* send echo command */
   buf[0] = UCMD_ECHO;
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 1, RS485_FLAG_LONG_TO);

   /* wait for ready, 1 sec timeout */
   if (buflen != 2) {
      printf("Error: timeout receiving upgrade acknowledge from remote node\n");

      /* send exit upgrade command, in case node gets to upgrade routine later */
      buf[0] = UCMD_RETURN;
      mscb_exchg(fd, buf, NULL, 1, RS485_FLAG_NO_ACK);

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
         for (retry = 0 ; retry < 5 ; retry++) {
            buf[0] = UCMD_READ;
            buf[1] = (unsigned char)page;
            buf[2] = (unsigned char)subpage;
            buflen = sizeof(buf);
            status = mscb_exchg(fd, buf, (int *)&buflen, 3, RS485_FLAG_LONG_TO);
            if (status != MSCB_SUCCESS || buflen != 32+3) {
               if (retry == 4) {
                 printf("\nError: timeout from remote node for verify page 0x%04X\n", page * 512);
                 goto ver_error;
               }
            } else
               break;
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
   mscb_exchg(fd, buf, NULL, 1, RS485_FLAG_NO_ACK);

   /* exit exclusive mode */
   buf[0] = MCMD_FREEZE;
   buf[1] = 0;
   buflen = sizeof(buf);
   mscb_exchg(fd, buf, (int *)&buflen, 2, RS485_FLAG_CMD);

   printf("Verify finished\n");

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
    MSCB_INVALID_INDEX      index parameter too large

\********************************************************************/
{
   int i, j, len, n, status;
   unsigned char buf[256], crc;
   char str[1000];

   if (_debug_flag)
      debug_log("mscb_read(fd=%d,adr=%d,index=%d,size=%d) ", 1, fd, adr, index, *size);

   if (*size > 256)
      return MSCB_INVAL_PARAM;

   memset(data, 0, *size);
   status = 0;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_READ, mscb_fd[fd - 1].remote_fd, adr, index, data, size);
#endif

   /* try ten times */
   for (n = i = 0; n < mscb_max_retry ; n++) {

      if (n > 0)
         debug_log("mscb_read: retry %d\n", n);

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_READ + 1;
      buf[5] = index;
      buf[6] = crc8(buf+4, 2);
      len = sizeof(buf);
      status = mscb_exchg(fd, buf, &len, 7, RS485_FLAG_ADR_CYCLE);

      if (status == MSCB_TIMEOUT) {
#ifndef _USRDLL
         /* show error, but continue repeating */
         printf("mscb_read: Timeout writing to submaster %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Timeout writing to submaster %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         continue;
      }
      if (status == MSCB_SUBM_ERROR) {
#ifndef _USRDLL
         /* show error, but continue repeating */
         printf("mscb_read: Connection to submaster %s:%d broken\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Connection to submaster %s:%d broken\n", 1, mscb_fd[fd-1].device, adr);
         break;
      }

      if (len == 1 && buf[0] == MCMD_ACK) {
         /* variable has been deleted on node, so refresh cache */
         if (n > 5) {
            mscb_clear_info_cache(fd);
            return MSCB_INVALID_INDEX;
         }
         continue;
      }

      if (len == 1) {
#ifndef _USRDLL
         printf("mscb_read: Timeout from RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Timeout from RS485 bus at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         status = MSCB_TIMEOUT;

#ifndef _USRDLL
         printf("mscb_read: Flush node communication at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Flush node communication at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         memset(buf, 0, sizeof(buf));
         mscb_exchg(fd, buf, NULL, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
         Sleep(100);

         continue;
      }

      if (len < 2) {
#ifndef _USRDLL
         /* show error, but repeat request */
         printf("mscb_read: Timeout reading from submaster %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         status = MSCB_TIMEOUT;
         continue;
      }

      crc = crc8(buf, len - 1);

      if (buf[0] != MCMD_ACK + len - 2 && buf[0] != MCMD_ACK + 7) {
         status = MSCB_FORMAT_ERROR;
#ifndef _USRDLL
         /* show error, but repeat */
         printf("mscb_read: Read error on RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         continue;
      }

      if (buf[len - 1] != crc) {
         status = MSCB_CRC_ERROR;
#ifndef _USRDLL
         /* show error, but repeat */
         printf("mscb_read: CRC error on RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         continue;
      }

      if (buf[0] == MCMD_ACK + 7) {
         if (len - 3 > *size) {
            *size = 0;
            debug_log("return MSCB_NO_MEM, len=%d, *size=%d\n", 0, len, *size);
            return MSCB_NO_MEM;
         }

         memcpy(data, buf + 2, len - 3);  // variable length
         *size = len - 3;
      } else {
         if (len - 2 > *size) {
            *size = 0;
            debug_log("return MSCB_NO_MEM, len=%d, *size=%d\n", 0, len, *size);
            return MSCB_NO_MEM;
         }

         memcpy(data, buf + 1, len - 2);
         *size = len - 2;
      }

      if (len - 2 == 2)
         WORD_SWAP(data);
      if (len - 2 == 4)
         DWORD_SWAP(data);

      if (_debug_flag) {
         sprintf((char *)str, "return %d bytes: ", *size);
         for (j=0 ; j<*size ; j++) {
            sprintf((char *)str+strlen(str), "0x%02X ", 
               *(((unsigned char *)data)+j));
            if (isalnum(*(((unsigned char *)data)+j)))
               sprintf(str+strlen(str), "('%c') ", 
                  *(((unsigned char *)data)+j));
         }
         strlcat(str, "\n", sizeof(str)); 
         debug_log(str, 0);
      }

      return MSCB_SUCCESS;
   }

   debug_log("\n", 0);
   if (status == MSCB_TIMEOUT)
     debug_log("return MSCB_TIMEOUT\n", 1);
   if (status == MSCB_CRC_ERROR)
      debug_log("return MSCB_CRC_ERROR\n", 1);
   if (status == MSCB_FORMAT_ERROR)
      debug_log("return MSCB_FORMAT_ERROR\n", 1);
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int j, len, status;
   char str[1000];
   unsigned char buf[256], crc;

   if (_debug_flag)
      debug_log("mscb_read_no_retries(fd=%d,adr=%d,index=%d,size=%d) ", 1, fd, adr, index, *size);

   if (*size > 256)
      return MSCB_INVAL_PARAM;

   memset(data, 0, *size);
   status = 0;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_READ_NO_RETRIES, mscb_fd[fd - 1].remote_fd, adr, index, data, size);
#endif

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = MCMD_READ + 1;
   buf[5] = index;
   buf[6] = crc8(buf+4, 2);
   len = sizeof(buf);
   status = mscb_exchg(fd, buf, &len, 7, RS485_FLAG_ADR_CYCLE);
   if (status == MSCB_TIMEOUT) {
#ifndef _USRDLL
      /* show error, but continue repeating */
      printf("mscb_read_no_retries: Timeout writing to submaster %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
      return MSCB_TIMEOUT;
   }

   if (len == 1) {
#ifndef _USRDLL
      printf("mscb_read_no_retries: Timeout from RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
      memset(buf, 0, sizeof(buf));
      mscb_exchg(fd, buf, NULL, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
      return MSCB_TIMEOUT;
   }

   if (len < 2) {
#ifndef _USRDLL
      /* show error, but repeat request */
      printf("mscb_read_no_retries: Timeout reading from submaster %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
      return MSCB_TIMEOUT;
   }

   crc = crc8(buf, len - 1);

   if ((buf[0] != MCMD_ACK + len - 2 && buf[0] != MCMD_ACK + 7)
      || buf[len - 1] != crc) {
#ifndef _USRDLL
      /* show error, but continue repeating */
         printf("mscb_read_no_retries: CRC error on RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
      return MSCB_CRC_ERROR;
   }

   if (buf[0] == MCMD_ACK + 7) {
      if (len - 3 > *size) {
         *size = 0;
         debug_log("return MSCB_NO_MEM, len=%d, *size=%d\n", 0, len, *size);
         return MSCB_NO_MEM;
      }

      memcpy(data, buf + 2, len - 3);  // variable length
      *size = len - 3;
   } else {
      if (len - 2 > *size) {
         *size = 0;
         debug_log("return MSCB_NO_MEM, len=%d, *size=%d\n", 0, len, *size);
         return MSCB_NO_MEM;
      }

      memcpy(data, buf + 1, len - 2);
      *size = len - 2;
   }

   if (len - 2 == 2)
      WORD_SWAP(data);
   if (len - 2 == 4)
      DWORD_SWAP(data);

   if (_debug_flag) {
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
   }

   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read_range(int fd, unsigned short adr, unsigned char index1, unsigned char index2, void *data, int *size)
/********************************************************************\

  Routine: mscb_read_range

  Purpose: Read data from multiple channels on node

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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb

\********************************************************************/
{
   int i, j, n, len, status;
   char str[1000];
   unsigned char buf[256], crc;

   if (_debug_flag)
      debug_log("mscb_read_range(fd=%d,adr=%d,index1=%d,index2=%d,size=%d) ", 
         1, fd, adr, index1, index2, *size);

   if (*size > 256)
      return MSCB_INVAL_PARAM;

   memset(data, 0, *size);
   status = 0;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_READ_RANGE,
                       mscb_fd[fd - 1].remote_fd, adr, index1, index2, data, size);
#endif

   /* retry several times as specified with mscb_max_retry */
   for (n = i = 0; n < mscb_max_retry; n++) {

      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_READ + 2;
      buf[5] = index1;
      buf[6] = index2;
      buf[7] = crc8(buf+4, 3);
      len = sizeof(buf);
      status = mscb_exchg(fd, buf, &len, 8, RS485_FLAG_ADR_CYCLE);
      if (status == MSCB_TIMEOUT) {
#ifndef _USRDLL
         /* show error, but continue with retry */
         printf("mscb_read_range: Timeout %d out of %d writing to submaster %s:%d\n", 
            n, mscb_max_retry, mscb_fd[fd-1].device, adr);
#endif
         debug_log("Timeout writing to submaster %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         continue;
      }
      if (status == MSCB_SUBM_ERROR) {
#ifndef _USRDLL
         /* show error, but continue with retry */
         printf("mscb_read_range: Connection to submaster %s:%d broken\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Connection to submaster %s:%d broken\n", 1, mscb_fd[fd-1].device, adr);
         break;
      }

      if (len == 1 && buf[0] == MCMD_ACK) {
         /* variable has been deleted on node, so refresh cache */ 
         if (n > 5) {
            mscb_clear_info_cache(fd);
            return MSCB_INVALID_INDEX;
         }
         continue;
      }

      if (len == 1) {
#ifndef _USRDLL
         printf("mscb_read_range: Timeout from RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Timeout from RS485 bus at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         status = MSCB_TIMEOUT;

#ifndef _USRDLL
         printf("mscb_read_range: Flush node communication at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         debug_log("Flush node communication at %s:%d\n", 1, mscb_fd[fd-1].device, adr);
         memset(buf, 0, sizeof(buf));
         mscb_exchg(fd, buf, NULL, 10, RS485_FLAG_BIT9 | RS485_FLAG_NO_ACK);
         Sleep(100);

         continue;
      }

      if (len < 2) {
#ifndef _USRDLL
         /* show error, but repeat request */
         printf("mscb_read_range: Timeout reading from submaster %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         status = MSCB_TIMEOUT;
         continue;
      }

      crc = crc8(buf, len - 1);

      if (buf[0] != MCMD_ACK + len - 2 && buf[0] != MCMD_ACK + 7) {
         status = MSCB_FORMAT_ERROR;
#ifndef _USRDLL
         /* show error, but repeat */
         printf("mscb_read_range: Read error on RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         continue;
      }

      if (buf[len - 1] != crc) {
         status = MSCB_CRC_ERROR;
#ifndef _USRDLL
         /* show error, but repeat */
         printf("mscb_read_range: CRC error on RS485 bus at %s:%d\n", mscb_fd[fd-1].device, adr);
#endif
         continue;
      }

      if (buf[0] == MCMD_ACK + 7) {
         if (len - 3 > *size) {
            *size = 0;
            debug_log("return MSCB_NO_MEM, len=%d, *size=%d\n", 0, len, *size);
            return MSCB_NO_MEM;
         }

         if (buf[1] & 0x80) {
            memcpy(data, buf + 3, len - 4);  // variable length with size in two byte
            *size = len - 4;
         } else {
            memcpy(data, buf + 2, len - 3);  // variable length with size in one byte
            *size = len - 3;
         }
      } else {
         if (len - 2 > *size) {
            *size = 0;
            debug_log("return MSCB_NO_MEM, len=%d, *size=%d\n", 0, len, *size);
            return MSCB_NO_MEM;
         }

         memcpy(data, buf + 1, len - 2);
         *size = len - 2;
      }

      if (_debug_flag) {
         sprintf(str, "return %d bytes: ", *size);
         for (j=0 ; j<*size ; j++) {
            if (strlen(str) > sizeof(str)-20) {
               strlcat(str, "...", sizeof(str));
               break;
            }
            sprintf(str+strlen(str), "0x%02X ", 
               *(((unsigned char *)data)+j));
            if (isalnum(*(((unsigned char *)data)+j)))
               sprintf(str+strlen(str), "('%c') ", 
                  *(((unsigned char *)data)+j));
         }
         strlcat(str, "\n", sizeof(str)); 
         debug_log(str, 0);
      }

      return MSCB_SUCCESS;
   }

   debug_log("\n", 0);
   if (status == MSCB_TIMEOUT)
     debug_log("return MSCB_TIMEOUT\n", 1);
   if (status == MSCB_CRC_ERROR)
      debug_log("return MSCB_CRC_ERROR\n", 1);
   if (status == MSCB_FORMAT_ERROR)
      debug_log("return MSCB_FORMAT_ERROR\n", 1);
   if (status == MSCB_SUBM_ERROR)
      debug_log("return MSCB_SUBM_ERROR\n", 1);

   return status;
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
    MSCB_FORMAT_ERROR       "size" parameter too large

\********************************************************************/
{
   int i, len, status;
   unsigned char buf[80];

   memset(result, 0, *rsize);

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

   if (size > 4 || size < 0) {
      return MSCB_FORMAT_ERROR;
   }

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_USER,
                       mscb_fd[fd - 1].remote_fd, adr, param, size, result, rsize);
#endif

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = (unsigned char)(MCMD_USER + size);

   for (i = 0; i < size; i++)
      buf[5 + i] = ((char *) param)[i];

   /* add CRC code and send data */
   buf[5 + i] = crc8(buf+4, 1 + i);
   len = sizeof(buf);
   status = mscb_exchg(fd, buf, &len, 6 + i, RS485_FLAG_ADR_CYCLE);

   if (status != MSCB_SUCCESS)
      return status;

   if (result == NULL)
      return MSCB_SUCCESS;

   if (len < 2)
      return MSCB_TIMEOUT;

   if (rsize)
      *rsize = len - 2;
   for (i = 0; i < len - 2; i++)
      ((char *) result)[i] = buf[1 + i];

   if (buf[len - 1] != crc8(buf, len - 1))
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
    MSCB_FORMAT_ERROR       "size" parameter too large

\********************************************************************/
{
   int len, status;
   unsigned char buf[64];

   *d2 = 0xFF;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_ECHO, mscb_fd[fd - 1].remote_fd, adr, d1, d2);
#endif

   buf[0] = MCMD_ADDR_NODE16;
   buf[1] = (unsigned char) (adr >> 8);
   buf[2] = (unsigned char) (adr & 0xFF);
   buf[3] = crc8(buf, 3);

   buf[4] = MCMD_ECHO;
   buf[5] = d1;

   /* add CRC code and send data */
   buf[6] = crc8(buf+4, 2);
   len = sizeof(buf);
   status = mscb_exchg(fd, buf, &len, 7, RS485_FLAG_ADR_CYCLE);
   if (status != MSCB_SUCCESS)
      return status;

   *d2 = buf[1];

   if (len == 1 && buf[0] == 0xFF)
      return MSCB_TIMEOUT_BUS;   // timeout RS485 bus

   if (buf[len - 1] != crc8(buf, len - 1))
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
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
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
    EMSCB_LOCKED            Submaster is locked
    0                       No submaster found

\********************************************************************/
{
#ifdef HAVE_USB
   char list[10][256], str[256];
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
      mscb_fd[index].semaphore_handle = mscb_semaphore_create(str);

      if (!mscb_lock(index + 1)) {
         printf("lock failed\n");
         debug_log("return EMSCB_LOCKED\n", 0);
         return EMSCB_LOCKED;
      }

      if (subm250_check(ui) == MSCB_SUCCESS)
         sprintf(list[n++], "usb%d", found++);

      mscb_release(index + 1);

      musb_close(ui);
   }

   /* if only one device, return it */
   if (n == 1 || select == 0) {
      strlcpy(device, list[0], size);
      return MSCB_SUCCESS;
   }

   if (n == 0)
      return 0;

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
#else // HAVE_USB
   return 0;
#endif
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

   if (n < 500) {
      /* PSI MAC pool */    
      cfg.eth_mac_addr[0] = 0x00;
      cfg.eth_mac_addr[1] = 0x50;
      cfg.eth_mac_addr[2] = 0xC2;
      cfg.eth_mac_addr[3] = 0x46;
      cfg.eth_mac_addr[4] = (unsigned char)(0xD0 | (n >> 8));
      cfg.eth_mac_addr[5] = (unsigned char)(n & 0xFF);

      printf("MAC Address is 00-50-C2-46-%02X-%02X\n",
             cfg.eth_mac_addr[4], cfg.eth_mac_addr[5]);
   } else {
      /* TRIUMF MAC pool */    
      cfg.eth_mac_addr[0] = 0x00;
      cfg.eth_mac_addr[1] = 0x50;
      cfg.eth_mac_addr[2] = 0xC2;
      cfg.eth_mac_addr[3] = 0x6B;
      cfg.eth_mac_addr[4] = (unsigned char)(0x50 | ((n-500) >> 8));
      cfg.eth_mac_addr[5] = (unsigned char)((n-500) & 0xFF);

      printf("MAC Address is 00-50-C2-6B-%02X-%02X\n",
           cfg.eth_mac_addr[4], cfg.eth_mac_addr[5]);
   }

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
   n = sizeof(buf);
   mscb_exchg(fd, buf, &n, 1+sizeof(cfg), RS485_FLAG_CMD);
   if (n == 2 && buf[0] == MCMD_ACK) {
      printf("\nConfiguration successfully downloaded.\n");
      return MSCB_SUCCESS;
   }

   printf("Error downloading configuration.\n");

   return 0;
}

/*------------------------------------------------------------------*/

void mscb_scan_udp()
/********************************************************************\

  Routine: mscb_scan_udp

  Purpose: Scan network for running ethernet submasters

\********************************************************************/
{
   char str[256];
   unsigned char buf[256];
   int i, n, rev, uptime;
   struct hostent *phe;
   struct sockaddr_in *psa_in;

#ifdef _MSC_VER
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return;
   }
#endif

   mscb_fd[0].type = MSCB_TYPE_ETH; 
   mscb_fd[0].semaphore_handle = mscb_semaphore_create("mscb");
   mscb_fd[0].eth_max_retry = 5;
   mscb_fd[0].fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (mscb_fd[0].fd == -1) {
      printf("cannot create socket\n");
      return;
   }

   for (i=0 ; i<1000 ; i++) {
      if (kbhit())
         break;

      sprintf(str, "MSCB%03d", i);
      printf("Checking %s...\r", str);
      fflush(stdout);

      /* retrieve destination address */
      phe = gethostbyname(str);
      if (phe == NULL)
         continue;

      memset(&mscb_fd[0].eth_addr, 0, sizeof(mscb_fd[0].eth_addr));
      psa_in = (struct sockaddr_in *)mscb_fd[0].eth_addr;
      memcpy((char *) &(psa_in->sin_addr), phe->h_addr, phe->h_length);
      psa_in->sin_port = htons((short) MSCB_NET_PORT);
      psa_in->sin_family = AF_INET;

      buf[0] = MCMD_ECHO;
      n = sizeof(buf);
      mscb_exchg(1, buf, &n, 1, RS485_FLAG_CMD | RS485_FLAG_NO_RETRY);

      if (n >= 4) {
         rev = (buf[2] << 8) + buf[3];
         printf("Found %s, PV %d, SVN rev. %d", str, buf[1], rev);
         if (n >= 8) {
            uptime = (buf[7]<<0) + (buf[6]<<8) + (buf[5]<<16) + (buf[4]<<24);
            printf(", UT %dd %02dh %02dm %02ds",
                   uptime / (3600 * 24),
                   (uptime % (3600 * 24)) / 3600, (uptime % 3600) / 60,
                   (uptime % 60));
         }
         printf("\n");
      }
   }

   printf("                    \n");
   while (kbhit())
      getch();
}

/*------------------------------------------------------------------*/

int mscb_subm_info(int fd)
/********************************************************************\

  Routine: mscb_subm_info

  Purpose: Show info for submaster

\********************************************************************/
{
   unsigned char buf[10];
   int n, rev, uptime;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type) {
      debug_log("return MSCB_INVAL_PARAM\n", 0);
      return MSCB_INVAL_PARAM;
   }

   if (mscb_fd[fd - 1].type == MSCB_TYPE_ETH) {
      int status;

      buf[0] = MCMD_ECHO;
      n = sizeof(buf);
      status = mscb_exchg(1, buf, &n, 1, RS485_FLAG_CMD | RS485_FLAG_NO_RETRY);

      if (status != MSCB_SUCCESS)
         return status;

      if (n >= 4) {
         rev = (buf[2] << 8) + buf[3];
         printf("Submaster        : %s\n", mscb_fd[fd-1].device);
         printf("Address          : %s\n", inet_ntoa(((struct sockaddr_in *)mscb_fd[fd-1].eth_addr)->sin_addr));
         printf("Protocol version : %d\n", buf[1]);
         printf("SVN revision     : %d\n", rev);

         if (n == 8) {
            uptime = (buf[7]<<0) + (buf[6]<<8) + (buf[5]<<16) + (buf[4]<<24);
            printf("Uptime           : %dd %02dh %02dm %02ds\n",
                   uptime / (3600 * 24),
                   (uptime % (3600 * 24)) / 3600, (uptime % 3600) / 60,
                   (uptime % 60));
         }
      }

   } else if (mscb_fd[fd - 1].type == MSCB_TYPE_USB) {
      printf("This function is currently not implemented for USB submasters\n");
   }

   debug_log("return MSCB_SUCCESS\n", 0);
   return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_time(int fd, int addr, int gaddr, int broadcast)
/********************************************************************\

  Routine: mscb_set_time

  Purpose: Set time of node equal to local time

  Input:
    int fd                  File descriptor for connection
    int addr                Node address
    int gaddr               Group address
    int broadcast           Broadcast flag

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_SEMAPHORE          Cannot obtain semaphore for mscb
    MSCB_TIMEOUT            Timeout receiving ping acknowledge

\********************************************************************/
{
   int i, size, status;
   unsigned char buf[64], dt[10];
   struct tm *ptm;
   time_t now;

   if (fd > MSCB_MAX_FD || fd < 1 || !mscb_fd[fd - 1].type)
      return MSCB_INVAL_PARAM;

#ifdef HAVE_MRPC
   if (mrpc_connected(fd))
      return mrpc_call(mscb_fd[fd - 1].fd, RPC_MSCB_SET_TIME, mscb_fd[fd - 1].remote_fd, addr, gaddr, broadcast);
#endif

   tzset();
   now = time(NULL);
   ptm = localtime(&now);
   dt[0] = (ptm->tm_mday       / 10) * 0x10 + (ptm->tm_mday       % 10);
   dt[1] = ((ptm->tm_mon+1)    / 10) * 0x10 + ((ptm->tm_mon+1)    % 10);
   dt[2] = ((ptm->tm_year-100) / 10) * 0x10 + ((ptm->tm_year-100) % 10);
   dt[3] = (ptm->tm_hour       / 10) * 0x10 + (ptm->tm_hour       % 10);
   dt[4] = (ptm->tm_min        / 10) * 0x10 + (ptm->tm_min        % 10);
   dt[5] = (ptm->tm_sec        / 10) * 0x10 + (ptm->tm_sec        % 10);
   
   size = sizeof(buf);
   if (addr >= 0) {
      buf[0] = MCMD_ADDR_NODE16;
      buf[1] = (unsigned char) (addr >> 8);
      buf[2] = (unsigned char) (addr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_SET_TIME;
      for (i=0 ; i<6 ; i++)
         buf[5+i] = dt[i];
      buf[11] = crc8(buf+4, 7);
      status = mscb_exchg(fd, buf, &size, 12, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (gaddr >= 0) {
      buf[0] = MCMD_ADDR_GRP16;
      buf[1] = (unsigned char) (gaddr >> 8);
      buf[2] = (unsigned char) (gaddr & 0xFF);
      buf[3] = crc8(buf, 3);

      buf[4] = MCMD_SET_TIME;
      for (i=0 ; i<6 ; i++)
         buf[5+i] = dt[i];
      buf[11] = crc8(buf+4, 7);
      status = mscb_exchg(fd, buf, &size, 12, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else if (broadcast) {
      buf[0] = MCMD_ADDR_BC;
      buf[1] = crc8(buf, 1);

      buf[2] = MCMD_SET_TIME;
      for (i=0 ; i<6 ; i++)
         buf[3+i] = dt[i];
      buf[9] = crc8(buf+2, 7);
      status = mscb_exchg(fd, buf, &size, 10, RS485_FLAG_NO_ACK | RS485_FLAG_ADR_CYCLE);
   } else
      status = MSCB_INVAL_PARAM;

   return status;
}

/*------------------------------------------------------------------*/

/********************************************************************\

  Name:         mscb.c
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus communication functions

  $Log$
  Revision 1.30  2003/03/06 16:08:50  midas
  Protocol version 1.3 (change node name)

  Revision 1.29  2003/03/03 15:58:50  midas
  V1.2.1, fixed communication problem with slow ADuC

  Revision 1.28  2003/02/27 10:44:05  midas
  Two-cygle communication with submaster to avoid data collision

  Revision 1.27  2003/01/27 16:11:14  midas
  Switched to version 1.1.1

  Revision 1.26  2003/01/27 16:10:09  midas
  Keep port in input mode by default (not to collide with submaster)

  Revision 1.25  2002/11/29 09:01:50  midas
  Added <unistd.h>

  Revision 1.24  2002/11/28 14:44:09  midas
  Removed SIZE_XBIT

  Revision 1.23  2002/11/28 13:04:36  midas
  Implemented protocol version 1.2 (echo, read_channels)

  Revision 1.22  2002/11/28 10:32:09  midas
  Implemented locking under linux

  Revision 1.21  2002/11/27 15:40:05  midas
  Added version, fixed few bugs

  Revision 1.20  2002/11/20 12:01:26  midas
  Added host to mscb_init

  Revision 1.19  2002/11/06 16:46:11  midas
  Check parameter size

  Revision 1.18  2002/11/06 14:01:20  midas
  Fixed small bugs

  Revision 1.17  2002/10/28 14:26:30  midas
  Changes from Japan

  Revision 1.16  2002/10/16 15:25:06  midas
  xxx16 now does 32 bit exchange

  Revision 1.15  2002/10/09 15:48:13  midas
  Fixed bug with download

  Revision 1.14  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.13  2002/10/07 15:16:32  midas
  Added upgrade facility

  Revision 1.12  2002/10/03 15:30:40  midas
  Added linux support

  Revision 1.11  2002/09/27 11:10:51  midas
  Changed buffer type

  Revision 1.10  2002/09/10 13:13:21  midas
  Remove strobe on timeout

  Revision 1.9  2002/08/12 12:10:55  midas
  Added error handling (zero padding)

  Revision 1.8  2002/07/10 09:51:18  midas
  Introduced mscb_flash()

  Revision 1.7  2001/11/05 11:47:04  midas
  Fixed bug to run msc under W95

  Revision 1.6  2001/10/31 11:16:54  midas
  Added IO check function

  Revision 1.5  2001/09/04 07:33:42  midas
  Rewrote write16/read16 functions

  Revision 1.4  2001/08/31 12:23:50  midas
  Added mutex protection

  Revision 1.3  2001/08/31 11:35:20  midas
  Added "wp" command in msc.c, changed parport to device in mscb.c

  Revision 1.2  2001/08/31 11:04:39  midas
  Added write16 and read16 (for LabView)


\********************************************************************/

#define MSCB_LIBRARY_VERSION   "1.3.0"
#define MSCB_PROTOCOL_VERSION  "1.3"

#ifdef _MSC_VER           // Windows includes

#include <windows.h>
#include <conio.h>

#elif defined(__linux__)  // Linux includes

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

#endif

#include <stdio.h>
#include "mscb.h"
#include "rpc.h"

/*------------------------------------------------------------------*/

/* file descriptor */

#define MSCB_MAX_FD 10

typedef struct {
  char device[256];
  int  fd;
} MSCB_FD;

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

   2  Yes   !ALF --- 14 ---- RESET   ->    (HW RESET)
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

#define TIMEOUT_OUT      1000    /* ~1ms */

/*------------------------------------------------------------------*/

/* missing linux functions */

#ifdef __linux__

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
  for (i=0 ; i<len ; i++)
    {
    index = data[i] ^ crc8_code;
    crc8_code = crc8_data[index];
    }

  return crc8_code;
}

/*------------------------------------------------------------------*/

#ifdef _MSC_VER
static HANDLE mutex_handle = 0;
#endif

int mscb_lock(int fd)
{
#ifdef _MSC_VER
int status;

  if (mutex_handle == 0)
    mutex_handle = CreateMutex(NULL, FALSE, "mscb");

  if (mutex_handle == 0)
    return 0;

  status = WaitForSingleObject(mutex_handle, 1000);

  if (status == WAIT_FAILED)
    return 0;
  if (status == WAIT_TIMEOUT)
    return 0;

#elif defined(__linux__)

  if (ioctl(mscb_fd[fd-1].fd, PPCLAIM))
    return 0;

#endif
  return MSCB_SUCCESS;
}

int mscb_release(int fd)
{
#ifdef _MSC_VER
int status;

  status = ReleaseMutex(mutex_handle);
  if (status == FALSE)
    return 0;

#elif defined(__linux__)

  if (ioctl(mscb_fd[fd-1].fd, PPRELEASE))
    return 0;

#endif
  return MSCB_SUCCESS;
}

/*---- low level functions for direct port access ------------------*/

void pp_wcontrol(int fd, int signal, int flag)
/* write control signal */
{
static unsigned int mask=0;

  switch (signal)
    {
    case LPT_STROBE: // negative
       mask = flag ? mask | (1<<0) : mask & ~(1<<0);
       break;
    case LPT_RESET:  // negative
       mask = flag ? mask | (1<<1) : mask & ~(1<<1);
       break;
    case LPT_BIT9:   // positive
       mask = flag ? mask | (1<<2) : mask & ~(1<<2);
       break;
    case LPT_ACK:    // negative
       mask = flag ? mask | (1<<3) : mask & ~(1<<3);
       break;
    case LPT_READMODE: // positive
       mask = flag ? mask | (1<<5) : mask & ~(1<<5);
       break;
    }

#ifdef _MSC_VER
  _outp((unsigned short)(mscb_fd[fd-1].fd+2), mask);
#else
  ioctl(mscb_fd[fd-1].fd, PPWCONTROL, &mask);
#endif
}

/*------------------------------------------------------------------*/

int pp_rstatus(int fd, int signal)
{
/* read status signal */
unsigned int mask = 0;

#ifdef _MSC_VER
  mask = _inp((unsigned short) (mscb_fd[fd-1].fd+1));
#else
  ioctl(mscb_fd[fd-1].fd, PPRSTATUS, &mask);
#endif

  switch (signal)
    {
    case LPT_BUSY:       return (mask & (1<<7)) == 0;
    case LPT_DATAREADY:  return (mask & (1<<6)) == 0;
    }

  return 0;
}

/*------------------------------------------------------------------*/

void pp_setdir(int fd, unsigned int r)
{
/* set port direction: r==0:write mode;  r==1:read mode */
#ifdef _MSC_VER
  pp_wcontrol(fd, LPT_READMODE, r);
#else
  if (ioctl(mscb_fd[fd-1].fd, PPDATADIR, &r))
    perror("PPDATADIR");
#endif
}

/*------------------------------------------------------------------*/

void pp_wdata(int fd, unsigned int data)
/* output data byte */
{
#ifdef _MSC_VER
  _outp((unsigned short)mscb_fd[fd-1].fd, data);
#else
  ioctl(mscb_fd[fd-1].fd, PPWDATA, &data);
#endif
}

/*------------------------------------------------------------------*/

unsigned char pp_rdata(int fd)
/* intput data byte */
{
#ifdef _MSC_VER
  return _inp((unsigned short)mscb_fd[fd-1].fd);
#else
  if (ioctl(mscb_fd[fd-1].fd, PPRDATA, &data))
    perror("PPRDATA");
  return data;
#endif
}

/*------------------------------------------------------------------*/

int mscb_out(int fd, unsigned char *buffer, int len, int bit9)
/********************************************************************\

  Routine: mscb_out

  Purpose: Write number of bytes to SM via parallel port

  Input:
    int  fd                 file descriptor for connection
    char *buffer            data buffer
    int  len                number of bytes in buffer
    int  bit9               set bit9 in communication, used for
                            node addressing

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout

\********************************************************************/
{
int i, timeout;

  /* remove accidental strobe */
  pp_wcontrol(fd, LPT_STROBE, 0);

  for (i=0 ; i<len ; i++)
    {
    /* wait for SM ready */
    for (timeout=0 ; timeout<TIMEOUT_OUT ; timeout++)
      {
      if (!pp_rstatus(fd, LPT_BUSY))
        break;
      }
    if (timeout == TIMEOUT_OUT)
      return MSCB_TIMEOUT;

    /* output data byte */
    pp_wdata(fd, buffer[i]);

    /* set bit 9 */
    if (bit9)
      pp_wcontrol(fd, LPT_BIT9, 1);
    else
      pp_wcontrol(fd, LPT_BIT9, 0);

    /* set strobe */
    pp_wcontrol(fd, LPT_STROBE, 1);

    /* wait for busy to become active */
    for (timeout=0 ; timeout<TIMEOUT_OUT ; timeout++)
      {
      if (pp_rstatus(fd, LPT_BUSY))
        break;
      }

    if (timeout == TIMEOUT_OUT)
      {
      pp_wdata(fd, 0xFF);
      pp_wcontrol(fd, LPT_STROBE, 0);
      return MSCB_TIMEOUT;
      }

    /* remove data, make port available for input */
    pp_wdata(fd, 0xFF);

    /* remove strobe */
    pp_wcontrol(fd, LPT_STROBE, 0);
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
    int  timeout            timeout in microseconds

  Output:
    char *c                 data bytes

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout

\********************************************************************/
{
int i;

  /* wait for DATAREADY, each port access takes roughly 1us */
  for (i=0 ; i<timeout ; i++)
    {
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
  for (i=0 ; i<1000 ; i++)
    {
    if (!pp_rstatus(fd, LPT_DATAREADY))
      break;
    }

  /* read data */
  *c = pp_rdata(fd);

  /* signal data read */
  pp_wcontrol(fd, LPT_ACK, 0);

  /* wait for mode switch */
  for (i=0 ; i<1000 ; i++)
    {
    if (pp_rstatus(fd, LPT_DATAREADY))
      break;
    }

  /* switch to output mode */
  pp_setdir(fd, 0);

  /* indicate mode switch */
  pp_wcontrol(fd, LPT_ACK, 1);

  /* wait for end of cycle */
  for (i=0 ; i<1000 ; i++)
    {
    if (!pp_rstatus(fd, LPT_DATAREADY))
      break;
    }

  /* remove acknowlege */
  pp_wcontrol(fd, LPT_ACK, 0);

  if (i == 1000)
    return MSCB_TIMEOUT;

  for (i=0 ; i<50 ; i++)
    pp_rdata(fd);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_in(int fd, char *buffer, int size, int timeout)
/********************************************************************\

  Routine: mscb_in

  Purpose: Read number of bytes from SM via parallel port

  Input:
    int  fd                 file descriptor for connection
    int  size               size of data buffer
    int  timeout            timeout in microseconds

  Output:
    char *buffer            data buffer

  Function value:
    int                     number of received bytes, -1 for timeout

\********************************************************************/
{
int i, n, len, status;

  n = 0;
  len = -1;

  /* wait for CMD_ACK command */
  do
    {
    status = mscb_in1(fd, buffer, timeout);

    /* return in case of timeout */
    if (status != MSCB_SUCCESS)
      return -1;

    /* check for Acknowledge */
    if ((buffer[0] & 0xF8) == CMD_ACK)
      {
      len = buffer[n++] & 0x7;

      /* check for variable length data block */
      if (len == 7)
        {
        status = mscb_in1(fd, buffer+n, timeout);

        /* return in case of timeout */
        if (status != MSCB_SUCCESS)
          return -1;

        len = buffer[n++];
        }
      }

    } while (len == -1);

  /* receive remaining bytes and CRC code */
  for (i=0 ; i<len+1 ; i++)
    {
    status = mscb_in1(fd, buffer+n+i, timeout);

    /* return in case of timeout */
    if (status != MSCB_SUCCESS)
      return -1;
    }

  return n+i;
}

/*------------------------------------------------------------------*/

void mscb_get_version(char *lib_version, char *prot_version)
{
  strcpy(lib_version, MSCB_LIBRARY_VERSION);
  strcpy(prot_version, MSCB_PROTOCOL_VERSION);
} 
                                        
/*------------------------------------------------------------------*/

int mscb_init(char *device)
/********************************************************************\

  Routine: mscb_init

  Purpose: Initialize and open MSCB

  Input:
    char *device            Under NT: lpt1 or lpt2
                            Under Linux: /dev/parport0 or /dev/parport1
                            "<host>:device" for RPC connection

  Function value:
    int fd                  device descriptor for connection, -1 if
                            error

\********************************************************************/
{
int           index, i;
int           status;
unsigned char c;
char          host[256], port[256];

  if (strchr(device, ':'))
    {
    strcpy(port, strchr(device, ':')+1);
    strcpy(host, device);
    *strchr(host, ':') = 0;
    if (rpc_connect(host) != RPC_SUCCESS)
      return -1;
    }
  else
    strcpy(port, device);

  if (rpc_connected())
    return rpc_call(RPC_MSCB_INIT, port);

  /* search for new file descriptor */
  for (index=0 ; index<MSCB_MAX_FD ; index++)
    if (mscb_fd[index].fd == 0)
      break;

  if (index == MSCB_MAX_FD)
    return -1;

  strcpy(mscb_fd[index].device, port);

#ifdef _MSC_VER

  if (port[1] == 'x')
    sscanf(port+2, "%x", &mscb_fd[index].fd);
  else
    {
    if (strlen(port) == 4)
      i = atoi(port+3);
    else
      return -1;

    /* derive base address from device name */
    if (i == 1)
      mscb_fd[index].fd = 0x378;
    else if (i == 2)
      mscb_fd[index].fd = 0x278;
    else
      return -1;
    }

  /* under NT, user directio driver */
  {

  OSVERSIONINFO vi;
  DWORD buffer[4];
  DWORD size;
  HANDLE hdio;

  buffer[0] = 6; /* give IO */
  buffer[1] = mscb_fd[index].fd;
  buffer[2] = buffer[1]+4;
  buffer[3] = 0;

  vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&vi);

  /* use DirectIO driver under NT to gain port access */
  if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
    hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL,
           OPEN_EXISTING, 0, NULL);
    if (hdio == INVALID_HANDLE_VALUE)
      {
      printf("mscb.c: Cannot access parallel port (No DirectIO driver installed)\n");
      return -1;
      }

    if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer),
        NULL, 0, &size, NULL))
      return -1;
    }
  }

#elif defined(__linux__)

  mscb_fd[index].fd = open(port, O_RDWR);
  if (mscb_fd[index].fd < 0)
    {
    perror("open");
    printf("Please make sure that device \"%s\" is world readable/writable\n", port);
    return -1;
    }

  if (ioctl(mscb_fd[index].fd, PPCLAIM))
    {
    perror("PPCLAIM");
    printf("Please load driver via \"modprobe parport_pc\" as root\n");
    return -1;
    }

  if (ioctl(mscb_fd[index].fd, PPRELEASE))
    {
    perror("PPRELEASE");
    return -1;
    }

  i = IEEE1284_MODE_BYTE;
  if (ioctl(mscb_fd[index].fd, PPSETMODE, &i))
    {
    perror("PPSETMODE");
    return -1;
    }

#endif

  mscb_lock(index+1);

  /* set initial state of handshake lines */
  pp_wcontrol(index+1, LPT_STROBE, 0);

  /* switch to output mode */
  pp_setdir(index+1, 0);
  
  /* check if SM alive */
  if (pp_rstatus(index+1, LPT_BUSY))
    {
    //printf("mscb.c: No SM present on parallel port\n");
    mscb_release(mscb_fd[index].fd);
    return -2;
    }

  /* empty RBuffer of SM */
  do
    {
    status = mscb_in1(index+1, &c, 1000);
    if (status == MSCB_SUCCESS)
      printf("%02X ", c);
    } while (status == MSCB_SUCCESS);

  mscb_release(index+1);

  return index+1;
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
  if (rpc_connected())
    return rpc_call(RPC_MSCB_EXIT, fd);

  if (fd > MSCB_MAX_FD)
    return MSCB_INVAL_PARAM;

#ifdef __linux__
  close(mscb_fd[fd-1].fd);
#endif

  memset(&mscb_fd[fd-1], 0, sizeof(MSCB_FD));

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

void mscb_check(char *device)
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

  mscb_init(device);
  fd = 1;

  mscb_lock(fd);

  printf("Toggling %s output pins, hit ENTER to stop.\n", device);
  printf("GND = 19-25, toggling 2-9, 1, 14, 16 and 17\n\n");
  do
    {
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

  do
    {
    d = pp_rdata(fd);
    for (i=0 ; i<8 ; i++)
      {
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

int mscb_addr(int fd, int cmd, int adr, int retry)
/********************************************************************\

  Routine: mscb_addr

  Purpose: Address node or nodes, only used internall from read and
           write routines. A MSCB lock has to be obtained outside
           of this routine!

  Input:
    int fd                  File descriptor for connection
    int cmd                 Addressing mode, one of
                              CMD_ADDR_BC
                              CMD_ADDR_NODE8
                              CMD_ADDR_GRP8
                              CMD_PING8
                              CMD_ADDR_NODE16
                              CMD_ADDR_GRP16
                              CMD_PING16

    int adr                 Node or group address
    int retry               Number of retries

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving ping acknowledge
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[10];
int i, n;

  for (n = 0 ; n < retry ; n++)
    {
    buf[0] = cmd;

    if (cmd == CMD_ADDR_NODE8 ||
        cmd == CMD_ADDR_GRP8 ||
        cmd == CMD_PING8)
      {
      buf[1] = (unsigned char) adr;
      buf[2] = crc8(buf, 2);
      mscb_out(fd, buf, 3, 1);
      }
    else if (cmd == CMD_ADDR_NODE16 ||
             cmd == CMD_ADDR_GRP16 ||
             cmd == CMD_PING16)
      {
      buf[1] = (unsigned char) (adr >> 8);
      buf[2] = (unsigned char) (adr & 0xFF);
      buf[3] = crc8(buf, 3);
      mscb_out(fd, buf, 4, 1);
      }
    else
      {
      buf[1] = crc8(buf, 1);
      mscb_out(fd, buf, 2, 1);
      }

    if (cmd == CMD_PING8 || cmd == CMD_PING16)
      {
      /* read back ping reply, 1ms timeout */
      i = mscb_in1(fd, buf, 1000);

      if (i == MSCB_SUCCESS && buf[0] == CMD_ACK)
        return MSCB_SUCCESS;

      if (retry > 1)
        {
        /* send 0's to overflow partially filled node receive buffer */
        memset(buf, 0, sizeof(buf));
        mscb_out(fd, buf, 10, 1);

        /* wait some time */
        Sleep(10);
        }

      /* try again.... */
      }
    else
      return MSCB_SUCCESS;
    }

  return MSCB_TIMEOUT;
}

/*------------------------------------------------------------------*/

int mscb_reboot(int fd, int adr)
/********************************************************************\

  Routine: mscb_reboot

  Purpose: Reboot node by sending CMD_INIT

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_TIMEOUT            Timeout receiving ping acknowledge

\********************************************************************/
{
unsigned char buf[10];
int status;

  if (rpc_connected())
    return rpc_call(RPC_MSCB_REBOOT, fd, adr);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_INIT;
  buf[1] = crc8(buf, 1);
  mscb_out(fd, buf, 2, 0);

  mscb_release(fd);

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
  if (rpc_connected())
    return rpc_call(RPC_MSCB_RESET, fd);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  /* toggle reset */
  pp_wcontrol(fd, LPT_RESET, 1);
  pp_wcontrol(fd, LPT_RESET, 0);

  /* wait for node to reboot */
  Sleep(5000);

  mscb_release(fd);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_ping(int fd, int adr)
/********************************************************************\

  Routine: mscb_info

  Purpose: Ping node to see if it's alive

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address

  Output:
    none

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int status;

  if (rpc_connected())
    return rpc_call(RPC_MSCB_PING, fd, adr);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 1);

  mscb_release(fd);
  return status;
}

/*------------------------------------------------------------------*/

int mscb_info(int fd, int adr, MSCB_INFO *info)
/********************************************************************\

  Routine: mscb_info

  Purpose: Retrieve info on addressd node

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address

  Output:
    MSCB_INFO *info         Info structure defined in mscb.h

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int i, status;
unsigned char buf[80];

  if (rpc_connected())
    return rpc_call(RPC_MSCB_INFO, fd, adr, info);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_GET_INFO;
  buf[1] = crc8(buf, 1);
  mscb_out(fd, buf, 2, 0);

  i = mscb_in(fd, buf, sizeof(buf), 5000);
  mscb_release(fd);

  if (i<(int)sizeof(MSCB_INFO)+2)
    return MSCB_TIMEOUT;

  memcpy(info, buf+2, sizeof(MSCB_INFO));

  /* do CRC check */
  if (crc8(buf, sizeof(MSCB_INFO)+2) != buf[sizeof(MSCB_INFO)+2])
    return MSCB_CRC_ERROR;

  WORD_SWAP(&info->node_address);
  WORD_SWAP(&info->group_address);
  WORD_SWAP(&info->watchdog_resets);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_info_channel(int fd, int adr, int type, int index, MSCB_INFO_CHN *info)
/********************************************************************\

  Routine: mscb_info_channel

  Purpose: Retrieve info on a specific channel

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    int type                Channel type, one of
                              GET_INFO_WRITE
                              GET_INFO_READ
                              GET_INFO_CONF
    int index               Channel index 0..255

  Output:
    MSCB_INFO_CHN *info     Info structure defined in mscb.h

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error

    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int i, status;
unsigned char buf[80];

  if (rpc_connected())
    return rpc_call(RPC_MSCB_INFO_CHANNEL, fd, adr, type, index, info);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_GET_INFO+2;
  buf[1] = type;
  buf[2] = index;
  buf[3] = crc8(buf, 3);
  mscb_out(fd, buf, 4, 0);

  i = mscb_in(fd, buf, sizeof(buf), 5000);
  mscb_release(fd);

  if (i<(int)sizeof(MSCB_INFO_CHN)+3)
    return MSCB_TIMEOUT;

  memcpy(info, buf+2, sizeof(MSCB_INFO_CHN));

  /* do CRC check */
  if (crc8(buf, sizeof(MSCB_INFO_CHN)+2) != buf[sizeof(MSCB_INFO_CHN)+2])
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_addr(int fd, int adr, int node, int group)
/********************************************************************\

  Routine: mscb_set_addr

  Purpose: Set node and group address of an node

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    int node                16-bit node address
    int group               16-bit group address

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[8];
int status;

  if (rpc_connected())
    return rpc_call(RPC_MSCB_SET_ADDR, fd, adr, node, group);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {

    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_SET_ADDR;
  buf[1] = (unsigned char ) (node >> 8);
  buf[2] = (unsigned char ) (node & 0xFF);
  buf[3] = (unsigned char ) (group >> 8);
  buf[4] = (unsigned char ) (group & 0xFF);
  buf[5] = crc8(buf, 5);
  mscb_out(fd, buf, 6, 0);

  mscb_release(fd);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_name(int fd, int adr, char *name)
/********************************************************************\

  Routine: mscb_set_name

  Purpose: Set node name

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    char *name              New node name, up to 16 characters

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[20];
int status, i;

  if (rpc_connected())
    return rpc_call(RPC_MSCB_SET_NAME, fd, adr, name);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {

    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_SET_ADDR | 0x07;
  for (i=0 ; i<16 && name[i] ; i++)
    buf[2+i] = name[i];

  if (i<16)
    buf[2 + (i++)] = 0;

  buf[1] = i; /* varibale buffer length */

  buf[2+i] = crc8(buf, 2+i);
  mscb_out(fd, buf, 3+i, 0);

  mscb_release(fd);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_group(int fd, int adr, unsigned char channel, void *data, int size)
/********************************************************************\

  Routine: mscb_write_na

  Purpose: Write data to channels on group of nodes

  Input:
    int fd                  File descriptor for connection
    int adr                 Group address
    unsigned char channel   Channel index 0..255
    unsigned int  data      Data to send
    int size                Data size in bytes 1..4 for byte, word,
                            and dword

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int i, status;
unsigned char *d;
unsigned char buf[10];

  if (rpc_connected())
    return rpc_call(RPC_MSCB_WRITE_GROUP, fd, adr, channel, data, size);

  if (size > 4 || size < 1)
    return MSCB_INVAL_PARAM;

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_ADDR_GRP16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_WRITE_NA+size+1;
  buf[1] = channel;

  for (i=0,d=data ; i<size ; i++)
    buf[2+size-1-i] = *d++;

  buf[2+i] = crc8(buf, 2+i);
  mscb_out(fd, buf, 3+i, 0);

  mscb_release(fd);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write(int fd, int adr, unsigned char channel, void *data, int size)
/********************************************************************\

  Routine: mscb_write

  Purpose: Write data to channel on single node

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    unsigned char channel   Channel index 0..255
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
int           i, status;
unsigned char buf[10], crc, ack[2];
unsigned char *d;

  if (rpc_connected())
    return rpc_call(RPC_MSCB_WRITE, fd, adr, channel, data, size);

  if (size > 4 || size < 1)
    return MSCB_INVAL_PARAM;

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_WRITE_ACK+size+1;
  buf[1] = channel;

  for (i=0,d=data ; i<size ; i++)
    buf[2+size-1-i] = *d++;

  crc = crc8(buf, 2+i);
  buf[2+i] = crc;
  mscb_out(fd, buf, 3+i, 0);

  /* read acknowledge */
  i = mscb_in(fd, ack, 2, 5000);
  mscb_release(fd);
  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_conf(int fd, int adr, unsigned char channel, void *data, int size)
/********************************************************************\

  Routine: mscb_write_conf

  Purpose: Write configuration parameter to node with acknowledge

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    unsigned char channel   Channel index 0..254, 255 for node CSR
    unsigned int  data      Data to send
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
int           i, status;
unsigned char *d;
unsigned char buf[10], crc, ack[2];

  if (rpc_connected())
    return rpc_call(RPC_MSCB_WRITE_CONF, fd, adr, channel, data, size);

  if (size > 4 || size < 1)
    return MSCB_INVAL_PARAM;

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_WRITE_CONF+size+1;
  buf[1] = channel;

  for (i=0,d=data ; i<size ; i++)
    buf[2+size-1-i] = *d++;

  crc = crc8(buf, 2+i);
  buf[2+i] = crc;
  mscb_out(fd, buf, 3+i, 0);

  /* read acknowledge, 100ms timeout */
  i = mscb_in(fd, ack, 2, 100000);
  mscb_release(fd);

  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_flash(int fd, int adr)
/********************************************************************\

  Routine: mscb_flash

  Purpose: Flash configuration parameter and channels values to
           EEPROM


  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
                            and dword

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int           i, status;
unsigned char buf[10], crc, ack[2];

  if (rpc_connected())
    return rpc_call(RPC_MSCB_FLASH, fd, adr);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_FLASH;
  crc = crc8(buf, 1);
  buf[1] = crc;
  mscb_out(fd, buf, 2, 0);

  /* read acknowledge, 100ms timeout */
  i = mscb_in(fd, ack, 2, 100000);
  mscb_release(fd);

  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_upload(int fd, int adr, char *buffer, int size)
/********************************************************************\

  Routine: mscb_upload

  Purpose: Upload new firmware to node


  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    char *filename          File name for Intel HEX file

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb
    MSCB_FORMAT_ERROR       Error in HEX file format

\********************************************************************/
{
unsigned char  buf[512], crc, ack[2], image[0x8000], *line;
unsigned int   len, ofh, ofl, type, d;
int            i, j, status, page;
unsigned short ofs;

  if (rpc_connected())
    return rpc_call(RPC_MSCB_UPLOAD, fd, adr, buffer, size);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  /* interprete HEX file */
  memset(image, 0xFF, sizeof(image));
  line = buffer;
  do
    {
    if (line[0] == ':')
      {
      sscanf(line+1, "%02x%02x%02x%02x", &len, &ofh, &ofl, &type);
      ofs = (ofh << 8) | ofl;

      for (i=0 ; i<(int)len ; i++)
        {
        sscanf(line+9+i*2, "%02x", &d);
        image[ofs+i] = d;
        }
      line = strchr(line, '\r')+1;
      if (line && *line == '\n')
        line++;
      }
    else
      return MSCB_FORMAT_ERROR;

    } while (*line);


  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  /* send upgrade command */
  buf[0] = CMD_UPGRADE;
  crc = crc8(buf, 1);
  buf[1] = crc;
  mscb_out(fd, buf, 2, 0);

  /* read acknowledge, 100ms timeout */
  i = mscb_in(fd, ack, 2, 100000);
  if (i<2)
    {
    printf("Error: Upload not implemented in remote node\n");
    mscb_release(fd);
    return MSCB_TIMEOUT;
    }

  if (ack[0] != CMD_ACK || ack[1] != crc)
    {
    printf("Error: Cannot set remote node into upload mode\n");
    mscb_release(fd);
    return MSCB_CRC_ERROR;
    }

  /* wait for ready, 3 sec timeout */
  for (i=0 ; i<30 ; i++)
    {
    if (mscb_in1(fd, ack, 1000) == MSCB_SUCCESS)
      break;
    Sleep(100);
    }

  if (i == 30 || ack[0] != 0xBE)
    {
    printf("Error: timeout from remote node\n");
    mscb_release(fd);
    return MSCB_TIMEOUT;
    }

  /* program pages up to 0x7000 */
  for (page=0 ; page<56 ; page++)
    {
    /* check if page contains data */
    for (i=0 ; i<512 ; i++)
      if (image[page*512+i] != 0xFF)
        break;

    if (i<512)
      {
      do
        {
        printf("E");
        fflush(stdout);

        /* erase page */
        buf[0] = 2;
        buf[1] = page;
        mscb_out(fd, buf, 2, 0);

        if (mscb_in1(fd, ack, 100000) != MSCB_SUCCESS)
          {
          printf("Error: timeout from remote node for erase page\n");
          mscb_release(fd);
          return MSCB_TIMEOUT;
          }

        printf("\bP");
        fflush(stdout);

        /* program page */
        buf[0] = 3;
        buf[1] = page;
        mscb_out(fd, buf, 2, 0);

        for (i=0 ; i<512 ; i++)
          buf[i] = image[page*512+i];

        mscb_out(fd, buf, 512, 0);

        if (mscb_in1(fd, ack, 100000) != MSCB_SUCCESS)
          {
          printf("Error: timeout from remote node for program page\n");
          mscb_release(fd);
          return MSCB_TIMEOUT;
          }

        for (j=0 ; j<10 ; j++)
          {
          printf("\bV");
          fflush(stdout);

          /* verify page */
          buf[0] = 4;
          buf[1] = page;
          mscb_out(fd, buf, 2, 0);

          for (i=0 ; i<512 ; i++)
            if (mscb_in1(fd, buf+i, 100000) != MSCB_SUCCESS)
              break;

          if (i == 512)
            if (mscb_in1(fd, ack, 100000) == MSCB_SUCCESS)
              break;

          Sleep(100);

          }

        if (j == 10)
          {
          printf("Error: error on page verification (tried 10 times)\n");
          mscb_release(fd);
          return MSCB_TIMEOUT;
          }

        for (i=0 ; i<512 ; i++)
          if (buf[i] != image[page*512+i])
            break;

        /* reprogram page until verified */
        } while (i < 512);

      printf("\b=");
      fflush(stdout);
      }
    }

  printf("\n");

  /* reboot node */
  buf[0] = 5;
  mscb_out(fd, buf, 1, 0);

  mscb_release(fd);
  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read(int fd, int adr, unsigned char channel, void *data, int *size)
/********************************************************************\

  Routine: mscb_read

  Purpose: Read data from channel on node

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    unsigned char channel   Channel index 0..255
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
int           i, n, status;
unsigned char buf[10], crc;

  memset(data, 0, *size);
  if (rpc_connected())
    return rpc_call(RPC_MSCB_READ, fd, adr, channel, data, size);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  /* try ten times */
  for (n=0 ; n<10 ; n++)
    {
    buf[0] = CMD_READ+1;
    buf[1] = channel;
    buf[2] = crc8(buf, 2);
    mscb_out(fd, buf, 3, 0);

    /* read data */
    i = mscb_in(fd, buf, 10, 5000);

    if (i<2)
      continue;

    crc = crc8(buf, i-1);

    if (buf[0] != CMD_ACK+i-2 || buf[i-1] != crc)
      continue;

    memcpy(data, buf+1, i-2);
    if (i-2 == 2)
      WORD_SWAP(data);
    if (i-2 == 4)
      DWORD_SWAP(data);

    *size = i-2;

    mscb_release(fd);
    return MSCB_SUCCESS;
    }

  mscb_release(fd);

  if (i<2)
    return MSCB_TIMEOUT;

  return MSCB_CRC_ERROR;
}

/*------------------------------------------------------------------*/

int mscb_read_channels(int fd, int adr, unsigned char channel1, unsigned char channel2, void *data, int *size)
/********************************************************************\

  Routine: mscb_read

  Purpose: Read data from channel on node

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    unsigned char channel1  First channel to read
    unsigned char channel2  Last channel to read
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
int           i, n, status;
unsigned char buf[256], crc;

  memset(data, 0, *size);
  if (rpc_connected())
    return rpc_call(RPC_MSCB_READ_CHANNELS, fd, adr, channel1, channel2, data, size);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  /* try ten times */
  for (n=0 ; n<10 ; n++)
    {
    buf[0] = CMD_READ+2;
    buf[1] = channel1;
    buf[2] = channel2;
    buf[3] = crc8(buf, 3);
    mscb_out(fd, buf, 4, 0);

    /* read data */
    i = mscb_in(fd, buf, 256, 5000);

    if (i<2)
      continue;

    crc = crc8(buf, i-1);

    if (buf[0] != CMD_ACK+7 || buf[i-1] != crc)
      continue;

    memcpy(data, buf+2, i-3);
    *size = i-3;

    mscb_release(fd);
    return MSCB_SUCCESS;
    }

  mscb_release(fd);

  if (i<2)
    return MSCB_TIMEOUT;

  return MSCB_CRC_ERROR;
}

/*------------------------------------------------------------------*/

int mscb_read_conf(int fd, int adr, unsigned char index, void *data, int *size)
/********************************************************************\

  Routine: mscb_read_conf

  Purpose: Read configuration parameter on node

  Input:
    int fd                  File descriptor for connection
    int adr                 Node address
    unsigned char index     Parameter index 0..255
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
int           i, n, status;
unsigned char buf[10], crc;

  memset(data, 0, *size);
  if (rpc_connected())
    return rpc_call(RPC_MSCB_READ_CONF, fd, adr, index, data, size);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  /* try ten times */
  for (n=0 ; n<10 ; n++)
    {
    buf[0] = CMD_READ_CONF+1;
    buf[1] = index;
    buf[2] = crc8(buf, 2);
    mscb_out(fd, buf, 3, 0);

    /* read data */
    i = mscb_in(fd, buf, 10, 5000);

    if (i<2)
      continue;

    crc = crc8(buf, i-1);

    if (buf[0] != CMD_ACK+i-2 || buf[i-1] != crc)
      continue;

    memcpy(data, buf+1, i-2);
    if (i-2 == 2)
      WORD_SWAP(data);
    if (i-2 == 4)
      DWORD_SWAP(data);

    *size = i-2;

    mscb_release(fd);
    return MSCB_SUCCESS;
    }

  mscb_release(fd);

  if (i<2)
    return MSCB_TIMEOUT;

  return MSCB_CRC_ERROR;
}

/*------------------------------------------------------------------*/

int mscb_user(int fd, int adr, void *param, int size, void *result, int *rsize)
/********************************************************************\

  Routine: mscb_user

  Purpose: Call user function on node

  Input:
    int  fd                 File descriptor for connection
    int adr                 Node address
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

  if (rpc_connected())
    return rpc_call(RPC_MSCB_USER, fd, adr, param, size, result, rsize);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 10);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  if (size > 4 || size < 0)
    return MSCB_FORMAT_ERROR;

  buf[0] = CMD_USER+size;

  for (i=0 ; i<size ; i++)
    buf[1+i] = ((char *)param)[i];

  /* add CRC code and send data */
  buf[1+i] = crc8(buf, 1+i);
  status = mscb_out(fd, buf, 2+i, 0);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  if (result == NULL)
    {
    mscb_release(fd);
    return MSCB_SUCCESS;
    }

  /* read result */
  n = mscb_in(fd, buf, sizeof(buf), 5000);
  mscb_release(fd);

  if (n<0)
    return MSCB_TIMEOUT;

  if (rsize)
    *rsize = n-2;
  for (i=0 ; i<n-2 ; i++)
    ((char *)result)[i] = buf[1+i];

  if (buf[n-1] != crc8(buf, n-1))
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_echo(int fd, int adr, unsigned char d1, unsigned char *d2)
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
unsigned char buf[80];
 
  *d2 = 0xFF;

  if (rpc_connected())
    return rpc_call(RPC_MSCB_ECHO, fd, adr, d1, d2);

  if (fd < 1 || mscb_fd[fd-1].fd == 0)
    return MSCB_INVAL_PARAM;

  if (mscb_lock(fd) != MSCB_SUCCESS)
    return MSCB_MUTEX;

  status = mscb_addr(fd, CMD_PING16, adr, 1);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  buf[0] = CMD_ECHO;
  buf[1] = d1;

  /* add CRC code and send data */
  buf[2] = crc8(buf, 2);
  status = mscb_out(fd, buf, 3, 0);
  if (status != MSCB_SUCCESS)
    {
    mscb_release(fd);
    return status;
    }

  /* read result */
  n = mscb_in(fd, buf, sizeof(buf), 5000);
  mscb_release(fd);

  if (n<0)
    return MSCB_TIMEOUT;

  *d2 = buf[1];

  if (buf[n-1] != crc8(buf, n-1))
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

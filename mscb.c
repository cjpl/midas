/********************************************************************\

  Name:         mscb.c
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus communication functions

  $Log$
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

#ifdef _MSC_VER
#include <windows.h>
#include <conio.h>
#endif

#include <stdio.h>
#include "mscb.h"

/*------------------------------------------------------------------*/

#if defined( _MSC_VER )
#define OUTP(_p, _d) _outp((unsigned short) (_p), (int) (_d))
#define OUTPW(_p, _d) _outpw((unsigned short) (_p), (unsigned short) (_d))
#define INP(_p) _inp((unsigned short) (_p))
#define INPW(_p) _inpw((unsigned short) (_p))
#define OUTP_P(_p, _d) {_outp((unsigned short) (_p), (int) (_d)); _outp((unsigned short)0x80,0);}
#define OUTPW_P(_p, _d) {_outpw((unsigned short) (_p), (unsigned short) (_d)); _outp((unsigned short)0x80,0);}
#define INP_P(_p) _inp((unsigned short) (_p)); _outp((unsigned short)0x80,0);
#define INPW_P(_p) _inpw((unsigned short) (_p));_outp((unsigned short)0x80,0);
#endif

/* Byte and Word swapping big endian <-> little endian */
#define WORD_SWAP(x) { BYTE _tmp;                               \
                       _tmp= *((BYTE *)(x));                    \
                       *((BYTE *)(x)) = *(((BYTE *)(x))+1);     \
                       *(((BYTE *)(x))+1) = _tmp; }

#define DWORD_SWAP(x) { BYTE _tmp;                              \
                       _tmp= *((BYTE *)(x));                    \
                       *((BYTE *)(x)) = *(((BYTE *)(x))+3);     \
                       *(((BYTE *)(x))+3) = _tmp;               \
                       _tmp= *(((BYTE *)(x))+1);                \
                       *(((BYTE *)(x))+1) = *(((BYTE *)(x))+2); \
                       *(((BYTE *)(x))+2) = _tmp; }

/* file descriptor */

#define MSCB_MAX_FD 10

typedef struct {
  int fd;
} MSCB_FD;

MSCB_FD mscb_fd[MSCB_MAX_FD];

/* pin definitions parallel port */

/*

  Ofs Inv   PC     DB25       SM    DIR   MSCB Name
                              
   0  No    D0 ----- 2 ----- P1.0   <->    |
   0  No    D1 ----- 3 ----- P1.1   <->    |
   0  No    D2 ----- 4 ----- P1.2   <->    |  D
   0  No    D3 ----- 5 ----- P1.3   <->     \ a
   0  No    D4 ----- 6 ----- P1.4   <->     / t
   0  No    D5 ----- 7 ----- P1.5   <->    |  a
   0  No    D6 ----- 8 ----- P1.6   <->    |
   0  No    D7 ----- 9 ----- P1.7   <->    |
                              
   2  Yes   !STR --- 1  ---- P0.7    ->    /STROBE
   1  Yes   !BSY --- 11 ---- P3.2   <-     BUSY

   1  Yes   !ACK --- 10 ---- P3.3   <-     /DATAREADY
   2  Yes   !DSL --- 17 ---- P0.3    ->    /ACK

   1  No    PAP ---- 12 ---- P3.1   <-     STATUS

   2  Yes   !ALF --- 14 ---- P3.0    ->    /RESET
   2  No    INI ---- 16 ---- P0.4    ->    BIT9

*/


/* stobe, busy: write to SM */

#define LPT_NSTROBE     (1<<0)   /* pin 1 on DB25 */
#define LPT_NSTROBE_OFS     2

#define LPT_NBUSY       (1<<7)   /* pin 11 on DB25 */
#define LPT_NBUSY_OFS       1

/* data ready (!ACK) and acknowledge (!DSL): read from SM */

#define LPT_NDATAREADY  (1<<6)   /* pin 10 on DB25 */
#define LPT_NDATAREADY_OFS  1

#define LPT_NACK        (1<<3)   /* pin 17 on DB25 */
#define LPT_NACK_OFS        2

/* /reset (!PAP), bit9 (INI) and direction switch */

#define LPT_NRESET      (1<<1)
#define LPT_NRESET_OFS      2

#define LPT_BIT9        (1<<2)   /* pin 16 on DB25 */
#define LPT_BIT9_OFS        2

#define LPT_DIRECTION   (1<<5)   /* no pin */
#define LPT_DIRECTION_OFS   2

/* other constants */

#define TIMEOUT_OUT      1000    /* ~1ms */

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

int mscb_lock()
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
#endif
  return MSCB_SUCCESS;
}

int mscb_release()
{
#ifdef _MSC_VER
int status;

  status = ReleaseMutex(mutex_handle);
  if (status == FALSE)
    return 0;

#endif
  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_out(int fd, char *buffer, int len, int bit9)
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
unsigned char busy, bit9_mask;

  bit9_mask = bit9 ? LPT_BIT9 : 0;

  for (i=0 ; i<len ; i++)
    {
    /* wait for SM ready */
    for (timeout=0 ; timeout<TIMEOUT_OUT ; timeout++)
      {
      busy = (INP(mscb_fd[fd-1].fd + LPT_NBUSY_OFS) & LPT_NBUSY) == 0;
      if (!busy)
        break;
      }
    if (timeout == TIMEOUT_OUT)
      return MSCB_TIMEOUT;

    /* output data byte */
    OUTP(mscb_fd[fd-1].fd, buffer[i]);

    /* set strobe */
    OUTP(mscb_fd[fd-1].fd + LPT_NSTROBE_OFS, LPT_NSTROBE | bit9_mask);

    /* wait for busy to become active */
    for (timeout=0 ; timeout<TIMEOUT_OUT ; timeout++)
      {
      busy = (INP(mscb_fd[fd-1].fd + LPT_NBUSY_OFS) & LPT_NBUSY) == 0;
      if (busy)
        break;
      }
    if (timeout == TIMEOUT_OUT)
      return MSCB_TIMEOUT;

    /* remove data, make port available for input */
    OUTP(mscb_fd[fd-1].fd, 0xFF);
    
    /* remove strobe */
    OUTP(mscb_fd[fd-1].fd + LPT_NSTROBE_OFS, 0);
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
unsigned char dataready;

  /* wait for DATAREADY, each port access takes roughly 1us */
  for (i=0 ; i<timeout ; i++)
    {
    dataready = (INP(mscb_fd[fd-1].fd + LPT_NDATAREADY_OFS) & LPT_NDATAREADY) == 0;
    if (dataready)
      break;
    }
  if (i == timeout)
    return MSCB_TIMEOUT;

  /* prepare port for input */
  OUTP(mscb_fd[fd-1].fd+LPT_DIRECTION_OFS, LPT_DIRECTION);

  /* get data */
  *c = INP(mscb_fd[fd-1].fd);

  /* set acknowlege, switch port to output */
  OUTP(mscb_fd[fd-1].fd + LPT_NACK_OFS, LPT_NACK);

  /* wait for DATAREADY to be removed */
  for (i=0 ; i<1000 ; i++)
    {
    dataready = (INP(mscb_fd[fd-1].fd + LPT_NDATAREADY_OFS) & LPT_NDATAREADY) == 0;
    if (!dataready)
      break;
    }
 
  /* remove acknowlege */
  OUTP(mscb_fd[fd-1].fd + LPT_NACK_OFS, 0);

  if (i == 1000)
    return MSCB_TIMEOUT;

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

int mscb_init(char *device)
/********************************************************************\

  Routine: mscb_init

  Purpose: Initialize and open MSCB 

  Input:
    char *device            Under NT: lpt1 or lpt2
                            Under Linux: /dev/parport0 or /dev/parport1

  Function value:
    int fd                  device descriptor for connection, -1 if
                            error

\********************************************************************/
{
int           index, i;
BYTE          d;
int           status;
unsigned char c;

  /* search for new file descriptor */
  for (index=0 ; index<MSCB_MAX_FD ; index++)
    if (mscb_fd[index].fd == 0)
      break;

  if (index == MSCB_MAX_FD)
    return -1;

#ifdef _MSC_VER

  if (strlen(device) == 4)
    i = atoi(device+3);
  else
    return -1;
  
  /* derive base address from device name */
  if (i == 1)
    mscb_fd[index].fd = 0x378;
  else if (i == 2)
    mscb_fd[index].fd = 0x278;
  else
    return -1;
  
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
      //printf("mscb.c: Cannot access parallel port (No DirectIO driver installed)\n");
      return -1;
      }

    if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), 
		    NULL, 0, &size, NULL))
      return -1;
    }
  }
#endif _MSC_VER

  mscb_lock();

  /* set initial state of handshake lines */
  OUTP(mscb_fd[index].fd + LPT_NSTROBE_OFS, 0);

  /* check if SM alive */
  d = INP(mscb_fd[index].fd + LPT_NBUSY_OFS);
  if ((d & LPT_NBUSY) == 0)
    {
    //printf("mscb.c: No SM present on parallel port\n");
    mscb_release();
    return -1;
    }

  /* empty RBuffer of SM */
  do
    {
    status = mscb_in1(index+1, &c, 1000);
    if (status == MSCB_SUCCESS)
      printf("%02X ", c);
    } while (status == MSCB_SUCCESS);

  mscb_release();

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
  if (fd > MSCB_MAX_FD)
    return MSCB_INVAL_PARAM;

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

#ifdef _MSC_VER

  if (strlen(device) == 4)
    i = atoi(device+3);
  else
    {
    printf("Wrong device, either LPT1 or LPT2.\n");
    return;
    }
  
  /* derive base address from device name */
  if (i == 1)
    fd = 0x378;
  else if (i == 2)
    fd = 0x278;
  else
    {
    printf("Wrong device, either LPT1 or LPT2.\n");
    return;
    }
  
  /* under NT, user directio driver */
  {
  OSVERSIONINFO vi;
  DWORD buffer[4];
  DWORD size;
  HANDLE hdio;

  buffer[0] = 6; /* give IO */
  buffer[1] = fd;
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
      return;
      }

    if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), 
		    NULL, 0, &size, NULL))
      return;
    }

  printf("Toggling %s output pins, hit ENTER to stop.\n", device);
  printf("GND = 19-25, toggling 2-9, 1, 14, 16 and 17\n\n");
  do
    {
    printf("\r00000000 0000");
    OUTP(fd, 0);
    OUTP(fd + LPT_NSTROBE_OFS, LPT_NSTROBE | LPT_NACK | LPT_NRESET);
    
    Sleep(300);

    printf("\r11111111 1111");
    OUTP(fd, 0xFF);
    OUTP(fd + LPT_NSTROBE_OFS, LPT_BIT9);

    Sleep(1000);

    } while (!kbhit());

  while (kbhit())
    getch();

  /* switch port to input */
  OUTP(fd+LPT_DIRECTION_OFS, LPT_DIRECTION);

  printf("\n\n\nInput display, hit ENTER to stop.\n");
  printf("Pins 2-9, 10, 11, and 12\n\n");

  do
    {
    d = INP(fd);
    for (i=0 ; i<8 ; i++)
      {
      printf("%d", (d & 1) > 0);
      d >>= 1;
      }

    d = INP(fd + LPT_NBUSY_OFS);
    printf(" %d%d\r", (d & LPT_NDATAREADY) > 0, (d & LPT_NBUSY) == 0);

    Sleep(100);
    } while (!kbhit());

  while (kbhit())
    getch();

  }
#endif _MSC_VER

}

/*------------------------------------------------------------------*/

int mscb_addr(int fd, int cmd, int adr)
/********************************************************************\

  Routine: mscb_addr

  Purpose: Address node or nodes

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

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving ping acknowledge
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[10];
int i;

  buf[0] = cmd;

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;
  
  if (cmd == CMD_ADDR_NODE8 ||
      cmd == CMD_ADDR_GRP8 ||
      cmd == CMD_PING8)
    {
    buf[1] = (unsigned char) adr;
    buf[2] = crc8(buf, 2);
    mscb_out(fd, buf, 3, TRUE);
    }
  else if (cmd == CMD_ADDR_NODE16 ||
           cmd == CMD_ADDR_GRP16 ||
           cmd == CMD_PING16)
    {
    buf[1] = (unsigned char) (adr >> 8);
    buf[2] = (unsigned char) (adr & 0xFF);
    buf[3] = crc8(buf, 3);
    mscb_out(fd, buf, 4, TRUE);
    }
  else
    {
    buf[1] = crc8(buf, 1);
    mscb_out(fd, buf, 2, TRUE);
    }

  if (cmd == CMD_PING8 || cmd == CMD_PING16)
    {
    /* read back ping reply, 1ms timeout */
    i = mscb_in1(fd, buf, 1000);

    mscb_release();
    
    if (i == MSCB_SUCCESS && buf[0] == CMD_ACK)
      return MSCB_SUCCESS;

    return  MSCB_TIMEOUT;
    }

  mscb_release();

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_reset(int fd)
/********************************************************************\

  Routine: mscb_reset

  Purpose: Reset addressed node(s) by sending CMD_INIT

  Input:
    int fd                  File descriptor for connection

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[10];

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_INIT;
  buf[1] = crc8(buf, 1);
  mscb_out(fd, buf, 2, FALSE);

  mscb_release();

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_baud(int fd, int baud)
/********************************************************************\

  Routine: mscb_set_baud

  Purpose: Set baud rate of all addressed nodes and submaster

  Input:
    int fd                  File descriptor for connection
    int baud                Baud rate. One of the BD_xxxx values
                            defined in mscb.h

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int status;
unsigned char buf[10];

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  /* address all nodes */
  status = mscb_addr(fd, CMD_ADDR_BC, 0);
  if (status != MSCB_SUCCESS)
    {
    mscb_release();
    return status;
    }

  buf[0] = CMD_SET_BAUD;
  buf[1] = baud;
  buf[2] = crc8(buf, 2);

  status = mscb_out(fd, buf, 3, FALSE);

  mscb_release();

  return status;
}

/*------------------------------------------------------------------*/

int mscb_info(int fd, MSCB_INFO *info)
/********************************************************************\

  Routine: mscb_info

  Purpose: Retrieve info on addressd node

  Input:
    int fd                  File descriptor for connection

  Output:
    MSCB_INFO *info         Info structure defined in mscb.h

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving data
    MSCB_CRC_ERROR          CRC error
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int i;
unsigned char buf[80];

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_GET_INFO;
  buf[1] = crc8(buf, 1);
  mscb_out(fd, buf, 2, FALSE);

  i = mscb_in(fd, buf, sizeof(buf), 5000);
  mscb_release();
  
  if (i<sizeof(MSCB_INFO)+2)
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

int mscb_info_channel(int fd, int type, int index, MSCB_INFO_CHN *info)
/********************************************************************\

  Routine: mscb_info_channel

  Purpose: Retrieve info on a specific channel

  Input:
    int fd                  File descriptor for connection
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
int i;
unsigned char buf[80];

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_GET_INFO+2;
  buf[1] = type;
  buf[2] = index;
  buf[3] = crc8(buf, 3);
  mscb_out(fd, buf, 4, FALSE);

  i = mscb_in(fd, buf, sizeof(buf), 5000);
  mscb_release();

  if (i<sizeof(MSCB_INFO_CHN)+2)
    return MSCB_TIMEOUT;
  
  memcpy(info, buf+2, sizeof(MSCB_INFO_CHN));

  /* do CRC check */
  if (crc8((void *)info, sizeof(MSCB_INFO_CHN)) != buf[sizeof(MSCB_INFO_CHN)+2])
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_set_addr(int fd, int node, int group)
/********************************************************************\

  Routine: mscb_set_addr

  Purpose: Set node and group address of an addressed node

  Input:
    int fd                  File descriptor for connection
    int node                16-bit node address
    int group               16-bit group address
                              
  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[8];

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_SET_ADDR;
  buf[1] = (unsigned char ) (node >> 8);
  buf[2] = (unsigned char ) (node & 0xFF);
  buf[3] = (unsigned char ) (group >> 8);  
  buf[4] = (unsigned char ) (group & 0xFF);
  buf[5] = crc8(buf, 5);
  mscb_out(fd, buf, 6, FALSE);

  mscb_release();

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_na(int fd, unsigned char channel, unsigned int data, int size)
/********************************************************************\

  Routine: mscb_write_na

  Purpose: Write data to channel on addressed node

  Input:
    int fd                  File descriptor for connection
    unsigned char channel   Channel index 0..255
    unsigned int  data      Data to send
    int size                Data size in bytes 1..4 for byte, word, 
                            and dword

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int i;
unsigned int d;
unsigned char buf[10];

  if (size > 4 || size < 1)
    return MSCB_INVAL_PARAM;

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_WRITE_NA+size+1;
  buf[1] = channel;

  for (i=0,d=data ; i<size ; i++)
    {
    buf[2+size-1-i] = d & 0xFF;
    d >>= 8;
    }

  buf[2+i] = crc8(buf, 2+i);
  mscb_out(fd, buf, 3+i, FALSE);

  mscb_release();

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write(int fd, unsigned char channel, unsigned int data, int size)
/********************************************************************\

  Routine: mscb_write

  Purpose: Write data to channel on addressed node with acknowledge

  Input:
    int fd                  File descriptor for connection
    unsigned char channel   Channel index 0..255
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
int           i;
unsigned int  d;
unsigned char buf[10], crc, ack[2];

  if (size > 4 || size < 1)
    return MSCB_INVAL_PARAM;

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_WRITE_ACK+size+1;
  buf[1] = channel;

  for (i=0,d=data ; i<size ; i++)
    {
    buf[2+size-1-i] = d & 0xFF;
    d >>= 8;
    }

  crc = crc8(buf, 2+i);
  buf[2+i] = crc;
  mscb_out(fd, buf, 3+i, FALSE);

  /* read acknowledge */
  i = mscb_in(fd, ack, 2, 5000);
  mscb_release();
  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_conf(int fd, unsigned char channel, unsigned int data, int size)
/********************************************************************\

  Routine: mscb_write_conf

  Purpose: Write configuration parameter to addressed node with acknowledge

  Input:
    int fd                  File descriptor for connection
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
int           i;
unsigned int  d;
unsigned char buf[10], crc, ack[2];

  if (size > 4 || size < 1)
    return MSCB_INVAL_PARAM;

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_WRITE_CONF+size+1;
  buf[1] = channel;

  for (i=0,d=data ; i<size ; i++)
    {
    buf[2+size-1-i] = d & 0xFF;
    d >>= 8;
    }

  crc = crc8(buf, 2+i);
  buf[2+i] = crc;
  mscb_out(fd, buf, 3+i, FALSE);

  /* read acknowledge, 100ms timeout */
  i = mscb_in(fd, ack, 2, 100000);
  mscb_release();

  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_flash(int fd)
/********************************************************************\

  Routine: mscb_flash

  Purpose: Flash configuration parameter and channels values to
           EEPROM
           

  Input:
    int fd                  File descriptor for connection
                            and dword

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int           i;
unsigned char buf[10], crc, ack[2];

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_FLASH;
  crc = crc8(buf, 1);
  buf[1] = crc;
  mscb_out(fd, buf, 2, FALSE);

  /* read acknowledge, 100ms timeout */
  i = mscb_in(fd, ack, 2, 100000);
  mscb_release();

  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read(int fd, unsigned char channel, unsigned int *data)
/********************************************************************\

  Routine: mscb_read

  Purpose: Read data from channel on addressed node

  Input:
    int fd                  File descriptor for connection
    unsigned char channel   Channel index 0..255
    int size                Data size in bytes 1..4 for byte, word, 
                            and dword for data to receive

  Output:
    unsigned int data       Read data

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int           i;
unsigned char buf[10], crc;

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_READ;
  buf[1] = channel;
  buf[2] = crc8(buf, 2);
  mscb_out(fd, buf, 3, FALSE);

  /* read data */
  i = mscb_in(fd, buf, 10, 5000);
  mscb_release();

  if (i<3)
    return MSCB_TIMEOUT;

  crc = crc8(buf, i-1);

  if (buf[0] != CMD_ACK+i-2 || buf[i-1] != crc)
    return MSCB_CRC_ERROR;

  *data = 0;
  memcpy(data, buf+1, i-2);
  if (i-2 == 2)
    WORD_SWAP(data);
  if (i-2 == 4)
    DWORD_SWAP(data);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read_conf(int fd, unsigned char index, unsigned int *data)
/********************************************************************\

  Routine: mscb_read_conf

  Purpose: Read configuration parameter on addressed node

  Input:
    int fd                  File descriptor for connection
    unsigned char index     Parameter index 0..255
    int size                Size of data buffer

  Output:
    unsigned int data       Read data

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Parameter "size" has invalid value
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
int           i;
unsigned char buf[10], crc;

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_READ_CONF;
  buf[1] = index;
  buf[2] = crc8(buf, 2);
  mscb_out(fd, buf, 3, FALSE);

  /* read data */
  i = mscb_in(fd, buf, 10, 5000);
  mscb_release();

  if (i<3)
    return MSCB_TIMEOUT;

  crc = crc8(buf, i-1);

  if (buf[0] != CMD_ACK+i-2 || buf[i-1] != crc)
    return MSCB_CRC_ERROR;

  *data = 0;
  memcpy(data, buf+1, i-2);
  if (i-2 == 2)
    WORD_SWAP(data);
  if (i-2 == 4)
    DWORD_SWAP(data);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_user(int fd, void *param, int size, void *result, int *rsize)
/********************************************************************\

  Routine: mscb_user

  Purpose: Call user function on node

  Input:
    int  fd                 File descriptor for connection
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

\********************************************************************/
{
int i, n, status;
unsigned char buf[80];

  if (mscb_lock() != MSCB_SUCCESS)
    return MSCB_MUTEX;

  buf[0] = CMD_USER+size;

  for (i=0 ; i<size ; i++)
    buf[1+i] = ((char *)param)[i];

  /* add CRC code and send data */
  buf[1+i] = crc8(buf, 1+i);
  status = mscb_out(fd, buf, 2+i, FALSE);
  if (status != MSCB_SUCCESS)
    {
    mscb_release();
    return status;
    }

  if (result == NULL)
    {
    mscb_release();
    return MSCB_SUCCESS;
    }

  /* read result */
  n = mscb_in(fd, buf, sizeof(buf), 5000);
  mscb_release();

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

static int _fd = 0;

int mscb_write16(char *device, unsigned short addr, unsigned char channel, unsigned short data)
/********************************************************************\

  Routine: mscb_write16

  Purpose: Write data to channel on a node with acknowledge

  Input:
    int  parport            Either 1 (lpt1) or 2 (lpt2)
    unsigned int  addr      Node address
    unsigned char channel   Channel index 0..255
    unsigned int  data      Data to send

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Cannot open device
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[10], ack[10], crc;
int           i;

  if (_fd == 0)
    {
    _fd = mscb_init(device);
    if (_fd < 0)
      return MSCB_INVAL_PARAM;
    }

  mscb_lock();

  /* send address command */
  buf[0] = CMD_ADDR_NODE16;
  buf[1] = (unsigned char) (addr >> 8);
  buf[2] = (unsigned char) (addr & 0xFF);
  buf[3] = crc8(buf, 3);
  mscb_out(_fd, buf, 4, TRUE);

  /* send write command */
  buf[0] = CMD_WRITE_ACK+3;
  buf[1] = channel;
  buf[2] = (data >> 8);
  buf[3] = data & 0xFF;
  crc = crc8(buf, 4);
  buf[4] = crc;
  mscb_out(_fd, buf, 5, FALSE);

  /* read acknowledge */
  i = mscb_in(_fd, ack, 2, 5000);
  mscb_release();
  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_write_conf16(char *device, unsigned short addr, unsigned char index,
                      unsigned short data)
/********************************************************************\

  Routine: mscb_write_conf16

  Purpose: Write configuration parameter to a node with acknowledge

  Input:
    int  parport            Either 1 (lpt1) or 2 (lpt2)
    unsigned int  addr      Node address
    unsigned char index     Parameter index 0..254, 255 for node CSR
    unsigned int  data      Data to send

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Cannot open device
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[10], ack[10], crc;
int           i;

  if (_fd == 0)
    {
    _fd = mscb_init(device);
    if (_fd < 0)
      return MSCB_INVAL_PARAM;
    }

  mscb_lock();

  /* send address command */
  buf[0] = CMD_ADDR_NODE16;
  buf[1] = (unsigned char) (addr >> 8);
  buf[2] = (unsigned char) (addr & 0xFF);
  buf[3] = crc8(buf, 3);
  mscb_out(_fd, buf, 4, TRUE);

  /* send write command */
  buf[0] = CMD_WRITE_CONF+3;
  buf[1] = index;
  buf[2] = (data >> 8);
  buf[3] = data & 0xFF;
  crc = crc8(buf, 4);
  buf[4] = crc;
  mscb_out(_fd, buf, 5, FALSE);

  /* read acknowledge */
  i = mscb_in(_fd, ack, 2, 5000);
  mscb_release();
  if (i<2)
    return MSCB_TIMEOUT;

  if (ack[0] != CMD_ACK || ack[1] != crc)
    return MSCB_CRC_ERROR;

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read16(char *device, unsigned short addr, unsigned char channel, unsigned short *data)
/********************************************************************\

  Routine: mscb_read16

  Purpose: Read data from channel on a node

  Input:
    char *device            Device name passed to mscb_init
    unsigned int  addr      Node address
    unsigned char channel   Channel index 0..255

  Output
    unsigned int  *data     Received data

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Cannot open device
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[10], crc;
int           i;

  if (_fd == 0)
    {
    _fd = mscb_init(device);
    if (_fd < 0)
      return MSCB_INVAL_PARAM;
    }

  mscb_lock();

  /* send address command */
  buf[0] = CMD_ADDR_NODE16;
  buf[1] = (unsigned char) (addr >> 8);
  buf[2] = (unsigned char) (addr & 0xFF);
  buf[3] = crc8(buf, 3);
  mscb_out(_fd, buf, 4, TRUE);

  /* send read command */
  buf[0] = CMD_READ;
  buf[1] = channel;
  buf[2] = crc8(buf, 2);
  mscb_out(_fd, buf, 3, FALSE);

  /* read data */
  i = mscb_in(_fd, buf, 10, 5000);
  mscb_release();

  if (i<3)
    return MSCB_TIMEOUT;

  crc = crc8(buf, i-1);

  if (buf[0] != CMD_ACK+i-2 || buf[i-1] != crc)
    return MSCB_CRC_ERROR;

  *data = *((unsigned short *)(buf+1));
  WORD_SWAP(data);

  return MSCB_SUCCESS;
}

/*------------------------------------------------------------------*/

int mscb_read_conf16(char *device, unsigned short addr, unsigned char index, 
                     unsigned short *data)
/********************************************************************\

  Routine: mscb_read_conf16

  Purpose: Read configuration parameter from node

  Input:
    char *device            Device name passed to mscb_init
    unsigned int  addr      Node address
    unsigned char index     Parameter index 0..254, 255 for node CSR

  Output:
    unsigned int  *data     Received data

  Function value:
    MSCB_SUCCESS            Successful completion
    MSCB_TIMEOUT            Timeout receiving acknowledge
    MSCB_CRC_ERROR          CRC error
    MSCB_INVAL_PARAM        Cannot open device
    MSCB_MUTEX              Cannot obtain mutex for mscb

\********************************************************************/
{
unsigned char buf[10], crc;
int           i;

  if (_fd == 0)
    {
    _fd = mscb_init(device);
    if (_fd < 0)
      return MSCB_INVAL_PARAM;
    }

  mscb_lock();

  /* send address command */
  buf[0] = CMD_ADDR_NODE16;
  buf[1] = (unsigned char) (addr >> 8);
  buf[2] = (unsigned char) (addr & 0xFF);
  buf[3] = crc8(buf, 3);
  mscb_out(_fd, buf, 4, TRUE);

  /* send read command */
  buf[0] = CMD_READ_CONF;
  buf[1] = index;
  buf[2] = crc8(buf, 2);
  mscb_out(_fd, buf, 3, FALSE);

  /* read data */
  i = mscb_in(_fd, buf, 10, 5000);
  mscb_release();

  if (i<3)
    return MSCB_TIMEOUT;

  crc = crc8(buf, i-1);

  if (buf[0] != CMD_ACK+i-2 || buf[i-1] != crc)
    return MSCB_CRC_ERROR;

  *data = *((unsigned short *)(buf+1));
  WORD_SWAP(data);

  return MSCB_SUCCESS;
}
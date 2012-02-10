/********************************************************************\

  Name:         mscb.h
  Created by:   Stefan Ritt

  Contents:     Header fiel for MSCB funcions

  $Id: mscb.h 4786 2010-07-19 16:44:45Z olchanski $

\********************************************************************/

#ifndef INCLUDE_MSCB_H
#define INCLUDE_MSCB_H

#ifdef HAVE_USB
#include <musbstd.h>
#endif

/*---- MSCB commands -----------------------------------------------*/

#define MCMD_ADDR_NODE8  0x09
#define MCMD_ADDR_NODE16 0x0A
#define MCMD_ADDR_BC     0x10
#define MCMD_ADDR_GRP8   0x11
#define MCMD_ADDR_GRP16  0x12
#define MCMD_PING8       0x19
#define MCMD_PING16      0x1A

#define MCMD_INIT        0x20
#define MCMD_GET_INFO    0x28
#define MCMD_SET_ADDR    0x33
#define MCMD_SET_NAME    0x37
#define MCMD_SET_BAUD    0x39

#define MCMD_FREEZE      0x41
#define MCMD_SYNC        0x49
#define MCMD_SET_TIME    0x4E
#define MCMD_UPGRADE     0x50
#define MCMD_USER        0x58

#define MCMD_ECHO        0x61
#define MCMD_TOKEN       0x68
#define MCMD_GET_UPTIME  0x70

#define MCMD_ACK         0x78

#define MCMD_WRITE_NA    0x80
#define MCMD_WRITE_ACK   0x88

#define MCMD_FLASH       0x98

#define MCMD_READ        0xA0

#define MCMD_WRITE_RANGE 0xA8

#define GET_INFO_GENERAL   0
#define GET_INFO_VARIABLE  1

#define ADDR_SET_NODE      1
#define ADDR_SET_HIGH      2
#define ADDR_SET_GROUP     3

#define UCMD_ECHO          1
#define UCMD_ERASE         2
#define UCMD_PROGRAM       3
#define UCMD_VERIFY        4
#define UCMD_READ          5
#define UCMD_REBOOT        6
#define UCMD_RETURN        7

/*---- flags from the configuration and status register (CSR) ------*/

#define CSR_DEBUG       (1<<0)
#define CSR_LCD_PRESENT (1<<1)
#define CSR_SYNC_MODE   (1<<2)
#define CSR_FREEZE_MODE (1<<3)
#define CSR_WD_RESET    (1<<2)

/*---- baud rates used ---------------------------------------------*/

#define BD_2400            1
#define BD_4800            2
#define BD_9600            3
#define BD_19200           4
#define BD_28800           5
#define BD_38400           6
#define BD_57600           7
#define BD_115200          8
#define BD_172800          9
#define BD_345600         10

/*---- info structures ---------------------------------------------*/

typedef struct {
   unsigned char  protocol_version;
   unsigned char  n_variables;
   unsigned short node_address;
   unsigned short group_address;
   unsigned short svn_revision;
   char           node_name[16];
   unsigned char  rtc[6];
} MSCB_INFO;

typedef struct {
   unsigned char width;         // width in bytes
   unsigned char unit;          // physical units UNIT_xxxx
   unsigned char prefix;        // unit prefix PRFX_xxx
   unsigned char status;        // status (not yet used)
   unsigned char flags;         // flags MSCBF_xxx
   char name[8];                // name
} MSCB_INFO_VAR;

#define MSCBF_FLOAT    (1<<0)   // channel in floating point format
#define MSCBF_SIGNED   (1<<1)   // channel is signed integer
#define MSCBF_DATALESS (1<<2)   // channel doesn't contain data
#define MSCBF_HIDDEN   (1<<3)   // used for internal config parameters
#define MSCBF_REMIN    (1<<4)   // get variable from remote node on subbus
#define MSCBF_REMOUT   (1<<5)   // send variable to remote node on subbus

/* physical units */

#define PRFX_PICO       -12
#define PRFX_NANO        -9
#define PRFX_MICRO       -6
#define PRFX_MILLI       -3
#define PRFX_NONE         0
#define PRFX_KILO         3
#define PRFX_MEGA         6
#define PRFX_GIGA         9
#define PRFX_TERA        12

#define UNIT_UNDEFINED    0

// SI base units
#define UNIT_METER        1
#define UNIT_GRAM         2
#define UNIT_SECOND       3
#define UNIT_MINUTE       4
#define UNIT_HOUR         5
#define UNIT_AMPERE       6
#define UNIT_KELVIN       7
#define UNIT_CELSIUS      8
#define UNIT_FARENHEIT    9

// SI derived units

#define UNIT_HERTZ       20
#define UNIT_PASCAL      21
#define UNIT_BAR         22
#define UNIT_WATT        23
#define UNIT_VOLT        24
#define UNIT_OHM         25
#define UNIT_TESLA       26
#define UNIT_LITERPERSEC 27
#define UNIT_RPM         28
#define UNIT_FARAD       29

// computer units

#define UNIT_BOOLEAN     50
#define UNIT_BYTE        52
#define UNIT_WORD        53
#define UNIT_DWORD       54
#define UNIT_ASCII       55
#define UNIT_STRING      56
#define UNIT_BAUD        57

// others

#define UNIT_PERCENT     90
#define UNIT_PPM         91
#define UNIT_COUNT       92
#define UNIT_FACTOR      93


/*---- file descriptor ---------------------------------------------*/

#define MSCB_MAX_FD      30

#define MSCB_TYPE_USB     1
#define MSCB_TYPE_ETH     2
#define MSCB_TYPE_RPC     3

typedef struct {
   char device[256];
   int type;
   int fd;
   int remote_fd;
   int semaphore_handle;
   int seq_nr;
   unsigned char eth_addr[16];
#ifdef HAVE_USB
   MUSB_INTERFACE *ui;
#endif
   int eth_max_retry;
} MSCB_FD;

extern MSCB_FD mscb_fd[MSCB_MAX_FD];

/*---- status codes ------------------------------------------------*/

#define MSCB_SUCCESS        1
#define MSCB_CRC_ERROR      2
#define MSCB_TIMEOUT        3
#define MSCB_TIMEOUT_BUS    4
#define MSCB_INVAL_PARAM    5
#define MSCB_SEMAPHORE      6
#define MSCB_FORMAT_ERROR   7
#define MSCB_NO_MEM         8
#define MSCB_SUBM_ERROR     9
#define MSCB_ADDR_EXISTS   10
#define MSCB_WRONG_PASS    11
#define MSCB_SUBADDR       12
#define MSCB_NOTREADY      13
#define MSCB_NO_VAR        14
#define MSCB_INVALID_INDEX 15

/*---- error codes ------------------------------------------------*/

#define EMSCB_UNDEFINED          -1
#define EMSCB_NO_MEM             -2
#define EMSCB_RPC_ERROR          -3
#define EMSCB_NO_ACCESS          -4
#define EMSCB_LOCKED             -5
#define EMSCB_NO_SUBM            -6
#define EMSCB_INVAL_PARAM        -7
#define EMSCB_WRONG_PASSWORD     -8
#define EMSCB_COMM_ERROR         -9
#define EMSCB_NOT_FOUND         -10
#define EMSCB_NO_WRITE_ACCESS   -11
#define EMSCB_SUBM_VERSION      -12

/*---- Byte and Word swapping big endian <-> little endian ---------*/
#ifndef WORD_SWAP
#define WORD_SWAP(x) { unsigned char _tmp;                               \
                       _tmp= *((unsigned char *)(x));                    \
                       *((unsigned char *)(x)) = *(((unsigned char *)(x))+1);     \
                       *(((unsigned char *)(x))+1) = _tmp; }
#endif

#ifndef DWORD_SWAP
#define DWORD_SWAP(x) { unsigned char _tmp;                              \
                       _tmp= *((unsigned char *)(x));                    \
                       *((unsigned char *)(x)) = *(((unsigned char *)(x))+3);     \
                       *(((unsigned char *)(x))+3) = _tmp;               \
                       _tmp= *(((unsigned char *)(x))+1);                \
                       *(((unsigned char *)(x))+1) = *(((unsigned char *)(x))+2); \
                       *(((unsigned char *)(x))+2) = _tmp; }
#endif

/*---- function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

/* make functions under WinNT dll exportable */
#ifndef EXPRT
#if defined(_MSC_VER) && defined(EXPORT_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif
#endif

   int EXPRT mscb_init(char *device, int device_size, const char *password, int debug);
   int EXPRT mscb_select_device(char *data, int size, int select);
   void EXPRT mscb_get_device(int fd, char *device, int bufsize);
   void EXPRT mscb_get_version(char *lib_version, char *prot_version);
   void EXPRT mscb_check(char *device, int size);
   int EXPRT mscb_exit(int fd);
   int EXPRT mscb_subm_reset(int fd);
   int EXPRT mscb_reboot(int fd, int addr, int gaddr, int broadcast);
   int EXPRT mscb_ping(int fd, unsigned short adr, int quick);
   int EXPRT mscb_echo(int fd, unsigned short add, unsigned char d1, unsigned char *d2);
   int EXPRT mscb_info(int fd, unsigned short adr, MSCB_INFO * info);
   int EXPRT mscb_info_variable(int fd, unsigned short adr, unsigned char index, MSCB_INFO_VAR * info);
   int EXPRT mscb_uptime(int fd, unsigned short adr, unsigned int *uptime);
   int EXPRT mscb_set_node_addr(int fd, int addr, int gaddr, int broadcast, unsigned short new_addr);
   int EXPRT mscb_set_group_addr(int fd, int addr, int gaddr, int broadcast, unsigned short new_addr);
   int EXPRT mscb_set_name(int fd, unsigned short adr, char *name);
   int EXPRT mscb_write(int fd, unsigned short adr, unsigned char index, void *data, int size);
   int EXPRT mscb_write_no_retries(int fd, unsigned short adr, unsigned char index, void *data, int size);
   int EXPRT mscb_write_group(int fd, unsigned short adr, unsigned char index, void *data, int size);
   int EXPRT mscb_write_range(int fd, unsigned short adr, unsigned char index1, unsigned char index2, void *data, int size);
   int EXPRT mscb_flash(int fd, int adr, int gaddr, int broadcast);
   int EXPRT mscb_set_baud(int fd, int baud);
   int EXPRT mscb_get_max_retry();
   int EXPRT mscb_set_max_retry(int max_retry);
   int EXPRT mscb_get_usb_timeout();
   int EXPRT mscb_set_usb_timeout(int timeout);
   int EXPRT mscb_get_eth_max_retry(int fd);
   int EXPRT mscb_set_eth_max_retry(int fd, int eth_max_retry);
   int EXPRT mscb_upload(int fd, unsigned short adr, unsigned char *buffer, int size, int debug);
   int EXPRT mscb_verify(int fd, unsigned short adr, unsigned char *buffer, int size);
   int EXPRT mscb_read(int fd, unsigned short adr, unsigned char index, void *data, int *size);
   int EXPRT mscb_read_no_retries(int fd, unsigned short adr, unsigned char index, void *data, int *size);
   int EXPRT mscb_read_range(int fd, unsigned short adr, unsigned char index1,
                             unsigned char index2, void *data, int *size);
   int EXPRT mscb_user(int fd, unsigned short adr, void *param, int size, void *result, int *rsize);
   int EXPRT mscb_link(int fd, unsigned short adr, unsigned char index, void *data, int size);
   int EXPRT mscb_addr(int fd, int cmd, unsigned short adr, int retry);
   int EXPRT mscb_set_time(int fd, int addr, int gaddr, int broadcast);

   int set_mac_address(int fd);
   void mscb_scan_udp();
   int mscb_subm_info(int fd);

#ifdef __cplusplus
}
#endif
/* define missing linux functions */
#if defined(OS_LINUX)
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
int kbhit();
#define getch() getchar()
#define Sleep(x) usleep(x*1000)

#endif

/* pointer definition, 64-bit compatible */
#if defined(__alpha) || defined(_LP64)
#define POINTER_T     long int
#else
#define POINTER_T     int
#endif

#endif

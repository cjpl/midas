/********************************************************************\

  Name:         mscb.h
  Created by:   Stefan Ritt

  Contents:     Header fiel for MSCB funcions

  $Log$
  Revision 1.15  2002/11/27 15:40:05  midas
  Added version, fixed few bugs

  Revision 1.14  2002/11/20 12:01:30  midas
  Added host to mscb_init

  Revision 1.13  2002/10/28 14:26:30  midas
  Changes from Japan

  Revision 1.12  2002/10/22 15:03:38  midas
  Added UNIT_FACTOR

  Revision 1.11  2002/10/16 15:25:07  midas
  xxx16 now does 32 bit exchange

  Revision 1.10  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.9  2002/10/07 15:16:32  midas
  Added upgrade facility

  Revision 1.8  2002/10/03 15:30:40  midas
  Added linux support

  Revision 1.7  2002/08/12 12:11:20  midas
  Added mscb_reset1

  Revision 1.6  2002/07/10 09:51:13  midas
  Introduced mscb_flash()

  Revision 1.5  2001/10/31 11:16:54  midas
  Added IO check function

  Revision 1.4  2001/08/31 12:23:51  midas
  Added mutex protection

  Revision 1.3  2001/08/31 11:35:20  midas
  Added "wp" command in msc.c, changed parport to device in mscb.c

  Revision 1.2  2001/08/31 11:05:18  midas
  Added write16 and read16 (for LabView)


\********************************************************************/

/*---- Device types ------------------------------------------------*/

#define MSCB_RS232         1
#define MSCB_PARPORT       2

/*---- MSCB commands -----------------------------------------------*/

#define CMD_ADDR_NODE8  0x09
#define CMD_ADDR_NODE16 0x0A
#define CMD_ADDR_BC     0x10
#define CMD_ADDR_GRP8   0x11
#define CMD_ADDR_GRP16  0x12
#define CMD_PING8       0x19
#define CMD_PING16      0x1A

#define CMD_INIT        0x20
#define CMD_GET_INFO    0x28
#define CMD_SET_ADDR    0x34
#define CMD_SET_BAUD    0x39

#define CMD_FREEZE      0x41
#define CMD_SYNC        0x49
#define CMD_UPGRADE     0x50
#define CMD_USER        0x58

#define CMD_TOKEN       0x60
#define CMD_SET_FLAGS   0x69

#define CMD_ACK         0x78

#define CMD_WRITE_NA    0x80
#define CMD_WRITE_ACK   0x88

#define CMD_WRITE_CONF  0x90
#define CMD_FLASH       0x98

#define CMD_READ        0xA1
#define CMD_READ_CONF   0xA9

#define CMD_WRITE_BLOCK 0xB5
#define CMD_READ_BLOCK  0xB9

#define GET_INFO_GENERAL   0
#define GET_INFO_CHANNEL   1
#define GET_INFO_CONF      2

#define SIZE_0BIT          0
#define SIZE_8BIT          1
#define SIZE_16BIT         2
#define SIZE_24BIT         3
#define SIZE_32BIT         4

/*---- flags from the configuration and status register (CSR) ------*/

#define CSR_DEBUG       (1<<0)
#define CSR_LCD_PRESENT (1<<1)
#define CSR_SYNC_MODE   (1<<2)
#define CSR_FREEZE_MODE (1<<3)
#define CSR_WD_RESET    (1<<2)

/*---- baud rates used ---------------------------------------------*/

#define BD_9600            1
#define BD_19200           2
#define BD_28800           3
#define BD_57600           4 
#define BD_115200          5
#define BD_172800          6
#define BD_345600          7

/*---- info structures ---------------------------------------------*/

typedef struct {
  unsigned char  protocol_version;
  unsigned char  node_status;
  unsigned char  n_channel;
  unsigned char  n_conf;
  unsigned short node_address;
  unsigned short group_address;
  unsigned short watchdog_resets;
  char           node_name[16];
} MSCB_INFO;

typedef struct {
  unsigned char width;    // width in bytes
  unsigned char unit;     // physical units UNIT_xxxx
  unsigned char prefix;   // unit prefix PRFX_xxx
  unsigned char status;   // status (not yet used)
  unsigned char flags;    // flags MSCBF_xxx
  char          name[8];  // name
} MSCB_INFO_CHN;

#define MSCBF_FLOAT  (1<<0) // channel in floating point format
#define MSCBF_SIGNED (1<<1) // channel is signed integer

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

/*---- status codes ------------------------------------------------*/

#define MSCB_SUCCESS       1
#define MSCB_CRC_ERROR     2
#define MSCB_TIMEOUT       3
#define MSCB_INVAL_PARAM   4
#define MSCB_MUTEX         5
#define MSCB_FORMAT_ERROR  6

/*---- function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

/* make functions under WinNT dll exportable */
#if defined(_MSC_VER) && defined(_USRDLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

int EXPRT mscb_init(char *device);
void EXPRT mscb_get_version(char *lib_version, char *prot_version);
void EXPRT mscb_check(char *device);
int EXPRT mscb_exit(int fd);
int EXPRT mscb_reset(int fd);
int EXPRT mscb_reboot(int fd, int adr);
int EXPRT mscb_ping(int fd, int adr);
int EXPRT mscb_info(int fd, int adr, MSCB_INFO *info);
int EXPRT mscb_info_channel(int fd, int adr, int type, int index, MSCB_INFO_CHN *info);
int EXPRT mscb_set_addr(int fd, int adr, int node, int group);
int EXPRT mscb_write(int fd, int adr, unsigned char channel, void *data, int size);
int EXPRT mscb_write_group(int fd, int adr, unsigned char channel, void *data, int size);
int EXPRT mscb_write_conf(int fd, int adr, unsigned char channel, void *data, int size);
int EXPRT mscb_flash(int fd, int adr);
int EXPRT mscb_upload(int fd, int adr, char *buffer, int size);
int EXPRT mscb_read(int fd, int adr, unsigned char channel, void *data, int *size);
int EXPRT mscb_read_conf(int fd, int adr, unsigned char channel, void *data, int *size);
int EXPRT mscb_user(int fd, int adr, void *param, int size, void *result, int *rsize);

#ifdef __cplusplus
}
#endif


/* define missing linux functions */
#ifdef __linux__

int kbhit();
#define getch() getchar()
#define Sleep(x) usleep(x*1000)

#endif

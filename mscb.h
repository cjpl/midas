/********************************************************************\

  Name:         mscb.h
  Created by:   Stefan Ritt

  Contents:     Header fiel for MSCB funcions

  $Log$
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
#define CMD_TRANSP      0x50
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
  unsigned char  channel_width;
  unsigned char  phys_units;
  unsigned char  status;
  unsigned char  flags;
  char           channel_name[8];
  } MSCB_INFO_CHN;

/* status codes */
#define MSCB_SUCCESS       1
#define MSCB_CRC_ERROR     2
#define MSCB_TIMEOUT       3
#define MSCB_INVAL_PARAM   4
#define MSCB_MUTEX         5

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
void EXPRT mscb_check(char *device);
int EXPRT mscb_exit(int fd);
int EXPRT mscb_reset(int fd);
int EXPRT mscb_reboot(int fd);
int EXPRT mscb_set_baud(int fd, int baud);
int EXPRT mscb_addr(int fd, int cmd, int adr);
int EXPRT mscb_info(int fd, MSCB_INFO *info);
int EXPRT mscb_info_channel(int fd, int type, int index, MSCB_INFO_CHN *info);
int EXPRT mscb_set_addr(int fd, int node, int group);
int EXPRT mscb_write(int fd, unsigned char channel, unsigned int data, int size);
int EXPRT mscb_write_na(int fd, unsigned char channel, unsigned int data, int size);
int EXPRT mscb_write_conf(int fd, unsigned char channel, unsigned int data, int size);
int EXPRT mscb_flash();
int EXPRT mscb_read(int fd, unsigned char channel, unsigned int *data);
int EXPRT mscb_read_conf(int fd, unsigned char channel, unsigned int *data);
int EXPRT mscb_user(int fd, void *param, int size, void *result, int *rsize);

int EXPRT mscb_write16(char *device, unsigned short addr, unsigned char channel, unsigned short data);
int EXPRT mscb_write_conf16(char *device, unsigned short addr, unsigned char channel, unsigned short data);

int EXPRT mscb_read16(char *device, unsigned short addr, unsigned char channel, unsigned short *data);
int EXPRT mscb_read_conf16(char *device, unsigned short addr, unsigned char channel, unsigned short *data);

int EXPRT mscb_reset1(char *device);

#ifdef __cplusplus
}
#endif

/********************************************************************\

  Name:         rpc.h
  Created by:   Stefan Ritt

  Contents:     Header fiel for MSCB RPC funcions

  $Log$
  Revision 1.1  2002/10/28 14:26:30  midas
  Changes from Japan


\********************************************************************/
        
/* Data types */        /*                      min      max    */
#define TID_BYTE      1 /* unsigned byte         0       255    */
#define TID_SBYTE     2 /* signed byte         -128      127    */
#define TID_CHAR      3 /* single character      0       255    */
#define TID_WORD      4 /* two bytes             0      65535   */
#define TID_SHORT     5 /* signed word        -32768    32767   */
#define TID_DWORD     6 /* four bytes            0      2^32-1  */
#define TID_INT       7 /* signed dword        -2^31    2^31-1  */
#define TID_BOOL      8 /* four bytes bool       0        1     */
#define TID_FLOAT     9 /* 4 Byte float format                  */
#define TID_DOUBLE   10 /* 8 Byte float format                  */
#define TID_BITFIELD 11 /* 32 Bits Bitfield      0  111... (32) */
#define TID_STRING   12 /* zero terminated string               */
#define TID_ARRAY    13 /* array with unknown contents          */
#define TID_STRUCT   14 /* structure with fixed length          */
#define TID_KEY      15 /* key in online database               */
#define TID_LINK     16 /* link in online database              */
#define TID_LAST     17 /* end of TID list indicator            */

/* RPC options */
#define RPC_OTIMEOUT       1
#define RPC_OTRANSPORT     2
#define RPC_OCONVERT_FLAG  3
#define RPC_OHW_TYPE       4
#define RPC_OSERVER_TYPE   5
#define RPC_OSERVER_NAME   6
#define RPC_CONVERT_FLAGS  7
#define RPC_ODB_HANDLE     8
#define RPC_CLIENT_HANDLE  9
#define RPC_SEND_SOCK      10
#define RPC_WATCHDOG_TIMEOUT 11

#define RPC_TCP            0
#define RPC_FTCP           1

/* flags */
#define RPC_IN       (1 << 0)
#define RPC_OUT      (1 << 1)
#define RPC_POINTER  (1 << 2)
#define RPC_FIXARRAY (1 << 3)
#define RPC_VARARRAY (1 << 4)
#define RPC_OUTGOING (1 << 5)

/* rpc parameter list */

typedef struct {
  unsigned short tid;
  unsigned short flags;
  int            n;
} RPC_PARAM;


typedef struct {
  int           id;
  char          *name;
  RPC_PARAM     param[20];
  int           (*dispatch)(int,void**);
} RPC_LIST;

/* function list */

#define RPC_MSCB_INIT              1
#define RPC_MSCB_EXIT              2
#define RPC_MSCB_RESET             3
#define RPC_MSCB_REBOOT            4
#define RPC_MSCB_PING              5
#define RPC_MSCB_INFO              6
#define RPC_MSCB_INFO_CHANNEL      7
#define RPC_MSCB_SET_ADDR          8
#define RPC_MSCB_WRITE             9
#define RPC_MSCB_WRITE_GROUP      10
#define RPC_MSCB_WRITE_CONF       11
#define RPC_MSCB_FLASH            12
#define RPC_MSCB_UPLOAD           13
#define RPC_MSCB_READ             14
#define RPC_MSCB_READ_CONF        15
#define RPC_MSCB_USER             16

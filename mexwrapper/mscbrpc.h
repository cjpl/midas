/********************************************************************\

  Name:         rpc.h
  Created by:   Stefan Ritt

  Contents:     Header fiel for MSCB RPC funcions

  $Id: mscbrpc.h 4283 2008-08-04 15:26:21Z ritt@PSI.CH $

\********************************************************************/

/* Data types */                /*                      min      max    */

#define TID_BYTE      1         /* unsigned byte         0       255    */
#define TID_SBYTE     2         /* signed byte         -128      127    */
#define TID_CHAR      3         /* single character      0       255    */
#define TID_WORD      4         /* two bytes             0      65535   */
#define TID_SHORT     5         /* signed word        -32768    32767   */
#define TID_DWORD     6         /* four bytes            0      2^32-1  */
#define TID_INT       7         /* signed dword        -2^31    2^31-1  */
#define TID_BOOL      8         /* four bytes bool       0        1     */
#define TID_FLOAT     9         /* 4 Byte float format                  */
#define TID_DOUBLE   10         /* 8 Byte float format                  */
#define TID_BITFIELD 11         /* 32 Bits Bitfield      0  111... (32) */
#define TID_STRING   12         /* zero terminated string               */
#define TID_ARRAY    13         /* array with unknown contents          */
#define TID_STRUCT   14         /* structure with fixed length          */
#define TID_KEY      15         /* key in online database               */
#define TID_LINK     16         /* link in online database              */
#define TID_LAST     17         /* end of TID list indicator            */

#define CBYTE(_i)        (* ((unsigned char *)   prpc_param[_i]))
#define CPBYTE(_i)       (  ((unsigned char *)   prpc_param[_i]))

#define CSHORT(_i)       (* ((short *)           prpc_param[_i]))
#define CPSHORT(_i)      (  ((short *)           prpc_param[_i]))

#define CINT(_i)         (* ((int *)             prpc_param[_i]))
#define CPINT(_i)        (  ((int *)             prpc_param[_i]))

#define CWORD(_i)        (* ((unsigned short *)  prpc_param[_i]))
#define CPWORD(_i)       (  ((unsigned short *)  prpc_param[_i]))

#define CLONG(_i)        (* ((long *)            prpc_param[_i]))
#define CPLONG(_i)       (  ((long *)            prpc_param[_i]))

#define CDWORD(_i)       (* ((unsigned int *)    prpc_param[_i]))
#define CPDWORD(_i)      (  ((unsigned int *)    prpc_param[_i]))

#define CBOOL(_i)        (* ((int *)             prpc_param[_i]))
#define CPBOOL(_i)       (  ((int *)             prpc_param[_i]))

#define CFLOAT(_i)       (* ((float *)           prpc_param[_i]))
#define CPFLOAT(_i)      (  ((float *)           prpc_param[_i]))

#define CDOUBLE(_i)      (* ((double *)          prpc_param[_i]))
#define CPDOUBLE(_i)     (  ((double *)          prpc_param[_i]))

#define CSTRING(_i)      (  ((char *)            prpc_param[_i]))
#define CARRAY(_i)       (  ((void *)            prpc_param[_i]))

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
   int n;
} RPC_PARAM;

typedef struct {
   int id;
   char *name;
   RPC_PARAM param[20];
} RPC_LIST;

/* function list */

#define RPC_MSCB_INIT              1
#define RPC_MSCB_EXIT              2
#define RPC_MSCB_SUBM_RESET        3
#define RPC_MSCB_REBOOT            4
#define RPC_MSCB_PING              5
#define RPC_MSCB_INFO              6
#define RPC_MSCB_INFO_VARIABLE     7
#define RPC_MSCB_UPTIME            8
#define RPC_MSCB_SET_NODE_ADDR     9 
#define RPC_MSCB_SET_GROUP_ADDR   10
#define RPC_MSCB_WRITE            11
#define RPC_MSCB_WRITE_NO_RETRIES 12
#define RPC_MSCB_WRITE_GROUP      13
#define RPC_MSCB_WRITE_RANGE      14
#define RPC_MSCB_FLASH            15
#define RPC_MSCB_UPLOAD           16
#define RPC_MSCB_VERIFY           17
#define RPC_MSCB_READ             18
#define RPC_MSCB_READ_NO_RETRIES  19
#define RPC_MSCB_READ_RANGE       20
#define RPC_MSCB_USER             21
#define RPC_MSCB_ECHO             22
#define RPC_MSCB_SET_NAME         23
#define RPC_MSCB_ADDR             24
#define RPC_MSCB_GET_DEVICE       25
#define RPC_MSCB_SET_BAUD         26
#define RPC_MSCB_SET_TIME         27

/*------------------------------------------------------------------*/

/* Network structures */

typedef struct {
   int routine_id;              /* routine ID like ID_BM_xxx    */
   int param_size;              /* size in Bytes of parameter   */
} NET_COMMAND_HEADER;

typedef struct {
   NET_COMMAND_HEADER header;
   char param[32];              /* parameter array              */
} NET_COMMAND;

/* default listen port */
#define MSCB_RPC_PORT           1176
#define MSCB_NET_PORT           1177

/* rpc timeout in milliseconds */
#define RPC_TIMEOUT            10000

/* RPC error codes */
#define RPC_SUCCESS                1
#define RPC_NET_ERROR              2
#define RPC_INVALID_ID             3
#define RPC_EXCEED_BUFFER          4
#define RPC_ERR_TIMEOUT            5

/* Align macro for data alignment on 8-byte boundary */
#define ALIGN8(x)  (((x)+7) & ~7)

/* maximal network packed size */
#define NET_TCP_SIZE           65535

/*------------------------------------------------------------------*/

/* function declarations */

void mrpc_server_loop(void);
int mrpc_connect(char *host_name, int port);
int mrpc_connected(int fd);
int mrpc_disconnect(int sock);
int mrpc_call(int fdi, const int routine_id, ...);
int msend_tcp(int sock, char *buffer, int buffer_size);
void mdrain_tcp(int sock);


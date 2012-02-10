/********************************************************************\

  Name:         MRPC.C
  Created by:   Stefan Ritt

  Contents:     List of MSCB RPC functions with parameters

  $Id: mscbrpc.c 4663 2009-12-14 11:19:09Z ritt $

\********************************************************************/

#ifdef _MSC_VER

#pragma warning(disable:4996)

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>

#elif defined(OS_LINUX)

#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define closesocket(s) close(s)

#endif

#include <stdio.h>
#include <assert.h>
#include "musbstd.h"
#include "mscb.h"
#include "mscbrpc.h"

typedef int INT;

/* functions defined in mscb.c */
extern int _debug_flag;
extern void debug_log(char *format, int start, ...);

/*------------------------------------------------------------------*/

static RPC_LIST rpc_list[] = {

   /* common functions */
   {RPC_MSCB_INIT, "mscb_init",
    {{TID_STRING, RPC_IN | RPC_OUT},
     {TID_INT, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_EXIT, "mscb_exit",
    {{TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_GET_DEVICE, "mscb_get_device",
    {{TID_INT, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_REBOOT, "mscb_reboot",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_SUBM_RESET, "mscb_subm_reset",
    {{TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_PING, "mscb_ping",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_INFO, "mscb_info",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_STRUCT, RPC_OUT, sizeof(MSCB_INFO)},
     {0}}},

   {RPC_MSCB_UPTIME, "mscb_uptime",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_DWORD, RPC_OUT},
     {0}}},

   {RPC_MSCB_INFO_VARIABLE, "mscb_info_variable",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_STRUCT, RPC_OUT, sizeof(MSCB_INFO_VAR)},
     {0}}},

   {RPC_MSCB_SET_NODE_ADDR, "mscb_set_node_addr",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_SHORT, RPC_IN},
     {0}}},

   {RPC_MSCB_SET_GROUP_ADDR, "mscb_set_group_addr",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_SHORT, RPC_IN},
     {0}}},

   {RPC_MSCB_SET_NAME, "mscb_set_name",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_MSCB_WRITE_GROUP, "mscb_write_group",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_WRITE, "mscb_write",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_WRITE_RANGE, "mscb_write_range",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_FLASH, "mscb_flash",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_UPLOAD, "mscb_upload",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_VERIFY, "mscb_verify",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_READ, "mscb_read",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT, RPC_IN | RPC_OUT},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_READ_RANGE, "mscb_read_range",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT, RPC_IN | RPC_OUT},
     {0}}},

   {RPC_MSCB_USER, "mscb_user",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT, RPC_IN | RPC_OUT},
     {0}}},

   {RPC_MSCB_ECHO, "mscb_echo",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_BYTE, RPC_IN},
     {TID_BYTE, RPC_OUT},
     {0}}},

   {RPC_MSCB_ADDR, "mscb_addr",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {RPC_MSCB_SET_TIME, "mscb_set_time",
    {{TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {TID_INT, RPC_IN},
     {0}}},

   {0}

};

/*------------------------------------------------------------------*/

/* data type sizes */
INT mtid_size[] = {
   0,                           /* tid == 0 not defined                               */
   1,                           /* TID_BYTE      unsigned byte         0       255    */
   1,                           /* TID_SBYTE     signed byte         -128      127    */
   1,                           /* TID_CHAR      single character      0       255    */
   2,                           /* TID_WORD      two bytes             0      65535   */
   2,                           /* TID_SHORT     signed word        -32768    32767   */
   4,                           /* TID_DWORD     four bytes            0      2^32-1  */
   4,                           /* TID_INT       signed dword        -2^31    2^31-1  */
   4,                           /* TID_BOOL      four bytes bool       0        1     */
   4,                           /* TID_FLOAT     4 Byte float format                  */
   8,                           /* TID_DOUBLE    8 Byte float format                  */
   1,                           /* TID_BITFIELD  8 Bits Bitfield    00000000 11111111 */
   0,                           /* TID_STRING    zero terminated string               */
   0,                           /* TID_ARRAY     variable length array of unkown type */
   0,                           /* TID_STRUCT    C structure                          */
   0,                           /* TID_KEY       key in online database               */
   0                            /* TID_LINK      link in online database              */
};

/*------------------------------------------------------------------*/

#define N_MAX_CONNECTION      10
#define NET_BUFFER_SIZE   100000

int rpc_sock[N_MAX_CONNECTION];
char net_buffer[NET_BUFFER_SIZE];
char recv_buffer[NET_BUFFER_SIZE];

/*------------------------------------------------------------------*/

void mdrain_tcp(int sock)
/* drain socket connection from any old data */
{
   char buffer[100];

   fd_set readfds;
   struct timeval timeout;
   int status;


   /* first receive header */
   do {
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec = 0;
      timeout.tv_usec = 0;

      do {
         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      } while (status == -1);   /* dont return if an alarm signal was cought */

      if (!FD_ISSET(sock, &readfds))
         return;

      status = recv(sock, buffer, sizeof(buffer), 0);

      if (status <= 0)
         return ;

   } while (1);
}

/*------------------------------------------------------------------*/

int mrecv_tcp(int sock, char *buffer, int buffer_size)
{
   INT param_size, n_received, n;
   NET_COMMAND *nc;

   fd_set readfds;
   struct timeval timeout;
   int status;
   int millisec;

   if (buffer_size < (int)sizeof(NET_COMMAND_HEADER)) {
      printf("mrecv_tcp_server: buffer too small\n");
      return -1;
   }

   /* fixed timeout of 2 sec for the moment */
   millisec = 2000;

   /* first receive header */
   n_received = 0;
   do {
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec = millisec / 1000;
      timeout.tv_usec = (millisec % 1000) * 1000;

      do {
         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      } while (status == -1);   /* dont return if an alarm signal was cought */

      if (!FD_ISSET(sock, &readfds))
         return n_received;

      n = recv(sock, buffer + n_received, sizeof(NET_COMMAND_HEADER), 0);

      if (n <= 0)
         return n;

      n_received += n;

   } while (n_received < (int)sizeof(NET_COMMAND_HEADER));

   /* now receive parameters */

   nc = (NET_COMMAND *) buffer;
   param_size = nc->header.param_size;
   n_received = 0;

   if (param_size == 0)
      return sizeof(NET_COMMAND_HEADER);

   do {
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec = millisec / 1000;
      timeout.tv_usec = (millisec % 1000) * 1000;

      do {
         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      } while (status == -1);   /* dont return if an alarm signal was cought */

      if (!FD_ISSET(sock, &readfds))
         return n_received;

      n = recv(sock, buffer + sizeof(NET_COMMAND_HEADER) + n_received,
               param_size - n_received, 0);

      if (n <= 0)
         return n;

      n_received += n;
   } while (n_received < param_size);

   return sizeof(NET_COMMAND_HEADER) + param_size;
}

/*------------------------------------------------------------------*/

int msend_tcp(int sock, char *buffer, int buffer_size)
{
   int count, status;

   /* drain receive queue if any late packet arrived in meantime */
   mdrain_tcp(sock);

   /* transfer fragments until complete buffer is transferred */

   for (count = 0; (int) count < (int) buffer_size - NET_TCP_SIZE;) {
      status = send(sock, buffer + count, NET_TCP_SIZE, 0);
      if (status != -1)
         count += status;
      else
         return status;
   }

   while (count < buffer_size) {
      status = send(sock, buffer + count, buffer_size - count, 0);
      if (status != -1)
         count += status;
      else
         return status;
   }

   return count;
}

/*------------------------------------------------------------------*/

int server_execute(int index, void *prpc_param[])
{
   int status;

   switch (index) {
   default:
      abort();
      break;

   case RPC_MSCB_INIT:
      status = mscb_init(CSTRING(0), CINT(1), CSTRING(2), CINT(3));
      break;

   case RPC_MSCB_EXIT:
      status = mscb_exit(CINT(0));
      break;

   case RPC_MSCB_REBOOT:
      status = mscb_reboot(CINT(0), CINT(1), CINT(2), CINT(3));
      break;

   case RPC_MSCB_SUBM_RESET:
      status = mscb_subm_reset(CINT(0));
      break;

   case RPC_MSCB_PING:
      status = mscb_ping(CINT(0), CWORD(1), CINT(2));
      break;

   case RPC_MSCB_INFO:
      status = mscb_info(CINT(0), CWORD(1), (MSCB_INFO*)CARRAY(2));
      break;

   case RPC_MSCB_INFO_VARIABLE:
      status = mscb_info_variable(CINT(0), CSHORT(1), CBYTE(2), (MSCB_INFO_VAR*)CARRAY(3));
      break;

   case RPC_MSCB_UPTIME:
      status = mscb_uptime(CINT(0), CWORD(1), CPDWORD(2));
      break;

   case RPC_MSCB_SET_NODE_ADDR:
      status = mscb_set_node_addr(CINT(0), CINT(1), CINT(2), CINT(3), CSHORT(4));
      break;

   case RPC_MSCB_SET_GROUP_ADDR:
      status = mscb_set_group_addr(CINT(0), CINT(1), CINT(2), CINT(3), CSHORT(4));
      break;

   case RPC_MSCB_SET_NAME:
      status = mscb_set_name(CINT(0), CSHORT(1), CSTRING(2));
      break;

   case RPC_MSCB_WRITE_GROUP:
      status = mscb_write_group(CINT(0), CSHORT(1), CBYTE(2), CARRAY(3), CINT(4));
      break;

   case RPC_MSCB_WRITE:
      status = mscb_write(CINT(0), CSHORT(1), CBYTE(2), CARRAY(3), CINT(4));
      break;

   case RPC_MSCB_WRITE_NO_RETRIES:
      status = mscb_write(CINT(0), CSHORT(1), CBYTE(2), CARRAY(3), CINT(4));
      break;

   case RPC_MSCB_WRITE_RANGE:
      status = mscb_write_range(CINT(0), CSHORT(1), CBYTE(2), CBYTE(3), CARRAY(4), CINT(5));
      break;

   case RPC_MSCB_FLASH:
      status = mscb_flash(CINT(0), CINT(1), CINT(2), CINT(3));
      break;

   case RPC_MSCB_SET_BAUD:
      status = mscb_set_baud(CINT(0), CINT(1));
      break;

   case RPC_MSCB_UPLOAD:
      status = mscb_upload(CINT(0), CSHORT(1), (unsigned char*)CARRAY(2), CINT(3), CINT(4));
      break;

   case RPC_MSCB_VERIFY:
      status = mscb_verify(CINT(0), CSHORT(1), (unsigned char*)CARRAY(2), CINT(3));
      break;

   case RPC_MSCB_READ:
      status = mscb_read(CINT(0), CSHORT(1), CBYTE(2), CARRAY(3), CPINT(4));
      break;

   case RPC_MSCB_READ_NO_RETRIES:
      status = mscb_read_no_retries(CINT(0), CSHORT(1), CBYTE(2), CARRAY(3), CPINT(4));
      break;

   case RPC_MSCB_READ_RANGE:
      status =
          mscb_read_range(CINT(0), CSHORT(1), CBYTE(2), CBYTE(3), CARRAY(4), CPINT(5));
      break;

   case RPC_MSCB_ECHO:
      status = mscb_echo(CINT(0), CSHORT(1), CBYTE(2), CPBYTE(3));
      break;

   case RPC_MSCB_USER:
      status = mscb_user(CINT(0), CSHORT(1), CARRAY(2), CINT(3), CARRAY(4), CPINT(5));
      break;

   case RPC_MSCB_ADDR:
      status = mscb_addr(CINT(0), CINT(1), CSHORT(2), CINT(3));
      break;

   case RPC_MSCB_SET_TIME:
      status = mscb_set_time(CINT(0), CINT(1), CINT(2), CINT(3));
      break;
   }

   return status;
}

/*------------------------------------------------------------------*/

int mrpc_execute(int sock, char *buffer)
{
   INT i, index, routine_id, status;
   char *in_param_ptr, *out_param_ptr, *last_param_ptr;
   INT tid, flags;
   NET_COMMAND *nc_in, *nc_out;
   INT param_size, max_size;
   void *prpc_param[20];
   char return_buffer[NET_BUFFER_SIZE];

   /* extract pointer array to parameters */
   nc_in = (NET_COMMAND *) buffer;
   nc_out = (NET_COMMAND *) return_buffer;

   /* find entry in rpc_list */
   routine_id = nc_in->header.routine_id;

   for (i = 0;; i++)
      if (rpc_list[i].id == 0 || rpc_list[i].id == routine_id)
         break;
   index = i;
   if (rpc_list[i].id == 0) {
      printf("mrpc_execute: Invalid rpc ID (%d)\n", routine_id);
      return RPC_INVALID_ID;
   }

   in_param_ptr = nc_in->param;
   out_param_ptr = nc_out->param;

   for (i = 0; rpc_list[index].param[i].tid != 0; i++) {
      tid = rpc_list[index].param[i].tid;
      flags = rpc_list[index].param[i].flags;

      if (flags & RPC_IN) {
         param_size = ALIGN8(mtid_size[tid]);

         if (tid == TID_STRING || tid == TID_LINK)
            param_size = ALIGN8(1 + strlen((char *) (in_param_ptr)));

         if (flags & RPC_VARARRAY) {
            /* for arrays, the size is stored as a INT in front of the array */
            param_size = *((INT *) in_param_ptr);
            param_size = ALIGN8(param_size);

            in_param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT)
            param_size = ALIGN8(rpc_list[index].param[i].n);

         prpc_param[i] = in_param_ptr;

         in_param_ptr += param_size;
      }

      if (flags & RPC_OUT) {
         param_size = ALIGN8(mtid_size[tid]);

         if (flags & RPC_VARARRAY || tid == TID_STRING) {
            /* save maximum array length */
            max_size = *((INT *) in_param_ptr);
            max_size = ALIGN8(max_size);

            *((INT *) out_param_ptr) = max_size;

            /* save space for return array length */
            out_param_ptr += ALIGN8(sizeof(INT));

            /* use maximum array length from input */
            param_size += max_size;
         }

         if (rpc_list[index].param[i].tid == TID_STRUCT)
            param_size = ALIGN8(rpc_list[index].param[i].n);

         if ((POINTER_T) out_param_ptr - (POINTER_T) nc_out + param_size > NET_BUFFER_SIZE) {
            printf
                ("mrpc_execute: return parameters (%d) too large for network buffer (%d)\n",
                 (int)((POINTER_T) out_param_ptr - (POINTER_T) nc_out + param_size), NET_BUFFER_SIZE);
            return RPC_EXCEED_BUFFER;
         }

         /* if parameter goes both directions, copy input to output */
         if (rpc_list[index].param[i].flags & RPC_IN)
            memcpy(out_param_ptr, prpc_param[i], param_size);

         prpc_param[i] = out_param_ptr;
         out_param_ptr += param_size;
      }

   }

   last_param_ptr = out_param_ptr;

  /*********************************\
  *   call dispatch function        *
  \*********************************/
   status = server_execute(routine_id, prpc_param);

   /* compress variable length arrays */
   out_param_ptr = nc_out->param;
   for (i = 0; rpc_list[index].param[i].tid != 0; i++)
      if (rpc_list[index].param[i].flags & RPC_OUT) {
         tid = rpc_list[index].param[i].tid;
         flags = rpc_list[index].param[i].flags;
         param_size = ALIGN8(mtid_size[tid]);

         if (tid == TID_STRING) {
            max_size = *((INT *) out_param_ptr);
            param_size = strlen((char *) prpc_param[i]) + 1;
            param_size = ALIGN8(param_size);

            /* move string ALIGN8(sizeof(INT)) left */
            memcpy(out_param_ptr, out_param_ptr + ALIGN8(sizeof(INT)), param_size);

            /* move remaining parameters to end of string */
            memcpy(out_param_ptr + param_size,
                   out_param_ptr + max_size + ALIGN8(sizeof(INT)),
                   (POINTER_T) last_param_ptr -
                   ((POINTER_T) out_param_ptr + max_size + ALIGN8(sizeof(INT))));
         }

         if (flags & RPC_VARARRAY) {
            /* store array length at current out_param_ptr */
            max_size = *((INT *) out_param_ptr);
            param_size = *((INT *) prpc_param[i + 1]);
            *((INT *) out_param_ptr) = param_size;

            out_param_ptr += ALIGN8(sizeof(INT));

            param_size = ALIGN8(param_size);

            /* move remaining parameters to end of array */
            memcpy(out_param_ptr + param_size,
                   out_param_ptr + max_size + ALIGN8(sizeof(INT)),
                   (POINTER_T) last_param_ptr -
                   ((POINTER_T) out_param_ptr + max_size + ALIGN8(sizeof(INT))));
         }

         if (tid == TID_STRUCT)
            param_size = ALIGN8(rpc_list[index].param[i].n);

         out_param_ptr += param_size;
      }

   /* send return parameters */
   param_size = (POINTER_T) out_param_ptr - (POINTER_T) nc_out->param;
   nc_out->header.routine_id = status;
   nc_out->header.param_size = param_size;

   status = msend_tcp(sock, return_buffer, sizeof(NET_COMMAND_HEADER) + param_size);

   if (status < 0) {
      printf("mrpc_execute: msend_tcp() failed\n");
      return RPC_NET_ERROR;
   }

   return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

int mrpc_connect(char *host_name, int port)
{
   INT status, sock;
   struct sockaddr_in bind_addr;
   struct hostent *phe;

#ifdef _MSC_VER
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return -1;
   }
#endif

   /* create a new socket for connecting to remote server */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock == -1) {
      perror("mrpc_connect,socket");
      return -1;
   }

   /* let OS choose any port number */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = 0;

   status = bind(sock, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
   if (status < 0) {
      perror("mrpc_connect,bind");
      return -1;
   }

   /* connect to remote node */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = htons((short) port);

   phe = gethostbyname(host_name);
   if (phe == NULL) {
      printf("Cannot find host name \"%s\"\n", host_name);
      return -1;
   }

   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);

   status = connect(sock, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
   if (status != 0) {
      if (errno)
         perror("mrpc_connect,connect");
      return -1;
   }

   return sock;
}

/*------------------------------------------------------------------*/

int mrpc_disconnect(int sock)
{
   closesocket(sock);
   return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

void mrpc_va_arg(va_list * arg_ptr, int arg_type, void *arg)
{
   switch (arg_type) {
      /* On the stack, the minimum parameter size is sizeof(int).
         To avoid problems on little endian systems, treat all
         smaller parameters as int's */
   case TID_BYTE:
   case TID_SBYTE:
   case TID_CHAR:
   case TID_WORD:
   case TID_SHORT:
      *((int *) arg) = va_arg(*arg_ptr, int);
      break;

   case TID_INT:
   case TID_BOOL:
      *((INT *) arg) = va_arg(*arg_ptr, INT);
      break;

   case TID_DWORD:
      *((unsigned int *) arg) = va_arg(*arg_ptr, unsigned int);
      break;

      /* float variables are passed as double by the compiler */
   case TID_FLOAT:
      *((float *) arg) = (float) va_arg(*arg_ptr, double);
      break;

   case TID_DOUBLE:
      *((double *) arg) = va_arg(*arg_ptr, double);
      break;

   case TID_ARRAY:
      *((char **) arg) = va_arg(*arg_ptr, char *);
      break;
   }
}

/*------------------------------------------------------------------*/

int mrpc_call(int sock, const int routine_id, ...)
{
   va_list ap, aptmp;
   char str[256], arg[8], arg_tmp[8];
   INT arg_type;
   INT i, index, status;
   INT param_size, arg_size, send_size;
   INT tid, flags;
   fd_set readfds;
   struct timeval timeout;
   char *param_ptr;
   int bpointer;
   NET_COMMAND *nc;
   time_t now;

   nc = (NET_COMMAND *) net_buffer;
   nc->header.routine_id = routine_id;

   for (i = 0;; i++)
      if (rpc_list[i].id == routine_id || rpc_list[i].id == 0)
         break;
   index = i;
   if (rpc_list[i].id == 0) {
      printf("mrpc_call: invalid rpc ID (%d)\n", routine_id);
      return RPC_INVALID_ID;
   }

   time(&now);
   strcpy(str, ctime(&now));
   str[19] = 0;
   debug_log("%s %s (", 0, str + 4, rpc_list[i].name);

   /* examine variable argument list and convert it to parameter array */
   va_start(ap, routine_id);

   for (i = 0, param_ptr = nc->param; rpc_list[index].param[i].tid != 0; i++) {
      tid = rpc_list[index].param[i].tid;
      flags = rpc_list[index].param[i].flags;

      bpointer = (flags & RPC_POINTER) || (flags & RPC_OUT) ||
          (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
          tid == TID_STRING || tid == TID_ARRAY || tid == TID_STRUCT || tid == TID_LINK;

      if (bpointer)
         arg_type = TID_ARRAY;
      else
         arg_type = tid;

      /* floats are passed as doubles, at least under NT */
      if (tid == TID_FLOAT && !bpointer)
         arg_type = TID_DOUBLE;

      /* get pointer to argument */
      mrpc_va_arg(&ap, arg_type, arg);

      if (flags & RPC_IN) {
         if (bpointer)
            arg_size = mtid_size[tid];
         else
            arg_size = mtid_size[arg_type];

         /* for strings, the argument size depends on the string length */
         if (tid == TID_STRING || tid == TID_LINK)
            arg_size = 1 + strlen((char *) *((char **) arg));

         /* for varibale length arrays, the size is given by
            the next parameter on the stack */
         if (flags & RPC_VARARRAY) {
            memcpy(&aptmp, &ap, sizeof(ap));
            mrpc_va_arg(&aptmp, TID_ARRAY, arg_tmp);

            if (flags & RPC_OUT)
               arg_size = *((INT *) * ((void **) arg_tmp));
            else
               arg_size = *((INT *) arg_tmp);

            *((INT *) param_ptr) = ALIGN8(arg_size);
            param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
            arg_size = rpc_list[index].param[i].n;

         /* always align parameter size */
         param_size = ALIGN8(arg_size);

         if ((POINTER_T) param_ptr - (POINTER_T) nc + param_size > NET_BUFFER_SIZE) {
            printf
                ("mrpc_call: parameters (%d) too large for network buffer (%d)\n",
                (int)((POINTER_T) param_ptr - (POINTER_T) nc + param_size), NET_BUFFER_SIZE);
            return RPC_EXCEED_BUFFER;
         }

         if (bpointer)
            memcpy(param_ptr, (void *) *((void **) arg), arg_size);
         else {
            /* floats are passed as doubles on most systems */
            if (tid != TID_FLOAT)
               memcpy(param_ptr, arg, arg_size);
            else
               *((float *) param_ptr) = (float) *((double *) arg);
         }

         switch (tid) {
         case TID_BYTE:
            debug_log(" %d", 0, *((unsigned char *) param_ptr));
            break;
         case TID_SBYTE:
            debug_log(" %d", 0, *((char *) param_ptr));
            break;
         case TID_CHAR:
            debug_log(" %c", 0, *((char *) param_ptr));
            break;
         case TID_WORD:
            debug_log(" %d", 0, *((unsigned short *) param_ptr));
            break;
         case TID_SHORT:
            debug_log(" %d", 0, *((short *) param_ptr));
            break;
         case TID_DWORD:
            debug_log(" %d", 0, *((unsigned int *) param_ptr));
            break;
         case TID_INT:
            debug_log(" %d", 0, *((int *) param_ptr));
            break;
         case TID_BOOL:
            debug_log(" %d", 0, *((int *) param_ptr));
            break;
         case TID_FLOAT:
            debug_log(" %f", 0, *((float *) param_ptr));
            break;
         case TID_DOUBLE:
            debug_log(" %lf", 0, *((double *) param_ptr));
            break;
         case TID_STRING:
            debug_log(" %s", 0, (char *) param_ptr);
            break;

         default:
            debug_log(" *", 0);
            break;
         }

         param_ptr += param_size;

      }
   }

   debug_log(" ) : (", 0);

   va_end(ap);

   nc->header.param_size = (POINTER_T) param_ptr - (POINTER_T) nc->param;

   send_size = nc->header.param_size + sizeof(NET_COMMAND_HEADER);

   /* send and wait for reply on send socket */
   i = msend_tcp(sock, (char *) nc, send_size);
   if (i != send_size) {
      printf("mrpc_call: msend_tcp() failed\n");
      return RPC_NET_ERROR;
   }

   /* make some timeout checking */
   FD_ZERO(&readfds);
   FD_SET(sock, &readfds);

   timeout.tv_sec = RPC_TIMEOUT / 1000;
   timeout.tv_usec = (RPC_TIMEOUT % 1000) * 1000;

   select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

   if (!FD_ISSET(sock, &readfds)) {
      printf("mrpc_call: rpc timeout, routine = \"%s\"\n", rpc_list[index].name);

      /* disconnect to avoid that the reply to this mrpc_call comes at
         the next mrpc_call */
      closesocket(sock);

      return RPC_ERR_TIMEOUT;
   }

   /* receive result on send socket */
   i = mrecv_tcp(sock, net_buffer, NET_BUFFER_SIZE);

   if (i <= 0) {
      printf("mrpc_call: mrecv_tcp() failed, routine = \"%s\"\n", rpc_list[index].name);
      return RPC_NET_ERROR;
   }

   /* extract result variables and place it to argument list */
   status = nc->header.routine_id;

   va_start(ap, routine_id);

   for (i = 0, param_ptr = nc->param; rpc_list[index].param[i].tid != 0; i++) {
      tid = rpc_list[index].param[i].tid;
      flags = rpc_list[index].param[i].flags;

      bpointer = (flags & RPC_POINTER) || (flags & RPC_OUT) ||
          (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
          tid == TID_STRING || tid == TID_ARRAY || tid == TID_STRUCT || tid == TID_LINK;

      if (bpointer)
         arg_type = TID_ARRAY;
      else
         arg_type = rpc_list[index].param[i].tid;

      if (tid == TID_FLOAT && !bpointer)
         arg_type = TID_DOUBLE;

      mrpc_va_arg(&ap, arg_type, arg);

      if (rpc_list[index].param[i].flags & RPC_OUT) {
         tid = rpc_list[index].param[i].tid;
         arg_size = mtid_size[tid];

         if (tid == TID_STRING || tid == TID_LINK)
            arg_size = strlen((char *) (param_ptr)) + 1;

         if (flags & RPC_VARARRAY) {
            arg_size = *((INT *) param_ptr);
            param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
            arg_size = rpc_list[index].param[i].n;

         /* return parameters are always pointers */
         if (*((char **) arg))
            memcpy((void *) *((char **) arg), param_ptr, arg_size);

         /* parameter size is always aligned */
         param_size = ALIGN8(arg_size);

         switch (tid) {
         case TID_BYTE:
            debug_log(" %d", 0, *((unsigned char *) param_ptr));
            break;
         case TID_SBYTE:
            debug_log(" %d", 0, *((char *) param_ptr));
            break;
         case TID_CHAR:
            debug_log(" %c", 0, *((char *) param_ptr));
            break;
         case TID_WORD:
            debug_log(" %d", 0, *((unsigned short *) param_ptr));
            break;
         case TID_SHORT:
            debug_log(" %d", 0, *((short *) param_ptr));
            break;
         case TID_DWORD:
            debug_log(" %d", 0, *((unsigned int *) param_ptr));
            break;
         case TID_INT:
            debug_log(" %d", 0, *((int *) param_ptr));
            break;
         case TID_BOOL:
            debug_log(" %d", 0, *((int *) param_ptr));
            break;
         case TID_FLOAT:
            debug_log(" %f", 0, *((float *) param_ptr));
            break;
         case TID_DOUBLE:
            debug_log(" %lf", 0, *((double *) param_ptr));
            break;
         case TID_STRING:
            debug_log(" %s", 0, (char *) param_ptr);
            break;

         default:
            debug_log(" *", 0);
            break;
         }

         param_ptr += param_size;
      }
   }

   debug_log(" ) : %d\n", 0, status);

   va_end(ap);

   return status;
}

/*------------------------------------------------------------------*/

extern void mscb_cleanup(int sock);
int _server_sock = 0;

void mrpc_server_loop(void)
{
   int i, status, sock, lsock, len, flag;
   struct sockaddr_in serv_addr, acc_addr;
   char str[256];
   struct hostent *phe;
   fd_set readfds;
   struct timeval timeout;

#ifdef _MSC_VER
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return;
   }
#endif

   /* create a new socket */
   lsock = socket(AF_INET, SOCK_STREAM, 0);

   if (lsock == -1) {
      perror("mrpc_server_loop");
      return;
   }

   /* bind local node name and port to socket */
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;

   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons((short) MSCB_RPC_PORT);

   /* try reusing address */
   flag = 1;
   setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(INT));

   status = bind(lsock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
   if (status < 0) {
      perror("mrpc_server_loop");
      return;
   }

   /* listen for connection */
   status = listen(lsock, SOMAXCONN);
   if (status < 0) {
      perror("mrpc_server_loop");
      return;
   }

   printf("Server listening...\n");
   do {
      FD_ZERO(&readfds);
      FD_SET(lsock, &readfds);

      for (i = 0; i < N_MAX_CONNECTION; i++)
         if (rpc_sock[i] > 0)
            FD_SET(rpc_sock[i], &readfds);

      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;

      status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

      if (FD_ISSET(lsock, &readfds)) {
         len = sizeof(acc_addr);
#ifdef _MSC_VER
         sock = accept(lsock, (struct sockaddr *) &acc_addr, (int *) &len);
#else
         sock = accept(lsock, (struct sockaddr *) &acc_addr, (unsigned int *) &len);
#endif

         /* find new entry in socket table */
         for (i = 0; i < N_MAX_CONNECTION; i++)
            if (rpc_sock[i] == 0)
               break;

         if (i == N_MAX_CONNECTION) {
            printf("Maximum number of connections exceeded\n");
            closesocket(sock);
         } else {
            rpc_sock[i] = sock;

            phe = gethostbyaddr((char *) &acc_addr.sin_addr, 4, AF_INET);
            if (phe != NULL)
               strcpy(str, phe->h_name);
            else
               strcpy(str, (char *) inet_ntoa(acc_addr.sin_addr));

            printf("Open new connection from %s\n", str);
         }
      } else {
         /* check if open connection received data */
         for (i = 0; i < N_MAX_CONNECTION; i++)
            if (rpc_sock[i] > 0 && FD_ISSET(rpc_sock[i], &readfds)) {
               len = mrecv_tcp(rpc_sock[i], net_buffer, NET_BUFFER_SIZE);
               if (len < 0) {
                  /* remove stale fd */
                  mscb_cleanup(rpc_sock[i]);

                  /* close broken connection */
                  printf("Close connection\n");
                  closesocket(rpc_sock[i]);
                  rpc_sock[i] = 0;
               } else {
                  _server_sock = rpc_sock[i];
                  mrpc_execute(rpc_sock[i], net_buffer);
               }
            }
      }

   } while (1);

   printf("Server aborted.\n");
}

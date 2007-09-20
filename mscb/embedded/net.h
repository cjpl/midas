/********************************************************************\

  Name:         net.h
  Created by:   Stefan Ritt

  Contents:     CMX Micronet data types and prototypes

  $Id$

\********************************************************************/

#define IP_ADDR_LEN          4     // number of bytes in IP address
#define ETH_ADDR_LEN         6     // number of bytes in ethernet hardware address
#define NUM_SOCKETS         16     // must match definition in netlib.lib

#define IP_VER               4
#define MIN_IHL              5
#define IP_VER_IHL        0x45
#define IP_VER_IHL_TOS  0x4500
#define IP_HEADER_LEN       20
#define ROUTINE_SERVICE      0 
#define LAST_FRAGMENT        0 
#define NO_FRAGMENT          0 
#define PROTO_ICMP           1 
#define PROTO_IGMP           2 
#define PROTO_IP             4 
#define PROTO_TCP            6 
#define PROTO_UDP           17

#define ICMP_ECHO            8
#define ICMP_ECHO_REPLY      0
#define ECHO_HEADER_LEN      8

/* packet types */
#define TCP_TYPE          0x01
#define UDP_TYPE          0x02
#define ICMP_TYPE         0x04
#define UNKNOWN_TYPE      0x08
#define FRAG_TYPE         0x10
#define ARP_TYPE          0x20
#define IGMP_TYPE         0x40

/* socket types */
#define STD_TYPE          0x00
#define HTTP_TYPE         0x01
#define FTP_TYPE          0x02
#define SMTP_TYPE         0x04
#define TFTP_TYPE         0x08
#define MULTICAST_TYPE    0x10
#define S_ARP_TYPE        0x20
#define AUTO_TYPE         0x40   /* for automatically created sockets */
#define INACTIVITY_TIMER  0x80

/* open types */
#define PASSIVE_OPEN         0
#define ACTIVE_OPEN          1
#define NO_OPEN              2

/* socket states */
#define ACTIVE_STATE      0x01
#define SENDING_STATE     0x02

/* DHCP */
#define DHCP_SERVER_ID_LEN   4
#define DHCP_DEAD            0
#define DHCP_OK              9
#define DHCP_RENEWING       10
#define DHCP_REBINDING      11

/* TCP states */
#define TCP_CLOSED           0   
#define TCP_LISTEN           1   
#define TCP_SYN_SENT         2   
#define TCP_SYN_RECEIVED     3   
#define TCP_ESTABLISHED      4   
#define TCP_FIN_WAIT_1       5   
#define TCP_FIN_WAIT_2       6   
#define TCP_CLOSE_WAIT       7   
#define TCP_CLOSING          8   
#define TCP_LAST_ACK         9   
#define TCP_TIME_WAIT       10  

/* TCP flag bits */
#define TCP_URG           0x20
#define TCP_ACK           0x10
#define TCP_PSH           0x08
#define TCP_RST           0x04
#define TCP_SYN           0x02
#define TCP_FIN           0x01
#define TCP_FIN_ACK       0x11
#define TCP_SYN_ACK       0x12

#define RESET_TCP_TIMER(p)    mn_reset_timer( &((p)->tcp_timer), (TCP_RESEND_TICKS) )
#define TCP_TIMER_EXPIRED(p)  (mn_timer_expired( &((p)->tcp_timer) ))

/*------------------------------------------------------------------*/

/* error numbers */
#define NOT_ENOUGH_SOCKETS       -128
#define SOCKET_ALREADY_EXISTS    -127
#define NOT_SUPPORTED            -126
#define PPP_OPEN_FAILED          -125
#define TCP_OPEN_FAILED          -124
#define BAD_SOCKET_DATA          -123
#define SOCKET_NOT_FOUND         -122
#define SOCKET_TIMED_OUT         -121
#define BAD_IP_HEADER            -120
#define NEED_TO_LISTEN           -119
#define RECV_TIMED_OUT           -118
#define ETHER_INIT_ERROR         -117
#define ETHER_SEND_ERROR         -116
#define ETHER_RECV_ERROR         -115
#define NEED_TO_SEND             -114
#define UNABLE_TO_SEND           -113
#define VFILE_ENTRY_IN_USE       -112
#define TFTP_FILE_NOT_FOUND      -111
#define TFTP_NO_FILE_SPECIFIED   -110
#define TFTP_FILE_TOO_BIG        -109
#define TFTP_FAILED              -108
#define SMTP_ALREADY_OPEN        -107
#define SMTP_OPEN_FAILED         -106
#define SMTP_NOT_OPEN            -105
#define SMTP_BAD_PARAM_ERR       -104
#define SMTP_ERROR               -103
#define NEED_TO_EXIT             -102
#define FTP_FILE_MAXOUT          -101
#define DHCP_ERROR               -100
#define DHCP_LEASE_EXPIRED        -99
#define PPP_LINK_DOWN             -98
#define GET_FUNC_ERROR            -97
#define FTP_SERVER_DOWN           -96
#define ARP_REQUEST_FAILED        -95
#define NEED_IGNORE_PACKET        -94
#define TASK_DID_NOT_START        -93
#define DHCP_LEASE_RENEWING       -92
#define IGMP_ERROR                -91

/* TCP function error codes */
#define TCP_ERROR                  -1
#define TCP_TOO_LONG               -2
#define TCP_BAD_HEADER             -3
#define TCP_BAD_CSUM               -4
#define TCP_BAD_FCS                -5
#define TCP_NO_CONNECT             -6
                                    
/* UDP error codes */               
#define UDP_ERROR                  -1
#define UDP_BAD_CSUM               -4
#define UDP_BAD_FCS                -5

/*------------------------------------------------------------------*/

typedef union seqnum_u {
   unsigned char NUMC[4];
   unsigned short NUMW[2];
   unsigned long NUML;
} SEQNUM_U;

typedef unsigned short TIMER_TICK_T;

typedef struct timer_info_s {
   TIMER_TICK_T timer_start;
   TIMER_TICK_T timer_end;
   unsigned char timer_wrap;
} TIMER_INFO_T;

typedef TIMER_INFO_T * PTIMER_INFO;

typedef struct socket_info_s {
   unsigned short src_port;
   unsigned short dest_port;
   unsigned char ip_dest_addr[IP_ADDR_LEN];
   unsigned char eth_dest_hw_addr[ETH_ADDR_LEN];
   unsigned char *send_ptr;
   unsigned short send_len;
   unsigned char *recv_ptr;
   unsigned char *recv_end;
   unsigned short recv_len;
   unsigned char ip_proto;
   unsigned char socket_no;
   unsigned char socket_type;
   unsigned char socket_state;
   unsigned char tcp_state;
   unsigned char tcp_resends;
   unsigned char tcp_flag;
   unsigned char recv_tcp_flag;
   unsigned char data_offset;
   unsigned short tcp_unacked_bytes;
   unsigned short recv_tcp_window;
   SEQNUM_U RCV_NXT;
   SEQNUM_U SEG_SEQ;
   SEQNUM_U SEG_ACK;
   SEQNUM_U SND_UNA;
   TIMER_INFO_T tcp_timer;
} SOCKET_INFO_T;

typedef SOCKET_INFO_T * PSOCKET_INFO;

typedef struct dhcp_lease_s
{
   unsigned long org_lease_time;                 /* last requested lease time */
   volatile unsigned long lease_time;            /* seconds left in current lease */
   unsigned long t1_renew_time;                  /* time to make renew request */
   unsigned long t2_renew_time;                  /* time to make rebind request */
   volatile unsigned char dhcp_state;            /* current dhcp state */
   unsigned char infinite_lease;                 /* infinite lease TRUE or FALSE */
   unsigned char server_id[DHCP_SERVER_ID_LEN];  /* DHCP server IP address */
} DHCP_LEASE_T;

typedef DHCP_LEASE_T * PDHCP_LEASE;

/*------------------------------------------------------------------*/

extern SOCKET_INFO_T sock_info[NUM_SOCKETS];
#define MK_SOCKET_PTR(s)      &sock_info[(s)]
#define SOCKET_ACTIVE(s)      (sock_info[(s)].socket_state & 0x01)

extern unsigned char null_addr[IP_ADDR_LEN];
extern unsigned char broadcast_addr[IP_ADDR_LEN];

extern DHCP_LEASE_T dhcp_lease;
#define DHCP_DEFAULT_LEASE_TIME  (72*3600)

/*------------------------------------------------------------------*/

int mn_init(void);
signed char mn_open(unsigned char [], unsigned short, unsigned short, unsigned char, unsigned char, unsigned char, unsigned char *, unsigned short);
signed char mn_open_socket(unsigned char [], unsigned short, unsigned short, unsigned char, unsigned char, unsigned char *, unsigned short);
int mn_send(signed char, unsigned char *, unsigned short);
int mn_recv(signed char, unsigned char *, unsigned short);
int mn_recv_wait(signed char, unsigned char *, unsigned short, unsigned short);
int mn_close(signed char);
int mn_abort(signed char);
unsigned char mn_get_socket_type(unsigned short src_port);
signed char mn_close_packet(PSOCKET_INFO, unsigned short);
PSOCKET_INFO mn_find_socket(unsigned short,unsigned short,unsigned char *,unsigned char);

void mn_dhcp_init(void);
int mn_dhcp_start(unsigned short *, unsigned long);
int mn_dhcp_release(void);
int mn_dhcp_renew(unsigned long);

void mn_ip_init(void);
unsigned char mn_ip_recv(void);
unsigned char mn_ip_get_pkt(void);
signed char mn_ip_send_header(PSOCKET_INFO,unsigned char,unsigned short);
void mn_ip_discard_packet(void);

void mn_tcp_init(PSOCKET_INFO);
void mn_tcp_abort(PSOCKET_INFO);
void mn_tcp_shutdown(PSOCKET_INFO);
signed char mn_tcp_open(byte,PSOCKET_INFO);
int mn_tcp_send(PSOCKET_INFO);
int mn_tcp_recv(PSOCKET_INFO *);
void mn_tcp_close(PSOCKET_INFO);

void mn_reset_timer(PTIMER_INFO, TIMER_TICK_T);
unsigned char mn_timer_expired(PTIMER_INFO);
TIMER_TICK_T mn_get_timer_tick(void);
void mn_wait_ticks(TIMER_TICK_T num_ticks);

int mn_udp_send(PSOCKET_INFO);
int mn_udp_recv(PSOCKET_INFO *);

/*------------------------------------------------------------------*/


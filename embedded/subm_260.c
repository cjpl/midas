/********************************************************************\

  Name:         subm_260.c
  Created by:   Stefan Ritt

  Contents:     MSCB program for Cygnal Ethernet sub-master
                SUBM260 running on Cygnal C8051F120

  $Log$
  Revision 1.1  2005/03/16 14:16:49  ritt
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "mscb.h"
#include "net.h"

/*------------------------------------------------------------------*/

#define MSCB_NET_PORT           1177

char *host_name = "MSCB001";    // used for DHCP

/* set MAC address for first PSI address by default */
unsigned char eth_src_hw_addr[ETH_ADDR_LEN] = { 0x00,0x50,0xC2,0x46,0xD0,0x00 };

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
#define MCMD_SET_ADDR    0x34
#define MCMD_SET_BAUD    0x39

#define MCMD_FREEZE      0x41
#define MCMD_SYNC        0x49
#define MCMD_UPGRADE     0x50
#define MCMD_USER        0x58

#define MCMD_ECHO        0x61
#define MCMD_TOKEN       0x68
#define MCMD_SET_FLAGS   0x69

#define MCMD_ACK         0x78

#define MCMD_WRITE_NA    0x80
#define MCMD_WRITE_ACK   0x88

#define MCMD_FLASH       0x98

#define MCMD_READ        0xA0

#define MCMD_WRITE_BLOCK 0xB5
#define MCMD_READ_BLOCK  0xB9

#define GET_INFO_GENERAL   0
#define GET_INFO_VARIABLE  1

#define RS485_FLAG_BIT9      (1<<0)
#define RS485_FLAG_NO_ACK    (1<<1)
#define RS485_FLAG_SHORT_TO  (1<<2)
#define RS485_FLAG_LONG_TO   (1<<3)
#define RS485_FLAG_CMD       (1<<4)

/*------------------------------------------------------------------*/

void hardware_init(void);
int tcp_send(char socket_no, unsigned char *buffer, int size);

/*------------------------------------------------------------------*/

#define DATA_BUFF_LEN    64
unsigned char tcp_tx_buf[NUM_SOCKETS][DATA_BUFF_LEN];
unsigned char tcp_rx_buf[NUM_SOCKETS][DATA_BUFF_LEN];

unsigned char rs485_tx_buf[DATA_BUFF_LEN];
unsigned char rs485_rx_buf[DATA_BUFF_LEN];

unsigned char n_tcp_rx, n_rs485_tx, i_rs485_tx, i_rs485_rx;

/*------------------------------------------------------------------*/

void setup(void)
{
   int i;                       // software timer

   WDTCN = 0xDE;                // Disable watchdog timer
   WDTCN = 0xAD;

   SFRPAGE = CONFIG_PAGE;       // set SFR page

   XBR0 = 0x04;                 // Enable UART0 & UART1
   XBR1 = 0x00;
   XBR2 = 0x44;

   // all pins used by the external memory interface are in push-pull mode
   P0MDOUT = 0xFF;
   P1MDOUT = 0xFF;
   P2MDOUT = 0xFF;
   P3MDOUT = 0xFF;
   P0 = 0xC0;                   // /WR, /RD, are high, RESET is low
   P1 = 0x00;
   P2 = 0x00;                   // P1, P2 contain the address lines
   P3 = 0xFF;                   // P3 contains the data lines

   OSCICN = 0x83;               // set internal oscillator to run
                                // at its maximum frequency

   CLKSEL = 0x00;               // Select the internal osc. as
                                // the SYSCLK source

   //Turn on the PLL and increase the system clock by a factor of M/N = 2
   SFRPAGE = CONFIG_PAGE;

   PLL0CN = 0x00;               // Set internal osc. as PLL source
   SFRPAGE = LEGACY_PAGE;
   FLSCL = 0x10;                // Set FLASH read time for 50MHz clk
   // or less
   SFRPAGE = CONFIG_PAGE;
   PLL0CN |= 0x01;              // Enable Power to PLL
   PLL0DIV = 0x01;              // Set Pre-divide value to N (N = 1)
   PLL0FLT = 0x01;              // Set the PLL filter register for
   // a reference clock from 19 - 30 MHz
   // and an output clock from 45 - 80 MHz
   PLL0MUL = 0x02;              // Multiply SYSCLK by M (M = 2)

   for (i = 0; i < 256; i++);   // Wait at least 5us
   PLL0CN |= 0x02;              // Enable the PLL
   while (!(PLL0CN & 0x10));    // Wait until PLL frequency is locked
   CLKSEL = 0x02;               // Select PLL as SYSCLK source

   SFRPAGE = LEGACY_PAGE;

   EMI0CF = 0xD7;               // Split-Mode, non-multiplexed
                                // on P0-P3 (use 0xF7 for P4 - P7)

   EMI0TC = 0xB7;               // This value may be modified
                                // according to SYSCLK to meet the
                                // timing requirements for the CS8900A
                                // For example, EMI0TC should be >= 0xB7
                                // for a 100 MHz SYSCLK.

   /* init memory */
   RS485_ENABLE = 0;

   /* initialize UART0 */
   uart_init(0, BD_115200);

   SFRPAGE = TMR2_PAGE;
   RCAP2L = 0x100 - 27; // 113425 Baud at 49 MHz
   SFRPAGE = LEGACY_PAGE;

   /* Blink LEDs */
   led_blink(0, 5, 150);
   led_blink(1, 5, 150);

   /* invert first LED */
   led_mode(0, 1);
}

/*------------------------------------------------------------------*/

void serial_int(void) interrupt 4 using 1
{
   if (TI0) {
      TI0 = 0;

      /* transmit next unsigned char */

      i_rs485_tx++;
      if (i_rs485_tx < n_rs485_tx) {
         DELAY_US(INTERCHAR_DELAY);
         SBUF0 = rs485_tx_buf[i_rs485_tx];
      } else {
         /* end of buffer */
         RS485_ENABLE = 0;
         i_rs485_tx = n_rs485_tx = 0;
      }
   }

   if (RI0) {
      /* put received unsigned char into buffer */
      rs485_rx_buf[i_rs485_rx++] = SBUF0;

      RI0 = 0;
   }
}

/*------------------------------------------------------------------*/

void yield()
{
}

/*------------------------------------------------------------------*/

sbit led_0 = LED_0;
sbit led_1 = LED_1;

unsigned char execute(char socket_no)
{
   if (rs485_tx_buf[1] == MCMD_INIT) {
      /* reboot */
      RSTSRC = 0x10;
   }

   if (rs485_tx_buf[1] == MCMD_ECHO) {
      /* return echo */
      led_blink(1, 1, 50);
      rs485_rx_buf[0] = MCMD_ACK;
      rs485_rx_buf[1] = 0;  // reserved for future use
      tcp_send(socket_no, rs485_rx_buf, 2);
      return 2;
   }

   if (rs485_tx_buf[1] == RS485_FLAG_CMD) {
      /* just blink LED, discard data */
      led_1 = !led_1;
   }

   return 0;
}

/*------------------------------------------------------------------*/

unsigned char rs485_send(char socket_no, unsigned char len, unsigned char flags)
{
   /* clear receive buffer */
   i_rs485_rx = 0;

   if (flags & RS485_FLAG_CMD) {

      /* execute direct command */
      execute(socket_no);
      n_rs485_tx = 0;
      return 0;

   } else {

      /* send buffer to RS485 */
      SFRPAGE = UART0_PAGE;

      /* set bit9 */
      TB80 = (flags & RS485_FLAG_BIT9) > 0;

      n_rs485_tx = len;
      DELAY_US(INTERCHAR_DELAY);
      RS485_ENABLE = 1;
      SBUF0 = rs485_tx_buf[1];
      i_rs485_tx = 1;
   }

   return 1;
}

/*------------------------------------------------------------------*/

void rs485_receive(char socket_no, unsigned char rs485_flags)
{
unsigned char  n;
unsigned short i, to;

   if (rs485_flags & RS485_FLAG_NO_ACK)
      return;

   if (rs485_flags & RS485_FLAG_SHORT_TO)
      to = 4;     // 400 us for PING
   else if (rs485_flags & RS485_FLAG_LONG_TO)
      to = 10000; // 1 s for flash/upgrade
   else
      to = 100;   // 10 ms for other commands

   n = 0;

   for (i = 0; i<to ; i++) {

      /* check for PING acknowledge (single unsigned char) */
      if (rs485_tx_buf[1] == MCMD_PING16 &&
          i_rs485_rx == 1 && rs485_rx_buf[0] == MCMD_ACK) {

         led_blink(1, 1, 50);
         tcp_send(socket_no, rs485_rx_buf, 1);
         break;
      }

      /* check for normal acknowledge */
      if (i_rs485_rx > 0 && (rs485_rx_buf[0] & MCMD_ACK) == MCMD_ACK) {

         n = rs485_rx_buf[0] & 0x07;     // length is three LSB
         if (n == 7 && i_rs485_rx > 1)
            n = rs485_rx_buf[1]+3;       // variable length acknowledge
         else
            n += 2;                      // add ACK and CRC

         if (i_rs485_rx == n) {
            led_blink(1, 1, 50);
            tcp_send(socket_no, rs485_rx_buf, n);
            break;
         }
      }

      yield();
      delay_us(100);
   }

   /* send timeout */
   if (i == to) {
      rs485_rx_buf[0] = 0xFF;
      tcp_send(socket_no, rs485_rx_buf, 1);
   }
}

/*------------------------------------------------------------------*/

int tcp_server(void)
{
	unsigned char packet_type;
	PSOCKET_INFO socket_ptr;
	int sent;
	int recvd;
   int status;
   char socket_no;
	static PSOCKET_INFO check_socket_ptr = sock_info;
   unsigned char dhcp_state;

	socket_ptr = &sock_info[0];
	sent = 0;
	recvd = 0;
   status = 0;

   /* if no listening socket open, open or reopen it */
   if (mn_find_socket(MSCB_NET_PORT, 0, null_addr, PROTO_TCP) == NULL) {
      socket_no = mn_open(null_addr, MSCB_NET_PORT, 0, NO_OPEN, PROTO_TCP,
                          STD_TYPE, tcp_rx_buf[0], DATA_BUFF_LEN);
      if (socket_no >= 0) {
         socket_ptr = &sock_info[socket_no];
         socket_ptr->recv_ptr = tcp_rx_buf[socket_no];
         socket_ptr->recv_end = tcp_rx_buf[socket_no] + DATA_BUFF_LEN - 1;
      }
   }

   /* loop until we get a tcp packet, will open a socket automatically
		if required.
	*/
	while (1) {
      dhcp_state = dhcp_lease.dhcp_state;
      if (dhcp_state == DHCP_DEAD)
         return (DHCP_LEASE_EXPIRED);

      /* renew with the previous lease time */
      if (dhcp_state == DHCP_RENEWING || dhcp_state == DHCP_REBINDING)
         mn_dhcp_renew(dhcp_lease.org_lease_time);

	 	packet_type = mn_ip_recv();

	 	if (packet_type & TCP_TYPE)
	 		break;

	 	if (packet_type & UDP_TYPE)
	 		break;

      /* Check if the socket pointed to by check_socket_ptr has bytes
         we need to resend. If it does, try a resend. Increment
         check_socket_ptr so another socket will be checked the next time
         around.
      */
      check_socket_ptr++;
      if (check_socket_ptr >= &sock_info[NUM_SOCKETS])
         check_socket_ptr = &sock_info[0];
      if (check_socket_ptr->ip_proto == PROTO_TCP && \
            TCP_TIMER_EXPIRED(check_socket_ptr) && \
            (check_socket_ptr->send_len || \
            check_socket_ptr->tcp_state >= TCP_FIN_WAIT_1))
         {
         socket_ptr = check_socket_ptr;
         goto send_data;
         }

 	}

	/* process the packet, switch sockets if neccessary */
   if (packet_type & TCP_TYPE)
	   recvd = mn_tcp_recv(&socket_ptr);

   if (packet_type & UDP_TYPE)
	   recvd = mn_udp_recv(&socket_ptr);

	/* if socket_ptr == NULL there was some sort of error. The TCP
		or UDP layer has thrown away the bad packet so we just wait for
      another packet.
	*/
	if (socket_ptr == NULL)
		return 0;

   if (packet_type & TCP_TYPE && \
         (recvd >= 0 || recvd == NEED_TO_LISTEN || recvd == TCP_NO_CONNECT))
      {
         {
         if (socket_ptr->tcp_state == TCP_CLOSED)  /* other side closed */
            {
            (void)mn_abort(socket_ptr->socket_no);
            socket_ptr = (PSOCKET_INFO)NULL;
            }
         }
      }


 	/* get another packet if error or NEED_TO_LISTEN */
	if (socket_ptr == NULL || recvd < 0)
		return 0;

   /* callback function to possibly do something with the packet we
      just received.
   */
   socket_ptr->send_ptr = socket_ptr->recv_ptr;
   socket_ptr->send_len = socket_ptr->recv_len;


send_data:
	if (socket_ptr != NULL)
      {
      if (socket_ptr->ip_proto == PROTO_TCP)
         {
	   	/* reply to the packet we just got */
         sent = mn_tcp_send(socket_ptr);

         if (sent != UNABLE_TO_SEND)
            {
               {
       	   	if ((sent < 0 && sent != TCP_NO_CONNECT) || \
                     socket_ptr->tcp_state == TCP_CLOSED)
         	   	mn_abort(socket_ptr->socket_no);
               }
            }
         }

      if (socket_ptr != NULL && socket_ptr->ip_proto == PROTO_UDP)
         {
   		/* reply to the packet we just got */
      	sent = mn_udp_send(socket_ptr);
         }

      }

   return (0);
}

/*------------------------------------------------------------------*/

int tcp_server1(void)
{
	unsigned char packet_type;
	PSOCKET_INFO socket_ptr;
	int sent;
	int recvd;
   int status;
   char socket_no;
	static PSOCKET_INFO check_socket_ptr = sock_info;
   unsigned char dhcp_state;

	socket_ptr = &sock_info[0];
	sent = 0;
	recvd = 0;
   status = 0;

   /* if no listening socket open, open or reopen it */
   if (mn_find_socket(MSCB_NET_PORT, 0, null_addr, PROTO_TCP) == NULL) {
      socket_no = mn_open(null_addr, MSCB_NET_PORT, 0, NO_OPEN, PROTO_TCP,
                          STD_TYPE, tcp_rx_buf[0], DATA_BUFF_LEN);
      if (socket_no >= 0) {
         socket_ptr = &sock_info[socket_no];
         socket_ptr->recv_ptr = tcp_rx_buf[socket_no];
         socket_ptr->recv_end = tcp_rx_buf[socket_no] + DATA_BUFF_LEN - 1;
      }
   }

   /* loop until we get a tcp packet, will open a socket automatically
		if required.
	*/
	while (1) {
      dhcp_state = dhcp_lease.dhcp_state;
      if (dhcp_state == DHCP_DEAD)
         return (DHCP_LEASE_EXPIRED);

      /* renew with the previous lease time */
      if (dhcp_state == DHCP_RENEWING || dhcp_state == DHCP_REBINDING)
         mn_dhcp_renew(dhcp_lease.org_lease_time);

	 	packet_type = mn_ip_recv();

	 	if (packet_type & TCP_TYPE) {
         recvd = mn_tcp_recv(&socket_ptr);
      	if (socket_ptr == NULL || recvd < 0)
      		return 0;

         if (recvd >= 0 || recvd == NEED_TO_LISTEN || recvd == TCP_NO_CONNECT)
            if (socket_ptr->tcp_state == TCP_CLOSED) { /* other side closed */
               (void)mn_abort(socket_ptr->socket_no);
               socket_ptr = (PSOCKET_INFO)NULL;
            }
         break;
      }

	 	if (packet_type & UDP_TYPE) {
    	   recvd = mn_udp_recv(&socket_ptr);
      	if (socket_ptr == NULL || recvd < 0)
      		return 0;

	 		break;
      }

      /* Check if the socket pointed to by check_socket_ptr has bytes
         we need to resend. If it does, try a resend. Increment
         check_socket_ptr so another socket will be checked the next time
         around.
      */
      check_socket_ptr++;
      if (check_socket_ptr >= &sock_info[NUM_SOCKETS])
         check_socket_ptr = &sock_info[0];
      if (check_socket_ptr->ip_proto == PROTO_TCP &&
            TCP_TIMER_EXPIRED(check_socket_ptr) &&
            (check_socket_ptr->send_len ||
            check_socket_ptr->tcp_state >= TCP_FIN_WAIT_1)) {

         socket_ptr = check_socket_ptr;
         sent = mn_tcp_send(socket_ptr);

         if (sent != UNABLE_TO_SEND)
    	   	if ((sent < 0 && sent != TCP_NO_CONNECT) ||
                  socket_ptr->tcp_state == TCP_CLOSED)
      	   	mn_abort(socket_ptr->socket_no);

         return 0;
      }

 	}

   /* callback function to possibly do something with the packet we
      just received.
   */
   socket_ptr->send_ptr = socket_ptr->recv_ptr;
   socket_ptr->send_len = socket_ptr->recv_len;


	if (socket_ptr != NULL)
      {
      if (socket_ptr->ip_proto == PROTO_TCP)
         {
	   	/* reply to the packet we just got */
         sent = mn_tcp_send(socket_ptr);

         if (sent != UNABLE_TO_SEND)
            {
               {
       	   	if ((sent < 0 && sent != TCP_NO_CONNECT) || \
                     socket_ptr->tcp_state == TCP_CLOSED)
         	   	mn_abort(socket_ptr->socket_no);
               }
            }
         }

      if (socket_ptr != NULL && socket_ptr->ip_proto == PROTO_UDP)
         {
   		/* reply to the packet we just got */
      	sent = mn_udp_send(socket_ptr);
         }

      }

   return (0);
}

/*------------------------------------------------------------------*/

int tcp_receive(unsigned char **data_ptr, char *socket_no_ptr)
{
   static PSOCKET_INFO check_socket_ptr = sock_info;

   unsigned char packet_type;
   PSOCKET_INFO socket_ptr;
   int recvd, sent;
   unsigned char dhcp_state;
   signed char socket_no;

   /* if no listening socket open, open or reopen it */
   if (mn_find_socket(MSCB_NET_PORT, 0, null_addr, PROTO_TCP) == NULL) {
      socket_no = mn_open(null_addr, MSCB_NET_PORT, 0, NO_OPEN, PROTO_TCP,
                          STD_TYPE, tcp_rx_buf[0], DATA_BUFF_LEN);
      if (socket_no >= 0) {
         socket_ptr = &sock_info[socket_no];
         socket_ptr->recv_ptr = tcp_rx_buf[socket_no];
         socket_ptr->recv_end = tcp_rx_buf[socket_no] + DATA_BUFF_LEN - 1;
      }
   }

   /* check DHCP status, renew if expired */
   dhcp_state = dhcp_lease.dhcp_state;
   if (dhcp_state == DHCP_DEAD)
      return (DHCP_LEASE_EXPIRED);

   /* renew with the previous lease time */
   if (dhcp_state == DHCP_RENEWING || dhcp_state == DHCP_REBINDING)
      (void) mn_dhcp_renew(dhcp_lease.org_lease_time);

   /* receive packet */
   packet_type = mn_ip_recv();

   socket_ptr = NULL;

   if (packet_type & TCP_TYPE) {

      recvd = mn_tcp_recv(&socket_ptr);
      if (socket_ptr == NULL)
         return 0;

      if (recvd >= 0 || recvd == NEED_TO_LISTEN || recvd == TCP_NO_CONNECT) {

         if (socket_ptr->tcp_state == TCP_CLOSED) {     /* other side closed */

            mn_abort(socket_ptr->socket_no);
            return 0;
         }
      }

   }

   if (packet_type & UDP_TYPE) {

      recvd = mn_udp_recv(&socket_ptr);
      if (socket_ptr == NULL)
         return 0;

   }

   /* Check if the socket pointed to by check_socket_ptr has bytes
      we need to resend. If it does, try a resend. Increment
      check_socket_ptr so another socket will be checked the next time
      around.
    */
   check_socket_ptr++;
   if (check_socket_ptr >= &sock_info[NUM_SOCKETS])
      check_socket_ptr = &sock_info[0];

   if (check_socket_ptr->ip_proto == PROTO_TCP &&
       TCP_TIMER_EXPIRED(check_socket_ptr) &&
       (check_socket_ptr->send_len || check_socket_ptr->tcp_state >= TCP_FIN_WAIT_1)) {
      socket_ptr = check_socket_ptr;

      sent = mn_tcp_send(socket_ptr);

      if (sent != UNABLE_TO_SEND) {
         if ((sent < 0 && sent != TCP_NO_CONNECT) || socket_ptr->tcp_state == TCP_CLOSED)
            mn_abort(socket_ptr->socket_no);
      }
   }

   /* get another packet if error or NEED_TO_LISTEN */
   if (socket_ptr == NULL || recvd < 0)
      return 0;

   *data_ptr = socket_ptr->recv_ptr;
   *socket_no_ptr = socket_ptr->socket_no;

   return socket_ptr->recv_len;
}

/*------------------------------------------------------------------*/

int tcp_send(unsigned char socket_no, unsigned char *buffer, int size)
{
   PSOCKET_INFO socket_ptr;
   int sent;

   if (socket_no < 0 || socket_no >= NUM_SOCKETS)
      return 0;

   socket_ptr = &sock_info[socket_no];

   /* copy data to tx buffer */
   memcpy(&tcp_tx_buf[socket_no]+1, buffer, size);
   tcp_tx_buf[socket_no][0] = size;
   socket_ptr->send_ptr = &tcp_tx_buf[socket_no];
   socket_ptr->send_len = size+1;

   if (socket_ptr->ip_proto == PROTO_TCP) {

      sent = mn_tcp_send(socket_ptr);

      if (sent != UNABLE_TO_SEND) {
         if ((sent < 0 && sent != TCP_NO_CONNECT) || socket_ptr->tcp_state == TCP_CLOSED) {
            mn_abort(socket_ptr->socket_no);
            return 0;
         }
      }

      return sent;
   } else if (socket_ptr->ip_proto == PROTO_UDP)
      sent = mn_udp_send(socket_ptr);

   return sent;
}

/*------------------------------------------------------------------*/

void main(void)
{
   unsigned char *ptr, n, flags;
   char socket_no;

   // initialize the C8051F12x
   setup();

   // initialize the CMX Micronet variables
   mn_init();

   // obtain IP address
   mn_dhcp_start(NULL, DHCP_DEFAULT_LEASE_TIME);

   do {
      yield();

      /* receive a TCP package, open socket if necessary */
      if ((n = tcp_receive(&ptr, &socket_no)) > 0) {

         /* signal incoming data */
         led_blink(0, 1, 50);

         /* check if buffer complete */
         if (ptr[0]+1 != n)
            continue;

         /* copy buffer after first unsigned char (buffer length) */
         memcpy(rs485_tx_buf, ptr+1, n-1);
         flags = rs485_tx_buf[0];

         if (rs485_send(socket_no, n-1, flags)) {

            /* wait until sent */
            while (n_rs485_tx)
               yield();

            /* wait for data to be received */
            rs485_receive(socket_no, flags);
         }
      }

   } while (1);
}



/********************************************************************\

  Name:         scs_260_cobra.c
  Created by:   Stefan Ritt

  Contents:     Broadcast program for COBRA magnet

  $Log$
  Revision 1.3  2005/03/17 14:20:09  ritt
  Changed host_name

  Revision 1.2  2005/03/17 11:19:24  ritt
  Changed indentation

  Revision 1.1  2005/03/16 14:21:20  ritt
  Initial revision

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "mscb.h"
#include "net.h"

/*------------------------------------------------------------------*/

#define TELNET_PORT       23

char host_name[] = "MSCB001";    // used for DHCP

/* set MAC address for first PSI address by default */
unsigned char eth_src_hw_addr[ETH_ADDR_LEN] = { 0x00, 0x50, 0xC2, 0x46, 0xD0, 0x00 };

/*------------------------------------------------------------------*/

void hardware_init(void);
int tcp_send(char socket_no, unsigned char *buffer, int size);

/*------------------------------------------------------------------*/

#define DATA_BUFF_LEN    64
unsigned char tcp_tx_buf[NUM_SOCKETS][DATA_BUFF_LEN];
unsigned char tcp_rx_buf[NUM_SOCKETS][DATA_BUFF_LEN];
unsigned char udp_buf[DATA_BUFF_LEN];

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

   /* start system clock */
   sysclock_init();
   CKCON = 0x00;                // assumed by tcp_timer()

   /* Blink LEDs */
   led_blink(0, 5, 150);
   led_blink(1, 5, 150);

   /* invert first LED */
   led_mode(0, 1);
}

/*------------------------------------------------------------------*/

void yield()
{
}

/*------------------------------------------------------------------*/

void tcp_server(void)
{
   unsigned char packet_type, i, n;
   PSOCKET_INFO socket_ptr;
   int sent, recvd;
   char socket_no;
   unsigned char dhcp_state;

   socket_ptr = &sock_info[0];
   recvd = 0;

   /* if no listening socket open, open or reopen it */
   if (mn_find_socket(TELNET_PORT, 0, null_addr, PROTO_TCP) == NULL) {

      /* count number of active sockets */
      for (i = n = 0; i < NUM_SOCKETS; i++)
         if (SOCKET_ACTIVE(i))
            n++;

      /* leave one inactive socket for ping and arp requests */
      if (n < NUM_SOCKETS - 1) {
         socket_no = mn_open(null_addr, TELNET_PORT, 0, NO_OPEN, PROTO_TCP,
                             STD_TYPE, tcp_rx_buf[0], DATA_BUFF_LEN);
         if (socket_no >= 0) {
            socket_ptr = &sock_info[socket_no];
            socket_ptr->recv_ptr = tcp_rx_buf[socket_no];
            socket_ptr->recv_end = tcp_rx_buf[socket_no] + DATA_BUFF_LEN - 1;
         }
      }
   }

   /* check DHCP status, renew if expired */
   dhcp_state = dhcp_lease.dhcp_state;
   if (dhcp_state == DHCP_DEAD)
      return;

   /* renew with the previous lease time */
   if (dhcp_state == DHCP_RENEWING || dhcp_state == DHCP_REBINDING)
      mn_dhcp_renew(dhcp_lease.org_lease_time);

   packet_type = mn_ip_recv();

   if (packet_type & TCP_TYPE) {
      recvd = mn_tcp_recv(&socket_ptr);
      if (socket_ptr == NULL)
         return;

      if (socket_ptr->tcp_state == TCP_CLOSED) {        /* other side closed */
         mn_abort(socket_ptr->socket_no);
         return;
      }
   }

   if (packet_type & UDP_TYPE) {
      recvd = mn_udp_recv(&socket_ptr);
      if (socket_ptr == NULL)
         return;
   }

   for (i = 0; i < NUM_SOCKETS; i++) {
      socket_ptr = MK_SOCKET_PTR(i);
      if (socket_ptr->ip_proto == PROTO_TCP && TCP_TIMER_EXPIRED(socket_ptr) &&
          (socket_ptr->send_len || socket_ptr->tcp_state >= TCP_FIN_WAIT_1)) {
         sent = mn_tcp_send(socket_ptr);

         if (sent == TCP_NO_CONNECT || socket_ptr->tcp_state == TCP_CLOSED)
            mn_abort(socket_ptr->socket_no);

         return;
      }
   }

   return;
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
   memcpy(&tcp_tx_buf[socket_no], buffer, size);
   socket_ptr->send_ptr = &tcp_tx_buf[socket_no];
   socket_ptr->send_len = size;

   sent = mn_tcp_send(socket_ptr);

   if (sent == TCP_NO_CONNECT || socket_ptr->tcp_state == TCP_CLOSED) {
      mn_abort(socket_ptr->socket_no);
      return 0;
   }

   return sent;
}

/*------------------------------------------------------------------*/

int udp_send(unsigned char socket_no, unsigned char *buffer, int size)
{
   PSOCKET_INFO socket_ptr;

   if (socket_no < 0 || socket_no >= NUM_SOCKETS)
      return 0;

   socket_ptr = &sock_info[socket_no];

   socket_ptr->send_ptr = buffer;
   socket_ptr->send_len = size;

   mn_udp_send(socket_ptr);

   return size;
}

/*------------------------------------------------------------------*/

void main(void)
{
   unsigned long last_time;
   unsigned char i, j;
   char str[80], bc_sock;

   // initialize the C8051F12x
   setup();

   // initialize the CMX Micronet variables
   mn_init();

   // obtain IP address
   mn_dhcp_start(NULL, DHCP_DEFAULT_LEASE_TIME);

   // open UDP socket for broadcasts
   bc_sock = mn_open(broadcast_addr, 1178, 1178, NO_OPEN, PROTO_UDP, STD_TYPE, udp_buf, DATA_BUFF_LEN);

   do {
      tcp_server();

      if (time() - last_time >= 100) {

         last_time = time();

         /* send time and socket states */
         sprintf(str, "Time: %1.3f:", (float) time() / 100.0);
         for (j = 0; j < NUM_SOCKETS; j++)
            sprintf(str + strlen(str), " %bd", sock_info[j].tcp_state);
         strcat(str, "\r\n");

         /* broadcast data */
         udp_send(bc_sock, str, strlen(str));

         /* send data to all connected sockets */
         for (i = 0; i < NUM_SOCKETS; i++) {
            if (sock_info[i].tcp_state == TCP_ESTABLISHED) {
               tcp_send(i, str, strlen(str));
            }
         }
      }


   } while (1);
}

/********************************************************************\

  Name:         cobra_current.c
  Created by:   Stefan Ritt

  Contents:     Receive broadcasts from COBRA magnet in PiE5 area
                and display coil currents.

  $Id$

\********************************************************************/

#include <stdio.h>
#include <time.h>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#endif

#define DEF_MCAST_GROUP  "239.208.0.1"

main()
{
   int sock, n;
   char cur_time[80], str[80], mcast_group[80];
   struct sockaddr_in myaddr, addr;
   struct ip_mreq req;
   int size;
   time_t now;

#if defined( _MSC_VER )
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return -1;
   }
#endif

   sock = socket(AF_INET, SOCK_DGRAM, 0);

   n = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(int)) < 0)
      perror("setsockopt SO_REUSEADDR");

   /* bind to port 1178 to receive cobra current broadcasts */
   memset(&myaddr, 0, sizeof(myaddr));
   myaddr.sin_family = AF_INET;
   myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   myaddr.sin_port = htons(1178);
   if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)))
      perror("bind");

   /* set multicast group */
   printf("Multicast group: %s\b", DEF_MCAST_GROUP);
   fgets(str, sizeof(str), stdin);
   if (strchr(str, '\n'))
      *strchr(str, '\n') = 0;
   strcpy(mcast_group, DEF_MCAST_GROUP);
   if (atoi(str) != 0)
      strcpy(mcast_group+10, str);

   memset(&req, 0, sizeof(req));
   req.imr_multiaddr.s_addr = inet_addr(mcast_group);
   req.imr_interface.s_addr = INADDR_ANY;
   
   if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&req, sizeof(req)) < 0)
      perror("setsockopt IP_ADD_MEMBERSHIP");

   printf("Waiting for data from multicast group %s ...\n", mcast_group);
   size = sizeof(addr);
   do {
      memset(str, 0, sizeof(str));
      n = recvfrom(sock, str, sizeof(str), 0, (struct sockaddr *)&addr, &size);

      /* get current time */
      time(&now);
      strcpy(cur_time, ctime(&now)+11);
      cur_time[8] = 0;

      /* print string */
      printf("%s - %s", cur_time, str);

   } while (1);
}

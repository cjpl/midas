/********************************************************************\

  Name:         cobra_current.c
  Created by:   Stefan Ritt

  Contents:     Receive broadcasts from COBRA magnet in PiE5 area
                and display coil currents.

  $Log$
  Revision 1.4  2005/03/21 10:52:46  ritt
  Added mcast display

  Revision 1.3  2005/03/18 13:44:27  ritt
  Switched to multicast mode

  Revision 1.2  2005/03/17 14:19:38  ritt
  Added TCP code

  Revision 1.1  2005/03/16 14:20:34  ritt
  Initial revision

\********************************************************************/

#include <stdio.h>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define MCAST_GROUP  "239.208.0.0"

main()
{
   int sock, n;
   char str[80];
   struct sockaddr_in myaddr, addr;
   struct ip_mreq req;
   int size;

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
   memset(&req, 0, sizeof(req));
   req.imr_multiaddr.s_addr = inet_addr(MCAST_GROUP);
   req.imr_interface.s_addr = INADDR_ANY;
   
   if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&req, sizeof(req)) < 0)
      perror("setsockopt IP_ADD_MEMBERSHIP");

   printf("Waiting for data from multicast group %s ...\n", MCAST_GROUP);
   size = sizeof(addr);
   do {
      memset(str, 0, sizeof(str));
      n = recvfrom(sock, str, sizeof(str), 0, (struct sockaddr *)&addr, &size);

      /* just print string */
      printf(str);
   } while (1);
}

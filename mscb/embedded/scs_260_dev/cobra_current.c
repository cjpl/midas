/********************************************************************\

  Name:         cobra_current.c
  Created by:   Stefan Ritt

  Contents:     Receive broadcasts from COBRA magnet in PiE5 area
                and display coil current. This program needs to run
                in subnet of the experimental hall.

  $Log$
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

main()
{
   int sock, size, n;
   char str[80];
   struct sockaddr_in myaddr, addr;

#if defined( _MSC_VER )
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return -1;
   }
#endif

   sock = socket(AF_INET, SOCK_DGRAM, 0);

   /* bind to port 1178 to receive cobra current broadcasts */
   memset(&myaddr, 0, sizeof(myaddr));
   myaddr.sin_family = AF_INET;
   myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   myaddr.sin_port = htons(1178);
   bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr));

   size = sizeof(addr);
   do {
      memset(str, 0, sizeof(str));
      n = recvfrom(sock, str, sizeof(str), 0, (struct sockaddr *)&addr, &size);

      /* just print string */
      printf(str);
   } while (1);

   printf("Hello\n");
}

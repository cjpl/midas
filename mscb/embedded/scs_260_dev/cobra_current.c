/********************************************************************\

  Name:         cobra_current.c
  Created by:   Stefan Ritt

  Contents:     Receive broadcasts from COBRA magnet in PiE5 area
                and display coil current. This program needs to run
                in subnet of the experimental hall.

  $Log$
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

#define UDP // Remove this line for TCP code

main()
{
   int sock, size, n;
   char str[80], host_name[80];
   struct sockaddr_in myaddr, addr;
   struct hostent *phe;

#if defined( _MSC_VER )
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return -1;
   }
#endif

#ifdef UDP // UDP code ---------------------------------------
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

#else // TCP code --------------------------------------------

   strcpy(host_name, "MSCB001");

   sock = socket(AF_INET, SOCK_STREAM, 0);

   /* let OS choose any port number */
   memset(&myaddr, 0, sizeof(myaddr));
   myaddr.sin_family = AF_INET;
   myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   myaddr.sin_port = 0;
   bind(sock, (void *) &myaddr, sizeof(myaddr));

   /* connect to remote node */
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = 0;
   addr.sin_port = htons(23); // telnet port

   phe = gethostbyname(host_name);
   if (phe == NULL) {
      printf("Cannot resolve host name %s\n", host_name);
      return -1;
   }
   memcpy((char *) &(addr.sin_addr), phe->h_addr, phe->h_length);

   if (connect(sock, (void *) &addr, sizeof(addr)) != 0) {
      printf("Cannot connect to %s\n", host_name);
      return -1;
   }

   do {
      memset(str, 0, sizeof(str));
      n = recv(sock, str, sizeof(str), 0);

      /* just print string */
      printf(str);
   } while (1);

#endif
}

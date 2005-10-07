/********************************************************************\

  Name:         rs232test.c
  Created by:   Stefan Ritt

  Contents:     Simple RS232 test program, needs  
                midas/drivers/bus/rs232.c

  $Id:$

\********************************************************************/

#include <midas.h>
#include "rs232.c"

int main()
{
   RS232_INFO info;
   char str[10000];

   printf("Enter port [/dev/ttyS0]: ");
   fgets(str, sizeof(str), stdin);
   if (strchr(str, '\n'))
      *strchr(str, '\n') = 0;

   if (!str[0])
      strcpy(str, "/dev/ttyS0");

   info.fd = rs232_open(str, 9600, 'N', 8, 1, 0);

   if (info.fd < 0) {
      printf("Cannot open ttyS0\n");
      return 0;
   }

   /* turn on debugging, will go to rs232.log */
   rs232(CMD_DEBUG, TRUE);

   printf("Connected to ttyS0, exit with <ESC>\n");

   do {
      memset(str, 0, sizeof(str));
      str[0] = ss_getchar(0);
      if (str[0] == 27)
         break;

      if (str[0] > 0)
         rs232_puts(&info, str);

      rs232_gets(&info, str, sizeof(str), "", 10);
      printf(str);
      fflush(stdout);

   } while (1);

   ss_getchar(TRUE);
   rs232_exit(&info);

   return 1;
}

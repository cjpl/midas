/********************************************************************\

  Name:         rs232test.c
  Created by:   Stefan Ritt

  Contents:     Simple RS232 test program, needs  
                midas/drivers/bus/rs232.c

  $Log$
  Revision 1.1  2001/01/05 15:08:50  midas
  Initial revision

\********************************************************************/

#include <midas.h>
#include "rs232.c"

int main()
{
RS232_INFO info;
char str[10000];

  /* turn on debugging, will go to rs232.log */
  rs232(CMD_DEBUG, TRUE);
  info.fd = rs232_open("/dev/ttyS0", 9600, 'N', 8, 1);

  if (info.fd < 0)
    {
    printf("Cannot open ttyS0\n");
    return 0;
    }
  
  printf("Connected to ttyS0, exit with <ESC>\n");
  
  do
    {
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

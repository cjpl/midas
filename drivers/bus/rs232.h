/********************************************************************\

  Name:         rs232.h
  Created by:   Stefan Ritt

  Contents:     Header file for RS232 communication routines

  $Log$
  Revision 1.1  1999/12/20 10:18:11  midas
  Reorganized driver directory structure

  Revision 1.2  1998/10/12 12:18:58  midas
  Added Log tag in header


\********************************************************************/

int  rs232_init(char *port, int baud, char parity,
              int data_bit, int stop_bit, int mode);
void rs232_exit(int hDev);
void rs232_putc(int port, char c);
void rs232_puts(int port, char *str);
int  rs232_gets(int port, char *s, int size, int end_char);
int  rs232_waitfor(int port, char *s, char *retstr, int size, int timeout);
void rs232_debug(BOOL flag);
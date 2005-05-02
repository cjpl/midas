/********************************************************************\

   Name:         strlcpy.h
   Created by:   Stefan Ritt

   Contents:     Header file for strlcpy.c

   $Log$
   Revision 1.1  2005/05/02 10:24:49  ritt
   *** empty log message ***

   Revision 1.1  2005/05/02 10:20:33  ritt
   Initial revision


\********************************************************************/

size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);

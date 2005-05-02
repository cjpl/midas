/********************************************************************\

   Name:         strlcpy.h
   Created by:   Stefan Ritt

   Contents:     Header file for strlcpy.c

   $Log$
   Revision 1.3  2005/05/02 10:50:42  ritt
   Moved strlcpy/strlcat in separate C file

   Revision 1.1  2005/05/02 10:20:33  ritt
   Initial revision


\********************************************************************/

#if defined(EXPORT_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

size_t EXPRT strlcpy(char *dst, const char *src, size_t size);
size_t EXPRT strlcat(char *dst, const char *src, size_t size);

/********************************************************************\

  Name:         MIDASINC.H
  Created by:   Stefan Ritt

  Purpose:      Centralized file with all #includes
  Contents:     Includes all necessary include files

  $Id$

\********************************************************************/

/* OS independent files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

/* OS dependent files */

#ifdef OS_VMS

#include <descrip.h>
#include <errno.h>
#include <in.h>
#include <inet.h>
#include <lckdef.h>
#include <lkidef.h>
#include <netdb.h>
#include <ppl$def.h>
#include <ppl$routines.h>
#include <ppldef.h>
#include <prcdef.h>
#include <socket.h>
#include <starlet.h>
#include <stdarg.h>
#include <ssdef.h>
#include <ucx$inetdef.h>
#include <file.h>
#include <stat.h>
#include <ctype.h>

#ifdef __ALPHA
#include <unixio.h>
#include <unixlib.h>
#include <lib$routines.h>
#endif

#endif

#ifdef OS_WINNT

#include <windows.h>
#include <conio.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#endif

#ifdef OS_MSDOS

#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netdb.h>
#include <in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <process.h>
#include <dos.h>
#include <alloc.h>
#include <errno.h>
#include <conio.h>
#include <ctype.h>

#endif

#ifdef OS_VXWORKS

#include <vxWorks.h>
#include <sockLib.h>
#include <selectLib.h>
#include <hostLib.h>
#include <in.h>
#include <ioLib.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
/*-PAA- permanent as semphore moved in ss_mutex_...() */
#include <semLib.h>
#include <taskLib.h>
#include <symLib.h>
#include <errnoLib.h>

#endif

#ifdef OS_UNIX

#ifndef __USE_GNU
#define __USE_GNU // needed for semtimedop()
#endif

#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <pwd.h>
#include <fcntl.h>
#include <syslog.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <sys/stat.h>

#if defined(OS_DARWIN)
/* mtio.h vanished from MacOS 10.6 */
#elif defined(OS_LINUX)
#include <sys/mtio.h>
#endif

#include <sys/syscall.h>
#include <dirent.h>
#include <pthread.h>

/* special code for Linux and FreeBSD */
#if defined(OS_LINUX) || defined(OS_FREEBSD)
#include <sys/time.h>
#ifdef OS_DARWIN
#else
#include <linux/unistd.h>
#include <sys/vfs.h>
#endif
#include <arpa/inet.h>
#include <fnmatch.h>
#ifdef OS_DARWIN
#include <util.h>
#else
#include <pty.h>
#endif
#undef LITTLE_ENDIAN
#endif

/* special code for OSF1 */
#ifdef OS_OSF1
#include <sys/time.h>
#include <sys/timers.h>

/*-PAA- Special for Ultrix */
#ifndef OS_ULTRIX
#include <fnmatch.h>
#define NO_PTY
#endif

extern void *malloc();          /* patches for wrong gcc include files */
extern void *realloc();
#endif

#ifdef OS_IRIX
#include <sys/statfs.h>
#include <fnmatch.h>
#define NO_PTY
#endif

#include <sys/wait.h>

#endif

/* special code for Solaris */
#ifdef OS_SOLARIS
#include <sys/filio.h>
#include <sys/statvfs.h>
#define NO_PTY
#endif

/* FTP library */
#include "ftplib.h"

/*------------------------------------------------------------------*/

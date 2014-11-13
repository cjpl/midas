#
# Find the OS specific flags and targets
#

# TODO: setup cross-compilation options ...

# Linux
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OS_EXTRA_UTILS   mlxspeaker dio)
  set(NEED_STRLCPY     TRUE)
  set(OS_EXTRA_LDFLAGS -lutil -lrt -ldl)
  set(OS_EXTRA_DEFS    -DOS_LINUX -fPIC -Wno-unused-function -D_LARGEFILE64_SOURCE )
  if(${CMAKE_SIZEOF_VOID_P} EQUAL 8) # 8*8 = 64-bit
    message(STATUS "Found a 64bit Linux system")
    set(OS_EXTRA_DEFS  ${OS_EXTRA_DEFS} -m64)
  else(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    message(STATUS "Found a 32bit system")
    set(OS_EXTRA_DEFS  ${OS_EXTRA_DEFS} -m32)
  endif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# Apple: Mac OS X, or "Darwin"
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(OS_EXTRA_UTILS mlxspeaker)
  set(NEED_RPATH     TRUE)
  set(NEED_STRLCPY   TRUE)
  set(OS_EXTRA_DEFS  -DOS_LINUX -DOS_DARWIN)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Windows cygwin
if(CYGWIN)
  set(OS_EXTRA_LDFLAGS -lutil)
  set(OS_EXTRA_DEFS    -DOS_LINUX -DOS_CYGWIN)
endif(CYGWIN)

# Solaris: use "SunOS" here
if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
  set(OS_EXTRA_LDFLAGS -lsocket -lnsl)
  set(OS_EXTRA_DEFS    -DOS_SOLARIS)
endif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

# FreeBSD
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  set(OS_EXTRA_LDFLAGS  -lbsd -lcompat)
  set(OS_EXTRA_DEFS     -DOS_FREEBSD)
endif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

# OSF1 (Is cmake avalible on this platform?)
if(${CMAKE_SYSTEM_NAME} MATCHES "OSF1")
  set(OS_EXTRA_LDFLAGS  -lc -lbsd)
  set(OS_EXTRA_DEFS     -OS_OSF1 )
  ## FFLAGS = -nofor_main -D 40000000 -T 20000000
  ## Fortran code is not used, why the FFLAG is here? May ask Stephen?
  enable_language(Fortran) # Should I?
  set (CMAKE_Fortran_FLAGS_RELEASE -nofor_main -D 40000000 -T 20000000)
  set (CMAKE_Fortran_FLAGS_DEBUG   -nofor_main -D 40000000 -T 20000000)
endif(${CMAKE_SYSTEM_NAME} MATCHES "OSF1")

# Ultrix (Is cmake avalible on this platform?)
if(${CMAKE_SYSTEM_NAME} MATCHES "ULTRIX")
  set(OS_EXTRA_DEFS -OS_ULTRIX -DNO_PTY)
endif(${CMAKE_SYSTEM_NAME} MATCHES "ULTRIX")

## Messages
message(STATUS "Current System: ${CMAKE_SYSTEM_NAME}")
message("\tExtra utilitis:    ${OS_EXTRA_UTILS}")
message("\tExtra link flags:  ${OS_EXTRA_LDFLAGS}")
message("\tExtra definitions: ${OS_EXTRA_DEFS}")

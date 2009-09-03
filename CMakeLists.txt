#-*- mode: cmake -*-

## CMake pre-settings
cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)
cmake_policy(SET CMP0011 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

## Project setup
project(MIDAS)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)
enable_language(C)
enable_language(CXX)

## Include utilities and options
# Find mxml
include(FindMXML)
include_directories(include ${MXML_PATH})

# OS specific flags
# OS-related CMake variables:
#  1. OS_EXTRA_UTILS   --- Extra utilities for specific OS
#  2. OS_EXTRA_LDFLAGS --- Extra link flags for specific OS
#  3. OS_EXTRA_DEFS    --- Extra definitions for specific OS
#  4. Other flags switched on for specific OS
include(OSflags)

include(FindROOT)  # Find ROOT environment
include(FindMySQL) # Find MySQL
include(FindODBC)  # Find ODBC
include(FindZLIB)  # Find zlib

## Options for MIDAS
set(NEED_CERNLIB TRUE)  # Need CERNLIB or not?
set(NEED_SHLIB   TRUE)  # Need shared library?
set(NEED_ROOT    TRUE)  # Need root?
set(NEED_FTPLIB  TRUE)  # Need ftp protocol?
#set(NEED_STRLCPY TRUE)  # Need strlcpy.h? -- maybe opened in OSflags.cmake
#set(MIDAS_MAX_EVENT_SIZE 1000000)  # maxmimum event size = ?

##############################################################################
## Setup options

## OS-specific
add_definitions(${OS_EXTRA_DEFS})
set(LDFLAGS "${LDFLAGS} ${OS_EXTRA_LDFLAGS}")
set(EXTRA_UTILS ${EXTRA_UTILS} ${OS_EXTRA_UTILS})

## Midas max event size
if(MIDAS_MAX_EVENT_SIZE)
  add_definitions(-DMAX_EVENT_SIZE=${MIDAS_MAX_EVENT_SIZE})
endif(MIDAS_MAX_EVENT_SIZE)

## strlcpy in "MXML"?
if(NEED_STRLCPY)
  set(EXTRA_OBJS ${EXTRA_OBJS} strlcpy)
  add_definitions( -DHAVE_STRLCPY )
  message(STATUS "Using Stephen's own strlcpy()")
endif(NEED_STRLCPY)

## FTPLIB needed?
if(NEED_FTPLIB)
  set(FTPLIB ftplib)
  add_definitions(-DINCLUDE_FTPLIB)
  message(STATUS "Using ftplib (FTP library)")
endif(NEED_FTPLIB)

## ODBC: -lodbc or -liodbc
if(ODBC_FOUND)
  message(STATUS "Add ODBC-specified targets")
  #set(CPP_UTILS ${CPP_UTILS} mh2sql)
  set(EXTRA_UTILS ${EXTRA_UTILS} mh2sql)
  add_definitions(-DHAVE_ODBC)
  #set(LDFLAGS "${LDFLAGS} -l${ODBC_LIBRARY}")
  set(LDFLAGS "${LDFLAGS} -l${ODBC_NAME}")
else(ODBC_FOUND)
  message(STATUS "ODBC not found!")
endif(ODBC_FOUND)

## ZLIB
if(ZLIB_FOUND)
  add_definitions(-DHAVE_ZLIB)
  set(LDFLAGS "${LDFLAGS} -lz")
endif(ZLIB_FOUND)

## ROOT
if(NEED_ROOT AND ROOT_FOUND)
  message(STATUS "Using ROOT at ${ROOTSYS}")

  if(NEED_RPATH)      # Add rpath ..
    set(ROOT_LIBS "${ROOT_LIBS} -Wl,-rpath,${ROOTSYS}/lib")
  endif(NEED_RPATH)

  ## libRoot.a: Static ROOT library
  if(NEED_LIBROOTA AND ROOT_LIBA)
    set(LDFLAGS "${LDFLAGS} ${ROOTSYS}/lib/libRoot.a -lssl -ldl -lcrypt")
    message(STATUS "Using statlic ROOT library: ${ROOT_LIBA}")
  endif(NEED_LIBROOTA AND ROOT_LIBA)
endif(NEED_ROOT AND ROOT_FOUND)

####### MIDAS library and objects #########################
## Objects needed to build the libmidas(.a,.so,..)

foreach(obj ${FTPLIB} midas system mrpc odb ybos history alarm elog)
  set(LIB_OBJS ${LIB_OBJS} src/${obj}.c)
endforeach(obj)
foreach(obj mxml ${EXTRA_OBJS})
  set(LIB_OBJS ${LIB_OBJS} ${MXML_PATH}/${obj}.c)
endforeach(obj)
if(ODBC_FOUND)
  set(LIB_OBJS ${LIB_OBJS} src/history_odbc.cxx)
endif(ODBC_FOUND)

## Library definition
## Add static lib
add_library(midas-static STATIC ${LIB_OBJS})
set_target_properties(midas-static PROPERTIES OUTPUT_NAME "midas")

if(NEED_SHLIB)  # Add shared lib
  add_library(midas-shared SHARED ${LIB_OBJS})
  set_target_properties(midas-shared PROPERTIES OUTPUT_NAME "midas")
endif(NEED_SHLIB)

########## Utilities ######################################

## Core Utilties
set(CORE_UTILS mserver mhttpd mlogger odbedit lazylogger)

add_executable(mserver    src/mserver.c)
add_executable(mlogger    src/mlogger.c)
add_executable(mhttpd     src/mhttpd.c  src/mgd.c )
add_executable(odbedit    src/odbedit.c src/cmdedit.c )
add_executable(lazylogger src/lazylogger.c)
foreach(exe ${CORE_UTILS})
  target_link_libraries(${exe} midas-static)
  set_target_properties(${exe} PROPERTIES LINK_FLAGS "${LDFLAGS}")
endforeach(exe)

set_target_properties( mhttpd PROPERTIES
  LINK_FLAGS "-lm ${LDFLAGS}" LINKER_LANGUAGE CXX )

## for mlogger --- It's BAD to name a CXX source using ".c" file extension !!!
set_target_properties( mlogger 
  PROPERTIES
  LINK_FLAGS "${MYSQL_LIBS} ${ROOT_LIBS} ${LDFLAGS}"
  LINKER_LANGUAGE CXX
  COMPILE_FLAGS "${ROOT_CFLAGS}" )
set_source_files_properties( src/mlogger.c  PROPERTIES LANGUAGE CXX )

## Utilities
set(UTILS mtape    mhist  mstat  mdump   mchart
  webpaw  odbhist  melog  mcnaf
  mhdump  mtransition # C++ codes
  ${EXTRA_UTILS}  )

add_executable(mtape   utils/mtape.c)
add_executable(mhist   utils/mhist.c)
add_executable(mstat   utils/mstat.c)
add_executable(mdump   utils/mdump.c)
add_executable(melog   utils/melog.c)
add_executable(mchart  utils/mchart.c)
add_executable(odbhist utils/odbhist.c)
add_executable(webpaw  utils/webpaw.c)
add_executable(mhdump  utils/mhdump.cxx)
add_executable(mcnaf   utils/mcnaf.c  drivers/camac/camacrpc.c)
add_executable(mtransition src/mtransition.cxx)

# optional utilities
if("${UTILS}" MATCHES "mh2sql")
  add_executable(mh2sql utils/mh2sql.cxx src/history_odbc.cxx )
endif("${UTILS}" MATCHES "mh2sql")
if("${UTILS}" MATCHES "mlxspeaker")
  add_executable(mlxspeaker utils/mlxspeaker.c)
endif("${UTILS}" MATCHES "mlxspeaker")
if("${UTILS}" MATCHES "dio")
  add_executable(dio utils/dio.c)
endif("${UTILS}" MATCHES "dio")

foreach(exe ${UTILS})
  add_dependencies(${exe} midas-static)
  target_link_libraries(${exe} midas-static)
  set_target_properties(${exe} PROPERTIES  LINK_FLAGS "${LDFLAGS}")
endforeach(exe)

## Scripts
set(SCRIPTS stripchart.tcl )   # TCL script

########## Special libraries ################################
#
# 'Cause CMake cannot handle "*.o" target, so Change these into libraries.
# All apps need to merge with these objects should be linked with the
# corresponding libraries.

add_library( midas_fe  src/mfe.c )
add_library( midas_fal  src/fal.c )
set_target_properties( midas_fal PROPERTIES
  COMPILE_FLAGS "-Dextname -DMANA_LITE" )

## for generating mfe.o ??
# add_custom_command( OUTPUT  ${PROJECT_BINARY_DIR}/lib/mfe.o
#   DEPENDS src/mfe.c
#   COMMAND ${CMAKE_C_COMPILER}
#   ARGS ${CMAKE_C_FLAGS} -c  )

## analyzer objects
add_library( midas_ana src/mana.c )
set_target_properties(midas_ana PROPERTIES LINKER_LANGUAGE C)

# rmana.o
if(NEED_ROOT)
  add_library( midas_rana src/mana.c)
  set_target_properties( midas_rana PROPERTIES
    LINKER_LANGUAGE CXX  
    COMPILE_FLAGS "-DUSE_ROOT ${ROOT_CFLAGS}" )
  set_source_files_properties( src/mana.c PROPERTIES LANGUAGE CXX )
endif(NEED_ROOT)

## hmana.o
# How to rebuild mama.c again using "C" not "C++" (?)
# if(NEED_CERNLIB)
#   add_library( midas_hana src/mana.c )
#   set_source_files_properties( src/mana.c
#     PROPERTIES LANGUAGE C)
#   set_target_properties( midas_hana PROPERTIES
#     COMPILE_FLAGS "-Dextname -DHAVE_HBOOK"
#     LINKER_LANGUAGE C)
# endif(NEED_CERNLIB)

##########  Drivers and modules ################################
#

##########  Examples ###########################################
#

##########  Installation########################################
#


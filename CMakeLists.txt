#-*- mode: cmake -*-

## CMake pre-settings
cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)
#cmake_policy(SET CMP0010 OLD)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

################################################################
## Project setup
project(MIDAS C CXX)
set(MIDAS_VERSION_MAJOR 2)
set(MIDAS_VERSION_MINOR 3)
set(MIDAS_VERSION_REVISION 0)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)

## install dirs
# if prefix = "/usr", better setup paths as
#   libexec = "/usr/lib/midas"
#   bindir  = "/usr/lib/midas/bin"
#   libdir  = "/usr/lib/midas/lib"
#   incdir  = "/usr/lib/midas/include"
#   datadir = "/usr/share/midas"
#   docdir  = "/usr/share/doc/midas"
# else:
#   libexec = <prefix>
#   libdir  = <prefix>/lib
#   bindir  = <prefix>/bin
#   incdir  = <prefix>/include
#   datadir = <prefix>/share
#   docdir  = <prefix>/doc
if(NOT DEFINED INSTALL_LIBEXECDIR)
  message(STATUS "Set default libexec as prefix")
  set(INSTALL_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}")
endif(NOT DEFINED INSTALL_LIBEXECDIR)
# libdir = ${libexec}/lib
set(INSTALL_LIBDIR "${INSTALL_LIBEXECDIR}/lib")
# incdir = ${libexec}/include
if(NOT DEFINED INSTALL_INCLUDEDIR)
  set(INSTALL_INCLUDEDIR "${INSTALL_LIBEXECDIR}/include")
endif(NOT DEFINED INSTALL_INCLUDEDIR)
# datadir = ${libexec}/share or ${prefix}/share/midas
if(NOT DEFINED INSTALL_DATADIR)
  set(INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/share")
endif(NOT DEFINED INSTALL_DATADIR)
# docdir = ${prefix}/share/doc/midas
if(NOT DEFINED INSTALL_DOCDIR)
  set(INSTALL_DOCDIR "${CMAKE_INSTALL_PREFIX}/doc")
endif(NOT DEFINED INSTALL_DOCDIR)

################################################################
## User-specified Options for MIDAS
#  You can specify it by using CMake options, e.g.
#    : cmake -DNEED_ROOT

#set(NEED_CERNLIB TRUE)  # Need CERNLIB or not?
set(NEED_SHLIB   TRUE)  # Need shared library?
set(NEED_FTPLIB  TRUE)  # Need ftp protocol?
set(NEED_ROOT    TRUE)
set(NEED_MSCB    FALSE)
# set(NEED_LIBROOTA TRUE) # use static root library?

# maxmimum event size = (default = 0x400000, 4M)
#set(MIDAS_MAX_EVENT_SIZE 1000000)

################################################################
## Include utilities and options
include_directories(include)

# Find mxml
include(FindMXML)
include_directories(${MXML_PATH})

## OS-related CMake variables:
#  1. OS_EXTRA_UTILS   --- Extra utilities for specific OS
#  2. OS_EXTRA_LDFLAGS --- Extra link flags for specific OS
#  3. OS_EXTRA_DEFS    --- Extra definitions for specific OS
#  4. Other flags switched on for specific OS
include(MIDASOSflags)

## MSCB
if(NEED_MSCB)
  set(midas_cflags ${midas_cflags} -DHAVE_MSCB)
  # Find mscb
  include(FindMSCB)
  if(MSCB_FOUND)
    message("** MSCB is found in ${MSCB_PATH}.")
    include_directories(${MSCB_PATH})
    set(MSCB_SRC "${MSCB_PATH}/mscb.c")
  else(MSCB_FOUND)
    message("** MSCB is not found.")
  endif(MSCB_FOUND)
endif(NEED_MSCB)

## Find ROOT environment
include(FindROOT)
if(ROOT_FOUND)
  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --cflags
    OUTPUT_VARIABLE ROOT_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --libs
    OUTPUT_VARIABLE ROOT_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
else(ROOT_FOUND)
  message("** WARNING: ROOTSYS not found in your system!")
  message("            All ROOT related codes will not be compiled!")
endif(ROOT_FOUND)

# Other packages
include(FindMySQL)   # Find MySQL
include(FindODBC)    # Find ODBC
include(FindZLIB)    # Find zlib
include(FindSqlite3) # Find sqlite3

################################################################
## Setup options

# Threads
find_package (Threads)
if(Threads_FOUND)
  set(OS_EXTRA_LDFLAGS ${OS_EXTRA_LDFLAGS} -lpthread)
  message(STATUS "Add -lpthread")
endif(Threads_FOUND)

## OS-specific utilities
set(EXTRA_UTILS ${EXTRA_UTILS} ${OS_EXTRA_UTILS})
set(midas_cflags ${midas_cflags} ${OS_EXTRA_DEFS})
set(midas_libs   ${OS_EXTRA_LDFLAGS})

## Midas max event size
if(MIDAS_MAX_EVENT_SIZE)
  set(midas_cflags ${midas_cflags} -DMAX_EVENT_SIZE=${MIDAS_MAX_EVENT_SIZE})
  message(STATUS "Max event size: ${MIDAS_MAX_EVENT_SIZE}")
endif(MIDAS_MAX_EVENT_SIZE)

## strlcpy in "MXML"?
if(NEED_STRLCPY)
  set(STRLCPY_OBJ strlcpy)
  set(midas_cflags ${midas_cflags}  -DHAVE_STRLCPY)
  message(STATUS "Using Stephen's own strlcpy()")
endif(NEED_STRLCPY)

## FTPLIB needed?
if(NEED_FTPLIB)
  set(FTPLIB ftplib)
  set(midas_cflags ${midas_cflags} -DINCLUDE_FTPLIB)
  message(STATUS "Using ftplib (FTP library)")
endif(NEED_FTPLIB)

## ODBC: -lodbc or -liodbc (?)
if(ODBC_FOUND)
  message(STATUS "Add ODBC-related targets using ${ODBC_LIBRARY}")
  set(EXTRA_UTILS ${EXTRA_UTILS} mh2sql)
  set(midas_cflags ${midas_cflags} -DHAVE_ODBC)
  set(midas_libs   ${midas_libs}   -lodbc)
else(ODBC_FOUND)
  message(STATUS "ODBC not found!")
endif(ODBC_FOUND)

## SQLite3
if( SQLITE3_FOUND )
  message(STATUS "Add sqlite3-related options using ${SQLITE3_LIBRARIES}")
  set(midas_cflags ${midas_cflags} -DHAVE_SQLITE)
  set(midas_libs   ${midas_libs} -lsqlite3)
  # SQLITE3_INCLUDE_DIR
else( SQLITE3_FOUND )
  message(STATUS "SQLite3 not found!")
endif( SQLITE3_FOUND )

## ZLIB
if(ZLIB_FOUND)
  set(midas_cflags ${midas_cflags} -DHAVE_ZLIB)
  set(midas_libs   ${midas_libs} -lz)
endif(ZLIB_FOUND)

## MySQL
if(MYSQL_FOUND)
  message(STATUS "MySQL support will be added to mblogger")
  set(MYSQL_CFLAGS "-DHAVE_MYSQL -I${MYSQL_INCLUDE_DIR}")
endif(MYSQL_FOUND)

## ROOT
set(ROOT_ENABLED NO)
set(ROOT_STATIC_ENABLED NO)
if(NEED_ROOT AND ROOT_FOUND)
  message(STATUS "Using ROOTSYS (${ROOT_VERSION}) found at ${ROOTSYS}")
  set(ROOT_ENABLED YES)
  set(BUILD_ROOT_FLAGS "-DHAVE_ROOT ${ROOT_CFLAGS}")
endif(NEED_ROOT AND ROOT_FOUND)

################################################################
# 

## Add definitions
add_definitions(${midas_cflags})

## Convert LDFLAGS from list to string
foreach(f ${midas_libs})
  set(LDFLAGS "${f} ${LDFLAGS}")
endforeach(f)
set(MDAQ_LIBS ${LDFLAGS})

## convert midas_cflags to string
foreach(o ${midas_cflags})
  set(MDAQ_CFLAGS "${o} ${MDAQ_CFLAGS}")
endforeach(o)

## messages
set(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS}")
message(STATUS "MIDAS_CFLAGS: ${midas_cflags}")
message(STATUS "MIDAS_LIBS: ${midas_libs}")

################################################################

# Addons: mdaq-config
configure_file(addons/mdaq-config.in
  ${PROJECT_BINARY_DIR}/bin/mdaq-config
  @ONLY)

# pkg-config: mdaq.pc
configure_file(addons/mdaq.pc.in
  ${PROJECT_BINARY_DIR}/mdaq.pc
  @ONLY)

# MIDAS library and core executables
add_subdirectory(src)

# MIDAS utils
add_subdirectory(utils)

# MIDAS doc
#add_subdirectory(doc)

################################################################
# install: headers
install(DIRECTORY include/  DESTINATION ${INSTALL_INCLUDEDIR})
install(FILES
  ${MXML_PATH}/mxml.h  ${MXML_PATH}/strlcpy.h
  DESTINATION ${INSTALL_INCLUDEDIR})

# install: src, examples, drivers
install(DIRECTORY src
  DESTINATION ${INSTALL_DATADIR}
  FILES_MATCHING REGEX ".(c|cxx)")
install(DIRECTORY examples  DESTINATION ${INSTALL_DATADIR})
install(DIRECTORY drivers   DESTINATION ${INSTALL_DATADIR})
install(FILES  odb.xsd utils/notify.wav src/elogd.cfg
  DESTINATION ${INSTALL_DATADIR})

# install addons
install(FILES addons/exptab addons/mdaq.yaml
  DESTINATION ${INSTALL_DATADIR})
install(FILES ${PROJECT_BINARY_DIR}/mdaq.pc
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
install(PROGRAMS addons/mdaq  ${PROJECT_BINARY_DIR}/bin/mdaq-config
  DESTINATION bin)
install(DIRECTORY cmake  DESTINATION ${INSTALL_DATADIR})

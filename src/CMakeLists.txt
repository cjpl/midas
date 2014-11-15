
################################################################
# MIDAS library

## Objects needed to build the libmidas.a
set( LIB_OBJS
  midas.c
  system.c
  mrpc.c
  odb.c
  ${MXML_PATH}/mxml.c
  dm_eb.c
  history_midas.cxx
  history_sql.cxx
  history.c
  alarm.c
  elog.c )
if(NEED_STRLCPY)
  set(LIB_OBJS  ${LIB_OBJS}  ${MXML_PATH}/strlcpy.c)
endif(NEED_STRLCPY)
if(NEED_FTPLIB)
  set(LIB_OBJS  ${LIB_OBJS}  ${FTPLIB}.c)
endif(NEED_FTPLIB)

## Add static lib
add_library(midas STATIC ${LIB_OBJS})

## Add shared lib
if(NEED_SHLIB)
  add_library(midas-shared SHARED ${LIB_OBJS})
endif(NEED_SHLIB)

################################################################
# Special targets: objects (*.o)

## generating mfe.o
add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/mfe.o
  DEPENDS mfe.c
  COMMAND ${CMAKE_C_COMPILER}
  ARGS -I${CMAKE_SOURCE_DIR}/include -c ${CMAKE_SOURCE_DIR}/src/mfe.c ${CFLAGS}
  WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_target(mfe ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/mfe.o)

## generating fal.o
add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/fal.o
  DEPENDS fal.c
  COMMAND ${CMAKE_CXX_COMPILER}
  ARGS -I${CMAKE_SOURCE_DIR}/include -I${MXML_PATH} -Dextname -DMANA_LITE -c ${CMAKE_SOURCE_DIR}/src/fal.c
  WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_target(fal ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/fal.o)

## analyzer objects: mana.o
add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/mana.o
  DEPENDS mana.c
  COMMAND ${CMAKE_C_COMPILER}
  ARGS -I${CMAKE_SOURCE_DIR}/include ${CFLAGS} -c ${CMAKE_SOURCE_DIR}/src/mana.c
  WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_target(mana ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/mana.o)

set(EXTRA_MOBJS mfe.o fal.o mana.o)

## hmana.o
if(NEED_CERNLIB)
  add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/hmana.o
    DEPENDS mana.c
    COMMAND ${CMAKE_C_COMPILER}
    ARGS -I${CMAKE_SOURCE_DIR}/include -Dextname -DHAVE_HBOOK -o hmana.o -c ${CMAKE_SOURCE_DIR}/src/mana.c
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
  add_custom_target(hmana ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/hmana.o)
  set(EXTRA_MOBJS ${EXTRA_MOBJS} hmana.o )
endif(NEED_CERNLIB)

# rmana.o
if(ROOT_ENABLED)
  set(ARG_LIST "-DUSE_ROOT ${ROOT_CFLAGS} -I${CMAKE_SOURCE_DIR}/include -o rmana.o -c ${CMAKE_SOURCE_DIR}/src/mana.c")
  separate_arguments(ARG_LIST)
  add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/rmana.o
    DEPENDS mana.c
    COMMAND ${CMAKE_CXX_COMPILER}
    ARGS ${ARG_LIST}
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
  add_custom_target(rmana ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/rmana.o )
  set(EXTRA_MOBJS ${EXTRA_MOBJS} rmana.o )
endif(ROOT_ENABLED)

############################################################
## Core Utilties
set( CORE_UTILS  mserver  mhttpd  mlogger odbedit lazylogger  mtransition )

add_executable(mserver     mserver.c)
add_executable(mhttpd      mhttpd.cxx mgd.c sequencer.cxx ${MSCB_SRC})
add_executable(mlogger     mlogger.cxx)
add_executable(odbedit     odbedit.c  cmdedit.c)
add_executable(lazylogger  lazylogger.cxx  mdsupport.c)
add_executable(mtransition mtransition.cxx)

## mhttpd -- something special
set_target_properties(mhttpd
  PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-lm ${LDFLAGS}")

## mlogger
# MySQL and with ROOT?
set_target_properties(mlogger
  PROPERTIES
  LINKER_LANGUAGE CXX
  COMPILE_FLAGS "${MYSQL_CFLAGS} ${BUILD_ROOT_FLAGS}"
  LINK_FLAGS    "${MYSQL_LIBRARIES} ${ROOT_LIBS} ${LDFLAGS}")

## Core LINK Tests
set( LINK_TESTS  mfe_link_test mana_link_test )

# mfe_link_test
add_executable(mfe_link_test mfe.c)
set_target_properties(mfe_link_test PROPERTIES
  COMPILE_FLAGS "-DLINK_TEST ${CFLAGS}"  LINKER_LANGUAGE C)

# mana_link_test
add_executable(mana_link_test mana.c)
set_target_properties(mana_link_test PROPERTIES
  COMPILE_FLAGS "-DLINK_TEST ${CFLAGS}"  LINKER_LANGUAGE C)

## All targets need to link to midas library

foreach(exe ${CORE_UTILS} ${LINK_TESTS})
  target_link_libraries(${exe} midas)
endforeach(exe)

################################################################
# installation
## Library and objects (custom targets)
install(TARGETS midas ARCHIVE DESTINATION ${INSTALL_LIBDIR})
if(NEED_SHLIB)
  install(TARGETS midas-shared LIBRARY DESTINATION ${INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)
endif(NEED_SHLIB)

foreach(obj ${EXTRA_MOBJS})
  install(FILES ${PROJECT_BINARY_DIR}/lib/${obj} DESTINATION ${INSTALL_LIBDIR})
endforeach(obj)

## Core and extra utilities, scripts
install(TARGETS ${CORE_UTILS} ${LINK_TEST}
  RUNTIME DESTINATION ${INSTALL_LIBEXECDIR}/bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)
# install mhttpd with setuid
install(TARGETS mhttpd
  RUNTIME DESTINATION ${INSTALL_LIBEXECDIR}/bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE SETUID
  GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)

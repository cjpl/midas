
## Extra utilities
set(UTILS
  mtape   mhist  mstat  mdump  mhdump  mchart  webpaw  odbhist  melog
  mcnaf  ${EXTRA_UTILS} )

add_executable(mtape   mtape.c)
add_executable(mhist   mhist.c)
add_executable(mstat   mstat.c)
add_executable(mdump   mdump.c ../src/mdsupport.c)
add_executable(mhdump  mhdump.cxx)
add_executable(mchart  mchart.c)
add_executable(webpaw  webpaw.c)
add_executable(odbhist odbhist.c)
add_executable(melog   melog.c)
add_executable(mcnaf   mcnaf.c ../drivers/camac/camacrpc.c)

if("${UTILS}" MATCHES "mh2sql")
  add_executable(mh2sql  mh2sql.cxx)
endif("${UTILS}" MATCHES "mh2sql")
if("${UTILS}" MATCHES "mlxspeaker")
  add_executable(mlxspeaker mlxspeaker.c)
endif("${UTILS}" MATCHES "mlxspeaker")
if("${UTILS}" MATCHES "dio")
  add_executable(dio dio.c)
endif("${UTILS}" MATCHES "dio")

foreach(exe ${UTILS})
  add_dependencies(${exe} midas)
  target_link_libraries(${exe} midas)
endforeach(exe)

## Scripts: how to create symlink `stripchart`?
set(SCRIPTS
  stripchart.tcl
  mcleanup
  lazy_castor.perl
  lazy_copy.perl
  lazy_dcache.perl
  lazy_test.perl)

################################################################
## installation
install(TARGETS ${UTILS}
  RUNTIME DESTINATION ${INSTALL_LIBEXECDIR}/bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)
install(PROGRAMS ${SCRIPTS}
  DESTINATION ${INSTALL_LIBEXECDIR}/bin)

# install with setuid: webpaw, dio
install(TARGETS webpaw
  RUNTIME DESTINATION ${INSTALL_LIBEXECDIR}/bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE SETUID
  GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)
if("${UTILS}" MATCHES "dio")
  install(TARGETS dio
    RUNTIME DESTINATION ${INSTALL_LIBEXECDIR}/bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE SETUID
    GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)
endif("${UTILS}" MATCHES "dio")

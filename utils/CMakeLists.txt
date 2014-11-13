
## Extra utilities
set(UTILS
  mtape   mhist  mstat  mdump  mhdump  mchart  webpaw  odbhist  melog
  mh2sql  mjson_test    mcnaf  ${EXTRA_UTILS} )

add_executable(mtape   mtape.c)
add_executable(mhist   mhist.cxx)
add_executable(mstat   mstat.c)
add_executable(mdump   mdump.cxx ../src/mdsupport.cxx)
add_executable(mhdump  mhdump.cxx)
add_executable(mchart  mchart.c)
add_executable(webpaw  webpaw.c)
add_executable(odbhist odbhist.c)
add_executable(melog   melog.c)
add_executable(mh2sql  mh2sql.cxx)
add_executable(mjson_test  mjson_test.cxx)
add_executable(mcnaf       mcnaf.c ../drivers/camac/camacrpc.c)

if("${UTILS}" MATCHES "mlxspeaker")
  add_executable(mlxspeaker mlxspeaker.c)
endif("${UTILS}" MATCHES "mlxspeaker")
if("${UTILS}" MATCHES "dio")
  add_executable(dio dio.c)
endif("${UTILS}" MATCHES "dio")

foreach(exe ${UTILS})
  add_dependencies(${exe} midas)
  target_link_libraries(${exe} midas)
  set_target_properties(${exe} PROPERTIES  LINK_FLAGS "${LDFLAGS}")
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

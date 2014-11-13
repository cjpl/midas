#
# Find the MSCB sources needed
#
# Provide:
#   MSCB_DIR  -- the directory contains mscb.h/c, mscbrpc.h/c
#

set(MSCB_FOUND FALSE)

find_path(MSCB_PATH mscb.h
  mscb/
  ../mscb/
  ${CMAKE_SOURCE_DIR}/../mscb/
  DOC "Specify the directory containing mscb.h"
  )

if(MSCB_PATH)
  set(MSCB_FOUND TRUE)
  message(STATUS "MSCB directory: ${MSCB_PATH}")
else(MSCB_PATH)
  message(FATAL_ERROR "Failed to found the MSCB source directory!")
endif(MSCB_PATH)


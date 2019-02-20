SET(DCMTK_VERSION_NUMBER 362)
SET(DCMTK_PACKAGE_VERSION "3.6.2")
SET(DCMTK_SOURCES_DIR ${CMAKE_BINARY_DIR}/dcmtk-3.6.2)
SET(DCMTK_URL "http://orthanc.osimis.io/ThirdPartyDownloads/dcmtk-3.6.2.tar.gz")
SET(DCMTK_MD5 "d219a4152772985191c9b89d75302d12")

macro(DCMTK_UNSET)
endmacro()

macro(DCMTK_UNSET_CACHE)
endmacro()

set(DCMTK_BINARY_DIR ${DCMTK_SOURCES_DIR}/)
set(DCMTK_CMAKE_INCLUDE ${DCMTK_SOURCES_DIR}/)
set(DCMTK_WITH_THREADS ON)

add_definitions(-DDCMTK_INSIDE_LOG4CPLUS=1)

if (IS_DIRECTORY "${DCMTK_SOURCES_DIR}")
  set(FirstRun OFF)
else()
  set(FirstRun ON)
endif()

DownloadPackage(${DCMTK_MD5} ${DCMTK_URL} "${DCMTK_SOURCES_DIR}")


if (FirstRun)
  # "3.6.2 CXX11 fails on Linux; patch suggestions included"
  # https://forum.dcmtk.org/viewtopic.php?f=3&t=4637
  message("Applying patch to detect mathematic primitives in DCMTK 3.6.2 with C++11")
  execute_process(
    COMMAND ${PATCH_EXECUTABLE} -p0 -N -i
    ${ORTHANC_ROOT}/Resources/Patches/dcmtk-3.6.2-cmath.patch
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE Failure
    )

  if (Failure)
    message(FATAL_ERROR "Error while patching a file")
  endif()
else()
  message("The patches for DCMTK have already been applied")
endif()


# C_CHAR_UNSIGNED *must* be set before calling "GenerateDCMTKConfigure.cmake"
IF (CMAKE_CROSSCOMPILING)
  if (CMAKE_COMPILER_IS_GNUCXX AND
      CMAKE_SYSTEM_NAME STREQUAL "Windows")  # MinGW
    SET(C_CHAR_UNSIGNED 1 CACHE INTERNAL "Whether char is unsigned.")

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")  # WebAssembly or asm.js

    # Check out "../WebAssembly/ArithmeticTests/" to regenerate the
    # "arith.h" file
    configure_file(
      ${ORTHANC_ROOT}/Resources/WebAssembly/arith.h
      ${DCMTK_SOURCES_DIR}/config/include/dcmtk/config/arith.h
      COPYONLY)

    UNSET(C_CHAR_UNSIGNED CACHE)
    SET(C_CHAR_UNSIGNED 0 CACHE INTERNAL "")

  else()
    message(FATAL_ERROR "Support your platform here")
  endif()
ENDIF()


if ("${CMAKE_SYSTEM_VERSION}" STREQUAL "LinuxStandardBase")
  SET(DCMTK_ENABLE_CHARSET_CONVERSION "iconv" CACHE STRING "")
  SET(HAVE_SYS_GETTID 0 CACHE INTERNAL "")

  execute_process(
    COMMAND ${PATCH_EXECUTABLE} -p0 -N -i
    ${ORTHANC_ROOT}/Resources/Patches/dcmtk-3.6.2-linux-standard-base.patch
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE Failure
    )

  if (FirstRun AND Failure)
    message(FATAL_ERROR "Error while patching a file")
  endif()
endif()

SET(DCMTK_SOURCE_DIR ${DCMTK_SOURCES_DIR})
include(${DCMTK_SOURCES_DIR}/CMake/CheckFunctionWithHeaderExists.cmake)
include(${DCMTK_SOURCES_DIR}/CMake/GenerateDCMTKConfigure.cmake)


if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")  # WebAssembly or
  # asm.js The macros below are not properly discovered by DCMTK
  # when using WebAssembly. Check out "../WebAssembly/arith.h" for
  # how we produced these values. This step MUST be after
  # "GenerateDCMTKConfigure" and before the generation of
  # "osconfig.h".
  UNSET(SIZEOF_VOID_P   CACHE)
  UNSET(SIZEOF_CHAR     CACHE)
  UNSET(SIZEOF_DOUBLE   CACHE)
  UNSET(SIZEOF_FLOAT    CACHE)
  UNSET(SIZEOF_INT      CACHE)
  UNSET(SIZEOF_LONG     CACHE)
  UNSET(SIZEOF_SHORT    CACHE)
  UNSET(SIZEOF_VOID_P   CACHE)

  SET(SIZEOF_VOID_P 4   CACHE INTERNAL "")
  SET(SIZEOF_CHAR 1     CACHE INTERNAL "")
  SET(SIZEOF_DOUBLE 8   CACHE INTERNAL "")
  SET(SIZEOF_FLOAT 4    CACHE INTERNAL "")
  SET(SIZEOF_INT 4      CACHE INTERNAL "")
  SET(SIZEOF_LONG 4     CACHE INTERNAL "")
  SET(SIZEOF_SHORT 2    CACHE INTERNAL "")
  SET(SIZEOF_VOID_P 4   CACHE INTERNAL "")
endif()


set(DCMTK_PACKAGE_VERSION_SUFFIX "")
set(DCMTK_PACKAGE_VERSION_NUMBER ${DCMTK_VERSION_NUMBER})

CONFIGURE_FILE(
  ${DCMTK_SOURCES_DIR}/CMake/osconfig.h.in
  ${DCMTK_SOURCES_DIR}/config/include/dcmtk/config/osconfig.h)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  link_libraries(netapi32)  # For NetWkstaUserGetInfo@12
  link_libraries(iphlpapi)  # For GetAdaptersInfo@8

  # Configure Wine if cross-compiling for Windows
  if (CMAKE_COMPILER_IS_GNUCXX)
    include(${DCMTK_SOURCES_DIR}/CMake/dcmtkUseWine.cmake)
    FIND_PROGRAM(WINE_WINE_PROGRAM wine)
    FIND_PROGRAM(WINE_WINEPATH_PROGRAM winepath)
    list(APPEND DCMTK_TRY_COMPILE_REQUIRED_CMAKE_FLAGS "-DCMAKE_EXE_LINKER_FLAGS=-static")
  endif()
endif()

# This step must be after the generation of "osconfig.h"
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  INSPECT_FUNDAMENTAL_ARITHMETIC_TYPES()
endif()

AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmdata/libsrc DCMTK_SOURCES)
AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/ofstd/libsrc DCMTK_SOURCES)

if (ENABLE_DCMTK_NETWORKING)
  AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmnet/libsrc DCMTK_SOURCES)
  include_directories(
    ${DCMTK_SOURCES_DIR}/dcmnet/include
    )
endif()

if (ENABLE_DCMTK_JPEG)
  AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc DCMTK_SOURCES)
  AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmjpeg/libijg8 DCMTK_SOURCES)
  AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmjpeg/libijg12 DCMTK_SOURCES)
  AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmjpeg/libijg16 DCMTK_SOURCES)
  include_directories(
    ${DCMTK_SOURCES_DIR}/dcmjpeg/include
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libijg8
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libijg12
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libijg16
    ${DCMTK_SOURCES_DIR}/dcmimgle/include
    )
  list(REMOVE_ITEM DCMTK_SOURCES 
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/ddpiimpl.cc

    # Disable support for encoding JPEG (modification in Orthanc 1.0.1)
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djcodece.cc
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djencsv1.cc
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djencbas.cc
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djencpro.cc
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djenclol.cc
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djencode.cc
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djencext.cc
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djencsps.cc
    )
endif()


if (ENABLE_DCMTK_JPEG_LOSSLESS)
  AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmjpls/libsrc DCMTK_SOURCES)
  AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/dcmjpls/libcharls DCMTK_SOURCES)
  include_directories(
    ${DCMTK_SOURCES_DIR}/dcmjpeg/include
    ${DCMTK_SOURCES_DIR}/dcmjpls/include
    ${DCMTK_SOURCES_DIR}/dcmjpls/libcharls
    )
  list(REMOVE_ITEM DCMTK_SOURCES 
    ${DCMTK_SOURCES_DIR}/dcmjpls/libsrc/djcodece.cc

    # Disable support for encoding JPEG-LS (modification in Orthanc 1.0.1)
    ${DCMTK_SOURCES_DIR}/dcmjpls/libsrc/djencode.cc
    )
  list(APPEND DCMTK_SOURCES 
    ${DCMTK_SOURCES_DIR}/dcmjpeg/libsrc/djrplol.cc
    )
endif()


# Source for the logging facility of DCMTK
AUX_SOURCE_DIRECTORY(${DCMTK_SOURCES_DIR}/oflog/libsrc DCMTK_SOURCES)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "kFreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
  list(REMOVE_ITEM DCMTK_SOURCES 
    ${DCMTK_SOURCES_DIR}/oflog/libsrc/clfsap.cc
    ${DCMTK_SOURCES_DIR}/oflog/libsrc/windebap.cc
    ${DCMTK_SOURCES_DIR}/oflog/libsrc/winsock.cc
    )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(REMOVE_ITEM DCMTK_SOURCES 
    ${DCMTK_SOURCES_DIR}/oflog/libsrc/unixsock.cc
    ${DCMTK_SOURCES_DIR}/oflog/libsrc/clfsap.cc
    )
endif()


if (ORTHANC_SANDBOXED)
  configure_file(
    ${ORTHANC_ROOT}/Resources/WebAssembly/dcdict.h
    ${DCMTK_SOURCES_DIR}/dcmdata/include/dcmtk/dcmdata/dcdict.h
    COPYONLY)
  
  configure_file(
    ${ORTHANC_ROOT}/Resources/WebAssembly/dcdict.cc
    ${DCMTK_SOURCES_DIR}/dcmdata/libsrc/dcdict.cc
    COPYONLY)
endif()


list(REMOVE_ITEM DCMTK_SOURCES 
  ${DCMTK_SOURCES_DIR}/dcmdata/libsrc/mkdictbi.cc
  ${DCMTK_SOURCES_DIR}/dcmdata/libsrc/mkdeftag.cc
  )


#set_source_files_properties(${DCMTK_SOURCES}
#  PROPERTIES COMPILE_DEFINITIONS
#  "PACKAGE_VERSION=\"${DCMTK_PACKAGE_VERSION}\";PACKAGE_VERSION_NUMBER=\"${DCMTK_VERSION_NUMBER}\"")

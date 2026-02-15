# Install script for directory: /work/modules/lib/picolibc/newlib/libc/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/work/VehicleOS-RT/build/modules/picolibc/newlib/libc/include/sys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/work/VehicleOS-RT/build/modules/picolibc/newlib/libc/include/machine/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/work/VehicleOS-RT/build/modules/picolibc/newlib/libc/include/ssp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/work/VehicleOS-RT/build/modules/picolibc/newlib/libc/include/rpc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/work/VehicleOS-RT/build/modules/picolibc/newlib/libc/include/arpa/cmake_install.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/work/modules/lib/picolibc/newlib/libc/include/alloca.h"
    "/work/modules/lib/picolibc/newlib/libc/include/_ansi.h"
    "/work/modules/lib/picolibc/newlib/libc/include/argz.h"
    "/work/modules/lib/picolibc/newlib/libc/include/ar.h"
    "/work/modules/lib/picolibc/newlib/libc/include/assert.h"
    "/work/modules/lib/picolibc/newlib/libc/include/byteswap.h"
    "/work/modules/lib/picolibc/newlib/libc/include/cpio.h"
    "/work/modules/lib/picolibc/newlib/libc/include/ctype.h"
    "/work/modules/lib/picolibc/newlib/libc/include/devctl.h"
    "/work/modules/lib/picolibc/newlib/libc/include/dirent.h"
    "/work/modules/lib/picolibc/newlib/libc/include/elf.h"
    "/work/modules/lib/picolibc/newlib/libc/include/endian.h"
    "/work/modules/lib/picolibc/newlib/libc/include/envlock.h"
    "/work/modules/lib/picolibc/newlib/libc/include/envz.h"
    "/work/modules/lib/picolibc/newlib/libc/include/errno.h"
    "/work/modules/lib/picolibc/newlib/libc/include/fastmath.h"
    "/work/modules/lib/picolibc/newlib/libc/include/fcntl.h"
    "/work/modules/lib/picolibc/newlib/libc/include/fenv.h"
    "/work/modules/lib/picolibc/newlib/libc/include/fnmatch.h"
    "/work/modules/lib/picolibc/newlib/libc/include/getopt.h"
    "/work/modules/lib/picolibc/newlib/libc/include/glob.h"
    "/work/modules/lib/picolibc/newlib/libc/include/grp.h"
    "/work/modules/lib/picolibc/newlib/libc/include/iconv.h"
    "/work/modules/lib/picolibc/newlib/libc/include/ieeefp.h"
    "/work/modules/lib/picolibc/newlib/libc/include/inttypes.h"
    "/work/modules/lib/picolibc/newlib/libc/include/langinfo.h"
    "/work/modules/lib/picolibc/newlib/libc/include/libgen.h"
    "/work/modules/lib/picolibc/newlib/libc/include/limits.h"
    "/work/modules/lib/picolibc/newlib/libc/include/locale.h"
    "/work/modules/lib/picolibc/newlib/libc/include/malloc.h"
    "/work/modules/lib/picolibc/newlib/libc/include/math.h"
    "/work/modules/lib/picolibc/newlib/libc/include/memory.h"
    "/work/modules/lib/picolibc/newlib/libc/include/newlib.h"
    "/work/modules/lib/picolibc/newlib/libc/include/paths.h"
    "/work/modules/lib/picolibc/newlib/libc/include/picotls.h"
    "/work/modules/lib/picolibc/newlib/libc/include/pwd.h"
    "/work/modules/lib/picolibc/newlib/libc/include/regdef.h"
    "/work/modules/lib/picolibc/newlib/libc/include/regex.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sched.h"
    "/work/modules/lib/picolibc/newlib/libc/include/search.h"
    "/work/modules/lib/picolibc/newlib/libc/include/setjmp.h"
    "/work/modules/lib/picolibc/newlib/libc/include/signal.h"
    "/work/modules/lib/picolibc/newlib/libc/include/spawn.h"
    "/work/modules/lib/picolibc/newlib/libc/include/stdint.h"
    "/work/modules/lib/picolibc/newlib/libc/include/stdlib.h"
    "/work/modules/lib/picolibc/newlib/libc/include/string.h"
    "/work/modules/lib/picolibc/newlib/libc/include/strings.h"
    "/work/modules/lib/picolibc/newlib/libc/include/_syslist.h"
    "/work/modules/lib/picolibc/newlib/libc/include/tar.h"
    "/work/modules/lib/picolibc/newlib/libc/include/termios.h"
    "/work/modules/lib/picolibc/newlib/libc/include/threads.h"
    "/work/modules/lib/picolibc/newlib/libc/include/time.h"
    "/work/modules/lib/picolibc/newlib/libc/include/unctrl.h"
    "/work/modules/lib/picolibc/newlib/libc/include/unistd.h"
    "/work/modules/lib/picolibc/newlib/libc/include/utime.h"
    "/work/modules/lib/picolibc/newlib/libc/include/utmp.h"
    "/work/modules/lib/picolibc/newlib/libc/include/wchar.h"
    "/work/modules/lib/picolibc/newlib/libc/include/wctype.h"
    "/work/modules/lib/picolibc/newlib/libc/include/wordexp.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/work/modules/lib/picolibc/newlib/libc/include/complex.h")
endif()


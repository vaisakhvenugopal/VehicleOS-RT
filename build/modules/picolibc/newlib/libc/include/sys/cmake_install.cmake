# Install script for directory: /work/modules/lib/picolibc/newlib/libc/include/sys

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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/sys" TYPE FILE FILES
    "/work/modules/lib/picolibc/newlib/libc/include/sys/auxv.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/cdefs.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/config.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/custom_file.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_default_fcntl.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/dirent.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/dir.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/errno.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/fcntl.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/features.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/file.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/iconvnls.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_intsup.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_locale.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/lock.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/param.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/queue.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/resource.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/sched.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/select.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/signal.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_sigset.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/stat.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_stdint.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/string.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/syslimits.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/timeb.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/time.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/times.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_timespec.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/timespec.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_timeval.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/tree.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_types.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/types.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/_tz_structs.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/unistd.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/utime.h"
    "/work/modules/lib/picolibc/newlib/libc/include/sys/wait.h"
    )
endif()


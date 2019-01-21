# Install script for directory: /home/ciprian/testtools/simpletracer/offline.sanitizers

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

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/river.sanitizer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/river.sanitizer")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/river.sanitizer"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/river.sanitizer")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/ciprian/testtools/simpletracer/offline.sanitizers/river.sanitizer")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/river.sanitizer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/river.sanitizer")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/local/bin/river.sanitizer"
         OLD_RPATH "/home/ciprian/testtools/river/z3/bin:/home/ciprian/testtools/river/z3/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/river.sanitizer")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/z3.handler.test" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/z3.handler.test")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/z3.handler.test"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/z3.handler.test")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/ciprian/testtools/simpletracer/offline.sanitizers/z3.handler.test")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/z3.handler.test" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/z3.handler.test")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/local/bin/z3.handler.test"
         OLD_RPATH "/home/ciprian/testtools/river/z3/bin:/home/ciprian/testtools/river/z3/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/z3.handler.test")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/interval.tree.test" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/interval.tree.test")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/interval.tree.test"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/interval.tree.test")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/ciprian/testtools/simpletracer/offline.sanitizers/interval.tree.test")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/interval.tree.test" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/interval.tree.test")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/interval.tree.test")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ciprian/testtools/simpletracer/offline.sanitizers/address.sanitizer/cmake_install.cmake")

endif()


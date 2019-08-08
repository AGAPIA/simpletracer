cmake_minimum_required (VERSION 2.8)
set (CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_VERBOSE_MAKEFILE on)
project (SIMPLE_TRACER)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

# check dependencies
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories($ENV{RIVER_SDK_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/river.format/include)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_compile_definitions(IS_DEBUG_BUILD)
	message("Added debug build definition!")
endif()


if (NOT DEFINED ENV{Z3_ROOT})
	message(FATAL_ERROR "Specify Z3_ROOT variable")
else()
    	set(Z3_ROOT ENV{Z3_ROOT})
endif()



# --------- Establish where z3 library should be -------------
message("z3 root is $ENV{Z3_ROOT}")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "") 
  set (CMAKE_BUILD_TYPE "Release")
endif()

message("build type ${CMAKE_BUILD_TYPE}")

set(Z3_ROOT_INCLUDE "$ENV{Z3_ROOT}/${CMAKE_BUILD_TYPE}/include")
set(Z3_ROOT_LIBS "$ENV{Z3_ROOT}/${CMAKE_BUILD_TYPE}/lib")

include_directories(Z3_ROOT_INCLUDE)
find_library (z3 
              NAMES libz3.so 
              HINTS ${Z3_ROOT_LIBS}
              NO_DEFAULT_PATH)

message("z3 lib path ${z3}")
set(EXTRA_LIBS ${EXTRA_LIBS} ${z3})
#--------------------------------------------------------------



# build targets
add_subdirectory (river.format/logger)

add_subdirectory (river.format/format.handler)



add_subdirectory (libtracer)
add_subdirectory (river.tracer)
#add_subdirectory (offline.sanitizers)

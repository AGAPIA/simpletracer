cmake_minimum_required (VERSION 2.8)
set (CMAKE_SYSTEM_NAME Linux)
project (main)
#add_executable(TestingExecution main.cpp)
set(CROSS_FLAGS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 ${CROSS_FLAGS}")


find_library (Z3_LIB libz3.so)
IF(NOT Z3_LIB)
	MESSAGE(SEND_ERROR "Cannot find library libz3.so")
ENDIF(NOT Z3_LIB)

set(SOURCE_FILES main.cpp Z3Interpretation.cpp FileHandling.cpp TestGraph.cpp ConcolicExecutor.cpp) 


#find_package(z3 REQUIRED)

set(MYANALYSE_INCLUDE_DIRS /home/ceachi/testtools/simpletracer/experiments/testingExecution/z3/include) #headers
set(MYANALYSE_LIBRARY      "/home/ceachi/testtools/simpletracer/experiments/testingExecution/z3/bin/libz3.so")
include_directories("${MYANALYSE_INCLUDE_DIRS}") #include headers
link_directories(${MYANALYSE_LIBRARY})

add_executable(main ${SOURCE_FILES}) #trebuie sa fie inainte de target_link_libraries
#target_link_libraries(main)
target_link_libraries(main "${MYANALYSE_LIBRARY}")

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)




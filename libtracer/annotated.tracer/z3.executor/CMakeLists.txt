## SymbolicDemo CMakeLists.txt

set(EXECUTABLE_NAME symbolic_demo)
set(LIBRARY_NAME z3executor)
set(CROSS_FLAGS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 ${CROSS_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -O0")

include_directories($ENV{RIVER_SDK_DIR}/include)

link_directories (${CMAKE_BINARY_DIR}/lib $ENV{RIVER_SDK_DIR}/lib)
find_library (symbolicenvironment libsymbolicenvironment.so)
find_library (execution libexecution.so)

add_executable(${EXECUTABLE_NAME}
	SymbolicDemoMain.cpp
	TrackingCookie.cpp
	Payload.cpp
	)

add_library(${LIBRARY_NAME}
	LargeStack.cpp
	Z3SymbolicExecutor.cpp
	Z3SymbolicFlags.cpp
	Z3Utils.cpp
	)

target_link_libraries(${EXECUTABLE_NAME}
	${LIBRARY_NAME}
	format.handler
	logger
	tracer
	)

target_link_libraries(${LIBRARY_NAME}
	${z3}
	symbolicenvironment
	execution
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

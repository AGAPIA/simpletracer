# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/cmake-3.15.3-Linux-x86_64/bin/cmake

# The command to remove a file.
RM = /opt/cmake-3.15.3-Linux-x86_64/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ciprian/testtools/simpletracer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ciprian/testtools/simpletracer

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target install/strip
install/strip: preinstall
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Installing the project stripped..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_INSTALL_DO_STRIP=1 -P cmake_install.cmake
.PHONY : install/strip

# Special rule for the target install/strip
install/strip/fast: preinstall/fast
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Installing the project stripped..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_INSTALL_DO_STRIP=1 -P cmake_install.cmake
.PHONY : install/strip/fast

# Special rule for the target install
install: preinstall
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Install the project..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/cmake -P cmake_install.cmake
.PHONY : install

# Special rule for the target install
install/fast: preinstall/fast
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Install the project..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/cmake -P cmake_install.cmake
.PHONY : install/fast

# Special rule for the target install/local
install/local: preinstall
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Installing only the local directory..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_INSTALL_LOCAL_ONLY=1 -P cmake_install.cmake
.PHONY : install/local

# Special rule for the target install/local
install/local/fast: preinstall/fast
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Installing only the local directory..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_INSTALL_LOCAL_ONLY=1 -P cmake_install.cmake
.PHONY : install/local/fast

# Special rule for the target list_install_components
list_install_components:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Available install components are: \"Unspecified\""
.PHONY : list_install_components

# Special rule for the target list_install_components
list_install_components/fast: list_install_components

.PHONY : list_install_components/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/ccmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/opt/cmake-3.15.3-Linux-x86_64/bin/cmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/ciprian/testtools/simpletracer/CMakeFiles /home/ciprian/testtools/simpletracer/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/ciprian/testtools/simpletracer/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named logger

# Build rule for target.
logger: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 logger
.PHONY : logger

# fast build rule for target.
logger/fast:
	$(MAKE) -f river.format/logger/CMakeFiles/logger.dir/build.make river.format/logger/CMakeFiles/logger.dir/build
.PHONY : logger/fast

#=============================================================================
# Target rules for targets named format.handler

# Build rule for target.
format.handler: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 format.handler
.PHONY : format.handler

# fast build rule for target.
format.handler/fast:
	$(MAKE) -f river.format/format.handler/CMakeFiles/format.handler.dir/build.make river.format/format.handler/CMakeFiles/format.handler.dir/build
.PHONY : format.handler/fast

#=============================================================================
# Target rules for targets named tracer

# Build rule for target.
tracer: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 tracer
.PHONY : tracer

# fast build rule for target.
tracer/fast:
	$(MAKE) -f libtracer/CMakeFiles/tracer.dir/build.make libtracer/CMakeFiles/tracer.dir/build
.PHONY : tracer/fast

#=============================================================================
# Target rules for targets named taintedindex

# Build rule for target.
taintedindex: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 taintedindex
.PHONY : taintedindex

# fast build rule for target.
taintedindex/fast:
	$(MAKE) -f libtracer/annotated.tracer/tainted.index/CMakeFiles/taintedindex.dir/build.make libtracer/annotated.tracer/tainted.index/CMakeFiles/taintedindex.dir/build
.PHONY : taintedindex/fast

#=============================================================================
# Target rules for targets named z3executor

# Build rule for target.
z3executor: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 z3executor
.PHONY : z3executor

# fast build rule for target.
z3executor/fast:
	$(MAKE) -f libtracer/annotated.tracer/z3.executor/CMakeFiles/z3executor.dir/build.make libtracer/annotated.tracer/z3.executor/CMakeFiles/z3executor.dir/build
.PHONY : z3executor/fast

#=============================================================================
# Target rules for targets named symbolic_demo

# Build rule for target.
symbolic_demo: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 symbolic_demo
.PHONY : symbolic_demo

# fast build rule for target.
symbolic_demo/fast:
	$(MAKE) -f libtracer/annotated.tracer/z3.executor/CMakeFiles/symbolic_demo.dir/build.make libtracer/annotated.tracer/z3.executor/CMakeFiles/symbolic_demo.dir/build
.PHONY : symbolic_demo/fast

#=============================================================================
# Target rules for targets named river.tracer

# Build rule for target.
river.tracer: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 river.tracer
.PHONY : river.tracer

# fast build rule for target.
river.tracer/fast:
	$(MAKE) -f river.tracer/CMakeFiles/river.tracer.dir/build.make river.tracer/CMakeFiles/river.tracer.dir/build
.PHONY : river.tracer/fast

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... install/strip"
	@echo "... install"
	@echo "... install/local"
	@echo "... list_install_components"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... logger"
	@echo "... format.handler"
	@echo "... tracer"
	@echo "... taintedindex"
	@echo "... z3executor"
	@echo "... symbolic_demo"
	@echo "... river.tracer"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system


# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

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
CMAKE_COMMAND = /usr/bin/cmake3

# The command to remove a file.
RM = /usr/bin/cmake3 -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/cweb

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/cweb/build

# Include any dependencies generated for this target.
include CMakeFiles/cweb.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/cweb.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/cweb.dir/flags.make

CMakeFiles/cweb.dir/src/http_conn.cc.o: CMakeFiles/cweb.dir/flags.make
CMakeFiles/cweb.dir/src/http_conn.cc.o: ../src/http_conn.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/cweb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/cweb.dir/src/http_conn.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/cweb.dir/src/http_conn.cc.o -c /root/cweb/src/http_conn.cc

CMakeFiles/cweb.dir/src/http_conn.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cweb.dir/src/http_conn.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/cweb/src/http_conn.cc > CMakeFiles/cweb.dir/src/http_conn.cc.i

CMakeFiles/cweb.dir/src/http_conn.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cweb.dir/src/http_conn.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/cweb/src/http_conn.cc -o CMakeFiles/cweb.dir/src/http_conn.cc.s

CMakeFiles/cweb.dir/src/mutex.cc.o: CMakeFiles/cweb.dir/flags.make
CMakeFiles/cweb.dir/src/mutex.cc.o: ../src/mutex.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/cweb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/cweb.dir/src/mutex.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/cweb.dir/src/mutex.cc.o -c /root/cweb/src/mutex.cc

CMakeFiles/cweb.dir/src/mutex.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cweb.dir/src/mutex.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/cweb/src/mutex.cc > CMakeFiles/cweb.dir/src/mutex.cc.i

CMakeFiles/cweb.dir/src/mutex.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cweb.dir/src/mutex.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/cweb/src/mutex.cc -o CMakeFiles/cweb.dir/src/mutex.cc.s

# Object files for target cweb
cweb_OBJECTS = \
"CMakeFiles/cweb.dir/src/http_conn.cc.o" \
"CMakeFiles/cweb.dir/src/mutex.cc.o"

# External object files for target cweb
cweb_EXTERNAL_OBJECTS =

../lib/libcweb.so: CMakeFiles/cweb.dir/src/http_conn.cc.o
../lib/libcweb.so: CMakeFiles/cweb.dir/src/mutex.cc.o
../lib/libcweb.so: CMakeFiles/cweb.dir/build.make
../lib/libcweb.so: CMakeFiles/cweb.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/cweb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX shared library ../lib/libcweb.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/cweb.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/cweb.dir/build: ../lib/libcweb.so

.PHONY : CMakeFiles/cweb.dir/build

CMakeFiles/cweb.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/cweb.dir/cmake_clean.cmake
.PHONY : CMakeFiles/cweb.dir/clean

CMakeFiles/cweb.dir/depend:
	cd /root/cweb/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/cweb /root/cweb /root/cweb/build /root/cweb/build /root/cweb/build/CMakeFiles/cweb.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/cweb.dir/depend


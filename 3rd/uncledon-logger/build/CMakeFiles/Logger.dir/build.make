# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.26

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

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.26.4/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.26.4/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/uncled/Downloads/UncleDon-Logger-main

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/uncled/Downloads/UncleDon-Logger-main/build

# Include any dependencies generated for this target.
include CMakeFiles/Logger.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/Logger.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/Logger.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Logger.dir/flags.make

CMakeFiles/Logger.dir/src/Logger.cpp.o: CMakeFiles/Logger.dir/flags.make
CMakeFiles/Logger.dir/src/Logger.cpp.o: /Users/uncled/Downloads/UncleDon-Logger-main/src/Logger.cpp
CMakeFiles/Logger.dir/src/Logger.cpp.o: CMakeFiles/Logger.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/uncled/Downloads/UncleDon-Logger-main/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Logger.dir/src/Logger.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/Logger.dir/src/Logger.cpp.o -MF CMakeFiles/Logger.dir/src/Logger.cpp.o.d -o CMakeFiles/Logger.dir/src/Logger.cpp.o -c /Users/uncled/Downloads/UncleDon-Logger-main/src/Logger.cpp

CMakeFiles/Logger.dir/src/Logger.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Logger.dir/src/Logger.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/uncled/Downloads/UncleDon-Logger-main/src/Logger.cpp > CMakeFiles/Logger.dir/src/Logger.cpp.i

CMakeFiles/Logger.dir/src/Logger.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Logger.dir/src/Logger.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/uncled/Downloads/UncleDon-Logger-main/src/Logger.cpp -o CMakeFiles/Logger.dir/src/Logger.cpp.s

# Object files for target Logger
Logger_OBJECTS = \
"CMakeFiles/Logger.dir/src/Logger.cpp.o"

# External object files for target Logger
Logger_EXTERNAL_OBJECTS =

libLogger.a: CMakeFiles/Logger.dir/src/Logger.cpp.o
libLogger.a: CMakeFiles/Logger.dir/build.make
libLogger.a: CMakeFiles/Logger.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/uncled/Downloads/UncleDon-Logger-main/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libLogger.a"
	$(CMAKE_COMMAND) -P CMakeFiles/Logger.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Logger.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Logger.dir/build: libLogger.a
.PHONY : CMakeFiles/Logger.dir/build

CMakeFiles/Logger.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Logger.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Logger.dir/clean

CMakeFiles/Logger.dir/depend:
	cd /Users/uncled/Downloads/UncleDon-Logger-main/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/uncled/Downloads/UncleDon-Logger-main /Users/uncled/Downloads/UncleDon-Logger-main /Users/uncled/Downloads/UncleDon-Logger-main/build /Users/uncled/Downloads/UncleDon-Logger-main/build /Users/uncled/Downloads/UncleDon-Logger-main/build/CMakeFiles/Logger.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Logger.dir/depend

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
CMAKE_SOURCE_DIR = /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/build

# Include any dependencies generated for this target.
include CMakeFiles/VulkanRender.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/VulkanRender.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/VulkanRender.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/VulkanRender.dir/flags.make

CMakeFiles/VulkanRender.dir/Launcher.cpp.o: CMakeFiles/VulkanRender.dir/flags.make
CMakeFiles/VulkanRender.dir/Launcher.cpp.o: /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/Launcher.cpp
CMakeFiles/VulkanRender.dir/Launcher.cpp.o: CMakeFiles/VulkanRender.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/VulkanRender.dir/Launcher.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/VulkanRender.dir/Launcher.cpp.o -MF CMakeFiles/VulkanRender.dir/Launcher.cpp.o.d -o CMakeFiles/VulkanRender.dir/Launcher.cpp.o -c /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/Launcher.cpp

CMakeFiles/VulkanRender.dir/Launcher.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/VulkanRender.dir/Launcher.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/Launcher.cpp > CMakeFiles/VulkanRender.dir/Launcher.cpp.i

CMakeFiles/VulkanRender.dir/Launcher.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/VulkanRender.dir/Launcher.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/Launcher.cpp -o CMakeFiles/VulkanRender.dir/Launcher.cpp.s

# Object files for target VulkanRender
VulkanRender_OBJECTS = \
"CMakeFiles/VulkanRender.dir/Launcher.cpp.o"

# External object files for target VulkanRender
VulkanRender_EXTERNAL_OBJECTS =

/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: CMakeFiles/VulkanRender.dir/Launcher.cpp.o
/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: CMakeFiles/VulkanRender.dir/build.make
/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: /usr/local/lib/libvulkan.dylib
/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/library/libengine.a
/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/library/libvulkan.a
/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/library/libapplication.a
/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/library/librenderer.a
/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender: CMakeFiles/VulkanRender.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/VulkanRender.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/VulkanRender.dir/build: /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/bin/VulkanRender
.PHONY : CMakeFiles/VulkanRender.dir/build

CMakeFiles/VulkanRender.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/VulkanRender.dir/cmake_clean.cmake
.PHONY : CMakeFiles/VulkanRender.dir/clean

CMakeFiles/VulkanRender.dir/depend:
	cd /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/build /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/build /Users/uncled/Documents/CFiles/VulkanEngine/RenderEngine/build/CMakeFiles/VulkanRender.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/VulkanRender.dir/depend


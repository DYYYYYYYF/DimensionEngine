cmake_minimum_required(VERSION 3.0.0)
project(VulkanRender VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Library)

set(UD_LOGGER_DIR ${PROJECT_SOURCE_DIR}/3rd/uncledon-logger)
set(STB_DIR ${PROJECT_SOURCE_DIR}/3rd/stb)

## Set build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message("-- Use default build type.")
 endif()

if (NOT LOG_LEVEL)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LOG_LEVEL "DEBUG")
    else()
        set(LOG_LEVEL "ERROR")
    endif()
endif()

if(${LOG_LEVEL} STREQUAL "INFO")
    add_definitions(-DLEVEL_INFO -DLEVEL_DEBUG -DLEVEL_WARN -DLEVEL_FATAL -DLEVEL_ERROR)
elseif(${LOG_LEVEL}  STREQUAL "DEBUG")
    add_definitions(-DLEVEL_DEBUG -DLEVEL_WARN -DLEVEL_FATAL -DLEVEL_ERROR)
elseif(${LOG_LEVEL}  STREQUAL "WARN")
    add_definitions(-DLEVEL_WARN -DLEVEL_FATAL -DLEVEL_ERROR)
elseif(${LOG_LEVEL}  STREQUAL "ERROR")
    add_definitions(-DLEVEL_ERROR  -DLEVEL_FATA)
elseif(${LOG_LEVEL}  STREQUAL "FATAL")
    add_definitions(-DLEVEL_FATAL)
endif()

if(${APPLE})
    message("-- Current environment: MacOS")

    find_package(Vulkan QUIET)
    find_package(PkgConfig REQUIRED)

    pkg_search_module(GLFW REQUIRED GLFW3)
    pkg_search_module(GLM REQUIRED GLM)
    
    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${UD_LOGGER_DIR}/lib/MacOS/${CMAKE_BUILD_TYPE})

elseif(${WIN32})
    message("-- Current environment: Windows")

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    set(Vulkan_DIR "D:/C_Library/VulkanSDK")

    if(${Vulkan_DIR} STREQUAL " ")
        message("-- Please set Vulkan direction!!!!")
        return()
    endif()

    set(Vulkan_LIBRARY_DIRS "${Vulkan_DIR}/Lib")
    set(Vulkan_INCLUDE_DIRS "${Vulkan_DIR}/Include")

    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${Vulkan_LIBRARY_DIRS} ${UD_LOGGER_DIR}/lib/Windows/${CMAKE_BUILD_TYPE})
endif()

add_subdirectory(3rd)
add_subdirectory(engine)
add_subdirectory(Editor)

# Unit test
add_subdirectory(Tests)

# Copy necessary dll
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../bin/engine.dll)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../bin/engine.dll DESTINATION ${EXECUTABLE_OUTPUT_PATH})
endif()

message("\n-- Log level: ${LOG_LEVEL}")
message("-- Build target mode: ${CMAKE_BUILD_TYPE}")
message("-- Add executable program to ${EXECUTABLE_OUTPUT_PATH}")
message("-- Generate project successful.\n")

cmake_minimum_required(VERSION 3.21)
project(DuckDuck 
    VERSION 1.0.0
    DESCRIPTION "A simple game engine"
    LANGUAGES CXX
)

# C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成Cmake指令
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Lib)

set(STB_DIR ${PROJECT_SOURCE_DIR}/3rd/stb)

# 多配置生成器
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/Bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/Lib/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/Lib/${CONFIG})
endforeach()

# 构建选项
option(ENABLE_PLUGINS_AUDIO "Enable audio module" OFF)
option(GENERATE_TEST_PROGRAME "Generate test module" ON)

if(MSVC)
    # 强制所有目标使用统一的警告级别（包含第三方库）
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive- /MP /wd4201 /wd4324 /wd4100")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

else()
    add_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# 第三方库
message(STATUS "Configuring Third-Party Libraries...")
add_subdirectory(3rd)

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

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_SRCS})

if(APPLE)
    message("-- Current environment: MacOS")

    set(SYSTEM_LIBRARY_DIR "/usr/local/lib")
    find_package(Vulkan QUIET)

    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories( ${SYSTEM_LIBRARY_DIR} ${LIBRARY_OUTPUT_PATH})

elseif(WIN32)
    message("-- Current environment: Windows")
	
    if(DEFINED ENV{VULKAN_SDK})
        set(Vulkan_DIR $ENV{VULKAN_SDK})
    else()
        message(FATAL_ERROR "VULKAN_SDK environment variable is not set.")
    endif()

    set(Vulkan_LIBRARY_DIRS "${Vulkan_DIR}/Lib")
    set(Vulkan_INCLUDE_DIRS "${Vulkan_DIR}/Include")
	
    message("-- Vulkan Lib: ${Vulkan_LIBRARY_DIRS}")
    message("-- Vulkan Inc: ${Vulkan_INCLUDE_DIRS}")

    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${Vulkan_LIBRARY_DIRS})
endif()

add_subdirectory(engine)
add_subdirectory(Plugins)
add_subdirectory(Editor)

# Unit test
if (GENERATE_TEST_PROGRAME)
    add_subdirectory(Tests)
endif()

# Copy necessary dll
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../bin/engine.dll)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../bin/engine.dll DESTINATION ${EXECUTABLE_OUTPUT_PATH})
endif()

message("\n-- Log level: ${LOG_LEVEL}")
message("-- Build target mode: ${CMAKE_BUILD_TYPE}")
message("-- Add executable program to ${EXECUTABLE_OUTPUT_PATH}")
message("-- Generate project successful.\n")

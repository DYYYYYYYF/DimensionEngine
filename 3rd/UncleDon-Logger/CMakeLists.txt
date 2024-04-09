project(Logger)
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(${CMAKE_HOST_WIN32})
    message("UncleDone::Logger  Current environment: Win32")
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/Windows/${CMAKE_BUILD_TYPE})
endif()

if (${CMAKE_HOST_APPLE})
    message("UncleDone::Logger  Current environment: Apple MacOS")
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/MacOS/${CMAKE_BUILD_TYPE})
endif()

message("UncleDone::Logger  Current build type: ${CMAKE_BUILD_TYPE}\n")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src MY_SRC_DIRS)

add_library(Logger ${MY_SRC_DIRS})
#add_executable(Logger main.cpp ${MY_SRC_DIRS})

target_include_directories(Logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
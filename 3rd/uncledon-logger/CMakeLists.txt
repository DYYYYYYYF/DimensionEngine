project(Logger)
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(${CMAKE_HOST_WIN32})
    message("Current environment: Win32\n")
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/Windows/debug)
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/Windows/release)
endif()

if (${CMAKE_HOST_APPLE})
    message("Current environment: Apple MacOS\n")
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/MacOS)
endif()


include_directories(./include)
aux_source_directory(./src MY_SRC_DIRS)

add_library(Logger ${MY_SRC_DIRS})
# add_executable(Logger main.cpp ${MY_SRC_DIRS})

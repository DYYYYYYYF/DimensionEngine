cmake_minimum_required(VERSION 3.0.0)
project(VulkanRender VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

aux_source_directory(./src DIR_SRCS)
include(CTest)
enable_testing()

find_package(Vulkan QUIET)
find_package(PkgConfig REQUIRED)

pkg_search_module(GLFW REQUIRED GLFW3)
link_directories(${GLFW_LIBRARY_DIRS})

pkg_search_module(GLM REQUIRED GLM)

message(${GLM_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} ./include /Library/Devp/tinyobjloader-master /Library/Devp/stb-master)

add_executable(VulkanRender main.cpp ${DIR_SRCS})

target_link_libraries(VulkanRender Vulkan::Vulkan)
target_link_libraries(VulkanRender ${GLFW_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

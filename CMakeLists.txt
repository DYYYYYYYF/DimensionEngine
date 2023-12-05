cmake_minimum_required(VERSION 3.0.0)
project(VulkanRender VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/library)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/library)

set(TINY_OBJ_LOADER_DIR ${PROJECT_SOURCE_DIR}/3rd/tinyobjloader-master)
set(STB_MASTER_DIR ${PROJECT_SOURCE_DIR}/3rd/stb-master)
set(UD_LOGGER_DIR ${PROJECT_SOURCE_DIR}/3rd/uncledon-logger)
set(GLFW_DIR ${PROJECT_SOURCE_DIR}/3rd/glfw3)

## Set build type
if(${CMAKE_BUILD_TYPE})
    if(${BUILD_TYPE} STREQUAL "debug")
        message("Debug:")
    elseif(${BUILD_TYPE} STREQUAL "release")
         message("Release:")
    endif()
else()
     set(CMAKE_BUILD_TYPE "Release")
     message("-- No specified mode!\n-- User default mode: ${CMAKE_BUILD_TYPE}")
endif()

if(${APPLE})
    message("-- Current environment: MacOS\n")

    find_package(Vulkan QUIET)
    find_package(PkgConfig REQUIRED)

    pkg_search_module(GLFW REQUIRED GLFW3)
    pkg_search_module(GLM REQUIRED GLM)
    
    link_directories(${GLFW_LIBRARY_DIRS} ${GLM_LIBRARY_DIRS} ${UD_LOGGER_DIR}/lib/MacOS)
    include_directories(${STB_MASTER_DIR} ${GLFW_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} 
        ${TINY_OBJ_LOADER_DIR} ${STB_MASTER_DIR} ${GLM_INCLUDE_DIRS} ${UD_LOGGER_DIR}/include ./imgui)

elseif(${WIN32})
    message("-- Current environment: Windows\n")

    set(Vulkan_DIR "D:/C_Library/VulkanSDK")

    if(${Vulkan_DIR} STREQUAL " ")
        message("-- Please set Vulkan direction!!!!")
        return()
    endif()

    set(Vulkan_LIBRARY_DIRS "${Vulkan_DIR}/Lib")
    set(Vulkan_INCLUDE_DIRS "${Vulkan_DIR}/Include")
    set(GLFW_INCLUDE_DIRS "${GLFW_DIR}/include")
    set(GLFW_LIBRARY_DIRS "${GLFW_DIR}/lib/Windows/Release")

    # file(COPY ${OPENAL_DIR}/bin/OpenAL32.dll DESTINATION ${EXECUTABLE_OUTPUT_PATH})

    link_directories(${Vulkan_LIBRARY_DIRS} ${GLFW_LIBRARY_DIRS} ${UD_LOGGER_DIR}/lib/Windows/release)
    include_directories(${GLFW_INCLUDE_DIRS} ${STB_MASTER_DIR} ${Vulkan_INCLUDE_DIRS} ${TINY_OBJ_LOADER_DIR} 
        ${STB_MASTER_DIR} ${GLFW_LIBRARY_DIRS} ${UD_LOGGER_DIR}/include ./imgui)

endif()

message("-- Include GLFW DIR: ${GLFW_INCLUDE_DIRS}")
message("-- Include VULKAN DIR: ${Vulkan_INCLUDE_DIRS}")
message("-- stb_master path: ${STB_MASTER_DIR}")
message("-- TinyObjLoader Path: ${TINY_OBJ_LOADER_DIR}")
message("-- UncleDon-Logger Path: ${UD_LOGGER_DIR}")

add_subdirectory(application)
add_subdirectory(renderer)
add_subdirectory(engine)

add_executable(VulkanRender runtime/Launcher.cpp runtime/logo.rc)

if(${APPLE})
    target_link_libraries(VulkanRender libLogger.a)
    target_link_libraries(VulkanRender Vulkan::Vulkan)
    target_link_libraries(VulkanRender ${GLFW_LIBRARIES})   
elseif(${WIN32})
    target_link_libraries(VulkanRender "vulkan-1.lib")
    target_link_libraries(VulkanRender "Logger.lib")
    target_link_libraries(VulkanRender "glfw3.lib")
endif()

target_link_libraries(VulkanRender engine application renderer)

message("-- Add executable program to ${PROJECT_SOURCE_DIR}/bin")

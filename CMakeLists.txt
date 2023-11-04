cmake_minimum_required(VERSION 3.0.0)
project(VulkanRender VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/library)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/library)

set(EIGEN_DIR ${PROJECT_SOURCE_DIR}/3rd/eigen3)
set(TINY_OBJ_LOADER_DIR ${PROJECT_SOURCE_DIR}/3rd/tinyobjloader-master)
set(STB_MASTER_DIR ${PROJECT_SOURCE_DIR}/3rd/stb-master)
set(UD_LOGGER_DIR ${PROJECT_SOURCE_DIR}/3rd/uncledon-logger)

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
    pkg_search_module(SDL REQUIRED SDL2)
    
    link_directories(${GLFW_LIBRARY_DIRS} ${SDL_LIBRARY_DIRS} ${UD_LOGGER_DIR}/lib/MacOS)
    include_directories(${EIGEN_DIR} ${STB_MASTER_DIR} ${GLFW_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} 
        ${TINY_OBJ_LOADER_DIR} ${STB_MASTER_DIR} ${SDL_INCLUDE_DIRS} ${UD_LOGGER_DIR}/include)

elseif(${WIN32})
    message("-- Current environment: Windows\n")

    set(Vulkan_DIR "D:/Library/Vulkan")

    if(${Vulkan_DIR} STREQUAL " ")
        message("-- Please set Vulkan direction!!!!")
        return()
    endif()

    set(Vulkan_LIBRARY_DIRS "${Vulkan_DIR}/Lib")
    set(Vulkan_INCLUDE_DIRS "${Vulkan_DIR}/Include")
    set(SDL_INCLUDE_DIRS "${Vulkan_DIR}/Include/SDL2")

    # Copy SDL2.dll to output path
    file(COPY ${Vulkan_DIR}/bin/SDL2.dll DESTINATION ${EXECUTABLE_OUTPUT_PATH})

    link_directories(${Vulkan_LIBRARY_DIRS} ${UD_LOGGER_DIR}/lib/Windows/release)
    include_directories(${EIGEN_DIR} ${STB_MASTER_DIR} ${Vulkan_INCLUDE_DIRS} ${TINY_OBJ_LOADER_DIR} 
        ${STB_MASTER_DIR} ${SDL_INCLUDE_DIRS} ${UD_LOGGER_DIR}/include)

endif()

message("-- Include EIGEN3 DIR: ${EIGEN_DIR}")
message("-- Include GLFW DIR: ${GLFW_INCLUDE_DIRS}")
message("-- Include VULKAN DIR: ${Vulkan_INCLUDE_DIRS}")
message("-- stb_master path: ${STB_MASTER_DIR}")
message("-- TinyObjLoader Path: ${TINY_OBJ_LOADER_DIR}")
message("-- UncleDon-Logger Path: ${UD_LOGGER_DIR}")
message("-- Include SDL DIR: ${SDL_INCLUDE_DIRS}")

add_subdirectory(engine)
add_subdirectory(renderer)
add_subdirectory(physics)
add_subdirectory(application)

add_executable(VulkanRender Launcher.cpp)

if(${APPLE})
    target_link_libraries(VulkanRender ${SDL_LIBRARIES} libLogger.a)
    target_link_libraries(VulkanRender Vulkan::Vulkan)
    target_link_libraries(VulkanRender ${GLFW_LIBRARIES})   
elseif(${WIN32})
    target_link_libraries(VulkanRender "vulkan-1.lib" "SDL2.lib" "SDL2main.lib" "Logger.lib")
endif()

target_link_libraries(VulkanRender engine application renderer)

message("-- Add executable program to ${PROJECT_SOURCE_DIR}/bin")

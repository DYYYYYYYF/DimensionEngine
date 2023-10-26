cmake_minimum_required(VERSION 3.0.0)
project(VulkanRender VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/library)

set(TINY_OBJ_LOADER_DIR ${PROJECT_SOURCE_DIR}/3rd/tinyobjloader-master)
set(STB_MASTER_DIR ${PROJECT_SOURCE_DIR}/3rd/stb_master)
set(UD_LOGGER_DIR ${PROJECT_SOURCE_DIR}/3rd/uncledon-logger)

message("-- stb_master path: ${STB_MASTER_DIR}")
message("-- TinyObjLoader Path: ${TINY_OBJ_LOADER_DIR}")
message("-- UncleDon-Logger Path: ${UD_LOGGER_DIR}")

if(${CMAKE_HOST_WIN32})
    message("Current environment: Win32\n")
endif()

if (${CMAKE_HOST_APPLE})
    message("Current environment: Apple MacOS\n")
endif()

if(${APPLE})

    message("-- Current environment: MacOS\n")

    find_package(Vulkan QUIET)
    find_package(PkgConfig REQUIRED)

    pkg_search_module(GLFW REQUIRED GLFW3)
    pkg_search_module(GLM REQUIRED GLM)
    pkg_search_module(SDL REQUIRED SDL2)

    link_directories(${GLFW_LIBRARY_DIRS} ${SDL_LIBRARY_DIRS} ${UD_LOGGER_DIR}/lib/MacOS)
    include_directories(${GLM_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} 
        ${TINY_OBJ_LOADER_DIR} ${STB_MASTER_DIR} ${SDL_INCLUDE_DIRS} ${UD_LOGGER_DIR}/include)

    message("-- Include GLM DIR: ${GLM_INCLUDE_DIRS}")
    message("-- Include SDL DIR: ${SDL_INCLUDE_DIRS}")
    message("-- Include GLFW DIR: ${GLFW_INCLUDE_DIRS}")
    message("-- Include VULKAN DIR: ${Vulkan_INCLUDE_DIRS}\n")

endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/library)
add_subdirectory(engine)
add_subdirectory(vulkan)
add_subdirectory(renderer)
add_subdirectory(physics)
add_subdirectory(application)

add_executable(VulkanRender Launcher.cpp)

target_link_libraries(VulkanRender Vulkan::Vulkan)
target_link_libraries(VulkanRender ${GLFW_LIBRARIES})

if(${APPLE})
    target_link_libraries(VulkanRender ${SDL_LIBRARIES} libLogger.a)
endif()

target_link_libraries(VulkanRender engine vulkan application renderer)

message("-- Add executable program to ${PROJECT_SOURCE_DIR}/bin")

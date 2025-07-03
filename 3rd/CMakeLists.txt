# Logger
add_subdirectory(UncleDon-Logger)

# Audio
if (ENABLE_PLUGINS_AUDIO)
    if (MSVC)
        message("-- MSVC")
        set(CMAKE_C_FLAGS_DEBUG "/W4 /MTd")
    endif()
    # 禁用不必要的选项（可选）
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "Build OpenAL Soft examples" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "Build OpenAL Soft tests" FORCE)
    set(ALSOFT_UTILS OFF CACHE BOOL "Build OpenAL Soft utilities" FORCE)
    add_subdirectory(openal-soft)                   # Audio player

    set(BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
    #set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
    add_subdirectory(sndfile-audio-loader)          # Audio loader
endif()

# yaml-cpp
# 关闭测试
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Don't build yaml-cpp tests")
# 关闭样例（examples）
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Don't build yaml-cpp contrib")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Don't build yaml-cpp tools")
add_subdirectory(yaml-cpp)

# assimp
if(MSVC)
    # 移除所有配置中的默认 /W3 标志
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        
        string(REGEX REPLACE "/W[0-4]" "" ${flag_var} "${${flag_var}}")
    endforeach()
    
    # 设置统一的警告级别
    add_compile_options(/W4)
endif()

# 不编译 assimp 的测试
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable assimp tests")
# 不编译 assimp 的工具（如 assimp viewer、assimp_cmd）
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable assimp tools")
# 不编译 assimp 的样例
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "Disable assimp samples")
add_subdirectory(assimp)

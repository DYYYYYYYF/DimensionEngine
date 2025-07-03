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
option(YAML_CPP_BUILD_TESTS OFF)
option(YAML_CPP_BUILD_TOOLS OFF)
add_subdirectory(yaml-cpp)

# assimp
add_subdirectory(assimp)

# UncleDon-Logger 日志库
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/UncleDon-Logger)
    include(FetchContent)
    message("-- Download UncleDon-Logger Lib...")
    FetchContent_Declare(
        UncleDon-Logger
        GIT_REPOSITORY https://github.com/DYYYYYYYF/UncleDon-Logger.git
        GIT_TAG 1.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    set(UL_BUILD_EXAMPLE_EXE OFF)           # 关闭Example编译
    FetchContent_MakeAvailable(UncleDon-Logger)
else()
    set(UL_BUILD_EXAMPLE_EXE OFF)			# 关闭Example编译
    add_subdirectory(UncleDon-Logger)
endif()

# Audio
if (ENABLE_PLUGINS_AUDIO)
    # 禁用不必要的选项（可选）
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "Build OpenAL Soft examples" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "Build OpenAL Soft tests" FORCE)
    set(ALSOFT_UTILS OFF CACHE BOOL "Build OpenAL Soft utilities" FORCE)
    add_subdirectory(openal-soft)                   # Audio player

    set(BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
    add_subdirectory(sndfile-audio-loader)          # Audio loader
endif()

# Json库
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann-json)
    include(FetchContent)
    message("-- Download nlohmann-json Lib...")
    FetchContent_Declare(
        nlohmann-json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG 3.9.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(nlohmann-json)

    message(STATUS "nlohmann-json源码路径: ${nlohmann-json_SOURCE_DIR}")
    message(STATUS "nlohmann-json构建路径: ${nlohmann-json_BINARY_DIR}")    

else()
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    add_subdirectory(nlohmann-json)
endif()

# assimp库
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "Disable assimp debug postfix")  # 编译debug文件不带d后缀
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable assimp tests")                   # 不编译 assimp 的测试
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable assimp tools")            # 不编译 assimp 的工具（如 assimp viewer、assimp_cmd）
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "Disable assimp samples")               # 不编译 assimp 的样例

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assimp)
    include(FetchContent)
    message("-- Download assimp Lib...")
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    FetchContent_MakeAvailable(assimp)
else()
    add_subdirectory(assimp)
endif()


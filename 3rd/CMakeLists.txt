# CMake 4.x 移除了对 cmake_minimum_required(<3.5) 的兼容，
# 部分三方库（如 nlohmann-json 3.9.0 声明 3.1）会直接报错，
# 这里显式放开策略版本下限，仅影响三方库的 Configure 检查
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# ---------------------------------------------------------------------------
# 首次通过 FetchContent 联网下载的三方库，下载完成后把源码固化到 3rd/<name>/，
# 这样后续（以及清空 build 后重建）会命中上面的 add_subdirectory 分支，不再联网。
# 拷贝时排除 .git 等版本控制目录与测试目录，避免把几百 MB 的历史记录带进工程。
# 注意：排除 test/ 后，如需开启三方库自带的测试（如 ASSIMP_BUILD_TESTS=ON），
#       需先删除 3rd/<name>/ 让其重新完整下载。
# ---------------------------------------------------------------------------
function(cache_third_party_source name)
    string(TOLOWER "${name}" _lc_name)
    set(_src "${${_lc_name}_SOURCE_DIR}")
    set(_dst "${CMAKE_CURRENT_SOURCE_DIR}/${name}")

    if(NOT EXISTS "${_src}/CMakeLists.txt")
        message(WARNING "-- ${name} 源码目录不存在，跳过固化: ${_src}")
        return()
    endif()
    if(EXISTS "${_dst}/CMakeLists.txt")
        return()
    endif()

    message(STATUS "-- Cache ${name} source into ${_dst} ...")
    file(COPY "${_src}/" DESTINATION "${_dst}"
        PATTERN ".git"        EXCLUDE
        PATTERN ".github"     EXCLUDE
        PATTERN ".gitignore"  EXCLUDE
        PATTERN ".gitmodules" EXCLUDE
        PATTERN "test"        EXCLUDE
        PATTERN "build"       EXCLUDE
    )
endfunction()

# UncleDon-Logger 日志库
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/UncleDon-Logger OR NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/UncleDon-Logger/CMakeLists.txt)
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
    cache_third_party_source(UncleDon-Logger)
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
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann-json OR NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann-json/CMakeLists.txt)
    include(FetchContent)
    message("-- Download nlohmann-json Lib...")
    FetchContent_Declare(
        nlohmann-json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.9.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(nlohmann-json)
    cache_third_party_source(nlohmann-json)

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

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assimp OR NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assimp/CMakeLists.txt)
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
    cache_third_party_source(assimp)
else()
    add_subdirectory(assimp)
endif()


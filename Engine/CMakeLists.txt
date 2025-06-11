message("-- Generating Engine Library")

# Load CPython
message("-- Load CPython")
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})
message("-- Python3 include path: ${Python3_INCLUDE_DIRS}")
message("-- Python3 libraries: ${Python3_LIBRARIES}")

file(GLOB_RECURSE ENGINE_SOURCE_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# macOS 平台下添加 Objective-C++ 文件
if(APPLE)
    list(APPEND ENGINE_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Platform/PlatformMacOS.mm
    )
endif()

add_library(engine SHARED ${ENGINE_SOURCE_FILES})

# Output .dll to bin directly.
set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

target_compile_definitions(engine PUBLIC -DDEXPORT) 
message("-- STB Image Path: ${STB_DIR}")

target_include_directories(
    engine 
    PUBLIC 
    UL::Logger
    yaml-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${STB_DIR}
    ${Python3_INCLUDE_DIRS}
)

if(${APPLE})
    target_link_libraries(engine PUBLIC libshaderc_combined.a)
    target_link_libraries(engine PUBLIC Vulkan::Vulkan)

    # MacOS screen
    target_link_libraries(engine PUBLIC "-framework CoreFoundation")
    target_link_libraries(engine PUBLIC "-framework Metal")
    target_link_libraries(engine PUBLIC "-framework QuartzCore")
    target_link_libraries(engine PUBLIC "-framework Cocoa")
    target_link_libraries(engine PUBLIC "-framework AppKit")
    target_link_libraries(engine PUBLIC "-framework CoreData")

    # other frameworks
    # target_link_libraries(program PUBLIC "-framework ")

    set_target_properties(engine PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif(${WIN32})
    # 设置库名称基于配置
    set(GLSLANGL_IBS 
    "$<$<CONFIG:Debug>:shaderc_combinedd.lib>"
    "$<$<CONFIG:Release>:shaderc_combined.lib>"
    "$<$<CONFIG:RelWithDebInfo>:shaderc_combined.lib>"
	)

# 链接库
message("-- Shaderc: ${GLSLANGL_IBS}")
    target_link_libraries(engine PUBLIC ${GLSLANGL_IBS})
    target_link_libraries(engine PUBLIC "vulkan-1.lib")
endif()

target_link_libraries(engine PUBLIC ${Python3_LIBRARIES} UL::Logger yaml-cpp)

message("-- Generated Engine Library\n")

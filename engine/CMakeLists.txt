message("-- Generating Engine Library")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UD_LOGGER_DIR}/include
)

if(${LOG_LEVEL} STREQUAL "INFO")
    add_definitions(-DLEVEL_INFO -DLEVEL_DEBUG -DLEVEL_WARN -DLEVEL_FATAL -DLEVEL_ERROR)
elseif(${LOG_LEVEL}  STREQUAL "DEBUG")
    add_definitions(-DLEVEL_DEBUG -DLEVEL_WARN -DLEVEL_FATAL -DLEVEL_ERROR)
elseif(${LOG_LEVEL}  STREQUAL "WARN")
    add_definitions(-DLEVEL_WARN -DLEVEL_FATAL -DLEVEL_ERROR)
elseif(${LOG_LEVEL}  STREQUAL "ERROR")
    add_definitions(-DLEVEL_ERROR  -DLEVEL_FATA)
elseif(${LOG_LEVEL}  STREQUAL "FATAL")
    add_definitions(-DLEVEL_FATAL)
endif()

file(GLOB ENGINE_SOURCE_FILES *.cpp */*.cpp */*/*.cpp)
add_library(engine SHARED ${ENGINE_SOURCE_FILES})

# Output .dll to bin directly.
set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

target_compile_definitions(engine PUBLIC -DDEXPORT) 
message("-- UncleDon-Logger Path: ${UD_LOGGER_DIR}")

target_include_directories(
    engine 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UD_LOGGER_DIR}/include
)

if(${APPLE})
    target_link_libraries(engine PUBLIC libLogger.a)
    target_link_libraries(engine PUBLIC Vulkan::Vulkan)

elseif(${WIN32})
    target_link_libraries(engine PUBLIC "Logger.lib")
    target_link_libraries(engine PUBLIC "vulkan-1.lib")
endif()

#file(COPY ${OPENAL_DIR}/bin/OpenAL32.dll DESTINATION ${EXECUTABLE_OUTPUT_PATH})

message("-- Generated Engine Library\n")

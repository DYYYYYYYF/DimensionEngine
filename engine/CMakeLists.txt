message("-- Generating Engine Library")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UD_LOGGER_DIR}/include
    ${STB_DIR}
)

file(GLOB ENGINE_SOURCE_FILES *.cpp */*.cpp */*/*.cpp */*/*/*.cpp)
add_library(engine SHARED ${ENGINE_SOURCE_FILES})

# Output .dll to bin directly.
set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

target_compile_definitions(engine PUBLIC -DDEXPORT) 
message("-- UncleDon-Logger Path: ${UD_LOGGER_DIR}")
message("-- STB Image Path: ${STB_DIR}")

target_include_directories(
    engine 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UD_LOGGER_DIR}/include
    ${STB_DIR}
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

message("-- Generating TestEditor")

file(GLOB_RECURSE GAME_SOURCE_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/GameLogic/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GameLogic/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/GameLogic
)
link_directories(${LIBRARY_OUTPUT_PATH})

add_executable(Editor Enter.cpp Game.cpp logo.rc 

    ${GAME_SOURCE_FILES}
)

target_link_libraries(Editor PUBLIC engine)
target_include_directories(Editor PUBLIC engine)

message("-- Generated TestEditor")

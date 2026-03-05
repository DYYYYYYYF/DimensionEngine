message("-- Generating TestEditor")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/GameLogic
    ${CMAKE_CURRENT_SOURCE_DIR}/GameLogic
)
link_directories(${LIBRARY_OUTPUT_PATH})

add_executable(Editor Enter.cpp Game.cpp logo.rc "GameConsole.cpp" "Keybinds.cpp" "GameCommands.cpp" "GameLogic/BaseActors/CubeActor.h" "GameLogic/BaseActors/CubeActor.cpp" "GameLogic/TestActors/RotationCubeActor.h" "GameLogic/TestActors/RotationCubeActor.cpp")

target_link_libraries(Editor PUBLIC engine)
target_include_directories(Editor PUBLIC engine)

message("-- Generated TestEditor")

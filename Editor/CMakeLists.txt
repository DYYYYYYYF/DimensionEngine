message("-- Generating TestEditor")

link_directories(${LIBRARY_OUTPUT_PATH})
add_executable(Editor Enter.cpp Game.cpp logo.rc "GameConsole.cpp" "Keybinds.cpp" "GameCommands.cpp" "GameLogic/BaseActors/RotationCube.h" "GameLogic/BaseActors/RotationCube.cpp")

target_link_libraries(Editor PUBLIC engine)
target_include_directories(Editor PUBLIC engine)

message("-- Generated TestEditor")

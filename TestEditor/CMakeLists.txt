message("-- Generating TestEditor")

link_directories(${LIBRARY_OUTPUT_PATH})
add_executable(TestEditor Enter.cpp Game.cpp logo.rc)

target_link_libraries(TestEditor PUBLIC engine)
target_include_directories(TestEditor PUBLIC engine)

message("-- Generated TestEditor")
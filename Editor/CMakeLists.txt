message("-- Generating TestEditor")

link_directories(${LIBRARY_OUTPUT_PATH})
add_executable(DimensionEditor Enter.cpp Game.cpp logo.rc)

target_link_libraries(DimensionEditor PUBLIC engine)
target_include_directories(DimensionEditor PUBLIC engine)

message("-- Generated TestEditor")
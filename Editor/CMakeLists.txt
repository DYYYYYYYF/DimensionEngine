message("-- Generating TestEditor")

link_directories(${LIBRARY_OUTPUT_PATH})
add_executable(DuckEditor Enter.cpp Game.cpp logo.rc)

target_link_libraries(DuckEditor PUBLIC engine)
target_include_directories(DuckEditor PUBLIC engine)

message("-- Generated TestEditor")

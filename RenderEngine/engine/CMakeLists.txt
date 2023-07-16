message("\n-- Engine:\n")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/library)

file(GLOB ENGINE_SOURCE_FILES *.cpp)
add_library(engine ${ENGINE_SOURCE_FILES})


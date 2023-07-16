message("\n-- Engine:\n")

file(GLOB ENGINE_SOURCE_FILES *.cpp)
add_library(engine ${ENGINE_SOURCE_FILES})


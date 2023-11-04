message("\n-- Engine:\n")

file(GLOB ENGINE_SOURCE_FILES *.cpp foundation/*.cpp)
add_library(engine ${ENGINE_SOURCE_FILES})

message("${RENDERER_RESOURCE_FILES}")


message("\n-- Renderer:\n")

include_directories("../")
file(GLOB RENDERER_RESOURCE_FILES *.cpp)

add_library(renderer ${RENDERER_RESOURCE_FILES})

message("-- Renderer file: ${RENDERER_RESOURCE_FILES}")

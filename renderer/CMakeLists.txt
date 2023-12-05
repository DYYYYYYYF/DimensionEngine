message("\n-- Renderer:")

include_directories("../" "interface" "vulkan")

file(GLOB RENDERER_RESOURCE_FILES *.cpp vulkan/*.cpp )

add_library(renderer ${RENDERER_RESOURCE_FILES})

message("${RENDERER_RESOURCE_FILES}")

message("\n-- Engine:\n")

include_directories("../" "global" "resource" "core" "platform")
file(GLOB ENGINE_SOURCE_FILES *.cpp global/*.cpp resource/*.cpp core/*.cpp platform/*.cpp)
add_library(engine ${ENGINE_SOURCE_FILES})

message("${RENDERER_RESOURCE_FILES}")


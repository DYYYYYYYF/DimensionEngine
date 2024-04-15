message("\n-- Engine:")

include_directories("../" "Containers" "platform" "resource" "core" )
file(GLOB ENGINE_SOURCE_FILES *.cpp Containers/*.cpp resource/*.cpp core/*.cpp platform/*.cpp )
add_library(engine ${ENGINE_SOURCE_FILES})

message("${ENGINE_SOURCE_FILES}\n")


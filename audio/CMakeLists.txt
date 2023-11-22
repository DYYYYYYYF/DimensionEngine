message("\n-- Audio:\n")

include_directories("../")

file(GLOB AUDIO_RESOURCE_FILES *.cpp )

add_library(audio ${AUDIO_RESOURCE_FILES})

message("${AUDIO_RESOURCE_FILES}")

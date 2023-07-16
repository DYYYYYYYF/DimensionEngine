message("\n-- Vulkan:\n")

file(GLOB VULkAN_RESOURCE_FILES *.cpp)
add_library(vulkan ${VULkAN_RESOURCE_FILES})

message("\n-- Application:\n")

include_directories("../")
file(GLOB APP_RESOURCE_FILES *.cpp)
add_library(application ${APP_RESOURCE_FILES})
message("-- Application file: ${APP_RESOURCE_FILES}")


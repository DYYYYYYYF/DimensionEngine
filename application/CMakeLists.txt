message("\n-- Application:")

include_directories("../")
file(GLOB APP_RESOURCE_FILES *.cpp)
add_library(application ${APP_RESOURCE_FILES})

message(" ${APP_RESOURCE_FILES}")


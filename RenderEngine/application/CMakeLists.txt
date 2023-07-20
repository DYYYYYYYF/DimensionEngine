message("\n-- Application:\n")

file(GLOB APP_RESOURCE_FILES *.cpp)
add_library(application ${APP_RESOURCE_FILES} Application.cpp Window.cpp)
message("-- Application file: ${APP_RESOURCE_FILES}")


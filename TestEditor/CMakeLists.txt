project(TestEditor)

set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

message("-- Generating TestEditor")

add_executable(TestEditor Enter.cpp)

if(${APPLE})
	target_link_libraries(TestEditor libLogger.a)
	target_link_libraries(TestEditor Vulkan::Vulkan)
	target_link_libraries(TestEditor ${GLFW_LIBRARIES})   
elseif(${WIN32})
	target_link_libraries(TestEditor "vulkan-1.lib")
	target_link_libraries(TestEditor "Logger.lib")
	target_link_libraries(TestEditor "glfw3.lib")
endif()
target_link_libraries(TestEditor engine application renderer)
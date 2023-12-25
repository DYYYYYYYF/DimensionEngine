project(examples)

set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file(GLOB_RECURSE example_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/default/default.cpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/texture/texture.cpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/window_size/window_size.cpp"
)

foreach(example_file IN LISTS example_files)

	message("-- Generating ${example_file}")

	get_filename_component(mainname ${example_file} NAME_WE)
	add_executable(${mainname} ${example_file})

	if(${APPLE})
		target_link_libraries(${mainname} libLogger.a)
		target_link_libraries(${mainname} Vulkan::Vulkan)
		target_link_libraries(${mainname} ${GLFW_LIBRARIES})   
	elseif(${WIN32})
		target_link_libraries(${mainname} "vulkan-1.lib")
		target_link_libraries(${mainname} "Logger.lib")
		target_link_libraries(${mainname} "glfw3.lib")
	endif()
	target_link_libraries(${mainname} engine application renderer)

	message("-- Generate ${mainname} to ${CMAKE_CURRENT_SOURCE_DIR}/bin successful")

endforeach()


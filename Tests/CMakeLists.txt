message("-- Generating Unit test project")

project(UnitTest)

set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

link_directories(${LIBRARY_OUTPUT_PATH})

file(GLOB_RECURSE example_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/UnitTest.cpp" 
)

foreach(example_file IN LISTS example_files)

	message("-- Generating ${example_file}")

	get_filename_component(mainname ${example_file} NAME_WE)
	add_executable(${mainname} ${example_file})
	
	target_link_libraries(${mainname} PUBLIC engine)
	target_include_directories(${mainname} PUBLIC engine)

	message("-- Generate ${mainname} to ${CMAKE_CURRENT_SOURCE_DIR}/bin successful")

endforeach()

message("-- Generated Unit test project")
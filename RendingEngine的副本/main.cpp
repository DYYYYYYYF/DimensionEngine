#include "Application.hpp"

int main() {
	
	TriangleApplication app;
	try{
		app.run();
	} catch(const std::exception& e){
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}
#include "MathLibraryTests.hpp"
using namespace std;

int TestMathLibrary() {
	try {
		// 运行所有测试
		MathLibraryTests::RunAllTests();

		return 0;
	}
	catch (const std::exception& e) {
		std::cerr << "Test execution failed with exception: " << e.what() << std::endl;
		return -1;
	}
	catch (...) {
		std::cerr << "Test execution failed with unknown exception" << std::endl;
		return -1;
	}
}
#include "MemoryAnalyer.h"
#include "MemoryAllocatorTester.h"

int TestFreelist() {
	AllocatorTester tester;

	std::cout << "=== 动态内存分配器测试 ===" << std::endl;

	// 运行正确性测试
	bool correctness_passed = tester.RunCorrectnessTests();
	if (!correctness_passed) {
		std::cout << "\n[FAIL] 正确性测试未通过" << std::endl;
		return 0;
	}

	// 运行性能测试
	tester.RunPerformanceTests();

	// 运行内存泄漏检测
	tester.TestMemoryLeaks();

	std::cout << "\n=== 测试完成 ===" << std::endl;


	std::cout << "=== 动态内存分析 ===" << std::endl;
	RunAnalysis();
	std::cout << "\n=== 分析完成 ===" << std::endl;

	return 1;
}

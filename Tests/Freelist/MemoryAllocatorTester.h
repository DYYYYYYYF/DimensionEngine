#include <Memory/DynamicAllocator.h>
#include <chrono>
#include <vector>
#include <random>
#include <iostream>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <algorithm>

class AllocatorTester {
private:
	DynamicAllocator allocator;
	std::vector<void*> allocated_blocks;
	std::vector<size_t> block_sizes;

public:
	// ================================
	// 正确性测试
	// ================================

	bool RunCorrectnessTests() {
		std::cout << "=== 开始正确性测试 ===" << std::endl;

		bool all_passed = true;
		all_passed &= TestBasicAllocation();
		all_passed &= TestAlignment();
		all_passed &= TestBoundaryConditions();
		all_passed &= TestMemoryIntegrity();
		all_passed &= TestFragmentation();
		all_passed &= TestErrorHandling();

		std::cout << (all_passed ? "[PASS] 所有正确性测试通过！" : "[FAIL] 部分测试失败！") << std::endl;
		return all_passed;
	}

	bool TestBasicAllocation() {
		std::cout << "[TEST] 测试基础分配功能..." << std::endl;

		// 初始化分配器
		if (!allocator.Create(1024 * 1024)) { // 1MB
			std::cout << "[FAIL] 分配器初始化失败" << std::endl;
			return false;
		}

		// 测试基础分配
		void* ptr1 = allocator.Allocate(256);
		void* ptr2 = allocator.Allocate(512);
		void* ptr3 = allocator.Allocate(128);

		if (!ptr1 || !ptr2 || !ptr3) {
			std::cout << "[FAIL] 基础分配失败" << std::endl;
			return false;
		}

		// 写入测试数据
		memset(ptr1, 0xAA, 256);
		memset(ptr2, 0xBB, 512);
		memset(ptr3, 0xCC, 128);

		// 验证数据完整性
		bool data_valid = true;
		for (int i = 0; i < 256; i++) {
			if (((uint8_t*)ptr1)[i] != 0xAA) data_valid = false;
		}
		for (int i = 0; i < 512; i++) {
			if (((uint8_t*)ptr2)[i] != 0xBB) data_valid = false;
		}
		for (int i = 0; i < 128; i++) {
			if (((uint8_t*)ptr3)[i] != 0xCC) data_valid = false;
		}

		if (!data_valid) {
			std::cout << "[FAIL] 数据完整性验证失败" << std::endl;
			return false;
		}

		// 测试释放
		if (!allocator.Free(ptr1, 256) || !allocator.Free(ptr2, 512) || !allocator.Free(ptr3, 128)) {
			std::cout << "[FAIL] 内存释放失败" << std::endl;
			return false;
		}

		allocator.Destroy();
		std::cout << "[PASS] 基础分配测试通过" << std::endl;
		return true;
	}

	bool TestAlignment() {
		std::cout << "[TEST] 测试内存对齐..." << std::endl;

		if (!allocator.Create(1024 * 1024)) {
			return false;
		}

		// 测试不同对齐要求
		std::vector<unsigned short> alignments = { 1, 2, 4, 8, 16, 32, 64, 128 };

		for (auto alignment : alignments) {
			void* ptr = allocator.AllocateAligned(100, alignment);
			if (!ptr) {
				std::cout << "[FAIL] 对齐分配失败: " << alignment << std::endl;
				return false;
			}

			if ((uintptr_t)ptr % alignment != 0) {
				std::cout << "[FAIL] 内存对齐失败: " << alignment << ", addr: " << ptr << std::endl;
				return false;
			}

			// 验证对齐信息
			size_t size;
			size_t stored_alignment;
			if (!allocator.GetAlignmentSize(ptr, &size, &stored_alignment)) {
				std::cout << "[FAIL] 获取对齐信息失败" << std::endl;
				return false;
			}

			if (stored_alignment != alignment || size != 100) {
				std::cout << "[FAIL] 对齐信息不匹配" << std::endl;
				return false;
			}

			allocator.FreeAligned(ptr);
		}

		allocator.Destroy();
		std::cout << "[PASS] 内存对齐测试通过" << std::endl;
		return true;
	}

	bool TestBoundaryConditions() {
		std::cout << "[TEST] 测试边界条件..." << std::endl;

		if (!allocator.Create(1024)) { // 小内存池
			return false;
		}

		// 测试分配整个内存池
		size_t total_space = allocator.GetFreeSpace();
		size_t header_size = allocator.AllocatorHeaderSize();

		// 尝试分配接近最大可用空间的内存
		size_t max_alloc_size = total_space - header_size - 64; // 留些余量
		void* large_ptr = allocator.Allocate(max_alloc_size);

		if (!large_ptr) {
			std::cout << "[FAIL] 大块内存分配失败" << std::endl;
			return false;
		}

		// 此时应该没有足够空间再分配
		void* should_fail = allocator.Allocate(100);
		if (should_fail != nullptr) {
			std::cout << "[FAIL] 应该失败的分配竟然成功了" << std::endl;
			return false;
		}

		// 释放大块内存后应该能再次分配
		allocator.Free(large_ptr, max_alloc_size);
		void* ptr = allocator.Allocate(100);
		if (!ptr) {
			std::cout << "[FAIL] 释放后重新分配失败" << std::endl;
			return false;
		}

		allocator.Free(ptr, 100);
		allocator.Destroy();
		std::cout << "[PASS] 边界条件测试通过" << std::endl;
		return true;
	}

	bool TestMemoryIntegrity() {
		std::cout << "[TEST] 测试内存完整性..." << std::endl;

		if (!allocator.Create(64 * 1024)) {
			return false;
		}

		// 分配多个不同大小的块
		struct Block {
			void* ptr;
			size_t size;
			uint8_t pattern;
		};

		std::vector<Block> blocks;
		uint8_t pattern = 0x10;

		for (size_t size = 16; size <= 1024; size *= 2) {
			void* ptr = allocator.Allocate(size);
			if (!ptr) continue;

			memset(ptr, pattern, size);
			blocks.push_back({ ptr, size, pattern });
			pattern += 0x10;
		}

		// 验证所有块的数据完整性
		for (const auto& block : blocks) {
			for (size_t i = 0; i < block.size; i++) {
				if (((uint8_t*)block.ptr)[i] != block.pattern) {
					std::cout << "[FAIL] 内存数据被破坏" << std::endl;
					return false;
				}
			}
		}

		// 释放一半的块
		for (size_t i = 0; i < blocks.size(); i += 2) {
			allocator.Free(blocks[i].ptr, blocks[i].size);
		}

		// 验证剩余块的数据完整性
		for (size_t i = 1; i < blocks.size(); i += 2) {
			for (size_t j = 0; j < blocks[i].size; j++) {
				if (((uint8_t*)blocks[i].ptr)[j] != blocks[i].pattern) {
					std::cout << "[FAIL] 释放后数据被破坏" << std::endl;
					return false;
				}
			}
		}

		// 清理剩余块
		for (size_t i = 1; i < blocks.size(); i += 2) {
			allocator.Free(blocks[i].ptr, blocks[i].size);
		}

		allocator.Destroy();
		std::cout << "[PASS] 内存完整性测试通过" << std::endl;
		return true;
	}

	bool TestFragmentation() {
		std::cout << "[TEST] 测试碎片化处理..." << std::endl;

		if (!allocator.Create(64 * 1024)) {
			return false;
		}

		std::vector<void*> ptrs;

		// 分配许多小块
		for (int i = 0; i < 100; i++) {
			void* ptr = allocator.Allocate(256);
			if (ptr) ptrs.push_back(ptr);
		}

		// 释放每隔一个块，创造碎片
		for (size_t i = 0; i < ptrs.size(); i += 2) {
			allocator.Free(ptrs[i], 256);
		}

		// 尝试分配一个大块（应该能合并碎片）
		void* large_ptr = allocator.Allocate(512);
		if (!large_ptr) {
			std::cout << "[FAIL] 碎片化后大块分配失败" << std::endl;
			return false;
		}

		// 清理
		allocator.Free(large_ptr, 512);
		for (size_t i = 1; i < ptrs.size(); i += 2) {
			allocator.Free(ptrs[i], 256);
		}

		allocator.Destroy();
		std::cout << "[PASS] 碎片化测试通过" << std::endl;
		return true;
	}

	bool TestErrorHandling() {
		std::cout << "[TEST] 测试错误处理..." << std::endl;

		// 测试无效参数
		if (allocator.Create(0)) {
			std::cout << "[FAIL] 零大小创建应该失败" << std::endl;
			return false;
		}

		if (!allocator.Create(1024)) {
			return false;
		}

		// 测试释放空指针
		if (allocator.Free(nullptr, 100)) {
			std::cout << "[FAIL] 释放空指针应该失败" << std::endl;
			return false;
		}

		// 测试释放无效指针
		int dummy_var;
		if (allocator.Free(&dummy_var, 100)) {
			std::cout << "[FAIL] 释放无效指针应该失败" << std::endl;
			return false;
		}

		allocator.Destroy();
		std::cout << "[PASS] 错误处理测试通过" << std::endl;
		return true;
	}

	// ================================
	// 性能测试
	// ================================

	void RunPerformanceTests() {
		std::cout << "\n=== 开始性能测试 ===" << std::endl;

		TestAllocationSpeed();
		TestDifferentSizePatterns();
		TestFragmentationPerformance();
		CompareWithMalloc();
	}

	void TestAllocationSpeed() {
		std::cout << "[PERF] 测试分配速度..." << std::endl;

		if (!allocator.Create(64 * 1024 * 1024)) { // 64MB
			return;
		}

		const int num_allocations = 10000;
		const size_t alloc_size = 256;

		auto start = std::chrono::high_resolution_clock::now();

		std::vector<void*> ptrs;
		ptrs.reserve(num_allocations);

		// 分配测试
		for (int i = 0; i < num_allocations; i++) {
			void* ptr = allocator.Allocate(alloc_size);
			if (ptr) ptrs.push_back(ptr);
		}

		auto mid = std::chrono::high_resolution_clock::now();

		// 释放测试
		for (void* ptr : ptrs) {
			allocator.Free(ptr, alloc_size);
		}

		auto end = std::chrono::high_resolution_clock::now();

		auto alloc_time = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
		auto free_time = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);

		std::cout << "[STATS] 分配 " << num_allocations << " 个 " << alloc_size << " 字节块:" << std::endl;
		std::cout << "   分配时间: " << alloc_time.count() << " μs" << std::endl;
		std::cout << "   释放时间: " << free_time.count() << " μs" << std::endl;
		std::cout << "   平均分配时间: " << (double)alloc_time.count() / num_allocations << " μs/次" << std::endl;
		std::cout << "   平均释放时间: " << (double)free_time.count() / num_allocations << " μs/次" << std::endl;

		allocator.Destroy();
	}

	void TestDifferentSizePatterns() {
		std::cout << "[PERF] 测试不同大小模式..." << std::endl;

		// 修复: 为不同大小使用不同的分配次数，避免内存不足
		std::vector<std::pair<size_t, int>> size_ops_pairs = {
			{16, 5000},      // 16字节: 5000次 ≈ 0.5MB
			{64, 5000},      // 64字节: 5000次 ≈ 2MB  
			{256, 5000},     // 256字节: 5000次 ≈ 8MB
			{1024, 3000},    // 1KB: 3000次 ≈ 12MB
			{4096, 1500},    // 4KB: 1500次 ≈ 24MB
			{16384, 600}     // 16KB: 600次 ≈ 40MB
		};

		for (auto [size, num_ops] : size_ops_pairs) {
			// 使用更大的分配器以确保有足够内存
			size_t allocator_size = 128 * 1024 * 1024; // 128MB

			DynamicAllocator TempAllocator;
			if (!TempAllocator.Create(allocator_size)) {
				std::cout << "   [FAIL] 创建分配器失败" << std::endl;
				continue;
			}

			auto start = std::chrono::high_resolution_clock::now();

			std::vector<void*> ptrs;
			ptrs.reserve(num_ops); // 预分配容器空间

			// 分配阶段
			int successful_allocations = 0;
			for (int i = 0; i < num_ops; i++) {
				void* ptr = TempAllocator.Allocate(size);
				if (ptr) {
					ptrs.push_back(ptr);
					successful_allocations++;
				}
				else {
					std::cout << "   [WARN] 分配失败在第 " << i << " 次，大小=" << size << std::endl;
					break;
				}
			}

			// 释放阶段
			for (void* ptr : ptrs) {
				TempAllocator.Free(ptr, size);
			}

			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

			std::cout << "   大小 " << std::setw(6) << size << " 字节: "
				<< std::setw(6) << duration.count() << " μs, "
				<< std::setw(8) << std::fixed << std::setprecision(2)
				<< (double)duration.count() / successful_allocations << " μs/次 "
				<< "(" << successful_allocations << "/" << num_ops << ")" << std::endl;

			TempAllocator.Destroy();
		}
	}

	void TestFragmentationPerformance() {
		std::cout << "[PERF] 测试碎片化性能影响..." << std::endl;

		if (!allocator.Create(32 * 1024 * 1024)) return;

		// 创建碎片化场景
		std::vector<void*> ptrs;
		for (int i = 0; i < 1000; i++) {
			void* ptr = allocator.Allocate(1024);
			if (ptr) ptrs.push_back(ptr);
		}

		// 释放一半创造碎片
		for (size_t i = 0; i < ptrs.size(); i += 2) {
			allocator.Free(ptrs[i], 1024);
		}

		// 测试在碎片化环境下的分配性能
		auto start = std::chrono::high_resolution_clock::now();

		std::vector<void*> new_ptrs;
		for (int i = 0; i < 500; i++) {
			void* ptr = allocator.Allocate(512);
			if (ptr) new_ptrs.push_back(ptr);
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		std::cout << "[STATS] 碎片化环境下分配 500 个 512 字节块: " << duration.count() << " μs" << std::endl;
		std::cout << "   平均时间: " << (double)duration.count() / 500 << " μs/次" << std::endl;

		// 清理
		for (void* ptr : new_ptrs) {
			allocator.Free(ptr, 512);
		}
		for (size_t i = 1; i < ptrs.size(); i += 2) {
			allocator.Free(ptrs[i], 1024);
		}

		allocator.Destroy();
	}

	void CompareWithMalloc() {
		std::cout << "[PERF] 与系统malloc对比..." << std::endl;

		const int num_allocations = 5000;
		const size_t alloc_size = 256;

		// 测试自定义分配器
		if (!allocator.Create(64 * 1024 * 1024)) return;

		auto start = std::chrono::high_resolution_clock::now();
		std::vector<void*> custom_ptrs;
		for (int i = 0; i < num_allocations; i++) {
			void* ptr = allocator.Allocate(alloc_size);
			if (ptr) custom_ptrs.push_back(ptr);
		}
		for (void* ptr : custom_ptrs) {
			allocator.Free(ptr, alloc_size);
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto custom_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		allocator.Destroy();

		// 测试系统malloc
		start = std::chrono::high_resolution_clock::now();
		std::vector<void*> malloc_ptrs;
		for (int i = 0; i < num_allocations; i++) {
			void* ptr = malloc(alloc_size);
			if (ptr) malloc_ptrs.push_back(ptr);
		}
		for (void* ptr : malloc_ptrs) {
			free(ptr);
		}
		end = std::chrono::high_resolution_clock::now();
		auto malloc_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		std::cout << "[STATS] 性能对比 (" << num_allocations << " 次 " << alloc_size << " 字节分配):" << std::endl;
		std::cout << "   自定义分配器: " << custom_time.count() << " μs" << std::endl;
		std::cout << "   系统malloc:   " << malloc_time.count() << " μs" << std::endl;
		std::cout << "   性能比率: " << std::fixed << std::setprecision(2)
			<< (double)custom_time.count() / malloc_time.count() << "x" << std::endl;
	}

	// ================================
	// 内存泄漏检测
	// ================================

	void TestMemoryLeaks() {
		std::cout << "\n[TEST] 内存泄漏检测..." << std::endl;

		if (!allocator.Create(1024 * 1024)) return;

		size_t initial_free = allocator.GetFreeSpace();

		// 执行一系列分配和释放操作
		for (int cycle = 0; cycle < 10; cycle++) {
			std::vector<void*> ptrs;

			// 分配
			for (int i = 0; i < 100; i++) {
				void* ptr = allocator.Allocate(128 + (i % 64));
				if (ptr) ptrs.push_back(ptr);
			}

			// 释放
			for (size_t i = 0; i < ptrs.size(); i++) {
				allocator.Free(ptrs[i], 128 + (i % 64));
			}
		}

		size_t final_free = allocator.GetFreeSpace();

		if (initial_free == final_free) {
			std::cout << "[PASS] 无内存泄漏检测到" << std::endl;
		}
		else {
			std::cout << "[FAIL] 可能存在内存泄漏: " << (initial_free - final_free) << " 字节" << std::endl;
		}

		allocator.Destroy();
	}
};

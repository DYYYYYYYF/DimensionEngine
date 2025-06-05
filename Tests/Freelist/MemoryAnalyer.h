#include <Memory/DynamicAllocator.h>
#include <chrono>        
#include <vector>        
#include <random>        
#include <iostream>      
#include <cassert>       
#include <cstring>       
#include <iomanip>       
#include <algorithm>     
#include <sstream>       
#include <fstream>       
#include <thread>        
#include <mutex>         
#include <atomic>        

class MemoryAnalyzer {
private:
	DynamicAllocator* allocator;
	std::mutex analysis_mutex;

	struct AllocationInfo {
		void* ptr;
		size_t size;
		size_t timestamp;
		bool is_freed;
	};

	std::vector<AllocationInfo> allocation_history;
	std::atomic<size_t> operation_counter{ 0 };

public:
	explicit MemoryAnalyzer(DynamicAllocator* alloc) : allocator(alloc) {}

	// ================================
	// 内存使用情况分析
	// ================================

	void AnalyzeMemoryUsage() {
		std::cout << "=== 内存使用分析 ===" << std::endl;

		size_t total_space = allocator->GetTotalSpace();
		size_t free_space = allocator->GetFreeSpace();
		size_t used_space = total_space - free_space;

		double usage_percentage = (double)used_space / total_space * 100.0;

		std::cout << "[STATS] 内存统计:" << std::endl;
		std::cout << "   总空间:     " << FormatBytes(total_space) << std::endl;
		std::cout << "   已使用:     " << FormatBytes(used_space) << " ("
			<< std::fixed << std::setprecision(1) << usage_percentage << "%)" << std::endl;
		std::cout << "   可用空间:   " << FormatBytes(free_space) << std::endl;

		// 分析碎片化程度
		AnalyzeFragmentation();
	}

	void AnalyzeFragmentation() {
		// 这里需要访问Freelist的内部状态，可能需要在Freelist中添加分析接口
		std::cout << "\n[ANALYSIS] 碎片化分析:" << std::endl;
		std::cout << "   (需要在Freelist中添加GetBlockCount()等分析接口)" << std::endl;

		// 示例分析 - 尝试分配不同大小来检测碎片化
		std::vector<size_t> test_sizes = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

		for (size_t size : test_sizes) {
			void* ptr = allocator->Allocate(size);
			if (ptr) {
				std::cout << "   [PASS] 可分配 " << FormatBytes(size) << std::endl;
				allocator->Free(ptr, size);
			}
			else {
				std::cout << "   [FAIL] 无法分配 " << FormatBytes(size) << std::endl;
			}
		}
	}

	// ================================
	// 压力测试
	// ================================

	void RunStressTest(int duration_seconds = 30) {
		std::cout << "\n=== 压力测试 (" << duration_seconds << " 秒) ===" << std::endl;

		std::atomic<bool> should_stop{ false };
		std::atomic<size_t> allocations{ 0 };
		std::atomic<size_t> deallocations{ 0 };
		std::atomic<size_t> failed_allocations{ 0 };

		// 启动多个工作线程
		const int num_threads = std::thread::hardware_concurrency();
		std::vector<std::thread> workers;

		auto worker_func = [&](int thread_id) {
			std::random_device rd;
			std::mt19937 gen(rd() + thread_id);
			std::uniform_int_distribution<> size_dist(16, 4096);
			std::uniform_int_distribution<> op_dist(1, 100);

			std::vector<std::pair<void*, size_t>> local_ptrs;

			while (!should_stop) {
				if (op_dist(gen) <= 60 || local_ptrs.empty()) {
					// 60% 概率分配内存
					size_t size = size_dist(gen);
					void* ptr = allocator->Allocate(size);

					if (ptr) {
						local_ptrs.push_back({ ptr, size });
						allocations.fetch_add(1);

						// 写入一些数据验证完整性
						uint8_t pattern = thread_id & 0xFF;
						memset(ptr, pattern, size);
					}
					else {
						failed_allocations.fetch_add(1);
					}
				}
				else {
					// 40% 概率释放内存
					if (!local_ptrs.empty()) {
						std::uniform_int_distribution<size_t> ptr_dist(0, local_ptrs.size() - 1);
						size_t idx = ptr_dist(gen);

						auto [ptr, size] = local_ptrs[idx];

						// 验证数据完整性
						uint8_t expected_pattern = thread_id & 0xFF;
						bool data_valid = true;
						for (size_t i = 0; i < size && i < 100; i++) {
							if (((uint8_t*)ptr)[i] != expected_pattern) {
								data_valid = false;
								break;
							}
						}

						if (!data_valid) {
							std::cout << "[ERROR] 线程 " << thread_id << " 数据损坏!" << std::endl;
						}

						allocator->Free(ptr, size);
						local_ptrs.erase(local_ptrs.begin() + idx);
						deallocations.fetch_add(1);
					}
				}

				// 短暂休息避免过度竞争
				std::this_thread::sleep_for(std::chrono::microseconds(10));
			}

			// 清理剩余内存
			for (auto [ptr, size] : local_ptrs) {
				allocator->Free(ptr, size);
				deallocations.fetch_add(1);
			}
			};

		// 启动工作线程
		for (int i = 0; i < num_threads; i++) {
			workers.emplace_back(worker_func, i);
		}

		// 等待指定时间
		std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
		should_stop.store(true);

		// 等待所有线程完成
		for (auto& worker : workers) {
			worker.join();
		}

		std::cout << "[STATS] 压力测试结果:" << std::endl;
		std::cout << "   线程数:       " << num_threads << std::endl;
		std::cout << "   成功分配:     " << allocations.load() << std::endl;
		std::cout << "   成功释放:     " << deallocations.load() << std::endl;
		std::cout << "   分配失败:     " << failed_allocations.load() << std::endl;
		std::cout << "   总操作数:     " << (allocations.load() + deallocations.load()) << std::endl;
		std::cout << "   操作速率:     " << (allocations.load() + deallocations.load()) / duration_seconds << " ops/秒" << std::endl;

		AnalyzeMemoryUsage();
	}

	// ================================
	// 内存使用模式测试
	// ================================

	void TestMemoryPatterns() {
		std::cout << "\n=== 内存使用模式测试 ===" << std::endl;

		TestSequentialPattern();
		TestRandomPattern();
		TestBurstPattern();
		TestMixedSizePattern();
	}

	void TestSequentialPattern() {
		std::cout << "\n[TEST] 顺序分配模式测试..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();

		std::vector<void*> ptrs;
		const int count = 1000;
		const size_t size = 256;

		// 顺序分配
		for (int i = 0; i < count; i++) {
			void* ptr = allocator->Allocate(size);
			if (ptr) ptrs.push_back(ptr);
		}

		// 顺序释放
		for (void* ptr : ptrs) {
			allocator->Free(ptr, size);
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		std::cout << "   顺序模式 " << count << " 次操作: " << duration.count() << " μs" << std::endl;
	}

	void TestRandomPattern() {
		std::cout << "[TEST] 随机分配模式测试..." << std::endl;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> size_dist(64, 1024);

		auto start = std::chrono::high_resolution_clock::now();

		std::vector<std::pair<void*, size_t>> ptrs;
		const int count = 1000;

		// 随机大小分配
		for (int i = 0; i < count; i++) {
			size_t size = size_dist(gen);
			void* ptr = allocator->Allocate(size);
			if (ptr) ptrs.push_back({ ptr, size });
		}

		// 随机顺序释放
		std::shuffle(ptrs.begin(), ptrs.end(), gen);
		for (auto [ptr, size] : ptrs) {
			allocator->Free(ptr, size);
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		std::cout << "   随机模式 " << count << " 次操作: " << duration.count() << " μs" << std::endl;
	}

	void TestBurstPattern() {
		std::cout << "[TEST] 突发分配模式测试..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();

		const int burst_count = 10;
		const int burst_size = 100;

		for (int burst = 0; burst < burst_count; burst++) {
			std::vector<void*> burst_ptrs;

			// 突发分配
			for (int i = 0; i < burst_size; i++) {
				void* ptr = allocator->Allocate(128);
				if (ptr) burst_ptrs.push_back(ptr);
			}

			// 立即释放
			for (void* ptr : burst_ptrs) {
				allocator->Free(ptr, 128);
			}

			// 短暂暂停
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		std::cout << "   突发模式 " << burst_count << " 轮: " << duration.count() << " μs" << std::endl;
	}

	void TestMixedSizePattern() {
		std::cout << "[TEST] 混合大小模式测试..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();

		// 小块分配
		std::vector<void*> small_ptrs;
		for (int i = 0; i < 500; i++) {
			void* ptr = allocator->Allocate(32);
			if (ptr) small_ptrs.push_back(ptr);
		}

		// 中等块分配
		std::vector<void*> medium_ptrs;
		for (int i = 0; i < 100; i++) {
			void* ptr = allocator->Allocate(512);
			if (ptr) medium_ptrs.push_back(ptr);
		}

		// 大块分配
		std::vector<void*> large_ptrs;
		for (int i = 0; i < 10; i++) {
			void* ptr = allocator->Allocate(8192);
			if (ptr) large_ptrs.push_back(ptr);
		}

		// 混合释放
		for (void* ptr : small_ptrs) allocator->Free(ptr, 32);
		for (void* ptr : large_ptrs) allocator->Free(ptr, 8192);
		for (void* ptr : medium_ptrs) allocator->Free(ptr, 512);

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		std::cout << "   混合大小模式: " << duration.count() << " μs" << std::endl;
	}

	// ================================
	// 报告生成
	// ================================

	void GenerateReport(const std::string& filename) {
		std::ofstream report(filename);
		if (!report.is_open()) {
			std::cout << "[ERROR] 无法创建报告文件: " << filename << std::endl;
			return;
		}

		report << "动态内存分配器性能报告\n";
		report << "========================\n\n";

		// 基本信息
		report << "基本信息:\n";
		report << "  总内存空间: " << FormatBytes(allocator->GetTotalSpace()) << "\n";
		report << "  当前可用: " << FormatBytes(allocator->GetFreeSpace()) << "\n";
		report << "  头部开销: " << allocator->AllocatorHeaderSize() << " 字节\n\n";

		// 测试结果会在这里添加...

		report.close();
		std::cout << "[INFO] 报告已生成: " << filename << std::endl;
	}

private:
	std::string FormatBytes(size_t bytes) {
		const char* units[] = { "B", "KB", "MB", "GB" };
		double size = (double)bytes;
		int unit = 0;

		while (size >= 1024 && unit < 3) {
			size /= 1024;
			unit++;
		}

		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
		return oss.str();
	}
};

// 使用示例
void RunAnalysis() {
	DynamicAllocator allocator;

	// 创建64MB内存池
	if (!allocator.Create(64 * 1024 * 1024)) {
		std::cout << "[ERROR] 分配器初始化失败" << std::endl;
		return;
	}

	MemoryAnalyzer analyzer(&allocator);

	// 运行各种分析
	analyzer.AnalyzeMemoryUsage();
	analyzer.TestMemoryPatterns();
	analyzer.RunStressTest(10); // 10秒压力测试
	analyzer.GenerateReport("allocator_report.txt");

	allocator.Destroy();
}

// 简化的单元测试宏
#ifndef TEST_ASSERT
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[ASSERT FAILED] " << message << std::endl; \
            return false; \
        } \
    } while(0)
#endif

#ifndef TEST_ASSERT_EQ
#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            std::cout << "[ASSERT FAILED] " << message \
                      << " (expected: " << (expected) << ", actual: " << (actual) << ")" << std::endl; \
            return false; \
        } \
    } while(0)
#endif

// 额外的单元测试
bool TestAlignmentEdgeCases() {
	DynamicAllocator allocator;
	if (!allocator.Create(1024 * 1024)) return false;

	// 测试各种对齐边界情况
	for (unsigned short align = 1; align <= 256; align *= 2) {
		for (size_t size = 1; size <= 1000; size += 99) {
			void* ptr = allocator.AllocateAligned(size, align);
			TEST_ASSERT(ptr != nullptr, "分配失败");
			TEST_ASSERT((uintptr_t)ptr % align == 0, "对齐失败");

			// 验证可以写入数据
			memset(ptr, 0x42, size);

			allocator.FreeAligned(ptr);
		}
	}

	allocator.Destroy();
	return true;
}

bool TestBoundaryWrites() {
	DynamicAllocator allocator;
	if (!allocator.Create(1024 * 1024)) return false;

	// 分配相邻的内存块并验证边界
	std::vector<std::pair<void*, size_t>> blocks;

	for (int i = 0; i < 10; i++) {
		size_t size = 100;
		void* ptr = allocator.Allocate(size);
		TEST_ASSERT(ptr != nullptr, "分配失败");

		// 写入独特的模式
		uint8_t pattern = (uint8_t)(0x10 + i);
		memset(ptr, pattern, size);

		blocks.push_back({ ptr, size });
	}

	// 验证所有块的数据完整性
	for (size_t i = 0; i < blocks.size(); i++) {
		uint8_t expected = (uint8_t)(0x10 + i);
		uint8_t* data = (uint8_t*)blocks[i].first;

		for (size_t j = 0; j < blocks[i].second; j++) {
			TEST_ASSERT_EQ(expected, data[j], "边界数据损坏");
		}

		allocator.Free(blocks[i].first, blocks[i].second);
	}

	allocator.Destroy();
	return true;
}
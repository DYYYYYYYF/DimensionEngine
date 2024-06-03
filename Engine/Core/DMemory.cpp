#include "DMemory.hpp"

#include "EngineLogger.hpp"
#include "Platform/Platform.hpp"

bool Memory::Initialize(size_t size) {
	Platform::PlatformZeroMemory(&stats, sizeof(stats));
	if (!DynamicAlloc.Create(size)) {
		UL_FATAL("Memory system is unable to setup internal allocator. Application can not continue.");
		return false;
	}

	AllocateCount = 0;
	TotalAllocateSize = size;

	if (!AllocationMutex.Create()) {
		UL_FATAL("Unable to create allocation mutex.");
		return false;
	}

	UL_DEBUG("Memory system successfully allocated %llu bytes.", TotalAllocateSize);

	return true;
}

void Memory::Shutdown() {
	AllocationMutex.Destroy();
	DynamicAlloc.Destroy();
	AllocateCount = 0;
	TotalAllocateSize = 0;

	UL_INFO("Shutdown memory system, left memory: %llu.", DynamicAlloc.GetFreeSpace());
}

void* Memory::Allocate(size_t size, MemoryType type = MemoryType::eMemory_Type_Array) {
	if (type == eMemory_Type_Unknow) {
		UL_WARN("Called allocate using eMemory_Type_Unknow. Re-class this allocation.");
	}

	void* Block = nullptr;
	stats.total_allocated += size;
	stats.tagged_allocations[type] += size;
	AllocateCount++;

	// Make sure multi-threaded requests don't trample each other.
	if (!AllocationMutex.Lock()) {
		UL_FATAL("Error obtaining mutex lock during allocation.");
		return nullptr;
	}

	Block = DynamicAlloc.Allocate(size);
	AllocationMutex.UnLock();

	if (Block == nullptr) {
		UL_WARN("Allocate by platform. Dynamic allocator memory is not enough!");
		Block = Platform::PlatformAllocate(size, false);
	}

	if (Block == nullptr) {
		UL_FATAL("Allocate failed.");
	}

	Platform::PlatformZeroMemory(Block, size);

	return Block;
}

void  Memory::Free(void* block, size_t size, MemoryType type) {
	if (type == eMemory_Type_Unknow) {
		UL_WARN("Called free using eMemory_Type_Unknow. Re-class this allocation.");
	}

	stats.total_allocated -= size;
	stats.tagged_allocations[type] -= size;

	// Make sure multi-threaded requests don't trample each other.
	if (!AllocationMutex.Lock()) {
		UL_FATAL("Unable to obtain mutex lock for free operation. Heap corruption is likely.");
		return;
	}

	bool Result = DynamicAlloc.Free(block, size);
	AllocationMutex.UnLock();

	if (!Result) {
		Platform::PlatformFree(block, false);
	}

	block = nullptr;
}

void* Memory::Zero(void* block, size_t size) {
	return Platform::PlatformZeroMemory(block, size);
}

void* Memory::Copy(void* dst, const void* src, size_t size) {
	return Platform::PlatformCopyMemory(dst, src, size);
}

void* Memory::Set(void* dst, int val, size_t size) {
	return Platform::PlatformSetMemory(dst, val, size);
}

char* Memory::GetMemoryUsageStr() {
	const size_t gid = 1024 * 1024 * 1024;
	const size_t mid = 1024 * 1024;
	const size_t kid = 1024;

	char buffer[8000] = "\nSystem memory use (Type): \n";
	size_t offset = strlen(buffer);
	for (size_t i = 0; i < eMemory_Type_Max; i++) {
		char unit[4] = "XiB";
		float amount = 1.0f;
		if(stats.tagged_allocations[i] >= gid) {
			unit[0] = 'G';
			amount = stats.tagged_allocations[i] / (float)gid;
		} else if (stats.tagged_allocations[i] >= mid) {
			unit[0] = 'M';
			amount = stats.tagged_allocations[i] / (float)mid;
		} else if(stats.tagged_allocations[i] >= kid) {
			unit[0] = 'K';
			amount = stats.tagged_allocations[i] / (float)kid;
		} else {
			unit[0] = 'B';
			unit[1] = 0;
			amount = (float)stats.tagged_allocations[i];
		}

		int length = snprintf(buffer + offset, 8000, " %s: %.2f%s\n", MemoryTypeStrings[i], amount, unit);
		offset += length;
	}

#if defined(DPLATFORM_WINDOWS)
	char* outString = _strdup(buffer);
#elif defined(DPLATFORM_MACOS)
	char* outString = strdup(buffer);
#endif
	return outString;
}
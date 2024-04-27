#include "DMemory.hpp"

#include "EngineLogger.hpp"
#include "Platform/Platform.hpp"

bool Memory::Initialize() {
	Platform::PlatformZeroMemory(&stats, sizeof(stats));
	return true;
}

void* Memory::Allocate(size_t size, MemoryType type) {
	if (type == eMemory_Type_Unknow) {
		UL_WARN("Called allocate using eMemory_Type_Unknow. Re-class this allocation.");
	}

	stats.total_allocated += size;
	stats.tagged_allocations[type] += size;

	// TODO: Memory alignment
	void* block = Platform::PlatformAllocate(size, false);
	Platform::PlatformZeroMemory(block, size);
	return block;
}

void  Memory::Free(void* block, size_t size, MemoryType type) {
	if (type == eMemory_Type_Unknow) {
		UL_WARN("Called free using eMemory_Type_Unknow. Re-class this allocation.");
	}

	stats.total_allocated -= size;
	stats.tagged_allocations[type] -= size;

	// TODO: Memory alignment
	Platform::PlatformFree(block, false);
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

	char* outString = _strdup(buffer);
	return outString;
}
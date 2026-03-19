#include "DMemory.hpp"

#include "EngineLogger.hpp"
#include "Platform/Platform.hpp"
#include "Containers/FString.hpp"

struct Memory::SMemoryStats Memory::stats;
size_t Memory::TotalAllocateSize;
size_t Memory::AllocateCount;
Mutex Memory::AllocationMutex;

bool Memory::Initialize(size_t size) {
	Platform::PlatformZeroMemory(&stats, sizeof(stats));
	
	DynamicAllocator::Get().Resize(size);

	AllocateCount = 0;
	TotalAllocateSize = size;

	GLOG(Log::eDebug, "Memory system successfully allocated %llu bytes.", TotalAllocateSize);
	return true;
}

void Memory::Shutdown() {
	AllocateCount = 0;
	TotalAllocateSize = 0;
}

void* Memory::Allocate(size_t size, MemoryType type) {
	return AllocateAligned(size, DEFAULT_ALIGNMENT_SIZE, type);
}

void* Memory::AllocateAligned(size_t size, size_t alignment, MemoryType type) {
	if (type == eMemory_Type_Unknow) {
		GLOG(Log::eWarn, "Called allocate using eMemory_Type_Unknow. Re-class this allocation.");
	}

	void* Block = nullptr;
	stats.total_allocated += size;
	stats.tagged_allocations[type] += size;
	AllocateCount++;

	// Make sure multi-threaded requests don't trample each other.
	if (!AllocationMutex.Lock()) {
		GLOG(Log::eFatal, "Error obtaining mutex lock during allocation.");
		return nullptr;
	}

	Block = DynamicAllocator::Get().AllocateAligned(size, alignment);
	AllocationMutex.UnLock();

	if (Block == nullptr) {
		GLOG(Log::eWarn, "Allocate by platform. Dynamic allocator memory is not enough!");
		Block = Platform::PlatformAllocate(size, false);
	}

	if (Block == nullptr) {
		GLOG(Log::eFatal, "Allocate failed.");
	}

	Platform::PlatformZeroMemory(Block, size);

	return Block;
}

void Memory::AllocateReport(size_t size, MemoryType type) {
	// Make sure multi-threaded requests don't trample each other.
	if (!AllocationMutex.Lock()) {
		GLOG(Log::eFatal, "Error obtaining mutex lock during allocation.");
		return;
	}

	stats.total_allocated += size;
	stats.tagged_allocations[type] += size;
	AllocateCount++;

	AllocationMutex.UnLock();
}

void  Memory::Free(void* block, MemoryType type) {
	size_t alloc_size = 0;
	Memory::GetAlignmentSize(block, &alloc_size, nullptr);
	FreeAligned(block, alloc_size, type);
}

void Memory::FreeAligned(void* block, size_t size, MemoryType type) {
	if (type == eMemory_Type_Unknow) {
		GLOG(Log::eWarn, "Called free using eMemory_Type_Unknow. Re-class this allocation.");
	}

	// Make sure multi-threaded requests don't trample each other.
	if (!AllocationMutex.Lock()) {
		GLOG(Log::eFatal, "Unable to obtain mutex lock for free operation. Heap corruption is likely.");
		return;
	}

	stats.total_allocated -= size;
	stats.tagged_allocations[type] -= size;
	AllocateCount--;

	bool Result = DynamicAllocator::Get().FreeAligned(block);
	AllocationMutex.UnLock();

	if (!Result) {
		Platform::PlatformFree(block, false);
	}

	block = nullptr;
}

void Memory::FreeReport(size_t size, MemoryType type) {
	if (!AllocationMutex.Lock()) {
		GLOG(Log::eFatal, "Unable to obtain mutex lock for free operation. Heap corruption is likely.");
		return;
	}

	stats.total_allocated -= size;
	stats.tagged_allocations[type] -= size;
	AllocateCount--;

	AllocationMutex.UnLock();
}

bool Memory::GetAlignmentSize(void* block, size_t* out_size, size_t* out_alignment) {
	return DynamicAllocator::Get().GetAlignmentSize(block, out_size, out_alignment);
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

const char* Memory::GetUnitForSize(size_t size_bytes, float* out_amount) {
	if (size_bytes >= GIBIBYTES(1)) {
		*out_amount = static_cast<float>((double)size_bytes / (double)GIBIBYTES(1));
		return "GiB";
	}
	else if (size_bytes >= MEBIBYTES(1)) {
		*out_amount = static_cast<float>((double)size_bytes / (double)MEBIBYTES(1));
		return "MiB";
	}
	else if (size_bytes >= KIBIBYTES(1)) {
		*out_amount = static_cast<float>((double)size_bytes / (double)KIBIBYTES(1));
		return "KiB";
	}
	else {
		*out_amount = (float)size_bytes;
		return "B";
	}
}

FString Memory::GetMemoryUsageStr() {
	char buffer[8000] = "\nSystem memory use (Type): \n";
	size_t offset = strlen(buffer);
	for (size_t i = 0; i < eMemory_Type_Max; i++) {
		float amount = 1.0f;
		const char* Unit = GetUnitForSize(stats.tagged_allocations[i], &amount);
		int length = snprintf(buffer + offset, 8000, " %s: %.2f%s\n", MemoryTypeStrings[i], amount, Unit);
		offset += length;
	}

	// Compute total usage.
	{
		DynamicAllocator& Allocator = DynamicAllocator::Get();
		size_t TotalSpace = Allocator.GetTotalSpace();
		size_t FreeSpace = Allocator.GetFreeSpace();
		size_t UsedSpace = TotalSpace - FreeSpace;

		float UsedAmount = 1.0f;
		const char* UsedUnit = GetUnitForSize(UsedSpace, &UsedAmount);

		float TotalAmount = 1.0f;
		const char* TotalUnit = GetUnitForSize(TotalSpace, &TotalAmount);

		double PercentUsed = (double)UsedSpace / (double)TotalSpace;

		snprintf(buffer + offset, 8000, "Total memory usage: %.2f%s of %.2f%s (%d%%%%)\n", UsedAmount, UsedUnit, TotalAmount, TotalUnit, (int)(PercentUsed * 100));
	}

	return buffer;
}

void Memory::ShowMemoryUsage() {
	FString Msg = GetMemoryUsageStr();
	GLOG(Log::eDebug, Msg.CStr());
}

size_t Memory::GetAllocateCount() { 
	return AllocateCount; 
}

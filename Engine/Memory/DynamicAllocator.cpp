#include "DynamicAllocator.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

bool DynamicAllocator::Create(unsigned long long total_size) {
	if (total_size < 1) {
		LOG_ERROR("Dynamic allocator create can not have a total_size of 0. Failed.");
		return false;
	}

	List.Create(total_size);
	TotalSize = total_size;
	MemoryBlock = Platform::PlatformAllocate(total_size,false);
	ASSERT(MemoryBlock);

	Memory::Zero(MemoryBlock, total_size);

	return true;
}

bool DynamicAllocator::Destroy() {
	if (MemoryBlock != nullptr) {
		List.Destroy();
		Memory::Zero(MemoryBlock, TotalSize);
		MemoryBlock = nullptr;
		TotalSize = 0;
		return true;
	}

	return false;
}

void* DynamicAllocator::Allocate(unsigned long long size) {
	if (size > 0) {
		unsigned long Offset = 0;
		if (List.AllocateBlock(size, &Offset)) {
			// Use that offset against the base memory block to get the block.
			void* Block = ((char*)MemoryBlock + Offset);
			return Block;
		}
		else {
			LOG_ERROR("Dynamic allocator allocate no blocks of memory large enough to allocate from.");
			size_t available = List.GetFreeSpace();
			LOG_ERROR("Requested size: %llu, Total space available: %llu.", size, available);
			// TODO: Report fragmentation?
			return nullptr;
		}
	}

	LOG_ERROR("Dynamic allocator allocate requires a valid size.");
	return nullptr;
}

bool DynamicAllocator::Free(void* block, unsigned long long size) {
	if (block == nullptr) {
		LOG_ERROR("Dynamic allocator free requires a valid block (0x%p).", block);
		return false;
	}

	bool IsOut = (size_t)((char*)block - (char*)MemoryBlock) > TotalSize;
	if (MemoryBlock && IsOut) {
		void* EndOfBlock = (char*)MemoryBlock + TotalSize;
		LOG_ERROR("Dynamic allocator trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p). Sub size: %uul, Total size: %uul.",
			block, MemoryBlock, EndOfBlock, (char*)block - (char*)MemoryBlock, TotalSize);
		return false;
	}

	size_t Offset = (char*)block - (char*)MemoryBlock;
	if (!List.FreeBlock(size, Offset)) {
		LOG_ERROR("Dynamic allocator free failed.");
		return false;
	}

	return true;
}

unsigned long long DynamicAllocator::GetFreeSpace() {
	return List.GetFreeSpace();
}

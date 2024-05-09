#include "DynamicAllocator.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

bool DynamicAllocator::Create(unsigned long long total_size) {
	if (total_size < 1) {
		UL_ERROR("Dynamic allocator create can not have a total_size of 0. Failed.");
		return false;
	}

	List.Create(total_size);
	TotalSize = total_size;
	MemoryBlock = Memory::Allocate(total_size, MemoryType::eMemory_Type_Array);
	Memory::Zero(MemoryBlock, total_size);
	ASSERT(MemoryBlock);

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
		size_t Offset = 0;
		if (List.AllocateBlock(size, &Offset)) {
			// Use that offset against the base memory block to get the block.
			void* Block = ((char*)MemoryBlock + Offset);
			return Block;
		}
		else {
			UL_ERROR("Dynamic allocator allocate no blocks of memory large enough to allocate from.");
			size_t available = List.GetFreeSpace();
			UL_ERROR("Requested size: %llu, Total space available: %llu.", size, available);
			// TODO: Report fragmentation?
			return nullptr;
		}
	}

	UL_ERROR("Dynamic allocator allocate requires a valid size.");
	return false;
}

bool DynamicAllocator::Free(void* block, unsigned long long size) {
	if (block == nullptr) {
		UL_ERROR("Dynamic allocator free requires a valid block (0x%p).", block);
		return false;
	}

	if (MemoryBlock || block > (char*)MemoryBlock + TotalSize) {
		void* EndOfBlock = (char*)MemoryBlock + TotalSize;
		UL_ERROR("Dynamic allocator trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p).", block, MemoryBlock, EndOfBlock);
		return false;
	}

	size_t Offset = (char*)block - (char*)MemoryBlock;
	if (!List.FreeBlock(size, Offset)) {
		UL_ERROR("Dynamic allocator free failed.");
		return false;
	}

	return true;
}

unsigned long long DynamicAllocator::GetFreeSpace() {
	return List.GetFreeSpace();
}

#include "DynamicAllocator.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

struct AllocHeader {
	size_t size;
	unsigned short alignment;
	unsigned short alignment_offset;
};

bool DynamicAllocator::Create(size_t total_size) {
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

void* DynamicAllocator::Allocate(size_t size) {
	return AllocateAligned(size, 1);
}

void* DynamicAllocator::AllocateAligned(size_t size, unsigned short alignment) {
	if (size > 0 && alignment > 0) {
		size_t Offset = 0;
		// Account for space for the header.
		size_t ActualSize = size + sizeof(AllocHeader);
		unsigned short AlignmentOffset = 0;

		void* Block = nullptr;
		if (List.AllocateBlockAligned(ActualSize, alignment, &Offset, &AlignmentOffset)) {
			// Set the header info.
			AllocHeader* Header = (AllocHeader*)(((unsigned char*)MemoryBlock) + Offset);
			Header->alignment = alignment;
			Header->alignment_offset = AlignmentOffset;
			Header->size = size; //	 Store the actual size here.
			// Use that offset against the base memory block to get the block.
			Block = ((char*)MemoryBlock) + Offset + sizeof(AllocHeader);
		}
		else {
			LOG_ERROR("DynamicAllocator::AllocateAligned() allocate no blocks of memory large enough to allocate from.");
			size_t available = List.GetFreeSpace();
			LOG_ERROR("Requested size: %llu, Total space available: %llu.", size, available);
			// TODO: Report fragmentation?
			Block = nullptr;
		}

		return Block;
	}

	LOG_ERROR("Dynamic allocator allocate requires a valid size and alignment.");
	return nullptr;
}

bool DynamicAllocator::Free(void* block, size_t size) {
	return FreeAligned(block);
}

bool DynamicAllocator::FreeAligned(void* block) {
	if (block == nullptr) {
		LOG_ERROR("DynamicAllocator::FreeAligned(): Free requires a valid block (0x%p).", block);
		return false;
	}

	bool IsOut = (size_t)((char*)block - (char*)MemoryBlock) > TotalSize;
	if (MemoryBlock && IsOut) {
		void* EndOfBlock = (char*)MemoryBlock + TotalSize;
		LOG_ERROR("DynamicAllocator::FreeAligned(): Trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p). Sub size: %uul, Total size: %uul.",
			block, MemoryBlock, EndOfBlock, (char*)block - (char*)MemoryBlock, TotalSize);
		return false;
	}

	size_t Offset = (char*)block - (char*)MemoryBlock;

	// Get the header.
	AllocHeader* Header = (AllocHeader*)(((unsigned char*)block) - sizeof(AllocHeader));
	size_t ActualSize = Header->size + sizeof(AllocHeader);
	if (!List.FreeBlockAligned(ActualSize, Offset - sizeof(AllocHeader), Header->alignment_offset)) {
		LOG_ERROR("DynamicAllocator::FreeAligned(): Free failed.");
		return false;
	}

	return true;
}

bool DynamicAllocator::GetAlignmentSize(void* block, size_t* out_size, unsigned short* out_alignment) {
	// Get the header.
	AllocHeader* Header = (AllocHeader*)((char*)block - sizeof(AllocHeader));
	*out_size = Header->size;
	*out_alignment = Header->alignment;
	return true;
}

size_t DynamicAllocator::GetFreeSpace() {
	return List.GetFreeSpace();
}

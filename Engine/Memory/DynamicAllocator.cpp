#include "DynamicAllocator.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

// The storage size in bytes of a node's user memory block size.
#define DSIZE_STORAGE sizeof(uint32_t)

struct AllocHeader {
	void* start;
	unsigned short alignment;
};

bool DynamicAllocator::Create(size_t total_size) {
	if (total_size < 1) {
		LOG_ERROR("Dynamic allocator create can not have a total_size of 0. Failed.");
		return false;
	}

	List.Create(total_size);
	TotalSize = total_size;
	MemoryBlock = Platform::PlatformAllocate(total_size, false);
	ASSERT(MemoryBlock);

	Platform::PlatformSetMemory(MemoryBlock, 0, total_size);

	return true;
}

bool DynamicAllocator::Destroy() {
	if (MemoryBlock != nullptr) {
		List.Destroy();
		Platform::PlatformFree(MemoryBlock, false);
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
		// The size required is based on the requested size, plus the alignment, header and a u32 to hold
		// the size for quick/easy lookups.
		size_t RequiredSize = alignment + sizeof(AllocHeader) + DSIZE_STORAGE + size;

		// NOTE: This cast will really only be an issue on allocations over ~4GiB, so... don't do that.
		ASSERT(RequiredSize < 4294967295U);

		size_t BaseOffset = 0;
		if (List.AllocateBlock(RequiredSize, &BaseOffset)) {
			void* ptr = (void*)((size_t)MemoryBlock + BaseOffset);
			// Start the alignment after enough space to hold a u32. This allows for the u32 to be stored
			// immediately before the user block, while maintaining alignment on said user block.
			size_t AlignedBlockOffset = PaddingAligned((size_t)ptr + BaseOffset + DSIZE_STORAGE, alignment);
			// Store the size just before the user data block
			uint32_t* BlockSize = (uint32_t*)(AlignedBlockOffset - DSIZE_STORAGE);
			*BlockSize = (uint32_t)size;

			// Store the header immediately after the user block.
			AllocHeader* Header = (AllocHeader*)(AlignedBlockOffset + size);
			Header->start = ptr;
			Header->alignment = alignment;

			return (void*)AlignedBlockOffset;
		}
		else {
			LOG_ERROR("DynamicAllocator::AllocateAligned() allocate no blocks of memory large enough to allocate from.");
			size_t available = List.GetFreeSpace();
			LOG_ERROR("Requested size: %llu, Total space available: %llu.", size, available);
			// TODO: Report fragmentation?
			return nullptr;
		}
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

	size_t Sub = (size_t)block - (size_t)MemoryBlock;
	bool IsOut = Sub > TotalSize;
	if (MemoryBlock && IsOut) {
		void* EndOfBlock = (char*)MemoryBlock + TotalSize;
		LOG_ERROR("DynamicAllocator::FreeAligned(): Trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p). Sub size: %uul, Total size: %uul.",
			block, MemoryBlock, EndOfBlock, (char*)block - (char*)MemoryBlock, TotalSize);
		return false;
	}

	uint32_t* BlockSize = (uint32_t*)((size_t)block - DSIZE_STORAGE);
	AllocHeader* Header = (AllocHeader*)((size_t)block + *BlockSize);
	size_t RequiredSize = Header->alignment + sizeof(AllocHeader) + DSIZE_STORAGE + *BlockSize;
	size_t Offset = (size_t)Header->start - (size_t)MemoryBlock;

	if (!List.FreeBlock(RequiredSize, Offset)) {
		LOG_ERROR("DynamicAllocator::FreeAligned(): Free failed.");
		return false;
	}

	return true;
}

bool DynamicAllocator::GetAlignmentSize(void* block, size_t* out_size, unsigned short* out_alignment) {
	// Get the header.
	*out_size = *(uint32_t*)((size_t)block - DSIZE_STORAGE);
	AllocHeader* Header = (AllocHeader*)((size_t)block + *out_size);
	*out_alignment = Header->alignment;
	return true;
}

size_t DynamicAllocator::GetTotalSpace() {
	return TotalSize;
}

size_t DynamicAllocator::GetFreeSpace() {
	return List.GetFreeSpace();
}

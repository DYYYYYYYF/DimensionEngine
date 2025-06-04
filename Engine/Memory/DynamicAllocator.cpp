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
		GLOG(Log::eError, "Dynamic allocator create can not have a total_size of 0. Failed.");
		return false;
	}

	TotalSize = total_size;
	if (!List.Create(TotalSize)) {
		return false;
	}
	
	MemoryBlock = Platform::PlatformAllocate(total_size, false);
	if (MemoryBlock == nullptr) {
		GLOG(Log::eFatal, "DynamicAllocator::Create() Cannot allocate enough memory for dynamic allocator.");
		return false;
	}

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
		size_t HeaderSize = sizeof(AllocHeader);
		size_t StorageSize = DSIZE_STORAGE;
		size_t RequiredSize = alignment + HeaderSize + StorageSize + size;

		// NOTE: This cast will really only be an issue on allocations over ~4GiB, so... don't do that.
		ASSERT(RequiredSize < 4294967295U);

		size_t BaseOffset = 0;
		if (List.AllocateBlock(RequiredSize, &BaseOffset)) {
			void* ptr = (void*)((size_t)MemoryBlock + BaseOffset);
			// Start the alignment after enough space to hold a u32. This allows for the u32 to be stored
			// immediately before the user block, while maintaining alignment on said user block.
			size_t AlignedBlockOffset = PaddingAligned((size_t)ptr + DSIZE_STORAGE, alignment);
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
			GLOG(Log::eWarn, "DynamicAllocator::AllocateAligned() allocate no blocks of memory large enough to allocate from.");
			size_t available = List.GetFreeSpace();
			GLOG(Log::eWarn, "Requested size: %llu, Total space available: %llu.", size, available);
			// TODO: Report fragmentation?
			return nullptr;
		}
	}

	GLOG(Log::eError, "Dynamic allocator allocate requires a valid size and alignment.");
	return nullptr;
}

bool DynamicAllocator::Free(void* block, size_t size) {
	if (block != nullptr) {
		size_t stored_size;
		unsigned short alignment;
		if (GetAlignmentSize(block, &stored_size, &alignment)) {
			if (stored_size != size) {
				GLOG(Log::eWarn, "Size mismatch in Free: expected %zu, got %zu", stored_size, size);
			}
		}
	}
	return FreeAligned(block);
}

bool DynamicAllocator::FreeAligned(void* block) {
	if (block == nullptr || MemoryBlock == nullptr) {
		GLOG(Log::eError, "DynamicAllocator::FreeAligned(): Free requires a valid block (0x%p).", block);
		return false;
	}

	void* EndOfBlock = (void*)((size_t)MemoryBlock + TotalSize);
	if (block < MemoryBlock || block >= EndOfBlock) {
		GLOG(Log::eError, "DynamicAllocator::FreeAligned(): Trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p). Sub size: %uul, Total size: %uul.",
			block, MemoryBlock, EndOfBlock, (size_t)block - (size_t)MemoryBlock, TotalSize);
		return false;
	}

	uint32_t* BlockSize = (uint32_t*)((size_t)block - DSIZE_STORAGE);
	AllocHeader* Header = (AllocHeader*)((size_t)block + *BlockSize);
	size_t RequiredSize = Header->alignment + sizeof(AllocHeader) + DSIZE_STORAGE + *BlockSize;
	size_t Offset = (size_t)Header->start - (size_t)MemoryBlock;

	if (!List.FreeBlock(RequiredSize, Offset)) {
		GLOG(Log::eError, "DynamicAllocator::FreeAligned(): Free failed.");
		return false;
	}

	return true;
}

bool DynamicAllocator::GetAlignmentSize(void* block, size_t* out_size, unsigned short* out_alignment) {
	// 添加基本的安全检查
	if (block == nullptr || MemoryBlock == nullptr || out_size == nullptr || out_alignment == nullptr) {
		return false;
	}

	// 边界检查
	void* EndOfMemory = (void*)((size_t)MemoryBlock + TotalSize);
	if (block < MemoryBlock || block >= EndOfMemory) {
		return false;
	}

	// 检查偏移量
	size_t offset = (size_t)block - (size_t)MemoryBlock;
	if (offset < DSIZE_STORAGE) {
		return false;
	}

	// 检查BlockSize指针
	void* BlockSizePtr = (void*)((size_t)block - DSIZE_STORAGE);
	if (BlockSizePtr < MemoryBlock || BlockSizePtr >= EndOfMemory) {
		return false;
	}

	// 安全读取块大小
	uint32_t block_size = *(uint32_t*)BlockSizePtr;
	if (block_size == 0 || block_size > TotalSize) {
		return false;
	}

	// 检查Header指针
	void* HeaderPtr = (void*)((size_t)block + block_size);
	if (HeaderPtr < MemoryBlock ||
		(size_t)HeaderPtr + sizeof(AllocHeader) >(size_t)EndOfMemory) {
		return false;
	}

	// 安全读取头部
	AllocHeader* Header = (AllocHeader*)HeaderPtr;
	if (Header->alignment == 0 || Header->alignment > 1024) {
		return false;
	}

	*out_size = block_size;
	*out_alignment = Header->alignment;
	return true;
}

size_t DynamicAllocator::GetTotalSpace() {
	return TotalSize;
}

size_t DynamicAllocator::GetFreeSpace() {
	return List.GetFreeSpace();
}

size_t DynamicAllocator::AllocatorHeaderSize() {
	// Enough space for a header and size storage.
	return sizeof(AllocHeader) + DSIZE_STORAGE;
}

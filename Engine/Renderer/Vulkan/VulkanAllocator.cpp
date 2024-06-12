#include "VulkanAllocator.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#if DVULKAN_USE_CUSTOM_ALLOCATOR == 1
void* VulkanAllocator::Allocation(void* user_data, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope) {
	// nullptr must be returned if this fails.
	if (size == 0) {
		return nullptr;
	}

	void* Result = Memory::AllocateAligned(size, (unsigned short)alignment, MemoryType::eMemory_Type_Vulkan);
#ifdef DVULKAN_ALLOCATOR_TRACE
	LOG_INFO("Allocated block %p. Size=%llu, Alignment=%llu.", Result, size, alignment);
#endif
	return Result;
}

void VulkanAllocator::Free(void* user_date, void* memory) {
	if (memory == nullptr) {
#ifdef DVULKAN_ALLOCATOR_TRACE
		LOG_INFO("Block is nullptr, nothing to free: %p.", memory);
#endif
		return;
	}

#ifdef DVULKAN_ALLOCATOR_TRACE
	LOG_INFO("Attempting to free block %p.", memory);
#endif
	size_t size;
	unsigned short alignment;
	bool Result = Memory::GetAlignmentSize(memory, &size, &alignment);
	if (Result) {
#ifdef DVULKAN_ALLOCATOR_TRACE
		LOG_INFO("Block %p found with size/alignment: %llu/%llu. Freeing aligned block.", memory, size, alignment);
#endif
		Memory::FreeAligned(memory, size, alignment, MemoryType::eMemory_Type_Vulkan);
	}
	else {
		LOG_ERROR("VulkanAllocFree failed to get alignment lookup for block %p.", memory);
	}
}

void* VulkanAllocator::Reallocation(void* user_data, void* original, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope) {
	if (original == nullptr) {
		return Allocation(user_data, size, alignment, allocation_scope);
	}

	if (size == 0) {
		return nullptr;
	}

	// NOTE: If pOriginal is not nullptr, the same alignment must be used for the new allocation as original.
	size_t alloc_size;
	unsigned short alloc_alignment;
	bool IsAligned = Memory::GetAlignmentSize(original, &alloc_size, &alloc_alignment);
	if (!IsAligned) {
		LOG_ERROR("VulkanAllocReallocation of unaligned block %p.", original);
		return nullptr;
	}

	if (alloc_alignment != alignment) {
		LOG_ERROR("Attemp to realloc using a different alignment of %llu than the original of %hu.", alignment, alloc_alignment);
		return nullptr;
	}

#ifdef DVULKAN_ALLOCATOR_TRACE
	LOG_INFO("Attempting to realloc block %p.", original);
#endif

	void* Result = Allocation(user_data, size, alloc_alignment, allocation_scope);
	if (Result) {
#ifdef DVULKAN_ALLOCATOR_TRACE
		LOG_INFO("Block %p reallocated to %p, copying data.", original, Result);
#endif

		// Copy over the original memory.
		Memory::Copy(Result, original, size);
#ifdef DVULKAN_ALLOCATOR_TRACE
		LOG_INFO("Freeing original aligned block %p.", original);
#endif
		// Free the original memory only if the new allocation was successful.
		Memory::FreeAligned(original, alloc_size, alloc_alignment, MemoryType::eMemory_Type_Vulkan);
	}
	else {
#ifdef DVULKAN_ALLOCATOR_TRACE
		LOG_ERROR("Failed to realloc %p.", original);
#endif
	}

	return Result;
}

void VulkanAllocator::InternalAlloc(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
#ifdef DVULKAN_ALLOCATOR_TRACE
	LOG_INFO("External allocation of size: %llu.", size);
#endif
	Memory::AllocateReport(size, MemoryType::eMemory_Type_Vulkan_EXT);
}

void VulkanAllocator::InternalFree(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
#ifdef DVULKAN_ALLOCATOR_TRACE
	LOG_INFO("External free of size: %llu.", size);
#endif
	Memory::FreeReport(size, MemoryType::eMemory_Type_Vulkan_EXT);
}

bool VulkanAllocator::Create(vk::AllocationCallbacks* callbacks, class VulkanContext* context) {
	if (callbacks == nullptr) {
		return false;
	}

	callbacks->setPfnAllocation(VulkanAllocator::Allocation);
	callbacks->setPfnReallocation(VulkanAllocator::Reallocation);
	callbacks->setPfnFree(VulkanAllocator::Free);
	callbacks->setPfnInternalAllocation(VulkanAllocator::InternalAlloc);
	callbacks->setPfnInternalFree(VulkanAllocator::InternalFree);
	callbacks->setPUserData(context);
	return true;
}
#endif	// DVULKAN_USE_CUSTOM_ALLOCATOR

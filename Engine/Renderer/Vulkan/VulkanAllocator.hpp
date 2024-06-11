#pragma once

#include <vulkan/vulkan.hpp>

// NOTE: If wanting to trace allocations, uncomment this.
#ifndef DVULKAN_ALLOCATOR_TRACE
#define DVULKAN_ALLOCATOR_TRACE 1
#endif

// NOTE: To disable the custom allocator, comment this out or set to 0.
#ifndef DVULKAN_USE_CUSTOM_ALLOCATOR
#define DVULKAN_USE_CUSTOM_ALLOCATOR 1
#endif 

class VulkanAllocator {
public:
#ifdef DVULKAN_USE_CUSTOM_ALLOCATOR == 1
	static void* Allocation(void* user_data, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope);
	static void Free(void* user_date, void* memory);
	static void* Reallocation(void* user_data, void* origin, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope);
	static void InternalFree(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
	static bool Create(vk::AllocationCallbacks* callbacks, class VulkanContext* context);
#endif
};

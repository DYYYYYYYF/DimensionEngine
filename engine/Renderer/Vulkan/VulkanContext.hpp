#pragma once
#include <vulkan/vulkan.hpp>

#include "Defines.hpp"
#include "VulkanDevice.hpp"

class VulkanContext {
public:
	VulkanContext(): Allocator(nullptr) {}
	virtual ~VulkanContext() {}

public:
	vk::Instance Instance;
	vk::AllocationCallbacks* Allocator;
	vk::SurfaceKHR Surface;

#ifdef LEVEL_DEBUG
	vk::DebugUtilsMessengerEXT DebugMessenger;
#endif

	VulkanDevice Device;
};

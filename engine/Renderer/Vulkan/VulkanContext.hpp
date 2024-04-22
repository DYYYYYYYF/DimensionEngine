#pragma once
#include <vulkan/vulkan.hpp>

#include "Defines.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanRenderpass.hpp"

class VulkanContext {
public:
	VulkanContext(): Allocator(nullptr) {}
	virtual ~VulkanContext() {}

public:
	virtual uint32_t FindMemoryIndex(uint32_t type_filter, vk::MemoryPropertyFlags property_flags) {
		vk::PhysicalDeviceMemoryProperties MemoryProperties;
		MemoryProperties = Device.GetPhysicalDevice().getMemoryProperties();

		for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i) {
			if (type_filter & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
				return i;
			}
		}

		UL_WARN("Unable to find suitable memory type!");
		return -1;
	}

public:
	uint32_t FrameBufferWidth;
	uint32_t FrameBufferHeight;

	uint32_t ImageIndex;
	uint32_t CurrentFrame;
	bool RecreatingSwapchain;

	vk::Instance Instance;
	vk::AllocationCallbacks* Allocator;
	vk::SurfaceKHR Surface;

#ifdef LEVEL_DEBUG
	vk::DebugUtilsMessengerEXT DebugMessenger;
#endif

	VulkanDevice Device;
	VulkanSwapchain Swapchain;
	VulkanRenderPass MainRenderPass;
};

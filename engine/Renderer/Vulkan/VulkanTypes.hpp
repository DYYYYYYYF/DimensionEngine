#pragma once

#include "Defines.hpp"

#include <vulkan/vulkan.hpp>

struct SSwapchainSupportInfo {
	vk::SurfaceCapabilitiesKHR capabilities;
	unsigned int format_count = 0;
	std::vector<vk::SurfaceFormatKHR> formats;
	unsigned int present_mode_count = 0;
	std::vector<vk::PresentModeKHR> present_modes;
};

struct SVulkanContext {
	vk::Instance instance;
	vk::AllocationCallbacks* allocator;
	vk::SurfaceKHR surface;

#ifdef LEVEL_DEBUG
	vk::DebugUtilsMessengerEXT debug_messenger;
#endif
};

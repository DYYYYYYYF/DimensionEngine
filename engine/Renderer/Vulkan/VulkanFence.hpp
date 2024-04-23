#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;

struct VulkanFence {
public:
	void Create(VulkanContext* context, bool signaled);
	void Destroy(VulkanContext* context);
	bool Wait(VulkanContext* context, size_t timeout_ns);
	void Reset(VulkanContext* context);

	vk::Fence fence;
	bool is_signaled;
};
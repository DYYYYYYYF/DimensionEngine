#pragma once

#include "vulkan/vulkan.hpp"

class VulkanContext;

class VulkanImage {
public:
	VulkanImage() {}
	virtual ~VulkanImage() {}

public:
	void CreateImage(VulkanContext* context, vk::ImageType type, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, 
		vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memory_flags, bool create_view, vk::ImageAspectFlags view_aspect_flags);

	void CreateImageView(VulkanContext* context, vk::Format format, vk::ImageAspectFlags view_aspect_flags);
	void Destroy(VulkanContext* context);

public:
	vk::Image Image;
	vk::DeviceMemory DeviceMemory;
	vk::ImageView ImageView;
	uint32_t Width;
	uint32_t Height;
};
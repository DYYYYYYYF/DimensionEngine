#pragma once

#include "vulkan/vulkan.hpp"
#include "Resources/Texture.hpp"

enum TextureType;
class VulkanContext;
class VulkanCommandBuffer;

class VulkanImage {
public:
	VulkanImage() {}
	virtual ~VulkanImage() {}

public:
	void CreateImage(VulkanContext* context, TextureType type, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, 
		vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memory_flags, bool create_view, vk::ImageAspectFlags view_aspect_flags);

	void CreateImageView(VulkanContext* context, TextureType type, vk::Format format, vk::ImageAspectFlags view_aspect_flags);
	void Destroy(VulkanContext* context);

	/*
	* Transitions the provided image from old_layout to new_layout
	*/
	void TransitionLayout(VulkanContext* context, TextureType type, VulkanCommandBuffer* command_buffer, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

	/*
	* Copies data in buffer to provided image.
	* @param context The Vulkan context.
	* @param image The image to copy the buffer's data to.
	* @param buffer The buffer whose data will be copied
	*/
	void CopyFromBuffer(VulkanContext* context, TextureType type, vk::Buffer buffer, VulkanCommandBuffer* command_buffer);

public:
	vk::Image Image;
	vk::DeviceMemory DeviceMemory;
	vk::ImageView ImageView;
	uint32_t Width;
	uint32_t Height;
};


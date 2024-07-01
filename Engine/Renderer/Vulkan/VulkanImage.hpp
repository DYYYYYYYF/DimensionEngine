#pragma once

#include "vulkan/vulkan.hpp"
#include "Resources/Texture.hpp"

enum TextureType;
class VulkanContext;
class VulkanCommandBuffer;

class VulkanImage {
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

	/**
	 * @brief Copies data in the provided image to the given buffer.
	 * 
	 * @param context The Vulkan context.
	 * @param type The type of texture. Provides hint to layer count.
	 * @param buffer The buffer to copy to.
	 * @param commandBuffer The command buffer to be used for the copy.
	 */
	void CopyToBuffer(VulkanContext* context, TextureType type, vk::Buffer buffer, VulkanCommandBuffer* commandBuffer);

	/**
	 * @brief Copies data in the provided image to the given buffer.
	 *
	 * @param context The Vulkan context.
	 * @param type The type of texture. Provides hint to layer count.
	 * @param buffer The buffer to copy to.
	 * @param x The x-coordinate of the pixel to copy.
	 * @param y The y-coordinate of the pixel to copy.
	 * @param commandBuffer The command buffer to be used for the copy.
	 */
	void CopyPixelToBuffer(VulkanContext* context, TextureType type, vk::Buffer buffer, uint32_t x, uint32_t y, VulkanCommandBuffer* commandBuffer);

public:
	vk::Image Image;
	vk::DeviceMemory DeviceMemory;
	vk::ImageView ImageView;
	vk::MemoryRequirements MemoryRequirements;
	vk::MemoryPropertyFlags MemoryFlags;
	uint32_t Width;
	uint32_t Height;
};


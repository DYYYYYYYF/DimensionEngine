#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;
class VulkanRenderPass;

class VulkanFrameBuffer {
public:
	VulkanFrameBuffer() {}
	virtual ~VulkanFrameBuffer() {}

public:
	void Create(VulkanContext* context, VulkanRenderPass* renderpass, uint32_t width, uint32_t height, uint32_t attachment_count, vk::ImageView* attachments);
	void Destroy(VulkanContext* context);
	
public:
	vk::Framebuffer FrameBuffer;
	uint32_t AttachmentCount;
	vk::ImageView* Attachments;
	VulkanRenderPass* RenderPass;

};
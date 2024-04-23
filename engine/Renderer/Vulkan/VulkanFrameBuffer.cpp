#include "VulkanFrameBuffer.hpp"
#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void VulkanFrameBuffer::Create(VulkanContext* context, VulkanRenderPass* renderpass, uint32_t width, uint32_t height, uint32_t attachment_count, vk::ImageView* attachments) {
	Attachments = (vk::ImageView*)Memory::Allocate(sizeof(vk::ImageView) * attachment_count, MemoryType::eMemory_Type_Renderer);
	for (uint32_t i = 0; i < attachment_count; ++i) {
		Attachments[i] = attachments[i];
	}

	RenderPass = renderpass;
	AttachmentCount = attachment_count;
	    
	// Create info
	vk::FramebufferCreateInfo FrameBufferCreateInfo;
	FrameBufferCreateInfo.setRenderPass(RenderPass->GetRenderPass())
		.setAttachmentCount(AttachmentCount)
		.setPAttachments(Attachments)
		.setWidth(width)
		.setHeight(height)
		.setLayers(1);

	FrameBuffer = context->Device.GetLogicalDevice().createFramebuffer(FrameBufferCreateInfo, context->Allocator);
	ASSERT(FrameBuffer);
}

void VulkanFrameBuffer::Destroy(VulkanContext* context) {
	context->Device.GetLogicalDevice().destroyFramebuffer(FrameBuffer, context->Allocator);
	if (Attachments != nullptr) {
		Memory::Free(Attachments, sizeof(vk::ImageView) * AttachmentCount, MemoryType::eMemory_Type_Renderer);
	}

	AttachmentCount = 0;
}
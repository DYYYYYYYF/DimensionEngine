#include "VulkanRenderpass.hpp"

#include "VulkanContext.hpp"
#include "VulkanCommandBuffer.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void VulkanRenderPass::Create(VulkanContext* context,
	float x, float y, float w, float h,
	float r, float g, float b, float a,
	float depth, uint32_t stencil) {

	X = x;
	Y = y;
	W = w;
	H = h;

	R = r;
	G = g;
	B = b;
	A = a;

	Depth = depth;
	Stencil = stencil;

	// Main subpass
	vk::SubpassDescription Subpass;
	Subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	// Attachments TODO: make this configurable.
	uint32_t AttachmentDescriptionCount = 2;
	std::vector<vk::AttachmentDescription> AttachmentDescriptions(AttachmentDescriptionCount);

	// Color attachment
	vk::AttachmentDescription ColorAttachment;
	ColorAttachment.setFormat(context->Swapchain.ImageFormat.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	AttachmentDescriptions[0] = ColorAttachment;

	// Color attachment reference
	vk::AttachmentReference ColorAttachmentReference;
	ColorAttachmentReference.setAttachment(0);
	ColorAttachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	Subpass.setColorAttachmentCount(1);
	Subpass.setPColorAttachments(&ColorAttachmentReference);

	// Depth attachment, if there is one
	vk::AttachmentDescription DepthAttachment;
	DepthAttachment.setFormat(context->Device.GetDepthFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	AttachmentDescriptions[1] = DepthAttachment;

	// Depth attachment reference
	vk::AttachmentReference DepthAttachmentReference;
	DepthAttachmentReference.setAttachment(1);
	DepthAttachmentReference.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	// TODO: other attachment types (input, resolve, preserve)

	// Depth stencil data
	Subpass.setPDepthStencilAttachment(&DepthAttachmentReference);

	// Input from a shader
	Subpass.setInputAttachmentCount(0);
	Subpass.setPInputAttachments(nullptr);

	// Attachments used for mutisampling color attachments
	Subpass.setPResolveAttachments(nullptr);

	// Attachments not used for in this subpass, but must be preserved for the next
	Subpass.setPreserveAttachmentCount(0);
	Subpass.setPPreserveAttachments(nullptr);

	// Render pass dependencies. TODO: make this configurable
	vk::SubpassDependency Dependency;
	Dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eNone)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
		.setDependencyFlags(vk::DependencyFlagBits::eByRegion);


	// Render pass create
	vk::RenderPassCreateInfo CreateInfo;
	CreateInfo.setAttachmentCount(AttachmentDescriptionCount)
		.setAttachments(AttachmentDescriptions)
		.setSubpassCount(1)
		.setPSubpasses(&Subpass)
		.setDependencyCount(1)
		.setDependencies(Dependency)
		.setPNext(nullptr);

	RenderPass = context->Device.GetLogicalDevice().createRenderPass(CreateInfo, context->Allocator);
	ASSERT(RenderPass);
}

void VulkanRenderPass::Destroy(VulkanContext* context) {
	if (RenderPass) {
		context->Device.GetLogicalDevice().destroyRenderPass(RenderPass, context->Allocator);
	}
}

void VulkanRenderPass::Begin(VulkanCommandBuffer* command_buffer, vk::Framebuffer frame_buffer) {
	vk::Rect2D Area;
	Area.setOffset({ (int32_t)X, (int32_t)Y })
		.setExtent({ (uint32_t)W, (uint32_t)H });

	vk::RenderPassBeginInfo BeginInfo;
	BeginInfo.setRenderPass(RenderPass)
		.setFramebuffer(frame_buffer)
		.setRenderArea(Area);

	std::array<vk::ClearValue,2> ClearValue;
	ClearValue[0].color.float32[0] = R;
	ClearValue[0].color.float32[1] = G;
	ClearValue[0].color.float32[2] = B;
	ClearValue[0].color.float32[3] = A;
	ClearValue[1].depthStencil.depth = Depth;
	ClearValue[1].depthStencil.stencil = Stencil;

	BeginInfo.setClearValueCount((uint32_t)ClearValue.size())
		.setClearValues(ClearValue);

	command_buffer->CommandBuffer.beginRenderPass(BeginInfo, vk::SubpassContents::eInline);
	command_buffer->State = VulkanCommandBufferState::eCommand_Buffer_State_In_Renderpass;
}

void VulkanRenderPass::End(VulkanCommandBuffer* command_buffer) {
	command_buffer->CommandBuffer.endRenderPass();
	command_buffer->State = VulkanCommandBufferState::eCommand_Buffer_State_Recording;
}

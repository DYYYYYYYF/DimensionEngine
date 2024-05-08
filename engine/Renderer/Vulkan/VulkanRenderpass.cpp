#include "VulkanRenderpass.hpp"

#include "VulkanContext.hpp"
#include "VulkanCommandBuffer.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void VulkanRenderPass::Create(VulkanContext* context,
	Vec4 render_area, Vec4 clear_color,
	float depth, uint32_t stencil,
	int clear_flags,
	bool has_prev_pass, bool has_next_pass) {

	ClearFlags = clear_flags;
	ClearColor = clear_color;
	RenderArea = render_area;
	HasPrevPass = has_prev_pass;
	HasNextPass = has_next_pass;

	Depth = depth;
	Stencil = stencil;

	// Main subpass
	vk::SubpassDescription Subpass;
	Subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	// Attachments TODO: make this configurable.
	uint32_t AttachmentDescriptionCount = 0;
	std::vector<vk::AttachmentDescription> AttachmentDescriptions;

	// Color attachment
	bool IsNeedClearColor = (ClearFlags & eRenderpass_Clear_Color_Buffer) != 0;
	vk::AttachmentDescription ColorAttachment;
	ColorAttachment.setFormat(context->Swapchain.ImageFormat.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(IsNeedClearColor ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	// If coming from a previous pass, should already be vk::ImageLayout::eColorAttachmentOptimal. Otherwise undefined.
	ColorAttachment.setInitialLayout(HasPrevPass ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eUndefined);
	// If going to another pass, use vk::ImageLayout::eColorAttachmentOptimal. Otherwise vk::ImageLayout::ePresentSrcKHR.
	ColorAttachment.setFinalLayout(HasNextPass ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR);

	AttachmentDescriptions.push_back(ColorAttachment);
	AttachmentDescriptionCount++;

	// Color attachment reference
	vk::AttachmentReference ColorAttachmentReference;
	ColorAttachmentReference.setAttachment(0);
	ColorAttachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	Subpass.setColorAttachmentCount(1);
	Subpass.setPColorAttachments(&ColorAttachmentReference);

	// Depth attachment, if there is one
	bool IsNeedClearDepth = (ClearFlags & eRenderpass_Clear_Depth_Buffer) != 0;
	
	// NOTE: Put out of block, it will release memory when release mode and cause unknown break!
	vk::AttachmentDescription DepthAttachment;
	vk::AttachmentReference DepthAttachmentReference;

	if (IsNeedClearDepth) {
		DepthAttachment.setFormat(context->Device.GetDepthFormat())
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(IsNeedClearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		AttachmentDescriptions.push_back(DepthAttachment);
		AttachmentDescriptionCount++;

		// Depth attachment reference
		DepthAttachmentReference.setAttachment(1);
		DepthAttachmentReference.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		// TODO: other attachment types (input, resolve, preserve)

		// Depth stencil data
		Subpass.setPDepthStencilAttachment(&DepthAttachmentReference);
	}
	else {
		// Memory::Zero(&AttachmentDescriptions[AttachmentDescriptionCount], sizeof(vk::AttachmentDescription));
		Subpass.setPDepthStencilAttachment(nullptr);
	}
	
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
	Area.setOffset({ (int32_t)RenderArea.x, (int32_t)RenderArea.y })
		.setExtent({ (uint32_t)RenderArea.z, (uint32_t)RenderArea.w });

	vk::RenderPassBeginInfo BeginInfo;
	BeginInfo.setRenderPass(RenderPass)
		.setFramebuffer(frame_buffer)
		.setRenderArea(Area);

	BeginInfo.setClearValueCount(0);
	BeginInfo.setPClearValues(nullptr);

	vk::ClearValue ClearValues[2];
	Memory::Zero(ClearValues, sizeof(vk::ClearValue) * 2);
	bool IsNeedClearColor = (ClearFlags & eRenderpass_Clear_Color_Buffer) != 0;
	if (IsNeedClearColor) {
		Memory::Copy(ClearValues[BeginInfo.clearValueCount].color.float32, ClearColor.elements, sizeof(float) * 4);
		BeginInfo.clearValueCount++;
	}

	bool IsNeedClearDepth = (ClearFlags & eRenderpass_Clear_Depth_Buffer) != 0;
	if (IsNeedClearDepth) {
		Memory::Copy(ClearValues[BeginInfo.clearValueCount].color.float32, ClearColor.elements, sizeof(float) * 4);
		ClearValues[BeginInfo.clearValueCount].depthStencil.depth = Depth;

		bool IsNeedClearStencil = (ClearFlags & eRenderpass_Clear_Stencil_Buffer) != 0;
		ClearValues[BeginInfo.clearValueCount].depthStencil.stencil = IsNeedClearStencil ? Stencil : 0;
		BeginInfo.clearValueCount++;
	}

	BeginInfo.setPClearValues(BeginInfo.clearValueCount > 0 ? ClearValues : nullptr);

	command_buffer->CommandBuffer.beginRenderPass(BeginInfo, vk::SubpassContents::eInline);
	command_buffer->State = VulkanCommandBufferState::eCommand_Buffer_State_In_Renderpass;
}

void VulkanRenderPass::End(VulkanCommandBuffer* command_buffer) {
	command_buffer->CommandBuffer.endRenderPass();
	command_buffer->State = VulkanCommandBufferState::eCommand_Buffer_State_Recording;
}

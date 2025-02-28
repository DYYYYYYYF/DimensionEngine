#include "VulkanRenderpass.hpp"

#include "VulkanContext.hpp"
#include "VulkanCommandBuffer.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

bool VulkanRenderPass::Create(VulkanContext* context, const RenderpassConfig& config) {

	Depth = config.depth;
	Stencil = config.stencil;
	Context = context;

	// Main subpass
	vk::SubpassDescription Subpass;
	Subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	// Attachments TODO:
	uint32_t AttachmentDescriptionCount = 0;
	std::vector<vk::AttachmentDescription> AttachmentDescriptions;
	std::vector<vk::AttachmentDescription> ColorAttachmentDescriptions;
	std::vector<vk::AttachmentDescription> DepthAttachmentDescriptions;

	// Can always just look at the first target since they are all the same(one per frame).
	// render target* taget = &Targets[0]
	vk::AttachmentDescription AttachmentDesc;
	for (uint32_t i = 0; i < config.target.attachments.size(); ++i) {
		const RenderTargetAttachmentConfig* AttachmentConfig = &config.target.attachments[i];
		if (AttachmentConfig->type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color) {
			// Color attachment
			bool IsNeedClearColor = (ClearFlags & eRenderpass_Clear_Color_Buffer) != 0;

			if (AttachmentConfig->source == RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default) {
				AttachmentDesc.setFormat(context->Swapchain.ImageFormat.format);
			}
			else {
				// TODO: configurable
				AttachmentDesc.setFormat(vk::Format::eR8G8B8A8Unorm);
			}

			AttachmentDesc.setSamples(vk::SampleCountFlagBits::e1);

			// Determine which load operation to use.
			if (AttachmentConfig->loadOperation == RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_DontCare) {
				// We dont care, they only other thing that needs checking is if the attachment is being cleared.
				AttachmentDesc.setLoadOp(IsNeedClearColor ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare);
			}
			else {
				// If we loading, check if we are also clearing. This combination doesn't make sense, and should be warned about.
				if (AttachmentConfig->loadOperation == RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load) {
					if (IsNeedClearColor) {
						LOG_WARN("Color attachment load operation set to load, but is also set to clear. This combination is invalid.");
						AttachmentDesc.setLoadOp(vk::AttachmentLoadOp::eClear);
					}
					else {
						AttachmentDesc.setLoadOp(vk::AttachmentLoadOp::eLoad);
					}
				}
				else {
					LOG_FATAL("Invalid and unsupported combination of load operation (0x%x) and clear flags (0x%x) for color attachment.", AttachmentDesc.loadOp, ClearFlags);
					return false;
				}
			}

			// Determine which store operation to use.
			if (AttachmentConfig->storeOperation == RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_DontCare) {
				AttachmentDesc.setStoreOp(vk::AttachmentStoreOp::eDontCare);
			}
			else if (AttachmentConfig->storeOperation == RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store) {
				AttachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
			}
			else {
				LOG_FATAL("Invalid store operation  (0x%x) for depth attachment. Check onfiguration.", AttachmentConfig->storeOperation);
				return false;
			}

			// NOTE: These will never be used on a color attachment.
			AttachmentDesc.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
			AttachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
			
			// If loading, that means coming from another pass, meaning the format should be  vk::ImageLayout::eColorAttachmentOptimal.
			AttachmentDesc.setInitialLayout(AttachmentConfig->loadOperation ==
				RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eUndefined);
			// If this is the last pass writing to this attachment, present after should be set to true.
			AttachmentDesc.setFinalLayout(AttachmentConfig->presentAfter ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eColorAttachmentOptimal);

			// Push to co
			ColorAttachmentDescriptions.push_back(AttachmentDesc);
		}	// Color attachment
		else if (AttachmentConfig->type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth) {
			// Depth attachment
			bool IsNeedClearDepth = (ClearFlags & eRenderpass_Clear_Depth_Buffer) != 0;

			if (AttachmentConfig->source == RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default) {
				AttachmentDesc.setFormat(Context->Device.GetDepthFormat());
			}
			else {
				// TODO: There may be a more optimal format to use when not the default depth target.
				AttachmentDesc.setFormat(Context->Device.GetDepthFormat());
			}

			AttachmentDesc.setSamples(vk::SampleCountFlagBits::e1);
			// Determine which load operation to use.
			if (AttachmentConfig->loadOperation == RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_DontCare) {
				// If we don't care, they only other thing that needs checking is if the attachment is being cleared.
				AttachmentDesc.setLoadOp(IsNeedClearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare);
			}
			else {
				// If we loading, check if we are also clearing. This combination doesn't make sense, and should be warned about.
				if (AttachmentConfig->loadOperation == RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load) {
					if (IsNeedClearDepth) {
						LOG_WARN("Depth attachment load operation set to load, but is also set to clear. This combination is invalid.");
						AttachmentDesc.setLoadOp(vk::AttachmentLoadOp::eClear);
					}
					else {
						AttachmentDesc.setLoadOp(vk::AttachmentLoadOp::eLoad);
					}
				}
				else {
					LOG_FATAL("Invalid and unsupported combination of load operation (0x%x) and clear flags (0x%x) for depth attachment.", AttachmentDesc.loadOp, ClearFlags);
					return false;
				}
			}

			// Determine which store operation to use.
			if (AttachmentConfig->storeOperation == RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_DontCare) {
				AttachmentDesc.setStoreOp(vk::AttachmentStoreOp::eDontCare);
			}
			else if (AttachmentConfig->storeOperation == RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store) {
				AttachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
			}
			else {
				LOG_FATAL("Invalid store operation  (0x%x) for depth attachment. Check onfiguration.", AttachmentConfig->storeOperation);
				return false;
			}

			// TODO: Configurable for stencil attachments.
			AttachmentDesc.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
			AttachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

			// If loading, that means coming from another pass, meaning the format should be  vk::ImageLayout::eDepthStencilAttachmentOptimal.
			AttachmentDesc.setInitialLayout(AttachmentConfig->loadOperation ==
				RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eUndefined);
			// Final layout for depth stencil attachments is always this.
			AttachmentDesc.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

			// Push to co
			DepthAttachmentDescriptions.push_back(AttachmentDesc);
		}// Depth attachment

		AttachmentDescriptions.push_back(AttachmentDesc);
	}

	// Setup the attachment references.
	uint32_t AttachmentsAdded = 0;
	// Color attachment reference.
	std::vector<vk::AttachmentReference> ColorAttachmentReferences;
	uint32_t ColorAttachmentCount = (uint32_t)ColorAttachmentDescriptions.size();
	if (ColorAttachmentCount > 0) {
		ColorAttachmentReferences.resize(ColorAttachmentCount);
		for (uint32_t i = 0; i < ColorAttachmentCount; ++i) {
			ColorAttachmentReferences[i].setAttachment(AttachmentsAdded);	// Attachment description array index
			ColorAttachmentReferences[i].setLayout(vk::ImageLayout::eColorAttachmentOptimal);
			AttachmentsAdded++;
		}

		Subpass.setColorAttachmentCount(ColorAttachmentCount);
		Subpass.setColorAttachments(ColorAttachmentReferences);
	}
	else {
		Subpass.setColorAttachmentCount(0);
		Subpass.setColorAttachments(nullptr);
	}

	// Depth attachment reference.
	std::vector<vk::AttachmentReference> DepthAttachmentReferences;
	uint32_t DepthAttachmentCount = (uint32_t)DepthAttachmentDescriptions.size();
	if (DepthAttachmentCount > 0) {
		ASSERT(DepthAttachmentCount == 1);	// Multiple depth attachments not supported.
		DepthAttachmentReferences.resize(DepthAttachmentCount);
		for (uint32_t i = 0; i < DepthAttachmentCount; ++i) {
			DepthAttachmentReferences[i].setAttachment(AttachmentsAdded);
			DepthAttachmentReferences[i].setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
			AttachmentsAdded++;
		}

		// Depth stencil data.
		Subpass.setPDepthStencilAttachment(DepthAttachmentReferences.data());
	}
	else {
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
	CreateInfo.setAttachmentCount((uint32_t)AttachmentDescriptions.size())
		.setAttachments(AttachmentDescriptions)
		.setSubpassCount(1)
		.setPSubpasses(&Subpass)
		.setDependencyCount(1)
		.setPDependencies(&Dependency)
		.setPNext(nullptr);

	if (context->Device.GetLogicalDevice().createRenderPass(&CreateInfo,
		context->Allocator, (vk::RenderPass*)&Renderpass) != vk::Result::eSuccess) {
		LOG_ERROR("VulkanRenderPass::Create() Failed to create renderpass.");
	}

	return true;
}

void VulkanRenderPass::Destroy() {
	if (Renderpass) {
		Context->Device.GetLogicalDevice().destroyRenderPass(GetRenderPass(), Context->Allocator);
		Renderpass = nullptr;
	}
}

void VulkanRenderPass::Begin(RenderTarget* target) {
	VulkanCommandBuffer* CmdBuffer = &Context->GraphicsCommandBuffers[Context->ImageIndex];

	vk::Rect2D Area;
	Area.setOffset({ (int32_t)RenderArea.x, (int32_t)RenderArea.y })
		.setExtent({ (uint32_t)RenderArea.z, (uint32_t)RenderArea.w });

	vk::RenderPassBeginInfo BeginInfo;
	BeginInfo.setRenderPass(*(vk::RenderPass*)&Renderpass)
		.setFramebuffer(*((vk::Framebuffer*)&target->internal_framebuffer))
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
	else {
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
	else {
		uint32_t AttachCount = (uint32_t)target->attachments.size();
		for (uint32_t i = 0; i < AttachCount; ++i) {
			if (target->attachments[i].type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth) {
				// If there is a depth attachment, make sure to add the clear count, but dont bother copying the data.
				BeginInfo.clearValueCount++;
			}
		}
	}

	BeginInfo.setPClearValues(BeginInfo.clearValueCount > 0 ? ClearValues : nullptr);

	CmdBuffer->CommandBuffer.beginRenderPass(BeginInfo, vk::SubpassContents::eInline);
	CmdBuffer->State = VulkanCommandBufferState::eCommand_Buffer_State_In_Renderpass;
}

void VulkanRenderPass::End() {
	VulkanCommandBuffer* CmdBuffer = &Context->GraphicsCommandBuffers[Context->ImageIndex];
	CmdBuffer->CommandBuffer.endRenderPass();
	CmdBuffer->State = VulkanCommandBufferState::eCommand_Buffer_State_Recording;
}

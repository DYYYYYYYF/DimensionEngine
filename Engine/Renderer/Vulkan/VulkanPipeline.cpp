#include "VulkanPipeline.hpp"
#include "VulkanContext.hpp"

#include "Core/EngineLogger.hpp"
#include "Math/MathTypes.hpp"

bool VulkanPipeline::Create(VulkanContext* context, VulkanRenderPass* renderpass, uint32_t stride,
	uint32_t attribute_count, vk::VertexInputAttributeDescription* attributes,
	uint32_t descriptor_set_layout_count, vk::DescriptorSetLayout* descriptor_set_layout,
	uint32_t stage_count, vk::PipelineShaderStageCreateInfo* stages,
	vk::Viewport viewport, vk::Rect2D scissor, FaceCullMode cull_mode,
	bool is_wireframe, bool depth_test_enabled,
	uint32_t push_constant_range_count, Range* push_constant_ranges){
	// Viewport state
	vk::PipelineViewportStateCreateInfo ViewportState;
	ViewportState.setViewportCount(1)
		.setPViewports(&viewport)
		.setScissorCount(1)
		.setPScissors(&scissor);

	// Rasterizer
	vk::PipelineRasterizationStateCreateInfo RasterizerCreateInfo;
	RasterizerCreateInfo.setDepthClampEnable(VK_FALSE)
		.setRasterizerDiscardEnable(VK_FALSE)
		.setPolygonMode(is_wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill)
		.setLineWidth(1.0f)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(VK_FALSE)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f);
	switch (cull_mode)
	{
	case eFace_Cull_Mode_None:
		RasterizerCreateInfo.setCullMode(vk::CullModeFlagBits::eNone);
		break;
	case eFace_Cull_Mode_Front:
		RasterizerCreateInfo.setCullMode(vk::CullModeFlagBits::eFront);
		break;
	case eFace_Cull_Mode_Back:
		RasterizerCreateInfo.setCullMode(vk::CullModeFlagBits::eBack);
		break;
	case eFace_Cull_Mode_Front_And_Back:
		RasterizerCreateInfo.setCullMode(vk::CullModeFlagBits::eFrontAndBack);
		break;
	}

	// Multisampling
	vk::PipelineMultisampleStateCreateInfo MultisamplingCreateInfo;
	MultisamplingCreateInfo.setSampleShadingEnable(VK_FALSE)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setMinSampleShading(1.0f)
		.setPSampleMask(nullptr)
		.setAlphaToCoverageEnable(VK_FALSE)
		.setAlphaToOneEnable(VK_FALSE);

	// Depth and stencil testing
	vk::PipelineDepthStencilStateCreateInfo DepthStencil;
	if (depth_test_enabled) {
		DepthStencil.setDepthTestEnable(VK_TRUE)
			.setDepthWriteEnable(VK_TRUE)
			.setDepthCompareOp(vk::CompareOp::eLess)
			.setDepthBoundsTestEnable(VK_FALSE)
			.setStencilTestEnable(VK_FALSE);
	}

	vk::PipelineColorBlendAttachmentState ColorBlendAttachmentState;
	ColorBlendAttachmentState.setBlendEnable(VK_TRUE)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorWriteMask(
			vk::ColorComponentFlagBits::eR | 
			vk::ColorComponentFlagBits::eG | 
			vk::ColorComponentFlagBits::eB | 
			vk::ColorComponentFlagBits::eA
		);

	vk::PipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo;
	ColorBlendStateCreateInfo.setLogicOpEnable(VK_FALSE)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachmentCount(1)
		.setPAttachments(&ColorBlendAttachmentState);

	// Dynamic state
	const uint32_t DynamicStateCount = 3;
	vk::DynamicState DynamicStates[DynamicStateCount] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
		vk::DynamicState::eLineWidth
	};

	vk::PipelineDynamicStateCreateInfo DynamicStateCreateInfo;
	DynamicStateCreateInfo.setDynamicStateCount(DynamicStateCount)
		.setPDynamicStates(DynamicStates);

	// Vertex input
	vk::VertexInputBindingDescription BindingDescription;
	BindingDescription.setBinding(0)
		.setStride(stride)
		.setInputRate(vk::VertexInputRate::eVertex);

	// Attributes
	vk::PipelineVertexInputStateCreateInfo VertexInputInfo;
	VertexInputInfo.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&BindingDescription)
		.setVertexAttributeDescriptionCount(attribute_count)
		.setPVertexAttributeDescriptions(attributes);

	// Input assembly
	vk::PipelineInputAssemblyStateCreateInfo InputAssembly;
	InputAssembly.setPrimitiveRestartEnable(VK_FALSE)
		.setTopology(vk::PrimitiveTopology::eTriangleList);

	//Push constants
	vk::PipelineLayoutCreateInfo LayoutCreateInfo;
	vk::PushConstantRange Ranges[32];
	if (push_constant_range_count > 0) {
		if (push_constant_range_count > 32) {
			LOG_ERROR("Vulkan graphics pipeline create: can not have more push constants.");
			return false;
		}

		// NOTE: 32 is the max number of ranges we can ever have.
		Memory::Zero(Ranges, sizeof(vk::PushConstantRange) * 32);
		for(uint32_t i = 0; i < push_constant_range_count; ++i) {
			Ranges[i].setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
				.setOffset((uint32_t)push_constant_ranges[i].offset)
				.setSize((uint32_t)push_constant_ranges[i].size);
		}

		LayoutCreateInfo.setPushConstantRangeCount(push_constant_range_count)
			.setPPushConstantRanges(Ranges);
	}
	else {
		LayoutCreateInfo.setPushConstantRangeCount(0)
			.setPPushConstantRanges(nullptr);
	}

	// Pipeline layout
	LayoutCreateInfo.setSetLayoutCount(descriptor_set_layout_count)
		.setPSetLayouts(descriptor_set_layout);

	// Create pipeline layout
	PipelineLayout = context->Device.GetLogicalDevice().createPipelineLayout(LayoutCreateInfo, context->Allocator);
	ASSERT(PipelineLayout);

	// Create the pipeline
	vk::GraphicsPipelineCreateInfo PipelineCreateInfo;
	PipelineCreateInfo.setStageCount(stage_count)
		.setPStages(stages)
		.setPVertexInputState(&VertexInputInfo)
		.setPInputAssemblyState(&InputAssembly)

		.setPViewportState(&ViewportState)
		.setPRasterizationState(&RasterizerCreateInfo)
		.setPMultisampleState(&MultisamplingCreateInfo)
		.setPDepthStencilState(depth_test_enabled ? &DepthStencil : nullptr)
		.setPColorBlendState(&ColorBlendStateCreateInfo)
		.setPDynamicState(&DynamicStateCreateInfo)
		.setPTessellationState(nullptr)

		.setLayout(PipelineLayout)

		.setRenderPass(renderpass->GetRenderPass())
		.setSubpass(0)
		.setBasePipelineHandle(nullptr)
		.setBasePipelineIndex(-1);

	vk::ResultValue<vk::Pipeline> Result = context->Device.GetLogicalDevice()
		.createGraphicsPipeline(VK_NULL_HANDLE, PipelineCreateInfo, context->Allocator);
	if (Result.result != vk::Result::eSuccess) {
		LOG_ERROR("Create graphics pipelines failed.");
		return false;
	}

	Handle = Result.value;
	ASSERT(Handle);

	LOG_DEBUG("Graphics pipeline created.");
	return true;
}

void VulkanPipeline::Destroy(VulkanContext* context) {
	// Destroy pipeline
	if (Handle) {
		context->Device.GetLogicalDevice().destroyPipeline(Handle, context->Allocator);
		Handle = nullptr;
	}

	// Destroy layout
	if (PipelineLayout) {
		context->Device.GetLogicalDevice().destroyPipelineLayout(PipelineLayout, context->Allocator);
		PipelineLayout = nullptr;
	}
}

void VulkanPipeline::Bind(VulkanCommandBuffer* command_buffer, vk::PipelineBindPoint bind_point) {
	command_buffer->CommandBuffer.bindPipeline(bind_point, Handle);
}
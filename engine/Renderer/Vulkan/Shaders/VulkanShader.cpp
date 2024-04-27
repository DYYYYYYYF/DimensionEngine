#include "VulkanShader.hpp"

#include "Math/MathTypes.hpp"
#include "Renderer/Vulkan/VulkanContext.hpp"
#include "Renderer/Vulkan/VulkanShaderUtils.hpp"

bool VulkanShaderModule::Create(VulkanContext* context) {
	// Shader module init per stage
	char StageTypeStrs[OBJECT_SHADER_STAGE_COUNT][5] = { "vert", "frag" };
	vk::ShaderStageFlagBits StageTypes[OBJECT_SHADER_STAGE_COUNT] = { vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment };

	for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
		if (!VulkanShaderUtils::CreateShaderModule(context, "default", StageTypeStrs[i], StageTypes[i], i, Stages)) {
			UL_ERROR("Unable to create %s shader module for '%s'", StageTypeStrs[i], "Test");
			return false;
		}
	}

	// TODO: Descriptors

	// Pipeline creation
	vk::Viewport Viewport;
	Viewport.setX(0.0f)
		.setY(0.0f)
		.setWidth((float)context->FrameBufferWidth)
		.setHeight((float)context->FrameBufferHeight)
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	// Scissor
	vk::Rect2D Scissor;
	Scissor.setOffset({ 0, 0 })
		.setExtent({ context->FrameBufferWidth, context->FrameBufferHeight });

	// Attributes
	size_t Offset = 0;
	const int AttributeCount = 1;
	vk::VertexInputAttributeDescription AttributeDescriptions[AttributeCount];
	// Position
	vk::Format Formats[AttributeCount] = {
		vk::Format::eR32G32B32Sfloat
	};
	size_t Sizes[AttributeCount] = {
		sizeof(Vec3)
	};

	for (uint32_t i = 0; i < AttributeCount; ++i) {
		AttributeDescriptions[i].setBinding(0);
		AttributeDescriptions[i].setLocation(i);
		AttributeDescriptions[i].setFormat(Formats[i]);
		AttributeDescriptions[i].setOffset((uint32_t)Offset);
		Offset += Sizes[i];
	}

	// TODO: Descriptor set layouts

	// Stages
	vk::PipelineShaderStageCreateInfo StageCreateInfos[OBJECT_SHADER_STAGE_COUNT];
	Memory::Zero(StageCreateInfos, sizeof(StageCreateInfos));
	for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
		StageCreateInfos[i].sType = Stages[i].shader_stage_create_info.sType;
		StageCreateInfos[i] = Stages[i].shader_stage_create_info;
	}

	if (!Pipeline.Create(context, &context->MainRenderPass, AttributeCount, AttributeDescriptions, 0, 0,
		OBJECT_SHADER_STAGE_COUNT, StageCreateInfos, Viewport, Scissor, false)) {
		UL_ERROR("Load graphics pipeline for object shader failed.");
		return false;
	}

	return true;
}

void VulkanShaderModule::Destroy(VulkanContext* context) {
	Pipeline.Destroy(context);

	// Destroy shader modules
	vk::Device LogicalDevice = context->Device.GetLogicalDevice();
	for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++) {
		LogicalDevice.destroyShaderModule(Stages[i].shader_module, context->Allocator);
		Stages[i].shader_module = nullptr;
	}
}

void VulkanShaderModule::Use(VulkanContext* context) {
	uint32_t ImageIndex = context->ImageIndex;
	Pipeline.Bind(&context->GraphicsCommandBuffers[ImageIndex], vk::PipelineBindPoint::eGraphics);
}

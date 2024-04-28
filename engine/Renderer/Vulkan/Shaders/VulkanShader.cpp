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

	// Global descriptors
	vk::DescriptorSetLayoutBinding GlobalUboLayoutBinding;
	GlobalUboLayoutBinding.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPImmutableSamplers(nullptr)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	vk::DescriptorSetLayoutCreateInfo GlobalLayoutCreateInfo;
	GlobalLayoutCreateInfo.setBindingCount(1)
		.setPBindings(&GlobalUboLayoutBinding);
	GlobalDescriptorSetLayout = context->Device.GetLogicalDevice().createDescriptorSetLayout(GlobalLayoutCreateInfo, context->Allocator);
	ASSERT(GlobalDescriptorSetLayout);

	// Global descriptor pool: used for global items such as view/projection matrix.
	vk::DescriptorPoolSize GlobalPoolSize;
	GlobalPoolSize.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(context->Swapchain.ImageCount);

	vk::DescriptorPoolCreateInfo GlobalDescriptorPoolInfo;
	GlobalDescriptorPoolInfo.setPoolSizeCount(1)
		.setPPoolSizes(&GlobalPoolSize)
		.setMaxSets(context->Swapchain.ImageCount);
	GlobalDescriptorPool = context->Device.GetLogicalDevice().createDescriptorPool(GlobalDescriptorPoolInfo, context->Allocator);
	ASSERT(GlobalDescriptorPool);

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

	// Descriptor set layouts
	const int DescriptorSetLayoutCount = 1;
	vk::DescriptorSetLayout Layouts[1] = {
		GlobalDescriptorSetLayout
	};

	// Stages
	vk::PipelineShaderStageCreateInfo StageCreateInfos[OBJECT_SHADER_STAGE_COUNT];
	Memory::Zero(StageCreateInfos, sizeof(StageCreateInfos));
	for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
		StageCreateInfos[i].sType = Stages[i].shader_stage_create_info.sType;
		StageCreateInfos[i] = Stages[i].shader_stage_create_info;
	}

	if (!Pipeline.Create(context, &context->MainRenderPass, AttributeCount, AttributeDescriptions, DescriptorSetLayoutCount, 
		Layouts, OBJECT_SHADER_STAGE_COUNT, StageCreateInfos, Viewport, Scissor, false)) {
		UL_ERROR("Load graphics pipeline for object shader failed.");
		return false;
	}

	// Create uniform buffer
	vk::DeviceSize DynamicAlignment = sizeof(SGlobalUBO);
	vk::PhysicalDeviceProperties properties = context->Device.GetPhysicalDevice().getProperties();
	size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
	if (minUboAlignment > 0) {
		DynamicAlignment = (DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	if (!GlobalUniformBuffer.Create(context, DynamicAlignment * 3,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		true)) {
		UL_ERROR("Create UBO failed.");
		return false;
	}

	// Allocate global descriptor sets.
	vk::DescriptorSetLayout GlobalLayouts[3] = {
		GlobalDescriptorSetLayout,
		GlobalDescriptorSetLayout,
		GlobalDescriptorSetLayout
	};

	vk::DescriptorSetAllocateInfo AllocateInfo;
	AllocateInfo.setDescriptorPool(GlobalDescriptorPool)
		.setDescriptorSetCount(3)
		.setPSetLayouts(GlobalLayouts);
	if (context->Device.GetLogicalDevice().allocateDescriptorSets(&AllocateInfo, GlobalDescriptorSets) != vk::Result::eSuccess) {
		UL_ERROR("Create global descriptor sets failed.");
		return false;
	 }

	return true;
}

void VulkanShaderModule::Destroy(VulkanContext* context) {
	// Destroy UBO buffer
	GlobalUniformBuffer.Destroy(context);

	// Destroy pipeline
	Pipeline.Destroy(context);

	// Destroy global descriptor pool.
	context->Device.GetLogicalDevice().destroyDescriptorPool(GlobalDescriptorPool, context->Allocator);

	// Destroy descriptor set layouts.
	context->Device.GetLogicalDevice().destroyDescriptorSetLayout(GlobalDescriptorSetLayout, context->Allocator);

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

void VulkanShaderModule::UpdateGlobalState(VulkanContext* context) {
	uint32_t ImageIndex = context->ImageIndex;
	vk::CommandBuffer CmdBuffer = context->GraphicsCommandBuffers[ImageIndex].CommandBuffer;
	vk::DescriptorSet GlobalDescriptor = GlobalDescriptorSets[ImageIndex];

	//if (!DescriptorUpdated[ImageIndex]) {
		// Configure the descriptors for the given index.
		vk::DeviceSize DynamicAlignment = sizeof(SGlobalUBO);
		vk::PhysicalDeviceProperties properties = context->Device.GetPhysicalDevice().getProperties();
		size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
		if (minUboAlignment > 0) {
			DynamicAlignment = (DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		vk::DeviceSize Range = DynamicAlignment;
		vk::DeviceSize Offset = DynamicAlignment * ImageIndex;

		// Copy data to buffer
		GlobalUniformBuffer.LoadData(context, Offset, Range, vk::MemoryMapFlags(), &GlobalUBO);

		vk::DescriptorBufferInfo BufferInfo;
		BufferInfo.setBuffer(GlobalUniformBuffer.Buffer)
			.setOffset(Offset)
			.setRange(Range);

		// Update descriptor sets.
		vk::WriteDescriptorSet DescriptorWrite;
		DescriptorWrite.setDstSet(GlobalDescriptorSets[ImageIndex])
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setPBufferInfo(&BufferInfo);

		context->Device.GetLogicalDevice().updateDescriptorSets(1, &DescriptorWrite, 0, nullptr);
		DescriptorUpdated[ImageIndex] = true;
	//}

	CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, 0);
}
	
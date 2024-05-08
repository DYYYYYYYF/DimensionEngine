#include "VulkanUIShader.hpp"

#include "Math/MathTypes.hpp"
#include "Renderer/Vulkan/VulkanContext.hpp"
#include "Renderer/Vulkan/VulkanShaderUtils.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Systems/TextureSystem.h"
#include "Systems/MaterialSystem.h"

#define SHADER_UI_NAME "ui_shader"

bool VulkanUIShader::Create(VulkanContext* context) {
	// Shader module init per stage
	char StageTypeStrs[UI_SHADER_STAGE_COUNT][5] = { "vert", "frag" };
	vk::ShaderStageFlagBits StageTypes[UI_SHADER_STAGE_COUNT] = { vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment };

	for (uint32_t i = 0; i < UI_SHADER_STAGE_COUNT; ++i) {
		if (!VulkanShaderUtils::CreateShaderModule(context, SHADER_UI_NAME, StageTypeStrs[i], StageTypes[i], i, Stages)) {
			UL_ERROR("Unable to create %s shader module for '%s'", StageTypeStrs[i], SHADER_UI_NAME);
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

	// Sampler uses.
	SamplerUsage[0] = TextureUsage::eTexture_Usage_Map_Diffuse;

	// Local/Object descriptors
	vk::DescriptorType DescriptorTypes[VULKAN_UI_SHADER_DESCRIPTOR_COUNT] = {
		vk::DescriptorType::eUniformBuffer,				// Binding 0 - Uniform buffer.
		vk::DescriptorType::eCombinedImageSampler		// Binding 1 - Diffuse sampler layout.
	};
	vk::DescriptorSetLayoutBinding Bindings[VULKAN_UI_SHADER_DESCRIPTOR_COUNT];
	Memory::Zero(Bindings, sizeof(vk::DescriptorSetLayoutBinding) * VULKAN_UI_SHADER_DESCRIPTOR_COUNT);
	for (uint32_t i = 0; i < VULKAN_UI_SHADER_DESCRIPTOR_COUNT; ++i) {
		Bindings[i].setBinding(i)
			.setDescriptorCount(1)
			.setDescriptorType(DescriptorTypes[i])
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	}

	vk::DescriptorSetLayoutCreateInfo LayoutInfo;
	LayoutInfo.setBindingCount(VULKAN_UI_SHADER_DESCRIPTOR_COUNT)
		.setPBindings(Bindings);
	ObjectDescriptorSetLayout = context->Device.GetLogicalDevice().createDescriptorSetLayout(LayoutInfo, context->Allocator);
	ASSERT(ObjectDescriptorSetLayout);

	// Local/Object descriptor pool: Used for object-specific items like diffuse color.
	vk::DescriptorPoolSize ObjectPoolSizes[2];
	// The first section will be used for uniform buffers.
	ObjectPoolSizes[0].setType(DescriptorTypes[0])
		.setDescriptorCount(VULKAN_MAX_UI_COUNT);
	ObjectPoolSizes[1].setType(DescriptorTypes[1])
		.setDescriptorCount(VULKAN_UI_SHADER_SAMPLER_COUNT * VULKAN_MAX_UI_COUNT);

	vk::DescriptorPoolCreateInfo ObjectPoolCreateInfo;
	ObjectPoolCreateInfo.setPoolSizeCount(2)
		.setPPoolSizes(ObjectPoolSizes)
		.setMaxSets(VULKAN_MAX_UI_COUNT)
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

	// Create object descriptor pool.
	ObjectDescriptorPool = context->Device.GetLogicalDevice().createDescriptorPool(ObjectPoolCreateInfo, context->Allocator);
	ASSERT(ObjectDescriptorPool);

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
#define ATTRIBUTE_COUNT 2
	size_t Offset = 0;
	vk::VertexInputAttributeDescription AttributeDescriptions[ATTRIBUTE_COUNT];
	// Position, Texcoord
	vk::Format Formats[ATTRIBUTE_COUNT] = {
		vk::Format::eR32G32Sfloat,
		vk::Format::eR32G32Sfloat
	};
	size_t Sizes[ATTRIBUTE_COUNT] = {
		sizeof(Vec2),
		sizeof(Vec2)
	};

	for (uint32_t i = 0; i < ATTRIBUTE_COUNT; ++i) {
		AttributeDescriptions[i].setBinding(0);
		AttributeDescriptions[i].setLocation(i);
		AttributeDescriptions[i].setFormat(Formats[i]);
		AttributeDescriptions[i].setOffset((uint32_t)Offset);
		Offset += Sizes[i];
	}

	// Descriptor set layouts
	const int DescriptorSetLayoutCount = 2;
	vk::DescriptorSetLayout Layouts[2] = {
		GlobalDescriptorSetLayout,
		ObjectDescriptorSetLayout
	};

	// Stages
	vk::PipelineShaderStageCreateInfo StageCreateInfos[UI_SHADER_STAGE_COUNT];
	Memory::Zero(StageCreateInfos, sizeof(StageCreateInfos));
	for (uint32_t i = 0; i < UI_SHADER_STAGE_COUNT; ++i) {
		StageCreateInfos[i].sType = Stages[i].shader_stage_create_info.sType;
		StageCreateInfos[i] = Stages[i].shader_stage_create_info;
	}

	if (!Pipeline.Create(context, &context->UIRenderPass, sizeof(Vertex2D), ATTRIBUTE_COUNT, AttributeDescriptions, DescriptorSetLayoutCount,
		Layouts, UI_SHADER_STAGE_COUNT, StageCreateInfos, Viewport, Scissor, false, false)) {
		UL_ERROR("Load graphics pipeline for object shader failed.");
		return false;
	}

	// Create uniform buffer
	vk::MemoryPropertyFlags DeviceLocalBits =
		context->Device.GetIsSupportDeviceLocalHostVisible() ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk::MemoryPropertyFlags(0);
	vk::DeviceSize DynamicAlignment = sizeof(UIShaderGlobalUbo);
	vk::PhysicalDeviceProperties properties = context->Device.GetPhysicalDevice().getProperties();
	size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
	if (minUboAlignment > 0) {
		DynamicAlignment = (DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	if (!GlobalUniformBuffer.Create(context, DynamicAlignment * VULKAN_MAX_UI_COUNT,
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

	// Create the object uniform buffer.
	if (!ObjectUniformBuffer.Create(context, sizeof(UIShaderInstanceUbo) * VULKAN_MAX_UI_COUNT,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		true)) {
		UL_ERROR("UI instance buffer create failed for shader.");
		return false;
	}

	return true;
}

void VulkanUIShader::Destroy(VulkanContext* context) {

	context->Device.GetLogicalDevice().destroyDescriptorPool(ObjectDescriptorPool, context->Allocator);
	context->Device.GetLogicalDevice().destroyDescriptorSetLayout(ObjectDescriptorSetLayout, context->Allocator);

	// Destroy UBO buffer
	GlobalUniformBuffer.Destroy(context);
	ObjectUniformBuffer.Destroy(context);

	// Destroy pipeline
	Pipeline.Destroy(context);

	// Destroy global descriptor pool.
	context->Device.GetLogicalDevice().destroyDescriptorPool(GlobalDescriptorPool, context->Allocator);

	// Destroy descriptor set layouts.
	context->Device.GetLogicalDevice().destroyDescriptorSetLayout(GlobalDescriptorSetLayout, context->Allocator);

	// Destroy shader modules
	vk::Device LogicalDevice = context->Device.GetLogicalDevice();
	for (uint32_t i = 0; i < UI_SHADER_STAGE_COUNT; i++) {
		LogicalDevice.destroyShaderModule(Stages[i].shader_module, context->Allocator);
		Stages[i].shader_module = nullptr;
	}
}

void VulkanUIShader::Use(VulkanContext* context) {
	uint32_t ImageIndex = context->ImageIndex;
	Pipeline.Bind(&context->GraphicsCommandBuffers[ImageIndex], vk::PipelineBindPoint::eGraphics);
}

void VulkanUIShader::UpdateGlobalState(VulkanContext* context, double delta_time) {
	uint32_t ImageIndex = context->ImageIndex;
	vk::CommandBuffer CmdBuffer = context->GraphicsCommandBuffers[ImageIndex].CommandBuffer;
	vk::DescriptorSet GlobalDescriptor = GlobalDescriptorSets[ImageIndex];

	// Configure the descriptors for the given index.
	vk::DeviceSize DynamicAlignment = sizeof(UIShaderGlobalUbo);
	vk::PhysicalDeviceProperties properties = context->Device.GetPhysicalDevice().getProperties();
	size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
	if (minUboAlignment > 0) {
		DynamicAlignment = (DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	vk::DeviceSize Range = DynamicAlignment;
	vk::DeviceSize Offset = DynamicAlignment * ImageIndex;

	// Copy data to buffer
	GlobalUniformBuffer.LoadData(context, Offset, Range, {}, &GlobalUBO);

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

	CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, 0);
}

void VulkanUIShader::SetModelMat(VulkanContext* context, Matrix4 model) {
	if (context != nullptr) {
		uint32_t ImageIndex = context->ImageIndex;
		vk::CommandBuffer CmdBuffer = context->GraphicsCommandBuffers[ImageIndex].CommandBuffer;
		CmdBuffer.pushConstants(Pipeline.PipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), &model);
	}
}

void VulkanUIShader::ApplyMaterial(VulkanContext* context, Material* material) {
	if (context != nullptr) {
		uint32_t ImageIndex = context->ImageIndex;
		vk::CommandBuffer CmdBuffer = context->GraphicsCommandBuffers[ImageIndex].CommandBuffer;

		// Obtain material data.
		VulkanUIShaderInstanceState* ObjectState = &InstanceStates[material->InternalId];
		vk::DescriptorSet ObjectDescriptorSet = ObjectState->descriptor_Sets[ImageIndex];

		// TODO: if needs update
		vk::WriteDescriptorSet DescriptorWrites[VULKAN_UI_SHADER_DESCRIPTOR_COUNT];
		Memory::Zero(DescriptorWrites, sizeof(vk::WriteDescriptorSet) * VULKAN_UI_SHADER_DESCRIPTOR_COUNT);
		uint32_t DescriptorCount = 0;
		uint32_t DescriptorIndex = 0;

		// Descriptor 0 - uniform buffer
		uint32_t Range = sizeof(UIShaderInstanceUbo);
		size_t Offset = sizeof(UIShaderInstanceUbo) * material->InternalId;
		vk::PhysicalDeviceProperties properties = context->Device.GetPhysicalDevice().getProperties();
		size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
		if (minUboAlignment > 0) {
			Offset = (Offset + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		UIShaderInstanceUbo InstanceUbo;

		// Get diffuse color from a material.
		InstanceUbo.diffuse_color = material->DiffuseColor;

		// Load the data into the buffer
		ObjectUniformBuffer.LoadData(context, Offset, Range, {}, &InstanceUbo);

		// Only do this if the descriptor has not yet been updated.
		uint32_t* GlobalUboGeneration = &ObjectState->descriptor_states[DescriptorIndex].generations[ImageIndex];
		if (*GlobalUboGeneration == INVALID_ID || *GlobalUboGeneration != material->Generation) {
			vk::DescriptorBufferInfo BufferInfo;
			BufferInfo.setBuffer(ObjectUniformBuffer.Buffer)
				.setOffset(Offset)
				.setRange(Range);

			vk::WriteDescriptorSet Descriptor;
			Descriptor.setDstSet(ObjectDescriptorSet)
				.setDstBinding(DescriptorIndex)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				.setPBufferInfo(&BufferInfo);

			DescriptorWrites[DescriptorCount] = Descriptor;
			DescriptorCount++;

			if (DescriptorCount > 0) {
				context->Device.GetLogicalDevice().updateDescriptorSets(DescriptorCount, DescriptorWrites, 0, nullptr);
			}

			// update the frame generation. In this case it is only needed once since this is a buffer.
			*GlobalUboGeneration = material->Generation;
		}

		DescriptorIndex++;

		// Sampler.
		const uint32_t SamplerCount = 1;
		vk::DescriptorImageInfo ImageInfos[1];
		for (uint32_t SamplerIndex = 0; SamplerIndex < SamplerCount; SamplerIndex++) {
			TextureUsage Usage = SamplerUsage[SamplerIndex];
			Texture* t = nullptr;
			switch (Usage)
			{
			case eTexture_Usage_Map_Diffuse:
				t = material->DiffuseMap.texture;
				break;
			default:
				UL_FATAL("Unable to bind sampler to unknown usage.");
				return;
			}

			uint32_t* DescriptorGeneration = &ObjectState->descriptor_states[DescriptorIndex].generations[ImageIndex];
			uint32_t* DescriptorId = &ObjectState->descriptor_states[DescriptorIndex].ids[ImageIndex];

			// If the texture hasn't been loaded yet, use the default.
			if (t->Generation == INVALID_ID) {
				t = TextureSystem::GetDefaultTexture();

				// Reset the descriptor generation if using the default texture.
				*DescriptorGeneration = INVALID_ID;
			}

			// Check if the descriptor needs updating first.
			if (t && (*DescriptorGeneration != t->Generation || *DescriptorGeneration == INVALID_ID)) {
				VulkanTexture* InternalData = (VulkanTexture*)t->InternalData;

				// Assign view and sampler.
				ImageInfos[SamplerIndex].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setImageView(InternalData->Image.ImageView)
					.setSampler(InternalData->sampler);

				vk::WriteDescriptorSet ObjectDescriptor;
				ObjectDescriptor.setDstSet(ObjectDescriptorSet)
					.setDstBinding(DescriptorIndex)
					.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
					.setDescriptorCount(1)
					.setPImageInfo(&ImageInfos[SamplerIndex]);

				DescriptorWrites[DescriptorCount] = ObjectDescriptor;
				DescriptorCount++;

				// Sync frame generation if not using a default texture.
				if (t->Generation != INVALID_ID) {
					*DescriptorGeneration = t->Generation;
				}
				DescriptorIndex++;
			}
		}

		if (DescriptorCount > 0) {
			context->Device.GetLogicalDevice().updateDescriptorSets(DescriptorCount, DescriptorWrites, 0, nullptr);
		}

		// Bind the descriptor set to be updated, or in case the shader changed.
		CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Pipeline.PipelineLayout, 1, 1, &ObjectDescriptorSet, 0, nullptr);
	}
}

bool VulkanUIShader::AcquireResources(VulkanContext* context, Material* material) {
	// TODO: free list
	material->InternalId = ObjectUniformBufferIndex;
	ObjectUniformBufferIndex++;

	VulkanUIShaderInstanceState* ObjectState = &InstanceStates[material->InternalId];
	for (uint32_t i = 0; i < VULKAN_UI_SHADER_DESCRIPTOR_COUNT; ++i) {
		for (uint32_t j = 0; j < 3; j++) {
			ObjectState->descriptor_states[i].generations[j] = INVALID_ID;
		}
	}

	// Allocate descriptor sets.
	vk::DescriptorSetLayout Layouts[3] = {
		ObjectDescriptorSetLayout,
		ObjectDescriptorSetLayout,
		ObjectDescriptorSetLayout
	};

	vk::DescriptorSetAllocateInfo AllocateInfo;
	AllocateInfo.setDescriptorPool(ObjectDescriptorPool)
		.setDescriptorSetCount(3)
		.setPSetLayouts(Layouts);

	if (context->Device.GetLogicalDevice().allocateDescriptorSets(&AllocateInfo, ObjectState->descriptor_Sets) != vk::Result::eSuccess) {
		UL_ERROR("Allocating descriptor sets in shader failed.");
		return false;
	}

	return true;
}

void VulkanUIShader::ReleaseResources(VulkanContext* context, Material* material) {
	VulkanUIShaderInstanceState* InstanceState = &InstanceStates[material->InternalId];

	const uint32_t DescriptorCount = 3;

	// Wait for any pending operations using the descriptor set to finish.
	context->Device.GetLogicalDevice().waitIdle();

	// Release object descriptor sets.
	context->Device.GetLogicalDevice().freeDescriptorSets(ObjectDescriptorPool, InstanceState->descriptor_Sets);

	for (uint32_t i = 0; i < VULKAN_UI_SHADER_DESCRIPTOR_COUNT; ++i) {
		for (uint32_t j = 0; j < 3; ++j) {
			InstanceState->descriptor_states[i].generations[j] = INVALID_ID;
			InstanceState->descriptor_states[i].ids[j] = INVALID_ID;
		}
	}

	material->InternalId = INVALID_ID;
	// TODO: add the object_id to the free list.
}
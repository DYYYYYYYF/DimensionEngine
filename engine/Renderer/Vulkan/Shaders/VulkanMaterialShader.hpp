#pragma once

#include <vulkan/vulkan.hpp>

#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Vulkan/VulkanPipeline.hpp"

#define OBJECT_SHADER_STAGE_COUNT 2
#define VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT 2

class VulkanContext;
class Texture;

struct VulkanDescriptorState {
	// One per frame
	uint32_t generations[3];
};

struct VulkanObjectShaderObjectState {
	// Per frame
	vk::DescriptorSet descriptor_Sets[3];

	// per descriptor
	VulkanDescriptorState descriptor_states[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
};

#define VULKAN_OBJECT_MAX_OBJECT_COUNT 1024

class VulkanMaterialShader {
public:
	VulkanMaterialShader() {}
	virtual ~VulkanMaterialShader() {}

public:
	bool Create(VulkanContext* context, Texture* default_diffuse);
	void Destroy(VulkanContext* context);
	void Use(VulkanContext* context);
	
	void UpdateGlobalState(VulkanContext* context, double delta_time);
	void UpdateObject(VulkanContext* context, GeometryRenderData geometry);

	bool AcquireResources(VulkanContext* context, uint32_t* id);
	void ReleaseResources(VulkanContext* context, uint32_t id);

public:
	// vertex, fragment
	VulkanShaderStage Stages[OBJECT_SHADER_STAGE_COUNT];
	VulkanPipeline Pipeline;

	// Descriptors
	vk::DescriptorPool GlobalDescriptorPool;
	vk::DescriptorSetLayout GlobalDescriptorSetLayout;

	// One descriptor set per frame - max 3 for triple-buffering.
	vk::DescriptorSet GlobalDescriptorSets[3];
	
	// Global Uniform buffer object.
	SGlobalUBO GlobalUBO;

	// Global uniform buffer.
	VulkanBuffer GlobalUniformBuffer;

	bool DescriptorUpdated[3];

	vk::DescriptorPool ObjectDescriptorPool;
	vk::DescriptorSetLayout ObjectDescriptorSetLayout;

	// Object uniform buffers.
	VulkanBuffer ObjectUniformBuffer;

	// TODO: manage a free list of some kind here instead.
	uint32_t ObjectUniformBufferIndex;

	// TODO: Make dynamic.
	VulkanObjectShaderObjectState ObjectStates[VULKAN_OBJECT_MAX_OBJECT_COUNT];
};
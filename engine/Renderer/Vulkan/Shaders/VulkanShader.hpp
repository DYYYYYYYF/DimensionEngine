#pragma once

#include <vulkan/vulkan.hpp>

#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Vulkan/VulkanPipeline.hpp"

#define OBJECT_SHADER_STAGE_COUNT 2
class VulkanContext;

class VulkanShaderModule {
public:
	VulkanShaderModule() {}
	virtual ~VulkanShaderModule() {}

public:
	bool Create(VulkanContext* context);
	void Destroy(VulkanContext* context);
	void Use(VulkanContext* context);
	
	void UpdateGlobalState(VulkanContext* context);

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
};
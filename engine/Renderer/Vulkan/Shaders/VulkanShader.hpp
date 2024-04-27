#pragma once

#include <vulkan/vulkan.hpp>

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

	// vertex, fragment
	VulkanShaderStage Stages[OBJECT_SHADER_STAGE_COUNT];
	VulkanPipeline Pipeline;
};
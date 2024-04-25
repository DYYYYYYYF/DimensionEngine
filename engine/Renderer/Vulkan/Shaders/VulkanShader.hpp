#pragma once

#include <vulkan/vulkan.hpp>

class VulkanShaderStage{
public:
	vk::ShaderModuleCreateInfo create_info;
	vk::ShaderModule shader_module;
	vk::PipelineShaderStageCreateInfo shader_stage_create_info;
};

class VulkanPipeline {
public:
	vk::Pipeline Handle;
	vk::PipelineLayout PipelineLayout;
};

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
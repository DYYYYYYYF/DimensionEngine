#pragma once

#include <vulkan/vulkan.hpp>
#include "Resources/ResourceTypes.hpp"
#include "VulkanPipeline.hpp"

class VulkanRenderPass;

#define VULKAN_SHADER_MAX_STAGES 8
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31

#define VULKAN_SHADER_MAX_ATTRIBUTES 16
#define VULKAN_SHADER_MAX_UNIFORMS 128

#define VULKAN_SHADER_MAX_BINDINGS 2
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32

struct VulkanShaderStageConfig{
	vk::ShaderStageFlagBits stage;
	char filename[255];
};

struct VulkanDescriptorSetConfig {
	unsigned short binding_count = 0;
	vk::DescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
};

struct VulkanShaderConfig {
	unsigned short stage_count;
	VulkanShaderStageConfig stages[VULKAN_SHADER_MAX_STAGES];
	vk::DescriptorPoolSize pool_sizes[2];

	unsigned short max_descriptor_set_count;
	unsigned short descriptor_set_count;
	VulkanDescriptorSetConfig descriptor_sets[2];
	vk::VertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];
};

struct VulkanDescriptorState {
	// One per frame
	uint32_t generations[3];
	uint32_t ids[3];
};

struct VulkanShaderDescriptorSetState {
	vk::DescriptorSet descriptorSets[3];
	VulkanDescriptorState descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
};

struct VulkanShaderStage {
	vk::ShaderModuleCreateInfo create_info;
	vk::ShaderModule shader_module;
	vk::PipelineShaderStageCreateInfo shader_stage_create_info;
};

struct VulkanShaderInstanceState {
	uint32_t id;
	size_t offset;
	VulkanShaderDescriptorSetState descriptor_set_state;
	std::vector<TextureMap*> instance_texture_maps;
};

class VulkanShader {
public:
	void* MappedUniformBufferBlock;
	uint32_t ID;
	VulkanShaderConfig Config;
	VulkanRenderPass* Renderpass = nullptr;
	VulkanShaderStage Stages[VULKAN_SHADER_MAX_STAGES];

	vk::DescriptorPool DescriptorPool;
	vk::DescriptorSetLayout DescriptorSetLayouts[2];
	vk::DescriptorSet GlobalDescriptorSets[3];
	VulkanBuffer UniformBuffer;

	VulkanPipeline Pipeline;

	uint32_t InstanceCount;
	VulkanShaderInstanceState InstanceStates[VULKAN_MAX_MATERIAL_COUNT];
};
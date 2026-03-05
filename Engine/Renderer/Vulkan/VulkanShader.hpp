#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanPipeline.hpp"
#include "VulkanBuffer.hpp"
#include "Resources/ResourceTypes.hpp"

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
	char filename[255] = "";
};

struct VulkanDescriptorSetConfig {
	unsigned short binding_count = 0;
	vk::DescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
	unsigned char sampler_binding_index = 0;
};

struct VulkanShaderConfig {
	uint16_t stage_count = 0;
	VulkanShaderStageConfig stages[VULKAN_SHADER_MAX_STAGES];
	vk::DescriptorPoolSize pool_sizes[2];

	uint16_t max_descriptor_set_count = 0;
	uint16_t descriptor_set_count = 0;
	VulkanDescriptorSetConfig descriptor_sets[2];
	vk::VertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];

	FaceCullMode cull_mode = FaceCullMode::eFace_Cull_Mode_Back;
	PolygonMode pology_mode = PolygonMode::ePology_Mode_Fill;
	PrimitiveTopology PrimTopo = PrimitiveTopology::eTriangleList;
};

struct VulkanDescriptorState {
	// One per frame
	uint32_t generations[3] = { INVALID_ID };
	uint32_t ids[3] = {INVALID_ID};
};

struct VulkanShaderDescriptorSetState {
	std::array<vk::DescriptorSet, 3> descriptorSets;
	VulkanDescriptorState descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
};

struct VulkanShaderStage {
	vk::ShaderModuleCreateInfo create_info;
	vk::ShaderModule shader_module;
	vk::PipelineShaderStageCreateInfo shader_stage_create_info;
};

struct VulkanShaderInstanceState {
	uint32_t id = INVALID_ID;
	size_t offset = 0;
	VulkanShaderDescriptorSetState descriptor_set_state;
	std::vector<TextureMap*> instance_texture_maps;
};

class VulkanShader : public Shader {
public:
	VulkanShader() : Shader() {
		ID = INVALID_ID;
		MappedUniformBufferBlock = nullptr;
		Renderpass = nullptr;
		InstanceCount = 0;
		GlobalUniformCount = 0;
		GlobalUniformSamplerCount = 0;
		InstanceUniformCount = 0;
		InstanceUniformSamplerCount = 0;
		LocalUniformCount = 0;
	}
	
	VulkanShader(IRenderer* Renderer) : Shader(Renderer) {
		ID = INVALID_ID;
		MappedUniformBufferBlock = nullptr;
		Renderpass = nullptr;
		InstanceCount = 0;
		GlobalUniformCount = 0;
		GlobalUniformSamplerCount = 0;
		InstanceUniformCount = 0;
		InstanceUniformSamplerCount = 0;
		LocalUniformCount = 0;
	}

	virtual ~VulkanShader() {
		if (Status != ShaderStatus::eShader_State_Not_Created) {
			Destroy();
		}
	}

public:
	virtual bool Initialize() override;
	virtual bool Reload() override;
	virtual void Destroy() override;

protected:
	virtual bool CreateModule();
	virtual bool CompileShaderFile(bool writeToDisk = true);

private:
	bool CreatePipeline();

public:
	void* MappedUniformBufferBlock;
	uint32_t ID;
	VulkanShaderConfig Config;
	VulkanRenderPass* Renderpass;
	VulkanShaderStage Stages[VULKAN_SHADER_MAX_STAGES];

	vk::DescriptorPool DescriptorPool;
	vk::DescriptorSetLayout DescriptorSetLayouts[2];
	vk::DescriptorSet GlobalDescriptorSets[3];
	VulkanBuffer UniformBuffer;

	VulkanPipeline Pipeline;

	uint32_t InstanceCount;
	VulkanShaderInstanceState InstanceStates[VULKAN_MAX_MATERIAL_COUNT];

	unsigned char GlobalUniformCount;
	unsigned char GlobalUniformSamplerCount;
	unsigned char InstanceUniformCount;
	unsigned char InstanceUniformSamplerCount;
	unsigned char LocalUniformCount;
};
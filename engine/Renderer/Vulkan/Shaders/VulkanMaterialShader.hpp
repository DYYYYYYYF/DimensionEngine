#pragma once

#include <vulkan/vulkan.hpp>

#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Vulkan/VulkanPipeline.hpp"
#include "Resources/Texture.hpp"
#include "Resources/Material.hpp"

#define MATERIAL_SHADER_STAGE_COUNT 2
#define VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT 2
#define VULKAN_MATERIAL_SHADER_SAMPLER_COUNT 1

class VulkanContext;
class Material;

// Uniform Buffer Object
struct MaterialShaderGlobalUbo {
	Matrix4 projection;	// 64 bytes
	Matrix4 view;		// 64 bytes
	Matrix4 reserved0;	// 64 bytes, reserved for future use
	Matrix4 reserved1;	// 64 bytes, reserved for future use
};

// Object Material
struct MaterialShaderInstanceUbo {
	Vec4 diffuse_color;	// 16 Bytes
	Vec4 v_reserved0;	// 16 Bytes,reserved for future use
	Vec4 v_reserved1;	// 16 Bytes,reserved for future use
	Vec4 v_reserved2;	// 16 Bytes,reserved for future use
};

struct VulkanDescriptorState {
	// One per frame
	uint32_t generations[3];
	uint32_t ids[3];
};

struct VulkanMaterialShaderInstanceState {
	// Per frame
	vk::DescriptorSet descriptor_Sets[3];

	// per descriptor
	VulkanDescriptorState descriptor_states[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
};

#define VULKAN_MAX_MATERIAL_COUNT 1024

class VulkanMaterialShader {
public:
	VulkanMaterialShader() {}
	virtual ~VulkanMaterialShader() {}

public:
	bool Create(VulkanContext* context);
	void Destroy(VulkanContext* context);
	void Use(VulkanContext* context);
	
	void UpdateGlobalState(VulkanContext* context, double delta_time);
	void SetModelMat(VulkanContext* context, Matrix4 model);
	void ApplyMaterial(VulkanContext* context, Material* material);

	bool AcquireResources(VulkanContext* context, Material* material);
	void ReleaseResources(VulkanContext* context, Material* material);

public:
	// vertex, fragment
	VulkanShaderStage Stages[MATERIAL_SHADER_STAGE_COUNT];
	VulkanPipeline Pipeline;

	// Descriptors
	vk::DescriptorPool GlobalDescriptorPool;
	vk::DescriptorSetLayout GlobalDescriptorSetLayout;

	// One descriptor set per frame - max 3 for triple-buffering.
	vk::DescriptorSet GlobalDescriptorSets[3];
	
	// Global Uniform buffer object.
	MaterialShaderGlobalUbo GlobalUBO;

	// Global uniform buffer.
	VulkanBuffer GlobalUniformBuffer;

	vk::DescriptorPool ObjectDescriptorPool;
	vk::DescriptorSetLayout ObjectDescriptorSetLayout;

	// Object uniform buffers.
	VulkanBuffer ObjectUniformBuffer;

	// TODO: manage a free list of some kind here instead.
	uint32_t ObjectUniformBufferIndex;

	TextureUsage SamplerUsage[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];

	// TODO: Make dynamic.
	VulkanMaterialShaderInstanceState InstanceStates[VULKAN_MAX_MATERIAL_COUNT];
};
#pragma once

#include <vulkan/vulkan.hpp>

#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Vulkan/VulkanPipeline.hpp"
#include "Resources/Texture.hpp"
#include "Resources/Material.hpp"

#include "VulkanMaterialShader.hpp"

#define UI_SHADER_STAGE_COUNT 2
#define VULKAN_UI_SHADER_DESCRIPTOR_COUNT 2
#define VULKAN_UI_SHADER_SAMPLER_COUNT 1

class VulkanContext;
class Material;

struct VulkanUIShaderInstanceState {
	// Per frame
	vk::DescriptorSet descriptor_Sets[3];

	// per descriptor
	VulkanDescriptorState descriptor_states[VULKAN_UI_SHADER_DESCRIPTOR_COUNT];
};

struct UIShaderGlobalUbo {
	Matrix4 projection;	// 64 bytes
	Matrix4 view;		// 64 bytes
	Matrix4 reserved0;	// 64 bytes, reserved for future use
	Matrix4 reserved1;	// 64 bytes, reserved for future use
};

struct UIShaderInstanceUbo {
	Vec4 diffuse_color;	// 16 bytes
	Vec4 reserved0;		// 16 bytes, reserved for future use
	Vec4 reserved1;		// 16 bytes, reserved for future use
	Vec4 reserved2;		// 16 bytes, reserved for future use
};

#define VULKAN_MAX_UI_COUNT 1024

class VulkanUIShader {
public:
	VulkanUIShader() {}
	virtual ~VulkanUIShader() {}

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
	VulkanShaderStage Stages[UI_SHADER_STAGE_COUNT];
	VulkanPipeline Pipeline;

	// Descriptors
	vk::DescriptorPool GlobalDescriptorPool;
	vk::DescriptorSetLayout GlobalDescriptorSetLayout;

	// One descriptor set per frame - max 3 for triple-buffering.
	vk::DescriptorSet GlobalDescriptorSets[3];

	// Global Uniform buffer object.
	UIShaderGlobalUbo GlobalUBO;

	// Global uniform buffer.
	VulkanBuffer GlobalUniformBuffer;

	vk::DescriptorPool ObjectDescriptorPool;
	vk::DescriptorSetLayout ObjectDescriptorSetLayout;

	// Object uniform buffers.
	VulkanBuffer ObjectUniformBuffer;

	// TODO: manage a free list of some kind here instead.
	uint32_t ObjectUniformBufferIndex;

	TextureUsage SamplerUsage[VULKAN_UI_SHADER_SAMPLER_COUNT];

	// TODO: Make dynamic.
	VulkanUIShaderInstanceState InstanceStates[VULKAN_MAX_UI_COUNT];
};
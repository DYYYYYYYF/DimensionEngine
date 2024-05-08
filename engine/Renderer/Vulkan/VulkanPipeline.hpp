#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;
class VulkanRenderPass;
class VulkanCommandBuffer;

class VulkanShaderStage {
public:
	vk::ShaderModuleCreateInfo create_info;
	vk::ShaderModule shader_module;
	vk::PipelineShaderStageCreateInfo shader_stage_create_info;
};

class VulkanPipeline {
public:
	VulkanPipeline() {}
	virtual ~VulkanPipeline() {}

public:
	bool Create(VulkanContext* context, VulkanRenderPass* renderpass, uint32_t stride,
		uint32_t attribute_count, vk::VertexInputAttributeDescription* attributes,
		uint32_t descriptor_set_layout_count, vk::DescriptorSetLayout* descriptor_set_layout,
		uint32_t stage_count, vk::PipelineShaderStageCreateInfo* stages,
		vk::Viewport viewport, vk::Rect2D scissor, bool is_wireframe, bool depth_test_enabled);

	void Destroy(VulkanContext* context);
	void Bind(VulkanCommandBuffer* command_buffer, vk::PipelineBindPoint bind_point);

public:
	vk::Pipeline Handle;
	vk::PipelineLayout PipelineLayout;

};
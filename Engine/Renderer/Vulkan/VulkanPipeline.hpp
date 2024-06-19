#pragma once

#include <vulkan/vulkan.hpp>
#include "Resources/Shader.hpp"

enum FaceCullMode;
class VulkanContext;
class VulkanRenderPass;
class VulkanCommandBuffer;
struct Range;

struct VulkanPipelineConfig {
	VulkanRenderPass* renderpass;
	uint32_t stride;
	uint32_t attribute_count;
	vk::VertexInputAttributeDescription* attributes = nullptr;
	uint32_t descriptor_set_layout_count;
	vk::DescriptorSetLayout* descriptor_set_layout = nullptr;
	uint32_t stage_count;
	vk::PipelineShaderStageCreateInfo* stages = nullptr;
	vk::Viewport viewport;
	vk::Rect2D scissor;
	FaceCullMode cull_mode;
	bool is_wireframe;
	bool depth_test_enabled;
	uint32_t push_constant_range_count;
	Range* push_constant_ranges = nullptr;
	ShaderFlagBits shaderFlags;
};

class VulkanPipeline {
public:
	VulkanPipeline() {}
	virtual ~VulkanPipeline() {}

public:
	bool Create(VulkanContext* context, const VulkanPipelineConfig& config);

	void Destroy(VulkanContext* context);
	void Bind(VulkanCommandBuffer* command_buffer, vk::PipelineBindPoint bind_point);

public:
	vk::Pipeline Handle;
	vk::PipelineLayout PipelineLayout;

};

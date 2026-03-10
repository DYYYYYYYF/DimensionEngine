#pragma once

#include <vulkan/vulkan.hpp>
#include "Rendering/Resources/Shader/Shader.hpp"

enum FaceCullMode;
class VulkanContext;
class VulkanRenderPass;
class VulkanCommandBuffer;
struct Range;

struct VulkanPipelineConfig {
	VulkanRenderPass* renderpass = nullptr;
	uint32_t stride = 0;
	uint32_t attribute_count = 0;
	vk::VertexInputAttributeDescription* attributes = nullptr;
	uint32_t descriptor_set_layout_count = 0;
	vk::DescriptorSetLayout* descriptor_set_layout = nullptr;
	uint32_t stage_count = 0;
	vk::PipelineShaderStageCreateInfo* stages = nullptr;
	vk::Viewport viewport;
	vk::Rect2D scissor;
	FaceCullMode cull_mode = FaceCullMode::eFace_Cull_Mode_Back;
	PrimitiveTopology PrimTopo = PrimitiveTopology::eTriangleList;
	bool is_wireframe = false;
	bool depth_test_enabled = true;
	uint32_t push_constant_range_count = 0;
	Range* push_constant_ranges = nullptr;
	ShaderFlagBits shaderFlags = 0;
};

class VulkanPipeline {
public:
	bool Create(VulkanContext* context, const VulkanPipelineConfig& config);

	void Destroy(VulkanContext* context);
	void Bind(VulkanCommandBuffer* command_buffer, vk::PipelineBindPoint bind_point);

public:
	vk::Pipeline Handle = nullptr;
	vk::PipelineLayout PipelineLayout = nullptr;

};

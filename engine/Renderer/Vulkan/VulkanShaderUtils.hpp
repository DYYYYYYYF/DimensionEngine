#pragma once

#include <vulkan/vulkan.hpp>
#include "Defines.hpp"

class VulkanContext;
class VulkanShaderStage;

class VulkanShaderUtils {
public:
	static bool CreateShaderModule(VulkanContext* context, const char* name, const char* type_str,
		vk::ShaderStageFlagBits stage_flag, uint32_t stage_index, VulkanShaderStage* shader_stage);
};

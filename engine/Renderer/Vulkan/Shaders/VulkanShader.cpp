#include "VulkanShader.hpp"

#include "Renderer/Vulkan/VulkanContext.hpp"
#include "Renderer/Vulkan/VulkanShaderUtils.hpp"

bool VulkanShaderModule::Create(VulkanContext* context) {
	// Shader module init per stage
	char StageTypeStrs[OBJECT_SHADER_STAGE_COUNT][5] = { "vert", "frag" };
	vk::ShaderStageFlagBits StageTypes[OBJECT_SHADER_STAGE_COUNT] = { vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment };

	for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
		if (!VulkanShaderUtils::CreateShaderModule(context, "default", StageTypeStrs[i], StageTypes[i], i, Stages)) {
			UL_ERROR("Unable to create %s shader module for '%s'", StageTypeStrs[i], "Test");
			return false;
		}
	}

	// Descriptors

	return true;
}

void VulkanShaderModule::Destroy(VulkanContext* context) {

}

void VulkanShaderModule::Use(VulkanContext* context) {

}

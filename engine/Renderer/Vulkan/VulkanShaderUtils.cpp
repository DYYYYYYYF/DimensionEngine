#include "VulkanShaderUtils.hpp"
#include "VulkanContext.hpp"
#include "Shaders/VulkanMaterialShader.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Systems/ResourceSystem.h"

bool VulkanShaderUtils::CreateShaderModule(VulkanContext* context, const char* name, const char* type_str,
	vk::ShaderStageFlagBits stage_flag, uint32_t stage_index, VulkanShaderStage* shader_stage) {
	// Build file name, which will also be used as the resource name.
	char FileName[512];
	sprintf(FileName, "Shaders/%s.%s.spv", name, type_str);

	// Read binary resource.
	Resource BinaryResource;
	if (!ResourceSystem::Load(FileName, eResource_type_Binary, &BinaryResource)) {
		UL_ERROR("Unable to read shader module: '%s'.", FileName);
		return false;
	}

	Memory::Zero(&shader_stage[stage_index].create_info, sizeof(vk::ShaderModuleCreateInfo));
	shader_stage[stage_index].create_info.sType = vk::StructureType::eShaderModuleCreateInfo;
	// Use the resource's size and data directly.
	shader_stage[stage_index].create_info.setCodeSize(BinaryResource.DataSize);
	shader_stage[stage_index].create_info.setPCode((uint32_t*)BinaryResource.Data);

	shader_stage[stage_index].shader_module = context->Device.GetLogicalDevice().createShaderModule(shader_stage[stage_index].create_info, context->Allocator);
	ASSERT(shader_stage[stage_index].shader_module);

	// Release the resource.
	ResourceSystem::Unload(&BinaryResource);

	// Shader stage info
	Memory::Zero(&shader_stage[stage_index].shader_stage_create_info, sizeof(vk::PipelineShaderStageCreateInfo));
	shader_stage[stage_index].shader_stage_create_info.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	shader_stage[stage_index].shader_stage_create_info.setStage(stage_flag)
		.setModule(shader_stage[stage_index].shader_module)
		.setPName("main");

	return true;
}
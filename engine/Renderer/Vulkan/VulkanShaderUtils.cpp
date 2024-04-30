#include "VulkanShaderUtils.hpp"
#include "VulkanContext.hpp"
#include "Shaders/VulkanMaterialShader.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Platform/FileSystem.hpp"

bool VulkanShaderUtils::CreateShaderModule(VulkanContext* context, const char* name, const char* type_str,
	vk::ShaderStageFlagBits stage_flag, uint32_t stage_index, VulkanShaderStage* shader_stage) {
	// Build file name
	char FileName[512];
	sprintf(FileName, "../shader/glsl/%s.%s.spv", name, type_str);

	Memory::Zero(&shader_stage[stage_index].create_info, sizeof(vk::ShaderModuleCreateInfo));
	shader_stage[stage_index].create_info.sType = vk::StructureType::eShaderModuleCreateInfo;

	// Obtain file handle
	FileHandle Handle;
	if (!FileSystemOpen(FileName, eFile_Mode_Read, true, &Handle)) {
		UL_ERROR("Unable to read shader module: %s", FileName);
		return false;
	}

	// Read the entire file as binary
	size_t Size = 0;
	char* FileBuffer = nullptr;
	if (!FileSystemReadAllBytes(&Handle, &FileBuffer, &Size)) {
		UL_ERROR("Unable to binary read shader module: %s", FileName);
		return false;
	}
	shader_stage[stage_index].create_info.setCodeSize(Size)
		.setPCode((uint32_t*)FileBuffer);

	// Close the file
	FileSystemClose(&Handle);

	shader_stage[stage_index].shader_module = context->Device.GetLogicalDevice().createShaderModule(shader_stage[stage_index].create_info, context->Allocator);
	ASSERT(shader_stage[stage_index].shader_module);

	// Shader stage info
	Memory::Zero(&shader_stage[stage_index].shader_stage_create_info, sizeof(vk::PipelineShaderStageCreateInfo));
	shader_stage[stage_index].shader_stage_create_info.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	shader_stage[stage_index].shader_stage_create_info.setStage(stage_flag)
		.setModule(shader_stage[stage_index].shader_module)
		.setPName("main");

	if (FileBuffer) {
		Memory::Free(FileBuffer, sizeof(char) * Size, MemoryType::eMemory_Type_String);
		FileBuffer = nullptr;
	}

	return true;
}
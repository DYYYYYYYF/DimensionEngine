#include "VulkanShader.hpp"
#include "Systems/ResourceSystem.h"
#include "Core/EngineLogger.hpp"
#include "Platform/FileSystem.hpp"
#include "Containers/TString.hpp"

std::vector<uint32_t> VulkanShader::CreateShaderModule(const char* filename, EShLanguage Stage) {
	std::vector<uint32_t> mSPIRVCode; // 编译好了之后的数据
	glslang::InitializeProcess();

	const char* FormatStr = "%s/%s";
	char FullFilePath[512];
	StringFormat(FullFilePath, 512, FormatStr, ResourceSystem::GetRootPath(), filename);

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, true, &File)) {
		LOG_ERROR("Binary loader load. Unable to open file for binary reading: '%s'.", filename);
		return mSPIRVCode;
	}

	size_t FileSize = 0;
	if (!FileSystemSize(&File, &FileSize)) {
		LOG_ERROR("Unable to binary read file: '%s'.", filename);
		FileSystemClose(&File);
		return mSPIRVCode;
	}

	// TODO: Should be using an allocator here.
	unsigned char* ResourceData = (unsigned char*)Memory::Allocate(sizeof(unsigned char) * FileSize, MemoryType::eMemory_Type_Array);
	size_t ReadSize = 0;
	if (!FileSystemReadAllBytes(&File, ResourceData, &ReadSize)) {
		LOG_ERROR("Unable to binary read file: '%s'.", filename);
		FileSystemClose(&File);
		return mSPIRVCode;
	}

	FileSystemClose(&File);

	// 指定需要编译的总类以及源码
	glslang::TShader vkShader(Stage);
	vkShader.setStrings((const char* const*)ResourceData, 1);
	//指定Shader版本号
	vkShader.setEnvInput(glslang::EShSourceGlsl, Stage, glslang::EShClientVulkan, 100);
	vkShader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
	vkShader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

	TBuiltInResource DefaultTBuiltInResource;
	if (!vkShader.parse(&DefaultTBuiltInResource, 100, ENoProfile, false, false, EShMsgDefault)) {
		LOG_FATAL(vkShader.getInfoLog());
		return mSPIRVCode;
	}

	// 实例化TProgram对象，作为编译Shader的类
	glslang::TProgram Program;
	Program.addShader(&vkShader);
	if (!Program.link(EShMsgDefault)) {
		return mSPIRVCode;
	}

	const auto Intermediate = Program.getIntermediate(Stage);
	//glslang::GlslangToSpv(*Intermediate, mSPIRVCode);

	////将编译好的Shader作为参数传给vk::ShaderModule，作为最终的渲染管线数据。
	glslang::FinalizeProcess();

	return mSPIRVCode;
}
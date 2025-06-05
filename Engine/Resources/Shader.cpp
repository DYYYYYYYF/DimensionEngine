#include "Shader.hpp"
#include "Systems/ResourceSystem.h"
#include "Platform/File.hpp"
#include "Renderer/Vulkan/VulkanShader.hpp"

std::vector<uint32_t> Shader::CompileShaderToSPV(const std::string& filename, enum ShaderStage shaderStage, bool writeToDisk) {
	size_t PrePathIndex = filename.find_first_of('/');
	size_t SufPathIndex = filename.find_last_of(".");
	std::string PrePath = filename.substr(0, PrePathIndex);
	std::string SufPath = filename.substr(PrePathIndex, SufPathIndex - PrePathIndex);

	std::string ShaderSourceFilename;
	shaderc_source_language SourceLanguage;
	switch (Language)
	{
	case ShaderLanguage::eGLSL:
		ShaderSourceFilename = ResourceSystem::GetRootPath() + std::string("/../Shaders/glsl") + SufPath;
		SourceLanguage = shaderc_source_language_glsl;
		break;
	case ShaderLanguage::eHLSL:
		ShaderSourceFilename = ResourceSystem::GetRootPath() + std::string("/../Shaders/hlsl") + SufPath + ".hlsl";
		SourceLanguage = shaderc_source_language_hlsl;
		break;
	default:
		GLOG(Log::eError, "Unknown shader language flag.");
		return std::vector<uint32_t>();
	}

	shaderc_shader_kind scShadercStage;
	switch (shaderStage)
	{
	case ShaderStage::eShader_Stage_Vertex:
		scShadercStage = shaderc_shader_kind::shaderc_vertex_shader;
		break;
	case ShaderStage::eShader_Stage_Fragment:
		scShadercStage = shaderc_shader_kind::shaderc_fragment_shader;
		break;
	case ShaderStage::eShader_Stage_Geometry:
		scShadercStage = shaderc_shader_kind::shaderc_geometry_shader;
		break;
	case ShaderStage::eShader_Stage_Compute:
		scShadercStage = shaderc_shader_kind::shaderc_compute_shader;
		break;
	default:
		GLOG(Log::eError, "Unknown shader stage flag.");
		return std::vector<uint32_t>();
	}

	GLOG(Log::eInfo, "Compile shader file %s...", ShaderSourceFilename.c_str());

	File ShaderSource(ShaderSourceFilename);
	std::string Content = ShaderSource.ReadBytes();
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	options.SetTargetSpirv(shaderc_spirv_version_1_6);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);	// 优化
	options.SetSourceLanguage(SourceLanguage);

	// Like -DMY_DEFINE=1
	//options.AddMacroDefinition("MY_DEFINE", "1");

	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(Content, scShadercStage, Name.c_str(), options);

	if (module.GetCompilationStatus() !=
		shaderc_compilation_status_success) {
		GLOG(Log::eError, "Compile shader %s failed.\n\
			Error msg: %s",
			Name.c_str(),
			module.GetErrorMessage().c_str()
		);
		return std::vector<uint32_t>();
	}

	std::vector<uint32_t> SPRIV = std::vector<uint32_t>(module.cbegin(), module.cend());

	// 写入文件
	if (writeToDisk && SPRIV.data()) {
		std::string SPRIVFilePath = ResourceSystem::GetRootPath() + std::string("/Shaders") + SufPath + ".spv";
		File OutFile(SPRIVFilePath);
		OutFile.WriteBytes(reinterpret_cast<const char*>(SPRIV.data()), SPRIV.size() * sizeof(uint32_t), std::ios::trunc | std::ios::binary);
		GLOG(Log::eInfo, "Write shader file into %s...", SPRIVFilePath.c_str());
	}

	return SPRIV;
}


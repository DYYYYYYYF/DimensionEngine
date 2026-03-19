#include "Shader.hpp"
#include "Platform/File/File.hpp"
#include "Systems/ResourceSystem.h"

std::vector<uint32_t> Shader::CompileShaderToSPV(const FString& filename, enum ShaderStage shaderStage, bool writeToDisk) {
	size_t PrePathIndex = filename.IndexOf('/');
	size_t SufPathIndex = filename.LastIndexOf('.');
	FString PrePath = filename.SubStr(0, PrePathIndex);
	FString SufPath = filename.SubStr(PrePathIndex, SufPathIndex - PrePathIndex);

	FString ShaderSourceFilename;
	shaderc_source_language SourceLanguage;
	switch (Language)
	{
	case EShaderLanguage::eGLSL:
		ShaderSourceFilename = ResourceSystem::Get().GetRootPath() + FString("/../Shaders/glsl") + SufPath;
		SourceLanguage = shaderc_source_language_glsl;
		break;
	case EShaderLanguage::eHLSL:
		ShaderSourceFilename = ResourceSystem::Get().GetRootPath() + FString("/../Shaders/hlsl") + SufPath + ".hlsl";
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

	GLOG(Log::eInfo, "Compile shader file %s...", ShaderSourceFilename.CStr());

	File ShaderSource(ShaderSourceFilename);
	FString Content = ShaderSource.ReadText();
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	options.SetTargetSpirv(shaderc_spirv_version_1_6);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);	// 优化
	options.SetSourceLanguage(SourceLanguage);

	// Like -DMY_DEFINE=1
	//options.AddMacroDefinition("MY_DEFINE", "1");

	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(Content.CStr(), scShadercStage, Name.CStr(), options);

	if (module.GetCompilationStatus() !=
		shaderc_compilation_status_success) {
		GLOG(Log::eError, "Compile shader %s failed.\n\
			Error msg: %s",
			Name.CStr(),
			module.GetErrorMessage().c_str()
		);
		return std::vector<uint32_t>();
	}

	std::vector<uint32_t> SPRIV = std::vector<uint32_t>(module.cbegin(), module.cend());

	// 写入文件
	if (writeToDisk && SPRIV.data()) {
		FString SPRIVFilePath = ResourceSystem::Get().GetRootPath() + FString("/Shaders") + SufPath + ".spv";
		File OutFile(SPRIVFilePath);
		OutFile.WriteBytes(reinterpret_cast<const char*>(SPRIV.data()), SPRIV.size() * sizeof(uint32_t), std::ios::trunc | std::ios::binary);
		GLOG(Log::eInfo, "Write shader file into %s...", SPRIVFilePath.CStr());
	}

	return SPRIV;
}


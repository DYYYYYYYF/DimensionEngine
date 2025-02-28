#include "Utils.hpp"
#include "Platform/File.hpp"
#include "Resources/Shader.hpp"

std::vector<uint32_t> Utils::CompileShader(const std::string& file, enum ShaderStage shaderStage, bool writeToDisk/* = true*/) {
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
	}

	File ShaderSource(file);
	std::string Content = ShaderSource.ReadBytes();
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	options.SetTargetSpirv(shaderc_spirv_version_1_6);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);	// 优化：性能优先

	// Like -DMY_DEFINE=1
	//options.AddMacroDefinition("MY_DEFINE", "1");

	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(Content, scShadercStage, file.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		LOG_ERROR("Compile shader %s failed.\n\
			Error msg: %s",
			file.c_str(),
			module.GetErrorMessage().c_str()
		);
		return std::vector<uint32_t>();
	}

	// TODO: Write to disk.
	if (writeToDisk) {
		File OutFile(file);

		if (!OutFile.IsExist()) {

		}
		
	}

	return std::vector<uint32_t>(module.cbegin(), module.cend());
}

std::string Utils::Strtrim(const std::string& str) {
	// 找到左边第一个非空白字符
	size_t start = str.find_first_not_of(" \t\n\r\f\v");
	if (start == std::string::npos) {
		return ""; // 全是空白字符
	}

	// 找到右边最后一个非空白字符
	size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> Utils::StringSplit(const std::string& str, char delimiter, bool trim_entries, bool include_empty) {
	std::vector<std::string> Result;
	size_t Head = 0;
	size_t Rear = str.find(delimiter);

	while (Rear != std::string::npos) {
		std::string SubString = str.substr(Head, Rear - Head);

		if (trim_entries) {
			SubString = Strtrim(SubString);
		}

		// 如果字符串不为空或者需要包含空字符都存储子字符串
		if (!SubString.empty() || include_empty) {
			Result.push_back(SubString); 
		}

		Head = Rear + 1;                                 // 更新起始位置
		Rear = str.find(delimiter, Head);                // 查找下一个分隔符
	}

	// 处理最后一个子字符串
	if (Head < str.length()) {
		std::string LastSubString = str.substr(Head);
		if (trim_entries) {
			LastSubString = Strtrim(LastSubString);

		}
		if (!LastSubString.empty() || include_empty) {
			Result.push_back(LastSubString);

		}
	}

	return Result;
}

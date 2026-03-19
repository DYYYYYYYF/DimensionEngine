#pragma once

#include "Rendering/Resources/Asset.hpp"
#include "ShaderType.hpp"
#include <unordered_map>

// Shader compiler
#include <shaderc/shaderc.hpp>

struct TextureMap;
class IRenderer;

class Shader : public UAsset{
public:
	Shader() {
		this->ID = INVALID_ID;
		this->RenderFrameNumber = INVALID_ID_U64;
		this->BoundInstanceId = INVALID_ID;
		this->Status = EShaderStatus::eShader_State_Uninitialized;
		this->Language = EShaderLanguage::eGLSL;
		this->Flags = 0;
		this->AttributeStride = 0;
		this->PushConstantsRangeCount = 0;
		this->RenderFrameNumber = 0;
		this->RequiredUboAlignment = 0;
		this->GlobalUboSize = 0;
		this->GlobalUboOffset = 0;
		this->GlobalUboStride = 0;
		this->UboSize = 0;
		this->UboStride = 0;
		this->PushConstantsSize = 0;
		this->PushConstantsStride = 0;
		this->InstanceTextureCount = 0;
		this->BoundScope = ShaderScope::eShader_Scope_Instance;
		this->BoundUboOffset = 0;
		this->Renderer = nullptr;
	}

	Shader(IRenderer* Renderer)
	{
		this->ID = INVALID_ID;
		this->RenderFrameNumber = INVALID_ID_U64;
		this->BoundInstanceId = INVALID_ID;
		this->Status = EShaderStatus::eShader_State_Uninitialized;
		this->Language = EShaderLanguage::eGLSL;
		this->Flags = 0;
		this->AttributeStride = 0;
		this->PushConstantsRangeCount = 0;
		this->RenderFrameNumber = 0;
		this->RequiredUboAlignment = 0;
		this->GlobalUboSize = 0;
		this->GlobalUboOffset = 0;
		this->GlobalUboStride = 0;
		this->UboSize = 0;
		this->UboStride = 0;
		this->PushConstantsSize = 0;
		this->PushConstantsStride = 0;
		this->InstanceTextureCount = 0;
		this->BoundScope = ShaderScope::eShader_Scope_Instance;
		this->BoundUboOffset = 0;
		this->Renderer = Renderer;
	}

	virtual ~Shader() {}

public:
	virtual bool Reload() = 0;
	virtual void Destroy() = 0;

	// Shader utils
	virtual std::vector<uint32_t> CompileShaderToSPV(const std::string& filename, enum ShaderStage shaderStage, bool writeToDisk = true);


public:
	IRenderer* Renderer;
	ShaderFlagBits Flags;
	uint32_t ID;
	FString Name;
	EShaderLanguage Language;

	size_t RenderFrameNumber;
	size_t RequiredUboAlignment;
	size_t GlobalUboSize;
	size_t GlobalUboOffset;
	size_t GlobalUboStride;
	size_t UboSize;
	size_t UboStride;
	size_t PushConstantsSize;
	size_t PushConstantsStride;
	uint32_t InstanceTextureCount;
	ShaderScope BoundScope;
	uint64_t BoundInstanceId;
	uint32_t BoundUboOffset;
	std::unordered_map<FString, uint32_t> HashMap;
	EShaderStatus Status;
	uint16_t PushConstantsRangeCount;
	Range PushConstantsRanges[32];
	uint16_t AttributeStride;

	std::vector<ShaderUniform> Uniforms;
	std::vector<ShaderAttribute> Attributes;
	std::vector<TextureMap*> GlobalTextureMaps;

};
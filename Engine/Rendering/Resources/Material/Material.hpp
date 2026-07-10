#pragma once

#include "MaterialType.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"

struct ShaderUniform;

struct UniformValue {
	ShaderUniform* uniform = nullptr;
	uint8_t data[64];
};

struct TextureBinding {
	ShaderUniform* uniform = nullptr;
	TextureMap texture;
};

class Material : public UAsset{
public:
	Material();
	virtual ~Material();

public:
	inline uint32_t GetID() const { return ID; }
	inline void SetID(uint32_t id) { ID = id; }

	inline size_t GetReferenceCount() const { return ReferenceCount; }
	inline void SetReferenceCount(uint32_t count) { ReferenceCount = count; }
	inline void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	inline void DecreaseReferenceCount(uint32_t count = 1) { ReferenceCount -= count; }

	inline bool IsAutoRelease() const { return AutoRelease; }
	inline void SetIsAutoRelease(bool b) { AutoRelease = b; }

private:
	uint32_t ID;
	size_t ReferenceCount = 0;
	bool AutoRelease = false;

public:
	// Base
	FString Name;
	uint32_t Generation;
	uint32_t InternalID;
	uint32_t ShaderID;
	uint32_t RenderFrameNumer;

	// Parameters
	TArray<UniformValue> UnifromValues;
	TArray<TextureBinding> TextureBindings;

	// TODO: Remove
	struct {
		// PBR
		const ShaderUniform* diffuseColor;
		const ShaderUniform* shininess;
		const ShaderUniform* metallic;
		const ShaderUniform* roughness;
		const ShaderUniform* ambientOcclusion;
		const ShaderUniform* normalIntensity;

		// Textures
		const ShaderUniform* diffuseTexture;
		const ShaderUniform* normalTexture;
		const ShaderUniform* roughnessMetallicTexture;
	} Handles;
};

#pragma once

#include "Math/MathTypes.hpp"
#include "Rendering/Resources/Texture.hpp"

#define DEFAULT_MATERIAL_NAME "Builtin.Material.Default"
#define VULKAN_MAX_MATERIAL_COUNT 1024

class Texture;

struct SMaterialConfig {
	std::string name;
	std::string shader_name;
	bool auto_release;
	Vector4 diffuse_color = Vector4(1.0f);
	float shininess = 16.0f;
	char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH] = "";
	char specular_map_name[TEXTURE_NAME_MAX_LENGTH] = "";
	char normal_map_name[TEXTURE_NAME_MAX_LENGTH] = "";

	// PBR
	float Metallic = 0.1f;					// 金属度
	float Roughness = 0.5f;					// 粗糙度
	float AmbientOcclusion = 1.0f;			// 环境光遮蔽
	Vector4 EmissiveColor;						// 自发光
	std::string MetallicRoughnessTexName;	// 金属度/粗糙度Texture
	std::string EmissiveFactorTexName;		// 自发光Texture

	float NormalIntensity = 1.0f;
};

class Material {
public:
	Material() {
		ReferenceCount = 0;
		AutoRelease = false;
		ID = INVALID_ID;
		Generation = INVALID_ID;
		InternalID = INVALID_ID;
		DiffuseColor = Vector4(1.0f);
		Shininess = 32.0f;
		ShaderID = INVALID_ID;
		RenderFrameNumer = 0;
		Metallic = 1.0f;
		Roughness = 32.0f;
		AmbientOcclusion = 1.0f;
	}

	virtual ~Material() {
		ReferenceCount = 0;
		AutoRelease = false;
		ID = INVALID_ID;
		Generation = INVALID_ID;
		InternalID = INVALID_ID;
		DiffuseColor = Vector4(1.0f);
		Shininess = 32.0f;
		ShaderID = INVALID_ID;
		RenderFrameNumer = 0;
		Metallic = 1.0f;
		Roughness = 32.0f;
		AmbientOcclusion = 1.0f;
	}

public:
	uint32_t GetID() const { return ID; }
	void SetID(uint32_t id) { ID = id; }

	size_t GetReferenceCount() const { return ReferenceCount; }
	void SetReferenceCount(uint32_t count) { ReferenceCount = count; }
	void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1) { ReferenceCount -= count; }

	bool IsAutoRelease() const { return AutoRelease; }
	void SetIsAutoRelease(bool b) { AutoRelease = b; }

private:
	uint32_t ID;
	size_t ReferenceCount = 0;
	bool AutoRelease = false;

public:
	uint32_t Generation;
	uint32_t InternalID;
	std::string Name;
	Vector4 DiffuseColor;
	TextureMap DiffuseMap;
	TextureMap NormalMap;
	TextureMap RoughnessMetallicMap;
	float Shininess;

	uint32_t ShaderID;
	uint32_t RenderFrameNumer;

	// PBR
	float Metallic;
	float Roughness;
	float AmbientOcclusion;
	float NormalIntensity;
};

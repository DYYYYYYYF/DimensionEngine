#pragma once

#include "MaterialType.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"

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
	uint32_t Generation;
	uint32_t InternalID;
	FString Name;
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

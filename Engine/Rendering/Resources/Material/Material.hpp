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
	friend class MaterialSystem;

public:
	Material();
	virtual ~Material();

public:
	inline uint32_t GetInternalID() const { return InternalID; }
	inline void SetInternalID(uint32_t id) { InternalID = id; }

	inline size_t GetReferenceCount() const { return ReferenceCount; }
	inline void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1);

	inline bool IsAutoRelease() const { return AutoRelease; }
	inline void SetIsAutoRelease(bool b) { AutoRelease = b; }

	const TArray<UniformValue>& GetUniformValues() const { return UnifromValues; }
	const TArray<TextureBinding>& GetTextureBindings() const { return TextureBindings; }

	void SetFrameNumber(size_t frame_number) { RenderFrameNumer = frame_number; }
	bool IsNeedUpdate(size_t current_frame_number) const { return RenderFrameNumer != current_frame_number; }

private:
	void DestroyInstance();

protected:
	// Base
	FString Name;
	uint32_t Generation;
	uint32_t InternalID;
	uint32_t ShaderID;
	size_t RenderFrameNumer;

	// Parameters
	TArray<UniformValue> UnifromValues;
	TArray<TextureBinding> TextureBindings;
	
private:
	size_t ReferenceCount = 0;
	bool AutoRelease = true;

};

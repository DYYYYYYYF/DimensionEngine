#pragma once

#include "Rendering/RenderTypes.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"

#define DEFAULT_DIFFUSE_TEXTURE_NAME "DefaultBaseColorTexture"
#define DEFAULT_SPECULAR_TEXTURE_NAME "DefaultSpecularTexture"
#define DEFAULT_NORMAL_TEXTURE_NAME "DefaultNormalTexture"
#define DEFAULT_ROUGHNESS_METALLIC_TEXTURE_NAME "DefaulRougnessMetallicTexture"

class IRenderer;

struct STextureSystemConfig {
	uint32_t max_texture_count;
};

struct TextureLoadParams {
	FString resource_name;
	UTexture* out_texture = nullptr;
	uint32_t current_generation;
};

class TextureSystem {
public:
	static TextureSystem& Get();

public:
	bool Initialize(IRenderer* renderer, STextureSystemConfig config);
	void Shutdown();

	UTexture* Acquire(const FString& name, bool auto_release = true);
	UTexture* AcquireCube(const FString& name, bool auto_release = true);
	UTexture* AcquireWriteable(const FString& name, uint32_t width, uint32_t height,
		unsigned char channel_count, bool has_transparency, bool has_depth = false);
	void Release(const FString& name);

	void WrapInternal(const FString& name, uint32_t width, uint32_t height, unsigned char channel_count,
		bool has_transparency, bool is_writeable, bool register_texture, UTexture* tex);
	bool Resize(UTexture* t, uint32_t width, uint32_t height, bool regenerate_internal_data);

	UTexture* GetDefaultDiffuseTexture();
	UTexture* GetDefaultSpecularTexture();
	UTexture* GetDefaultNormalTexture();
	UTexture* GetDefaultRoughnessMetallicTexture();

private:
	UTexture* CheckTextureName(const FString& name);
	bool LoadTexture(const FString& name, UTexture* texture);
	bool LoadCubeTexture(const FString& name, const TArray<FString>& texture_names, UTexture* t);
	void DestroyTexture(UTexture* t);

	bool CreateDefaultTexture();
	void DestroyDefaultTexture();
	bool ProcessTextureReference(const FString& name, TextureType type,
		short reference_diff, bool auto_release, bool skip_load);

	void LoadJobSuccess(void* params);
	void LoadJobFail(void* params);
	bool LoadJobStart(void* params, void* result_data);

private:
	STextureSystemConfig TextureSystemConfig;
	UTexture* DefaultDiffuseTexture;
	UTexture* DefaultSpecularTexture;
	UTexture* DefaultNormalTexture;
	UTexture* DefaultRoughnessMetallicTexture;

	// Hashtable for texture lookups.
	std::unordered_map<FString, UTexture*> TextureMap;

	bool Initilized;

	IRenderer* Renderer;
};
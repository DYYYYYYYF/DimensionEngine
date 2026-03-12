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
	UTexture temp_texture;
	uint32_t current_generation;
	UAsset ImageResource;
};

class TextureSystem {
public:
	static bool Initialize(IRenderer* renderer, STextureSystemConfig config);
	static void Shutdown();

	static UTexture* Acquire(const FString& name, bool auto_release);
	static UTexture* AcquireCube(const FString& name, bool auto_release);
	static UTexture* AcquireWriteable(const FString& name, uint32_t width, uint32_t height,
		unsigned char channel_count, bool has_transparency, bool has_depth = false);
	static void Release(const FString& name);

	static void WrapInternal(const char* name, uint32_t width, uint32_t height, unsigned char channel_count,
		bool has_transparency, bool is_writeable, bool register_texture, void* internal_data, UTexture* tex);
	static bool SetInternal(UTexture* t, void* internal_data);
	static bool Resize(UTexture* t, uint32_t width, uint32_t height, bool regenerate_internal_data);
	static bool WriteData(UTexture* t, uint32_t offset, uint32_t size, void* data);

	static UTexture* GetDefaultDiffuseTexture();
	static UTexture* GetDefaultSpecularTexture();
	static UTexture* GetDefaultNormalTexture();
	static UTexture* GetDefaultRoughnessMetallicTexture();

private:
	static UTexture* CheckTextureName(const FString& name);
	static bool LoadTexture(const FString& name, UTexture* texture);
	static bool LoadCubeTexture(const FString& name, FString texture_names[6], UTexture* t);
	static void DestroyTexture(UTexture* t);

	static bool CreateDefaultTexture();
	static void DestroyDefaultTexture();
	static bool ProcessTextureReference(const FString& name, TextureType type,
		short reference_diff, bool auto_release, bool skip_load);

	static void LoadJobSuccess(void* params);
	static void LoadJobFail(void* params);
	static bool LoadJobStart(void* params, void* result_data);

private:
	static STextureSystemConfig TextureSystemConfig;
	static UTexture* DefaultDiffuseTexture;
	static UTexture* DefaultSpecularTexture;
	static UTexture* DefaultNormalTexture;
	static UTexture* DefaultRoughnessMetallicTexture;

	// Hashtable for texture lookups.
	static std::unordered_map<FString, UTexture*> TextureMap;

	static bool Initilized;

	static IRenderer* Renderer;
};
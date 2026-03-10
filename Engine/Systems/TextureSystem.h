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
	std::string resource_name;
	Texture* out_texture = nullptr;
	Texture temp_texture;
	uint32_t current_generation;
	UAsset ImageResource;
};

class TextureSystem {
public:
	static bool Initialize(IRenderer* renderer, STextureSystemConfig config);
	static void Shutdown();

	static Texture* Acquire(const char* name, bool auto_release);
	static Texture* AcquireCube(const char* name, bool auto_release);
	static Texture* AcquireWriteable(const char* name, uint32_t width, uint32_t height, 
		unsigned char channel_count, bool has_transparency, bool has_depth = false);
	static void Release(const std::string& name);

	static void WrapInternal(const char* name, uint32_t width, uint32_t height, unsigned char channel_count,
		bool has_transparency, bool is_writeable, bool register_texture, void* internal_data, Texture* tex);
	static bool SetInternal(Texture* t, void* internal_data);
	static bool Resize(Texture* t, uint32_t width, uint32_t height, bool regenerate_internal_data);
	static bool WriteData(Texture* t, uint32_t offset, uint32_t size, void* data);

	static Texture* GetDefaultDiffuseTexture();
	static Texture* GetDefaultSpecularTexture();
	static Texture* GetDefaultNormalTexture();
	static Texture* GetDefaultRoughnessMetallicTexture();

private:
	static Texture* CheckTextureName(const std::string& name);
	static bool LoadTexture(const std::string& name, Texture* texture);
	static bool LoadCubeTexture(const std::string& name, FString texture_names[6], Texture* t);
	static void DestroyTexture(Texture* t);

	static bool CreateDefaultTexture();
	static void DestroyDefaultTexture();
	static bool ProcessTextureReference(const std::string& name, TextureType type,
		short reference_diff, bool auto_release, bool skip_load);

	static void LoadJobSuccess(void* params);
	static void LoadJobFail(void* params);
	static bool LoadJobStart(void* params, void* result_data);

private:
	static STextureSystemConfig TextureSystemConfig;
	static Texture* DefaultDiffuseTexture;
	static Texture* DefaultSpecularTexture;
	static Texture* DefaultNormalTexture;
	static Texture* DefaultRoughnessMetallicTexture;

	// Hashtable for texture lookups.
	static std::unordered_map<std::string, Texture*> TextureMap;

	static bool Initilized;

	static IRenderer* Renderer;
};
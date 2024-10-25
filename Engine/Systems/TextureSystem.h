#pragma once

#include "Renderer/RendererTypes.hpp"
#include "Resources/Texture.hpp"
#include "Resources/Resource.hpp"
#include "Containers/THashTable.hpp"

#define DEFAULT_DIFFUSE_TEXTURE_NAME "default_diffuse"
#define DEFAULT_SPECULAR_TEXTURE_NAME "default_specular"
#define DEFAULT_NORMAL_TEXTURE_NAME "default_normal"

class IRenderer;

struct STextureSystemConfig {
	uint32_t max_texture_count;
};

struct STextureReference {
	size_t reference_count;
	unsigned int handle;
	bool auto_release;
};

struct TextureLoadParams {
	char* resource_name = nullptr;
	Texture* out_texture = nullptr;
	Texture temp_texture;
	uint32_t current_generation;
	Resource ImageResource;
};

class DAPI TextureSystem {
public:
	static bool Initialize(IRenderer* renderer, STextureSystemConfig config);
	static void Shutdown();

	static Texture* Acquire(const char* name, bool auto_release);
	static Texture* AcquireCube(const char* name, bool auto_release);
	static Texture* AcquireWriteable(const char* name, uint32_t width, uint32_t height, unsigned char channel_count, bool has_transparency);
	static void Release(const char* name);

	static void WrapInternal(const char* name, uint32_t width, uint32_t height, unsigned char channel_count,
		bool has_transparency, bool is_writeable, bool register_texture, void* internal_data, Texture* tex);
	static bool SetInternal(Texture* t, void* internal_data);
	static bool Resize(Texture* t, uint32_t width, uint32_t height, bool regenerate_internal_data);
	static bool WriteData(Texture* t, uint32_t offset, uint32_t size, void* data);

	static Texture* GetDefaultDiffuseTexture();
	static Texture* GetDefaultSpecularTexture();
	static Texture* GetDefaultNormalTexture();

private:
	static bool LoadTexture(const char* name, Texture* texture);
	static bool LoadCubeTexture(const char* name, const char texture_names[6][TEXTURE_NAME_MAX_LENGTH], Texture* t);
	static void DestroyTexture(Texture* t);

	static bool CreateDefaultTexture();
	static void DestroyDefaultTexture();
	static bool ProcessTextureReference(const char* name, TextureType type, short reference_diff, bool auto_release, bool skip_load, uint32_t* out_texture_id);
	
	static void LoadJobSuccess(void* params);
	static void LoadJobFail(void* params);
	static bool LoadJobStart(void* params, void* result_data);

private:
	static STextureSystemConfig TextureSystemConfig;
	static Texture DefaultDiffuseTexture;
	static Texture DefaultSpecularTexture;
	static Texture DefaultNormalTexture;

	// Array of registered textures.
	static Texture* RegisteredTextures;

	// Hashtable for texture lookups.
	static STextureReference* TableMemory;
	static HashTable RegisteredTextureTable;

	static bool Initilized;

	static IRenderer* Renderer;
};
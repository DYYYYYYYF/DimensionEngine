#pragma once

#include "Renderer/RendererTypes.hpp"
#include "Resources/Texture.hpp"
#include "Containers/THashTable.hpp"

#define DEFAULT_TEXTURE_NAME "default"

class IRenderer;

struct STextureSystemConfig {
	uint32_t max_texture_count;
};

struct STextureReference {
	size_t reference_count;
	unsigned int handle;
	bool auto_release;
};

class TextureSystem {
public:
	static bool Initialize(IRenderer* renderer, STextureSystemConfig config);
	static void Shutdown();

	static Texture* Acquire(const char* name, bool auto_release);
	static void Release(const char* name);

	static Texture* GetDefaultTexture();

	static bool LoadTexture(const char* name, Texture* texture);
	static void DestroyTexture(Texture* t);

private:
	static bool CreateDefaultTexture();
	static void DestroyDefaultTexture();

private:
	static STextureSystemConfig TextureSystemConfig;
	static Texture DefaultTexture;

	// Array of registered textures.
	static Texture* RegisteredTextures;

	// Hashtable for texture lookups.
	static STextureReference* TableMemory;
	static HashTable RegisteredTextureTable;

	static bool Initilized;

	static IRenderer* Renderer;
};
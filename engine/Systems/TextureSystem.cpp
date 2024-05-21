#include "TextureSystem.h"

#include "Core/EngineLogger.hpp"
#include "Core/Application.hpp"

#include "Systems/ResourceSystem.h"
#include "Renderer/RendererFrontend.hpp"

STextureSystemConfig TextureSystem::TextureSystemConfig;
Texture TextureSystem::DefaultTexture;
Texture TextureSystem::DefaultDiffuseTexture;
Texture TextureSystem::DefaultSpecularTexture;
Texture TextureSystem::DefaultNormalTexture;
Texture* TextureSystem::RegisteredTextures = nullptr;
STextureReference* TextureSystem::TableMemory = nullptr;
HashTable TextureSystem::RegisteredTextureTable;
bool TextureSystem::Initilized = false;
IRenderer* TextureSystem::Renderer = nullptr;

bool TextureSystem::Initialize(IRenderer* renderer, STextureSystemConfig config) {
	if (config.max_texture_count == 0) {
		UL_FATAL("Texture system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		UL_FATAL("Texture system init failed. Renderer is nullptr.");
		return false;
	}

	if (Initilized) {
		return true;
	}

	TextureSystemConfig = config;
	Renderer = renderer;

	// Block of memory will block for array, then block for hashtable.
	size_t ArraryRequirement = sizeof(Texture) * TextureSystemConfig.max_texture_count;
	size_t HashtableRequirement = sizeof(STextureReference) * TextureSystemConfig.max_texture_count;

	// The array block is after the state. Already allocated, so just set the pointer.
	RegisteredTextures = (Texture*)Memory::Allocate(ArraryRequirement, MemoryType::eMemory_Type_Texture);

	// Create a hashtable for texture lookups.
	TableMemory = (STextureReference*)Memory::Allocate(HashtableRequirement, MemoryType::eMemory_Type_DArray);
	RegisteredTextureTable.Create(sizeof(STextureReference), TextureSystemConfig.max_texture_count, TableMemory, false);

	// Fill the hashtable with invalid references to use as a default.
	STextureReference InvalidRef;
	InvalidRef.auto_release = false;
	InvalidRef.handle = INVALID_ID;
	InvalidRef.reference_count = 0;
	RegisteredTextureTable.Fill(&InvalidRef);

	// Invalidate all textures in the array.
	uint32_t Count = TextureSystemConfig.max_texture_count;
	for (uint32_t i = 0; i < Count; ++i) {
		RegisteredTextures[i].Id = INVALID_ID;
		RegisteredTextures[i].Generation = INVALID_ID;
	}

	// Create default textures for use in the system.
	if (!CreateDefaultTexture()) {
		UL_FATAL("Create default texture failed. Application quit now!");
		return false;
	}

	Initilized = true;
	return true;
}

void TextureSystem::Shutdown() {
	// Destroy all loaded textures.
	for (uint32_t i = 0; i < TextureSystemConfig.max_texture_count; ++i) {
		Texture* t = &RegisteredTextures[i];
		if (t->Generation != INVALID_ID) {
			UL_DEBUG("Destroying texture: '%s'.", t->Name);
			Renderer->DestroyTexture(t);
		}
	}

	DestroyDefaultTexture();
}

Texture* TextureSystem::Acquire(const char* name, bool auto_release) {
	// Return default texture, but warn about it since this should be returned via GetDefaultTexture()
	if (strcmp(name, DEFAULT_TEXTURE_NAME) == 0) {
		UL_WARN("Texture acquire return default texture. Use GetDefaultTexture() for texture 'default'");
		return &DefaultTexture;
	}

	STextureReference Ref;
	if (RegisteredTextureTable.Get(name, &Ref)) {
		// This can only be changed the first time a texture is loaded.
		if (Ref.reference_count == 0) {
			Ref.auto_release = auto_release;
		}

		Ref.reference_count++;
		if (Ref.handle == INVALID_ID) {
			// This mean no texture exists here. Fina a free index first.
			uint32_t Count = TextureSystemConfig.max_texture_count;
			Texture* t = nullptr;
			for (uint32_t i = 0; i < Count; ++i) {
				if (RegisteredTextures[i].Id == INVALID_ID) {
					// A free slot has been found. Use it index as the handle.
					Ref.handle = i;
					t = &RegisteredTextures[i];
					break;
				}
			}

			// Make sure an empty slot was actually found.
			if (t == nullptr || Ref.handle == INVALID_ID) {
				UL_FATAL("Texture acquire failed. Texture system cannot hold anymore textures. Adjust configuration to allow more.");
				return nullptr;
			}

			// Create new texture.
			if (!LoadTexture(name, t)) {
				UL_ERROR("Load %s texture failed.", name);
				return nullptr;
			}

			// Also use the handle as the texture id.
			t->Id = Ref.handle;
			UL_INFO("Texture '%s' does not yet exist. Created and RefCount is now %i.", name, Ref.reference_count);
		}
		else {
			UL_INFO("Texture '%s' already exist. RefCount increased to %i.", name, Ref.reference_count);
		}

		// Update the entry.
		RegisteredTextureTable.Set(name, &Ref);
		return &RegisteredTextures[Ref.handle];
	}

	// NOTO: This can only happen in the event something went wrong with the state.
	UL_ERROR("Texture acquire failed to acquire texture % s, nullptr will be returned.", name);
	return nullptr;
}

void TextureSystem::Release(const char* name) {
	// Ignore release requests for the default texture.
	if (strcmp(name, DEFAULT_TEXTURE_NAME)) {
		return;
	}

	STextureReference Ref;
	if (RegisteredTextureTable.Get(name, &Ref)) {
		if (Ref.reference_count == 0) {
			UL_WARN("Tried to release non-existent texture: %s", name);
			return;
		}

		// Take a copy of the name since it will be wiped out by destroy.
		// (as passed in name is generally a pointer to the actual texture's name).
		char NameCopy[TEXTURE_NAME_MAX_LENGTH];
		strncpy(NameCopy, name, TEXTURE_NAME_MAX_LENGTH);

		Ref.reference_count--;
		if (Ref.reference_count == 0 && Ref.auto_release) {
			Texture* t = &RegisteredTextures[Ref.handle];

			// Release texture.
			Renderer->DestroyTexture(t);

			// Reset the array entry, ensure invalid ids are set.
			Memory::Zero(t, sizeof(Texture));
			t->Id = INVALID_ID;
			t->Generation = INVALID_ID;

			// Reset the reference.
			Ref.handle = INVALID_ID;
			Ref.auto_release = false;
			UL_INFO("Released texture '%s'. Texture unloaded.", NameCopy);
		}
		else {
			UL_INFO("Release texture '%s'. Now has a reference count of '%i' (auto release = %s)", NameCopy,
				Ref.reference_count, Ref.auto_release ? "True" : "False");
		}

		// Update the entry.
		RegisteredTextureTable.Set(NameCopy, &Ref);
	}
	else {
		UL_ERROR("Texture release failed to release texture '%s'.", name);
	}
}

void TextureSystem::DestroyTexture(Texture* t) {
	// Release texture.
	Renderer->DestroyTexture(t);

	Memory::Zero(t->Name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
	Memory::Zero(t, sizeof(Texture));
	t->Id = INVALID_ID;
	t->Generation = INVALID_ID;
}

Texture* TextureSystem::GetDefaultTexture() {
	if (Initilized) {
		return &DefaultTexture;
	}
	
	return nullptr;
}

Texture* TextureSystem::GetDefaultDiffuseTexture() {
	if (Initilized) {
		return &DefaultDiffuseTexture;
	}

	return nullptr;
}

Texture* TextureSystem::GetDefaultSpecularTexture() {
	if (Initilized) {
		return &DefaultSpecularTexture;
	}

	return nullptr;
}

Texture* TextureSystem::GetDefaultNormalTexture() {
	if (Initilized) {
		return &DefaultNormalTexture;
	}

	return nullptr;
}

bool TextureSystem::CreateDefaultTexture() {

	// NOTE: create default texture, a 256x256 blue/white checkerboard pattern.
	// This is done in code to eliminate asset dependencies.
	UL_INFO("Createing default texture...");
	const uint32_t TexDimension = 256;
	const uint32_t bpp = 4;
	const uint32_t PixelCount = TexDimension * TexDimension;
	unsigned char Pixels[PixelCount * bpp];

	Memory::Set(Pixels, 255, sizeof(unsigned char) * PixelCount * bpp);

	// Each pixel.
	for (size_t row = 0; row < TexDimension; row++) {
		for (size_t col = 0; col < TexDimension; col++) {
			size_t Index = (row * TexDimension) + col;
			size_t IndexBpp = Index * bpp;
			if (row % 2) {
				if (col % 2) {
					Pixels[IndexBpp + 0] = 0;
					Pixels[IndexBpp + 1] = 0;
				}
			}
			else {
				if (!(col % 2)) {
					Pixels[IndexBpp + 0] = 0;
					Pixels[IndexBpp + 1] = 0;
				}
			}
		}
	}

	strncpy(DefaultTexture.Name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
	DefaultTexture.Width = TexDimension;
	DefaultTexture.Height = TexDimension;
	DefaultTexture.ChannelCount = 4;
	DefaultTexture.Generation = INVALID_ID;
	DefaultTexture.HasTransparency = false;
	Renderer->CreateTexture(Pixels, &DefaultTexture);
	UL_INFO("Default texture created.");
	// Manually set the texture generation to invalid since this is a default texture.
	DefaultTexture.Generation = INVALID_ID;

	// Diffuse texture.
	UL_INFO("Creating default diffuse texture...");
	unsigned char DiffusePixels[16 * 16 * 4];
	// Default spec map is black (no specular).
	Memory::Set(DiffusePixels, 255, sizeof(unsigned char) * 16 * 16 * 4);
	strncpy(DefaultDiffuseTexture.Name, DEFAULT_DIFFUSE_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
	DefaultDiffuseTexture.Width = 16;
	DefaultDiffuseTexture.Height = 16;
	DefaultDiffuseTexture.ChannelCount = 4;
	DefaultDiffuseTexture.Generation = INVALID_ID;
	DefaultDiffuseTexture.HasTransparency = false;
	Renderer->CreateTexture(DiffusePixels, &DefaultDiffuseTexture);
	UL_INFO("Default diffuse texture created.");
	// Manually set the texture generation to invalid since this is a default texture.
	DefaultDiffuseTexture.Generation = INVALID_ID;

	// Specular texture.
	UL_INFO("Creating default specular texture...");
	unsigned char SpecularPixels[16 * 16 * 4];
	// Default spec map is black (no specular).
	Memory::Set(SpecularPixels, 0, sizeof(unsigned char) * 16 * 16 * 4);
	strncpy(DefaultSpecularTexture.Name, DEFAULT_SPECULAR_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
	DefaultSpecularTexture.Width = 16;
	DefaultSpecularTexture.Height = 16;
	DefaultSpecularTexture.ChannelCount = 4;
	DefaultSpecularTexture.Generation = INVALID_ID;
	DefaultSpecularTexture.HasTransparency = false;
	Renderer->CreateTexture(SpecularPixels, &DefaultSpecularTexture);
	UL_INFO("Default specular texture created.");
	// Manually set the texture generation to invalid since this is a default texture.
	DefaultSpecularTexture.Generation = INVALID_ID;

	// Normal texture.
	UL_INFO("Creating default normal texture...");
	unsigned char NormalPixels[16 * 16 * 4];
	// Default spec map is black (no specular).
	Memory::Set(NormalPixels, 0, sizeof(unsigned char) * 16 * 16 * 4);

	// Each pixel.
	for (size_t row = 0; row < 16; row++) {
		for (size_t col = 0; col < 16; col++) {
			uint32_t Index = (uint32_t)((row * 16) + col);
			uint32_t IndexBpp = Index * bpp;
			// Set blue, z-axis by default and alpha.
			NormalPixels[IndexBpp + 0] = 128;
			NormalPixels[IndexBpp + 1] = 128;
			NormalPixels[IndexBpp + 2] = 255;
			NormalPixels[IndexBpp + 3] = 255;
		}
	}

	strncpy(DefaultNormalTexture.Name, DEFAULT_NORMAL_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
	DefaultNormalTexture.Width = 16;
	DefaultNormalTexture.Height = 16;
	DefaultNormalTexture.ChannelCount = 4;
	DefaultNormalTexture.Generation = INVALID_ID;
	DefaultNormalTexture.HasTransparency = false;
	Renderer->CreateTexture(NormalPixels, &DefaultNormalTexture);
	UL_INFO("Default normal texture created.");
	// Manually set the texture generation to invalid since this is a default texture.
	DefaultNormalTexture.Generation = INVALID_ID;

	return true;
}

void TextureSystem::DestroyDefaultTexture() {
	DestroyTexture(&DefaultTexture);
	DestroyTexture(&DefaultDiffuseTexture);
	DestroyTexture(&DefaultSpecularTexture);
	DestroyTexture(&DefaultNormalTexture);
}


bool TextureSystem::LoadTexture(const char* name, Texture* texture) {
	Resource ImgResource;
	if (!ResourceSystem::Load(name, ResourceType::eResource_type_Image, &ImgResource)) {
		UL_ERROR("Failed to load image resource for texture '%s'.", name);
		return false;
	}

	ImageResourceData* ResourceData = (ImageResourceData*)ImgResource.Data;

	// Use a temporary texture to load into.
	Texture TempTexture;
	TempTexture.Width = ResourceData->width;
	TempTexture.Height= ResourceData->height;
	TempTexture.ChannelCount = ResourceData->channel_count;

		uint32_t CurrentGeneration = texture->Generation;
		texture->Generation = INVALID_ID;

		size_t TotalSize = TempTexture.Width * TempTexture.Height * TempTexture.ChannelCount;
		// Check for transparency.
		bool HasTransparency = false;
		for (size_t i = 0; i < TotalSize; i += TempTexture.ChannelCount) {
			unsigned char a = ResourceData->pixels[i + 3];
			if (a < 255) {
				HasTransparency = true;
				break;
			}
		}

		// Take a copy of the name
		strncpy(TempTexture.Name, name, TEXTURE_NAME_MAX_LENGTH);
		TempTexture.Generation = INVALID_ID;
		TempTexture.HasTransparency = HasTransparency;

		//Acquire internal texture resources and upload to GPU.
		Renderer->CreateTexture(ResourceData->pixels, &TempTexture);

		// Take a copy of the old texture.
		Texture Old = *texture;

		// Assign the temp texture to the pointer.
		*texture = TempTexture;

		// Destroy the old texture.
		Renderer->DestroyTexture(&Old);

		if (CurrentGeneration == INVALID_ID) {
			texture->Generation = 0;
		}
		else {
			texture->Generation = CurrentGeneration + 1;
		}

		// Clean up data.
		ResourceSystem::Unload(&ImgResource);
		return true;
}

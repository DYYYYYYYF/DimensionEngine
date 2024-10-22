#include "TextureSystem.h"

#include "Core/EngineLogger.hpp"
#include "Core/Application.hpp"

#include "Containers/TString.hpp"

#include "Systems/ResourceSystem.h"
#include "Systems/JobSystem.hpp"
#include "Renderer/RendererFrontend.hpp"

STextureSystemConfig TextureSystem::TextureSystemConfig;
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
		LOG_FATAL("Texture system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		LOG_FATAL("Texture system init failed. Renderer is nullptr.");
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
		LOG_FATAL("Create default texture failed. Application quit now!");
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
			LOG_DEBUG("Destroying texture: '%s'.", t->Name);
			Renderer->DestroyTexture(t);
		}
	}

	DestroyDefaultTexture();
}

Texture* TextureSystem::Acquire(const char* name, bool auto_release) {
	// Return default texture, but warn about it since this should be returned via GetDefaultTexture()
	if (StringEquali(name, DEFAULT_DIFFUSE_TEXTURE_NAME)) {
		LOG_WARN("Texture acquire return default texture. Use GetDefaultTexture() for texture 'DEFAULT_DIFFUSE_TEXTURE_NAME'");
		return &DefaultDiffuseTexture;
	}

	if (StringEquali(name, DEFAULT_NORMAL_TEXTURE_NAME)) {
		LOG_WARN("Texture acquire return default texture. Use GetDefaultTexture() for texture 'DEFAULT_NORMAL_TEXTURE_NAME'");
		return &DefaultNormalTexture;
	}

	if (StringEquali(name, DEFAULT_SPECULAR_TEXTURE_NAME)) {
		LOG_WARN("Texture acquire return default texture. Use GetDefaultTexture() for texture 'DEFAULT_SPECULAR_TEXTURE_NAME'");
		return &DefaultSpecularTexture;
	}

	uint32_t ID = INVALID_ID;
	// NOTE: Increments reference_cout, or creates new entry.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, 1, auto_release, false, &ID)) {
		LOG_ERROR("TextureSystem::Acquire() failed to obtain a new texture id.");
		return nullptr;
	}

	return &RegisteredTextures[ID];
}

Texture* TextureSystem::AcquireCube(const char* name, bool auto_release) {
	// Return default texture, but warn about it since this should be returned via GetDefaultTexture()
	if (strcmp(name, DEFAULT_DIFFUSE_TEXTURE_NAME) == 0) {
		LOG_WARN("Texture acquire cube return default texture. Use GetDefaultDiffuseTexture() for texture 'default'");
		return &DefaultDiffuseTexture;
	}

	uint32_t ID = INVALID_ID;
	// NOTE: Increments reference_cout, or creates new entry.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_Cube, 1, auto_release, false, &ID)) {
		LOG_ERROR("TextureSystem::AcquireCube() failed to obtain a new texture id.");
		return nullptr;
	}

	return &RegisteredTextures[ID];
}

Texture* TextureSystem::AcquireWriteable(const char* name, uint32_t width, uint32_t height, unsigned char channel_count, bool has_transparency){
	uint32_t ID = INVALID_ID;
	// NOTE: Wrapped textures are never auto-release because it means that thier
	// resources are created and managed somewhere within the renderer internals.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, 1, false, true, &ID)) {
		LOG_ERROR("TextureSystem::AcquireWriteable() failed to obtain a new texture id.");
		return nullptr;
	}
	
	Texture* t = &RegisteredTextures[ID];
	t->Id = ID;
	t->Type = TextureType::eTexture_Type_2D;
	strncpy(t->Name, name, TEXTURE_NAME_MAX_LENGTH);
	t->Width = width;
	t->Height = height;
	t->ChannelCount = channel_count;
	t->Generation = INVALID_ID;
	t->Flags |= has_transparency ? TextureFlagBits::eTexture_Flag_Has_Transparency : 0;
	t->Flags |= TextureFlagBits::eTexture_Flag_Is_Writeable;
	t->InternalData = nullptr;

	Renderer->CreateWriteableTexture(t);
	return t;
}

void TextureSystem::Release(const char* name) {
	// Ignore release requests for the default texture.
	if (strcmp(name, DEFAULT_DIFFUSE_TEXTURE_NAME) == 0 ||
		strcmp(name, DEFAULT_SPECULAR_TEXTURE_NAME) == 0 ||
		strcmp(name, DEFAULT_NORMAL_TEXTURE_NAME) == 0 ) {
		return;
	}

	uint32_t ID = INVALID_ID;
	// NOTE: Decrement the reference count.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, -1, false, false, &ID)) {
		LOG_ERROR("TextureSystem::Release() failed to release texture '%s' properly.", name);
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

void TextureSystem::WrapInternal(const char* name, uint32_t width, uint32_t height, 
	unsigned char channel_count, bool has_transparency, bool is_writeable, bool register_texture, void* internal_data, Texture* tex) {
	uint32_t ID = INVALID_ID;
	Texture* t = nullptr;
	if (register_texture) {
		// NOTE: Wrapped textures are never auto-release because it means that their
		// resource are created and managed somewhere within the renderer internals.
		if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, 1, false, true, &ID)) {
			LOG_ERROR("TextureSystem::WrapInternal() fialed to obtain a new texture id.");
			return;
		}

		t = &RegisteredTextures[ID];
	}
	else {
		if (tex) {
			t = tex;
		}
		else {
			t = (Texture*)Memory::Allocate(sizeof(Texture), MemoryType::eMemory_Type_Texture);
		}
		// LOG_INFO("TextureSystem::WrapInternal() created texture '%s', but not registering, resulting in an allocation. It is up to the caller for free this memory.", name);
	}

	t->Id = ID;
	t->Type = TextureType::eTexture_Type_2D;
	strncpy(t->Name, name, TEXTURE_NAME_MAX_LENGTH);
	t->Width = width;
	t->Height = height;
	t->ChannelCount = channel_count;
	t->Generation = INVALID_ID;
	t->Flags |= has_transparency ? TextureFlagBits::eTexture_Flag_Has_Transparency : 0;
	t->Flags |= is_writeable ? TextureFlagBits::eTexture_Flag_Is_Writeable : 0;
	t->Flags |= TextureFlagBits::eTexture_Flag_Is_Wrapped;
	t->InternalData = internal_data;

}

bool TextureSystem::SetInternal(Texture* t, void* internal_data) {
	if (t == nullptr) {
		return false;
	}

	t->InternalData = internal_data;
	t->Generation++;
	return true;
}

bool TextureSystem::Resize(Texture* t, uint32_t width, uint32_t height, bool regenerate_internal_data) {
	if (t == nullptr) {
		return false;
	}

	if (!(t->Flags & TextureFlagBits::eTexture_Flag_Is_Writeable)) {
		LOG_WARN("Texture system resize should not be called on textures that are not writeable.");
		return false;
	}

	t->Width = width;
	t->Height = height;
	// Only allow this for writeable textures that are not wrapped.
	// Wrapped textures can call TextureSystem::SetInternal() then call this function
	// to get the above parameter updates and a generation update.
	if (!(t->Flags & TextureFlagBits::eTexture_Flag_Is_Wrapped) && regenerate_internal_data) {
		// Regenerate internals for the new size.
		Renderer->ResizeTexture(t, width, height);
		return false;
	}

	t->Generation++;
	return true;
}

bool TextureSystem::WriteData(Texture* t, uint32_t offset, uint32_t size, void* data) {
	if (t == nullptr) {
		return false;
	}

	Renderer->WriteTextureData(t, offset, size, (unsigned char*)data);
	return true;
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
	LOG_INFO("Createing default texture...");
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

	strncpy(DefaultDiffuseTexture.Name, DEFAULT_DIFFUSE_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
	DefaultDiffuseTexture.Width = TexDimension;
	DefaultDiffuseTexture.Height = TexDimension;
	DefaultDiffuseTexture.ChannelCount = 4;
	DefaultDiffuseTexture.Generation = INVALID_ID;
	DefaultDiffuseTexture.Flags = 0;
	DefaultDiffuseTexture.Type = TextureType::eTexture_Type_2D;
	Renderer->CreateTexture(Pixels, &DefaultDiffuseTexture);
	LOG_INFO("Default diffuse texture created.");
	// Manually set the texture generation to invalid since this is a default texture.
	DefaultDiffuseTexture.Generation = INVALID_ID;

	// Specular texture.
	LOG_INFO("Creating default specular texture...");
	unsigned char SpecularPixels[16 * 16 * 4];
	// Default spec map is black (no specular).
	Memory::Set(SpecularPixels, 0, sizeof(unsigned char) * 16 * 16 * 4);
	strncpy(DefaultSpecularTexture.Name, DEFAULT_SPECULAR_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
	DefaultSpecularTexture.Width = 16;
	DefaultSpecularTexture.Height = 16;
	DefaultSpecularTexture.ChannelCount = 4;
	DefaultSpecularTexture.Generation = INVALID_ID;
	DefaultSpecularTexture.Flags = 0;
	DefaultSpecularTexture.Type = TextureType::eTexture_Type_2D;
	Renderer->CreateTexture(SpecularPixels, &DefaultSpecularTexture);
	LOG_INFO("Default specular texture created.");
	// Manually set the texture generation to invalid since this is a default texture.
	DefaultSpecularTexture.Generation = INVALID_ID;

	// Normal texture.
	LOG_INFO("Creating default normal texture...");
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
	DefaultNormalTexture.Flags = 0;
	DefaultNormalTexture.Type = TextureType::eTexture_Type_2D;
	Renderer->CreateTexture(NormalPixels, &DefaultNormalTexture);
	LOG_INFO("Default normal texture created.");
	// Manually set the texture generation to invalid since this is a default texture.
	DefaultNormalTexture.Generation = INVALID_ID;

	return true;
}

void TextureSystem::DestroyDefaultTexture() {
	DestroyTexture(&DefaultDiffuseTexture);
	DestroyTexture(&DefaultSpecularTexture);
	DestroyTexture(&DefaultNormalTexture);
}

bool TextureSystem::LoadCubeTexture(const char* name, const char texture_names[6][TEXTURE_NAME_MAX_LENGTH], Texture* t) {
	unsigned char* piexels = nullptr;
	size_t ImageSize = 0;
	for (unsigned char i = 0; i < 6; ++i) {
		ImageResourceParams Params;
		Params.flip_y = false;

		Resource ImageResource;
		if (!ResourceSystem::Load(texture_names[i], ResourceType::eResource_type_Image, &Params, &ImageResource)) {
			LOG_ERROR("TextureSystem::LoadCubeTexture() Failed to load image resource for texture '%s'.", texture_names[i]);
			return false;
		}

		ImageResourceData* ResourceData = (ImageResourceData*)ImageResource.Data;
		if (!piexels) {
			t->Width = ResourceData->width;
			t->Height = ResourceData->height;
			t->ChannelCount = ResourceData->channel_count;
			t->Flags = 0;
			t->Generation = 0;
			strncpy(t->Name, name, TEXTURE_NAME_MAX_LENGTH);
			ImageSize = t->Width * t->Height * t->ChannelCount;
			piexels = (unsigned char*)Memory::Allocate(ImageSize * sizeof(unsigned char) * 6, MemoryType::eMemory_Type_Array);
		}
		else {
			// verify all textures are the same size.
			if (t->Width != ResourceData->width || t->Height != ResourceData->height || t->ChannelCount != ResourceData->channel_count) {
				LOG_ERROR("TextureSystem::LoadCubeTexture() All textures must be the same resolution and bit depth.");
				Memory::Free(piexels, sizeof(unsigned char) * ImageSize * 6, MemoryType::eMemory_Type_Array);
				piexels = nullptr;
				return false;
			}
		}

		// Copy to the relevant portion of the array.
		Memory::Copy(piexels + sizeof(unsigned char) * ImageSize * i, ResourceData->pixels, ImageSize);

		// Clean up data.
		ResourceSystem::Unload(&ImageResource);
	}

	// Acquire internal texture resources and upload to GPU.
	Renderer->CreateTexture(piexels, t);

	Memory::Free(piexels, sizeof(unsigned char) * ImageSize * 6, MemoryType::eMemory_Type_Array);
	piexels = nullptr;
	return true;
}

void TextureSystem::LoadJobSuccess(void* params) {
	TextureLoadParams* TextureParams = (TextureLoadParams*)params;

	// This also handles the GPU upload. Can't be jobfied until the renderer is multithread.
	ImageResourceData* ResourceData = (ImageResourceData*)TextureParams->ImageResource.Data;

	// Acquire internal texture resources and upload to GPU. Can't be jobfied until renderer is multithread.
	Renderer->CreateTexture(ResourceData->pixels, &TextureParams->temp_texture);

	// Take a copy of the old texture.
	Texture Old = *TextureParams->out_texture;

	// Assign the temp texture to the pointer.
	uint32_t ID = TextureParams->out_texture->Id;
	*TextureParams->out_texture = TextureParams->temp_texture;
	TextureParams->out_texture->Id = ID;

	// Destroy the old texture.
	Renderer->DestroyTexture(&Old);
	Memory::Zero(&Old, sizeof(Texture));

	if (TextureParams->current_generation == INVALID_ID) {
		TextureParams->out_texture->Generation = 0;
	}
	else {
		TextureParams->out_texture->Generation = TextureParams->current_generation + 1;
	}

	LOG_INFO("Successfully loaded texture '%s.", TextureParams->resource_name);

	// Clean up data.
	ResourceSystem::Unload(&TextureParams->ImageResource);
	if (TextureParams->resource_name) {
		Memory::Free(TextureParams->resource_name, sizeof(char) * (strlen(TextureParams->resource_name) + 1), MemoryType::eMemory_Type_String);
		TextureParams->resource_name = nullptr;
	}
}

void TextureSystem::LoadJobFail(void* params) {
	TextureLoadParams* TextureParams = (TextureLoadParams*)params;
	LOG_ERROR("Failed to load texture '%s'.", TextureParams->resource_name);
	ResourceSystem::Unload(&TextureParams->ImageResource);
}

bool TextureSystem::LoadJobStart(void* params, void* result_data) {
	TextureLoadParams* LoadParams = (TextureLoadParams*)params;

	ImageResourceParams ResourceParams;
	ResourceParams.flip_y = true;

	bool Result = ResourceSystem::Load(LoadParams->resource_name, ResourceType::eResource_type_Image, &ResourceParams, &LoadParams->ImageResource);
	if (!Result) {
		return false;
	}

	ImageResourceData* ResourceData = (ImageResourceData*)LoadParams->ImageResource.Data;
	// Use a temporary texture to load into.
	LoadParams->temp_texture.Width = ResourceData->width;
	LoadParams->temp_texture.Height = ResourceData->height;
	LoadParams->temp_texture.ChannelCount = ResourceData->channel_count;
	LoadParams->temp_texture.Type = LoadParams->out_texture->Type;
	strncpy(LoadParams->temp_texture.Name, LoadParams->resource_name, strlen(LoadParams->resource_name) + 1);
	LoadParams->current_generation = LoadParams->out_texture->Generation;
	LoadParams->out_texture->Generation = INVALID_ID;

	size_t TotalSize = LoadParams->temp_texture.Width * LoadParams->temp_texture.Height * LoadParams->temp_texture.ChannelCount;
	// Check for transparency.
	bool HasTransparency = false;
	for (size_t i = 0; i < TotalSize; i += LoadParams->temp_texture.ChannelCount) {
		unsigned char a = ResourceData->pixels[i + 3];
		if (a < 255) {
			HasTransparency = true;
			break;
		}
	}

	// Take a copy of the name
	strncpy(LoadParams->temp_texture.Name, LoadParams->resource_name, TEXTURE_NAME_MAX_LENGTH);
	LoadParams->temp_texture.Generation = INVALID_ID;
	LoadParams->temp_texture.Flags |= HasTransparency ? TextureFlagBits::eTexture_Flag_Has_Transparency : 0;

	// NOTE: The load params are also used as the result data here, only the image_resource field is populated now.
	Memory::Copy(result_data, LoadParams, sizeof(TextureLoadParams));

	return Result;
}

bool TextureSystem::LoadTexture(const char* name, Texture* texture) {
	// Kick off a texture loading job. Only handles loading from disk to CPU.
	// GPU upload is handled after completion of this job.
	TextureLoadParams Params;
	Params.resource_name = StringCopy(name);
	Params.out_texture = texture;
	Params.current_generation = texture->Generation;

	JobInfo Job = JobSystem::CreateJob(
		std::bind(&TextureSystem::LoadJobStart, std::placeholders::_1, std::placeholders::_2),
		std::bind(&TextureSystem::LoadJobSuccess, std::placeholders::_1),
		std::bind(&TextureSystem::LoadJobFail, std::placeholders::_1),
		&Params, 
		sizeof(TextureLoadParams),
		sizeof(TextureLoadParams));
	JobSystem::Submit(Job);
	return true;
}

bool TextureSystem::ProcessTextureReference(const char* name, TextureType type ,
	short reference_diff, bool auto_release, bool skip_load, uint32_t* out_texture_id) {
	*out_texture_id = INVALID_ID;
	if (!Initilized) {
		return false;
	}

	STextureReference Ref;
	if (RegisteredTextureTable.Get(name, &Ref)) {
		// If the reference count starts off at zero, one of two things can be true.
		// If incrementing references, this means the entry is new. If decrementing,
		// then the texture doesn't exist _if_ not auto-release.
		if (Ref.reference_count == 0 && reference_diff > 0) {
			if (reference_diff > 0) {
				// This can only be changed the first time a texture is loaded.
				Ref.auto_release = auto_release;
			}
			else {
				if (Ref.auto_release) {
					LOG_WARN("Tried to release non-existent texture: '%s'.", name);
					return false;
				}
				else {
					LOG_WARN("Tried to release a texture where auto-release=false, but references was already: 0.");
					// Still count this as a success but warn about it.
					return true;
				}
			}
		}

		Ref.reference_count += reference_diff;

		// Take a copy of the name since it would be wiped out if destroyed,
		// (as passed in name is generally a pointer to the actual texture's name).
		char NameCopy[TEXTURE_NAME_MAX_LENGTH];
		strncpy(NameCopy, name, TEXTURE_NAME_MAX_LENGTH);

		// If decrementing, this means a release.
		if (reference_diff < 0) {
			// Check if the reference count has reached 0. If it has, and the reference
			// is set to auto-release, destroy the texture.
			if (Ref.reference_count == 0 && Ref.auto_release) {
				Texture* t = &RegisteredTextures[Ref.handle];

				// Destroy/reset texture.
				DestroyTexture(t);

				// Reset the reference.
				Ref.handle = INVALID_ID;
				Ref.auto_release = false;
				LOG_DEBUG("Released texture '%s', Texture unloaded because count=0 and auto_release=true.", NameCopy);
			}
		}
		else {
			// Incrementing. Check if the handle is now or not.
			if (Ref.handle == INVALID_ID) {
				// This means no texture exists here. Find a free index first.
				uint32_t Count = TextureSystemConfig.max_texture_count;
				for (uint32_t i = 0; i < Count; ++i) {
					if (RegisteredTextures[i].Id == INVALID_ID) {
						// A free slot has been found. Use its index as the handle.
						Ref.handle = i;
						*out_texture_id = i;
						break;
					}
				}

				// An empty slot was not found, bleat about it and boot out.
				if (*out_texture_id == INVALID_ID) {
					LOG_FATAL("ProcessTextureReference() texture system can not hold anymore textures. Adjust configuration to allow more.");
					return false;
				}
				else {
					Texture* t = &RegisteredTextures[Ref.handle];
					t->Type = type;
					// Create new texture.
					if (skip_load) {
						LOG_DEBUG("Load skipped for texture '%s'. This is expected behaviour.", name);
					}
					else {
						if (type == TextureType::eTexture_Type_2D) {
							if (!LoadTexture(name, t)) {
								*out_texture_id = INVALID_ID;
								LOG_ERROR("Failed to load texture '%s'.", name);
								return false;
							}
						}
						else {
							char TextureNames[6][TEXTURE_NAME_MAX_LENGTH];
							
							// +x,-X,+y,-Y,+Z,-Z in _cubemap_ space, which is LH y-down.
							StringFormat(TextureNames[0], 512, "%s_r", name);		// Right texture.
							StringFormat(TextureNames[1], 512, "%s_l", name);		// Left texture.
							StringFormat(TextureNames[2], 512, "%s_u", name);		// Up texture.
							StringFormat(TextureNames[3], 512, "%s_d", name);		// Down texture.
							StringFormat(TextureNames[4], 512, "%s_f", name);		// Front texture.
							StringFormat(TextureNames[5], 512, "%s_b", name);		// Back texture.

							if (!LoadCubeTexture(name, TextureNames, t)) {
								*out_texture_id = INVALID_ID;
								LOG_ERROR("Failed to load cube texture '%s'.", name);
								return false;
							}
						}
						
						t->Id = Ref.handle;
					}
					UL_DEBUG("Texture '%s' does not yet exist. Created, and ref_count is now %i.", name, Ref.reference_count);
				}
			}
			else {
				*out_texture_id = Ref.handle;
				UL_DEBUG("Texture '%s' already exists, ref_count increased to %i.", name, Ref.reference_count);
			}
		}

		// Either way, update the entry.
		RegisteredTextureTable.Set(NameCopy, &Ref);
		return true;
	}

	// NOTE: This would only happen in the event something went wrong with the state.
	LOG_ERROR("ProcessTextureReference() failed to acquire id for name '%s'.", name);
	return false;
}
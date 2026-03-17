#include "TextureSystem.h"

#include "Core/Engine.hpp"

#include "Containers/TString.hpp"

#include "Systems/JobSystem.hpp"
#include "Rendering/Renderer.hpp"
#include "Rendering/Resources/Texture/Loader/TextureHelper.hpp"

TextureSystem& TextureSystem::Get() {
	static TextureSystem TextureSystemInstance;
	return TextureSystemInstance;
}

bool TextureSystem::Initialize(IRenderer* renderer, STextureSystemConfig config) {
	if (config.max_texture_count == 0) {
		GLOG(Log::eFatal, "Texture system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		GLOG(Log::eFatal, "Texture system init failed. Renderer is nullptr.");
		return false;
	}

	if (Initilized) {
		return true;
	}

	TextureSystemConfig = config;
	Renderer = renderer;

	// Create default textures for use in the system.
	if (!CreateDefaultTexture()) {
		GLOG(Log::eFatal, "Create default texture failed. Application quit now!");
		return false;
	}

	Initilized = true;
	return true;
}

void TextureSystem::Shutdown() {
	// Destroy all loaded textures.
	for (auto& PairsTex : TextureMap) {
		UTexture* tex = PairsTex.second;
		if (tex != nullptr && tex->Generation != INVALID_ID) {
			GLOG(Log::eDebug, "Destroying texture: '%s'.", tex->GetName().CStr());
			tex->Destroy();
			DeleteObject(tex);
		}
	}

	TextureMap.clear();

	DestroyDefaultTexture();
}

UTexture* TextureSystem::Acquire(const FString& name, bool auto_release) {
	// Return default texture, but warn about it since this should be returned via GetDefaultTexture()
	UTexture* OutTexture = CheckTextureName(name);
	if (OutTexture) {
		return OutTexture;
	}

	// NOTE: Increments reference_cout, or creates new entry.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, 1, auto_release, false)) {
		GLOG(Log::eError, "TextureSystem::Acquire() failed to obtain a new texture id.");
		return nullptr;
	}

	OutTexture = TextureMap[name];
	if (OutTexture == nullptr) {
		GLOG(Log::eError, "TextureSystem::Acquire() failed to get texture.");
		return nullptr;
	}

	OutTexture->SetName(name);

	return OutTexture;
}

UTexture* TextureSystem::AcquireCube(const FString& name, bool auto_release) {
	// Return default texture, but warn about it since this should be returned via GetDefaultTexture()
	UTexture* OutTexture = CheckTextureName(name);
	if (OutTexture) {
		return OutTexture;
	}

	// NOTE: Increments reference_cout, or creates new entry.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_Cube, 1, auto_release, false)) {
		GLOG(Log::eError, "TextureSystem::AcquireCube() failed to obtain a new texture id.");
		return nullptr;
	}

	OutTexture = TextureMap[name];
	if (OutTexture == nullptr) {
		GLOG(Log::eError, "TextureSystem::Acquire() failed to get texture.");
		return nullptr;
	}

	return OutTexture;
}

UTexture* TextureSystem::AcquireWriteable(const FString& name, uint32_t width, uint32_t height,
	unsigned char channel_count, bool has_transparency, bool has_depth){
	uint32_t ID = INVALID_ID;
	// NOTE: Wrapped textures are never auto-release because it means that their
	// resources are created and managed somewhere within the renderer internals.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, 1, false, true)) {
		GLOG(Log::eError, "TextureSystem::AcquireWriteable() failed to obtain a new texture id.");
		return nullptr;
	}
	
	UTexture* t = TextureMap[name];
	t->SetID(ID);
	t->Type = TextureType::eTexture_Type_2D;
	t->SetName(name);
	t->Width = width;
	t->Height = height;
	t->ChannelCount = channel_count;
	t->Generation = INVALID_ID;
	t->Flags |= has_transparency ? TextureFlagBits::eTexture_Flag_Has_Transparency : 0;
	t->Flags |= has_depth ? TextureFlagBits::eTexture_Flag_Depth : 0;
	t->Flags |= TextureFlagBits::eTexture_Flag_Is_Writeable;
	t->LoadWriteable();

	return t;
}

void TextureSystem::Release(const FString& name) {
	// Ignore release requests for the default texture.
	if (strcmp(name.CStr(), DEFAULT_DIFFUSE_TEXTURE_NAME) == 0 ||
		strcmp(name.CStr(), DEFAULT_SPECULAR_TEXTURE_NAME) == 0 ||
		strcmp(name.CStr(), DEFAULT_NORMAL_TEXTURE_NAME) == 0 ||
		strcmp(name.CStr(), DEFAULT_ROUGHNESS_METALLIC_TEXTURE_NAME) == 0
	){
		return;
	}

	// NOTE: Decrement the reference count.
	if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, -1, false, false)) {
		GLOG(Log::eError, "TextureSystem::Release() failed to release texture '%s' properly.", name.CStr());
	}
}

void TextureSystem::DestroyTexture(UTexture* t) {
	// Release texture.
	t->Destroy();
}

void TextureSystem::WrapInternal(const FString& name, uint32_t width, uint32_t height, 
	unsigned char channel_count, bool has_transparency, bool is_writeable, bool register_texture, UTexture* tex) {
	uint32_t ID = INVALID_ID;
	UTexture* t = nullptr;
	if (register_texture) {
		// NOTE: Wrapped textures are never auto-release because it means that their
		// resource are created and managed somewhere within the renderer internals.
		if (!ProcessTextureReference(name, TextureType::eTexture_Type_2D, 1, false, true)) {
			GLOG(Log::eError, "TextureSystem::WrapInternal() fialed to obtain a new texture id.");
			return;
		}

		t = TextureMap[name];
	}
	else {
		if (tex) {
			t = tex;
		}
		else {
			t = (UTexture*)Memory::Allocate(sizeof(UTexture), MemoryType::eMemory_Type_Texture);
		}
	}

	t->SetID(ID);
	t->Type = TextureType::eTexture_Type_2D;
	t->Width = width;
	t->Height = height;
	t->ChannelCount = channel_count;
	t->Generation = INVALID_ID;
	t->Flags |= has_transparency ? TextureFlagBits::eTexture_Flag_Has_Transparency : 0;
	t->Flags |= is_writeable ? TextureFlagBits::eTexture_Flag_Is_Writeable : 0;
	t->Flags |= TextureFlagBits::eTexture_Flag_Is_Wrapped;

}

bool TextureSystem::SetInternal(UTexture* t) {
	if (t == nullptr) {
		return false;
	}

	t->Generation++;
	return true;
}

bool TextureSystem::Resize(UTexture* t, uint32_t width, uint32_t height, bool regenerate_internal_data) {
	if (t == nullptr) {
		return false;
	}

	if (!(t->Flags & TextureFlagBits::eTexture_Flag_Is_Writeable)) {
		GLOG(Log::eWarn, "Texture system resize should not be called on textures that are not writeable.");
		return false;
	}

	t->Width = width;
	t->Height = height;
	// Only allow this for writeable textures that are not wrapped.
	// Wrapped textures can call TextureSystem::SetInternal() then call this function
	// to get the above parameter updates and a generation update.
	if (!(t->Flags & TextureFlagBits::eTexture_Flag_Is_Wrapped) && regenerate_internal_data) {
		// Regenerate internals for the new size.
		t->Resize(width, height);
		return false;
	}

	t->Generation++;
	return true;
}

UTexture* TextureSystem::GetDefaultDiffuseTexture() {
	if (Initilized) {
		return DefaultDiffuseTexture;
	}

	return nullptr;
}

UTexture* TextureSystem::GetDefaultSpecularTexture() {
	if (Initilized) {
		return DefaultSpecularTexture;
	}

	return nullptr;
}

UTexture* TextureSystem::GetDefaultNormalTexture() {
	if (Initilized) {
		return DefaultNormalTexture;
	}

	return nullptr;
}

UTexture* TextureSystem::GetDefaultRoughnessMetallicTexture() {
	if (Initilized) {
		return DefaultRoughnessMetallicTexture;
	}

	return nullptr;
}

bool TextureSystem::CreateDefaultTexture() {
	// NOTE: create default texture, a 256x256 blue/white checkerboard pattern.
	// This is done in code to eliminate asset dependencies.
	GLOG(Log::eInfo, "Createing default texture...");
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

	// Diffuse texture.
	if (DefaultDiffuseTexture == nullptr) {
		DefaultDiffuseTexture = Renderer->AcquireTexture(DEFAULT_DIFFUSE_TEXTURE_NAME);
		DefaultDiffuseTexture->Width = TexDimension;
		DefaultDiffuseTexture->Height = TexDimension;
		DefaultDiffuseTexture->ChannelCount = 4;
		DefaultDiffuseTexture->Generation = INVALID_ID;
		DefaultDiffuseTexture->Flags = 0;
		DefaultDiffuseTexture->Type = TextureType::eTexture_Type_2D;
		DefaultDiffuseTexture->Load(Pixels);
		GLOG(Log::eInfo, "Default diffuse texture created.");
		// Manually set the texture generation to invalid since this is a default texture.
		DefaultDiffuseTexture->Generation = INVALID_ID;
	}

	// Specular texture.
	if (DefaultSpecularTexture == nullptr) {
		GLOG(Log::eInfo, "Creating default specular texture...");
		unsigned char SpecularPixels[16 * 16 * 4];
		// Default spec map is black (no specular).
		Memory::Set(SpecularPixels, 0, sizeof(unsigned char) * 16 * 16 * 4);
		DefaultSpecularTexture = Renderer->AcquireTexture(DEFAULT_SPECULAR_TEXTURE_NAME);
		DefaultSpecularTexture->Width = 16;
		DefaultSpecularTexture->Height = 16;
		DefaultSpecularTexture->ChannelCount = 4;
		DefaultSpecularTexture->Generation = INVALID_ID;
		DefaultSpecularTexture->Flags = 0;
		DefaultSpecularTexture->Type = TextureType::eTexture_Type_2D;
		DefaultSpecularTexture->Load(SpecularPixels);
		GLOG(Log::eInfo, "Default specular texture created.");
		// Manually set the texture generation to invalid since this is a default texture.
		DefaultSpecularTexture->Generation = INVALID_ID;
	}
	
	// Normal texture.
	if (DefaultNormalTexture == nullptr) {
		GLOG(Log::eInfo, "Creating default normal texture...");
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

		DefaultNormalTexture = Renderer->AcquireTexture(DEFAULT_NORMAL_TEXTURE_NAME);
		DefaultNormalTexture->SetName(DEFAULT_NORMAL_TEXTURE_NAME);
		DefaultNormalTexture->Width = 16;
		DefaultNormalTexture->Height = 16;
		DefaultNormalTexture->ChannelCount = 4;
		DefaultNormalTexture->Generation = INVALID_ID;
		DefaultNormalTexture->Flags = 0;
		DefaultNormalTexture->Type = TextureType::eTexture_Type_2D;
		DefaultNormalTexture->Load(NormalPixels);
		GLOG(Log::eInfo, "Default normal texture created.");
		// Manually set the texture generation to invalid since this is a default texture.
		DefaultNormalTexture->Generation = INVALID_ID;

	}
	
	// Roughness metallic texture.
	if (DefaultRoughnessMetallicTexture == nullptr) {
		GLOG(Log::eInfo, "Creating default roughness metallic texture...");
		unsigned char RoughnessMetallicPixels[16 * 16 * 4];
		// Default spec map is black (no specular).
		Memory::Set(RoughnessMetallicPixels, 0, sizeof(unsigned char) * 16 * 16 * 4);

		// Each pixel.
		for (size_t row = 0; row < 16; row++) {
			for (size_t col = 0; col < 16; col++) {
				uint32_t Index = (uint32_t)((row * 16) + col);
				uint32_t IndexBpp = Index * bpp;
				// Set blue, z-axis by default and alpha.
				RoughnessMetallicPixels[IndexBpp + 0] = static_cast<unsigned char>(1.0 * 255); // ao 
				RoughnessMetallicPixels[IndexBpp + 1] = static_cast<unsigned char>(0.0 * 255); // metallic 
				RoughnessMetallicPixels[IndexBpp + 2] = static_cast<unsigned char>(0.5 * 255); // roughness 
				RoughnessMetallicPixels[IndexBpp + 3] = static_cast<unsigned char>(1.0 * 255);
			}
		}

		DefaultRoughnessMetallicTexture = Renderer->AcquireTexture(DEFAULT_ROUGHNESS_METALLIC_TEXTURE_NAME);
		DefaultRoughnessMetallicTexture->Width = 16;
		DefaultRoughnessMetallicTexture->Height = 16;
		DefaultRoughnessMetallicTexture->ChannelCount = 4;
		DefaultRoughnessMetallicTexture->Generation = INVALID_ID;
		DefaultRoughnessMetallicTexture->Flags = 0;
		DefaultRoughnessMetallicTexture->Type = TextureType::eTexture_Type_2D;
		DefaultRoughnessMetallicTexture->Load(RoughnessMetallicPixels);
		GLOG(Log::eInfo, "Default roughness metallic texture created.");
		// Manually set the texture generation to invalid since this is a default texture.
		DefaultRoughnessMetallicTexture->Generation = INVALID_ID;

	}
	
	return true;
}

void TextureSystem::DestroyDefaultTexture() {
	if (DefaultDiffuseTexture) {
		DestroyTexture(DefaultDiffuseTexture);
		DeleteObject(DefaultDiffuseTexture);
		DefaultDiffuseTexture = nullptr;
	}
	if (DefaultSpecularTexture) {
		DestroyTexture(DefaultSpecularTexture);
		DeleteObject(DefaultSpecularTexture);
		DefaultSpecularTexture = nullptr;
	}
	if (DefaultNormalTexture) {
		DestroyTexture(DefaultNormalTexture);
		DeleteObject(DefaultNormalTexture);
		DefaultNormalTexture = nullptr;
	}
	if (DefaultRoughnessMetallicTexture) {
		DestroyTexture(DefaultRoughnessMetallicTexture);
		DeleteObject(DefaultRoughnessMetallicTexture);
		DefaultRoughnessMetallicTexture = nullptr;
	}
}

bool TextureSystem::LoadCubeTexture(const FString& name, const TArray<FString>& texture_names, UTexture* t) {
	ASSERT(texture_names.Size() == 6);

	unsigned char* piexels = nullptr;
	size_t ImageSize = 0;
	for (unsigned char i = 0; i < 6; ++i) {
		ImageResourceParams Params;
		Params.flip_y = false;

		UTexture* ImageResource = Renderer->AcquireTexture(texture_names[i]);
		if (!TextureHelper::Load(texture_names[i], &Params, ImageResource)) {
			GLOG(Log::eError, "TextureSystem::LoadCubeTexture() Failed to load image resource for texture '%s'.", texture_names[i].CStr());
			return false;
		}

		if (!piexels) {
			t->Width = ImageResource->GetWidth();
			t->Height = ImageResource->GetHeight();
			t->ChannelCount = ImageResource->GetChannelCount();
			t->Flags = 0;
			t->Generation = 0;
			t->SetName(name);
			ImageSize = t->GetWidth() * t->GetHeight() * t->GetChannelCount();
			piexels = (unsigned char*)Memory::Allocate(ImageSize * sizeof(unsigned char) * 6, MemoryType::eMemory_Type_Array);
		}

		// Copy to the relevant portion of the array.
		Memory::Copy(piexels + sizeof(unsigned char) * ImageSize * i, ImageResource->GetPixels(), ImageSize);

		// Clean up data.
		TextureHelper::Unload(ImageResource);

		DestroyTexture(ImageResource);
		ImageResource = nullptr;
	}

	// Acquire internal texture resources and upload to GPU.
	t->Load(piexels);

	Memory::Free(piexels, MemoryType::eMemory_Type_Array);
	piexels = nullptr;
	return true;
}

void TextureSystem::LoadJobSuccess(void* params) {
	TextureLoadParams* TextureParams = (TextureLoadParams*)params;

	// Acquire internal texture resources and upload to GPU. Can't be jobfied until renderer is multithread.
	TextureParams->out_texture->Load(TextureParams->out_texture->GetPixels());

	if (TextureParams->current_generation == INVALID_ID) {
		TextureParams->out_texture->Generation = 0;
	}
	else {
		TextureParams->out_texture->Generation = TextureParams->current_generation + 1;
	}

	GLOG(Log::eInfo, "Successfully loaded texture '%s.", TextureParams->resource_name.CStr());
}

void TextureSystem::LoadJobFail(void* params) {
	TextureLoadParams* TextureParams = (TextureLoadParams*)params;
	GLOG(Log::eError, "Failed to load texture '%s'.", TextureParams->resource_name.CStr());
	TextureHelper::Unload(TextureParams->out_texture);
}

bool TextureSystem::LoadJobStart(void* params, void* result_data) {
	TextureLoadParams* LoadParams = (TextureLoadParams*)params;

	ImageResourceParams ResourceParams;
	ResourceParams.flip_y = true;
	
	ASSERT(LoadParams->out_texture);
	bool Result = TextureHelper::Load(LoadParams->resource_name, &ResourceParams, LoadParams->out_texture);
	if (!Result) {
		return false;
	}

	// Use a temporary texture to load into.
	LoadParams->current_generation = LoadParams->out_texture->Generation;
	LoadParams->out_texture->Generation = INVALID_ID;

	size_t TotalSize = LoadParams->out_texture->GetSize();
	// Check for transparency.
	bool HasTransparency = false;
	unsigned char* RawPixels = LoadParams->out_texture->GetPixels();
	for (size_t i = 0; i < TotalSize; i += LoadParams->out_texture->GetChannelCount()) {
		unsigned char a = RawPixels[i + 3];
		if (a < 255) {
			HasTransparency = true;
			break;
		}
	}

	// Take a copy of the name
	LoadParams->out_texture->Generation = INVALID_ID;
	LoadParams->out_texture->Flags |= HasTransparency ? TextureFlagBits::eTexture_Flag_Has_Transparency : 0;

	// NOTE: The load params are also used as the result data here, only the image_resource field is populated now.
	Memory::Copy(result_data, LoadParams, sizeof(TextureLoadParams));
	((TextureLoadParams*)result_data)->resource_name = LoadParams->resource_name;

	return Result;
}

UTexture* TextureSystem::CheckTextureName(const FString& name) {
	if (name.Equali(DEFAULT_DIFFUSE_TEXTURE_NAME)) {
		GLOG(Log::eWarn, "Texture acquire return default texture. Use GetDefaultTexture() for texture 'DEFAULT_DIFFUSE_TEXTURE_NAME'");
		return DefaultDiffuseTexture;
	}

	if (name.Equali(DEFAULT_NORMAL_TEXTURE_NAME)) {
		GLOG(Log::eWarn, "Texture acquire return default texture. Use GetDefaultTexture() for texture 'DEFAULT_NORMAL_TEXTURE_NAME'");
		return DefaultNormalTexture;
	}

	if (name.Equali(DEFAULT_SPECULAR_TEXTURE_NAME)) {
		GLOG(Log::eWarn, "Texture acquire return default texture. Use GetDefaultTexture() for texture 'DEFAULT_SPECULAR_TEXTURE_NAME'");
		return DefaultSpecularTexture;
	}

	if (name.Equali(DEFAULT_ROUGHNESS_METALLIC_TEXTURE_NAME)) {
		GLOG(Log::eWarn, "Texture acquire return default texture. Use GetDefaultTexture() for texture 'DEFAULT_ROUGHNESS_METALLIC_TEXTURE_NAME'");
		return DefaultRoughnessMetallicTexture;
	}

	return nullptr;
}

bool TextureSystem::LoadTexture(const FString& name, UTexture* texture) {
	// Kick off a texture loading job. Only handles loading from disk to CPU.
	// GPU upload is handled after completion of this job.
	TextureLoadParams Params;
	Params.resource_name = name;
	Params.out_texture = texture;
	Params.current_generation = texture->Generation;

	JobInfo Job = JobSystem::CreateJob(
		std::bind(&TextureSystem::LoadJobStart, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&TextureSystem::LoadJobSuccess, this, std::placeholders::_1),
		std::bind(&TextureSystem::LoadJobFail, this, std::placeholders::_1),
		std::make_shared<TextureLoadParams>(Params),
		sizeof(TextureLoadParams),
		sizeof(TextureLoadParams));
	JobSystem::Submit(Job);
	return true;
}

bool TextureSystem::ProcessTextureReference(const FString& name, TextureType type ,
	short reference_diff, bool auto_release, bool skip_load) {
	if (!Initilized) {
		return false;
	}

	UTexture* Tex = TextureMap[name];
	// 创建新贴图资源
	if (Tex == nullptr) {
		// This means no texture exists here. Find a free index first.
		uint32_t Count = TextureSystemConfig.max_texture_count;
		for (uint32_t i = 0; i < Count; ++i) {
			if (Tex == nullptr) {
				// A free slot has been found. Use its index as the handle.
				Tex = Renderer->AcquireTexture(name);
				Tex->SetID(i);
				// Either way, update the entry.
				TextureMap[name] = Tex;
				break;
			}
		}
        
        if (Tex == nullptr){
            GLOG(Log::eError, "TextureSystem::ProcessTextureReference() There is not enough space to create new texture.");
            return false;
        }

		// An empty slot was not found, bleat about it and boot out.
		if (Tex->GetID() == INVALID_ID) {
			GLOG(Log::eFatal, "ProcessTextureReference() texture system can not hold anymore textures. Adjust configuration to allow more.");
			return false;
		}
		else {
			Tex->Type = type;
			// Create new texture.
			if (skip_load) {
				GLOG(Log::eDebug, "Load skipped for texture '%s'. This is expected behaviour.", name.CStr());
			}
			else {
				if (type == TextureType::eTexture_Type_2D) {
					if (!LoadTexture(name, Tex)) {
						GLOG(Log::eError, "Failed to load texture '%s'.", name.CStr());
						return false;
					}
				}
				else {
					TArray<FString> TextureNames;

					// +x,-X,+y,-Y,+Z,-Z in _cubemap_ space, which is LH y-down.
					TextureNames.Push(FString::Format("%s_r", name.CStr()));	// Right texture.
					TextureNames.Push(FString::Format("%s_l", name.CStr()));	// Left texture.
					TextureNames.Push(FString::Format("%s_u", name.CStr()));	// Up texture.
					TextureNames.Push(FString::Format("%s_d", name.CStr()));	// Down texture.
					TextureNames.Push(FString::Format("%s_f", name.CStr()));	// Front texture.
					TextureNames.Push(FString::Format("%s_b", name.CStr()));	// Back texture.

					if (!LoadCubeTexture(name, TextureNames, Tex)) {
						GLOG(Log::eError, "Failed to load cube texture '%s'.", name.CStr());
						return false;
					}
				}
			}
			GLOG(Log::eDebug, "Texture '%s' does not yet exist. Created, and ref_count is now %i.", name.CStr(), Tex->GetReferenceCount());
		}
	}

	// If the reference count starts off at zero, one of two things can be true.
	// If incrementing references, this means the entry is new. If decrementing,
	// then the texture doesn't exist _if_ not auto-release.
	if (Tex->GetReferenceCount() == 0 && reference_diff > 0) {
		if (reference_diff > 0) {
			// This can only be changed the first time a texture is loaded.
			Tex->SetIsAutoRelease(auto_release);
		}
		else {
			if (Tex->IsAutoRelease()) {
				GLOG(Log::eWarn, "Tried to release non-existent texture: '%s'.", name.CStr());
				return false;
			}
			else {
				GLOG(Log::eWarn, "Tried to release a texture where auto-release=false, but references was already: 0.");
				// Still count this as a success but warn about it.
				return true;
			}
		}
	}

	Tex->IncreaseReferenceCount(reference_diff);

	// Take a copy of the name since it would be wiped out if destroyed,
	// (as passed in name is generally a pointer to the actual texture's name).
	FString NameCopy = name;

	// If decrementing, this means a release.
	if (reference_diff < 0) {
		// Check if the reference count has reached 0. If it has, and the reference
		// is set to auto-release, destroy the texture.
		if (Tex->GetReferenceCount() == 0 && Tex->IsAutoRelease()) {
			// Destroy/reset texture.
			DestroyTexture(Tex);

			// Reset the reference.
			Tex->SetID(INVALID_ID);
			Tex->SetIsAutoRelease(false);
			GLOG(Log::eDebug, "Released texture '%s', Texture unloaded because count=0 and auto_release=true.", NameCopy.CStr());
		}
	}
	else {
		// Incrementing. Check if the handle is now or not.
		GLOG(Log::eDebug, "Texture '%s' already exists, ref_count increased to %i.", name.CStr(), Tex->GetReferenceCount());
	}

	return true;
}

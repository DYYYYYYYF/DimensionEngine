#include "ImageLoader.hpp"
#include "Systems/ResourceSystem.h"

#include "Platform/FileSystem.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

// Use our own filesystem
#ifndef STBI_NO_STDIO
#define STBI_NO_STDIO
#endif STBI_NO_STDIO

#include "stb_image.h"
#include "../Texture/Texture.hpp"

ImageLoader::ImageLoader() {
	Type = EAssetType::Texture;
	TypePath = "Textures";
}

bool ImageLoader::Load(const FString& name, void* params, UAsset* resource) {
	if (name.Length() == 0 || resource == nullptr) {
		return false;
	}

	UTexture* TexAsset = (UTexture*)resource;
	if (!TexAsset) {
		return false;
	}

	ImageResourceParams* TypedParams = (ImageResourceParams*)params;
	if (params == nullptr) {
		GLOG(Log::eError, "ImageLoader::Load() Required a valid params (ImageResourceParams).");
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	const int RequiredChannelCount = 4;
	stbi_set_flip_vertically_on_load_thread(TypedParams->flip_y);
	char FullFilePath[512];

	// Try different extensions.
#define IMAGE_EXTENSION_COUNT 4
	bool Found = false;
	const char* Extensions[IMAGE_EXTENSION_COUNT] = { ".tga", ".png", ".jpg", ".bmp" };
	for (uint32_t i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
		StringFormat(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath.c_str(), name.CStr(), Extensions[i]);
		if (FileSystemExists(FullFilePath)) {
			Found = true;
			break;
		}
	}

	if (!Found) {
		GLOG(Log::eError, "Image resource loader failed find file '%s'or with any supported extensions.", name.CStr());
		return false;
	}

	// Take a copy of the resource full path and name first.
	TexAsset->SetName(name);
	TexAsset->SetFullPath(FullFilePath);

	FileHandle f;
	if (!FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, true, &f)) {
		GLOG(Log::eError, "Unable to read file: %s.", FullFilePath);
		FileSystemClose(&f);
		return false;
	}

	size_t FileSize = 0;
	if (!FileSystemSize(&f, &FileSize)) {
		GLOG(Log::eError, "Unable to get size of file: %s.", FullFilePath);
		FileSystemClose(&f);
		return false;
	}

	int Width, Height, ChannelCount;
	unsigned char* RawData = (unsigned char*)Memory::Allocate(FileSize, MemoryType::eMemory_Type_Texture);
	if (RawData == nullptr) {
		GLOG(Log::eError, "Image resource loader failed to load file: '%s'.", FullFilePath);
		FileSystemClose(&f);
		return false;
	}

	size_t BytesRead = 0;
	bool ReadResult = FileSystemReadAllBytes(&f, RawData, &BytesRead);
	FileSystemClose(&f);

	if (!ReadResult) {
		GLOG(Log::eError, "Unable to read file: '%s'.", FullFilePath);
		return false;
	}

	if (BytesRead != FileSize) {
		GLOG(Log::eError, "File size if %llu does not match expected: %llu.", BytesRead, FileSize);
		return false;
	}
	
	unsigned char* Data = stbi_load_from_memory(RawData, (int)FileSize, &Width, &Height, &ChannelCount, RequiredChannelCount);
	if (Data == nullptr) {
		GLOG(Log::eError, "Image resource loader failed to load file: '%s'.", FullFilePath);
		return false;
	}

	ImageResourceData* ResourceData = (ImageResourceData*)Memory::Allocate(sizeof(ImageResourceData), MemoryType::eMemory_Type_Texture);
	TexAsset->SetPixels(Data);
	TexAsset->SetWidth(Width);
	TexAsset->SetHeight(Height);
	TexAsset->SetChannelCount(RequiredChannelCount);

	Memory::Free(RawData, MemoryType::eMemory_Type_Texture);
	RawData = nullptr;

	return true;
}

void ImageLoader::Unload(UAsset* resource) {
	UTexture* TexAsset = (UTexture*)resource;
	if (!TexAsset) {
		return;
	}

	stbi_image_free(TexAsset->GetPixels());
}

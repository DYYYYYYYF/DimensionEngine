#include "TextureHelper.hpp"
#include "Systems/ResourceSystem.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Platform/File/File.hpp"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

// Use our own filesystem
#ifndef STBI_NO_STDIO
#define STBI_NO_STDIO
#endif STBI_NO_STDIO

#include "stb_image.h"
#include "../Texture.hpp"

bool TextureHelper::Load(const FString& name, void* params, UTexture* resource) {
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
		StringFormat(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), "Textures", name.CStr(), Extensions[i]);
		File AssetFile(FullFilePath);
		if (AssetFile.IsExist()) {
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

	int Width, Height, ChannelCount;
	File AssetFile(FullFilePath);
	size_t FileSize = AssetFile.GetFileSize();
	std::vector<unsigned char> fileData = AssetFile.ReadBytes();
	if (fileData.empty()) {
		GLOG(Log::eError, "Unable to read file: '%s'.", FullFilePath);
		return false;
	}

	size_t BytesRead = fileData.size();
	if (FileSize != BytesRead) {
		GLOG(Log::eError, "File size is %llu does not match expected: %llu.", BytesRead, FileSize);
		return false;
	}

	unsigned char* Data = stbi_load_from_memory(
		fileData.data(),
		(int)FileSize,
		&Width, &Height, &ChannelCount, RequiredChannelCount
	);

	if (Data == nullptr) {
		GLOG(Log::eError, "Image resource loader failed to load file: '%s'.", FullFilePath);
		return false;
	}

	TexAsset->SetPixels(Data);
	TexAsset->SetWidth(Width);
	TexAsset->SetHeight(Height);
	TexAsset->SetChannelCount(RequiredChannelCount);

	return true;
}

void TextureHelper::Unload(UTexture* resource) {
	if (!resource) {
		return;
	}

	stbi_image_free(resource->GetPixels());
	resource->SetPixels(nullptr);
}

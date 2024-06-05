#include "ImageLoader.hpp"
#include "Systems/ResourceSystem.h"

#include "Platform/FileSystem.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"

// TODO: resource loader.
#define STB_IMAGE_IMPLEMENTATION
// Use our own filesystem
#define STBI_NO_STDIO
#include "stb_image.h"

ImageLoader::ImageLoader() {
	Type = eResource_type_Image;
	CustomType = nullptr;
	TypePath = "Textures";
}

bool ImageLoader::Load(const char* name, void* params, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	ImageResourceParams* TypedParams = (ImageResourceParams*)params;
	if (params == nullptr) {
		UL_ERROR("ImageLoader::Load() Required a valid params (ImageResourceParams).");
		return false;
	}

	char* FormatStr = "%s/%s/%s%s";
	const int RequiredChannelCount = 4;
	stbi_set_flip_vertically_on_load_thread(TypedParams->flip_y);
	char FullFilePath[512];

	// Try different extensions.
#define IMAGE_EXTENSION_COUNT 4
	bool Found = false;
	char* Extensions[IMAGE_EXTENSION_COUNT] = { ".tga", ".png", ".jpg", ".bmp" };
	for (uint32_t i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
		sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, Extensions[i]);
		if (FileSystemExists(FullFilePath)) {
			Found = true;
			break;
		}
	}

	// Take a copy of the resource full path and name first.
	resource->FullPath = StringCopy(FullFilePath);
	resource->Name = StringCopy(name);

	if (!Found) {
		UL_ERROR("Image resource loader failed find file '%s'or with any supported extensions.", FullFilePath);
		return false;
	}

	FileHandle f;
	if (!FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, true, &f)) {
		LOG_ERROR("Unable to read file: %s.", FullFilePath);
		FileSystemClose(&f);
		return false;
	}

	size_t FileSize = 0;
	if (!FileSystemSize(&f, &FileSize)) {
		LOG_ERROR("Unable to get size of file: %s.", FullFilePath);
		FileSystemClose(&f);
		return false;
	}

	int Width, Height, ChannelCount;
	unsigned char* RawData = (unsigned char*)Memory::Allocate(FileSize, MemoryType::eMemory_Type_Texture);
	if (RawData == nullptr) {
		LOG_ERROR("Image resource loader failed to load file: '%s'.", FullFilePath);
		FileSystemClose(&f);
		return false;
	}

	size_t BytesRead = 0;
	bool ReadResult = FileSystemReadAllBytes(&f, RawData, &BytesRead);
	FileSystemClose(&f);

	if (!ReadResult) {
		LOG_ERROR("Unable to read file: '%s'.", FullFilePath);
		return false;
	}

	if (BytesRead != FileSize) {
		LOG_ERROR("File size if %llu does not match expected: %llu.", BytesRead, FileSize);
		return false;
	}
	
	unsigned char* Data = stbi_load_from_memory(RawData, (int)FileSize, &Width, &Height, &ChannelCount, RequiredChannelCount);
	if (Data == nullptr) {
		LOG_ERROR("Image resource loader failed to load file: '%s'.", FullFilePath);
		return false;
	}

	ImageResourceData* ResourceData = (ImageResourceData*)Memory::Allocate(sizeof(ImageResourceData), MemoryType::eMemory_Type_Texture);
	ResourceData->pixels = Data;
	ResourceData->width = Width;
	ResourceData->height = Height;
	ResourceData->channel_count = RequiredChannelCount;

	resource->Data = ResourceData;
	resource->DataSize = sizeof(ImageResourceData);
	resource->DataCount = 1;

	return true;
}

void ImageLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
		return;
	}

	stbi_image_free(((ImageResourceData*)resource->Data)->pixels);

	if (resource->Name) {
		Memory::Free(resource->Name, sizeof(char) * (strlen(resource->Name) + 1), MemoryType::eMemory_Type_String);
		resource->Name = nullptr;
	}

	if (resource->FullPath) {
		Memory::Free(resource->FullPath, sizeof(char) * (strlen(resource->FullPath) + 1), MemoryType::eMemory_Type_String);
		resource->FullPath = nullptr;
	}

	if (resource->Data) {
		Memory::Free(resource->Data, resource->DataSize * resource->DataCount, MemoryType::eMemory_Type_Texture);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->DataCount = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

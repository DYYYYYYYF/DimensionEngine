#include "ImageLoader.hpp"
#include "Systems/ResourceSystem.h"

#include "Platform/FileSystem.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"

// TODO: resource loader.
#define STB_IMAGE_IMPLEMENTATION
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
	stbi_set_flip_vertically_on_load(TypedParams->flip_y);
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

	if (!Found) {
		UL_ERROR("Image resource loader failed find file '%s'or with any supported extensions.", FullFilePath);
		return false;
	}

	//sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, ".png");

	int Width, Height, ChannelCount;

	// For now, assume 8 bits per channel, 4 channels.
	// TODO: extend this to make it configurable.
	unsigned char* Data = stbi_load(FullFilePath, &Width, &Height, &ChannelCount, RequiredChannelCount);
	if (Data == nullptr) {
		UL_ERROR("Image resource loader failed to load file '%s'.", FullFilePath);
		return false;
	}

	// TODO: Should be using an allocator here.
	resource->FullPath = StringCopy(FullFilePath);

	// TODO: Should be using an allocator here.
	ImageResourceData* ResourceData = (ImageResourceData*)Memory::Allocate(sizeof(ImageResourceData), MemoryType::eMemory_Type_Texture);
	ResourceData->pixels = Data;
	ResourceData->width = Width;
	ResourceData->height = Height;
	ResourceData->channel_count = RequiredChannelCount;

	resource->Data = ResourceData;
	resource->DataSize = sizeof(ImageResourceData);
	resource->Name = StringCopy(name);
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

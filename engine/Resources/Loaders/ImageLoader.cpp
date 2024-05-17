#include "ImageLoader.hpp"
#include "Systems/ResourceSystem.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

// TODO: resource loader.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ImageLoader::ImageLoader() {
	Type = eResource_type_Image;
	CustomType = nullptr;
	TypePath = "Textures";
}

bool ImageLoader::Load(const char* name, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	char* FormatStr = "%s/%s/%s%s";
	const int RequiredChannelCount = 4;
	stbi_set_flip_vertically_on_load(true);
	char FullFilePath[512];

	// TODO: try different extensions.
	sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, ".png");

	int Width, Height, ChannelCount;

	// For now, assume 8 bits per channel, 4 channels.
	// TODO: extend this to make it configurable.
	unsigned char* Data = stbi_load(FullFilePath, &Width, &Height, &ChannelCount, RequiredChannelCount);

	// Check for a failure reason. If there is one, abort, clear memory if allocated, return false.
	if (stbi_failure_reason() != nullptr) {
		UL_WARN("Load texture failed to load file %s : %s", FullFilePath, stbi_failure_reason());

		// Clear errors so the next load doesn't fail.
		stbi__err(0, 0);

		if (Data) {
			stbi_image_free(Data);
		}

		return false;
	}

	if (Data == nullptr) {
		UL_ERROR("Image resource loader failed to load file '%s'.", FullFilePath);
		return false;
	}

	// TODO: Should be using an allocator here.
	resource->FullPath = (char*)Memory::Allocate(sizeof(char) * strlen(FullFilePath), MemoryType::eMemory_Type_String);
	strncpy(resource->FullPath, FullFilePath, sizeof(char) * strlen(FullFilePath));

	// TODO: Should be using an allocator here.
	ImageResourceData* ResourceData = (ImageResourceData*)Memory::Allocate(sizeof(ImageResourceData), MemoryType::eMemory_Type_Texture);
	ResourceData->pixels = Data;
	ResourceData->width = Width;
	ResourceData->height = Height;
	ResourceData->channel_count = RequiredChannelCount;

	resource->Data = ResourceData;
	resource->DataSize = sizeof(ImageResourceData);
	resource->Name = name;

	return true;
}

void ImageLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
		return;
	}

	uint32_t PathLength = (uint32_t)strlen(resource->FullPath);
	if (PathLength > 0) {
		Memory::Free(resource->FullPath, sizeof(char) * PathLength, MemoryType::eMemory_Type_String);
	}
	resource->FullPath = nullptr;

	if (resource->Data) {
		Memory::Free(resource->Data, resource->DataSize, MemoryType::eMemory_Type_Texture);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

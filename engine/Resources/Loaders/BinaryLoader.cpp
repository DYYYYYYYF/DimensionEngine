#include "BinaryLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Systems/ResourceSystem.h"
#include "Platform/FileSystem.hpp"

BinaryLoader::BinaryLoader() {
	Type = eResource_type_Binary;
	CustomType = nullptr;
	TypePath = "";
}

bool BinaryLoader::Load(const char* name, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, "");

	// TODO: Should be using an allocator here.
	size_t FullPathLength = sizeof(char) * strlen(FullFilePath);
	resource->FullPath = (char*)Memory::Allocate(FullPathLength, MemoryType::eMemory_Type_String);
	strncpy(resource->FullPath, FullFilePath, FullPathLength);

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, true, &File)) {
		UL_ERROR("Binary loader load. Unable to open file for binary reading: '%s'.", FullFilePath);
		return false;
	}

	size_t FileSize = 0;
	if (!FileSystemSize(&File, &FileSize)) {
		UL_ERROR("Unable to binary read file: '%s'.", FullFilePath);
		FileSystemClose(&File);
		return false;
	}

	// TODO: Should be using an allocator here.
	unsigned char* ResourceData = (unsigned char*)Memory::Allocate(sizeof(unsigned char) * FileSize, MemoryType::eMemory_Type_Array);
	size_t ReadSize = 0;
	if (!FileSystemReadAllBytes(&File, ResourceData, &ReadSize)) {
		UL_ERROR("Unable to binary read file: '%s'.", FullFilePath);
		FileSystemClose(&File);
		return false;
	}

	FileSystemClose(&File);

	resource->Data = ResourceData;
	resource->DataSize = ReadSize;
	resource->Name = name;

	return true;
}

void BinaryLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
		UL_WARN("Material loader unload called with nullptr.");
		return;
	}

	uint32_t PathLength = (uint32_t)strlen(resource->FullPath);
	if (PathLength > 0) {
		Memory::Free(resource->FullPath, sizeof(char) * PathLength, MemoryType::eMemory_Type_String);
	}

	if (resource->Data) {
		Memory::Free(resource->Data, resource->DataSize, MemoryType::eMemory_Type_Array);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->LoaderID = INVALID_ID;
	}
}
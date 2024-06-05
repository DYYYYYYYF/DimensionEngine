#include "BinaryLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"

#include "Systems/ResourceSystem.h"
#include "Platform/FileSystem.hpp"

BinaryLoader::BinaryLoader() {
	Type = eResource_type_Binary;
	CustomType = nullptr;
	TypePath = "";
}

bool BinaryLoader::Load(const char* name, void* params, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, "");

	// TODO: Should be using an allocator here.
	resource->FullPath = StringCopy(FullFilePath);

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, true, &File)) {
		LOG_ERROR("Binary loader load. Unable to open file for binary reading: '%s'.", FullFilePath);
		return false;
	}

	size_t FileSize = 0;
	if (!FileSystemSize(&File, &FileSize)) {
		LOG_ERROR("Unable to binary read file: '%s'.", FullFilePath);
		FileSystemClose(&File);
		return false;
	}

	// TODO: Should be using an allocator here.
	unsigned char* ResourceData = (unsigned char*)Memory::Allocate(sizeof(unsigned char) * FileSize, MemoryType::eMemory_Type_Array);
	size_t ReadSize = 0;
	if (!FileSystemReadAllBytes(&File, ResourceData, &ReadSize)) {
		LOG_ERROR("Unable to binary read file: '%s'.", FullFilePath);
		FileSystemClose(&File);
		return false;
	}

	FileSystemClose(&File);

	resource->Data = ResourceData;
	resource->DataSize = ReadSize;
	resource->Name = StringCopy(name);

	return true;
}

void BinaryLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
		LOG_WARN("Material loader unload called with nullptr.");
		return;
	}

	if (resource->Name) {
		Memory::Free(resource->Name, sizeof(char) * (strlen(resource->Name) + 1), MemoryType::eMemory_Type_String);
		resource->Name = nullptr;
	}

	if (resource->FullPath) {
		Memory::Free(resource->FullPath, sizeof(char) * (strlen(resource->FullPath) + 1), MemoryType::eMemory_Type_String);
		resource->FullPath = nullptr;
	}

	if (resource->Data) {
		Memory::Free(resource->Data, resource->DataSize, MemoryType::eMemory_Type_Texture);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}
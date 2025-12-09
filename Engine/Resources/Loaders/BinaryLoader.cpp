#include "BinaryLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"

#include "Systems/ResourceSystem.h"
#include "Platform/FileSystem.hpp"

BinaryLoader::BinaryLoader() {
	Type = eResource_type_Binary;
	TypePath = "";
}

bool BinaryLoader::Load(const std::string& name, void* params, Resource* resource) {
	(void)params;

	if (name.size() == 0 || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	StringFormat(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath.c_str(), name.c_str(), "");

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, true, &File)) {
		GLOG(Log::eError, "Binary loader load. Unable to open file for binary reading: '%s'.", FullFilePath);
		return false;
	}

	size_t FileSize = 0;
	if (!FileSystemSize(&File, &FileSize)) {
		GLOG(Log::eError, "Unable to binary read file: '%s'.", FullFilePath);
		FileSystemClose(&File);
		return false;
	}

	// TODO: Should be using an allocator here.
	unsigned char* ResourceData = (unsigned char*)Memory::Allocate(sizeof(unsigned char) * FileSize, MemoryType::eMemory_Type_Array);
	size_t ReadSize = 0;
	if (!FileSystemReadAllBytes(&File, ResourceData, &ReadSize)) {
		GLOG(Log::eError, "Unable to binary read file: '%s'.", FullFilePath);
		FileSystemClose(&File);
		return false;
	}

	FileSystemClose(&File);

	resource->Data = ResourceData;
	resource->DataSize = ReadSize;
	resource->FullPath = FullFilePath;
	resource->Name = name;

	return true;
}

void BinaryLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
		GLOG(Log::eWarn, "Material loader unload called with nullptr.");
		return;
	}

	if (resource->Data) {
		Memory::Free(resource->Data, MemoryType::eMemory_Type_Array);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

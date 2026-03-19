#include "BinaryLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Systems/ResourceSystem.h"
#include "Platform/File/File.hpp"

BinaryLoader::BinaryLoader() {
	Type = EAssetType::Binary;
}

bool BinaryLoader::Load(const FString& name, void* params, UAsset* resource) {
	(void)params;

	if (name.Length() == 0 || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	FString FullFilePath;
	FullFilePath = FString::Format(FormatStr, ResourceSystem::Get().GetRootPath(), TypePath.CStr(), name.CStr(), "");

	File AssetFile(FullFilePath.CStr());
	if (!AssetFile.IsExist()) {
		GLOG(Log::eError, "Binary loader load. Unable to find file for binary reading: '%s'.", FullFilePath.CStr());
		return false;
	}

	std::vector<unsigned char> FileData = AssetFile.ReadBytes();
	if (FileData.empty()) {
		GLOG(Log::eError, "Unable to binary read file: '%s'.", FullFilePath.CStr());
		return false;
	}

	resource->Data = Memory::Allocate(sizeof(unsigned char) * FileData.size(), MemoryType::eMemory_Type_Array);
	Memory::Copy(resource->Data, FileData.data(), sizeof(unsigned char) * FileData.size());

	resource->DataSize = FileData.size();
	resource->FullPath = FullFilePath;
	resource->Name = name;

	return true;
}

void BinaryLoader::Unload(UAsset* resource) {
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

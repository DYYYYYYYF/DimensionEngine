#include "MaterialLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Rendering/Resources/ResourceTypes.hpp"
#include "Platform/File/File.hpp"

MaterialLoader::MaterialLoader() {
	Type = EAssetType::Material;
	TypePath = "Materials";
}

bool MaterialLoader::Load(const FString& name, void* params, UAsset* resource) {
	if (name.Length() == 0 || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	FString FullFilePath;
	FullFilePath = FString::Format(FormatStr, ResourceSystem::Get().GetRootPath(), TypePath.CStr(), name.CStr(), ".dmt");

	File AssetFile(FullFilePath.CStr());
	if (!AssetFile.IsExist()){
		GLOG(Log::eError, "Material loader load. Unable to open material file for reading: '%s'.", FullFilePath.CStr());
		return false;
	}

	// TODO: Should be using an allocator here.
	void* mem = Memory::Allocate(sizeof(SMaterialConfig), MemoryType::eMemory_Type_Material_Instance);
	SMaterialConfig* ResourceData = new(mem) SMaterialConfig();

	// Set defaults.
	ResourceData->auto_release = true;
	ResourceData->shader_name = "Shader.Builtin.World";
	ResourceData->diffuse_color = Vector4(1.0f);	// White
	ResourceData->diffuse_map_name = "";
	ResourceData->shininess = 32.0f;
	ResourceData->Metallic = 1.0f;
	ResourceData->Roughness = 0.0f;
	ResourceData->AmbientOcclusion = 0.7f;
	ResourceData->name = name;

	AssetFile.ReadLineByLine([this, ResourceData](size_t index, const FString& line) {
		return ParseLineData(index, line, ResourceData);
	});

	resource->Data = ResourceData;
	resource->DataSize = sizeof(SMaterialConfig);
	resource->Name = name;
	resource->FullPath = FullFilePath;
	resource->DataCount = 1;

	return true;
}

bool MaterialLoader::ParseLineData(size_t index, const FString& line, SMaterialConfig* resource) {
	// Trim the string.
	FString TrimmedLine = line.Trimmed();

	// Skip blank lines and comments.
	if (TrimmedLine.IsEmpty() || TrimmedLine[0] == '#') {
		// Continue to read next line.
		return true;
	}

	// Split into var-value
	int EqualIndex = TrimmedLine.IndexOf('=');
	if (EqualIndex == -1) {
		// Continue to read next line.
		GLOG(Log::eWarn, "Potential formatting issue found in file '%s': '=' token not found. Skiping line %ui.", resource->name.CStr(), index);
		return true;
	}

	// Value name.
	FString RawVarName = TrimmedLine.SubStr(0, EqualIndex);
	FString TrimmedVarName = RawVarName.Trim();

	// Value.
	FString RawValue = TrimmedLine.SubStr(EqualIndex + 1);
	FString TrimmedValue = RawValue.Trim();

	// Process the variable.
	if (TrimmedVarName.Compare("version") == 0) {
		//TODO: version

	}
	else if (TrimmedVarName.Compare("name") == 0) {
		resource->name = std::move(TrimmedValue);
	}
	else if (TrimmedVarName.Compare("shader") == 0) {
		resource->shader_name = TrimmedValue;
	}
	else {
		resource->Properties.Insert(TrimmedVarName, TrimmedValue);
	}
	return true;
}

void MaterialLoader::Unload(UAsset* resource) {
	if (resource == nullptr) {
		GLOG(Log::eWarn, "Material loader unload called with nullptr.");
		return;
	}

	if (resource->Data) {
		Memory::Free(resource->Data, MemoryType::eMemory_Type_Material_Instance);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->DataCount = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

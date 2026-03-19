#include "MaterialLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
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
	char FullFilePath[512];
	StringFormat(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath.c_str(), name.CStr(), ".dmt");

	File AssetFile(FullFilePath);
	if (!AssetFile.IsExist()){
		GLOG(Log::eError, "Material loader load. Unable to open material file for reading: '%s'.", FullFilePath);
		return false;
	}

	// TODO: Should be using an allocator here.
	SMaterialConfig* ResourceData = (SMaterialConfig*)Memory::Allocate(sizeof(SMaterialConfig), MemoryType::eMemory_Type_Material_Instance);
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

	AssetFile.ReadLineByLine([this, ResourceData](size_t index, const std::string& line) {
		return ParseLineData(index, line.c_str(), ResourceData);
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
	else if (TrimmedVarName.Compare("diffuse_map_name") == 0) {
		resource->diffuse_map_name = TrimmedValue;
	}
	else if (TrimmedVarName.Compare("specular_map_name") == 0) {
		resource->specular_map_name = TrimmedValue;
	}
	else if (TrimmedVarName.Compare("normal_map_name") == 0) {
		resource->normal_map_name = TrimmedValue;
	}
	else if (TrimmedVarName.Compare("roughness_metallic_map_name") == 0) {
		resource->MetallicRoughnessTexName = TrimmedValue;
	}
	else if (TrimmedVarName.Compare("diffuse_color") == 0) {
		resource->diffuse_color = Vector4::StringToVec4(TrimmedValue.CStr());
	}
	else if (TrimmedVarName.Compare("shader") == 0) {
		resource->shader_name = TrimmedValue;
	}
	else if (TrimmedVarName.Compare("shininess") == 0) {
		if (!FString::ToFloat(TrimmedValue, &resource->shininess)) {
			GLOG(Log::eWarn, "Error parsing shininess in file '%s'. Using default of 32.0f instead.", resource->name.CStr());
			resource->shininess = 32.0f;
		}
	}
	else if (TrimmedVarName.Compare("metallic") == 0) {
		if (!FString::ToFloat(TrimmedValue, &resource->Metallic)) {
			GLOG(Log::eWarn, "Error parsing metallic in file '%s'. Using default of 0.1f instead.", resource->name.CStr());
			resource->Metallic = 0.1f;
		}
	}
	else if (TrimmedVarName.Compare("roughness") == 0) {
		if (!FString::ToFloat(TrimmedValue, &resource->Roughness)) {
			GLOG(Log::eWarn, "Error parsing Roughness in file '%s'. Using default of 0.5f instead.", resource->name.CStr());
			resource->Roughness = 0.5f;
		}
	}
	else if (TrimmedVarName.Compare("ambient_occlusion") == 0) {
		if (!FString::ToFloat(TrimmedValue, &resource->AmbientOcclusion)) {
			GLOG(Log::eWarn, "Error parsing AmbientOcclusion in file '%s'. Using default of 0.7f instead.", resource->name.CStr());
			resource->AmbientOcclusion = 0.7f;
		}
	}
	else if (TrimmedVarName.Compare("normal_intensity") == 0) {
		if (!FString::ToFloat(TrimmedValue, &resource->NormalIntensity)) {
			GLOG(Log::eWarn, "Error parsing AmbientOcclusion in file '%s'. Using default of 0.7f instead.", resource->name.CStr());
			resource->NormalIntensity = 1.0f;
		}
	}
	// TODO: more fields.

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

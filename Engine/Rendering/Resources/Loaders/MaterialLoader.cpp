#include "MaterialLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"

MaterialLoader::MaterialLoader() {
	Type = EAssetType::Material;
	TypePath = "Materials";
}

bool MaterialLoader::Load(const std::string& name, void* params, UAsset* resource) {
	if (name.length() == 0 || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	StringFormat(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath.c_str(), name.c_str(), ".dmt");

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, false, &File)) {
		GLOG(Log::eError, "Material loader load. Unable to open material file for reading: '%s'.", FullFilePath);
		return false;
	}

	// TODO: Should be using an allocator here.
	SMaterialConfig* ResourceData = (SMaterialConfig*)Memory::Allocate(sizeof(SMaterialConfig), MemoryType::eMemory_Type_Material_Instance);
	// Set defaults.
	ResourceData->auto_release = true;
	ResourceData->shader_name = "Shader.Builtin.World";
	ResourceData->diffuse_color = Vector4(1.0f);	// White
	ResourceData->diffuse_map_name[0] = '\0';
	ResourceData->shininess = 32.0f;
	ResourceData->Metallic = 1.0f;
	ResourceData->Roughness = 0.0f;
	ResourceData->AmbientOcclusion = 0.7f;
	ResourceData->name = std::move(name);

	char LineBuffer[512] = "";
	char* p = &LineBuffer[0];
	size_t LineLength = 0;
	uint32_t LineNumber = 1;
	while (FileSystemReadLine(&File, 511, &p, &LineLength)) {
		// Trim the string.
		char* Trimmed = Strtrim(LineBuffer);

		// Get the trimmed length.
		LineLength = strlen(Trimmed);

		// Skip blank lines and comments.
		if (LineLength < 1 || Trimmed[0] == '#') {
			LineNumber++;
			continue;
		}

		// Split into var-value
		int EqualIndex = StringIndexOf(Trimmed, '=');
		if (EqualIndex == -1) {
			GLOG(Log::eWarn, "Potential formatting issue found in file '%s': '=' token not found. Skiping line %ui.", FullFilePath, LineNumber);
			LineNumber++;
			continue;
		}

		// Assume a max of 64 characters for the variable name.
		char RawVarName[64];
		Memory::Zero(RawVarName, sizeof(char) * 64);
		StringMid(RawVarName, Trimmed, 0, EqualIndex);
		char* TrimmedVarName = Strtrim(RawVarName);

		// Assume a max of 511-64(446) characters for the max length of the value to account for the variable name and the '='.
		char RawValue[446];
		Memory::Zero(RawValue, sizeof(char) * 446);
		StringMid(RawValue, Trimmed, EqualIndex + 1);
		char* TrimmedValue = Strtrim(RawValue);

		// Process the variable.
		if (strcmp(TrimmedVarName, "version") == 0) {
			//TODO: version

		}
		else if (strcmp(TrimmedVarName, "name") == 0) {
			ResourceData->name = std::move(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "diffuse_map_name") == 0) {
			ResourceData->diffuse_map_name = TrimmedValue;
		}
		else if (strcmp(TrimmedVarName, "specular_map_name") == 0) {
			// Parse the color
			ResourceData->specular_map_name = TrimmedValue;
		}
		else if (strcmp(TrimmedVarName, "normal_map_name") == 0) {
			// Parse the color
			ResourceData->normal_map_name = TrimmedValue;
		}
		else if (strcmp(TrimmedVarName, "roughness_metallic_map_name") == 0) {
			// Parse the color
			char TexName[TEXTURE_NAME_MAX_LENGTH] = "";
			strncpy(TexName, TrimmedValue, TEXTURE_NAME_MAX_LENGTH);
			ResourceData->MetallicRoughnessTexName = std::string(TexName);
		}
		else if (strcmp(TrimmedVarName, "diffuse_color") == 0) {
			// Parse the color
			ResourceData->diffuse_color = Vector4::StringToVec4(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "shader") == 0) {
			ResourceData->shader_name = std::string(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "shininess") == 0){
			if (!FString::ToFloat(TrimmedValue, &ResourceData->shininess)) {
				GLOG(Log::eWarn, "Error parsing shininess in file '%s'. Using default of 32.0f instead.", FullFilePath);
				ResourceData->shininess = 32.0f;
			}
		}
		else if (strcmp(TrimmedVarName, "metallic") == 0) {
			if (!FString::ToFloat(TrimmedValue, &ResourceData->Metallic)) {
				GLOG(Log::eWarn, "Error parsing metallic in file '%s'. Using default of 0.1f instead.", FullFilePath);
				ResourceData->Metallic = 0.1f;
			}
		}
		else if (strcmp(TrimmedVarName, "roughness") == 0) {
			if (!FString::ToFloat(TrimmedValue, &ResourceData->Roughness)) {
				GLOG(Log::eWarn, "Error parsing Roughness in file '%s'. Using default of 0.5f instead.", FullFilePath);
				ResourceData->Roughness = 0.5f;
			}
		}
		else if (strcmp(TrimmedVarName, "ambient_occlusion") == 0) {
			if (!FString::ToFloat(TrimmedValue, &ResourceData->AmbientOcclusion)) {
				GLOG(Log::eWarn, "Error parsing AmbientOcclusion in file '%s'. Using default of 0.7f instead.", FullFilePath);
				ResourceData->AmbientOcclusion = 0.7f;
			}
		}
		else if (strcmp(TrimmedVarName, "normal_intensity") == 0) {
			if (!FString::ToFloat(TrimmedValue, &ResourceData->NormalIntensity)) {
				GLOG(Log::eWarn, "Error parsing AmbientOcclusion in file '%s'. Using default of 0.7f instead.", FullFilePath);
				ResourceData->NormalIntensity = 1.0f;
			}
		}
		// TODO: more fields.

		// Clear the line buffer.
		Memory::Zero(LineBuffer, sizeof(char) * 512);
		LineNumber++;
	}

	FileSystemClose(&File);

	resource->Data = ResourceData;
	resource->DataSize = sizeof(SMaterialConfig);
	resource->Name = name.c_str();
	resource->FullPath = FullFilePath;
	resource->DataCount = 1;

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

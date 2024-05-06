#include "MaterialLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"

MaterialLoader::MaterialLoader() {
	Type = eResource_type_Material;
	CustomType = nullptr;
	TypePath = "Materials";
}

bool MaterialLoader::Load(const char* name, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, ".dmt");

	// TODO: Should be using an allocator here.
	resource->FullPath = (char*)Memory::Allocate(sizeof(char) * strlen(FullFilePath), MemoryType::eMemory_Type_String);
	strncpy(resource->FullPath, FullFilePath, sizeof(char) * strlen(FullFilePath));

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, false, &File)) {
		UL_ERROR("Material loader load. Unable to open material file for reading: '%s'.", FullFilePath);
		return false;
	}

	// TODO: Should be using an allocator here.
	SMaterialConfig* ResourceData = (SMaterialConfig*)Memory::Allocate(sizeof(SMaterialConfig), MemoryType::eMemory_Type_Material_Instance);
	// Set defaults.
	ResourceData->auto_release = true;
	ResourceData->diffuse_color = Vec4(1.0f);	// White
	ResourceData->diffuse_map_name[0] = '\0';
	strncpy(ResourceData->name, name, MATERIAL_NAME_MAX_LENGTH);

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
			UL_WARN("Potential formatting issue found in file '%s': '=' token not found. Skiping line %ui.", FullFilePath, LineNumber);
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
			strncpy(ResourceData->name, TrimmedValue, MATERIAL_NAME_MAX_LENGTH);
		}
		else if (strcmp(TrimmedVarName, "diffuse_map_name") == 0) {
			strncpy(ResourceData->diffuse_map_name, TrimmedValue, TEXTURE_NAME_MAX_LENGTH);
		}
		else if (strcmp(TrimmedVarName, "diffuse_color") == 0) {
			// Parse the color
			ResourceData->diffuse_color = Vec4::StringToVec4(TrimmedValue);
		}

		// TODO: more fields.

		// Clear the line buffer.
		Memory::Zero(LineBuffer, sizeof(char) * 512);
		LineNumber++;
	}

	FileSystemClose(&File);

	resource->Data = ResourceData;
	resource->DataSize = sizeof(SMaterialConfig);
	resource->Name = name;

	return true;
}

void MaterialLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
		UL_WARN("Material loader unload called with nullptr.");
		return;
	}

	uint32_t PathLength = (uint32_t)strlen(resource->FullPath);
	if (PathLength > 0) {
		Memory::Free(resource->FullPath, sizeof(char) * PathLength, MemoryType::eMemory_Type_String);
	}

	if (resource->Data) {
		Memory::Free(resource->Data, resource->DataSize, MemoryType::eMemory_Type_Material_Instance);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->LoaderID = INVALID_ID;
	}
}
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

bool MaterialLoader::Load(const char* name, void* params, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	StringFormat(FullFilePath, 512, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, ".dmt");

	// TODO: Should be using an allocator here.
	resource->FullPath = StringCopy(FullFilePath);

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, false, &File)) {
		LOG_ERROR("Material loader load. Unable to open material file for reading: '%s'.", FullFilePath);
		return false;
	}

	// TODO: Should be using an allocator here.
	SMaterialConfig* ResourceData = (SMaterialConfig*)Memory::Allocate(sizeof(SMaterialConfig), MemoryType::eMemory_Type_Material_Instance);
	// Set defaults.
	ResourceData->auto_release = true;
	ResourceData->shader_name = "Builtin.World";
	ResourceData->diffuse_color = Vec4(1.0f);	// White
	ResourceData->diffuse_map_name[0] = '\0';
	ResourceData->shininess = 32.0f;
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
			LOG_WARN("Potential formatting issue found in file '%s': '=' token not found. Skiping line %ui.", FullFilePath, LineNumber);
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
		else if (strcmp(TrimmedVarName, "specular_map_name") == 0) {
			// Parse the color
			strncpy(ResourceData->specular_map_name, TrimmedValue, TEXTURE_NAME_MAX_LENGTH);
		}
		else if (strcmp(TrimmedVarName, "normal_map_name") == 0) {
			// Parse the color
			strncpy(ResourceData->normal_map_name, TrimmedValue, TEXTURE_NAME_MAX_LENGTH);
		}
		else if (strcmp(TrimmedVarName, "diffuse_color") == 0) {
			// Parse the color
			ResourceData->diffuse_color = Vec4::StringToVec4(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "shader") == 0) {
			ResourceData->shader_name = StringCopy(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "shininess") == 0){
			if (!StringToFloat(TrimmedValue, &ResourceData->shininess)) {
				LOG_WARN("Error parsing shininess in file '%s'. Using default of 32.0f instead.", FullFilePath);
				ResourceData->shininess = 32.0f;
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
	resource->Name = StringCopy(name);
	resource->DataCount = 1;

	return true;
}

void MaterialLoader::Unload(Resource* resource) {
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
		Memory::Free(resource->Data, resource->DataSize * resource->DataCount, MemoryType::eMemory_Type_Material_Instance);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->DataCount = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

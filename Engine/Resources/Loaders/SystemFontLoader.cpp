#include "SystemFontLoader.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"
#include "stdio.h"

SystemFontLoader::SystemFontLoader() {
	CustomType = nullptr;
	Type = ResourceType::eResource_Type_System_Font;
	TypePath = "Fonts";
}

bool SystemFontLoader::Load(const char* name, void* params, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	FileHandle f;

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedSystemFontFiletype SupportedFieTypes[SUPPORTED_FILETYPE_COUNT];
	SupportedFieTypes[0] = SupportedSystemFontFiletype{ ".dsf", SystemFontFileType::eSystem_Font_File_Type_DSF, true };
	SupportedFieTypes[1] = SupportedSystemFontFiletype{ ".fontcfg", SystemFontFileType::eSystem_Font_File_Type_Font_Config, false };

	char FullFilePath[512];
	SystemFontFileType Type = SystemFontFileType::eSystem_Font_File_Type_Not_Found;
	// Try each supported extension.
	for (uint32_t i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
		StringFormat(FullFilePath, 512, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, SupportedFieTypes[i].extension);
		// If the file exist, open it and stop looking.
		if (FileSystemExists(FullFilePath)) {
			if (FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, SupportedFieTypes[i].isBinary, &f)) {
				Type = SupportedFieTypes[i].type;
				break;
			}
		}

	}

	if (Type == SystemFontFileType::eSystem_Font_File_Type_Not_Found) {
		LOG_ERROR("Unable to find system font of supported type called: '%s'.", name);
		return false;
	}

	resource->FullPath = StringCopy(FullFilePath);
	resource->Name = StringCopy(name);

	SystemFontResourceData ResourceData;
	bool Result = false;
	switch (Type)
	{
	case eSystem_Font_File_Type_Not_Found:
		LOG_ERROR("Unable to find system font of supported type called '%s'.", name);
		Result = false;
		break;
	case eSystem_Font_File_Type_DSF:
		Result = ReadDSFFile(&f, &ResourceData);
		break;
	case eSystem_Font_File_Type_Font_Config:
		// Generate the dsf file.
		char DSFFilename[512];
		StringFormat(DSFFilename, 512, "%s/%s/%s%s", ResourceSystem::GetRootPath(), TypePath, name, ".dsf");
		Result = ImportFontconfigFile(&f, TypePath, DSFFilename, &ResourceData);
		break;
	}

	FileSystemClose(&f);

	if (!Result) {
		LOG_ERROR("Failed to process system font file '%s'.", FullFilePath);
		resource->Data = nullptr;
		resource->DataSize = 0;
		return false;
	}

	resource->Data = Memory::Allocate(sizeof(SystemFontResourceData), MemoryType::eMemory_Type_Resource);
	Memory::Copy(resource->Data, &ResourceData, sizeof(SystemFontResourceData));
	resource->DataSize = sizeof(SystemFontResourceData);

	return true;
}

void SystemFontLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
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
		SystemFontResourceData* Data = (SystemFontResourceData*)resource->Data;
		if (Data->fonts.Size() > 0) {
			Data->fonts.Clear();
		}

		if (Data->fontBinary) {
			Memory::Free(Data->fontBinary, Data->binarySize, MemoryType::eMemory_Type_Resource);
			Data->fontBinary = nullptr;
			Data->binarySize = 0;
		}

		Memory::Free(resource->Data, resource->DataSize * resource->DataCount, MemoryType::eMemory_Type_Texture);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->DataCount = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

bool SystemFontLoader::ImportFontconfigFile(FileHandle* f, const char* typePath, const char* outDSFFilename, SystemFontResourceData* outResource) {
	outResource->binarySize = 0;
	outResource->fontBinary = nullptr;

	// Read each line of the file.
	char LineBuf[512] = "";
	char* p = &LineBuf[0];
	size_t LineLength = 0;
	uint32_t LineNumber = 1;
	while (FileSystemReadLine(f, 511, &p, &LineLength)) {
		// Trim the string.
		char* Trimmed = Strtrim(LineBuf);

		// Get the trimmed length.
		LineLength = strlen(Trimmed);

		// Skip blank lines and comments.
		if (LineLength < 1 || Trimmed[0] == '\0') {
			LineNumber++;
			continue;
		}

		// Split into var/value.
		int EqualIndex = StringIndexOf(Trimmed, '=');
		if (EqualIndex == -1) {
			LOG_WARN("Potential formatting issue found in file: '=' token not found. Skipping line  %u.", LineNumber);
			LineNumber++;
			continue;
		}

		// Assume a max of 64 characters for the variable name.
		char RawVarName[64];
		Memory::Zero(RawVarName, sizeof(char) * 64);
		StringMid(RawVarName, Trimmed, 0, EqualIndex);
		char* TrimmedVarName = Strtrim(RawVarName);

		// Assume a max of 511-64 (446) for the max length of the value to account for the variable name and '='.
		char RawValue[446];
		Memory::Zero(RawValue, sizeof(char) * 446);
		StringMid(RawValue, Trimmed, EqualIndex + 1, -1);
		char* TrimmedValue = Strtrim(RawValue);

		// Process the variable.
		if (StringEquali(TrimmedVarName, "version")) {
			// TODO: version
            LOG_INFO("Test");
		}
		else if (StringEquali(TrimmedVarName, "file")) {
			const char* FormatStr = "%s/%s/%s";
			char FullFilePath[512];
			StringFormat(FullFilePath, 511, FormatStr, ResourceSystem::GetRootPath(), typePath, TrimmedValue);

			// Open and read the font file as binary, and save into an allocated.
			FileHandle FontBinaryHandle;
			if (!FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, true, &FontBinaryHandle)) {
				LOG_ERROR("Unable to open binary font file. Load process failed.");
				return false;
			}

			size_t FileSize;
			if (!FileSystemSize(&FontBinaryHandle, &FileSize)) {
				LOG_ERROR("Unable to get binary font file size. Load process failed.");
				return false;
			}

			outResource->fontBinary = Memory::Allocate(FileSize, MemoryType::eMemory_Type_Resource);
			if (!FileSystemReadAllBytes(&FontBinaryHandle, (unsigned char*)outResource->fontBinary, &outResource->binarySize)) {
				LOG_ERROR("Unable to perform binary read on font file. Load process failed.");
				return false;
			}

			// Might still work anyway. so continue.
			if (outResource->binarySize != FileSize) {
				LOG_WARN("Mismatch between filesize and bytes read in font file. File may be corrupt.");
			}
			
			FileSystemClose(&FontBinaryHandle);
		}
		else if (StringEquali(TrimmedVarName, "face")) {
			// Read in the font face and store it for later.
			SystemFontFace NewFace;
			strncpy(NewFace.name, TrimmedValue, 255);
			outResource->fonts.Push(NewFace);
		}

		// Clear the line buffer.
		Memory::Zero(LineBuf, sizeof(char) * 512);
		LineNumber++;
	}

	FileSystemClose(f);

	// Check here to make sure a binary was loaded, and at least one font face was found.
	if (!outResource->fontBinary || outResource->fonts.Size() < 1) {
		LOG_ERROR("Font configuration did not provide a binary and at least one font face. Load process failed.");
		return false;
	}

	return WriteDSFFile(outDSFFilename, outResource);
}

bool SystemFontLoader::ReadDSFFile(FileHandle* file, SystemFontResourceData* data) {
	Memory::Zero(data, sizeof(SystemFontResourceData));

	size_t BytesRead = 0;
	uint32_t ReadSize = 0;

	// Write the resource header first.
	ResourceHeader Header;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(ResourceHeader), &Header, &BytesRead), file);

	// Verify header contents.
	if (Header.magicNumber != RESOURCES_MAGIC && Header.resourceType == ResourceType::eResource_Type_System_Font) {
		LOG_ERROR("DSF file header is invalid and can not be read.");
		FileSystemClose(file);
		return false;
	}

	// TODO: read in/process file version.

	// Size of font binary.
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(size_t), &data->binarySize, &BytesRead), file);

	// The font binary
	CLOSE_IF_FAILED(FileSystemRead(file, data->binarySize, &data->fontBinary, &BytesRead), file);

	// The number of fonts
	uint32_t FontCount = (uint32_t)data->fonts.Size();
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FontCount, &BytesRead), file);

	// Iterate faces metedata and output as well.
	for (uint32_t i = 0; i < FontCount; ++i) {
		// Length of face name string.
		uint32_t FaceLength = (uint32_t)strlen(data->fonts[i].name);
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FaceLength, &BytesRead), file);

		// Face string.
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char) * FaceLength + 1, data->fonts[i].name, &BytesRead), file);
	}

	return true;
}

bool SystemFontLoader::WriteDSFFile(const char* outDSFFilename, SystemFontResourceData* resource) {
	return true;
}

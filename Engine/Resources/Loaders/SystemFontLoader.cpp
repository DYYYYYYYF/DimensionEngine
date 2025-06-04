#include "SystemFontLoader.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"
#include "stdio.h"

SystemFontLoader::SystemFontLoader() {
	Type = ResourceType::eResource_Type_System_Font;
	TypePath = "Fonts";
}

bool SystemFontLoader::Load(const std::string& name, void* params, Resource* resource) {
	if (name.length() == 0 || resource == nullptr) {
		return false;
	}

	resource->Data = NewObject<SystemFontResourceData>();
	SystemFontResourceData* ResourceData = (SystemFontResourceData*)resource->Data;

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
		StringFormat(FullFilePath, 512, FormatStr, ResourceSystem::GetRootPath(), TypePath.c_str(), name.c_str(), SupportedFieTypes[i].extension);
		// If the file exist, open it and stop looking.
		if (FileSystemExists(FullFilePath)) {
			if (FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, SupportedFieTypes[i].isBinary, &f)) {
				Type = SupportedFieTypes[i].type;
				break;
			}
		}

	}

	if (Type == SystemFontFileType::eSystem_Font_File_Type_Not_Found) {
		GLOG(Log::eError, "Unable to find system font of supported type called: '%s'.", name.c_str());
		return false;
	}

	resource->FullPath = FullFilePath;
	resource->Name = name;

	bool Result = false;
	switch (Type)
	{
	case eSystem_Font_File_Type_Not_Found:
		GLOG(Log::eError, "Unable to find system font of supported type called '%s'.", name.c_str());
		Result = false;
		break;
	case eSystem_Font_File_Type_DSF:
		Result = ReadDSFFile(&f, ResourceData);
		break;
	case eSystem_Font_File_Type_Font_Config:
		// Generate the dsf file.
		char DSFFilename[512];
		StringFormat(DSFFilename, 512, "%s/%s/%s%s", ResourceSystem::GetRootPath(), TypePath.c_str(), name.c_str(), ".dsf");
		Result = ImportFontconfigFile(&f, TypePath.c_str(), DSFFilename, ResourceData);
		break;
	}

	FileSystemClose(&f);

	if (!Result) {
		GLOG(Log::eError, "Failed to process system font file '%s'.", FullFilePath);
		resource->Data = nullptr;
		resource->DataSize = 0;
		return false;
	}

	resource->DataSize = sizeof(SystemFontResourceData);

	return true;
}

void SystemFontLoader::Unload(Resource* resource) {
	if (resource == nullptr) {
		return;
	}

	if (resource->Data) {
		SystemFontResourceData* Data = (SystemFontResourceData*)resource->Data;
		if (Data->fonts.size() > 0) {
			Data->fonts.clear();
		}

		if (Data->fontBinary) {
			Memory::Free(Data->fontBinary, Data->binarySize, MemoryType::eMemory_Type_Resource);
			Data->fontBinary = nullptr;
			Data->binarySize = 0;
		}

		Memory::Free(resource->Data, resource->DataSize * resource->DataCount, MemoryType::eMemory_Type_System_Font);
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
			GLOG(Log::eWarn, "Potential formatting issue found in file: '=' token not found. Skipping line  %u.", LineNumber);
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
            GLOG(Log::eInfo, "Test");
		}
		else if (StringEquali(TrimmedVarName, "file")) {
			const char* FormatStr = "%s/%s/%s";
			char FullFilePath[512];
			StringFormat(FullFilePath, 511, FormatStr, ResourceSystem::GetRootPath(), typePath, TrimmedValue);

			// Open and read the font file as binary, and save into an allocated.
			FileHandle FontBinaryHandle;
			if (!FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, true, &FontBinaryHandle)) {
				GLOG(Log::eError, "Unable to open binary font file. Load process failed.");
				return false;
			}

			size_t FileSize;
			if (!FileSystemSize(&FontBinaryHandle, &FileSize)) {
				GLOG(Log::eError, "Unable to get binary font file size. Load process failed.");
				return false;
			}

			outResource->fontBinary = Memory::Allocate(FileSize, MemoryType::eMemory_Type_Resource);
			if (!FileSystemReadAllBytes(&FontBinaryHandle, (unsigned char*)outResource->fontBinary, &outResource->binarySize)) {
				GLOG(Log::eError, "Unable to perform binary read on font file. Load process failed.");
				return false;
			}

			// Might still work anyway. so continue.
			if (outResource->binarySize != FileSize) {
				GLOG(Log::eWarn, "Mismatch between filesize and bytes read in font file. File may be corrupt.");
			}
			
			FileSystemClose(&FontBinaryHandle);
		}
		else if (StringEquali(TrimmedVarName, "face")) {
			// Read in the font face and store it for later.
			SystemFontFace NewFace;
			NewFace.name = std::move(TrimmedValue);
			outResource->fonts.push_back(NewFace);
		}

		// Clear the line buffer.
		Memory::Zero(LineBuf, sizeof(char) * 512);
		LineNumber++;
	}

	FileSystemClose(f);

	// Check here to make sure a binary was loaded, and at least one font face was found.
	if (!outResource->fontBinary || outResource->fonts.size() < 1) {
		GLOG(Log::eError, "Font configuration did not provide a binary and at least one font face. Load process failed.");
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
		GLOG(Log::eError, "DSF file header is invalid and can not be read.");
		FileSystemClose(file);
		return false;
	}

	// TODO: read in/process file version.

	// Size of font binary.
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(size_t), &data->binarySize, &BytesRead), file);

	// The font binary
	CLOSE_IF_FAILED(FileSystemRead(file, data->binarySize, &data->fontBinary, &BytesRead), file);

	// The number of fonts
	uint32_t FontCount = (uint32_t)data->fonts.size();
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FontCount, &BytesRead), file);

	// Iterate faces metedata and output as well.
	for (uint32_t i = 0; i < FontCount; ++i) {
		// Length of face name string.
		uint32_t FaceLength = (uint32_t)data->fonts[i].name.length();
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FaceLength, &BytesRead), file);

		// Face string.
		char* f = (char*)Memory::Allocate(sizeof(char) * FaceLength, MemoryType::eMemory_Type_String);
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char) * FaceLength, f, &BytesRead), file);
		data->fonts[i].name = std::move(f);
		Memory::Free(f, sizeof(char) * FaceLength, MemoryType::eMemory_Type_String);
	}

	return true;
}

bool SystemFontLoader::WriteDSFFile(const char* outDSFFilename, SystemFontResourceData* resource) {
	return true;
}

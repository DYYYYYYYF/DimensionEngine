#include "SystemFontLoader.hpp"
#include "Rendering/Resources/Font/SystemFont.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"
#include "Systems/ResourceSystem.h"

#include <stdio.h>

SystemFontLoader::SystemFontLoader() {
	Type = EAssetType::SystemFont;
	TypePath = "Fonts";
}

// ─────────────────────────────────────────────────────────────────
//  Load / Unload
// ─────────────────────────────────────────────────────────────────

bool SystemFontLoader::Load(const FString& name, void* params, UAsset* resource) {
	if (name.Length() == 0 || resource == nullptr) {
		return false;
	}

	// SystemFont 的加载分两步：
	//   1. Loader 负责读取 TTF 二进制和 face 元数据 → SystemFontResourceData
	//   2. SystemFont::InitFromResourceData() 负责 stbtt 初始化和 GPU 资源
	// 此处只完成第一步，resource->Data 指向 SystemFontResourceData
	resource->Data = NewObject<SystemFontResourceData>();
	SystemFontResourceData* resourceData = static_cast<SystemFontResourceData*>(resource->Data);

	const char* formatStr = "%s/%s/%s%s";
	FileHandle f;

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedSystemFontFiletype supportedTypes[SUPPORTED_FILETYPE_COUNT];
	supportedTypes[0] = { ".dsf",      SystemFontFileType::eSystem_Font_File_Type_DSF,         true };
	supportedTypes[1] = { ".fontcfg",  SystemFontFileType::eSystem_Font_File_Type_Font_Config,  false };

	char fullFilePath[512];
	SystemFontFileType fontType = SystemFontFileType::eSystem_Font_File_Type_Not_Found;

	for (uint32_t i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
		StringFormat(fullFilePath, formatStr,
			ResourceSystem::GetRootPath(), TypePath.c_str(),
			name.CStr(), supportedTypes[i].extension);

		if (FileSystemExists(fullFilePath)) {
			if (FileSystemOpen(fullFilePath, FileMode::eFile_Mode_Read, supportedTypes[i].isBinary, &f)) {
				fontType = supportedTypes[i].type;
				break;
			}
		}
	}

	if (fontType == SystemFontFileType::eSystem_Font_File_Type_Not_Found) {
		GLOG(Log::eError, "SystemFontLoader: unable to find font of supported type: '%s'.", name.CStr());
		return false;
	}

	resource->FullPath = fullFilePath;
	resource->Name = name;

	bool result = false;
	switch (fontType) {
	case eSystem_Font_File_Type_Not_Found:
		GLOG(Log::eError, "SystemFontLoader: font type not found for '%s'.", name.CStr());
		break;

	case eSystem_Font_File_Type_DSF:
		result = ReadDSFFile(&f, resourceData);
		break;

	case eSystem_Font_File_Type_Font_Config: {
		char dsfFilename[512];
		StringFormat(dsfFilename, "%s/%s/%s%s",
			ResourceSystem::GetRootPath(), TypePath.c_str(), name.CStr(), ".dsf");
		result = ImportFontconfigFile(&f, TypePath.c_str(), dsfFilename, resourceData);
		break;
	}
	}

	FileSystemClose(&f);

	if (!result) {
		GLOG(Log::eError, "SystemFontLoader: failed to process font file: '%s'.", fullFilePath);
		resource->Data = nullptr;
		resource->DataSize = 0;
		return false;
	}

	resource->DataSize = sizeof(SystemFontResourceData);
	return true;
}

void SystemFontLoader::Unload(UAsset* resource) {
	if (!resource || !resource->Data) { return; }

	SystemFontResourceData* data = static_cast<SystemFontResourceData*>(resource->Data);

	data->fonts.Clear();

	// TTF 二进制块由 Loader 分配，在此释放
	// SystemFont 持有的是借用指针，不重复释放
	if (data->fontBinary) {
		Memory::Free(data->fontBinary, MemoryType::eMemory_Type_Resource);
		data->fontBinary = nullptr;
		data->binarySize = 0;
	}

	Memory::Free(resource->Data, MemoryType::eMemory_Type_System_Font);
	resource->Data = nullptr;
	resource->DataSize = 0;
	resource->DataCount = 0;
	resource->LoaderID = INVALID_ID;
}

// ─────────────────────────────────────────────────────────────────
//  ImportFontconfigFile — 解析 .fontcfg 文本配置
// ─────────────────────────────────────────────────────────────────

bool SystemFontLoader::ImportFontconfigFile(FileHandle* f, const char* typePath,
	const char* outDSFFilename, SystemFontResourceData* outResource) {

	outResource->binarySize = 0;
	outResource->fontBinary = nullptr;

	char    lineBuf[512] = "";
	char* p = &lineBuf[0];
	size_t  lineLength = 0;
	uint32_t lineNumber = 1;

	while (FileSystemReadLine(f, 511, &p, &lineLength)) {
		char* trimmed = Strtrim(lineBuf);
		lineLength = strlen(trimmed);

		// 跳过空行
		if (lineLength < 1 || trimmed[0] == '\0') {
			++lineNumber;
			continue;
		}

		// 分割 key=value
		int equalIndex = StringIndexOf(trimmed, '=');
		if (equalIndex == -1) {
			GLOG(Log::eWarn,
				"SystemFontLoader: '=' not found on line %u, skipping.", lineNumber);
			++lineNumber;
			continue;
		}

		char rawVarName[64];
		Memory::Zero(rawVarName, sizeof(rawVarName));
		StringMid(rawVarName, trimmed, 0, equalIndex);
		char* varName = Strtrim(rawVarName);

		char rawValue[446];
		Memory::Zero(rawValue, sizeof(rawValue));
		StringMid(rawValue, trimmed, equalIndex + 1, -1);
		char* value = Strtrim(rawValue);

		if (StringEquali(varName, "version")) {
			// TODO: version 处理
		}
		else if (StringEquali(varName, "file")) {
			// 读取 TTF 二进制文件
			char fullFontPath[512];
			StringFormat(fullFontPath, "%s/%s/%s",
				ResourceSystem::GetRootPath(), typePath, value);

			FileHandle fontHandle;
			if (!FileSystemOpen(fullFontPath, FileMode::eFile_Mode_Read, true, &fontHandle)) {
				GLOG(Log::eError, "SystemFontLoader: unable to open binary font: %s.", fullFontPath);
				return false;
			}

			size_t fileSize = 0;
			if (!FileSystemSize(&fontHandle, &fileSize)) {
				GLOG(Log::eError, "SystemFontLoader: unable to get font file size.");
				return false;
			}

			outResource->fontBinary = Memory::Allocate(fileSize, MemoryType::eMemory_Type_Resource);
			if (!FileSystemReadAllBytes(&fontHandle,
				static_cast<unsigned char*>(outResource->fontBinary),
				&outResource->binarySize)) {
				GLOG(Log::eError, "SystemFontLoader: failed to read binary font.");
				return false;
			}

			if (outResource->binarySize != fileSize) {
				GLOG(Log::eWarn, "SystemFontLoader: font file size mismatch, file may be corrupt.");
			}

			FileSystemClose(&fontHandle);
		}
		else if (StringEquali(varName, "face")) {
			SystemFontFace newFace;
			newFace.name = value;
			outResource->fonts.Push(newFace);
		}

		Memory::Zero(lineBuf, sizeof(lineBuf));
		++lineNumber;
	}

	FileSystemClose(f);

	if (!outResource->fontBinary || outResource->fonts.IsEmpty()) {
		GLOG(Log::eError,
			"SystemFontLoader: config must provide a binary and at least one face.");
		return false;
	}

	return WriteDSFFile(outDSFFilename, outResource);
}

// ─────────────────────────────────────────────────────────────────
//  ReadDSFFile — 读取引擎二进制格式
// ─────────────────────────────────────────────────────────────────

bool SystemFontLoader::ReadDSFFile(FileHandle* file, SystemFontResourceData* data) {
	size_t bytesRead = 0;

	// 文件头校验
	ResourceHeader header;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(ResourceHeader), &header, &bytesRead), file);

	if (header.magicNumber != RESOURCES_MAGIC ||
		header.resourceType != static_cast<char>(EAssetType::SystemFont)) {
		GLOG(Log::eError, "SystemFontLoader: DSF file header is invalid.");
		FileSystemClose(file);
		return false;
	}

	// TTF 二进制大小
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(size_t), &data->binarySize, &bytesRead), file);

	// TTF 二进制数据
	data->fontBinary = Memory::Allocate(data->binarySize, MemoryType::eMemory_Type_Resource);
	CLOSE_IF_FAILED(FileSystemRead(file, data->binarySize, data->fontBinary, &bytesRead), file);

	// Face 数量
	uint32_t faceCount = 0;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &faceCount, &bytesRead), file);
	data->fonts.Resize(faceCount);

	// 每个 face 的名称
	for (uint32_t i = 0; i < faceCount; ++i) {
		uint32_t faceLength = 0;
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &faceLength, &bytesRead), file);

		char* faceBuf = static_cast<char*>(
			Memory::Allocate(sizeof(char) * faceLength, MemoryType::eMemory_Type_String));
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char) * faceLength, faceBuf, &bytesRead), file);
		data->fonts[i].name = faceBuf;
		Memory::Free(faceBuf, MemoryType::eMemory_Type_String);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────
//  WriteDSFFile — 写出引擎二进制格式
// ─────────────────────────────────────────────────────────────────

bool SystemFontLoader::WriteDSFFile(const char* outDSFFilename, SystemFontResourceData* resource) {
	FileHandle file;
	if (!FileSystemOpen(outDSFFilename, FileMode::eFile_Mode_Write, true, &file)) {
		GLOG(Log::eError, "SystemFontLoader: failed to open DSF for writing: %s.", outDSFFilename);
		return false;
	}

	size_t bytesWritten = 0;

	// 文件头
	ResourceHeader header;
	header.magicNumber = RESOURCES_MAGIC;
	header.resourceType = static_cast<char>(EAssetType::SystemFont);
	header.version = 0x01U;
	header.reserved = 0;
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(ResourceHeader), &header, &bytesWritten), &file);

	// TTF 二进制大小和数据
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(size_t), &resource->binarySize, &bytesWritten), &file);
	CLOSE_IF_FAILED(FileSystemWrite(&file, resource->binarySize, resource->fontBinary, &bytesWritten), &file);

	// Face 数量
	uint32_t faceCount = static_cast<uint32_t>(resource->fonts.Size());
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(uint32_t), &faceCount, &bytesWritten), &file);

	// 每个 face 的名称
	for (uint32_t i = 0; i < faceCount; ++i) {
		uint32_t faceLength = static_cast<uint32_t>(resource->fonts[i].name.Length()) + 1;
		CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(uint32_t), &faceLength, &bytesWritten), &file);
		CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(char) * faceLength,
			const_cast<char*>(resource->fonts[i].name.CStr()), &bytesWritten), &file);
	}

	FileSystemClose(&file);
	return true;
}
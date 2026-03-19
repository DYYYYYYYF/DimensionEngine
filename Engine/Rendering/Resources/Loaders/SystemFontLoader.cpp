#include "SystemFontLoader.hpp"
#include "Rendering/Resources/Font/SystemFont.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Platform/File/File.hpp"
#include "Systems/ResourceSystem.h"

#include <stdio.h>

SystemFontLoader::SystemFontLoader() {
	Type = EAssetType::SystemFont;
	TypePath = "Fonts";
}

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

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedSystemFontFiletype supportedTypes[SUPPORTED_FILETYPE_COUNT];
	supportedTypes[0] = { ".dsf",      SystemFontFileType::eSystem_Font_File_Type_DSF,         true };
	supportedTypes[1] = { ".fontcfg",  SystemFontFileType::eSystem_Font_File_Type_Font_Config,  false };

	FString fullFilePath;
	SystemFontFileType fontType = SystemFontFileType::eSystem_Font_File_Type_Not_Found;

	for (uint32_t i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
		fullFilePath = FString::Format(formatStr,
			ResourceSystem::Get().GetRootPath(), TypePath.CStr(),
			name.CStr(), supportedTypes[i].extension);

		File AssetFile(fullFilePath.CStr());
		if (AssetFile.IsExist()) {
			fontType = supportedTypes[i].type;
			break;
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
		result = ReadDSFFile(fullFilePath, resourceData);
		break;

	case eSystem_Font_File_Type_Font_Config: {
		FString dsfFilename = FString::Format("%s/%s/%s%s",
			ResourceSystem::Get().GetRootPath(), TypePath.CStr(), name.CStr(), ".dsf");
		result = ImportFontconfigFile(fullFilePath, TypePath, dsfFilename, resourceData);
		break;
	}
	}

	if (!result) {
		GLOG(Log::eError, "SystemFontLoader: failed to process font file: '%s'.", fullFilePath.CStr());
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

bool SystemFontLoader::ImportFontconfigFile(const FString& configPath, const FString& typePath,
	const FString& outDSFFilename, SystemFontResourceData* outResource) {

	outResource->binarySize = 0;
	outResource->fontBinary = nullptr;

	File configFile(configPath.CStr());
	if (!configFile.IsExist()) {
		GLOG(Log::eError, "SystemFontLoader: config file '%s' does not exist.", configPath.CStr());
		return false;
	}

	uint32_t lineNumber = 1;
	bool parseSuccess = configFile.ReadLineByLine(
		[this, typePath, outResource, &lineNumber](size_t index, const FString& line) -> bool {
			// 跳过空行
			FString trimmed = line.Trimmed();
			if (trimmed.IsEmpty()) {
				++lineNumber;
				return true;
			}

			// 分割 key=value
			int equalPos = trimmed.IndexOf('=');
			if (equalPos == -1) {
				GLOG(Log::eWarn, "SystemFontLoader: '=' not found on line %u, skipping.", lineNumber);
				++lineNumber;
				return true;
			}

			FString varName = trimmed.SubStr(0, static_cast<size_t>(equalPos)).Trimmed();
			FString value = trimmed.SubStr(static_cast<size_t>(equalPos + 1)).Trimmed();

			if (varName.Equali("version")) {
				// TODO: version 处理
			}
			else if (varName.Equali("file")) {
				FString fullFontPath = FString::Format("%s/%s/%s",
					ResourceSystem::Get().GetRootPath(), typePath.CStr(), value.CStr());

				File fontFile(fullFontPath);
				if (!fontFile.IsExist()) {
					GLOG(Log::eError, "SystemFontLoader: binary font not found: %s.", fullFontPath.CStr());
					return false;
				}

				auto bytes = fontFile.ReadBytes();
				if (bytes.empty()) {
					GLOG(Log::eError, "SystemFontLoader: failed to read binary font: %s.", fullFontPath.CStr());
					return false;
				}

				outResource->binarySize = bytes.size();
				outResource->fontBinary = Memory::Allocate(outResource->binarySize,
					MemoryType::eMemory_Type_Resource);
				Memory::Copy(outResource->fontBinary, bytes.data(), outResource->binarySize);
			}
			else if (varName.Equali("face")) {
				SystemFontFace newFace;
				newFace.name = value;
				outResource->fonts.Push(newFace);
			}

			++lineNumber;
			return true;
		});

	if (!parseSuccess) {
		GLOG(Log::eError, "SystemFontLoader: failed to read config file '%s'.", configPath.CStr());
		return false;
	}

	if (!outResource->fontBinary || outResource->fonts.IsEmpty()) {
		GLOG(Log::eError,
			"SystemFontLoader: config must provide a binary and at least one face.");
		return false;
	}

	return WriteDSFFile(outDSFFilename, outResource);
}

bool SystemFontLoader::ReadDSFFile(const FString& path, SystemFontResourceData* data) {
	File f(path.CStr());
	if (!f.IsExist()) {
		GLOG(Log::eError, "SystemFontLoader: DSF file '%s' does not exist.", path.CStr());
		return false;
	}

	if (!f.Open(eFileMode::Read, true)) {
		GLOG(Log::eError, "SystemFontLoader: failed to open DSF file '%s'.", path.CStr());
		return false;
	}

	// 文件头校验
	ResourceHeader header;
	if (!f.Read(&header)) return false;

	if (header.magicNumber != RESOURCES_MAGIC ||
		header.resourceType != static_cast<char>(EAssetType::SystemFont)) {
		GLOG(Log::eError, "SystemFontLoader: DSF file header is invalid.");
		return false;
	}

	// TTF 二进制大小
	if (!f.Read(&data->binarySize)) return false;

	// TTF 二进制数据
	data->fontBinary = Memory::Allocate(data->binarySize, MemoryType::eMemory_Type_Resource);
	if (!f.ReadBuffer(data->fontBinary, data->binarySize)) return false;

	// Face 数量
	uint32_t faceCount = 0;
	if (!f.Read(&faceCount)) return false;
	data->fonts.Resize(faceCount);

	// 每个 face 的名称
	for (uint32_t i = 0; i < faceCount; ++i) {
		uint32_t faceLength = 0;
		if (!f.Read(&faceLength)) return false;

		char* faceBuf = static_cast<char*>(
			Memory::Allocate(sizeof(char) * faceLength, MemoryType::eMemory_Type_String));
		if (!f.ReadBuffer(faceBuf, sizeof(char) * faceLength)) {
			Memory::Free(faceBuf, MemoryType::eMemory_Type_String);
			return false;
		}
		data->fonts[i].name = faceBuf;
		Memory::Free(faceBuf, MemoryType::eMemory_Type_String);
	}

	f.Close();
	return true;
}

bool SystemFontLoader::WriteDSFFile(const FString& outDSFFilename, SystemFontResourceData* resource) {
	File f(outDSFFilename.CStr());
	if (!f.Open(eFileMode::Write, true)) {
		GLOG(Log::eError, "SystemFontLoader: failed to open DSF for writing: %s.", outDSFFilename.CStr());
		return false;
	}

	// 文件头
	ResourceHeader header;
	header.magicNumber = RESOURCES_MAGIC;
	header.resourceType = static_cast<char>(EAssetType::SystemFont);
	header.version = 0x01U;
	header.reserved = 0;
	if (!f.Write(&header)) return false;

	// TTF 二进制大小和数据
	if (!f.Write(&resource->binarySize))                              return false;
	if (!f.WriteBuffer(resource->fontBinary, resource->binarySize))   return false;

	// Face 数量
	uint32_t faceCount = static_cast<uint32_t>(resource->fonts.Size());
	if (!f.Write(&faceCount)) return false;

	// 每个 face 的名称
	for (uint32_t i = 0; i < faceCount; ++i) {
		uint32_t faceLength = static_cast<uint32_t>(resource->fonts[i].name.Length()) + 1;
		if (!f.Write(&faceLength)) return false;
		if (!f.WriteBuffer(resource->fonts[i].name.CStr(), sizeof(char) * faceLength)) return false;
	}

	f.Close();
	return true;
}
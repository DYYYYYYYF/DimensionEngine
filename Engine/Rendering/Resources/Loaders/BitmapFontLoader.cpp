#include "BitmapFontLoader.hpp"
#include "Rendering/Resources/Font/BitmapFont.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Platform/File/File.hpp"
#include "Systems/ResourceSystem.h"

#include <stdio.h>

BitmapFontLoader::BitmapFontLoader() {
	Type = EAssetType::BitmapFont;
	TypePath = "Fonts";
}

// ─────────────────────────────────────────────────────────────────
//  Load / Unload
// ─────────────────────────────────────────────────────────────────

bool BitmapFontLoader::Load(const FString& name, void* params, UAsset* resource) {
	if (name.Length() == 0 || resource == nullptr) {
		return false;
	}

	// resource 的真实类型是 BitmapFont（继承自 UAsset）
	// Loader 只填充 BitmapFontResourceData，不做 GPU 初始化
	BitmapFont* fontAsset = static_cast<BitmapFont*>(resource);

	const char* formatStr = "%s/%s/%s%s";

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedBitmapFontFileType supportedTypes[SUPPORTED_FILETYPE_COUNT];
	supportedTypes[0] = { ".dbf", BitmapFontFileType::eBitmap_Font_File_Type_DBF, true };
	supportedTypes[1] = { ".fnt", BitmapFontFileType::eBitmap_Font_File_Type_FNT, false };

	FString fullFilePath;
	BitmapFontFileType fontType = BitmapFontFileType::eBitmap_Font_File_Type_Not_Found;

	for (uint32_t i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
		fullFilePath = FString::Format(formatStr,
			ResourceSystem::GetRootPath(), TypePath.c_str(),
			name.CStr(), supportedTypes[i].extension);

		File AssetFile(fullFilePath.CStr());
		if (AssetFile.IsExist()) {
			fontType = supportedTypes[i].type;
			break;
		}
	}

	if (fontType == BitmapFontFileType::eBitmap_Font_File_Type_Not_Found) {
		GLOG(Log::eError, "BitmapFontLoader: unable to find font of supported type: '%s'.", name.CStr());
		return false;
	}

	resource->FullPath = fullFilePath;
	resource->Name = name;

	// ResourceData 临时栈上分配，data 指针指向 fontAsset 自身
	// Loader 将解析出的 glyph/kerning/page 数据直接写入 fontAsset
	BitmapFontResourceData resourceData;
	resourceData.data = fontAsset;

	bool result = false;
	switch (fontType) {
	case eBitmap_Font_File_Type_Not_Found:
		GLOG(Log::eError, "BitmapFontLoader: font type not found for '%s'.", name.CStr());
		break;

	case eBitmap_Font_File_Type_DBF:
		result = ReadDbfFile(fullFilePath, &resourceData);
		break;

	case eBitmap_Font_File_Type_FNT: {
		FString dbfFilename = FString::Format("%s/%s/%s%s",
			ResourceSystem::GetRootPath(), TypePath.c_str(), name.CStr(), ".dbf");
		result = ImportFntFile(fullFilePath, dbfFilename, &resourceData);
		break;
	}
	}

	if (!result) {
		GLOG(Log::eError, "BitmapFontLoader: failed to process font file: '%s'.", fullFilePath.CStr());
		resource->FullPath = nullptr;
		resource->Data = nullptr;
		resource->DataSize = 0;
		return false;
	}

	// 将 resourceData 持久化到堆上，供 BitmapFont::InitFromResourceData() 使用
	// BitmapFont 会在 InitFromResourceData() 结束后不再需要它，由 Unload() 释放
	resource->Data = Memory::Allocate(sizeof(BitmapFontResourceData), MemoryType::eMemory_Type_Bitmap_Font);
	resource->DataSize = sizeof(BitmapFontResourceData);
	Memory::Copy(resource->Data, &resourceData, sizeof(BitmapFontResourceData));

	return true;
}

void BitmapFontLoader::Unload(UAsset* resource) {
	if (!resource || !resource->Data) { return; }

	BitmapFontResourceData* data = static_cast<BitmapFontResourceData*>(resource->Data);

	// glyph / kerning 数据的所有权在 BitmapFont 自身（data->data 指向它）
	// BitmapFont 析构时负责释放，此处只清理 Page 元数据
	if (data->pageCount && data->Pages) {
		Memory::Free(data->Pages, MemoryType::eMemory_Type_Array);
		data->Pages = nullptr;
		data->pageCount = 0;
	}

	Memory::Free(resource->Data, MemoryType::eMemory_Type_Bitmap_Font);
	resource->Data = nullptr;
	resource->DataSize = 0;
	resource->DataCount = 0;
	resource->LoaderID = INVALID_ID;
}

// ─────────────────────────────────────────────────────────────────
//  ImportFntFile — 解析 AngelCode .fnt 文本格式
// ─────────────────────────────────────────────────────────────────

#define VERIFY_LINE(line_type, line_num, expected, actual)                          \
	if ((actual) != (expected)) {                                                   \
		GLOG(Log::eError,                                                           \
			"BitmapFontLoader: format error in type '%s', line %u. "               \
			"Expected %d element(s) but read %d.",                                 \
			(line_type), (line_num), (expected), (actual));                        \
		return false;                                                               \
	}

bool BitmapFontLoader::ImportFntFile(const FString& asset_path, const FString& outDbfFilename, BitmapFontResourceData* outData) {
	uint32_t glyphsRead = 0;
	uint32_t kerningsRead = 0;

	File AssetFile(asset_path.CStr());

	// ReadLineByLine 内部自己开关文件，不需要手动 Open/Close
	// 同时在 lambda 内部维护行号
	bool success = AssetFile.ReadLineByLine(
		[this, &glyphsRead, &kerningsRead, outData]
		(size_t lineIndex, const std::string& line) -> bool {
			if (line.empty()) {
				return true;
			}
			return ParseFntLine(line.c_str(), (uint32_t)lineIndex, &glyphsRead, &kerningsRead, outData);
		});

	if (!success) {
		GLOG(Log::eError, "BitmapFontLoader: failed to read fnt file '%s'.", asset_path.CStr());
		return false;
	}

	return WriteDbfFile(outDbfFilename, outData);
}

bool BitmapFontLoader::ParseFntLine(const FString& LineBuffer, uint32_t lineNum, 
	uint32_t* glyphsRead, uint32_t* kerningsRead, BitmapFontResourceData* outData) {

	BitmapFont* font = outData->data;
	char firstChar = LineBuffer[0];

	switch (firstChar) {
	case 'i': {
		// info line
		char* tempFace = static_cast<char*>(
			Memory::Allocate(sizeof(char) * 512, MemoryType::eMemory_Type_String));
		int read = sscanf(LineBuffer.CStr(), "info face=\"%[^\"]\" size=%u",
			tempFace, &font->size_);
		font->face_ = tempFace;
		Memory::Free(tempFace, MemoryType::eMemory_Type_String);
		VERIFY_LINE("info", lineNum, 2, read);
		break;
	}

	case 'c': {
		if (LineBuffer[1] == 'o') {
			// common line
			int read = sscanf(LineBuffer.CStr(),
				"common lineHeight=%d base=%u scaleW=%d scaleH=%d pages=%d",
				&font->lineHeight_, &font->baseLine_,
				&font->atlasSizeX_, &font->atlasSizeY_,
				&outData->pageCount);
			VERIFY_LINE("common", lineNum, 5, read);

			if (outData->pageCount == 0) {
				GLOG(Log::eError, "BitmapFontLoader: page count is 0, aborting.");
				return true;
			}
			if (!outData->Pages) {
				outData->Pages = static_cast<BitmapFontPage*>(
					Memory::Allocate(sizeof(BitmapFontPage) * outData->pageCount,
						MemoryType::eMemory_Type_Array));
			}
		}
		else if (LineBuffer[1] == 'h') {
			if (LineBuffer[4] == 's') {
				// chars count
				int read = sscanf(LineBuffer.CStr(), "chars count=%u", &font->glyphCount_);
				VERIFY_LINE("chars", lineNum, 1, read);

				if (font->glyphCount_ == 0) {
					GLOG(Log::eError, "BitmapFontLoader: glyph count is 0, aborting.");
					return true;
				}
				font->glyphs_ = static_cast<FontGlyph*>(
					Memory::Allocate(sizeof(FontGlyph) * font->glyphCount_,
						MemoryType::eMemory_Type_Array));
			}
			else {
				// char record
				FontGlyph* g = &font->glyphs_[*glyphsRead];
				int read = sscanf(LineBuffer.CStr(),
					"char id=%d x=%hu y=%hu width=%hu height=%hu "
					"xoffset=%hd yoffset=%hd xadvance=%hd page=%hhu chnl=%*u",
					&g->codePoint, &g->x, &g->y, &g->width, &g->height,
					&g->offsetX, &g->offsetY, &g->advanceX, &g->pageID);
				VERIFY_LINE("char", lineNum, 9, read);
				++(*glyphsRead);
			}
		}
		break;
	}

	case 'p': {
		// page record
		char pageId = 0;
		char* tempFile = static_cast<char*>(
			Memory::Allocate(sizeof(char) * 512, MemoryType::eMemory_Type_String));
		int read = sscanf(LineBuffer.CStr(), "page id=%hhi file=\"%[^\"]\"",
			&pageId, tempFile);
		VERIFY_LINE("page", lineNum, 2, read);

		if (pageId < (int)outData->pageCount) {
			BitmapFontPage* page = &outData->Pages[pageId];
			page->id = static_cast<char>(pageId);
			// 去掉扩展名
			FString nameNoExt = FString::FilenameNoExtensionFromPath(tempFile);
			page->filename = nameNoExt;
		}

		break;
	}

	case 'k': {
		if (LineBuffer[7] == 's') {
			// kernings count
			int read = sscanf(LineBuffer.CStr(), "kernings count=%u", &font->kerningCount_);
			VERIFY_LINE("kernings", lineNum, 1, read);

			if (font->kerningCount_ > 0 && !font->kernings_) {
				font->kernings_ = static_cast<FontKerning*>(
					Memory::Allocate(sizeof(FontKerning) * font->kerningCount_,
						MemoryType::eMemory_Type_Array));
			}
		}
		else if (LineBuffer[7] == ' ') {
			// kerning record
			FontKerning* k = &font->kernings_[*kerningsRead];
			int read = sscanf(LineBuffer.CStr(),
				"kerning first=%i second=%i amount=%hi",
				&k->codePoint0, &k->codePoint1, &k->amount);
			VERIFY_LINE("kerning", lineNum, 3, read);
			++(*kerningsRead);
		}
		break;
	}

	default:
		break;
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────
//  ReadDbfFile — 读取引擎二进制格式
// ─────────────────────────────────────────────────────────────────

bool BitmapFontLoader::ReadDbfFile(const FString& asset_path, BitmapFontResourceData* data) {
	File f(asset_path.CStr());
	if (!f.IsExist()) {
		GLOG(Log::eError, "BitmapFontLoader: DBF file '%s' does not exist.", asset_path.CStr());
		return false;
	}

	if (!f.Open(eFileMode::Read, true)) {
		GLOG(Log::eError, "BitmapFontLoader: failed to open DBF file '%s'.", asset_path.CStr());
		return false;
	}

	// 文件头
	ResourceHeader header;
	if (!f.Read(&header)) return false;

	if (header.magicNumber != RESOURCES_MAGIC ||
		header.resourceType != static_cast<char>(EAssetType::BitmapFont)) {
		GLOG(Log::eError, "BitmapFontLoader: DBF file header is invalid.");
		return false;
	}

	BitmapFont* font = data->data;

	// Face 字符串
	uint32_t faceLength = 0;
	if (!f.Read(&faceLength)) return false;
	char* faceBuf = static_cast<char*>(
		Memory::Allocate(sizeof(char) * faceLength, MemoryType::eMemory_Type_String));
	if (!f.ReadBuffer(faceBuf, sizeof(char) * faceLength)) {
		Memory::Free(faceBuf, MemoryType::eMemory_Type_String);
		return false;
	}
	font->face_ = faceBuf;
	Memory::Free(faceBuf, MemoryType::eMemory_Type_String);

	// 基础度量
	if (!f.Read(&font->size_))      return false;
	if (!f.Read(&font->lineHeight_)) return false;
	if (!f.Read(&font->baseLine_))  return false;
	if (!f.Read(&font->atlasSizeX_)) return false;
	if (!f.Read(&font->atlasSizeY_)) return false;

	// Pages
	if (!f.Read(&data->pageCount)) return false;
	data->Pages = static_cast<BitmapFontPage*>(
		Memory::Allocate(sizeof(BitmapFontPage) * data->pageCount, MemoryType::eMemory_Type_Array));

	for (uint32_t i = 0; i < data->pageCount; ++i) {
		if (!f.Read(&data->Pages[i].id)) return false;

		uint32_t filenameLen = 0;
		if (!f.Read(&filenameLen)) return false;

		char* filenameBuf = static_cast<char*>(
			Memory::Allocate(sizeof(char) * filenameLen, MemoryType::eMemory_Type_String));
		if (!f.ReadBuffer(filenameBuf, sizeof(char) * filenameLen)) {
			Memory::Free(filenameBuf, MemoryType::eMemory_Type_String);
			return false;
		}
		data->Pages[i].filename = filenameBuf;
		Memory::Free(filenameBuf, MemoryType::eMemory_Type_String);
	}

	// Glyphs
	if (!f.Read(&font->glyphCount_)) return false;
	font->glyphs_ = static_cast<FontGlyph*>(
		Memory::Allocate(sizeof(FontGlyph) * font->glyphCount_, MemoryType::eMemory_Type_Array));
	if (!f.ReadBuffer(font->glyphs_, sizeof(FontGlyph) * font->glyphCount_)) return false;

	// Kernings
	if (!f.Read(&font->kerningCount_)) return false;
	if (font->kerningCount_ > 0) {
		font->kernings_ = static_cast<FontKerning*>(
			Memory::Allocate(sizeof(FontKerning) * font->kerningCount_, MemoryType::eMemory_Type_Array));
		if (!f.ReadBuffer(font->kernings_, sizeof(FontKerning) * font->kerningCount_)) return false;
	}

	f.Close();
	return true;
}

bool BitmapFontLoader::WriteDbfFile(const FString& asset_path, BitmapFontResourceData* data) {
	File f(asset_path.CStr());
	if (!f.Open(eFileMode::Write, true)) {
		GLOG(Log::eError, "BitmapFontLoader: failed to open file for writing: %s.", asset_path.CStr());
		return false;
	}

	BitmapFont* font = data->data;

	// 文件头
	ResourceHeader header;
	header.magicNumber = RESOURCES_MAGIC;
	header.resourceType = static_cast<char>(EAssetType::BitmapFont);
	header.version = 0x01U;
	header.reserved = 0;
	if (!f.Write(&header)) return false;

	// Face 字符串
	uint32_t faceLength = static_cast<uint32_t>(font->GetFace().Length()) + 1;
	if (!f.Write(&faceLength)) return false;
	if (!f.WriteBuffer(font->GetFace().CStr(), sizeof(char) * faceLength)) return false;

	// 基础度量
	if (!f.Write(&font->size_))       return false;
	if (!f.Write(&font->lineHeight_)) return false;
	if (!f.Write(&font->baseLine_))   return false;
	if (!f.Write(&font->atlasSizeX_)) return false;
	if (!f.Write(&font->atlasSizeY_)) return false;

	// Pages
	if (!f.Write(&data->pageCount)) return false;
	for (uint32_t i = 0; i < data->pageCount; ++i) {
		if (!f.Write(&data->Pages[i].id)) return false;

		uint32_t filenameLen = static_cast<uint32_t>(data->Pages[i].filename.Length()) + 1;
		if (!f.Write(&filenameLen)) return false;
		if (!f.WriteBuffer(data->Pages[i].filename.CStr(), sizeof(char) * filenameLen)) return false;
	}

	// Glyphs
	if (!f.Write(&font->glyphCount_)) return false;
	if (!f.WriteBuffer(font->glyphs_, sizeof(FontGlyph) * font->glyphCount_)) return false;

	// Kernings
	if (!f.Write(&font->kerningCount_)) return false;
	if (font->kerningCount_ > 0) {
		if (!f.WriteBuffer(font->kernings_, sizeof(FontKerning) * font->kerningCount_)) return false;
	}

	f.Close();
	return true;
}
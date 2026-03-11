#include "BitmapFontLoader.hpp"
#include "Rendering/Resources/Font/BitmapFont.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"
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
	FileHandle f;

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedBitmapFontFileType supportedTypes[SUPPORTED_FILETYPE_COUNT];
	supportedTypes[0] = { ".dbf", BitmapFontFileType::eBitmap_Font_File_Type_DBF, true };
	supportedTypes[1] = { ".fnt", BitmapFontFileType::eBitmap_Font_File_Type_FNT, false };

	char fullFilePath[512];
	BitmapFontFileType fontType = BitmapFontFileType::eBitmap_Font_File_Type_Not_Found;

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
		result = ReadDbfFile(&f, &resourceData);
		break;

	case eBitmap_Font_File_Type_FNT: {
		char dbfFilename[512];
		StringFormat(dbfFilename, "%s/%s/%s%s",
			ResourceSystem::GetRootPath(), TypePath.c_str(), name.CStr(), ".dbf");
		result = ImportFntFile(&f, dbfFilename, &resourceData);
		break;
	}
	}

	FileSystemClose(&f);

	if (!result) {
		GLOG(Log::eError, "BitmapFontLoader: failed to process font file: '%s'.", fullFilePath);
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

bool BitmapFontLoader::ImportFntFile(FileHandle* fntFile, const char* outDbfFilename, BitmapFontResourceData* outData) {
	char    lineBuf[512] = "";
	char* p = &lineBuf[0];
	size_t  lineLength = 0;
	uint32_t lineNum = 0;
	uint32_t glyphsRead = 0;
	uint32_t kerningsRead = 0;

	// outData->data は BitmapFont* であり、フィールドへのアクセスには
	// BitmapFont の protected accessor が必要。ここでは friend 経由か
	// LoaderInterface を通じてアクセスする想定。
	// 簡便のため BitmapFont に LoaderData 構造体を公開する方式を採用。
	BitmapFont* font = outData->data;

	while (true) {
		++lineNum;
		if (!FileSystemReadLine(fntFile, 511, &p, &lineLength)) { break; }
		if (lineLength < 1) { continue; }

		char firstChar = lineBuf[0];
		switch (firstChar) {
		case 'i': {
			// info line — face と size のみ取得
			char* tempFace = static_cast<char*>(
				Memory::Allocate(sizeof(char) * 512, MemoryType::eMemory_Type_String));
			int read = sscanf(lineBuf, "info face=\"%[^\"]\" size=%u",
				tempFace, &font->size_);
			font->face_ = tempFace;
			Memory::Free(tempFace, MemoryType::eMemory_Type_String);
			VERIFY_LINE("info", lineNum, 2, read);
			break;
		}

		case 'c': {
			if (lineBuf[1] == 'o') {
				// common line
				int read = sscanf(lineBuf,
					"common lineHeight=%d base=%u scaleW=%d scaleH=%d pages=%d",
					&font->lineHeight_, &font->baseLine_,
					&font->atlasSizeX_, &font->atlasSizeY_,
					&outData->pageCount);
				VERIFY_LINE("common", lineNum, 5, read);

				if (outData->pageCount == 0) {
					GLOG(Log::eError, "BitmapFontLoader: page count is 0, aborting.");
					return false;
				}
				if (!outData->Pages) {
					outData->Pages = static_cast<BitmapFontPage*>(
						Memory::Allocate(sizeof(BitmapFontPage) * outData->pageCount,
							MemoryType::eMemory_Type_Array));
				}
			}
			else if (lineBuf[1] == 'h') {
				if (lineBuf[4] == 's') {
					// chars count
					int read = sscanf(lineBuf, "chars count=%u", &font->glyphCount_);
					VERIFY_LINE("chars", lineNum, 1, read);

					if (font->glyphCount_ == 0) {
						GLOG(Log::eError, "BitmapFontLoader: glyph count is 0, aborting.");
						return false;
					}
					font->glyphs_ = static_cast<FontGlyph*>(
						Memory::Allocate(sizeof(FontGlyph) * font->glyphCount_,
							MemoryType::eMemory_Type_Array));
				}
				else {
					// char record
					FontGlyph* g = &font->glyphs_[glyphsRead];
					int read = sscanf(lineBuf,
						"char id=%d x=%hu y=%hu width=%hu height=%hu "
						"xoffset=%hd yoffset=%hd xadvance=%hd page=%hhu chnl=%*u",
						&g->codePoint, &g->x, &g->y, &g->width, &g->height,
						&g->offsetX, &g->offsetY, &g->advanceX, &g->pageID);
					VERIFY_LINE("char", lineNum, 9, read);
					++glyphsRead;
				}
			}
			break;
		}

		case 'p': {
			// page record
			BitmapFontPage* page = &outData->Pages[0]; // 暂只支持单 page
			char* tempFile = static_cast<char*>(
				Memory::Allocate(sizeof(char) * 512, MemoryType::eMemory_Type_String));
			int read = sscanf(lineBuf, "page id=%hhi file=\"%[^\"]\"",
				&page->id, tempFile);
			page->filename = tempFile;
			Memory::Free(tempFile, MemoryType::eMemory_Type_String);

			// 去掉扩展名
			char* nameNoExt = static_cast<char*>(
				Memory::Allocate(sizeof(char) * (page->filename.Length() + 1),
					MemoryType::eMemory_Type_String));
			StringFilenameNoExtensionFromPath(nameNoExt, page->filename.CStr());
			page->filename = nameNoExt;
			Memory::Free(nameNoExt, MemoryType::eMemory_Type_String);

			VERIFY_LINE("page", lineNum, 2, read);
			break;
		}

		case 'k': {
			if (lineBuf[7] == 's') {
				// kernings count
				int read = sscanf(lineBuf, "kernings count=%u", &font->kerningCount_);
				VERIFY_LINE("kernings", lineNum, 1, read);

				if (font->kerningCount_ > 0 && !font->kernings_) {
					font->kernings_ = static_cast<FontKerning*>(
						Memory::Allocate(sizeof(FontKerning) * font->kerningCount_,
							MemoryType::eMemory_Type_Array));
				}
			}
			else if (lineBuf[7] == ' ') {
				// kerning record
				FontKerning* k = &font->kernings_[kerningsRead];
				int read = sscanf(lineBuf,
					"kerning first=%i second=%i amount=%hi",
					&k->codePoint0, &k->codePoint1, &k->amount);
				VERIFY_LINE("kerning", lineNum, 3, read);
				++kerningsRead;
			}
			break;
		}

		default:
			break;
		}
	}

	return WriteDbfFile(outDbfFilename, outData);
}

// ─────────────────────────────────────────────────────────────────
//  ReadDbfFile — 读取引擎二进制格式
// ─────────────────────────────────────────────────────────────────

bool BitmapFontLoader::ReadDbfFile(FileHandle* file, BitmapFontResourceData* data) {
	size_t   bytesRead = 0;
	uint32_t readSize = 0;

	// 文件头
	ResourceHeader header;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(ResourceHeader), &header, &bytesRead), file);

	if (header.magicNumber != RESOURCES_MAGIC ||
		header.resourceType != static_cast<char>(EAssetType::BitmapFont)) {
		GLOG(Log::eError, "BitmapFontLoader: DBF file header is invalid.");
		FileSystemClose(file);
		return false;
	}

	// data->data 已由 Load() 指向 BitmapFont 自身，直接写入其成员
	BitmapFont* font = data->data;

	// Face 字符串
	uint32_t faceLength = 0;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &faceLength, &bytesRead), file);
	char* faceBuf = static_cast<char*>(
		Memory::Allocate(sizeof(char) * faceLength, MemoryType::eMemory_Type_String));
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char) * faceLength, faceBuf, &bytesRead), file);
	font->face_ = faceBuf;
	Memory::Free(faceBuf, MemoryType::eMemory_Type_String);

	// 基础度量
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &font->size_, &bytesRead), file);
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &font->lineHeight_, &bytesRead), file);
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &font->baseLine_, &bytesRead), file);
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &font->atlasSizeX_, &bytesRead), file);
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &font->atlasSizeY_, &bytesRead), file);

	// Pages
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &data->pageCount, &bytesRead), file);
	data->Pages = static_cast<BitmapFontPage*>(
		Memory::Allocate(sizeof(BitmapFontPage) * data->pageCount, MemoryType::eMemory_Type_Array));

	for (uint32_t i = 0; i < data->pageCount; ++i) {
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char), &data->Pages[i].id, &bytesRead), file);

		uint32_t filenameLen = 0;
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &filenameLen, &bytesRead), file);

		char* filenameBuf = static_cast<char*>(
			Memory::Allocate(sizeof(char) * filenameLen, MemoryType::eMemory_Type_String));
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char) * filenameLen, filenameBuf, &bytesRead), file);
		data->Pages[i].filename = filenameBuf;
		Memory::Free(filenameBuf, MemoryType::eMemory_Type_String);
	}

	// Glyphs
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &font->glyphCount_, &bytesRead), file);
	font->glyphs_ = static_cast<FontGlyph*>(
		Memory::Allocate(sizeof(FontGlyph) * font->glyphCount_, MemoryType::eMemory_Type_Array));
	readSize = sizeof(FontGlyph) * font->glyphCount_;
	CLOSE_IF_FAILED(FileSystemRead(file, readSize, font->glyphs_, &bytesRead), file);

	// Kernings
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &font->kerningCount_, &bytesRead), file);
	if (font->kerningCount_ > 0) {
		font->kernings_ = static_cast<FontKerning*>(
			Memory::Allocate(sizeof(FontKerning) * font->kerningCount_, MemoryType::eMemory_Type_Array));
		readSize = sizeof(FontKerning) * font->kerningCount_;
		CLOSE_IF_FAILED(FileSystemRead(file, readSize, font->kernings_, &bytesRead), file);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────
//  WriteDbfFile — 写出引擎二进制格式
// ─────────────────────────────────────────────────────────────────

bool BitmapFontLoader::WriteDbfFile(const char* path, BitmapFontResourceData* data) {
	FileHandle file;
	if (!FileSystemOpen(path, FileMode::eFile_Mode_Write, true, &file)) {
		GLOG(Log::eError, "BitmapFontLoader: failed to open file for writing: %s.", path);
		return false;
	}

	size_t   bytesWritten = 0;
	uint32_t writeSize = 0;

	BitmapFont* font = data->data;

	// 文件头
	ResourceHeader header;
	header.magicNumber = RESOURCES_MAGIC;
	header.resourceType = static_cast<char>(EAssetType::BitmapFont);
	header.version = 0x01U;
	header.reserved = 0;
	writeSize = sizeof(ResourceHeader);
	CLOSE_IF_FAILED(FileSystemWrite(&file, writeSize, &header, &bytesWritten), &file);

	// Face 字符串
	uint32_t faceLength = static_cast<uint32_t>(font->GetFace().Length()) + 1;
	writeSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, writeSize, &faceLength, &bytesWritten), &file);
	writeSize = sizeof(char) * faceLength;
	CLOSE_IF_FAILED(FileSystemWrite(&file, writeSize, const_cast<char*>(font->GetFace().CStr()), &bytesWritten), &file);

	// 基础度量
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(uint32_t), &font->size_, &bytesWritten), &file);
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(int), &font->lineHeight_, &bytesWritten), &file);
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(int), &font->baseLine_, &bytesWritten), &file);
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(int), &font->atlasSizeX_, &bytesWritten), &file);
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(int), &font->atlasSizeY_, &bytesWritten), &file);

	// Pages
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(uint32_t), &data->pageCount, &bytesWritten), &file);
	for (uint32_t i = 0; i < data->pageCount; ++i) {
		CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(char), &data->Pages[i].id, &bytesWritten), &file);

		uint32_t filenameLen = static_cast<uint32_t>(data->Pages[i].filename.Length()) + 1;
		CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(uint32_t), &filenameLen, &bytesWritten), &file);
		CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(char) * filenameLen,
			const_cast<char*>(data->Pages[i].filename.CStr()), &bytesWritten), &file);
	}

	// Glyphs
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(uint32_t), &font->glyphCount_, &bytesWritten), &file);
	writeSize = sizeof(FontGlyph) * font->glyphCount_;
	CLOSE_IF_FAILED(FileSystemWrite(&file, writeSize, font->glyphs_, &bytesWritten), &file);

	// Kernings
	CLOSE_IF_FAILED(FileSystemWrite(&file, sizeof(uint32_t), &font->kerningCount_, &bytesWritten), &file);
	if (font->kerningCount_ > 0) {
		writeSize = sizeof(FontKerning) * font->kerningCount_;
		CLOSE_IF_FAILED(FileSystemWrite(&file, writeSize, font->kernings_, &bytesWritten), &file);
	}

	FileSystemClose(&file);
	return true;
}
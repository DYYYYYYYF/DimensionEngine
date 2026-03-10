#include "BitmapFontLoader.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"
#include "stdio.h"

BitmapFontLoader::BitmapFontLoader() {
	Type = EAssetType::BitmapFont;
	TypePath = "Fonts";
}

bool BitmapFontLoader::Load(const std::string& name, void* params, UAsset* resource) {
	if (name.size() == 0 || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	FileHandle f;

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedBitmapFontFileType SupportedFieTypes[SUPPORTED_FILETYPE_COUNT];
	SupportedFieTypes[0] = SupportedBitmapFontFileType{ ".dbf", BitmapFontFileType::eBitmap_Font_File_Type_DBF, true };
	SupportedFieTypes[1] = SupportedBitmapFontFileType{ ".fnt", BitmapFontFileType::eBitmap_Font_File_Type_FNT, false };

	char FullFilePath[512];
	BitmapFontFileType FontType = BitmapFontFileType::eBitmap_Font_File_Type_Not_Found;
	// Try each supported extension.
	for (uint32_t i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
		StringFormat(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath.c_str(), name.c_str(), SupportedFieTypes[i].extension);
		// If the file exist, open it and stop looking.
		if (FileSystemExists(FullFilePath)) {
			if (FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, SupportedFieTypes[i].isBinary, &f)) {
				FontType = SupportedFieTypes[i].type;
				break;
			}
		}

	}

	if (FontType == BitmapFontFileType::eBitmap_Font_File_Type_Not_Found) {
		GLOG(Log::eError, "Unable to find bit map font of supported type called: '%s'.", name.c_str());
		return false;
	}

	resource->FullPath = FullFilePath;
	resource->Name = name.c_str();

	BitmapFontResourceData ResourceData;
	bool Result = false;
	switch (FontType)
	{
	case eBitmap_Font_File_Type_Not_Found:
		GLOG(Log::eError, "Unable to find bitmap font of supported type called '%s'.", name.c_str());
		Result = false;
		break;
	case eBitmap_Font_File_Type_DBF:
		Result = ReadDbfFile(&f, &ResourceData);
		break;
	case eBitmap_Font_File_Type_FNT:
		// Generate the DBF filename.
		char DBFFileName[512];
		StringFormat(DBFFileName, "%s/%s/%s%s", ResourceSystem::GetRootPath(), TypePath.c_str(), name.c_str(), ".dbf");
		Result = ImportFntFile(&f, DBFFileName, &ResourceData);
		break;
	}

	FileSystemClose(&f);
	if (!Result) {
		GLOG(Log::eError, "Failed to process bitmap font file: '%s'.", FullFilePath);
		resource->FullPath = nullptr;
		resource->Data = nullptr;
		resource->DataSize = 0;
		return false;
	}

	resource->Data = Memory::Allocate(sizeof(BitmapFontResourceData), MemoryType::eMemory_Type_Bitmap_Font);
	Memory::Copy(resource->Data, &ResourceData, sizeof(BitmapFontResourceData));
	resource->DataSize = sizeof(BitmapFontResourceData);

	return true;
}

void BitmapFontLoader::Unload(UAsset* resource) {
	if (resource == nullptr) {
		return;
	}

	if (resource->Data) {
		BitmapFontResourceData* Data = (BitmapFontResourceData*)resource->Data;
		if (Data->data->glyphCount && Data->data->glyphs) {
			Memory::Free(Data->data->glyphs, MemoryType::eMemory_Type_Array);
			Data->data->glyphs = nullptr;
		}

		if (Data->data->kerningCount && Data->data->kernings) {
			Memory::Free(Data->data->kernings, MemoryType::eMemory_Type_Array);
			Data->data->kernings = nullptr;
		}

		if (Data->pageCount && Data->Pages) {
			Memory::Free(Data->Pages, MemoryType::eMemory_Type_Array);
			Data->Pages = nullptr;
		}

		Memory::Free(resource->Data, MemoryType::eMemory_Type_Bitmap_Font);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->DataCount = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

#define VERIFY_LINE(line_type, line_num, expected, actual)	\
	if (actual != expected) {	\
		GLOG(Log::eError, "Error in file format reading type '%s', line %u. Expected %d element(s) but read %d.", line_type, line_num, expected, actual); \
		return false;	\
	}	

bool BitmapFontLoader::ImportFntFile(FileHandle* fntFile, const char* outDbfFilename, BitmapFontResourceData* out_data) {
	Memory::Zero(out_data, sizeof(BitmapFontResourceData));
	char LineBuf[512] = "";
	char* p = &LineBuf[0];
	size_t LineLength = 0;
	uint32_t LineNum = 0;
	uint32_t GlyphsRead = 0;
	unsigned char PagesRead = 0;
	uint32_t KerningsRead = 0;
	while (true) {
		++LineNum;
		if (!FileSystemReadLine(fntFile, 511, &p, &LineLength)) {
			break;
		}

		// Skip blank lines.
		if (LineLength < 1) {
			continue;
		}

		char FirstChar = LineBuf[0];
		switch (FirstChar)
		{
		case 'i': {
			// Info line.

			// NOTE: Only extract the face and size, ignore the rest.
			char* TempFacePoint = (char*)Memory::Allocate(sizeof(char) * 512, MemoryType::eMemory_Type_String);
			int ElementsRead = sscanf(
				LineBuf, "info face=\"%[^\"]\" size=%u",
				TempFacePoint,
				&out_data->data->size
			);
			out_data->data->face = TempFacePoint;
			VERIFY_LINE("info", LineNum, 2, ElementsRead);
			break;
		}
		case 'c': {
			// "common" "char" or "chars" line.
			if (LineBuf[1] == 'o') {
				// common
				int ElementsRead = sscanf(
					LineBuf,
					"common lineHeight=%d base=%u scaleW=%d scaleH=%d pages=%d",
					&out_data->data->lineHeight,
					&out_data->data->baseLine,
					&out_data->data->atlasSizeX,
					&out_data->data->atlasSizeY,
					&out_data->pageCount
					);
				VERIFY_LINE("common", LineNum, 5, ElementsRead);

				// Allocate the pages array.
				if (out_data->pageCount > 0) {
					if (!out_data->Pages) {
						out_data->Pages = (BitmapFontPage*)Memory::Allocate(sizeof(BitmapFontPage) * out_data->pageCount, MemoryType::eMemory_Type_Array);
					}
				}
				else {
					GLOG(Log::eError, "Pages is 0, which should not be possible. Font file reading aborted.");
					return false;
				}
			}
			else if (LineBuf[1] == 'h') {
				if (LineBuf[4] == 's') {
					// chars line
					int ElementsRead = sscanf(LineBuf, "chars count=%u", &out_data->data->glyphCount);
					VERIFY_LINE("chars", LineNum, 1, ElementsRead);

					// Allocate the glyphs array
					if (out_data->data->glyphCount > 0) {
						out_data->data->glyphs = (FontGlyph*)Memory::Allocate(sizeof(FontGlyph) * out_data->data->glyphCount, MemoryType::eMemory_Type_Array);
					}
					else {
						GLOG(Log::eError, "Glyph count is 0, which should not be possible. Font file reading aborted.");
						return false;
					}
				}
				else {
					// Assume char line.
					FontGlyph* g = &out_data->data->glyphs[GlyphsRead];
					int ElementsRead = sscanf(
						LineBuf, "char id=%d x=%hu y=%hu width=%hu height=%hu xoffset=%hd yoffset=%hd xadvance=%hd page=%hhu chnl=%*u",
						&g->codePoint,
						&g->x,
						&g->y,
						&g->width,
						&g->height,
						&g->offsetX,
						&g->offsetY,
						&g->advanceX,
						&g->pageID
					);

					VERIFY_LINE("char", LineNum, 9, ElementsRead);
					GlyphsRead++;
				}
			}
			else {
				// Invalid ignore.
			}
		} break;	// case 'c'
		case 'p': {
			// Page line
			BitmapFontPage* page = &out_data->Pages[PagesRead];
			char* TempFilePoint = (char*)Memory::Allocate(sizeof(char) * 512, MemoryType::eMemory_Type_String);
			int ElementsRead = sscanf(LineBuf,
				"page id=%hhi file=\"%[^\"]\"",
				&page->id,
				TempFilePoint);
			page->file = std::string(TempFilePoint);
			Memory::Free(TempFilePoint, MemoryType::eMemory_Type_String);

			// Strip the extension.
			char* f = (char*)Memory::Allocate(sizeof(char) * page->file.length() + 1, MemoryType::eMemory_Type_String);
			StringFilenameNoExtensionFromPath(f, page->file.c_str());
			page->file = std::string(f);
			Memory::Free(f, MemoryType::eMemory_Type_String);

			VERIFY_LINE("page", LineNum, 2, ElementsRead);
		}break;	// case 'p'
		case 'k': {
			// Kernings or kerning line
			if (LineBuf[7] == 's') {
				// Kernings
				int ElementRead = sscanf(LineBuf, "kernings count=%u", &out_data->data->kerningCount);

				VERIFY_LINE("kernings", LineNum, 1, ElementRead);

				// Allocate kernings array.
				if (out_data->data->kerningCount > 0 && out_data->data->kernings == nullptr) {
					out_data->data->kernings = (FontKerning*)Memory::Allocate(sizeof(FontKerning) * out_data->data->kerningCount, MemoryType::eMemory_Type_Array);
				}
			}
			else if (LineBuf[7] == ' ') {
				// Kerning record
				FontKerning* Kerning = &out_data->data->kernings[KerningsRead];
				int ElementsRead = sscanf(
					LineBuf,
					"kerning first=%i second=%i amount=%hi",
					&Kerning->codePoint0,
					&Kerning->codePoint1,
					&Kerning->amount
					);

				VERIFY_LINE("kerning", LineNum, 3, ElementsRead);
			}
		} break;	//case 'k'
		default:
			// Skip the line.
			break;
		}	// switch
	}

	// Write the binary biymap font file.
	return WriteDbfFile(outDbfFilename, out_data);
}

bool BitmapFontLoader::ReadDbfFile(FileHandle* file, BitmapFontResourceData* data) {
	size_t BytesRead = 0;
	uint32_t ReadSize = 0;

	// Write the resource header first.
	ResourceHeader Header;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(ResourceHeader), &Header, &BytesRead), file);
	switch (EAssetType(Header.resourceType))
	{
	case EAssetType::BitmapFont: {
		data->data = NewObject<BitmapFontInternalData>();
	} break;
	case EAssetType::SystemFont: {
		data->data = NewObject<SystemFontVariantData>();
	}break;
	default:
		break;
	}

	// Verify hreader contents.
	if (Header.magicNumber != RESOURCES_MAGIC && Header.resourceType == (char)EAssetType::BitmapFont) {
		GLOG(Log::eError, "DBF File header is invalid and cannot be read.");
		FileSystemClose(file);
		return false;
	}

	// TODO: read in/process file version.

	// Length of face string.
	uint32_t FaceLength = 0;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FaceLength, &BytesRead), file);

	// Face string.
	ReadSize = sizeof(char) * FaceLength;
	char* ff = (char*)Memory::Allocate(sizeof(char) * ReadSize, MemoryType::eMemory_Type_String);
	CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, ff, &BytesRead), file);
	data->data->face = ff;
	Memory::Free(ff, MemoryType::eMemory_Type_String);

	// Font size.
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &data->data->size, &BytesRead), file);

	// line height
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data->lineHeight, &BytesRead), file);

	// Baseline
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data->baseLine, &BytesRead), file);

	// Scale x
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data->atlasSizeX, &BytesRead), file);

	// Scale Y
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data->atlasSizeY, &BytesRead), file);

	// Page count
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &data->pageCount, &BytesRead), file);

	// Allocate pages array.
	data->Pages = (BitmapFontPage*)Memory::Allocate(sizeof(BitmapFontPage) * data->pageCount, MemoryType::eMemory_Type_Array);

	// Read pages.
	for (uint32_t i = 0; i < data->pageCount; ++i) {
		// Page id.
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char), &data->Pages[i].id, &BytesRead), file);

		// File name length
		uint32_t FilenameLength = (uint32_t)strlen(data->Pages[i].file.c_str());
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FilenameLength, &BytesRead), file);

		// The file name
		ReadSize = sizeof(char) * FilenameLength;
		char* frd = (char*)Memory::Allocate(sizeof(char) * ReadSize, MemoryType::eMemory_Type_String);
		CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, frd, &BytesRead), file);
		data->Pages[i].file = std::string(frd);
		Memory::Free(frd, MemoryType::eMemory_Type_String);
	}

	// Glyph count
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &data->data->glyphCount, &BytesRead), file);

	// Allocate glyphs array.
	data->data->glyphs = (FontGlyph*)Memory::Allocate(sizeof(FontGlyph) * data->data->glyphCount, MemoryType::eMemory_Type_Array);

	// Read glyphs.
	ReadSize = sizeof(FontGlyph) * data->data->glyphCount;
	CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, data->data->glyphs, &BytesRead), file);

	// Kerning count
	ReadSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, &data->data->kerningCount, &BytesRead), file);

	// It's possible to have a font with no kernings. If this is the case, nothing can be written. This
	// is also why this is done last.
	if (data->data->kerningCount > 0) {
		data->data->kernings = (FontKerning*)Memory::Allocate(sizeof(FontKerning) * data->data->kerningCount, MemoryType::eMemory_Type_Array);

		// No strings for kernings, so write the entire block.
		ReadSize = sizeof(FontKerning) * data->data->kerningCount;
		CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, data->data->kernings, &BytesRead), file);
	}

	return true;
}

bool BitmapFontLoader::WriteDbfFile(const char* path, BitmapFontResourceData* data) {
	// Header info first.
	FileHandle file;
	if (!FileSystemOpen(path, FileMode::eFile_Mode_Write, true, &file)) {
		GLOG(Log::eError, "Failed to open file for writing: %s.", path);
		return false;
	}

	size_t BytesWritten = 0;
	uint32_t WriteSize = 0;

	// Write the resource header first.
	ResourceHeader Header;
	Header.magicNumber = RESOURCES_MAGIC;
	Header.resourceType = (char)EAssetType::BitmapFont;
	Header.version = 0x01U;
	Header.reserved = 0;
	WriteSize = sizeof(ResourceHeader);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &Header, &BytesWritten), &file);

	// Length of face string.
	uint32_t FaceLength = (uint32_t)data->data->face.Length() + 1;
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &FaceLength, &BytesWritten), &file);

	// Face string
	WriteSize = sizeof(char) * FaceLength;
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, (void*)data->data->face.CStr(), &BytesWritten), &file);

	// Font size
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data->size, &BytesWritten), &file);

	// Line height
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data->lineHeight, &BytesWritten), &file);

	// Baseline
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data->baseLine, &BytesWritten), &file);

	// scale x
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data->atlasSizeX, &BytesWritten), &file);

	// scale y
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data->atlasSizeY, &BytesWritten), &file);

	// page count
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->pageCount, &BytesWritten), &file);

	// Write pages.
	for (uint32_t i = 0; i < data->pageCount; ++i) {
		// Page id
		WriteSize = sizeof(char);
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->Pages[i].id, &BytesWritten), &file);

		// File name length
		uint32_t FilenameLength = (uint32_t)data->Pages[i].file.length() + 1;
		WriteSize = sizeof(uint32_t);
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &FilenameLength, &BytesWritten), &file);

		// File name
		WriteSize = sizeof(char) * FilenameLength;
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, (void*)data->Pages[i].file.c_str(), &BytesWritten), &file);
	}

	// Glyph count
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data->glyphCount, &BytesWritten), &file);

	// Write glyphs. These don't contain ant strings, so can just write out the entire block.
	WriteSize = sizeof(FontGlyph) * data->data->glyphCount;
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, data->data->glyphs, &BytesWritten), &file);

	// Kerning count
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data->kerningCount, &BytesWritten), &file);

	// It's possible to have a font with no kernings. If this is the case, nothing can be written. This
	// is also why this is done last.
	if (data->data->kerningCount > 0) {
		// No strings for kernings, so write the entire block.
		WriteSize = sizeof(FontKerning) * data->data->kerningCount;
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, data->data->kernings, &BytesWritten), &file);
	}

	// Done, close the file.
	FileSystemClose(&file);
	return true;
}

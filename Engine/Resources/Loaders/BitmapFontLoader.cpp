#include "BitmapFontLoader.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Platform/FileSystem.hpp"
#include "stdio.h"

BitmapFontLoader::BitmapFontLoader() {
	CustomType = nullptr;
	Type = ResourceType::eResource_Type_Bitmap_Font;
	TypePath = "Fonts";
}

bool BitmapFontLoader::Load(const char* name, void* params, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	FileHandle f;

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedBitmapFontFileType SupportedFieTypes[SUPPORTED_FILETYPE_COUNT];
	SupportedFieTypes[0] = SupportedBitmapFontFileType{ ".dbf", BitmapFontFileType::eBitmap_Font_File_Type_DBF, true };
	SupportedFieTypes[1] = SupportedBitmapFontFileType{ ".fnt", BitmapFontFileType::eBitmap_Font_File_Type_FNT, false };

	char FullFilePath[512];
	BitmapFontFileType Type = BitmapFontFileType::eBitmap_Font_File_Type_Not_Found;
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

	if (Type == BitmapFontFileType::eBitmap_Font_File_Type_Not_Found) {
		LOG_ERROR("Unable to find bit map font of supported type called: '%s'.", name);
		return false;
	}

	resource->FullPath = StringCopy(FullFilePath);
	resource->Name = StringCopy(name);

	BitmapFontResourceData ResourceData;
	ResourceData.data.type = FontType::eFont_Type_Bitmap;

	bool Result = false;
	switch (Type)
	{
	case eBitmap_Font_File_Type_Not_Found:
		LOG_ERROR("Unable to find bitmap font of supported type called '%s'.", name);
		Result = false;
		break;
	case eBitmap_Font_File_Type_DBF:
		Result = ReadDbfFile(&f, &ResourceData);
		break;
	case eBitmap_Font_File_Type_FNT:
		// Generate the DBF filename.
		char DBFFileName[512];
		StringFormat(DBFFileName, 512, "%s/%s/%s%s", ResourceSystem::GetRootPath(), TypePath, name, ".dbf");
		Result = ImportFntFile(&f, DBFFileName, &ResourceData);
		break;
	}

	FileSystemClose(&f);
	if (!Result) {
		LOG_ERROR("Failed to process bitmap font file: '%s'.", FullFilePath);
		Memory::Free(resource->FullPath, sizeof(char) * (strlen(resource->FullPath) + 1), MemoryType::eMemory_Type_String);
		resource->FullPath = nullptr;
		resource->Data = nullptr;
		resource->DataSize = 0;
		return false;
	}

	resource->Data = Memory::Allocate(sizeof(BitmapFontResourceData), MemoryType::eMemory_Type_Resource);
	Memory::Copy(resource->Data, &ResourceData, sizeof(BitmapFontResourceData));
	resource->DataSize = sizeof(BitmapFontResourceData);

	return true;
}

void BitmapFontLoader::Unload(Resource* resource) {
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
		BitmapFontResourceData* Data = (BitmapFontResourceData*)resource->Data;
		if (Data->data.glyphCount && Data->data.glyphs) {
			Memory::Free(Data->data.glyphs, sizeof(FontGlyph) * Data->data.glyphCount, MemoryType::eMemory_Type_Array);
			Data->data.glyphs = nullptr;
		}

		if (Data->data.kerningCount && Data->data.kernings) {
			Memory::Free(Data->data.kernings, sizeof(FontKerning) * Data->data.kerningCount, MemoryType::eMemory_Type_Array);
			Data->data.kernings = nullptr;
		}

		if (Data->pageCount && Data->Pages) {
			Memory::Free(Data->Pages, sizeof(BitmapFontPage) * Data->pageCount, MemoryType::eMemory_Type_Array);
			Data->Pages = nullptr;
		}

		Memory::Free(resource->Data, resource->DataSize * resource->DataCount, MemoryType::eMemory_Type_Texture);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->DataCount = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

#define VERIFY_LINE(line_type, line_num, expected, actual)	\
	if (actual != expected) {	\
		LOG_ERROR("Error in file format reading type '%s', line %u. Expected %d element(s) but read %d.", line_type, line_num, expected, actual); \
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
			int ElementsRead = sscanf(
				LineBuf, "info face=\"%[^\"]\" size=%u",
				out_data->data.face,
				&out_data->data.size
			);
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
					&out_data->data.lineHeight,
					&out_data->data.baseLine,
					&out_data->data.atlasSizeX,
					&out_data->data.atlasSizeY,
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
					LOG_ERROR("Pages is 0, which should not be possible. Font file reading aborted.");
					return false;
				}
			}
			else if (LineBuf[1] == 'h') {
				if (LineBuf[4] == 's') {
					// chars line
					int ElementsRead = sscanf(LineBuf, "chars count=%u", &out_data->data.glyphCount);
					VERIFY_LINE("chars", LineNum, 1, ElementsRead);

					// Allocate the glyphs array
					if (out_data->data.glyphCount > 0) {
						out_data->data.glyphs = (FontGlyph*)Memory::Allocate(sizeof(FontGlyph) * out_data->data.glyphCount, MemoryType::eMemory_Type_Array);
					}
					else {
						LOG_ERROR("Glyph count is 0, which should not be possible. Font file reading aborted.");
						return false;
					}
				}
				else {
					// Assume char line.
					FontGlyph* g = &out_data->data.glyphs[GlyphsRead];
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
			int ElementsRead = sscanf(LineBuf,
				"page id=%hhi file=\"%[^\"]\"",
				&page->id,
				page->file);
			
			// Strip the extension.
			StringFilenameNoExtensionFromPath(page->file, page->file);

			VERIFY_LINE("page", LineNum, 2, ElementsRead);
		}break;	// case 'p'
		case 'k': {
			// Kernings or kerning line
			if (LineBuf[7] == 's') {
				// Kernings
				int ElementRead = sscanf(LineBuf, "kernings count=%u", &out_data->data.kerningCount);

				VERIFY_LINE("kernings", LineNum, 1, ElementRead);

				// Allocate kernings array.
				if (out_data->data.kerningCount > 0 && out_data->data.kernings == nullptr) {
					out_data->data.kernings = (FontKerning*)Memory::Allocate(sizeof(FontKerning) * out_data->data.kerningCount, MemoryType::eMemory_Type_Array);
				}
			}
			else if (LineBuf[7] == ' ') {
				// Kerning record
				FontKerning* Kerning = &out_data->data.kernings[KerningsRead];
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
	Memory::Zero(data, sizeof(BitmapFontResourceData));

	size_t BytesRead = 0;
	uint32_t ReadSize = 0;

	// Write the resource header first.
	ResourceHeader Header;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(ResourceHeader), &Header, &BytesRead), file);

	// Verify hreader contents.
	if (Header.magicNumber != RESOURCES_MAGIC && Header.resourceType == ResourceType::eResource_Type_Bitmap_Font) {
		LOG_ERROR("DBF File header is invalid and cannot be read.");
		FileSystemClose(file);
		return false;
	}

	// TODO: read in/process file version.

	// Length of face string.
	uint32_t FaceLength = 0;
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FaceLength, &BytesRead), file);

	// Face string.
	ReadSize = sizeof(char) * FaceLength;
	CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, data->data.face, &BytesRead), file);
	// Ensure zero-termination
	// data->data.face[FaceLength] = '\0';

	// Font size.
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &data->data.size, &BytesRead), file);

	// line height
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data.lineHeight, &BytesRead), file);

	// Baseline
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data.baseLine, &BytesRead), file);

	// Scale x
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data.atlasSizeX, &BytesRead), file);

	// Scale Y
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(int), &data->data.atlasSizeY, &BytesRead), file);

	// Page count
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &data->pageCount, &BytesRead), file);

	// Allocate pages array.
	data->Pages = (BitmapFontPage*)Memory::Allocate(sizeof(BitmapFontPage) * data->pageCount, MemoryType::eMemory_Type_Array);

	// Read pages.
	for (uint32_t i = 0; i < data->pageCount; ++i) {
		// Page id.
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(char), &data->Pages[i].id, &BytesRead), file);

		// File name length
		uint32_t FilenameLength = (uint32_t)strlen(data->Pages[i].file) + 1;
		CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &FilenameLength, &BytesRead), file);

		// The file name
		ReadSize = sizeof(char) * FilenameLength;
		CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, data->Pages[i].file, &BytesRead), file);
		// Ensure zero-termination.
		// data->Pages[i].file[FilenameLength] = '\0';
	}

	// Glyph count
	CLOSE_IF_FAILED(FileSystemRead(file, sizeof(uint32_t), &data->data.glyphCount, &BytesRead), file);

	// Allocate glyphs array.
	data->data.glyphs = (FontGlyph*)Memory::Allocate(sizeof(FontGlyph) * data->data.glyphCount, MemoryType::eMemory_Type_Array);

	// Read glyphs.
	ReadSize = sizeof(FontGlyph) * data->data.glyphCount;
	CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, data->data.glyphs, &BytesRead), file);

	// Kerning count
	ReadSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, &data->data.kerningCount, &BytesRead), file);

	// It's possible to have a font with no kernings. If this is the case, nothing can be written. This
	// is also why this is done last.
	if (data->data.kerningCount > 0) {
		data->data.kernings = (FontKerning*)Memory::Allocate(sizeof(FontKerning) * data->data.kerningCount, MemoryType::eMemory_Type_Array);

		// No strings for kernings, so write the entire block.
		ReadSize = sizeof(FontKerning) * data->data.kerningCount;
		CLOSE_IF_FAILED(FileSystemRead(file, ReadSize, data->data.kernings, &BytesRead), file);
	}

	// Done.
	FileSystemClose(file);
	return true;
}

bool BitmapFontLoader::WriteDbfFile(const char* path, BitmapFontResourceData* data) {
	// Header info first.
	FileHandle file;
	if (!FileSystemOpen(path, FileMode::eFile_Mode_Write, true, &file)) {
		LOG_ERROR("Failed to open file for writing: %s.", path);
		return false;
	}

	size_t BytesWritten = 0;
	uint32_t WriteSize = 0;

	// Write the resource header first.
	ResourceHeader Header;
	Header.magicNumber = RESOURCES_MAGIC;
	Header.resourceType = ResourceType::eResource_Type_Bitmap_Font;
	Header.version = 0x01U;
	Header.reserved = 0;
	WriteSize = sizeof(ResourceHeader);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &Header, &BytesWritten), &file);

	// Length of face string.
	uint32_t FaceLength = (uint32_t)strlen(data->data.face) + 1;
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &FaceLength, &BytesWritten), &file);

	// Face string
	WriteSize = sizeof(char) * FaceLength;
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, data->data.face, &BytesWritten), &file);

	// Font size
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data.size, &BytesWritten), &file);

	// Line height
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data.lineHeight, &BytesWritten), &file);

	// Baseline
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data.baseLine, &BytesWritten), &file);

	// scale x
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data.atlasSizeX, &BytesWritten), &file);

	// scale y
	WriteSize = sizeof(int);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data.atlasSizeY, &BytesWritten), &file);

	// page count
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->pageCount, &BytesWritten), &file);

	// Write pages.
	for (uint32_t i = 0; i < data->pageCount; ++i) {
		// Page id
		WriteSize = sizeof(char);
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->Pages[i].id, &BytesWritten), &file);

		// File name length
		uint32_t FilenameLength = (uint32_t)strlen(data->Pages[i].file) + 1;
		WriteSize = sizeof(uint32_t);
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &FilenameLength, &BytesWritten), &file);

		// File name
		WriteSize = sizeof(char) * FilenameLength;
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, data->Pages[i].file, &BytesWritten), &file);
	}

	// Glyph count
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data.glyphCount, &BytesWritten), &file);

	// Write glyphs. These don't contain ant strings, so can just write out the entire block.
	WriteSize = sizeof(FontGlyph) * data->data.glyphCount;
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, data->data.glyphs, &BytesWritten), &file);

	// Kerning count
	WriteSize = sizeof(uint32_t);
	CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, &data->data.kerningCount, &BytesWritten), &file);

	// It's possible to have a font with no kernings. If this is the case, nothing can be written. This
	// is also why this is done last.
	if (data->data.kerningCount > 0) {
		// No strings for kernings, so write the entire block.
		WriteSize = sizeof(FontKerning) * data->data.kerningCount;
		CLOSE_IF_FAILED(FileSystemWrite(&file, WriteSize, data->data.kernings, &BytesWritten), &file);
	}

	// Done, close the file.
	FileSystemClose(&file);
	return true;
}

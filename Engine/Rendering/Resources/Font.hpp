#pragma once

#include "Resource.hpp"
#include "Texture.hpp"
#include <vector>
#include <string>

// --------------------------  ENUM  ------------------------- //
enum FontType {
	eFont_Type_Bitmap,
	eFont_Type_System
};

// -------------------------  STRUCT  ------------------------- //
struct FontGlyph {
	int codePoint;
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	short offsetX;
	short offsetY;
	short advanceX;
	unsigned char pageID;
};

struct FontKerning {
	int codePoint0;
	int codePoint1;
	short amount;
};

struct BitmapFontPage {
	char id = INVALID_ID_U8;
	std::string file;
};

struct BitmapFontResourceData {
	class IFontDataBase* data = nullptr;
	BitmapFontPage* Pages = nullptr;
	unsigned int pageCount = 0;
};

struct SystemFontFace {
	FString name;
};

struct SystemFontResourceData {
	std::vector<SystemFontFace> fonts;
	size_t binarySize = 0;
	void* fontBinary = nullptr;
};

// -------------------------  CLASS  ------------------------- //
class IFontDataBase {
public:
	FontType type;
	FString face;
	unsigned int size = 0;
	int lineHeight = -1;
	int baseLine = -1;
	int atlasSizeX = 1024;
	int atlasSizeY = 1024;
	struct TextureMap atlas;
	unsigned int glyphCount = 0;
	FontGlyph* glyphs = nullptr;
	unsigned int kerningCount = 0;
	FontKerning* kernings = nullptr;
	float tabXAdvance = 0.0f;
	unsigned int internalDataSize = 0;
};

class BitmapFontInternalData : public IFontDataBase {
public:
	BitmapFontInternalData() : IFontDataBase() {
		type = FontType::eFont_Type_Bitmap;
	}

public:
	Resource loadedResource;
	// Casted pointer to resource data for convenience.
	BitmapFontResourceData* resourceData = nullptr;
};

class SystemFontVariantData : public IFontDataBase {
public:
	SystemFontVariantData() : IFontDataBase() {
		type = FontType::eFont_Type_System;
	}

public:
	std::vector<int> codepoints;
	float scale;
};

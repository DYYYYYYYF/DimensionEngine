#pragma once

#include "Resource.hpp"
#include "Containers/TArray.hpp"

enum FontType {
	eFont_Type_Bitmap,
	eFont_Type_System
};

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

struct FontData {
	FontType type;
	char face[256];
	unsigned int size;
	int lineHeight;
	int baseLine;
	int atlasSizeX;
	int atlasSizeY;
	struct TextureMap atlas;
	unsigned int glyphCount;
	FontGlyph* glyphs = nullptr;
	unsigned int kerningCount;
	FontKerning* kernings = nullptr;
	float tabXAdvance;
	unsigned int internalDataSize;
	void* internalData = nullptr;
};

struct BitmapFontPage {
	char id;
	char file[256];
};

struct BitmapFontResourceData {
	FontData data;
	unsigned int pageCount;
	BitmapFontPage* Pages = nullptr;
};

struct SystemFontFace {
	char name[256];
};

struct SystemFontResourceData {
	TArray<SystemFontFace> fonts;
	size_t binarySize;
	void* fontBinary = nullptr;
};
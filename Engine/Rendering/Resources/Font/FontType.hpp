#pragma once

#include "Containers/FString.hpp"
#include "Containers/TArray.hpp"

struct FontGlyph {
	int            codePoint;
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	short          offsetX;
	short          offsetY;
	short          advanceX;
	unsigned char  pageID;
};

struct FontKerning {
	int   codePoint0;
	int   codePoint1;
	short amount;
};

struct BitmapFontPage {
	char        id = INVALID_ID_U8;
	FString filename;
};

struct BitmapFontResourceData {
	class BitmapFont* data = nullptr;   // 改为指向 BitmapFont 自身
	BitmapFontPage* Pages = nullptr;
	unsigned int      pageCount = 0;
};

struct SystemFontFace {
	FString name;
};

struct SystemFontResourceData {
	TArray<SystemFontFace> fonts;
	size_t binarySize = 0;
	void* fontBinary = nullptr;
};

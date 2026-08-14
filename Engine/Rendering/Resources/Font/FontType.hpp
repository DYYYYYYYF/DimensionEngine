#pragma once

#include "Containers/FString.hpp"
#include "Containers/TArray.hpp"

struct FFontGlyph {
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

struct FFontKerning {
	int   codePoint0;
	int   codePoint1;
	short amount;
};

struct FBitmapFontPage {
	char        id = INVALID_ID_U8;
	FString filename;
};

struct FBitmapFontResourceData {
	class UBitmapFont* data = nullptr;   // 改为指向 BitmapFont 自身
	FBitmapFontPage* Pages = nullptr;
	unsigned int      pageCount = 0;
};

struct FSystemFontFace {
	FString name;
};

struct FSystemFontResourceData {
	TArray<FSystemFontFace> fonts;
	size_t binarySize = 0;
	void* fontBinary = nullptr;
};

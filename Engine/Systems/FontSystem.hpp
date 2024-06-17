#pragma once

#include "Math/MathTypes.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Renderer/RendererTypes.hpp"

struct SystemFontConfig {
	char* name = nullptr;
	unsigned short defaultSize;
	char* resourceName = nullptr;
};

struct BitmapFontConfig {
	char* name = nullptr;
	unsigned short size;
	char* resourceName = nullptr;
};

struct FontSystemConfig {
	unsigned char defaultSystemFontCount;
	SystemFontConfig* systemFontConfigs = nullptr;
	unsigned char defaultBitmapFontCount;
	BitmapFontConfig* bitmapFontConfigs = nullptr;
	unsigned char maxSystemFontCount;
	unsigned char maxBitmapFontCount;
	bool autoRelease;
};

// Material system member
struct BitmapFontInternalData {
	Resource loadedResource;
	// Casted pointer to resource data for convenience.
	BitmapFontResourceData* resourceData = nullptr;
};

struct BitmapFontLookup {
	unsigned short id;
	unsigned short referenceCount;
	BitmapFontInternalData font;
};

class FontSystem {
public:
	static bool Initialize(IRenderer* renderer, FontSystemConfig* config);
	static void Shutdown();

	static bool LoadSystemFont(SystemFontConfig* config);
	static bool LoadBitmapFont(BitmapFontConfig* config);

	static bool Acquire(const char* fontName, unsigned short fontSize, class UIText* text);
	static bool Release(class UIText* text);

	static bool VerifyAtlas(struct FontData* data, const char* text);


private:
	static bool SetupFontData(FontData* font);
	static void CleanupFontData(FontData* font);

private:
	static FontSystemConfig Config;
	static HashTable BitmapLookup;
	static BitmapFontLookup* BitmapFonts;
	static void* BitmapHashTableBlock;

	static IRenderer* Renderer;
	static bool Initilized;

};
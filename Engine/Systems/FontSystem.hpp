#pragma once

#include "Math/MathTypes.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Renderer/RendererTypes.hpp"

struct BitmapFontLookup;
struct SystemFontLookup;

struct SystemFontConfig {
	const char* name = nullptr;
	unsigned short defaultSize;
	const char* resourceName = nullptr;
};

struct BitmapFontConfig {
	const char* name = nullptr;
	unsigned short size;
	const char* resourceName = nullptr;
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

	// System fonts.
	static bool CreateSystemFontVariant(SystemFontLookup* lookup, unsigned short size, const char* fontName, FontData* outVariant);
	static bool RebuildSystemFontVariantAtlas(SystemFontLookup* lookip, FontData* variant);
	static bool VerifySystemFontSizeVariant(SystemFontLookup* lookup, FontData* variant, const char* text);

private:
	static FontSystemConfig Config;
	static HashTable BitFontLookup;
	static HashTable SysFontLookup;
	static BitmapFontLookup* BitmapFonts;
	static SystemFontLookup* SystemFonts;
	static void* BitmapHashTableBlock;
	static void* SystemHashTableBlock;

	static IRenderer* Renderer;
	static bool Initilized;

};

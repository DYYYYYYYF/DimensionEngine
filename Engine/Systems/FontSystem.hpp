#pragma once

#include "Math/MathTypes.hpp"
#include "Framework/Classes/TextActor.h"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Rendering/RenderTypes.hpp"

#include <string>
#include <unordered_map>

struct BitmapFontLookup;
struct SystemFontLookup;

struct SystemFontConfig {
public:
	SystemFontConfig() {
		defaultSize = 0;
	}
	
	SystemFontConfig(const SystemFontConfig& s) {
		name = s.name;
		defaultSize = s.defaultSize;
		resourceName = s.resourceName;
	}

	FString name;
	FString resourceName;
	unsigned short defaultSize;
};

struct BitmapFontConfig {
public:
	BitmapFontConfig() {
		size = 0;
	}
	
	BitmapFontConfig(const BitmapFontConfig& b) {
		name = b.name;
		size = b.size;
		resourceName = b.resourceName;
	}

	FString name;
	FString resourceName;
	unsigned short size;
};

struct FontSystemConfig {
	unsigned char defaultSystemFontCount = 0;
	SystemFontConfig* systemFontConfigs = nullptr;
	unsigned char defaultBitmapFontCount = 0;
	BitmapFontConfig* bitmapFontConfigs = nullptr;
	unsigned char maxSystemFontCount = 10;
	unsigned char maxBitmapFontCount = 10;
	bool autoRelease = false;
};

class FontSystem {
public:
	static bool Initialize(IRenderer* renderer, const FontSystemConfig& config);
	static void Shutdown();

	static bool LoadSystemFont(SystemFontConfig* config);
	static bool LoadBitmapFont(BitmapFontConfig* config);

	static IFontDataBase* Acquire(const FString& fontName, UITextType type, int fontSize);
	static bool Release(IFontDataBase* text);

	static bool VerifyAtlas(IFontDataBase* data, const FString& text);


private:
	static bool SetupFontData(IFontDataBase* font);
	static void CleanupFontData(IFontDataBase* font);

	// System fonts.
	static SystemFontVariantData* CreateSystemFontVariant(SystemFontLookup* lookup, int size, const FString& fontName);
	static bool RebuildSystemFontVariantAtlas(SystemFontLookup* lookip, IFontDataBase* variant);
	static bool VerifySystemFontSizeVariant(SystemFontLookup* lookup, IFontDataBase* variant, const FString& text);

private:
	static bool Initilized;
	static IRenderer* Renderer;
	static FontSystemConfig Config;

	static std::vector<BitmapFontLookup*> BitmapFonts;
	static std::vector<SystemFontLookup*> SystemFonts;
	static std::unordered_map<FString, uint32_t> SystemFontMap;
	static std::unordered_map<FString, uint32_t> BitmapFontMap;

};

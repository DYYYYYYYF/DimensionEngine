#pragma once

#include "Math/MathTypes.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Renderer/RendererTypes.hpp"

#include <string>
#include <unordered_map>

class UIText;
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

	std::string name;
	std::string resourceName;
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

	std::string name;
	std::string resourceName;
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

	static bool Acquire(const std::string& fontName, unsigned short fontSize, class UIText* text);
	static bool Release(UIText* text);

	static bool VerifyAtlas(IFontDataBase* data, const std::string& text);


private:
	static bool SetupFontData(IFontDataBase* font);
	static void CleanupFontData(IFontDataBase* font);

	// System fonts.
	static SystemFontVariantData* CreateSystemFontVariant(SystemFontLookup* lookup, unsigned short size, const std::string& fontName);
	static bool RebuildSystemFontVariantAtlas(SystemFontLookup* lookip, IFontDataBase* variant);
	static bool VerifySystemFontSizeVariant(SystemFontLookup* lookup, IFontDataBase* variant, const std::string& text);

private:
	static bool Initilized;
	static IRenderer* Renderer;
	static FontSystemConfig Config;

	static std::vector<BitmapFontLookup*> BitmapFonts;
	static std::vector<SystemFontLookup*> SystemFonts;
	static std::unordered_map<std::string, uint32_t> SystemFontMap;
	static std::unordered_map<std::string, uint32_t> BitmapFontMap;

};

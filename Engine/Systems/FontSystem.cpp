#include "FontSystem.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TArray.hpp"
#include "Containers/TString.hpp"
#include "Containers/THashTable.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Systems/TextureSystem.h"
#include "Systems/ResourceSystem.h"

HashTable FontSystem::BitmapLookup;
FontSystemConfig FontSystem::Config;
BitmapFontLookup* FontSystem::BitmapFonts = nullptr;
void* FontSystem::BitmapHashTableBlock = nullptr;
IRenderer* FontSystem::Renderer = nullptr;
bool FontSystem::Initilized = false;

bool FontSystem::Initialize(IRenderer* renderer, FontSystemConfig* config){
	if (renderer == nullptr) {
		return false;
	}

	if (config->maxBitmapFontCount == 0 || config->maxSystemFontCount == 0) {
		LOG_FATAL("FontSystem::Initialize() config->maxBitmapFontCount and config->maxSystemFontCount must be > 0.");
		return false;
	}

	// Figure out how large of a hashtable is needed.
	// Block of memory will contain state structure then the block for the hashtable.
	BitmapHashTableBlock = Memory::Allocate(sizeof(unsigned short) * config->maxBitmapFontCount, MemoryType::eMemory_Type_Hashtable);
	BitmapFonts = (BitmapFontLookup*)Memory::Allocate(sizeof(BitmapFontLookup) * config->maxBitmapFontCount, MemoryType::eMemory_Type_Array);

	Config = *config;
	Renderer = renderer;

	BitmapLookup.Create(sizeof(unsigned short), config->maxBitmapFontCount, BitmapHashTableBlock, false);

	// Fill the table with invalid ids.
	uint32_t InvalidFillID = INVALID_ID_U16;
	if (!BitmapLookup.Fill(&InvalidFillID)) {
		LOG_ERROR("hashtable_fill failed.");
		return false;
	}

	for (uint32_t i = 0; i < config->maxBitmapFontCount; ++i) {
		BitmapFonts[i].id = INVALID_ID_U16;
		BitmapFonts[i].referenceCount = 0;
	}

	// Load up any default fonts.
	// Bitmap fonts.
	for (uint32_t i = 0; i < Config.defaultBitmapFontCount; ++i) {
		if (!LoadBitmapFont(&Config.bitmapFontConfigs[i])) {
			LOG_ERROR("Failed to load bitmap font: %s.", Config.bitmapFontConfigs[i].name);
		}
	}

	Initilized = true;
	return true;
}

void FontSystem::Shutdown() {
	if (Initilized) {
		// Clean up bitmap fonts.
		for (unsigned short i = 0; i < Config.maxBitmapFontCount; ++i) {
			if (BitmapFonts[i].id != INVALID_ID_U16) {
				FontData* Data = &BitmapFonts[i].font.resourceData->data;
				CleanupFontData(Data);
				Data = nullptr;
				BitmapFonts[i].id = INVALID_ID_U16;
			}
		}
	}
}

bool FontSystem::LoadSystemFont(SystemFontConfig* config){
	LOG_WARN("System fonts not supported yet.");
	return false;
}

bool FontSystem::LoadBitmapFont(BitmapFontConfig* config) {
	// Make sure a font with this name doesn't already exist.
	unsigned short ID = INVALID_ID_U16;
	if (!BitmapLookup.Get(config->name, &ID)) {
		LOG_ERROR("Hashtable lookup failed. Font will not be loaded.");
		return false;
	}

	if (ID != INVALID_ID_U16) {
		LOG_WARN("A font named '%s already exists and will not be loaded again.", config->name);
		return true;
	}

	// Get a new id.
	for (unsigned short i = 0; i < Config.maxBitmapFontCount; ++i) {
		if (BitmapFonts[i].id == INVALID_ID_U16) {
			ID = i;
			break;
		}
	}

	if (ID == INVALID_ID_U16) {
		LOG_ERROR("No space left to allocate a new bitmap font. Increase maximum number allowed in font system config.");
		return false;
	}

	// Obtain the lookup.
	BitmapFontLookup* Lookup = &BitmapFonts[ID];

	if (!ResourceSystem::Load(config->resourceName, ResourceType::eResource_Type_Bitmap_Font, nullptr, &Lookup->font.loadedResource)) {
		LOG_ERROR("Failed to load bitmap font.");
		return false;
	}

	// Keep a casted pointer to the resource data for convenience.
	Lookup->font.resourceData = (BitmapFontResourceData*)Lookup->font.loadedResource.Data;

	// Acquire the texture.
	// TODO: only accounts for one page at the moment.
	Lookup->font.resourceData->data.atlas.texture = TextureSystem::Acquire(Lookup->font.resourceData->Pages[0].file, true);

	bool Result = SetupFontData(&Lookup->font.resourceData->data);

	// Set the entry id here last before updating the hastable.
	if (!BitmapLookup.Set(config->name, &ID)) {
		LOG_ERROR("Hashtable set failed on bitmap font load.");
		return false;
	}

	Lookup->id = ID;
	return Result;
}

bool FontSystem::Acquire(const char* fontName, unsigned short fontSize, class UIText* text) {
	if (text->Type == UITextType::eUI_Text_Type_Bitmap) {
		unsigned short ID = INVALID_ID_U16;
		if (!BitmapLookup.Get(fontName, &ID)) {
			LOG_ERROR("Bitmap font lookup failed on acquire.");
			return false;
		}

		if (ID == INVALID_ID_U16) {
			LOG_ERROR("A bitmap font named '%s' was not found. Font acquisition failed.", fontName);
			return false;
		}

		// Get the lookup.
		BitmapFontLookup* Lookup = &BitmapFonts[ID];

		// Assign the data, increment the reference.
		text->Data = &Lookup->font.resourceData->data;
		Lookup->referenceCount++;

		return true;
	}
	else if (text->Type == UITextType::eUI_Text_Type_system) {
		LOG_WARN("System font not supported yet.");
		return false;
	}

	LOG_WARN("Unsupported font type.");
	return false;
}

bool FontSystem::Release(class UIText* text) {
	// TODO: Lookup font by name in appropriate hashtable.
	return true;
}

bool FontSystem::VerifyAtlas(struct FontData* font, const char* text) {
	if (font->type == FontType::eFont_Type_Bitmap) {
		// Bitmaps don't need verification since they are already generated.
		return true;
	}

	LOG_ERROR("FontSystem::VerifyAtlas() Failed: unknown font type.");
	return false;
}

bool FontSystem::SetupFontData(FontData* font) {
	// Create map resource.
	font->atlas.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	font->atlas.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	font->atlas.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
	if (!Renderer->AcquireTextureMap(&font->atlas)) {
		LOG_ERROR("Unable to acquire resource for font atlas texture map.");
		return false;
	}

	// Check for a tab glyph, as there may not always be one exported. If there is,
	// store its advanceX and just use it. If there is not, then create one based off spacex4.
	if (!font->tabXAdvance) {
		for (uint32_t i = 0; i < font->glyphCount; ++i) {
			if (font->glyphs[i].codePoint == '\t') {
				font->tabXAdvance = font->glyphs[i].advanceX;
				break;
			}
		}

		// If still not found, use space x 4.
		if (!font->tabXAdvance) {
			for (uint32_t i = 0; i < font->glyphCount; ++i) {
				// Search for space
				if (font->glyphs[i].codePoint == ' ') {
					font->tabXAdvance = (float)font->glyphs[i].advanceX * 4;
					break;
				}
			}

			if (!font->tabXAdvance) {
				// If still not, hard-code font size * 4.
				font->tabXAdvance = (float)font->size * 4;
			}
		}
	}

	return true;
}

void FontSystem::CleanupFontData(FontData* font) {
	// Relase the texture map resource.
	Renderer->ReleaseTextureMap(&font->atlas);

	// If a bitmap font, release the reference to the texture.
	if (font->type == FontType::eFont_Type_Bitmap && font->atlas.texture) {
		TextureSystem::Release(font->atlas.texture->Name);
	}
	font->atlas.texture = nullptr;
}

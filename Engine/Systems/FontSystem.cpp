#include "FontSystem.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Systems/TextureSystem.h"
#include "Systems/ResourceSystem.h"

// For system fonts.
#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include <stb_truetype.h>

// Material system member
struct BitmapFontLookup {
	unsigned short id;
	unsigned short referenceCount;
	BitmapFontInternalData font;
};

struct SystemFontLookup {
	unsigned short id;
	unsigned short referenceCount;
	std::vector<IFontDataBase*> sizeVariants;
	size_t binarySize;
	char* face = nullptr;
	void* fontBinary = nullptr;
	int offset;
	int index;
	struct stbtt_fontinfo info;
};

FontSystemConfig FontSystem::Config;
std::vector<BitmapFontLookup*> FontSystem::BitmapFonts;
std::vector<SystemFontLookup*> FontSystem::SystemFonts;
IRenderer* FontSystem::Renderer = nullptr;
bool FontSystem::Initilized = false;
std::unordered_map<std::string, uint32_t> FontSystem::SystemFontMap;
std::unordered_map<std::string, uint32_t> FontSystem::BitmapFontMap;

bool FontSystem::Initialize(IRenderer* renderer, FontSystemConfig* config){
	if (renderer == nullptr) {
		return false;
	}

	if (config->maxBitmapFontCount == 0 || config->maxSystemFontCount == 0) {
		GLOG(Log::eFatal, "FontSystem::Initialize() config->maxBitmapFontCount and config->maxSystemFontCount must be > 0.");
		return false;
	}

	Config = *config;
	Renderer = renderer;

	BitmapFonts.resize(config->maxBitmapFontCount);
	SystemFonts.resize(config->maxSystemFontCount);

	// Load up any default fonts.
	// Bitmap fonts.
	for (uint32_t i = 0; i < Config.defaultBitmapFontCount; ++i) {
		if (!LoadBitmapFont(&Config.bitmapFontConfigs[i])) {
			GLOG(Log::eError, "Failed to load bitmap font: %s.", Config.bitmapFontConfigs[i].name.c_str());
		}
	}

	// System fonts.
	for (uint32_t i = 0; i < Config.defaultSystemFontCount; ++i) {
		if (!LoadSystemFont(&Config.systemFontConfigs[i])) {
			GLOG(Log::eError, "Failed to load system font: %s.", Config.systemFontConfigs[i].name.c_str());
		}
	}

	Initilized = true;
	return true;
}

void FontSystem::Shutdown() {
	if (Initilized) {
		// Clean up bitmap fonts.
		for (unsigned short i = 0; i < Config.maxBitmapFontCount; ++i) {
			if (BitmapFonts[i] != nullptr) {
				CleanupFontData(BitmapFonts[i]->font.resourceData->data);
				BitmapFonts[i]->font.resourceData->data = nullptr;
				BitmapFonts[i]->id = INVALID_ID_U16;
				DeleteObject(BitmapFonts[i]);
			}
		}

		// Clean up system fonts.
		for (unsigned short i = 0; i < Config.maxSystemFontCount; ++i) {
			if (SystemFonts[i] != nullptr) {
				// Clean up each variant.
				uint32_t VariantCount = (uint32_t)SystemFonts[i]->sizeVariants.size();
				for (uint32_t j = 0; j < VariantCount; ++j) {
					CleanupFontData(SystemFonts[i]->sizeVariants[j]);
					SystemFonts[i]->sizeVariants[j] = nullptr;
				}

				SystemFonts[i]->id = INVALID_ID_U16;
				SystemFonts[i]->sizeVariants.clear();
			}
		}
	}
}

bool FontSystem::LoadSystemFont(SystemFontConfig* config){
	// For system fonts, they can actually contain multiple fonts. For this reason,
	// a copy of the resource's data will be held in each resulting variant, and the
	// resource will be released.
	Resource LoadedResource;
	if (!ResourceSystem::Load(config->resourceName, ResourceType::eResource_Type_System_Font, nullptr, &LoadedResource)) {
		GLOG(Log::eError, "Failed to load system font.");
		return false;
	}

	// Keep a casted pointer to the resource data for convenience.
	SystemFontResourceData* ResourceData = (SystemFontResourceData*)LoadedResource.Data;

	// Loop through the faces and create one lookup for each, as well as a default size variant for each lookup.
	uint32_t FontFaceCount = (uint32_t)ResourceData->fonts.size();
	for (uint32_t i = 0; i < FontFaceCount; ++i) {
		SystemFontFace* Face = &ResourceData->fonts[i];

		// Make sure a font with this name doesn't already exist.
		if (SystemFontMap.find(Face->name) != SystemFontMap.end()) {
			GLOG(Log::eWarn, "A font named '%s' already exists and will not be loaded again.", config->name.c_str());
			return true;
		}

		// Get a new id
		unsigned short ID = INVALID_ID_U16;
		for (unsigned short j = 0; j < Config.maxSystemFontCount; ++j) {
			if (SystemFonts[j] == nullptr) {
				ID = j;
				break;
			}
		}

		if (ID == INVALID_ID_U16) {
			GLOG(Log::eError, "No space left to allocate a new system font. Increase maximum number allowed in font system config.");
			return false;
		}

		// Obtain the lookup.
		SystemFontLookup* Lookup = NewObject<SystemFontLookup>();
		Lookup->binarySize = ResourceData->binarySize;
		Lookup->fontBinary = ResourceData->fontBinary;
		Lookup->face = StringCopy(Face->name.c_str());
		Lookup->index = i;

		// The offset
		Lookup->offset = stbtt_GetFontOffsetForIndex((unsigned char*)Lookup->fontBinary, i);
		int Result = stbtt_InitFont(&Lookup->info, (unsigned char*)Lookup->fontBinary, Lookup->offset);
		if (Result == 0) {
			// Zero indicates failure.
			GLOG(Log::eError, "Failed to init system font %s at index %i.", LoadedResource.FullPath.c_str(), i);
			return false;
		}

		// Create a default size variant.
		SystemFontVariantData* Variant = CreateSystemFontVariant(Lookup, config->defaultSize, Face->name.c_str());
		if (!Variant) {
			GLOG(Log::eError, "Failed to create variant: %s, index %i.", Face->name.c_str(), i);
			continue;
		}

		// Also perform setup for the variant.
		if (!SetupFontData(Variant)) {
			GLOG(Log::eError, "Failed to setup font data.");
			continue;
		}

		// Add to lookup's size variant.
		Lookup->sizeVariants.push_back(Variant);

		// Set the entry id here last before updating the hashtable.
		Lookup->id = ID;
		SystemFontMap[Face->name] = ID;
		SystemFonts[ID] = Lookup;
	}

	return true;
}

bool FontSystem::LoadBitmapFont(BitmapFontConfig* config) {
	// Make sure a font with this name doesn't already exist.
	if (BitmapFontMap.find(config->name) != BitmapFontMap.end()) {
		GLOG(Log::eWarn, "A font named '%s already exists and will not be loaded again.", config->name.c_str());
		return true;
	}

	// Get a new id.
	unsigned short ID = INVALID_ID_U16;
	for (unsigned short i = 0; i < Config.maxBitmapFontCount; ++i) {
		if (BitmapFonts[i] == nullptr) {
			ID = i;
			break;
		}
	}

	if (ID == INVALID_ID_U16) {
		GLOG(Log::eError, "No space left to allocate a new bitmap font. Increase maximum number allowed in font system config.");
		return false;
	}

	// Obtain the lookup.
	BitmapFontLookup* Lookup = NewObject<BitmapFontLookup>();

	if (!ResourceSystem::Load(config->resourceName, ResourceType::eResource_Type_Bitmap_Font, nullptr, &Lookup->font.loadedResource)) {
		GLOG(Log::eError, "Failed to load bitmap font.");
		return false;
	}

	// Keep a casted pointer to the resource data for convenience.
	Lookup->font.resourceData = (BitmapFontResourceData*)Lookup->font.loadedResource.Data;

	// Acquire the texture.
	// TODO: only accounts for one page at the moment.
	Lookup->font.resourceData->data->atlas.texture = TextureSystem::Acquire(Lookup->font.resourceData->Pages[0].file.c_str(), true);

	bool Result = SetupFontData(Lookup->font.resourceData->data);

	// Set the entry id here last before updating the hastable.
	Lookup->id = ID;
	BitmapFontMap[config->name] = ID;
	BitmapFonts[ID] = Lookup;

	return Result;
}

bool FontSystem::Acquire(const std::string& fontName, unsigned short fontSize, class UIText* text) {
	if (text->Type == UITextType::eUI_Text_Type_Bitmap) {
		if (BitmapFontMap.find(fontName) == BitmapFontMap.end()) {
			GLOG(Log::eError, "A bitmap font named '%s' was not found. Font acquisition failed.", fontName.c_str());
			return false;
		}

		// Get the lookup.
		unsigned short ID = BitmapFontMap[fontName];
		BitmapFontLookup* Lookup = BitmapFonts[ID];

		// Assign the data, increment the reference.
		text->Data = Lookup->font.resourceData->data;
		Lookup->referenceCount++;

		return true;
	}
	else if (text->Type == UITextType::eUI_Text_Type_system) {
		if (SystemFontMap.find(fontName) == SystemFontMap.end()) {
			GLOG(Log::eError, "A system font named '%s' was not found. Font acquisition failed.", fontName.c_str());
			return false;
		}

		// Get the lookup.
		unsigned short ID = SystemFontMap[fontName];
		SystemFontLookup* Lookup = SystemFonts[ID];

		// Search the size variants for the correct size.
		uint32_t Count = (uint32_t)Lookup->sizeVariants.size();
		for (uint32_t i = 0; i < Count; ++i) {
			if (Lookup->sizeVariants[i]->size == fontSize) {
				// Assign the data, increment the reference.
				text->Data = Lookup->sizeVariants[i];
				Lookup->referenceCount++;
				return true;
			}
		}

		// If we reach this point, the size variant doesn't exist. Create it.
		SystemFontVariantData* Variant = CreateSystemFontVariant(Lookup, fontSize, fontName);
		if (!Variant) {
			GLOG(Log::eError, "Failed to create variant: %s, index %i, size %i", Lookup->face, Lookup->index, fontSize);
			return false;
		}

		// Also perform setup for the variant.
		if (!SetupFontData(Variant)) {
			GLOG(Log::eError, "Failed to setup font data.");
		}

		// Add to the lookup's size variant.
		Lookup->sizeVariants.push_back(Variant);
		uint32_t Length = (uint32_t)Lookup->sizeVariants.size();
		// Assign the data, increment the reference.
		text->Data = Lookup->sizeVariants[Length - 1];
		Lookup->referenceCount++;
		SystemFonts[ID] = Lookup;
		return true;
	}

	GLOG(Log::eWarn, "Unsupported font type.");
	return false;
}

bool FontSystem::Release(UIText* text) {
	// TODO: Lookup font by name in appropriate hashtable.
	return true;
}

bool FontSystem::VerifyAtlas(IFontDataBase* font, const std::string& text) {
    if (font == nullptr || text.length() == 0){ return false;}
    
	if (font->type == FontType::eFont_Type_Bitmap) {
		// Bitmaps don't need verification since they are already generated.
		return true;
	} 
	else if (font->type == FontType::eFont_Type_System) {
		if (SystemFontMap.find(font->face) == SystemFontMap.end()){
			GLOG(Log::eError, "A system font named '%s' was not found. Font acquisition failed.", font->face.c_str());
			return false;
		}

		// Get the lookup.
		unsigned short ID = SystemFontMap[font->face];
		SystemFontLookup* Lookup = SystemFonts[ID];

		return VerifySystemFontSizeVariant(Lookup, font, text);
	}

	GLOG(Log::eError, "FontSystem::VerifyAtlas() Failed: unknown font type.");
	return false;
}

bool FontSystem::SetupFontData(IFontDataBase* font) {
	// Create map resource.
	font->atlas.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	font->atlas.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	font->atlas.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
	if (!Renderer->AcquireTextureMap(&font->atlas)) {
		GLOG(Log::eError, "Unable to acquire resource for font atlas texture map.");
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

void FontSystem::CleanupFontData(IFontDataBase* font) {
	// Relase the texture map resource.
	Renderer->ReleaseTextureMap(&font->atlas);

	// If a bitmap font, release the reference to the texture.
	if (font->type == FontType::eFont_Type_Bitmap && font->atlas.texture) {
		TextureSystem::Release(font->atlas.texture->GetName());
	}
	font->atlas.texture = nullptr;
}

SystemFontVariantData* FontSystem::CreateSystemFontVariant(SystemFontLookup* lookup, unsigned short size, const std::string& fontName) {
	SystemFontVariantData* InternalData = NewObject<SystemFontVariantData>();
	if (!InternalData) {
		return nullptr;
	}

	InternalData->atlasSizeX = 1024;	// TODO: Configurable
	InternalData->atlasSizeY = 1024;
	InternalData->size = size;
	InternalData->type = FontType::eFont_Type_System;
	InternalData->face = std::move(fontName);
	InternalData->internalDataSize = sizeof(SystemFontVariantData);

	// Push default codepoints (ascii 32-127) always, plus a -1 for unknown.
	InternalData->codepoints = std::vector<int>(96);
	InternalData->codepoints.push_back(-1);
	for (int i = 0; i < 95; ++i) {
		InternalData->codepoints.push_back(i + 32);
	}

	// Create textures.
	char FontTexName[255];
	StringFormat(FontTexName, 255, "__system_text_atlas_%s_i%i_sz%i__", fontName.c_str(), lookup->index, size);
	InternalData->atlas.texture = TextureSystem::AcquireWriteable(FontTexName, InternalData->atlasSizeX, InternalData->atlasSizeY, 4, true);

	// Obtain some metrics.
	InternalData->scale = stbtt_ScaleForPixelHeight(&lookup->info, (float)size);
	int Ascent, Descent, Linegap;
	stbtt_GetFontVMetrics(&lookup->info, &Ascent, &Descent, &Linegap);
	InternalData->lineHeight = static_cast<int>((Ascent - Descent + Linegap) * InternalData->scale);

	if (!RebuildSystemFontVariantAtlas(lookup, InternalData)) {
		return nullptr;
	}

	return InternalData;
}

bool FontSystem::RebuildSystemFontVariantAtlas(SystemFontLookup* lookip, IFontDataBase* variant) {
	SystemFontVariantData* InternalData = (SystemFontVariantData*)variant;

	uint32_t PackImageSize = variant->atlasSizeX * variant->atlasSizeY * sizeof(unsigned char);
	unsigned char* Pixels = (unsigned char* )Memory::Allocate(PackImageSize, MemoryType::eMemory_Type_Array);
	uint32_t CodepointCount = (uint32_t)InternalData->codepoints.size();
	stbtt_packedchar* PackedChars = (stbtt_packedchar*)Memory::Allocate(sizeof(stbtt_packedchar) * CodepointCount, MemoryType::eMemory_Type_Array);
	
	// Begin packing all known characters into the atlas. This creates a single-channel image
	// with rendered glyphs at the given size.
	stbtt_pack_context Context;
	if (!stbtt_PackBegin(&Context, Pixels, variant->atlasSizeX, variant->atlasSizeY, 0, 1, 0)) {
        GLOG(Log::eError, "stbtt_PackBegin() Failed.");
		return false;
	}

	// Fit all codepoints into a single range for packing.
	stbtt_pack_range Range;
	Range.first_unicode_codepoint_in_range = 0;
	Range.font_size = (float)variant->size;
	Range.num_chars = CodepointCount;
	Range.chardata_for_range = PackedChars;
	Range.array_of_unicode_codepoints = InternalData->codepoints.data();
	if (!stbtt_PackFontRanges(&Context, (unsigned char*)lookip->fontBinary, lookip->index, &Range, 1)) {
        stbtt_PackEnd(&Context);
        GLOG(Log::eError, "stbtt_PackFontRanges() Failed.");
		return false;
	}

	stbtt_PackEnd(&Context);

	// Convert from single-channel to RGBA, or pack_image_size * 4.
	unsigned char* RGBAPixels = (unsigned char*)Memory::Allocate(PackImageSize * 4, eMemory_Type_Array);
	for (uint32_t j = 0; j < PackImageSize; j++) {
		RGBAPixels[(j * 4) + 0] = Pixels[j];
		RGBAPixels[(j * 4) + 1] = Pixels[j];
		RGBAPixels[(j * 4) + 2] = Pixels[j];
		RGBAPixels[(j * 4) + 3] = Pixels[j];
	}

	// Write texture data to atlas.
	TextureSystem::WriteData(variant->atlas.texture, 0, PackImageSize * 4, RGBAPixels);

	// Free pixel/rgba pixel data.
	Memory::Free(Pixels, PackImageSize, MemoryType::eMemory_Type_Array);
	Pixels = nullptr;
	Memory::Free(RGBAPixels, PackImageSize * 4, MemoryType::eMemory_Type_Array);
	RGBAPixels = nullptr;

	// Regenerate glyphs
	if (variant->glyphs && variant->glyphCount) {
		Memory::Free(variant->glyphs, sizeof(FontGlyph) * variant->glyphCount, MemoryType::eMemory_Type_Array);
		variant->glyphs = nullptr;
	}

	variant->glyphCount = CodepointCount;
	variant->glyphs = (FontGlyph*)Memory::Allocate(sizeof(FontGlyph) * variant->glyphCount, MemoryType::eMemory_Type_Array);
	for (unsigned short i = 0; i < variant->glyphCount; ++i) {
		stbtt_packedchar* pc = &PackedChars[i];
		FontGlyph* g = &variant->glyphs[i];
		g->codePoint = InternalData->codepoints[i];
		g->pageID = 0;
		g->offsetX = (short)pc->xoff;
		g->offsetY = (short)pc->yoff;
		g->x = pc->x0;
		g->y = pc->y0;
		g->width = pc->x1 - pc->x0;
		g->height = pc->y1 - pc->y0;
		g->advanceX = (short)pc->xadvance;
	}

	// Regenerate kernings.
	if (variant->kerningCount && variant->kernings) {
		Memory::Free(variant->kernings, sizeof(FontKerning) * variant->kerningCount, MemoryType::eMemory_Type_Array);
		variant->kernings = nullptr;
	}

	variant->kerningCount = stbtt_GetKerningTableLength(&lookip->info);
	if (variant->kerningCount) {
		variant->kernings = (FontKerning*)Memory::Allocate(sizeof(FontKerning) * variant->kerningCount, MemoryType::eMemory_Type_Array);
		// Get the kerning table for the current font.
		stbtt_kerningentry* KerningTable = (stbtt_kerningentry*)Memory::Allocate(sizeof(stbtt_kerningentry) * variant->kerningCount, MemoryType::eMemory_Type_Array);
		int EntryCount = stbtt_GetKerningTable(&lookip->info, KerningTable, variant->kerningCount);
		if (EntryCount != variant->kerningCount) {
			GLOG(Log::eError, "Kerning entry count mismatch: %i -> %i.", EntryCount, variant->kerningCount);
			return false;
		}

		for (uint32_t i = 0; i < variant->kerningCount; ++i) {
			FontKerning* k = &variant->kernings[i];
			k->codePoint0 = KerningTable[i].glyph1;
			k->codePoint1 = KerningTable[i].glyph2;
			k->amount = KerningTable[i].advance;
		}
	}
	else {
		variant->kernings = nullptr;
	}

	return true;
}

bool FontSystem::VerifySystemFontSizeVariant(SystemFontLookup* lookup, IFontDataBase* variant, const std::string& text) {
	SystemFontVariantData* InternalData = (SystemFontVariantData*)variant;

	uint32_t CharLength = (uint32_t)text.length();
	uint32_t AddedCodepointCount = 0;
	for (uint32_t i = 0; i < CharLength;) {
		int Codepoint;
		unsigned char Advance;
		if (!StringBytesToCodepoint(text.c_str(), i, &Codepoint, &Advance)) {
			GLOG(Log::eError, "BytesToCodepoint() Failed to get codepoint.");
			++i;
			continue;
		}
		else {
			i += Advance;
			if (Codepoint < 128) {
				continue;
			}

			uint32_t CodepointCount = (uint32_t)InternalData->codepoints.size();
			bool Found = false;
			for (uint32_t j = 95; j < CodepointCount; j++) {
				if (InternalData->codepoints[j] == Codepoint) {
					Found = true;
					break;
				}
			}

			if (!Found) {
				InternalData->codepoints.push_back(Codepoint);
				AddedCodepointCount++;
			}
		}
	}

	// If codepoints were added, rebuild the atlas.
	if (AddedCodepointCount > 0) {
		return RebuildSystemFontVariantAtlas(lookup, variant);
	}

	// Otherwise, proceed as normal.
	return true;
}

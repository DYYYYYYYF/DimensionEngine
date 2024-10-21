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

// For system fonts.
#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include <stb_truetype.h>

// Material system member
struct BitmapFontInternalData {
	Resource loadedResource;
	// Casted pointer to resource data for convenience.
	BitmapFontResourceData* resourceData = nullptr;
};

struct SystemFontVariantData {
	TArray<int> codepoints;
	float scale;
};

struct BitmapFontLookup {
	unsigned short id;
	unsigned short referenceCount;
	BitmapFontInternalData font;
};

struct SystemFontLookup {
	unsigned short id;
	unsigned short referenceCount;
	TArray<FontData> sizeVariants;
	size_t binarySize;
	char* face = nullptr;
	void* fontBinary = nullptr;
	int offset;
	int index;
	struct stbtt_fontinfo info;
};

HashTable FontSystem::BitFontLookup;
HashTable FontSystem::SysFontLookup;
FontSystemConfig FontSystem::Config;
BitmapFontLookup* FontSystem::BitmapFonts = nullptr;
SystemFontLookup* FontSystem::SystemFonts = nullptr;
void* FontSystem::BitmapHashTableBlock = nullptr;
void* FontSystem::SystemHashTableBlock = nullptr;
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

	SystemHashTableBlock = Memory::Allocate(sizeof(unsigned short) * config->maxSystemFontCount, MemoryType::eMemory_Type_Hashtable);
	SystemFonts = (SystemFontLookup*)Memory::Allocate(sizeof(SystemFontLookup) * config->maxSystemFontCount, MemoryType::eMemory_Type_Array);

	Config = *config;
	Renderer = renderer;

	BitFontLookup.Create(sizeof(unsigned short), config->maxBitmapFontCount, BitmapHashTableBlock, false);
	SysFontLookup.Create(sizeof(unsigned short), config->maxSystemFontCount, SystemHashTableBlock, false);

	// Fill the table with invalid ids.
	uint32_t InvalidFillID = INVALID_ID_U16;
	if (!BitFontLookup.Fill(&InvalidFillID) || !SysFontLookup.Fill(&InvalidFillID)) {
		LOG_ERROR("hashtable_fill failed.");
		return false;
	}

	for (uint32_t i = 0; i < config->maxBitmapFontCount; ++i) {
		BitmapFonts[i].id = INVALID_ID_U16;
		BitmapFonts[i].referenceCount = 0;
	}

	for (uint32_t i = 0; i < config->maxSystemFontCount; ++i) {
		SystemFonts[i].id = INVALID_ID_U16;
		SystemFonts[i].referenceCount = 0;
	}

	// Load up any default fonts.
	// Bitmap fonts.
	for (uint32_t i = 0; i < Config.defaultBitmapFontCount; ++i) {
		if (!LoadBitmapFont(&Config.bitmapFontConfigs[i])) {
			LOG_ERROR("Failed to load bitmap font: %s.", Config.bitmapFontConfigs[i].name);
		}
	}

	// System fonts.
	for (uint32_t i = 0; i < Config.defaultSystemFontCount; ++i) {
		if (!LoadSystemFont(&Config.systemFontConfigs[i])) {
			LOG_ERROR("Failed to load system font: %s.", Config.systemFontConfigs[i].name);
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

		// Clean up system fonts.
		for (unsigned short i = 0; i < Config.maxSystemFontCount; ++i) {
			if (SystemFonts[i].id != INVALID_ID_U16) {
				// Clean up each variant.
				uint32_t VariantCount = (uint32_t)SystemFonts[i].sizeVariants.Size();
				for (uint32_t j = 0; j < VariantCount; ++j) {
					FontData* Data = &SystemFonts[i].sizeVariants[j];
					CleanupFontData(Data);
					Data = nullptr;
				}

				SystemFonts[i].id = INVALID_ID_U16;
				SystemFonts[i].sizeVariants.Clear();
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
		LOG_ERROR("Failed to load system font.");
		return false;
	}

	// Keep a casted pointer to the resource data for convenience.
	SystemFontResourceData* ResourceData = (SystemFontResourceData*)LoadedResource.Data;

	// Loop through the faces and create one lookup for each, as well as a default size variant for each lookup.
	uint32_t FontFaceCount = (uint32_t)ResourceData->fonts.Size();
	for (uint32_t i = 0; i < FontFaceCount; ++i) {
		SystemFontFace* Face = &ResourceData->fonts[i];

		// Make sure a font with this name doesn't already exist.
		unsigned short ID = INVALID_ID_U16;
		if (!SysFontLookup.Get(Face->name, &ID)) {
			LOG_ERROR("Hashtable lookup failed. font will not be loaded.");
			return false;
		}

		if (ID != INVALID_ID_U16) {
			LOG_WARN("A font named '%s' already exists and will not be loaded again.", config->name);
			return true;
		}

		// Get a new id
		for (unsigned short j = 0; j < Config.maxSystemFontCount; ++j) {
			if (SystemFonts[j].id == INVALID_ID_U16) {
				ID = j;
				break;
			}
		}

		if (ID == INVALID_ID_U16) {
			LOG_ERROR("No space left to allocate a new system font. Increase maximum number allowed in font system config.");
			return false;
		}

		// Obtain the lookup.
		SystemFontLookup* Lookup = &SystemFonts[ID];
		Lookup->binarySize = ResourceData->binarySize;
		Lookup->fontBinary = ResourceData->fontBinary;
		Lookup->face = StringCopy(Face->name);
		Lookup->index = i;
		Lookup->sizeVariants = TArray<FontData>();

		// The offset
		Lookup->offset = stbtt_GetFontOffsetForIndex((unsigned char*)Lookup->fontBinary, i);
		int Result = stbtt_InitFont(&Lookup->info, (unsigned char*)Lookup->fontBinary, Lookup->offset);
		if (Result == 0) {
			// Zero indicates failure.
			LOG_ERROR("Failed to init system font %s at index %i.", LoadedResource.FullPath, i);
			return false;
		}

		// Create a default size variant.
		FontData Variant;
		if (!CreateSystemFontVariant(Lookup, config->defaultSize, Face->name, &Variant)) {
			LOG_ERROR("Failed to create variant: %s, index %i.", Face->name, i);
			continue;
		}

		// Also perform setup for the variant.
		if (!SetupFontData(&Variant)) {
			LOG_ERROR("Failed to setup font data.");
			continue;
		}

		// Add to lookup's size variant.
		Lookup->sizeVariants.Push(Variant);

		// Set the entry id here last before updating the hashtable.
		Lookup->id = ID;
		if (!SysFontLookup.Set(Face->name, &ID)) {
			LOG_ERROR("Hashtable failed to set on font load.");
			return false;
		}
	}

	return true;
}

bool FontSystem::LoadBitmapFont(BitmapFontConfig* config) {
	// Make sure a font with this name doesn't already exist.
	unsigned short ID = INVALID_ID_U16;
	if (!BitFontLookup.Get(config->name, &ID)) {
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
	if (!BitFontLookup.Set(config->name, &ID)) {
		LOG_ERROR("Hashtable set failed on bitmap font load.");
		return false;
	}

	Lookup->id = ID;
	return Result;
}

bool FontSystem::Acquire(const char* fontName, unsigned short fontSize, class UIText* text) {
	if (text->Type == UITextType::eUI_Text_Type_Bitmap) {
		unsigned short ID = INVALID_ID_U16;
		if (!BitFontLookup.Get(fontName, &ID)) {
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
		unsigned short ID = INVALID_ID_U16;
		if (!SysFontLookup.Get(fontName, &ID)) {
			LOG_ERROR("System font lookup failed on acquire.");
			return false;
		}

		if (ID == INVALID_ID_U16) {
			LOG_ERROR("A system font named '%s' was not found. Font acquisition failed.", fontName);
			return false;
		}

		// Get the lookup.
		SystemFontLookup* Lookup = &SystemFonts[ID];

		// Search the size variants for the correct size.
		uint32_t Count = (uint32_t)Lookup->sizeVariants.Size();
		for (uint32_t i = 0; i < Count; ++i) {
			if (Lookup->sizeVariants[i].size == fontSize) {
				// Assign the data, increment the reference.
				text->Data = &Lookup->sizeVariants[i];
				Lookup->referenceCount++;
				return true;
			}
		}

		// If we reach this point, the size variant doesn't exist. Create it.
		FontData Variant;
		if (!CreateSystemFontVariant(Lookup, fontSize, fontName, &Variant)) {
			LOG_ERROR("Failed to create variant: %s, index %i, size %i", Lookup->face, Lookup->index, fontSize);
			return false;
		}

		// Also perform setup for the variant.
		if (!SetupFontData(&Variant)) {
			LOG_ERROR("Failed to setup font data.");
		}

		// Add to the lookup's size variant.
		Lookup->sizeVariants.Push(Variant);
		uint32_t Length = (uint32_t)Lookup->sizeVariants.Size();
		// Assign the data, increment the reference.
		text->Data = &Lookup->sizeVariants[Length - 1];
		Lookup->referenceCount++;
		return true;
	}

	LOG_WARN("Unsupported font type.");
	return false;
}

bool FontSystem::Release(class UIText* text) {
	// TODO: Lookup font by name in appropriate hashtable.
	return true;
}

bool FontSystem::VerifyAtlas(struct FontData* font, const char* text) {
    if (font == nullptr || text == nullptr){ return false;}
    
	if (font->type == FontType::eFont_Type_Bitmap) {
		// Bitmaps don't need verification since they are already generated.
		return true;
	} 
	else if (font->type == FontType::eFont_Type_System) {
		unsigned short ID = INVALID_ID_U16;
		if (!SysFontLookup.Get(font->face, &ID)) {
			LOG_ERROR("System font lookup failed on acquire.");
			return false;
		}

		if (ID == INVALID_ID_U16) {
			LOG_ERROR("A system font named '%s' was not found. Font acquisition failed.", font->face);
			return false;
		}

		// Get the lookup.
		SystemFontLookup* Lookup = &SystemFonts[ID];

		return VerifySystemFontSizeVariant(Lookup, font, text);
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

bool FontSystem::CreateSystemFontVariant(SystemFontLookup* lookup, unsigned short size, const char* fontName, FontData* outVariant) {
	Memory::Zero(outVariant, sizeof(FontData));
	outVariant->atlasSizeX = 1024;	// TODO: Configurable
	outVariant->atlasSizeY = 1024;
	outVariant->size = size;
	outVariant->type = FontType::eFont_Type_System;
	strncpy(outVariant->face, fontName, 255);
	outVariant->internalDataSize = sizeof(SystemFontVariantData);
	outVariant->internalData = Memory::Allocate(outVariant->internalDataSize, MemoryType::eMemory_Type_System_Font);

	SystemFontVariantData* InternalData = (SystemFontVariantData*)outVariant->internalData;

	// Push default codepoints (ascii 32-127) always, plus a -1 for unknown.
	InternalData->codepoints = TArray<int>(96);
	InternalData->codepoints.Push(-1);
	for (int i = 0; i < 95; ++i) {
		InternalData->codepoints.Push(i + 32);
	}

	// Create textures.
	char FontTexName[255];
	StringFormat(FontTexName, 255, "__system_text_atlas_%s_i%i_sz%i__", fontName, lookup->index, size);
	outVariant->atlas.texture = TextureSystem::AcquireWriteable(FontTexName, outVariant->atlasSizeX, outVariant->atlasSizeY, 4, true);

	// Obtain some metrics.
	InternalData->scale = stbtt_ScaleForPixelHeight(&lookup->info, (float)size);
	int Ascent, Descent, Linegap;
	stbtt_GetFontVMetrics(&lookup->info, &Ascent, &Descent, &Linegap);
	outVariant->lineHeight = static_cast<int>((Ascent - Descent + Linegap) * InternalData->scale);

	return RebuildSystemFontVariantAtlas(lookup, outVariant);
}

bool FontSystem::RebuildSystemFontVariantAtlas(SystemFontLookup* lookip, FontData* variant) {
	SystemFontVariantData* InternalData = (SystemFontVariantData*)variant->internalData;

	uint32_t PackImageSize = variant->atlasSizeX * variant->atlasSizeY * sizeof(unsigned char);
	unsigned char* Pixels = (unsigned char* )Memory::Allocate(PackImageSize, MemoryType::eMemory_Type_Array);
	uint32_t CodepointCount = (uint32_t)InternalData->codepoints.Size();
	stbtt_packedchar* PackedChars = (stbtt_packedchar*)Memory::Allocate(sizeof(stbtt_packedchar) * CodepointCount, MemoryType::eMemory_Type_Array);
	
	// Begin packing all known characters into the atlas. This creates a single-channel image
	// with rendered glyphs at the given size.
	stbtt_pack_context Context;
	if (!stbtt_PackBegin(&Context, Pixels, variant->atlasSizeX, variant->atlasSizeY, 0, 1, 0)) {
		LOG_ERROR("stbtt_PackBegin() Failed.");
		return false;
	}

	// Fit all codepoints into a single range for packing.
	stbtt_pack_range Range;
	Range.first_unicode_codepoint_in_range = 0;
	Range.font_size = (float)variant->size;
	Range.num_chars = CodepointCount;
	Range.chardata_for_range = PackedChars;
	Range.array_of_unicode_codepoints = InternalData->codepoints.Data();
	if (!stbtt_PackFontRanges(&Context, (unsigned char*)lookip->fontBinary, lookip->index, &Range, 1)) {
		LOG_ERROR("stbtt_PackFontRanges() Failed.");
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
	Memory::Free(RGBAPixels, PackImageSize * 4, MemoryType::eMemory_Type_Array);

	// Regenerate glyphs
	if (variant->glyphs && variant->glyphCount) {
		Memory::Free(variant->glyphs, sizeof(FontGlyph) * variant->glyphCount, MemoryType::eMemory_Type_Array);
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
	}

	variant->kerningCount = stbtt_GetKerningTableLength(&lookip->info);
	if (variant->kerningCount) {
		variant->kernings = (FontKerning*)Memory::Allocate(sizeof(FontKerning) * variant->kerningCount, MemoryType::eMemory_Type_Array);
		// Get the kerning table for the current font.
		stbtt_kerningentry* KerningTable = (stbtt_kerningentry*)Memory::Allocate(sizeof(stbtt_kerningentry) * variant->kerningCount, MemoryType::eMemory_Type_Array);
		int EntryCount = stbtt_GetKerningTable(&lookip->info, KerningTable, variant->kerningCount);
		if (EntryCount != variant->kerningCount) {
			LOG_ERROR("Kerning entry count mismatch: %i -> %i.", EntryCount, variant->kerningCount);
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

bool FontSystem::VerifySystemFontSizeVariant(SystemFontLookup* lookup, FontData* variant, const char* text) {
	SystemFontVariantData* InternalData = (SystemFontVariantData*)variant->internalData;

	uint32_t CharLength = (uint32_t)strlen(text);
	uint32_t AddedCodepointCount = 0;
	for (uint32_t i = 0; i < CharLength;) {
		int Codepoint;
		unsigned char Advance;
		if (!StringBytesToCodepoint(text, i, &Codepoint, &Advance)) {
			LOG_ERROR("BytesToCodepoint() Failed to get codepoint.");
			++i;
			continue;
		}
		else {
			i += Advance;
			if (Codepoint < 128) {
				continue;
			}

			uint32_t CodepointCount = (uint32_t)InternalData->codepoints.Size();
			bool Found = false;
			for (uint32_t j = 95; j < CodepointCount; j++) {
				if (InternalData->codepoints[j] == Codepoint) {
					Found = true;
					break;
				}
			}

			if (!Found) {
				InternalData->codepoints.Push(Codepoint);
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

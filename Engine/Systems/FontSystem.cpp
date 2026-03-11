#include "FontSystem.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Rendering/Renderer.hpp"
#include "Systems/TextureSystem.h"
#include "Systems/ResourceSystem.h"

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include <stb_truetype.h>

// ─────────────────────────────────────────────
//  静态成员定义
// ─────────────────────────────────────────────

struct SystemFontContext {
	void* fontBinary = nullptr;
	size_t         binarySize = 0;
	int            offset = 0;
	int            index = 0;
	stbtt_fontinfo info = {};
};

// ─────────────────────────────────────────────
//  FontSystem
// ─────────────────────────────────────────────

FontSystem& FontSystem::Get() {
	static FontSystem instance;
	return instance;
}

bool FontSystem::Initialize(IRenderer* renderer, const FontSystemConfig& config) {
	if (!renderer) {
		GLOG(Log::eFatal, "FontSystem::Initialize() renderer must not be null.");
		return false;
	}

	if (config.maxBitmapFontCount == 0 || config.maxSystemFontCount == 0) {
		GLOG(Log::eFatal, "FontSystem::Initialize() maxBitmapFontCount and maxSystemFontCount must be > 0.");
		return false;
	}

	Config = config;
	Renderer = renderer;

	// 加载默认 Bitmap 字体
	for (uint32_t i = 0; i < Config.defaultBitmapFontCount; ++i) {
		if (!RegisterBitmapFont(Config.bitmapFontConfigs[i])) {
			GLOG(Log::eError, "Failed to register bitmap font: %s.", Config.bitmapFontConfigs[i].name.CStr());
		}
	}

	// 加载默认 System 字体
	for (uint32_t i = 0; i < Config.defaultSystemFontCount; ++i) {
		if (!RegisterSystemFont(Config.systemFontConfigs[i])) {
			GLOG(Log::eError, "Failed to register system font: %s.", Config.systemFontConfigs[i].name.CStr());
		}
	}

	Initialized = true;
	return true;
}

void FontSystem::Shutdown() {
	if (!Initialized) { return; }

	// 释放 GPU 资源
	for (auto& [name, font] : BitmapFonts) {
		if (font) font->ReleaseResource(Renderer);
	}
	BitmapFonts.Clear();

	for (auto& [name, font] : SystemFonts) {
		if(font) font->ReleaseResource(Renderer);
	}
	SystemFonts.Clear();

	Initialized = false;
}

// ─────────────────────────────────────────────

bool FontSystem::RegisterBitmapFont(const BitmapFontConfig& config) {
	if (BitmapFonts.Contains(config.name)) {
		GLOG(Log::eWarn, "Bitmap font '%s' already registered.", config.name.CStr());
		return true;
	}

	if (BitmapFonts.Size() >= Config.maxBitmapFontCount) {
		GLOG(Log::eError, "Bitmap font limit reached (%u). Increase maxBitmapFontCount.", Config.maxBitmapFontCount);
		return false;
	}

	// 从资源系统加载原始数据
	// BitmapFont 继承自 UAsset，直接作为加载目标
	BitmapFont* font = NewObject<BitmapFont>();
	if (!ResourceSystem::Load(config.resourceName, EAssetType::BitmapFont, nullptr, font)) {
		GLOG(Log::eError, "Failed to load bitmap font resource: %s.", config.resourceName.CStr());
		return false;
	}

	// 从 UAsset::Data 中取出 ResourceData，完成 GPU 侧初始化
	BitmapFontResourceData* resourceData = static_cast<BitmapFontResourceData*>(font->Data);
	if (!font->InitFromResourceData(resourceData, Renderer)) {
		GLOG(Log::eError, "Failed to initialize bitmap font: %s.", config.name.CStr());
		return false;
	}

	BitmapFonts[config.name] = font;
	return true;
}

bool FontSystem::RegisterSystemFont(const SystemFontConfig& config) {
	// 先加载资源，一个 TTF 文件可能包含多个字型（face）
	UAsset loadedAsset;
	if (!ResourceSystem::Load(config.resourceName.CStr(), EAssetType::SystemFont, nullptr, &loadedAsset)) {
		GLOG(Log::eError, "Failed to load system font resource: %s.", config.resourceName.CStr());
		return false;
	}

	SystemFontResourceData* resourceData = static_cast<SystemFontResourceData*>(loadedAsset.Data);
	uint32_t faceCount = static_cast<uint32_t>(resourceData->fonts.Size());

	for (uint32_t i = 0; i < faceCount; ++i) {
		const FString& faceName = resourceData->fonts[i].name;

		if (SystemFonts.Contains(faceName)) {
			GLOG(Log::eWarn, "System font '%s' already registered.", faceName.CStr());
			continue;
		}

		if (SystemFonts.Size() >= Config.maxSystemFontCount) {
			GLOG(Log::eError, "System font limit reached (%u). Increase maxSystemFontCount.", Config.maxSystemFontCount);
			return false;
		}

		SystemFont* font = NewObject<SystemFont>();

		// 传入 index=i，SystemFont 内部用它定位 TTF 文件内的具体字型
		if (!font->InitFromResourceData(resourceData, i, Renderer)) {
			GLOG(Log::eError, "Failed to initialize system font '%s' at index %u.", faceName.CStr(), i);
			continue;
		}

		// 预创建 defaultSize 对应的 Variant，避免首次 Acquire 时卡顿
		if (config.defaultSize > 0) {
			if (!font->AcquireVariant(config.defaultSize, Renderer)) {
				GLOG(Log::eError, "Failed to create default size variant for '%s'.", faceName.CStr());
			}
		}

		SystemFonts[faceName] = font;
	}

	return true;
}

// ─────────────────────────────────────────────

IFont* FontSystem::Acquire(const FString& fontName, UITextType type, int fontSize) {
	if (type == UITextType::eUI_Text_Type_Bitmap) {
		if (!BitmapFonts.Contains(fontName)) {
			GLOG(Log::eError, "Bitmap font '%s' not found.", fontName.CStr());
			return nullptr;
		}
		
		// BitmapFont 自身实现 IFont，直接返回
		IFont* Font = BitmapFonts[fontName];
		if (!Font) {
			return nullptr;
		}

		Font->AddRef();
		return Font;
	}

	if (type == UITextType::eUI_Text_Type_system) {
		if (!SystemFonts.Contains(fontName)) {
			GLOG(Log::eError, "System font '%s' not found.", fontName.CStr());
			return nullptr;
		}
		// 委托给 SystemFont，由它负责查找或创建对应 size 的 Variant
		SystemFont* Font = SystemFonts[fontName];
		if (!Font) {
			return nullptr;
		}

		SystemFontVariant* Variant = Font->AcquireVariant(fontSize, Renderer);
		if (!Variant) {
			return nullptr;
		}

		Variant->AddRef();
		return Variant;
	}

	GLOG(Log::eWarn, "FontSystem::Acquire() unsupported font type.");
	return nullptr;
}

bool FontSystem::Release(IFont* font) {
	if (!font) { return false; }

	if (font->Release()) {
		// 引用计数归零，根据 autoRelease 决定是否卸载
		if (Config.autoRelease) {
			// 需要找到这个 font 属于哪张注册表并移除
			// BitmapFont 直接在 BitmapFonts 里找
			for (auto& [name, bitmapFont] : BitmapFonts) {
				if (bitmapFont == font) {
					bitmapFont->ReleaseResource(Renderer);
					BitmapFonts.Remove(name);
					return true;
				}
			}

			// SystemFontVariant 需要通过 face + size 在 SystemFonts 里找
			// 可以通过 font->GetFace() 和 font->GetSize() 定位
			FString FaceName = font->GetFace();
			uint32_t FontSize = font->GetSize();
			if (SystemFonts.Contains(FaceName)) {
				SystemFonts[FaceName]->ReleaseVariant(FontSize, Renderer);
			}
		}
	}

	return true;
}

bool FontSystem::VerifyAtlas(IFont* font, const FString& text) {
	if (!font) { return false; }
	// FontSystem 不再需要感知字体类型，直接委托
	return font->VerifyAtlas(text);
}

// ═════════════════════════════════════════════
//  BitmapFont 实现
// ═════════════════════════════════════════════

bool BitmapFont::InitFromResourceData(BitmapFontResourceData* resourceData, IRenderer* renderer) {
	if (!resourceData) { return false; }

	resourceData_ = resourceData;

	// resourceData->data 原来指向 IFontDataBase，现在指向 BitmapFont 自身
	// 在 ResourceSystem 加载完成后由外部赋值，或在此处直接读取字段
	// 此处假设 ResourceSystem 已将 glyph/kerning 数据填入 resourceData
	BitmapFont* src = resourceData->data;
	if (src) {
		face_ = src->face_;
		size_ = src->size_;
		lineHeight_ = src->lineHeight_;
		baseLine_ = src->baseLine_;
		atlasSizeX_ = src->atlasSizeX_;
		atlasSizeY_ = src->atlasSizeY_;
		glyphs_ = src->glyphs_;
		glyphCount_ = src->glyphCount_;
		kernings_ = src->kernings_;
		kerningCount_ = src->kerningCount_;
	}

	// 获取 atlas 纹理（目前只处理单 page）
	if (resourceData->pageCount > 0) {
		atlas_.texture = TextureSystem::Acquire(resourceData->Pages[0].filename, true);
	}

	// 建立 GPU 侧 texture map
	atlas_.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	atlas_.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	atlas_.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
	if (!renderer->AcquireTextureMap(&atlas_)) {
		GLOG(Log::eError, "BitmapFont: failed to acquire texture map.");
		return false;
	}

	ComputeTabXAdvance();
	return true;
}

void BitmapFont::ReleaseResource(IRenderer* renderer) {
	renderer->ReleaseTextureMap(&atlas_);
	if (atlas_.texture) {
		TextureSystem::Release(atlas_.texture->GetName());
		atlas_.texture = nullptr;
	}
}

void BitmapFont::ComputeTabXAdvance() {
	if (tabXAdvance_ != 0.f) { return; }

	// 优先使用 \t glyph 的 advanceX
	for (uint32_t i = 0; i < glyphCount_; ++i) {
		if (glyphs_[i].codePoint == '\t') {
			tabXAdvance_ = static_cast<float>(glyphs_[i].advanceX);
			return;
		}
	}

	// 其次使用空格 × 4
	for (uint32_t i = 0; i < glyphCount_; ++i) {
		if (glyphs_[i].codePoint == ' ') {
			tabXAdvance_ = static_cast<float>(glyphs_[i].advanceX) * 4.f;
			return;
		}
	}

	// 最终回退
	tabXAdvance_ = static_cast<float>(size_) * 4.f;
}

// ═════════════════════════════════════════════
//  SystemFont 实现
// ═════════════════════════════════════════════

bool SystemFont::InitFromResourceData(SystemFontResourceData* resourceData, int index, IRenderer* renderer) {
	if (!resourceData) { return false; }

	renderer_ = renderer;
	ctx_ = NewObject<SystemFontContext>();
	if (!ctx_) {
		return false;
	}

	ctx_->fontBinary = resourceData->fontBinary;
	ctx_->binarySize = resourceData->binarySize;
	ctx_->index = index;
	ctx_->offset = stbtt_GetFontOffsetForIndex(
		static_cast<unsigned char*>(ctx_->fontBinary), index);

	if (!stbtt_InitFont(&ctx_->info,
		static_cast<unsigned char*>(ctx_->fontBinary), ctx_->offset)) {
		GLOG(Log::eError, "SystemFont: stbtt_InitFont failed at index %d.", index);
		return false;
	}

	face_ = resourceData->fonts[index].name;
	return true;
}

void SystemFont::ReleaseResource(IRenderer* renderer) {
	// 清理所有 Variant
	for (auto& [size, variant] : variants_) {
		variant->Cleanup(renderer);
	}
	variants_.Clear();
}

SystemFontVariant* SystemFont::AcquireVariant(int size, IRenderer* renderer) {
	// 已存在直接返回
	if (variants_.Contains(size)) {
		return variants_[size];
	}

	// 不存在则创建
	return CreateVariant(size, renderer);
}

bool SystemFont::ReleaseVariant(int size, IRenderer* renderer) {
	if (!variants_.Contains(size)) {
		return false;
	}

	variants_[size]->Cleanup(renderer);
	variants_.Remove(size);
	return true;
}

SystemFontVariant* SystemFont::CreateVariant(int size, IRenderer* renderer) {
	SystemFontVariant* variant = NewObject<SystemFontVariant>(ctx_, size, face_);
	if (!variant) {
		return nullptr;
	}

	if (!variant->Setup(renderer)) {
		GLOG(Log::eError, "SystemFont: failed to setup variant size=%d face=%s.",
			size, face_.CStr());
		return nullptr;
	}

	variants_[size] = variant;
	return variant;
}

// ═════════════════════════════════════════════
//  SystemFontVariant 实现
// ═════════════════════════════════════════════

SystemFontVariant::SystemFontVariant(SystemFontContext* ctx, int size, const FString& face)
	: ctx_(ctx), size_(size), face_(face) {

	// 预填 ASCII 95 个可打印字符（32-126）以及 -1（未知字符占位）
	codepoints_.reserve(96);
	codepoints_.push_back(-1);
	for (int i = 0; i < 95; ++i) {
		codepoints_.push_back(i + 32);
	}
}

SystemFontVariant::~SystemFontVariant() {
	// GPU 资源由 Cleanup() 显式释放，此处不重复操作
	if (glyphs_) {
		Memory::Free(glyphs_, MemoryType::eMemory_Type_Array);
		glyphs_ = nullptr;
	}
	if (kernings_) {
		Memory::Free(kernings_, MemoryType::eMemory_Type_Array);
		kernings_ = nullptr;
	}
}

bool SystemFontVariant::Setup(IRenderer* renderer) {
	// 建立 atlas 纹理
	char texName[255];
	StringFormat(texName, "__system_text_atlas_%s_i%i_sz%i__",
		face_.CStr(), ctx_->index, size_);

	atlas_.texture = TextureSystem::AcquireWriteable(
		texName, atlasSizeX_, atlasSizeY_, 4, true);

	// 从 stbtt 获取行高等度量
	scale_ = stbtt_ScaleForPixelHeight(&ctx_->info, static_cast<float>(size_));
	int ascent, descent, linegap;
	stbtt_GetFontVMetrics(&ctx_->info, &ascent, &descent, &linegap);
	lineHeight_ = static_cast<int>((ascent - descent + linegap) * scale_);

	// 建立 GPU 侧 texture map
	atlas_.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	atlas_.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	atlas_.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
	if (!renderer->AcquireTextureMap(&atlas_)) {
		GLOG(Log::eError, "SystemFontVariant: failed to acquire texture map.");
		return false;
	}

	// 首次构建 atlas 和 glyph 数据
	if (!RebuildAtlas()) {
		return false;
	}

	// 计算 tab advance
	if (tabXAdvance_ == 0.f) {
		for (uint32_t i = 0; i < glyphCount_; ++i) {
			if (glyphs_[i].codePoint == '\t') {
				tabXAdvance_ = static_cast<float>(glyphs_[i].advanceX);
				break;
			}
		}
		if (tabXAdvance_ == 0.f) {
			for (uint32_t i = 0; i < glyphCount_; ++i) {
				if (glyphs_[i].codePoint == ' ') {
					tabXAdvance_ = static_cast<float>(glyphs_[i].advanceX) * 4.f;
					break;
				}
			}
		}
		if (tabXAdvance_ == 0.f) {
			tabXAdvance_ = static_cast<float>(size_) * 4.f;
		}
	}

	return true;
}

void SystemFontVariant::Cleanup(IRenderer* renderer) {
	renderer->ReleaseTextureMap(&atlas_);
	if (atlas_.texture) {
		TextureSystem::Release(atlas_.texture->GetName());
		atlas_.texture = nullptr;
	}
}

bool SystemFontVariant::VerifyAtlas(const FString& text) {
	uint32_t charLen = static_cast<uint32_t>(text.Length());
	uint32_t addedCount = 0;

	for (uint32_t i = 0; i < charLen;) {
		FCodepointResult result = FString::BytesToCodepoint(text, text.Length(), i);
		if (!result.bValid) {
			GLOG(Log::eError, "SystemFontVariant::VerifyAtlas() invalid codepoint.");
			++i;
			continue;
		}

		i += result.Advance;

		// ASCII 范围已在构造时预填，跳过
		if (result.Codepoint < 128) { continue; }

		// 检查是否已包含该 codepoint（从第 96 个开始是扩展部分）
		bool found = false;
		uint32_t count = static_cast<uint32_t>(codepoints_.size());
		for (uint32_t j = 96; j < count; ++j) {
			if (codepoints_[j] == result.Codepoint) {
				found = true;
				break;
			}
		}

		if (!found) {
			codepoints_.push_back(result.Codepoint);
			++addedCount;
		}
	}

	// 有新 codepoint 加入，需要重建 atlas
	if (addedCount > 0) {
		return RebuildAtlas();
	}

	return true;
}

bool SystemFontVariant::RebuildAtlas() {
	uint32_t packImageSize = atlasSizeX_ * atlasSizeY_ * sizeof(unsigned char);
	uint32_t codepointCount = static_cast<uint32_t>(codepoints_.size());

	unsigned char* pixels = static_cast<unsigned char*>(
		Memory::Allocate(packImageSize, MemoryType::eMemory_Type_Array));
	stbtt_packedchar* packedChars = static_cast<stbtt_packedchar*>(
		Memory::Allocate(sizeof(stbtt_packedchar) * codepointCount, MemoryType::eMemory_Type_Array));

	// ── stbtt 打包 ───────────────────────────
	stbtt_pack_context ctx;
	if (!stbtt_PackBegin(&ctx, pixels, atlasSizeX_, atlasSizeY_, 0, 1, nullptr)) {
		GLOG(Log::eError, "SystemFontVariant: stbtt_PackBegin failed.");
		Memory::Free(pixels, MemoryType::eMemory_Type_Array);
		Memory::Free(packedChars, MemoryType::eMemory_Type_Array);
		return false;
	}

	stbtt_pack_range range;
	range.first_unicode_codepoint_in_range = 0;
	range.font_size = static_cast<float>(size_);
	range.num_chars = codepointCount;
	range.chardata_for_range = packedChars;
	range.array_of_unicode_codepoints = codepoints_.data();

	if (!stbtt_PackFontRanges(&ctx,
		static_cast<unsigned char*>(ctx_->fontBinary), ctx_->index, &range, 1)) {
		stbtt_PackEnd(&ctx);
		GLOG(Log::eError, "SystemFontVariant: stbtt_PackFontRanges failed.");
		Memory::Free(pixels, MemoryType::eMemory_Type_Array);
		Memory::Free(packedChars, MemoryType::eMemory_Type_Array);
		return false;
	}
	stbtt_PackEnd(&ctx);

	// ── 单通道 → RGBA ────────────────────────
	unsigned char* rgbaPixels = static_cast<unsigned char*>(
		Memory::Allocate(packImageSize * 4, MemoryType::eMemory_Type_Array));
	for (uint32_t j = 0; j < packImageSize; ++j) {
		rgbaPixels[(j * 4) + 0] = pixels[j];
		rgbaPixels[(j * 4) + 1] = pixels[j];
		rgbaPixels[(j * 4) + 2] = pixels[j];
		rgbaPixels[(j * 4) + 3] = pixels[j];
	}

	TextureSystem::WriteData(atlas_.texture, 0, packImageSize * 4, rgbaPixels);

	Memory::Free(pixels, MemoryType::eMemory_Type_Array);
	Memory::Free(rgbaPixels, MemoryType::eMemory_Type_Array);
	pixels = nullptr; rgbaPixels = nullptr;

	// ── 重建 Glyph 数据 ──────────────────────
	if (glyphs_ && glyphCount_) {
		Memory::Free(glyphs_, MemoryType::eMemory_Type_Array);
	}

	glyphCount_ = codepointCount;
	glyphs_ = static_cast<FontGlyph*>(
		Memory::Allocate(sizeof(FontGlyph) * glyphCount_, MemoryType::eMemory_Type_Array));

	for (uint32_t i = 0; i < glyphCount_; ++i) {
		const stbtt_packedchar& pc = packedChars[i];
		FontGlyph& g = glyphs_[i];
		g.codePoint = codepoints_[i];
		g.pageID = 0;
		g.offsetX = static_cast<short>(pc.xoff);
		g.offsetY = static_cast<short>(pc.yoff);
		g.x = pc.x0;
		g.y = pc.y0;
		g.width = pc.x1 - pc.x0;
		g.height = pc.y1 - pc.y0;
		g.advanceX = static_cast<short>(pc.xadvance);
	}

	// ── 重建 Kerning 数据 ────────────────────
	if (kernings_ && kerningCount_) {
		Memory::Free(kernings_, MemoryType::eMemory_Type_Array);
		kernings_ = nullptr;
		kerningCount_ = 0;
	}

	kerningCount_ = static_cast<uint32_t>(stbtt_GetKerningTableLength(&ctx_->info));
	if (kerningCount_ > 0) {
		kernings_ = static_cast<FontKerning*>(
			Memory::Allocate(sizeof(FontKerning) * kerningCount_, MemoryType::eMemory_Type_Array));

		stbtt_kerningentry* kerningTable = static_cast<stbtt_kerningentry*>(
			Memory::Allocate(sizeof(stbtt_kerningentry) * kerningCount_, MemoryType::eMemory_Type_Array));

		int entryCount = stbtt_GetKerningTable(&ctx_->info, kerningTable, kerningCount_);
		if (entryCount != static_cast<int>(kerningCount_)) {
			GLOG(Log::eError, "SystemFontVariant: kerning count mismatch %d -> %d.",
				entryCount, kerningCount_);
			Memory::Free(kerningTable, MemoryType::eMemory_Type_Array);
			Memory::Free(packedChars, MemoryType::eMemory_Type_Array);
			return false;
		}

		for (uint32_t i = 0; i < kerningCount_; ++i) {
			kernings_[i].codePoint0 = kerningTable[i].glyph1;
			kernings_[i].codePoint1 = kerningTable[i].glyph2;
			kernings_[i].amount = static_cast<short>(kerningTable[i].advance);
		}

		Memory::Free(kerningTable, MemoryType::eMemory_Type_Array);
	}

	Memory::Free(packedChars, MemoryType::eMemory_Type_Array);
	return true;
}
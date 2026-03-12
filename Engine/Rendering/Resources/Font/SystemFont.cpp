#include "SystemFont.hpp"
#include "Rendering/Renderer.hpp"
#include "Systems/TextureSystem.h"
#include "Systems/ResourceSystem.h"

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include <stb_truetype.h>

struct SystemFontContext {
	void* fontBinary = nullptr;
	size_t         binarySize = 0;
	int            offset = 0;
	int            index = 0;
	stbtt_fontinfo info = {};
};

bool SystemFont::InitFromResourceData(SystemFontResourceData* resourceData, int index) {
	if (!resourceData) { return false; }

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
		variant->Cleanup();
	}
	variants_.Clear();
}

SystemFontVariant* SystemFont::AcquireVariant(int size) {
	// 已存在直接返回
	if (variants_.Contains(size)) {
		return variants_[size];
	}

	// 不存在则创建
	return CreateVariant(size);
}

bool SystemFont::ReleaseVariant(int size) {
	IRenderer* renderer = IRenderer::GetRenderer();
	if (!renderer) {
		GLOG(Log::eError, "SystemFont::ReleaseVariant() failed to get renderer.");
		return false;
	}

	if (!variants_.Contains(size)) {
		return false;
	}

	variants_[size]->Cleanup();
	variants_.Remove(size);
	return true;
}

SystemFontVariant* SystemFont::CreateVariant(int size) {
	SystemFontVariant* variant = NewObject<SystemFontVariant>(ctx_, size, face_);
	if (!variant) {
		return nullptr;
	}

	if (!variant->Setup()) {
		GLOG(Log::eError, "SystemFont: failed to setup variant size=%d face=%s.",
			size, face_.CStr());
		return nullptr;
	}

	variants_[size] = variant;
	return variant;
}

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

bool SystemFontVariant::Setup() {
	// 建立 atlas 纹理
	FString texName = FString::Format(
		"__system_text_atlas_%s_i%i_sz%i__", face_.CStr(), ctx_->index, size_
	);

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

	IRenderer* Renderer = IRenderer::GetRenderer();
	if (!Renderer) {
		GLOG(Log::eError, "SystemFont::Setup() failed to get renderer.");
		return false;
	}

	if (!Renderer->AcquireTextureMap(&atlas_)) {
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

void SystemFontVariant::Cleanup() {
	IRenderer* Renderer = IRenderer::GetRenderer();
	if (!Renderer) {
		GLOG(Log::eError, "SystemFont::Cleanup() failed to get renderer.");
		return;
	}

	Renderer->ReleaseTextureMap(&atlas_);
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
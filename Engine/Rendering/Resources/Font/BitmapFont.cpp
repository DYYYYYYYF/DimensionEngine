#include "BitmapFont.hpp"
#include "Rendering/Renderer.hpp"
#include "Systems/TextureSystem.h"

bool BitmapFont::InitFromResourceData(BitmapFontResourceData* resourceData) {
	if (!resourceData) { return false; }

	resourceData_ = resourceData;

	IRenderer* Renderer = IRenderer::GetRenderer();
	if (!Renderer) {
		return false;
	}

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
	if (!Renderer->AcquireTextureMap(&atlas_)) {
		GLOG(Log::eError, "BitmapFont: failed to acquire texture map.");
		return false;
	}

	ComputeTabXAdvance();
	return true;
}

void BitmapFont::ReleaseResource() {
	IRenderer* Renderer = IRenderer::GetRenderer();
	if (Renderer) {
		Renderer->ReleaseTextureMap(&atlas_);
	}
	
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

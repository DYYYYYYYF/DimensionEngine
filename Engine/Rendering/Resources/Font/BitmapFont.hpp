#pragma once

#include "Font.hpp"

// ─────────────────────────────────────────────
//  BitmapFont : UAsset, IFont
//  职责：位图字体，既是磁盘资产，也是直接可用的渲染对象
// ─────────────────────────────────────────────

class BitmapFont : public UAsset, public IFont {
	friend class BitmapFontLoader;

public:
	BitmapFont() = default;
	~BitmapFont() = default;

	// 禁止拷贝
	BitmapFont(const BitmapFont&) = delete;
	BitmapFont& operator=(const BitmapFont&) = delete;

	// ── UAsset 资源层（由 ResourceSystem 调用）──
	// 从已加载的 BitmapFontResourceData 初始化自身数据
	bool InitFromResourceData(BitmapFontResourceData* resourceData);
	void ReleaseResource();

	// ── IFont 接口 ────────────────────────────
	// Bitmap 字体 atlas 已在加载时生成，无需运行时校验
	bool               VerifyAtlas(const FString&) override { return true; }
	const FontGlyph* GetGlyphs()       const override { return glyphs_; }
	uint32_t           GetGlyphCount()   const override { return glyphCount_; }
	const FontKerning* GetKernings()     const override { return kernings_; }
	uint32_t           GetKerningCount() const override { return kerningCount_; }
	const TextureMap& GetAtlas()        const override { return atlas_; }
	int                GetLineHeight()   const override { return lineHeight_; }
	int                GetBaseLine()     const override { return baseLine_; }
	float              GetTabXAdvance()  const override { return tabXAdvance_; }
	const FString& GetFace()         const override { return face_; }
	unsigned int       GetSize()         const override { return size_; }

private:
	// 从 resourceData 读取后填充 tabXAdvance
	void ComputeTabXAdvance();

	// ── 数据成员 ──────────────────────────────
	FString       face_;
	unsigned int  size_ = 0;
	int           lineHeight_ = -1;
	int           baseLine_ = -1;
	float         tabXAdvance_ = 0.f;
	int           atlasSizeX_ = 1024;
	int           atlasSizeY_ = 1024;
	TextureMap    atlas_ = {};
	FontGlyph* glyphs_ = nullptr;
	uint32_t      glyphCount_ = 0;
	FontKerning* kernings_ = nullptr;
	uint32_t      kerningCount_ = 0;

	BitmapFontResourceData* resourceData_ = nullptr;
};

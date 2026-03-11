#pragma once

#include "FontType.hpp"
#include "Rendering/Resources/Asset.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

// Forward declarations
class IRenderer;
struct SystemFontContext;

// ─────────────────────────────────────────────
//  IFont —— 渲染层接口
//  职责：提供渲染所需的 glyph / atlas / kerning 数据
//  不负责磁盘加载，不继承 UAsset
// ─────────────────────────────────────────────

class IFont {
public:
	virtual ~IFont() = default;

	// Atlas 校验：SystemFontVariant 需要按需扩充，BitmapFont 直接返回 true
	virtual bool VerifyAtlas(const FString& text) = 0;

	// ── 数据访问 ──────────────────────────────
	virtual const FontGlyph* GetGlyphs()       const = 0;
	virtual uint32_t           GetGlyphCount()   const = 0;
	virtual const FontKerning* GetKernings()     const = 0;
	virtual uint32_t           GetKerningCount() const = 0;
	virtual const TextureMap& GetAtlas()        const = 0;
	virtual int                GetLineHeight()   const = 0;
	virtual int                GetBaseLine()     const = 0;
	virtual float              GetTabXAdvance()  const = 0;
	virtual const FString& GetFace()         const = 0;
	virtual unsigned int       GetSize()         const = 0;

public:
	void AddRef() { ++refCount_; }
	bool Release() { return --refCount_ == 0; }

private:
	int refCount_ = 0;
};

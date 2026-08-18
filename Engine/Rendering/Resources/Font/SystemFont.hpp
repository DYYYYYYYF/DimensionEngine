#pragma once

#include "Font.hpp"

// ─────────────────────────────────────────────
//  SystemFontContext
//  SystemFont 加载后共享给所有 Variant 的 stbtt 运行时数据
//  对外不可见，仅在 Font.hpp / FontSystem.cpp 内部使用
// ─────────────────────────────────────────────

struct FSystemFontContext;

// ─────────────────────────────────────────────
//  SystemFontVariant : IFont
//  职责：代表"某个 SystemFont 在某个具体尺寸下"的渲染实例
//  生命周期依附于父级 SystemFont，不是独立的磁盘资源
// ─────────────────────────────────────────────

class USystemFontVariant : public IFont {
public:
	USystemFontVariant(FSystemFontContext* ctx, int size, const FString& face);
	~USystemFontVariant();

	// 禁止拷贝
	USystemFontVariant(const USystemFontVariant&) = delete;
	USystemFontVariant& operator=(const USystemFontVariant&) = delete;

	// ── IFont 接口 ────────────────────────────
	bool               VerifyAtlas(const FString& text) override;
	const FFontGlyph* GetGlyphs()       const override { return glyphs_; }
	uint32_t           GetGlyphCount()   const override { return glyphCount_; }
	const FFontKerning* GetKernings()     const override { return kernings_; }
	uint32_t           GetKerningCount() const override { return kerningCount_; }
	const FTextureMap& GetAtlas()        const override { return atlas_; }
	int                GetLineHeight()   const override { return lineHeight_; }
	int                GetBaseLine()     const override { return baseLine_; }
	float              GetTabXAdvance()  const override { return tabXAdvance_; }
	const FString& GetFace()         const override { return face_; }
	unsigned int       GetSize()         const override { return size_; }

	// ── 初始化（由 SystemFont 调用）───────────
	// 构建 atlas 纹理、采集 glyph 数据，完成后才可用于渲染
	bool Setup();
	void Cleanup();

private:
	// 重建 atlas（首次 Setup 或新增 codepoint 时调用）
	bool RebuildAtlas();

	// 新增 codepoint 并重建 atlas
	void AddCodepoint(int codepoint);

	// ── 运行时数据 ────────────────────────────
	FSystemFontContext* ctx_ = nullptr;  // 不拥有，借用父级数据
	FString            face_;
	unsigned int       size_ = 0;
	float              scale_ = 0.f;
	int                lineHeight_ = -1;
	int                baseLine_ = -1;
	float              tabXAdvance_ = 0.f;
	int                atlasSizeX_ = 1024;
	int                atlasSizeY_ = 1024;
	FTextureMap         atlas_ = {};
	FFontGlyph* glyphs_ = nullptr;
	uint32_t           glyphCount_ = 0;
	FFontKerning* kernings_ = nullptr;
	uint32_t           kerningCount_ = 0;
	std::vector<int>   codepoints_;
};


// ─────────────────────────────────────────────
//  SystemFont : UAsset
//  职责：TTF 字体文件的资产容器，是 SystemFontVariant 的工厂
//  本身不实现 IFont，因为没有具体尺寸就没有渲染数据
// ─────────────────────────────────────────────

class USystemFont : public UAsset {
public:
	USystemFont(const FString& Name) : UAsset(Name) {}
	~USystemFont() = default;

	// 禁止拷贝
	USystemFont(const USystemFont&) = delete;
	USystemFont& operator=(const USystemFont&) = delete;

	// ── UAsset 资源层 ─────────────────────────
	// 从已加载的 SystemFontResourceData 初始化，建立 stbtt 解析上下文
	// index 对应 TTF 文件内的字体索引（一个 TTF 可含多个字型）
	bool InitFromResourceData(FSystemFontResourceData* resourceData, int index);
	void ReleaseResource(IRenderer* renderer);

	// ── Variant 工厂 ──────────────────────────
	// 按尺寸获取 Variant；不存在则创建并初始化
	// 返回 nullptr 表示创建失败
	USystemFontVariant* AcquireVariant(int size);
	bool ReleaseVariant(int size);

	const FString& GetFace() const { return face_; }

private:
	USystemFontVariant* CreateVariant(int size);

	// ── 数据成员 ──────────────────────────────
	FString            face_;
	FSystemFontContext* ctx_ = {};   // stbtt 运行时数据，共享给所有 Variant

	// size → Variant，替代原来的 vector + 线性搜索
	TMap<int, USystemFontVariant*> variants_;
};
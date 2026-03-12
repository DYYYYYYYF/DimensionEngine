#pragma once

#include "Math/MathTypes.hpp"
#include "Framework/Classes/TextActor.h"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Rendering/RenderTypes.hpp"

#include "Rendering/Resources/Font/BitmapFont.hpp"
#include "Rendering/Resources/Font/SystemFont.hpp"

#include <string>
#include <unordered_map>
#include <memory>

// ─────────────────────────────────────────────
//  Config 结构体（基本不变，略作清理）
// ─────────────────────────────────────────────

struct SystemFontConfig {
	FString        name;
	FString        resourceName;
	unsigned short defaultSize = 0;
};

struct BitmapFontConfig {
	FString        name;
	FString        resourceName;
	unsigned short size = 0;
};

struct FontSystemConfig {
	unsigned char       defaultSystemFontCount = 0;
	SystemFontConfig* systemFontConfigs = nullptr;
	unsigned char       defaultBitmapFontCount = 0;
	BitmapFontConfig* bitmapFontConfigs = nullptr;
	unsigned char       maxSystemFontCount = 10;
	unsigned char       maxBitmapFontCount = 10;
	bool                autoRelease = false;
};

// ─────────────────────────────────────────────
//  FontSystem
//
//  职责：字体资产的注册表 + Acquire/Release 入口
//
//  对外只暴露 IFont*，调用方不需要感知
//  BitmapFont / SystemFont / SystemFontVariant 的区别。
//
//  加载流程：
//    RegisterBitmapFont → ResourceSystem::Load → BitmapFont::InitFromResourceData
//    RegisterSystemFont → ResourceSystem::Load → SystemFont::InitFromResourceData
//
//  Acquire 流程：
//    BitmapFont  → 直接返回 BitmapFont*（它自身实现 IFont）
//    SystemFont  → 委托给 SystemFont::AcquireVariant(size) 返回 SystemFontVariant*
//
//  VerifyAtlas：
//    直接调用 font->VerifyAtlas(text)，FontSystem 不再感知字体类型
// ─────────────────────────────────────────────

class FontSystem {
public:
	static FontSystem& Get();

public:
	bool Initialize(IRenderer* renderer, const FontSystemConfig& config);
	void Shutdown();

	// 注册字体资产（替代原来的 LoadBitmapFont / LoadSystemFont）
	// 内部调用 ResourceSystem::Load，成功后存入对应注册表
	bool RegisterBitmapFont(const BitmapFontConfig& config);
	bool RegisterSystemFont(const SystemFontConfig& config);

	// 获取可渲染字体
	//   BitmapFont：fontSize 忽略，name 唯一确定字体
	//   SystemFont：name + fontSize 共同确定 Variant
	// 返回 nullptr 表示未找到或创建失败
	IFont* Acquire(const FString& fontName, UITextType type, int fontSize = 0);

	// 归还引用；引用计数归零时由 FontSystem 决定是否卸载
	bool   Release(IFont* font);

	// Atlas 校验：直接委托给 font->VerifyAtlas()
	// FontSystem 不再需要感知字体类型
	bool   VerifyAtlas(IFont* font, const FString& text);

private:
	// 内部注册表
	// BitmapFont 和 SystemFont 都继承自 UAsset，用 unique_ptr 管理生命周期
	TMap<FString, BitmapFont*> BitmapFonts;
	TMap<FString, SystemFont*> SystemFonts;

	bool       Initialized;
	IRenderer* Renderer;
	FontSystemConfig Config;
};
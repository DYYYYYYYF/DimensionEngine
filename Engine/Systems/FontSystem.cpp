#include "FontSystem.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include "Systems/TextureSystem.h"
#include "Systems/ResourceSystem.h"

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
		if (font) font->ReleaseResource();
	}
	BitmapFonts.Clear();

	for (auto& [name, font] : SystemFonts) {
		if(font) font->ReleaseResource(Renderer);
	}
	SystemFonts.Clear();

	Initialized = false;
}

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
	if (!ResourceSystem::Get().Load(config.resourceName, EAssetType::BitmapFont, nullptr, font)) {
		GLOG(Log::eError, "Failed to load bitmap font resource: %s.", config.resourceName.CStr());
		return false;
	}

	// 从 UAsset::Data 中取出 ResourceData，完成 GPU 侧初始化
	BitmapFontResourceData* resourceData = static_cast<BitmapFontResourceData*>(font->Data);
	if (!font->InitFromResourceData(resourceData)) {
		GLOG(Log::eError, "Failed to initialize bitmap font: %s.", config.name.CStr());
		return false;
	}

	BitmapFonts[config.name] = font;
	return true;
}

bool FontSystem::RegisterSystemFont(const SystemFontConfig& config) {
	// 先加载资源，一个 TTF 文件可能包含多个字型（face）
	UAsset loadedAsset;
	if (!ResourceSystem::Get().Load(config.resourceName.CStr(), EAssetType::SystemFont, nullptr, &loadedAsset)) {
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
		if (!font->InitFromResourceData(resourceData, i)) {
			GLOG(Log::eError, "Failed to initialize system font '%s' at index %u.", faceName.CStr(), i);
			continue;
		}

		// 预创建 defaultSize 对应的 Variant，避免首次 Acquire 时卡顿
		if (config.defaultSize > 0) {
			if (!font->AcquireVariant(config.defaultSize)) {
				GLOG(Log::eError, "Failed to create default size variant for '%s'.", faceName.CStr());
			}
		}

		SystemFonts[faceName] = font;
	}

	return true;
}

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

		SystemFontVariant* Variant = Font->AcquireVariant(fontSize);
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
					bitmapFont->ReleaseResource();
					BitmapFonts.Remove(name);
					return true;
				}
			}

			// SystemFontVariant 需要通过 face + size 在 SystemFonts 里找
			// 可以通过 font->GetFace() 和 font->GetSize() 定位
			FString FaceName = font->GetFace();
			uint32_t FontSize = font->GetSize();
			if (SystemFonts.Contains(FaceName)) {
				SystemFonts[FaceName]->ReleaseVariant(FontSize);
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

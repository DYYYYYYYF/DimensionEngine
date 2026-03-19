#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"

struct FileHandle;
struct SystemFontResourceData;

enum SystemFontFileType {
	eSystem_Font_File_Type_Not_Found,
	eSystem_Font_File_Type_DSF,
	eSystem_Font_File_Type_Font_Config
};

struct SupportedSystemFontFiletype {
	const char* extension = nullptr;
	SystemFontFileType type;
	bool               isBinary;
};

// ─────────────────────────────────────────────────────────────────
//  SystemFontLoader
//
//  负责从磁盘加载 TTF 字体的元数据与二进制内容，
//  填充 SystemFontResourceData（font face 列表 + TTF 二进制块）。
//
//  Load() 接收的 UAsset* 是普通 UAsset，不是 SystemFont*。
//  原因：SystemFont 在 Load 完成后才能初始化（需要 stbtt），
//  GPU 侧和 stbtt 初始化由 SystemFont::InitFromResourceData() 完成。
//
//  支持格式：
//    .fontcfg  文本配置文件，首次加载时转换为 .dsf
//    .dsf      引擎二进制缓存格式，直接读取
// ─────────────────────────────────────────────────────────────────

class SystemFontLoader : public IResourceLoader {
public:
	SystemFontLoader();

	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

private:
	// 从 .fontcfg 文本文件导入，并输出 .dsf 二进制缓存
	bool ImportFontconfigFile(const FString& f, const FString& typePath,
		const FString& outDSFFilename, SystemFontResourceData* outResource);

	// 读 / 写引擎二进制格式 .dsf
	bool ReadDSFFile(const FString& file, SystemFontResourceData* data);
	bool WriteDSFFile(const FString& outDSFFilename, SystemFontResourceData* resource);
};
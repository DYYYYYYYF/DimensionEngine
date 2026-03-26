#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"

struct FileHandle;
struct BitmapFontResourceData;

enum BitmapFontFileType {
	eBitmap_Font_File_Type_Not_Found,
	eBitmap_Font_File_Type_DBF,
	eBitmap_Font_File_Type_FNT
};

struct SupportedBitmapFontFileType {
	const char* extension = nullptr;
	BitmapFontFileType type;
	bool               isBinary;
};

// ─────────────────────────────────────────────────────────────────
//  BitmapFontLoader
//
//  负责从磁盘加载位图字体资产，填充 BitmapFontResourceData。
//
//  Load() 接收的 UAsset* 实际上是 BitmapFont*（继承自 UAsset），
//  Loader 只负责填充原始数据，不做 GPU 侧初始化。
//  GPU 初始化由 BitmapFont::InitFromResourceData() 完成。
//
//  支持格式：
//    .fnt  文本格式（AngelCode BMFont），首次加载时转换为 .dbf
//    .dbf  引擎二进制缓存格式，直接读取
// ─────────────────────────────────────────────────────────────────

class BitmapFontLoader : public IResourceLoader {
public:
	BitmapFontLoader();

	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

private:
	// 从 .fnt 文本文件导入，并输出 .dbf 二进制缓存
	bool ImportFntFile(const FString& asset_path, const FString& outDbfFilename, BitmapFontResourceData* outData);
	bool ParseFntLine(const FString& line, uint32_t lineNum, 
		uint32_t* glyphsRead, uint32_t* kerningsRead, BitmapFontResourceData* outData);

	// 读 / 写引擎二进制格式 .dbf
	bool ReadDbfFile(const FString& asset_path, BitmapFontResourceData* data);
	bool WriteDbfFile(const FString& asset_path, BitmapFontResourceData* data);
};
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
	bool isBinary;
};

class BitmapFontLoader : public IResourceLoader {
public:
	BitmapFontLoader();

public:
	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

public: 
	bool ImportFntFile(FileHandle* fntFile, const char* outDbfFilename, BitmapFontResourceData* out_data);
	bool ReadDbfFile(FileHandle* file, BitmapFontResourceData* data);
	bool WriteDbfFile(const char* path, BitmapFontResourceData* data);

};

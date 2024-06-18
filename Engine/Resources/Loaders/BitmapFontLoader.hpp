#pragma once

#include "Resources/Loaders/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

struct FileHandle;
struct BitmapFontResourceData;

enum BitmapFontFileType {
	eBitmap_Font_File_Type_Not_Found,
	eBitmap_Font_File_Type_DBF,
	eBitmap_Font_File_Type_FNT
};

struct SupportedBitmapFontFileType {
	char* extension;
	BitmapFontFileType type;
	bool isBinary;
};

class BitmapFontLoader : public IResourceLoader {
public:
	BitmapFontLoader();

public:
	virtual bool Load(const char* name, void* params, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

public: 
	bool ImportFntFile(FileHandle* fntFile, const char* outDbfFilename, BitmapFontResourceData* out_data);
	bool ReadDbfFile(FileHandle* file, BitmapFontResourceData* data);
	bool WriteDbfFile(const char* path, BitmapFontResourceData* data);

};
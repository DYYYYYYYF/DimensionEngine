#pragma once

#include "Resources/Loaders/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

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
	bool isBinary;
};

class SystemFontLoader : public IResourceLoader {
public:
	SystemFontLoader();

public:
	virtual bool Load(const char* name, void* params, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

public:
	bool ImportFontconfigFile(FileHandle* f, const char* typePath, const char* outDSFFilename, SystemFontResourceData* outResource);
	bool ReadDSFFile(FileHandle* file, SystemFontResourceData* data);
	bool WriteDSFFile(const char* outDSFFilename, SystemFontResourceData* resource);

};

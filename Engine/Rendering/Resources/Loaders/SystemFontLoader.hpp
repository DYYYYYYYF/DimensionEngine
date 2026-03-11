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
	bool isBinary;
};

class SystemFontLoader : public IResourceLoader {
public:
	SystemFontLoader();

public:
	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

public:
	bool ImportFontconfigFile(FileHandle* f, const char* typePath, const char* outDSFFilename, SystemFontResourceData* outResource);
	bool ReadDSFFile(FileHandle* file, SystemFontResourceData* data);
	bool WriteDSFFile(const char* outDSFFilename, SystemFontResourceData* resource);

};

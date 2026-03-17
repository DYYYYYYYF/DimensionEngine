#pragma once

#include "Framework/Iobject.h"
#include "Containers/FString.hpp"

/** @brief A magic number indicating the file as engine file. */
#ifndef RESOURCES_MAGIC
#define RESOURCES_MAGIC 0xdddddddd
#endif

enum class EAssetType {
	Text = 0,
	Binary,
	Texture,
	Material,
	Geometry,
	StaticMesh,
	Shader,
	BitmapFont,
	SystemFont,
	Custom,
	Unkonw
};

struct ResourceHeader {
	uint32_t magicNumber;
	unsigned char resourceType;
	unsigned char version;
	unsigned short reserved;
};

class DAPI UAsset : public IObject {
public:
	UAsset() {}
	UAsset(const FString& name) : Name(name){}

	virtual ~UAsset() = default;

public:
	virtual void PreInitialize() override {}
	virtual bool Initialize() override { return true; }
	virtual void PostInitialize() override {}

public:
	void SetName(const FString& n) { Name = n; }
	const FString& GetName() const { return Name; }

	void SetFullPath(const FString& p) { FullPath = p; }
	const FString& GetFullPath() const { return FullPath; }

	void SetLoaded(bool b = true) { bIsLoaded = b; }
	bool IsLoaded() const { return bIsLoaded; }


public:
	EAssetType AssetType = EAssetType::Unkonw;
	uint32_t LoaderID = INVALID_ID;
	FString Name;
	FString FullPath;
	size_t DataSize = 0;
	size_t DataCount = 0;
	void* Data = nullptr;

	bool bIsLoaded = false;
};

// TODO: 移到Image内部
struct ImageResourceData {
	unsigned char channel_count = 4;
	uint32_t width = 1920;
	uint32_t height = 1080;
	unsigned char* pixels = nullptr;
};

struct ImageResourceParams {
	bool flip_y = false;
};
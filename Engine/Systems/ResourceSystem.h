#pragma once

#include "Rendering/Resources/Asset.hpp"
#include <vector>

class IResourceLoader;

struct SResourceSystemConfig {
	uint32_t max_loader_count;

	// The relative base path for assets.
    FString asset_base_path;
};

class ResourceSystem {
public:
	static ResourceSystem& Get();

public:
	bool Initialize(SResourceSystemConfig config);
	void Shutdown();

public:
	bool RegisterLoader(IResourceLoader* loader);
	bool Load(const FString& name, EAssetType type, void* params, UAsset* resource);
	bool LoadCustom(const FString& name, const char* custom_type, void* params, UAsset* resource);

	void Unload(UAsset* resource);
	const char* GetRootPath();

public:
	SResourceSystemConfig Config;
	std::vector<IResourceLoader*> RegisteredLoaders;

	bool Initilized = false;
};

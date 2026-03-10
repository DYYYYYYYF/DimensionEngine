#pragma once

#include "Rendering/Resources/Asset.hpp"
#include <vector>

class IResourceLoader;

struct SResourceSystemConfig {
	uint32_t max_loader_count;

	// The relative base path for assets.
    std::string asset_base_path;
};

class ResourceSystem {
public:
	static bool Initialize(SResourceSystemConfig config);
	static void Shutdown();

public:
	static bool RegisterLoader(IResourceLoader* loader);
	static bool Load(const std::string& name, EAssetType type, void* params, UAsset* resource);
	static bool LoadCustom(const std::string& name, const char* custom_type, void* params, UAsset* resource);

	static void Unload(UAsset* resource);
	static const char* GetRootPath();

public:
	static SResourceSystemConfig Config;
	static std::vector<IResourceLoader*> RegisteredLoaders;

	static bool Initilized;
};

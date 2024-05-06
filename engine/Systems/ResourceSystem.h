#pragma once

#include "Resources/Resource.hpp"
#include "Containers/TArray.hpp"

class IResourceLoader;

struct SResourceSystemConfig {
	uint32_t max_loader_count;

	// The relative base path for assets.
	char* asset_base_path;
};

class ResourceSystem {
public:
	static bool Initialize(SResourceSystemConfig config);
	static void Shutdown();

public:
	static bool RegisterLoader(IResourceLoader* loader);
	static bool Load(const char* name, ResourceType type, Resource* resource);
	static bool LoadCustom(const char* name, const char* custom_type, Resource* resource);

	static void Unload(Resource* resource);
	static const char* GetRootPath();

public:
	static SResourceSystemConfig Config;
	static TArray<IResourceLoader*> RegisteredLoaders;

	static bool Initilized;
};
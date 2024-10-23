#include "ResourceSystem.h"

#include "Renderer/Vulkan/VulkanContext.hpp"

// Known resource loaders.
#include "Resources/Loaders/BinaryLoader.h"
#include "Resources/Loaders/ImageLoader.hpp"
#include "Resources/Loaders/MaterialLoader.h"
#include "Resources/Loaders/ShaderLoader.h"
#include "Resources/Loaders/MeshLoader.h"
#include "Resources/Loaders/BitmapFontLoader.hpp"
#include "Resources/Loaders/SystemFontLoader.hpp"

SResourceSystemConfig ResourceSystem::Config;
std::vector<IResourceLoader*> ResourceSystem::RegisteredLoaders;
bool ResourceSystem::Initilized = false;

bool ResourceSystem::Initialize(SResourceSystemConfig config) {
	if (config.max_loader_count == 0) {
		LOG_FATAL("Resource system initialize failed because config.max_loader_count == 0.");
		return false;
	}

	Config = config;

	// Invalidate all loaders.
	RegisteredLoaders.resize(config.max_loader_count);
	for (uint32_t i = 0; i < config.max_loader_count; ++i) {
		RegisteredLoaders[i] = (IResourceLoader*)Memory::Allocate(sizeof(IResourceLoader), MemoryType::eMemory_Type_Array);
		RegisteredLoaders[i]->Id = INVALID_ID;
	}

	// NOTE: Auto-register known loader types here.
	IResourceLoader* BinLoader = NewObject<BinaryLoader>();
	RegisterLoader(BinLoader);
	IResourceLoader* ImgLoader = NewObject<ImageLoader>();
	RegisterLoader(ImgLoader);
	IResourceLoader* MatLoader = NewObject<MaterialLoader>();
	RegisterLoader(MatLoader);
	IResourceLoader* ShaLoader = NewObject<ShaderLoader>();
	RegisterLoader(ShaLoader);
	IResourceLoader* MesLoader = NewObject<MeshLoader>();
	RegisterLoader(MesLoader);	
	IResourceLoader* BitFontLoader = NewObject<BitmapFontLoader>();
	RegisterLoader(BitFontLoader);
	IResourceLoader* SysFontLoader = NewObject<SystemFontLoader>();
	RegisterLoader(SysFontLoader);

	Initilized = true;
	LOG_INFO("Resource system initialize with base path: '%s'.", config.asset_base_path);
	return true;

}

void ResourceSystem::Shutdown() {
	if (Initilized) {
		for (uint32_t i = 0; i < RegisteredLoaders.size(); i++) {
			DeleteObject(RegisteredLoaders[i]);
		}

		RegisteredLoaders.clear();
		std::vector<IResourceLoader*>().swap(RegisteredLoaders);
	}
}

bool ResourceSystem::RegisterLoader(IResourceLoader* loader) {
	uint32_t Count = Config.max_loader_count;

	// Ensure no loaders for the given type already exist.
	for (uint32_t i = 0; i < Count; ++i) {
		if (RegisteredLoaders[i]->Id != INVALID_ID) {
			if (RegisteredLoaders[i]->Type == loader->Type) {
				LOG_ERROR("Resource system register loader error. Loader of type %d already exists and will ot be registered.", loader->Type);
				return false;
			}
			else if (RegisteredLoaders[i]->CustomType && strlen(RegisteredLoaders[i]->CustomType) > 0 && strcmp(RegisteredLoaders[i]->CustomType, loader->CustomType) == 0) {
				LOG_ERROR("Resource system register loader error. Loader of custom type %d already exists and will ot be registered.", loader->CustomType);
				return false;
			}
		}
	}

	for (uint32_t i = 0; i < Count; ++i) {
		if (RegisteredLoaders[i]->Id == INVALID_ID) {
			RegisteredLoaders[i] = loader;
			RegisteredLoaders[i]->Id = i;

			LOG_INFO("Loader registered.");
			return true;
		}
	}

	return false;
}

bool ResourceSystem::Load(const char* name, ResourceType type, void* params, Resource* resource) {
	if (type != eResource_type_Custom) {
		// Select loader.
		uint32_t Count = Config.max_loader_count;
		for (uint32_t i = 0; i < Count; ++i) {
			if (RegisteredLoaders[i]->Id != INVALID_ID && RegisteredLoaders[i]->Type == type) {
				resource->LoaderID = RegisteredLoaders[i]->Id;
				return RegisteredLoaders[i]->Load(name, params, resource);
			}
		}
	}

	resource->LoaderID = INVALID_ID;
	LOG_ERROR("Resouce system load. No loader for type %d was found.", type);
	return false;
}

bool ResourceSystem::LoadCustom(const char* name, const char* custom_type, void* params, Resource* resource) {
	if (custom_type == nullptr || strlen(custom_type) == 0) {
		LOG_ERROR("Resouce system load custom failed. custom type is invalid.");
		return false;
	}

	uint32_t Count = Config.max_loader_count;
	for (uint32_t i = 0; i < Count; ++i) {
		if (RegisteredLoaders[i]->Id != INVALID_ID && RegisteredLoaders[i]->Type == eResource_type_Custom && strcmp(RegisteredLoaders[i]->CustomType, custom_type)== 0) {
			resource->LoaderID = RegisteredLoaders[i]->Id;
			return RegisteredLoaders[i]->Load(name, params, resource);
		}
	}

	resource->LoaderID = INVALID_ID;
	LOG_ERROR("Resouce system load custom. No custom loader for type %d was found.", custom_type);
	return false;
}

void ResourceSystem::Unload(Resource* resource) {
	if (resource == nullptr) {
		return;
	}

	if (resource->LoaderID != INVALID_ID) {
		IResourceLoader* Loader = RegisteredLoaders[resource->LoaderID];
		if (Loader->Id != INVALID_ID) {
			Loader->Unload(resource);
		}
	}
}

const char* ResourceSystem::GetRootPath() {
	if (Initilized) {
		return Config.asset_base_path;
	}

	LOG_ERROR("Resource system GetRootPaht() called beform initialization, returning empty string.");
	return "";
}


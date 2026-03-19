#include "ResourceSystem.h"

#include "Rendering/Vulkan/VulkanContext.hpp"

// Known resource loaders.
#include "Rendering/Resources/Loaders/BinaryLoader.h"
#include "Rendering/Resources/Loaders/MaterialLoader.h"
#include "Rendering/Resources/Loaders/ShaderLoader.h"
#include "Rendering/Resources/Loaders/MeshLoader.h"
#include "Rendering/Resources/Loaders/BitmapFontLoader.hpp"
#include "Rendering/Resources/Loaders/SystemFontLoader.hpp"

ResourceSystem& ResourceSystem::Get() {
	static ResourceSystem ResouceSystemInstance;
	return ResouceSystemInstance;
}

bool ResourceSystem::Initialize(SResourceSystemConfig config) {
	if (config.max_loader_count == 0) {
		GLOG(Log::eFatal, "Resource system initialize failed because config.max_loader_count == 0.");
		return false;
	}

	Config = config;

	// NOTE: Auto-register known loader types here.
	IResourceLoader* BinLoader = NewObject<BinaryLoader>();
	RegisterLoader(BinLoader);
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
	GLOG(Log::eInfo, "Resource system initialize with base path: '%s'.", config.asset_base_path.CStr());
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
	uint32_t Count = (uint32_t)RegisteredLoaders.size();
	if (Count > Config.max_loader_count) {
		GLOG(Log::eError, "Can not register more loader, max loader count is %d.", Config.max_loader_count);
		return false;
	}

	// Ensure no loaders for the given type already exist.
	for (uint32_t i = 0; i < Count; ++i) {
		if (RegisteredLoaders[i] == nullptr) continue;
		if (RegisteredLoaders[i]->Id != INVALID_ID) {
			if (RegisteredLoaders[i]->Type == loader->Type) {
				GLOG(Log::eError, "Resource system register loader error. Loader of type %d already exists and will ot be registered.", loader->Type);
				return false;
			}
			else if (RegisteredLoaders[i]->CustomType.Length() > 0 && RegisteredLoaders[i]->CustomType.Compare(loader->CustomType) == 0) {
				GLOG(Log::eError, "Resource system register loader error. Loader of custom type %d already exists and will ot be registered.", loader->CustomType.CStr());
				return false;
			}
		}
	}

	loader->Id = Count;
	RegisteredLoaders.push_back(loader);

	return true;
}

bool ResourceSystem::Load(const FString& name, EAssetType type, void* params, UAsset* resource) {
	if (type != EAssetType::Custom) {
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
	GLOG(Log::eError, "Resouce system load. No loader for type %d was found.", type);
	return false;
}

bool ResourceSystem::LoadCustom(const FString& name, const char* custom_type, void* params, UAsset* resource) {
	if (custom_type == nullptr || strlen(custom_type) == 0) {
		GLOG(Log::eError, "Resouce system load custom failed. custom type is invalid.");
		return false;
	}

	uint32_t Count = Config.max_loader_count;
	for (uint32_t i = 0; i < Count; ++i) {
		if (RegisteredLoaders[i]->Id != INVALID_ID && RegisteredLoaders[i]->Type == EAssetType::Custom && RegisteredLoaders[i]->CustomType.Compare(custom_type)== 0) {
			resource->LoaderID = RegisteredLoaders[i]->Id;
			return RegisteredLoaders[i]->Load(name, params, resource);
		}
	}

	resource->LoaderID = INVALID_ID;
	GLOG(Log::eError, "Resouce system load custom. No custom loader for type %d was found.", custom_type);
	return false;
}

void ResourceSystem::Unload(UAsset* resource) {
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
		return Config.asset_base_path.CStr();
	}

	GLOG(Log::eError, "Resource system GetRootPaht() called beform initialization, returning empty string.");
	return "";
}


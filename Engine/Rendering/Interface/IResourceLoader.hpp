#pragma once

#include "Rendering/Resources/Asset.hpp"
#include <string>

class IResourceLoader {
public:
	IResourceLoader() : Id(INVALID_ID), Type(EAssetType::Unkonw){}
	virtual ~IResourceLoader() {
		Id = INVALID_ID;
		Type = EAssetType::Unkonw;
	}

public:
	virtual bool Load(const std::string& name, void* params, UAsset* resource) = 0;
	virtual void Unload(UAsset* resouce) = 0;

public:
	uint32_t Id;
	EAssetType Type;
	std::string CustomType;
	std::string TypePath;

};

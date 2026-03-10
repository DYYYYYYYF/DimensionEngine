#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

class ImageLoader : public IResourceLoader {
public:
	ImageLoader();

public:
	virtual bool Load(const std::string& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

};
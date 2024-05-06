#pragma once

#include "Resources/Loaders/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

class ImageLoader : public IResourceLoader {
public:
	ImageLoader();

public:
	virtual bool Load(const char* name, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

};
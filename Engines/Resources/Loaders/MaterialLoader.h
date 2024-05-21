#pragma once

#include "Resources/Loaders/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

class MaterialLoader : public IResourceLoader {
public:
	MaterialLoader();

public:
	virtual bool Load(const char* name, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

};
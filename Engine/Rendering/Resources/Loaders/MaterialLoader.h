#pragma once

#include "Rendering/Resources/Loaders/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

class MaterialLoader : public IResourceLoader {
public:
	MaterialLoader();

public:
	virtual bool Load(const std::string&, void* params, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

};
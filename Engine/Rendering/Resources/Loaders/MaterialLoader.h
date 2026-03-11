#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

class MaterialLoader : public IResourceLoader {
public:
	MaterialLoader();

public:
	virtual bool Load(const FString&, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

};
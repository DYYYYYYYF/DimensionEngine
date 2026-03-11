#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"

class ImageLoader : public IResourceLoader {
public:
	ImageLoader();

public:
	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

};
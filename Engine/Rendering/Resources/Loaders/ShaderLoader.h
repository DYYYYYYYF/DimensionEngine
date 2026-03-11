#pragma once
#include "Rendering/Interface/IResourceLoader.hpp"

class ShaderLoader : public IResourceLoader {
public:
	ShaderLoader();

public:
	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

};

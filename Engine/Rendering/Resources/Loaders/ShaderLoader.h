#pragma once
#include "Rendering/Interface/IResourceLoader.hpp"

class ShaderLoader : public IResourceLoader {
public:
	ShaderLoader();

public:
	std::string GetPath() { return TypePath; }
	virtual bool Load(const std::string& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

};

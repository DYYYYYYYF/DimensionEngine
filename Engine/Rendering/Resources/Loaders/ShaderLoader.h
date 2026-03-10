#pragma once
#include "IResourceLoader.hpp"

class ShaderLoader : public IResourceLoader {
public:
	ShaderLoader();

public:
	std::string GetPath() { return TypePath; }
	virtual bool Load(const std::string& name, void* params, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

};

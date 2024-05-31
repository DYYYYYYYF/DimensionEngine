#pragma once
#include "IResourceLoader.hpp"

class ShaderLoader : public IResourceLoader {
public:
	ShaderLoader();

public:
	virtual bool Load(const char* name, void* params, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

};

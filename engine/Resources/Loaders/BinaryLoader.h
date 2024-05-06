#pragma once

#include "Resources/Loaders/IResourceLoader.hpp"

class BinaryLoader : public IResourceLoader {
public:
	BinaryLoader();

public:
	virtual bool Load(const char* name, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

};
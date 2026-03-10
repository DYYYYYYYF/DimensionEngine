#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"

class BinaryLoader : public IResourceLoader {
public:
	BinaryLoader();

public:
	virtual bool Load(const std::string& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

};
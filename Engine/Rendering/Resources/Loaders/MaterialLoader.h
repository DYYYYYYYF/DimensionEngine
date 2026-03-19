#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"
#include "Systems/ResourceSystem.h"

struct SMaterialConfig;

class MaterialLoader : public IResourceLoader {
public:
	MaterialLoader();

public:
	virtual bool Load(const FString&, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

private:
	bool ParseLineData(size_t index, const FString& line, SMaterialConfig* resource);

};
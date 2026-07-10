#pragma once
#include "Rendering/Interface/IResourceLoader.hpp"
#include "Rendering/Resources/Shader/ShaderType.hpp"

struct ShaderConfig;

class ShaderLoader : public IResourceLoader {
public:
	ShaderLoader();

public:
	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

private:
	bool ParseLineData(size_t index, const FString& line, ShaderConfig* resource);
	ShaderSemantic ParseSemantic(const FString& semantic);

};

#pragma once
#include "Rendering/Interface/IResourceLoader.hpp"
#include "Rendering/Resources/Shader/ShaderType.hpp"

struct FShaderConfig;

class ShaderLoader : public IResourceLoader {
public:
	ShaderLoader();

public:
	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

private:
	bool ParseLineData(size_t index, const FString& line, FShaderConfig* resource);
	ShaderSemantic ParseSemantic(const FString& semantic);

};

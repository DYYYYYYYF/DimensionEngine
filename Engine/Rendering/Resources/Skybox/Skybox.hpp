#pragma once

#include "Rendering/Resources/Asset.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"

class UGeometry;
class IRenderer;

class DAPI USkybox : public UAsset {
public:
	bool Create(const FString& cubeName);
	void Destroy();

public:
	UGeometry* GetGeometry() const { return geo; }

public:
	IRenderer* Renderer = nullptr;
	FTextureMap CubeMap;
	uint32_t InstanceID = INVALID_ID;
	size_t RenderFrameNumber = 0;

private:
	UGeometry* geo = nullptr;

};

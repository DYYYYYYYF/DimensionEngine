#pragma once

#include "Rendering/Resources/Asset.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"

class IRenderer;

class DAPI Skybox : public UAsset {
public:
	bool Create(const FString& cubeName);
	void Destroy();

public:
	IRenderer* Renderer = nullptr;
	TextureMap CubeMap;
	class Geometry* g = nullptr;
	uint32_t InstanceID = INVALID_ID;
	size_t RenderFrameNumber = 0;
};

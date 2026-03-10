#pragma once
#include "Texture.hpp"

class IRenderer;

class DAPI Skybox {
public:
	bool Create(const std::string& cubeName);
	void Destroy();

public:
	IRenderer* Renderer = nullptr;
	TextureMap CubeMap;
	class Geometry* g = nullptr;
	uint32_t InstanceID = INVALID_ID;
	size_t RenderFrameNumber = 0;
};

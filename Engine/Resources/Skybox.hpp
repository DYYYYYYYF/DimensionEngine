#pragma once
#include "Texture.hpp"

class IRenderer;

class DAPI Skybox {
public:
	bool Create(const char* cubeName, IRenderer* renderer);
	void Destroy();

public:
	IRenderer* Renderer = nullptr;
	TextureMap CubeMap;
	class Geometry* g = nullptr;
	uint32_t InstanceID;
	size_t RenderFrameNumber;
};

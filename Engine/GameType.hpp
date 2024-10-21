#pragma once

#include "Core/Application.hpp"

static IRenderer* Renderer = nullptr;

class IGame {
private:
	struct GameFrameData {
	public:
		std::vector<GeometryRenderData> WorldGeometries;
	};

public:
	virtual bool Boot(IRenderer* renderer) = 0;
	virtual void Shutdown() = 0;

	virtual bool Initialize() = 0;
	virtual bool Update(float delta_time) = 0;
	virtual bool Render(struct SRenderPacket* packet, float delta_time) = 0;

	virtual void OnResize(unsigned int width, unsigned int height) = 0;

public:
	SApplicationConfig app_config;
	GameFrameData FrameData;
};

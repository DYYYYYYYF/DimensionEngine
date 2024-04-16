#pragma once

#include "RendererTypes.hpp"

struct SStaticMeshData;
struct SPlatformState;
class IRendererBackend;

class IRenderer {
public:
	IRenderer(const char* application_name, struct SPlatformState* plat_state);
	virtual ~IRenderer();

public:
	bool BeginFrame(double delta_time);
	bool EndFrame(double delta_time);

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state);
	virtual void Shutdown();

	virtual void OnResize(unsigned short width, unsigned short height);
	virtual bool DrawFrame(SRenderPacket* packet);

private:
	class IRendererBackend* Backend;
};

#pragma once

#include "RendererTypes.hpp"
#include "Resources/Texture.hpp"

struct SStaticMeshData;
struct SPlatformState;
class IRendererBackend;

// Temp
struct SEventContext;

class IRenderer {
public:
	IRenderer();
	IRenderer(RendererBackendType type, struct SPlatformState* plat_state);
	virtual ~IRenderer();

public:
	bool BeginFrame(double delta_time);
	bool EndFrame(double delta_time);

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state);
	virtual void Shutdown();

	virtual void OnResize(unsigned short width, unsigned short height);
	virtual bool DrawFrame(SRenderPacket* packet);
	
	virtual void SetViewTransform(Matrix4 view) { View = view; }


	virtual void CreateTexture(const char* name, bool auto_release, int width, int height, int channel_count,
		const unsigned char* pixels, bool has_transparency, Texture* texture);
	virtual void DestroyTexture(Texture* txture);

	virtual void CreateTexture(Texture* texture);
	virtual bool LoadTexture(const char* name, Texture* texture);

public:
	Texture TestDiffuse;

protected:
	RendererBackendType BackendType;
	class IRendererBackend* Backend;

	// Projection perspective
	Matrix4 Projection;
	Matrix4 View;
	float NearClip;
	float FarClip;

	Texture DefaultTexture;
};

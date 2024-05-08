#pragma once

#include "RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

struct SStaticMeshData;
struct SPlatformState;
class IRendererBackend;
class Geometry;

// Temp
struct SEventContext;

class IRenderer {
public:
	IRenderer();
	IRenderer(RendererBackendType type, struct SPlatformState* plat_state);
	virtual ~IRenderer();

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state);
	virtual void Shutdown();

	virtual void OnResize(unsigned short width, unsigned short height);
	virtual bool DrawFrame(SRenderPacket* packet);
	
	virtual void SetViewTransform(Matrix4 view) { View = view; }

	virtual void CreateTexture(const unsigned char* pixels, Texture* texture);
	virtual void CreateTexture(Texture* texture);
	virtual void DestroyTexture(Texture* txture);

	virtual bool CreateMaterial(Material* material);
	virtual void DestroyMaterial(Material* material);

	virtual bool IRenderer::CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices);

	virtual void IRenderer::DestroyGeometry(Geometry* geometry);

protected:
	RendererBackendType BackendType;
	class IRendererBackend* Backend;

	// Projection perspective
	Matrix4 Projection;
	Matrix4 View;
	float NearClip;
	float FarClip;

	Matrix4 UIProjection;
	Matrix4 UIView;

};

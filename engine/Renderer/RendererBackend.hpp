#pragma once

#include "RendererTypes.hpp"

struct SPlatformState;
class Texture;
class Material;
class Geometry;

class IRendererBackend {
public:
	IRendererBackend() {};

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state) = 0;
	virtual void Shutdown() = 0;

	virtual bool BeginFrame(double delta_time) = 0;
	virtual void UpdateGlobalState(Matrix4 projection, Matrix4 view, Vec3 view_position, Vec4 ambient_color, int mode) = 0;
	virtual bool EndFrame(double delta_time) = 0;
	virtual void Resize(unsigned short width, unsigned short height) = 0;

	virtual void DrawGeometry(GeometryRenderData geometry) = 0;

	virtual void CreateTexture(const unsigned char* pixels, Texture* texture) = 0;
	virtual void DestroyTexture(Texture* txture) = 0;

	virtual bool CreateMaterial(Material* material) = 0;
	virtual void DestroyMaterial(Material* material) = 0;

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_count, const Vertex* vertices, uint32_t index_count, const uint32_t* indices) = 0;
	virtual void DestroyGeometry(Geometry* geometry) = 0;

public:
	SPlatformState* GetPlatformState() { return PlatformState; }

	size_t GetFrameNum() const { return FrameNum; }
	void SetFrameNum(size_t num) { FrameNum = num; }

public:
	// Points to default textures.
	Texture* DefaultDiffuse;

protected:
	RendererBackendType BackendType;
	struct SPlatformState* PlatformState;
	size_t FrameNum;

};


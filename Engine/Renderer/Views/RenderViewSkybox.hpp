#pragma once

#include "Defines.hpp"
#include "Renderer/Interface/IRenderView.hpp"

class Camera;

class RenderViewSkybox : public IRenderView {
public:
	virtual void OnCreate() override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool OnBuildPacket(void* data, struct RenderViewPacket* out_packet) const override;
	virtual bool OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) const override;

private:
	bool ReserveY;
	uint32_t ShaderID;
	float Fov;
	float NearClip;
	float FarClip;
	Matrix4 ProjectionMatrix;
	Camera* WorldCamera = nullptr;
	// Uniform locations
	unsigned short ProjectionLocation;
	unsigned short ViewLocation;
	unsigned short CubeMapLocation;
};
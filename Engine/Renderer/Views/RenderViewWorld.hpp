#pragma once

#include "Defines.hpp"
#include "Renderer/Interface/IRenderView.hpp"

class Camera;
class Shader;

class RenderViewWorld : public IRenderView {
public:
	RenderViewWorld();
	RenderViewWorld(const RenderViewConfig& config);
	virtual bool OnCreate(const RenderViewConfig& config) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool OnBuildPacket(void* data, struct RenderViewPacket* out_packet) override;
	virtual void OnDestroyPacket(struct RenderViewPacket* packet) override;
	virtual bool OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) override;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) override;

private:
	bool ReserveY;
	Shader* UsedShader = nullptr;
	float NearClip;
	float FarClip;
	float Fov;
	Matrix4 ProjectionMatrix;
	Camera* WorldCamera = nullptr;
	Vec4 AmbientColor;
};
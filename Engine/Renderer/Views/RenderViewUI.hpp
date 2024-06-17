#pragma once

#include "Defines.hpp"
#include "Renderer/Interface/IRenderView.hpp"

class Shader;

class RenderViewUI : public IRenderView {
public:
	virtual void OnCreate() override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool OnBuildPacket(void* data, struct RenderViewPacket* out_packet) const override;
	virtual void OnDestroyPacket(struct RenderViewPacket* packet) const override;
	virtual bool OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) const override;

private:
	uint32_t ShaderID;
	Shader* UsedShader;
	float NearClip;
	float FarClip;
	Matrix4 ProjectionMatrix;
	Matrix4 ViewMatrix;

	// UI text
	unsigned short DiffuseMapLocation;
	unsigned short DiffuseColorLocation;
	unsigned short ModelLocation;
};
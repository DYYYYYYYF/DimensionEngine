#pragma once

#include "Defines.hpp"
#include "Renderer/Interface/IRenderView.hpp"
#include "Resources/Texture.hpp"

class Shader;
class IREnderer;

struct RenderviewPickShaderInfo {
	Shader* UsedShader = nullptr;
	VulkanRenderPass* Pass = nullptr;
	float NearClip;
	float FarClip;
	float Fov;
	Matrix4 ProjectionMatrix;
	Matrix4 ViewMatrix;

	unsigned short IDColorLocation;
	unsigned short ViewLocation;
	unsigned short ModelLocation;
	unsigned short ProjectionLocation;
};

class RenderViewPick : public IRenderView {
public:
	RenderViewPick(const RenderViewConfig& config, IRenderer* renderer);
	virtual bool OnCreate(const RenderViewConfig& config) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool OnBuildPacket(void* data, struct RenderViewPacket* out_packet) override;
	virtual void OnDestroyPacket(struct RenderViewPacket* packet) override;
	virtual bool OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) override;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) override;

public:
	void AcquireShaderInstance();
	void ReleaseShaderInstance();

public:
	short GetMouseX() const { return MouseX; }
	void SetMouseX(short x) { MouseX = x; }
	short GetMouseY() const { return MouseY; }
	void SetMouseY(short y) { MouseY = y; }

private:
	IRenderer* Renderer = nullptr;
	RenderviewPickShaderInfo UIShaderInfo;
	RenderviewPickShaderInfo WorlShaderInfo;

	// Used as the color attachment for both renderpass.
	Texture ColorTargetAttachment;
	Texture DepthTargetAttachment;

	int InstanceCount;
	std::vector<bool> InstanceUpdated;

	short MouseX, MouseY;

};
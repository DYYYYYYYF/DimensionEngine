#pragma once

#include "Defines.hpp"
#include "Rendering/Interface/IRenderView.hpp"

class UShader;
class ACameraActor;

class RenderViewSkybox : public IRenderView {
public:
	RenderViewSkybox(const RenderViewConfig& config);
	virtual bool OnCreate(const RenderViewConfig& config) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) override;

	virtual void Render(const TArray<FRenderProxy*>& RenderObejcts) override;

private:
	IRenderer* Renderer;
	UShader* UsedShader = nullptr;
	float Fov;
	float NearClip;
	float FarClip;
	Matrix4 ProjectionMatrix;
	ACameraActor* WorldCamera = nullptr;
	// Uniform locations
	uint32_t ProjectionLocation;
	uint32_t ViewLocation;
	uint32_t CubeMapLocation;
};
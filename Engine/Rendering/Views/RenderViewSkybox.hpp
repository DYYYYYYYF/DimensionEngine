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
	// 覆写：视口矩形变化时更新投影矩阵（按视口宽高比）
	virtual void SetViewportRect(const Vector4& rect) override;

	virtual void Render(const TArray<FRenderProxy*>& RenderObejcts) override;

private:
	// 已应用的视口矩形；(0,0,0,0) 表示尚未按视口配置过资源
	Vector4 AppliedViewportRect = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

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
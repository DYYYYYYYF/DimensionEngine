#pragma once

#include "Defines.hpp"
#include "Rendering/Interface/IRenderView.hpp"

class UShader;

class RenderViewUI : public IRenderView {
public:
	RenderViewUI(const RenderViewConfig& config);
	virtual bool OnCreate(const RenderViewConfig& config) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) override;

	virtual void Render(const TArray<FRenderProxy*>& RenderObejcts) override;

private:
	UShader* UsedShader = nullptr;
	// UI 合批着色器：UI 代理几何优先使用（颜色入顶点 + blend=1 真混合）。
	// 加载失败时回退 UsedShader（原 UI shader），保证旧路径可运行。
	UShader* BatchedShader = nullptr;
	float NearClip;
	float FarClip;
	Matrix4 ProjectionMatrix;
	Matrix4 ViewMatrix;

	IRenderer* Renderer;

	// UI text
	uint32_t DiffuseMapLocation;
	uint32_t DiffuseColorLocation;
	uint32_t ModelLocation;

	// R4 静态 DrawCall 缓存：UI 代理内容代次未变且无文本代理时，直接复用
	// 缓存数组，跳过每帧的 DrawCall 重建与排序（静止 UI 帧零构建开销）。
	TArray<DrawCall> CachedUIDrawCalls;
	uint64_t CachedUIDrawGeneration = 0;
	bool bUIDrawCacheValid = false;
};
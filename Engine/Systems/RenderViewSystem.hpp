#pragma once

#include "Defines.hpp"
#include "Math/MathTypes.hpp"
#include "Containers/TArray.hpp"
#include "Rendering/Interface/IRenderView.hpp"

class IRenderer;

struct SRenderViewSystemConfig {
	unsigned short max_view_count;
	FString config_path;
};

class RenderViewSystem {
public:
	static DAPI RenderViewSystem& Get();

public:
	IRenderView* Get(ERenderViewType Type);

	bool Initialize(IRenderer* renderer, SRenderViewSystemConfig config);
	void Shutdown();

	bool Create(const RenderViewConfig& config);
	void OnWindowResize(uint32_t width, uint32_t height);
	void RegenerateRendertargets(IRenderView* view);

	// 设置世界渲染（Skybox / Deferred）共用的视口矩形（窗口坐标系）。
	// 编辑器布局在窗口中央留出渲染区域后，通过此接口把矩形推给世界视图；
	// UI / Pick 视图仍按全窗口渲染，不受影响。
	DAPI void SetWorldViewportRect(const Vector4& rect);

private:
	bool LoadRenderviewConfig(const FString& path);

private:
	bool Initialized;
	IRenderer* Renderer;
	uint16_t MaxViewCount;

	TArray<IRenderView*> RegisteredViews;
	TMap<ERenderViewType, uint16_t> RegisteredViewMap;
};
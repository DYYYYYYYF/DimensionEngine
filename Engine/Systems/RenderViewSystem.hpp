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
	bool Initialize(IRenderer* renderer, SRenderViewSystemConfig config);
	void Shutdown();

	bool Create(const RenderViewConfig& config);
	void OnWindowResize(uint32_t width, uint32_t height);

	DAPI IRenderView* Get(const FString& name);

	DAPI bool BuildPacket(IRenderView* view, IRenderviewPacketData* data, struct RenderViewPacket* out_packet);
	bool OnRender(IRenderView* view, RenderViewPacket* packet, size_t frame_number, size_t render_target_index);

	void RegenerateRendertargets(IRenderView* view);

private:
	bool LoadRenderviewConfig(const FString& path);

private:
	bool Initialized;
	IRenderer* Renderer;
	uint16_t MaxViewCount;

	std::vector<IRenderView*> RegisteredViews;
	std::unordered_map<FString, uint16_t> RegisteredViewMap;
};
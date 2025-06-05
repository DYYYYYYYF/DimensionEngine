#pragma once

#include "Defines.hpp"
#include "Math/MathTypes.hpp"
#include "Containers/TArray.hpp"
#include "Containers/THashTable.hpp"
#include "Renderer/Interface/IRenderView.hpp"

class IRenderer;

struct SRenderViewSystemConfig {
	unsigned short max_view_count;
};

class RenderViewSystem {
public:
	static bool Initialize(IRenderer* renderer, SRenderViewSystemConfig config);
	static void Shutdown();

	static bool Create(const RenderViewConfig& config);
	static void OnWindowResize(uint32_t width, uint32_t height);

	DAPI static IRenderView* Get(const std::string& name);

	DAPI static bool BuildPacket(IRenderView* view, IRenderviewPacketData* data, struct RenderViewPacket* out_packet);
	static bool OnRender(IRenderView* view, RenderViewPacket* packet, size_t frame_number, size_t render_target_index);

	static void RegenerateRendertargets(IRenderView* view);

private:
	static bool Initialized;
	static IRenderer* Renderer;
	static uint32_t MaxViewCount;

	static std::vector<IRenderView*> RegisteredViews;
	static std::unordered_map<std::string, uint32_t> RegisteredViewMap;
};
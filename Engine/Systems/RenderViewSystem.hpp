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

	static IRenderView* Get(const char* name);

	static bool BuildPacket(const IRenderView* view, void* data, struct RenderViewPacket* out_packet);
	static bool OnRender(const IRenderView* view, RenderViewPacket* packet, size_t frame_number, size_t render_target_index);

private:
	static HashTable Lookup;
	static void* TableBlock;
	static uint32_t MaxViewCount;
	static TArray<IRenderView*> RegisteredViews;

	static bool Initialized;
	static IRenderer* Renderer;
};
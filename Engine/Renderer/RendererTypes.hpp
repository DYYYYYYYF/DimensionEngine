#pragma once

#include "Defines.hpp"
#include "Math/MathTypes.hpp"

#include <vector>
#include <functional>

struct RenderpassConfig;

enum RendererBackendType {
	eRenderer_Backend_Type_Vulkan,
	eRenderer_Backend_Type_OpenGL,
	eRenderer_Backend_Type_DirecX
};

enum RenderpassClearFlags {
	eRenderpass_Clear_None = 0x00,
	eRenderpass_Clear_Color_Buffer = 0x01,
	eRenderpass_Clear_Depth_Buffer = 0x02,
	eRenderpass_Clear_Stencil_Buffer = 0x04
};

struct SRenderPacket {
	double delta_time;

	uint32_t geometry_count;
	std::vector<struct GeometryRenderData> geometries;

	uint32_t ui_geometry_count;
	struct GeometryRenderData* ui_geometries = nullptr;
};

struct GeometryRenderData {
	Matrix4 model;
	class Geometry* geometry = nullptr;
};

struct RenderTarget {
	bool sync_to_window_size;
	unsigned char attachment_count;
	std::vector<class Texture*> attachments;
	void* internal_framebuffer = nullptr;
};

struct RenderBackendConfig {
	const char* application_name = nullptr;
	unsigned short renderpass_count;
	RenderpassConfig* pass_config = nullptr	;

	// Function
	std::function<void()> OnRenderTargetRefreshRequired;
};
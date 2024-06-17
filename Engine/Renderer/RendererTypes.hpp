#pragma once

#include "Defines.hpp"
#include "Math/MathTypes.hpp"
#include "Resources/Shader.hpp"
#include "Renderer/Interface/IRenderbuffer.hpp"

#include <vector>
#include <functional>

struct RenderpassConfig;
class Mesh;
class Skybox;
class IRenderer;
class IRenderpass;

enum RendererBackendType {
	eRenderer_Backend_Type_Vulkan,
	eRenderer_Backend_Type_OpenGL,
	eRenderer_Backend_Type_DirecX
};

struct GeometryRenderData {
	Matrix4 model;
	class Geometry* geometry = nullptr;
};

struct SRenderViewPassConfig {
	const char* name = nullptr;
};

struct SRenderPacket {
	double delta_time;
	unsigned short view_count;
	std::vector<struct RenderViewPacket> views;
};

struct RenderTarget {
	bool sync_to_window_size;
	unsigned char attachment_count;
	std::vector<class Texture*> attachments;
	void* internal_framebuffer = nullptr;
};

struct MeshPacketData {
	uint32_t mesh_count;
	std::vector<Mesh*> meshes;
};

struct UIPacketData {
	MeshPacketData meshData;
	// TODO: temp
	uint32_t textCount;
	std::vector<class UIText*> Textes;
};

struct SkyboxPacketData {
	Skybox* sb = nullptr;
};

struct RenderBackendConfig {
	const char* application_name = nullptr;
	unsigned short renderpass_count;
	RenderpassConfig* pass_config = nullptr	;

	// Function
	std::function<void()> OnRenderTargetRefreshRequired;
};

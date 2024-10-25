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

enum RenderTargetAttachmentType {
	eRender_Target_Attachment_Type_Color = 0x01,
	eRender_Target_Attachment_Type_Depth = 0x02,
	eRender_Target_Attachment_Type_Stencil = 0x04
};

enum RenderTargetAttachmentSource {
	eRender_Target_Attachment_Source_Default = 0x1,
	eRender_Target_Attachment_Source_View = 0x2
};

enum RenderTargetAttachmentLoadOperation {
	eRender_Target_Attachment_Load_Operation_DontCare = 0x0,
	eRender_Target_Attachment_Load_Operation_Load = 0x1
};

enum RenderTargetAttachmentStoreOperation {
	eRender_Target_Attachment_Store_Operation_DontCare = 0x0,
	eRender_Target_Attachment_Store_Operation_Store = 0x1
};

struct RenderTargetAttachmentConfig {
	RenderTargetAttachmentType type;
	RenderTargetAttachmentSource source;
	RenderTargetAttachmentLoadOperation loadOperation;
	RenderTargetAttachmentStoreOperation storeOperation;
	bool presentAfter;
};

struct RenderTargetConfig {
	unsigned char attachmentCount;
	std::vector<RenderTargetAttachmentConfig> attachments;
};

struct RenderTargetAttachment {
	RenderTargetAttachmentType type;
	RenderTargetAttachmentSource source;
	RenderTargetAttachmentLoadOperation loadOperation;
	RenderTargetAttachmentStoreOperation storeOperation;
	bool presentAfter;
	class Texture* texture = nullptr;
};

struct GeometryRenderData {
	Matrix4 model;
	class Geometry* geometry = nullptr;
	uint32_t uniqueID;
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
	std::vector<struct RenderTargetAttachment> attachments;
	void* internal_framebuffer = nullptr;
};

struct MeshPacketData {
	uint32_t mesh_count;
	Mesh** meshes = nullptr;
};

struct UIPacketData {
	MeshPacketData meshData;
	// TODO: temp
	uint32_t textCount;
	class UIText** Textes = nullptr;
};

struct PickPacketData {
	std::vector<GeometryRenderData> WorldMeshData;
	MeshPacketData UIMeshData;
	uint32_t UIGeometryCount;
	// TODO: Temp.
	uint32_t TextCount;
	class UIText** Texts = nullptr;
};

struct SkyboxPacketData {
	Skybox* sb = nullptr;
};

struct RenderBackendConfig {
	const char* application_name = nullptr;
};

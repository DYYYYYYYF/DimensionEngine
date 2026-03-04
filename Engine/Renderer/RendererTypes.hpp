#pragma once

#include "Defines.hpp"
#include "Math/MathTypes.hpp"
#include "Containers/TArray.hpp"
#include "Resources/Shader.hpp"
#include "Renderer/Interface/IRenderbuffer.hpp"
#include "Framework/Classes/MeshActor.h"

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
	eRender_Target_Attachment_Load_Operation_Load = 0x1,
	eRender_Target_Attachment_Load_Operation_Clear = 0x2
};

enum RenderTargetAttachmentStoreOperation {
	eRender_Target_Attachment_Store_Operation_DontCare = 0x0,
	eRender_Target_Attachment_Store_Operation_Store = 0x1,
};

struct RenderTargetAttachmentConfig {
	uint32_t index = 0;
	RenderTargetAttachmentType type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	RenderTargetAttachmentSource source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	RenderTargetAttachmentLoadOperation loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load;
	RenderTargetAttachmentStoreOperation storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	// 如果当前Attachment是需要渲染到屏幕的则为true
	bool presentAfter = true;
};

struct RenderTargetConfig {
	std::vector<RenderTargetAttachmentConfig> attachments;
};

struct RenderTargetAttachment {
	uint32_t index = 0;	// 用于区分多个颜色附件的索引
	RenderTargetAttachmentType type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	RenderTargetAttachmentSource source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	RenderTargetAttachmentLoadOperation loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load;
	RenderTargetAttachmentStoreOperation storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	bool presentAfter = true;
	class Texture* texture = nullptr;
};

struct GeometryRenderData {
	Matrix4 model_mat;
	class Geometry* geometry = nullptr;
	uint32_t uniqueID = INVALID_ID;
};

struct SRenderViewPassConfig {
	const char* name = nullptr;
};

struct SRenderPacket {
	double delta_time = 0.0;
	unsigned short view_count = 0;
	std::vector<struct RenderViewPacket> views;
};

struct RenderTarget {
	bool sync_to_window_size = true;
	std::vector<struct RenderTargetAttachment> attachments;
	void* internal_framebuffer = nullptr;
};

struct IRenderviewPacketData {};
struct WorldPacketData : public IRenderviewPacketData {
public:
	WorldPacketData() {}
	WorldPacketData(const WorldPacketData& data) {
		Meshes = data.Meshes;
	}

	std::vector<GeometryRenderData> Meshes;
	float GlobalTime;
};

struct MeshPacketData : public IRenderviewPacketData {
	uint32_t mesh_count = 0;
	AMeshActor** meshes = nullptr;
};

struct UIPacketData : public IRenderviewPacketData {
public:
	UIPacketData() {}
	UIPacketData(const UIPacketData& data) {
		meshData = data.meshData;
		textCount = data.textCount;
		Textes = data.Textes;
	}

	MeshPacketData meshData;
	// TODO: temp
	uint32_t textCount = 0;
	class ATextActor** Textes = nullptr;
};

struct PickPacketData : public IRenderviewPacketData {
public:
	PickPacketData() {}
	PickPacketData(const PickPacketData& data) {
		WorldMeshData = data.WorldMeshData;
		UIMeshData = data.UIMeshData;
		UIGeometryCount = data.UIGeometryCount;
		TextCount = data.TextCount;
		Texts = data.Texts;
	}

	std::vector<GeometryRenderData> WorldMeshData;
	MeshPacketData UIMeshData;
	uint32_t UIGeometryCount = 0;
	// TODO: Temp.
	uint32_t TextCount = 0;
	class ATextActor** Texts = nullptr;
};

struct SkyboxPacketData : public IRenderviewPacketData {
public:
	SkyboxPacketData() {}
	SkyboxPacketData(const SkyboxPacketData& data) {
		sb = data.sb;
	}

	Skybox* sb = nullptr;
};

struct RenderBackendConfig {
	std::string application_name;
};

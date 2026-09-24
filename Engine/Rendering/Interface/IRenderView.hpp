#pragma once
#include "Math/MathTypes.hpp"
#include "Rendering/Vulkan/VulkanRenderpass.hpp"

#include <vector>
#include <functional>

class FRenderProxy;
struct RenderViewPacket;
struct RenderTargetAttachment;

enum class ERenderViewType {
	Unknown = 0x01,
	World = 0x02,
	UI = 0x03,
	Skybox = 0x04,
	Pick = 0x05,
	Deferred = 0x06,
};

enum class RenderViewViewMatrixtSource {
	eRender_View_View_Matrix_Source_Scene_Camera = 0x01,
	eRender_View_View_Matrix_Source_UI_Camera = 0x02,
	eRender_View_View_Matrix_Source_Light_Camera = 0x03
};

enum class RenderViewProjectionMatrixSource {
	eRender_View_Projection_Matrix_Source_Default_Perspective = 0x01,
	eRender_View_Projection_Matrix_Source_Default_Orthographic = 0x02,
};

struct RenderViewConfig {
	FString name;
	FString custom_shader_name;
	unsigned short width = 1920;
	unsigned short height = 1080;
	ERenderViewType type = ERenderViewType::Deferred;
	RenderViewViewMatrixtSource view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;
	RenderViewProjectionMatrixSource projection_matrix_source = RenderViewProjectionMatrixSource::eRender_View_Projection_Matrix_Source_Default_Perspective;
	unsigned char pass_count = 0;
	TArray<struct RenderpassConfig> passes;
};

class IRenderView {
public:
	virtual bool OnCreate(const RenderViewConfig& config) = 0;
	virtual void OnDestroy() = 0;
	virtual void OnResize(uint32_t width, uint32_t height) = 0;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) = 0;

	virtual void Render(const TArray<FRenderProxy*>& RenderObejcts) {};

public:
	virtual uint16_t GetID() { return ID; }
	virtual void SetID(uint16_t id) { ID = id; }
	virtual EShaderRenderMode GetRenderMode() const { return render_mode; }
	virtual void SetRenderMode(EShaderRenderMode mode) { render_mode = mode; }
	virtual TArray<class VulkanRenderPass>& GetRenderpass() { return Passes; }

public:
	// ---- 视口矩形（窗口坐标系：x=水平偏移, y=垂直偏移, z=宽度, w=高度）----
	// 默认 (0,0,0,0) 表示未设置，视图渲染时回退为全窗口。
	virtual void SetViewportRect(const Vector4& rect) { ViewportRect = rect; }
	virtual const Vector4& GetViewportRect() const { return ViewportRect; }
	// 解析有效视口矩形：未设置（宽或高为 0）时回退为全窗口。
	Vector4 ResolveViewportRect(uint32_t winW, uint32_t winH) const {
		if (ViewportRect.z > 0.0f && ViewportRect.w > 0.0f) {
			return ViewportRect;
		}
		return Vector4(0.0f, 0.0f, (float)winW, (float)winH);
	}

public:
	uint16_t ID = INVALID_ID_U16;
	FString Name;
	uint16_t Width = 1920;
	uint16_t Height = 1080;
	ERenderViewType Type = ERenderViewType::Deferred;
	unsigned char RenderpassCount = 0;
	TArray<class VulkanRenderPass> Passes;

	// 正在重建的渲染目标索引：RenderViewSystem::RegenerateRendertargets 在调用
	// RegenerateAttachmentTarget 之前写入当前渲染目标的下标，使视图能够为不同渲染目标
	// 绑定各自的多缓冲资源（如按 swapchain 图像索引区分的 G-Buffer 套件、阴影贴图）。
	// 默认 0，对不读取该字段的视图保持原有行为。
	uint32_t RegeneratingTargetIndex = 0;

	// 视口矩形（窗口坐标系：x=水平偏移, y=垂直偏移, z=宽度, w=高度）。
	// 默认 (0,0,0,0) 表示未设置，ResolveViewportRect 会回退为全窗口。
	Vector4 ViewportRect = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

	FString CustomShaderName;
	EShaderRenderMode render_mode = EShaderRenderMode::eShader_Render_Mode_Default;

};

struct RenderViewPacket {
	IRenderView* view = nullptr;
	Matrix4 view_matrix;
	Matrix4 projection_matrix;
	Vector3 view_position;
	Vector4 ambient_color;
	float global_time;
	uint32_t geometry_count = 0;
	TArray<struct GeometryRenderData> geometries;
	const char* custom_shader_name = nullptr;
	IRenderviewPacketData* extended_data = nullptr;
};

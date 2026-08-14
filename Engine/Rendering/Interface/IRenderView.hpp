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
	std::vector<struct RenderpassConfig> passes;
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
	virtual std::vector<class VulkanRenderPass>& GetRenderpass() { return Passes; }

public:
	uint16_t ID = INVALID_ID_U16;
	FString Name;
	uint16_t Width = 1920;
	uint16_t Height = 1080;
	ERenderViewType Type = ERenderViewType::Deferred;
	unsigned char RenderpassCount = 0;
	std::vector<class VulkanRenderPass> Passes;
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
	std::vector<struct GeometryRenderData> geometries;
	const char* custom_shader_name = nullptr;
	IRenderviewPacketData* extended_data = nullptr;
};

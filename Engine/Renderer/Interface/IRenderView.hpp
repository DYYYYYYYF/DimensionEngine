#pragma once
#include "Math/MathTypes.hpp"
#include "Renderer/Vulkan/VulkanRenderpass.hpp"

#include <vector>
#include <functional>

struct RenderViewPacket;
struct RenderTargetAttachment;

enum RenderViewKnownType {
	eRender_View_Known_Type_World = 0x01,
	eRender_View_Known_Type_UI = 0x02,
	eRender_View_Known_Type_Skybox = 0x03,
	/** @brief A view which only renders ui and world objects to be picked. */
	eRender_View_Known_Type_Pick = 0x04
};

enum RenderViewViewMatrixtSource {
	eRender_View_View_Matrix_Source_Scene_Camera = 0x01,
	eRender_View_View_Matrix_Source_UI_Camera = 0x02,
	eRender_View_View_Matrix_Source_Light_Camera = 0x03
};

enum RenderViewProjectionMatrixSource {
	eRender_View_Projection_Matrix_Source_Default_Perspective = 0x01,
	eRender_View_Projection_Matrix_Source_Default_Orthographic = 0x02,
};

struct RenderViewConfig {
	const char* name = nullptr;
	const char* custom_shader_name = nullptr;
	unsigned short width;
	unsigned short height;
	RenderViewKnownType type;
	RenderViewViewMatrixtSource view_matrix_source;
	RenderViewProjectionMatrixSource projection_matrix_source;
	unsigned char pass_count;
	std::vector<struct RenderpassConfig> passes;
};

class IRenderView {
public:
	virtual bool OnCreate(const RenderViewConfig& config) = 0;
	virtual void OnDestroy() = 0;
	virtual void OnResize(uint32_t width, uint32_t height) = 0;
	virtual bool OnBuildPacket(void* data, struct RenderViewPacket* out_packet) = 0;
	virtual void OnDestroyPacket(struct RenderViewPacket* packet) = 0;
	virtual bool OnRender(struct RenderViewPacket* packet, class IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) = 0;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) = 0;

public:
	virtual unsigned short GetID() { return ID; }
	virtual void SetID(unsigned short id) { ID = id; }
	virtual ShaderRenderMode GetRenderMode() const { return render_mode; }
	virtual void SetRenderMode(ShaderRenderMode mode) { render_mode = mode; }
	virtual std::vector<class VulkanRenderPass>& GetRenderpass() { return Passes; }

public:
	unsigned short ID;
	const char* Name = nullptr;
	unsigned short Width;
	unsigned short Height;
	RenderViewKnownType Type;
	unsigned char RenderpassCount;
	std::vector<class VulkanRenderPass> Passes;
	const char* CustomShaderName = nullptr;
	ShaderRenderMode render_mode = ShaderRenderMode::eShader_Render_Mode_Default;

};

struct RenderViewPacket {
	IRenderView* view = nullptr;
	Matrix4 view_matrix;
	Matrix4 projection_matrix;
	Vec3 view_position;
	Vec4 ambient_color;
	uint32_t geometry_count;
	std::vector<struct GeometryRenderData> geometries;
	const char* custom_shader_name = nullptr;
	void* extended_data = nullptr;
};

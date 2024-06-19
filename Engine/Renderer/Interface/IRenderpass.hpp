#pragma once

#include <vulkan/vulkan.hpp>
#include "Renderer/RendererTypes.hpp"

class VulkanContext;
class VulkanCommandBuffer;

enum VulkanRenderPassState {
	eRenderPass_State_Ready,
	eRenderPass_State_Recording,
	eRenderPass_State_In_Renderpass,
	eRenderPass_State_Recording_Ended,
	eRenderPass_State_Submited,
	eRenderPass_State_Not_Allocated
};

enum RenderpassClearFlags {
	eRenderpass_Clear_None = 0x00,
	eRenderpass_Clear_Color_Buffer = 0x01,
	eRenderpass_Clear_Depth_Buffer = 0x02,
	eRenderpass_Clear_Stencil_Buffer = 0x04
};

struct RenderpassConfig {
	const char* name = nullptr;

	float depth;
	uint32_t stencil;

	Vec4 render_area;
	Vec4 clear_color;

	unsigned char clear_flags;

	unsigned char renderTargetCount;
	struct RenderTargetConfig target;
};

class IRenderpass {
public:
	virtual bool Create(VulkanContext* context, const RenderpassConfig* config) = 0;
	virtual void Destroy(VulkanContext* context) = 0;

	virtual void Begin(struct RenderTarget* target) = 0;
	virtual void End() = 0;

public:
	void SetRenderArea(const Vec4& area) { RenderArea = area; }
	const Vec4& GetRenderArea() const { return RenderArea; }

	void SetClearColor(const Vec4& col) { ClearColor = col; }
	const Vec4& GetClearColor() const { return ClearColor; }

	void SetClearFlags(unsigned char val) { ClearFlags = val; }
	unsigned char GetClearFlags() const { return ClearFlags; }

	void SetID(unsigned short i) { ID = i; }
	unsigned short GetID() const { return ID; }

public:
	unsigned char RenderTargetCount;
	std::vector<struct RenderTarget> Targets; 
	void* Renderpass = nullptr;

protected:
	unsigned short ID;
	Vec4 RenderArea;
	Vec4 ClearColor;
	unsigned char ClearFlags;

};
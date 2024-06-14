#pragma once

#include <vulkan/vulkan.hpp>
#include "Renderer/RendererTypes.hpp"

#define  VULKAN_MAX_REGISTERED_RENDERPASSES 31

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
	const char* name;
	const char* prev_name;
	const char* next_name;
	Vec4 render_area;
	Vec4 clear_color;

	unsigned char clear_flags;
};

class IRenderpass {
public:
	virtual void Create(VulkanContext* context,
		float depth, uint32_t stencil,
		bool has_prev_pass, bool has_next_pass) = 0;
	virtual void Destroy(VulkanContext* context) = 0;

	virtual void Begin(RenderTarget* target) = 0;
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
	std::vector<RenderTarget> Targets;
	void* Renderpass = nullptr;

protected:
	unsigned short ID;
	Vec4 RenderArea;
	Vec4 ClearColor;
	unsigned char ClearFlags;

};
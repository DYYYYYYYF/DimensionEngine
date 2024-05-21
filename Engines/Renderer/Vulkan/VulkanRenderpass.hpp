#pragma once

#include "vulkan/vulkan.hpp"
#include "Math/MathTypes.hpp"

enum RenderpassClearFlags {
	eRenderpass_Clear_None = 0x00,
	eRenderpass_Clear_Color_Buffer = 0x01,
	eRenderpass_Clear_Depth_Buffer = 0x02,
	eRenderpass_Clear_Stencil_Buffer = 0x04
};

enum VulkanRenderPassState {
	eRenderPass_State_Ready,
	eRenderPass_State_Recording,
	eRenderPass_State_In_Renderpass,
	eRenderPass_State_Recording_Ended,
	eRenderPass_State_Submited,
	eRenderPass_State_Not_Allocated
};

class VulkanContext;
class VulkanCommandBuffer;

class VulkanRenderPass {
public:
	VulkanRenderPass() {}
	virtual ~VulkanRenderPass() {}

public:
	void Create(VulkanContext* context,
		Vec4 render_area, Vec4 clear_color,
		float depth, uint32_t stencil,
		int clear_flags,
		bool has_prev_pass, bool has_next_pass);
	void Destroy(VulkanContext* context);

	void Begin(VulkanCommandBuffer* command_buffer, vk::Framebuffer frame_buffer);
	void End(VulkanCommandBuffer* command_buffer);

	vk::RenderPass GetRenderPass() { return RenderPass; }

public:
	void SetW(float w) { RenderArea.z = w; }
	void SetH(float h) { RenderArea.w = h; }

	void SetX(float x) { RenderArea.x = x; }
	void SetY(float y) { RenderArea.y = y; }

private:
	vk::RenderPass RenderPass;
	Vec4 RenderArea;
	Vec4 ClearColor;

	float Depth;
	uint32_t Stencil;

	bool HasPrevPass;
	bool HasNextPass;
	
	VulkanRenderPassState State;
	int ClearFlags;
};
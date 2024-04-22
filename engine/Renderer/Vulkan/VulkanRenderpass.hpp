#pragma once

#include "vulkan/vulkan.hpp"

enum VulkanRenderPassState {
	eRenderPass_State_Ready,
	eRenderPass_State_Recording,
	eRenderPass_State_In_Renderpass,
	eRenderPass_State_Recording_Ended,
	eRenderPass_State_Submited,
	eRenderPass_State_Not_Allocated
};

enum VulkanCommandBufferState {
	eCommand_Buffer_State_Ready,
	eCommand_Buffer_State_Recording,
	eCommand_Buffer_State_In_Renderpass,
	eCommand_Buffer_State_Recording_Ended,
	eCommand_Buffer_State_Submited,
	eCommand_Buffer_State_Not_Allocated
};

struct VulkanCommandBuffer {
	vk::CommandBuffer CommandBuffer;

	VulkanCommandBufferState State;
};

class VulkanContext;

class VulkanRenderPass {
public:
	VulkanRenderPass() {}
	virtual ~VulkanRenderPass() {}

public:
	void Create(VulkanContext* context,
		float x, float y, float w, float h,
		float r, float g, float b, float a,
		float depth, uint32_t stencil);
	void Destroy(VulkanContext* context);

	void Begin(VulkanCommandBuffer* command_buffer, vk::Framebuffer* frame_buffer);
	void End(VulkanCommandBuffer* command_buffer);

private:
	vk::RenderPass RenderPass;
	float X, Y, W, H;
	float R, G, B, A;

	float Depth;
	uint32_t Stencil;

	VulkanRenderPassState State;

};
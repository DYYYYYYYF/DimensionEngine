#pragma once

#include <vulkan/vulkan.hpp>

enum VulkanCommandBufferState {
	eCommand_Buffer_State_Ready,
	eCommand_Buffer_State_Recording,
	eCommand_Buffer_State_In_Renderpass,
	eCommand_Buffer_State_Recording_Ended,
	eCommand_Buffer_State_Submited,
	eCommand_Buffer_State_Not_Allocated
};

class VulkanContext;

class VulkanCommandBuffer {
public:
	void Allocate(VulkanContext* context, vk::CommandPool pool, bool is_primary);
	void Free(VulkanContext* context, vk::CommandPool pool);

	void BeginCommand(bool is_single_use, bool is_renderpass_continue, bool is_synchronized);
	void EndCommand();

	void UpdateSubmitted();
	void Reset();

	/*
	*  Allocates and begins recording to Out_Command_Buffer
	*/
	void AllocateAndBeginSingleUse(VulkanContext* context, vk::CommandPool pool);

	/*
	*  End recording, submits to and waits for queue operation and frees the provided command buffer
	*/
	void EndSingleUse(VulkanContext* context, vk::CommandPool pool, vk::Queue queue);

public:
	vk::CommandBuffer CommandBuffer;
	VulkanCommandBufferState State;

};
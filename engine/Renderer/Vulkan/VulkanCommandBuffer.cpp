#include "VulkanCommandBuffer.hpp"
#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void VulkanCommandBuffer::Allocate(VulkanContext* context, vk::CommandPool pool, bool is_primary) {
	vk::CommandBufferAllocateInfo AllocateInfo;
	AllocateInfo.setCommandPool(pool)
		.setLevel(is_primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary)
		.setCommandBufferCount(1)
		.setPNext(nullptr);

	State = eCommand_Buffer_State_Not_Allocated;
	CommandBuffer = context->Device.GetLogicalDevice().allocateCommandBuffers(AllocateInfo)[0];
	State = eCommand_Buffer_State_Ready;
}

void VulkanCommandBuffer::Free(VulkanContext* context, vk::CommandPool pool) {
	context->Device.GetLogicalDevice().freeCommandBuffers(pool, CommandBuffer);
	CommandBuffer = nullptr;
	State = eCommand_Buffer_State_Not_Allocated;
}

void VulkanCommandBuffer::BeginCommand(bool is_single_use, bool is_renderpass_continue, bool is_synchronized) {
	vk::CommandBufferBeginInfo BeginInfo;
	if (is_single_use) {
		BeginInfo.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	}
	if (is_renderpass_continue) {
		BeginInfo.flags |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;
	}
	if (is_synchronized) {
		BeginInfo.flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
	}

	CommandBuffer.begin(BeginInfo);
	State = eCommand_Buffer_State_Recording;
}

void VulkanCommandBuffer::EndCommand() {
	CommandBuffer.end();
	State = eCommand_Buffer_State_Recording_Ended;
}

void VulkanCommandBuffer::UpdateSubmitted() {
	State = eCommand_Buffer_State_Submited;
}

void VulkanCommandBuffer::Reset() {
	State = eCommand_Buffer_State_Ready;
}

void VulkanCommandBuffer::AllocateAndBeginSingleUse(VulkanContext* context, vk::CommandPool pool) {
	Allocate(context, pool, true);
	BeginCommand(true, false, false);
}

void VulkanCommandBuffer::EndSingleUse(VulkanContext* context, vk::CommandPool pool, vk::Queue queue) {
	// End command buffer
	EndCommand();

	// Submit queue
	vk::SubmitInfo SubmitInfo;
	SubmitInfo.setCommandBufferCount(1)
		.setCommandBuffers(CommandBuffer);
	if (queue.submit(1, &SubmitInfo, nullptr) != vk::Result::eSuccess) {
		UL_ERROR("Can not submit command.");
		return;
	}

	// Wait for it to finish
	queue.waitIdle();

	// Free the Command Buffer
	Free(context, pool);
}
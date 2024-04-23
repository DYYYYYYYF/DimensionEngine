#include "VulkanFence.hpp"
#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void VulkanFence::Create(VulkanContext* context, bool signaled) {
	//Make sure to signal the fence if required
	is_signaled = signaled;
	
	vk::FenceCreateInfo FenceCreateInfo;
	if (is_signaled) {
		FenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
	}

	fence = context->Device.GetLogicalDevice().createFence(FenceCreateInfo, context->Allocator);
	ASSERT(fence);
}

void VulkanFence::Destroy(VulkanContext* context) {
	if (fence) {
		context->Device.GetLogicalDevice().destroyFence(fence, context->Allocator);
	}

	is_signaled = false;
}

bool VulkanFence::Wait(VulkanContext* context, size_t timeout_ns) {
	if (!is_signaled) {
		vk::Result Result = context->Device.GetLogicalDevice().waitForFences(1, &fence, true, timeout_ns);

		switch (Result)
		{
		case vk::Result::eSuccess:
			is_signaled = true;
			return true;
		case vk::Result::eTimeout:
			UL_WARN("Wait fence: time out.");
			break;
		case vk::Result::eErrorDeviceLost:
			UL_ERROR("Wait fence: Device lost.");
			break;
		case vk::Result::eErrorOutOfHostMemory:
			UL_ERROR("Wait fence: Out of host memory.");
			break;
		case vk::Result::eErrorOutOfDeviceMemory:
			UL_ERROR("Wait fence: Out of device memory.");
			break;
		default:
			UL_ERROR("Wait fence: Unknow error.");
			break;
		}
			return false;
	}
	else {
		return true;
	}

	return false;
}

void VulkanFence::Reset(VulkanContext* context) {
	if (is_signaled) {
		if (context->Device.GetLogicalDevice().resetFences(1, &fence) != vk::Result::eSuccess) {
			UL_ERROR("Reset fence error.");
			return;
		}
		is_signaled = false;
	}
}

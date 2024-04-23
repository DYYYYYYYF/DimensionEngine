#include "VulkanSwapchain.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "VulkanContext.hpp"

void VulkanSwapchain::Create(VulkanContext* context, unsigned int width, unsigned int height) {
	vk::Extent2D SwapchainExtent = { width, height };
	MaxFramesInFlight = 2;

	// Choose a swap surface format.
	bool Found = false;
	SSwapchainSupportInfo* SupportInfo = context->Device.GetSwapchainSupportInfo();
	for (uint32_t i = 0; i < SupportInfo->format_count; ++i) {
		vk::SurfaceFormatKHR Format = SupportInfo->formats[i];
		// Preferred formats
		if (Format.format == vk::Format::eB8G8R8A8Unorm && Format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			ImageFormat = Format;
			Found = true;
			break;
		}
	}

	if (!Found) {
		ImageFormat = SupportInfo->formats[0];
	}

	vk::PresentModeKHR PresentMode = vk::PresentModeKHR::eFifo;
	for (uint32_t i = 0; i < SupportInfo->present_mode_count; ++i) {
		vk::PresentModeKHR Mode = SupportInfo->present_modes[i];
		if (Mode == vk::PresentModeKHR::eMailbox) {
			PresentMode = Mode;
			break;
		}
	}

	// Requery swapchain support.
	context->Device.QuerySwapchainSupport(context->Device.GetPhysicalDevice(), context->Surface, context->Device.GetSwapchainSupportInfo());

	if (SupportInfo->capabilities.currentExtent.width != UINT32_MAX) {
		SwapchainExtent = SupportInfo->capabilities.currentExtent;
	}

	// Clamp to the value allowed by the GPU
	vk::Extent2D MinExtent = SupportInfo->capabilities.minImageExtent;
	vk::Extent2D MaxExtent = SupportInfo->capabilities.maxImageExtent;
	SwapchainExtent.width = CLAMP(SwapchainExtent.width, MinExtent.width, MaxExtent.width);
	SwapchainExtent.height = CLAMP(SwapchainExtent.height, MinExtent.height, MaxExtent.height);

	uint32_t MinImageCount = SupportInfo->capabilities.minImageCount + 1;
	if (SupportInfo->capabilities.maxImageCount > 0 && MinImageCount > SupportInfo->capabilities.maxImageCount) {
		MinImageCount = SupportInfo->capabilities.maxImageCount;
	}

	// Swapchain create info
	vk::SwapchainCreateInfoKHR SwapchainCreateInfo;
	SwapchainCreateInfo.setSurface(context->Surface)
		.setMinImageCount(MinImageCount)
		.setImageFormat(ImageFormat.format)
		.setImageColorSpace(ImageFormat.colorSpace)
		.setImageExtent(SwapchainExtent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

	// Setup the queue family indices
	SVulkanPhysicalDeviceQueueFamilyInfo* QueueFamilyInfo = context->Device.GetQueueFamilyInfo();
	if (QueueFamilyInfo->graphics_index != QueueFamilyInfo->present_index) {
		uint32_t QueueFamilyIndices[] = {
			(uint32_t)QueueFamilyInfo->graphics_index,
			(uint32_t)QueueFamilyInfo->present_index
		};
		SwapchainCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndexCount(2)
			.setQueueFamilyIndices(QueueFamilyIndices);
	}
	else {
		SwapchainCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndexCount(0)
			.setQueueFamilyIndices({});
	}

	SwapchainCreateInfo.setPreTransform(SupportInfo->capabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(PresentMode)
		.setClipped(VK_TRUE)
		.setOldSwapchain({});

	vk::Device LogicalDevice = context->Device.GetLogicalDevice();
	Handle = LogicalDevice.createSwapchainKHR(SwapchainCreateInfo, context->Allocator);
	ASSERT(Handle);

	// Start with a zero frame index.
	context->CurrentFrame = 0;

	// Images
	ImageCount = 0;
	if (LogicalDevice.getSwapchainImagesKHR(Handle, &ImageCount, 0) != vk::Result::eSuccess) {
		UL_INFO("Get swapchain images failed.");
		return;
	}

	if (Images == nullptr) {
		Images = (vk::Image*)Memory::Allocate(sizeof(vk::Image) * ImageCount, MemoryType::eMemory_Type_Renderer);
	}

	if (ImageViews == nullptr) {
		ImageViews = (vk::ImageView*)Memory::Allocate(sizeof(vk::ImageView) * ImageCount, MemoryType::eMemory_Type_Renderer);
	}

	if (LogicalDevice.getSwapchainImagesKHR(Handle, &ImageCount, Images) != vk::Result::eSuccess) {
		UL_INFO("Get swapchain images failed.");
		return;
	}

	// Image views
	for (uint32_t i = 0; i < ImageCount; ++i) {
		vk::ImageSubresourceRange Range;
		Range.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);

		vk::ImageViewCreateInfo ViewInfo;
		ViewInfo.setImage(Images[i])
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(ImageFormat.format)
			.setSubresourceRange(Range);

		ImageViews[i] = LogicalDevice.createImageView(ViewInfo, context->Allocator);
		ASSERT(ImageViews[i]);
	}

	// Depth resources
	if (!context->Device.DetectDepthFormat()) {
		context->Device.SetDepthFormat(vk::Format::eUndefined);
		UL_FATAL("Failed to find a supported format!");
	}

	// Create depth image and view
	DepthAttachment.CreateImage(context, vk::ImageType::e2D, SwapchainExtent.width, SwapchainExtent.height, context->Device.GetDepthFormat(),
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, true, vk::ImageAspectFlagBits::eDepth);

	UL_INFO("Create swapchain successful.");
}

void VulkanSwapchain::Recreate(VulkanContext* context, unsigned int width, unsigned int height) {
	Destroy(context);
	Create(context, width, height);
}

bool VulkanSwapchain::Destroy(VulkanContext* context) {
	context->Device.GetLogicalDevice().waitIdle();
	DepthAttachment.Destroy(context);

	// Only destroy views, not the images, since those are owned by swapchain and are thus destroyed when it is.
	vk::Device LogicalDevice = context->Device.GetLogicalDevice();
	for (uint32_t i = 0; i < ImageCount; ++i) {
		LogicalDevice.destroyImageView(ImageViews[i], context->Allocator);
	}

	LogicalDevice.destroySwapchainKHR(Handle, context->Allocator);
	return false;
}

void VulkanSwapchain::Presnet(VulkanContext* context, vk::Queue graphics_queue, vk::Queue present_queue, vk::Semaphore render_complete_semaphore, uint32_t present_image_index) {
	// Return the image to the swapchain for presentation.
	vk::PresentInfoKHR PresentInfo;
	PresentInfo.setWaitSemaphoreCount(1)
		.setWaitSemaphores(render_complete_semaphore)
		.setSwapchainCount(1)
		.setSwapchains(Handle)
		.setImageIndices(present_image_index);

	vk::Result Result = present_queue.presentKHR(PresentInfo);
	if (Result == vk::Result::eErrorOutOfDateKHR || Result == vk::Result::eSuboptimalKHR) {
		Recreate(context, context->FrameBufferWidth, context->FrameBufferHeight);
	}
	else if (Result != vk::Result::eSuccess) {
		UL_FATAL("Failed to present swapchain image.");
	}

	// Increment (and loop) the index;
	context->CurrentFrame = (context->CurrentFrame + 1) % context->Swapchain.MaxFramesInFlight;
}

uint32_t VulkanSwapchain::AcquireNextImageIndex(VulkanContext* context, size_t timeout_ns, vk::Semaphore image_available_semaphore, vk::Fence fence) {

	vk::ResultValue<uint32_t> Result = context->Device.GetLogicalDevice().acquireNextImageKHR(Handle, timeout_ns, image_available_semaphore, fence);
	if (Result.result == vk::Result::eErrorOutOfDateKHR) {
		Recreate(context, context->FrameBufferWidth, context->FrameBufferHeight);
		return -1;
	}
	else if (Result.result != vk::Result::eSuccess && Result.result != vk::Result::eSuboptimalKHR) {
		UL_FATAL("Acquire swapchain image failed.");
		return -1;
	}

	return Result.value;
}
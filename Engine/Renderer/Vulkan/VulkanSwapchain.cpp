#include "VulkanSwapchain.hpp"
#include "VulkanContext.hpp"
#include "VulkanImage.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Systems/TextureSystem.h"

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
		SwapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(1)
			.setQueueFamilyIndices(QueueFamilyInfo->graphics_index);
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
		LOG_INFO("Get swapchain images failed.");
		return;
	}

	if (RenderTextures.size() == 0) {
		RenderTextures.resize(ImageCount);
		// If creating the array, then the internal texture objects aren't created yet either.
		for (uint32_t i = 0; i < ImageCount; ++i) {
			void* InternalData = Memory::Allocate(sizeof(VulkanImage), MemoryType::eMemory_Type_Texture);

			char TexName[38] = "__internal_vulkan_swapchain_image_0__";
			TexName[34] = '0' + (char)i;

			RenderTextures[i] = TextureSystem::WrapInternal(
				TexName,
				SwapchainExtent.width,
				SwapchainExtent.height,
				4,
				false,
				true,
				false,
				InternalData);
			if (!RenderTextures[i]) {
				LOG_FATAL("Failed to generate new swapchain image texture.");
				return;
			}
		}
	}
	else {
		for (uint32_t i = 0; i < ImageCount; ++i) {
			TextureSystem::Resize(RenderTextures[i], SwapchainExtent.width, SwapchainExtent.height, false);
		}
	}

	vk::Image SwapchainImages[32];
	if (LogicalDevice.getSwapchainImagesKHR(Handle, &ImageCount, SwapchainImages) != vk::Result::eSuccess) {
		LOG_INFO("Get swapchain images failed.");
		return;
	}

	for (uint32_t i = 0; i < ImageCount; ++i) {
		// Update the internal image for each.
		VulkanImage* Image = (VulkanImage*)RenderTextures[i]->InternalData;
		Image->Image = SwapchainImages[i];
		Image->Width = SwapchainExtent.width;
		Image->Height = SwapchainExtent.height;
	}

	// Image views
	for (uint32_t i = 0; i < ImageCount; ++i) {
		VulkanImage* Image = (VulkanImage*)RenderTextures[i]->InternalData;

		vk::ImageSubresourceRange Range;
		Range.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0) 
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);

		vk::ImageViewCreateInfo ViewInfo;
		ViewInfo.setImage(Image->Image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(ImageFormat.format)
			.setSubresourceRange(Range);

		Image->ImageView = LogicalDevice.createImageView(ViewInfo, context->Allocator);
		ASSERT(Image->ImageView);
	}

	// Depth resources
	if (!context->Device.DetectDepthFormat()) {
		context->Device.SetDepthFormat(vk::Format::eUndefined);
		LOG_FATAL("Failed to find a supported format!");
	};

	// Create depth image and view
	VulkanImage* DepthImage = (VulkanImage*)Memory::Allocate(sizeof(VulkanImage), MemoryType::eMemory_Type_Texture);
	DepthImage->CreateImage(context, TextureType::eTexture_Type_2D, SwapchainExtent.width, SwapchainExtent.height, context->Device.GetDepthFormat(),
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, true, vk::ImageAspectFlagBits::eDepth);

	// Wrap it in a texture.
	context->Swapchain.DepthTexture = TextureSystem::WrapInternal(
		"__default_depth_texture__",
		SwapchainExtent.width,
		SwapchainExtent.height,
		context->Device.GetDepthChannelCount(),
		false, true, false, DepthImage);

	LOG_INFO("Create swapchain successful.");
}

void VulkanSwapchain::Recreate(VulkanContext* context, unsigned int width, unsigned int height) {
	Destroy(context);
	Create(context, width, height);
}

bool VulkanSwapchain::Destroy(VulkanContext* context) {
	context->Device.GetLogicalDevice().waitIdle();
	((VulkanImage*)DepthTexture->InternalData)->Destroy(context);
	Memory::Free(DepthTexture->InternalData, sizeof(VulkanImage), MemoryType::eMemory_Type_Texture);
	DepthTexture->InternalData = nullptr;

	// Only destroy views, not the images, since those are owned by swapchain and are thus destroyed when it is.
	vk::Device LogicalDevice = context->Device.GetLogicalDevice();
	for (uint32_t i = 0; i < ImageCount; ++i) {
		VulkanImage* Image = (VulkanImage*)RenderTextures[i]->InternalData;
		LogicalDevice.destroyImageView(Image->ImageView, context->Allocator);
	}

	LogicalDevice.destroySwapchainKHR(Handle, context->Allocator);
	Handle = nullptr;

	return false;
}

void VulkanSwapchain::Presnet(VulkanContext* context, vk::Queue graphics_queue, vk::Queue present_queue, vk::Semaphore render_complete_semaphore, uint32_t present_image_index) {
	// Return the image to the swapcahin for presentation.
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
		LOG_FATAL("Failed to present swapchain image.");
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
		LOG_FATAL("Acquire swapchain image failed.");
		return -1;
	}

	return Result.value;
}